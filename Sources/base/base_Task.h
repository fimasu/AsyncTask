#pragma once

// --------------------------------------------------------------

namespace base
{
template <class ResultT>
struct promise;
}

// --------------------------------------------------------------

namespace base
{

// --------------------------------------------------------------

template <class ResultT = void>
struct task
{
public:
    using result_type   = ResultT;
    using promise_type  = promise<result_type>;
    using handle_type   = std::coroutine_handle<promise_type>;

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
    bool move_next();
    int current_value();

private:
    handle_type m_Handle;
};

// --------------------------------------------------------------

template <class ResultT>
struct promise
{
    using task_type     = task<ResultT>;
    using handle_type   = task_type::handle_type;

    static auto get_return_object_on_allocation_failure() { return task_type{ nullptr }; }
    auto get_return_object() { return task_type{ handle_type::from_promise(*this) }; }
    
    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    
    void unhandled_exception() { std::terminate(); }
    
    void return_void() {}
    auto yield_value(int value)
    {
        current_value = value;
        return std::suspend_always{};
    }

    int current_value;
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
    : m_Handle(rhs.m_Handle)
{
    rhs.m_Handle = nullptr;
}

template <class ResultT>
task<ResultT>& task<ResultT>::operator=(task<ResultT>&& rhs) noexcept
{
    if (this != &rhs)
    {
        if (m_Handle) m_Handle.destroy();
        m_Handle = rhs.m_Handle;
        rhs.m_Handle = nullptr;
    }
    return *this;
}

template <class ResultT>
task<ResultT>::~task<ResultT>() noexcept
{
    if (m_Handle) m_Handle.destroy();
}

template <class ResultT>
bool task<ResultT>::move_next()
{ 
    if (m_Handle)
    {
        m_Handle.resume();
        return !m_Handle.done();

    }
    return false;
}

template <class ResultT>
int task<ResultT>::current_value()
{
    return m_Handle.promise().current_value;
}

// --------------------------------------------------------------

} // namespace base