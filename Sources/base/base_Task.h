#pragma once
#include "base/base_Promise.h"

// --------------------------------------------------------------

namespace base
{

// --------------------------------------------------------------

template <class ResultT = void>
struct task
{
public:
    using result_type   = ResultT;
    using promise_type  = promise<task>;
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
    bool done() const { return !m_Handle || m_Handle.done(); }
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
task<ResultT>::result_type task<ResultT>::consume_result()
{
    return m_Handle.promise().consume_result();
}

// --------------------------------------------------------------

} // namespace base