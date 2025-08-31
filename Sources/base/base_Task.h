#pragma once
#include "base/detail/base_detail_Promise.h"

// --------------------------------------------------------------

namespace base
{

// --------------------------------------------------------------

template <class ResultT = void>
struct task
{
public:
    using result_value  = ResultT;
    using result_type   = std::conditional_t<std::is_void_v<ResultT>, void, std::optional<ResultT>>;
    using promise_type  = detail::promise<task>;
    using handle_type   = std::coroutine_handle<promise_type>;

private:
    class awaiter;

public:
    task() noexcept;
    explicit task(handle_type h) noexcept;
    ~task() noexcept;

public:
    task(const task&)  noexcept             = delete;
    task& operator=(const task&) noexcept   = delete;
    task(task&& rhs) noexcept;
    task& operator=(task&& rhs) noexcept;

public:
    auto operator co_await()&&;

public:
    bool has_next() const;
    bool move_next();
    result_type consume_result();

private:
    handle_type m_Handle;
};

// --------------------------------------------------------------

template <class ResultT>
task<ResultT>::task() noexcept
    : m_Handle(nullptr)
{
}

template <class ResultT>
task<ResultT>::task(handle_type h) noexcept
    : m_Handle(h)
{
}

template <class ResultT>
task<ResultT>::task(task&& rhs) noexcept
    : m_Handle(std::exchange(rhs.m_Handle, {}))
{
}

template <class ResultT>
task<ResultT>& task<ResultT>::operator=(task<ResultT>&& rhs) noexcept
{
    if (this != &rhs)
    {
        if (m_Handle)
        {
            m_Handle.destroy();
        }

        m_Handle = std::exchange(rhs.m_Handle, {});
    }
    return *this;
}

template <class ResultT>
task<ResultT>::~task<ResultT>() noexcept
{
    if (m_Handle)
    {
        m_Handle.destroy();
    }
}

template <class ResultT>
bool task<ResultT>::has_next() const 
{
    return m_Handle && !m_Handle.done();
}

template <class ResultT>
bool task<ResultT>::move_next()
{ 
    if (has_next())
    {
        m_Handle.resume();
        return has_next(); // TODO: キャンセル時は終了扱いにすべき

    }
    return false;
}

template <class ResultT>
task<ResultT>::result_type task<ResultT>::consume_result()
{
    return m_Handle.promise().consume_result();
}

// --------------------------------------------------------------

// taskをawaitableにするためのawaiter
template <class ResultT>
class task<ResultT>::awaiter final
{
public:
    using task_type   = task<ResultT>;
    using result_type = task_type::result_type;

public:
    explicit awaiter(task_type&& task) noexcept
        : m_Task(std::move(task))
    {
    }
    ~awaiter() noexcept
    {
    }

    awaiter(const awaiter&)  noexcept           = delete;
    awaiter& operator=(const awaiter&) noexcept = delete;
    awaiter(awaiter&& rhs) noexcept
        : m_Task(std::move(rhs.m_Task))
    {
    }
    awaiter& operator=(awaiter&&) noexcept = delete;

public:
    bool await_ready() { return !m_Task.has_next(); }

    template <class ReturnObjectT>
    auto await_suspend(std::coroutine_handle<detail::promise<ReturnObjectT>> hOuterCoroutine)
    {
        const auto hInnerCoroutine = m_Task.m_Handle;
        hInnerCoroutine.promise().set_continuation(hOuterCoroutine);
        return hInnerCoroutine;
    }

    result_type await_resume()
    {
        return m_Task.consume_result();
    }

private:
    task_type m_Task;
};

// --------------------------------------------------------------

// taskをawaitableにするためのoperator co_await

template <class ResultT>
auto task<ResultT>::operator co_await()&&
{
    return awaiter{ std::move(*this) };
}
// --------------------------------------------------------------

} // namespace base
