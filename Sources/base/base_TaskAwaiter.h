#pragma once
#include "base/base_IAwaiter.h"
#include "base/base_Task.h"

// --------------------------------------------------------------

namespace base
{

// --------------------------------------------------------------

// taskをawaitableにするためのawaiter
template <class ResultT = void>
class task_awaiter : public ::base::IAwaiter
{
public:
    using result_type   = ResultT;
    using task_tape     = task<ResultT>;

public:
    explicit task_awaiter(task_tape&& task) noexcept
        : m_Task(std::move(task))
    {
    }
    ~task_awaiter() noexcept
    {
    }

    task_awaiter(const task_awaiter&)  noexcept           = delete;
    task_awaiter& operator=(const task_awaiter&) noexcept = delete;
    task_awaiter(task_awaiter&& rhs) noexcept
        : m_Task(std::move(rhs.m_Task))
    {
    }
    task_awaiter& operator=(task_awaiter&&) noexcept      = delete;

public:
    void resume() override
    {
        if (m_Task.move_next())
        {
            // If the task has more work to do, we can resume it.
            return;
        }
        // If the task is done, we can handle the result or cleanup.
    }

    bool done() const override
    {
        return !m_Task.move_next();
    }

private:
    task_tape m_Task;
};

// --------------------------------------------------------------

// taskをawaitableにするためのoperator co_await
template <class ResultT>
auto operator co_await(task<ResultT>&& task)
{
    return task_awaiter<ResultT>(std::move(task));
}

// --------------------------------------------------------------

} // namespace base