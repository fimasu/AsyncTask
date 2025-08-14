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
    using task_type     = task<ResultT>;

public:
    explicit task_awaiter(task_type&& task) noexcept
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
    bool await_ready() { return m_Task.done(); }

    template <class ReturnObjectT>
    bool await_suspend(std::coroutine_handle<detail::promise<ReturnObjectT>> hOuterCoroutine)
    {
        resume();
        if (m_Task.done())
        {
            // フレーム待ちなしで終了した場合は登録不要
            return false;
        }
        // このハンドルはco_awaitがある親コルーチン側のもの。
        // thisはco_awaitで変換された子コルーチン側のAwaiterである
        // ここでハンドルをセットすることで、親コルーチンが再開されたときに
        // 子コルーチンのawait_resume()が呼ばれるようになる
        hOuterCoroutine.promise().bind_sub_awaiter(this);
        return true;
    }

    ResultT await_resume()
    {
        return m_Task.consume_result();
    }

public:
    void resume() override
    {
        m_Task.move_next();
    }

    bool done() const override
    {
        return !m_Task.done();
    }

private:
    task_type m_Task;
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