#pragma once

namespace base
{

// --------------------------------------------------------------

struct task
{
public:
    struct promise_type;
    using handle = std::coroutine_handle<promise_type>;

public:
    task() noexcept;
private:
    task(handle h) noexcept;
public:
    ~task() noexcept;

public:
    task(const task&)  noexcept = delete;
    task& operator=(const task&) noexcept = delete;
    task(task&& rhs) noexcept;
    task& operator=(task&& rhs) noexcept;


public:
    bool move_next();
    int current_value();

private:
    handle m_Handle;
};

// --------------------------------------------------------------

struct task::promise_type
{
    int current_value;
    static auto get_return_object_on_allocation_failure() { return task{ nullptr }; }
    auto get_return_object() { return task{ handle::from_promise(*this) }; }
    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    void unhandled_exception() { std::terminate(); }
    void return_void() {}
    auto yield_value(int value)
    {
        current_value = value;
        return std::suspend_always{};
    }
};

// --------------------------------------------------------------

task::task() noexcept
    : m_Handle(nullptr)
{
}

task::task(handle h) noexcept
    : m_Handle(h)
{
}

task::task(task&& rhs) noexcept
    : m_Handle(rhs.m_Handle)
{
    rhs.m_Handle = nullptr;
}

task& task::operator=(task&& rhs) noexcept
{
    if (this != &rhs)
    {
        if (m_Handle) m_Handle.destroy();
        m_Handle = rhs.m_Handle;
        rhs.m_Handle = nullptr;
    }
    return *this;
}

task::~task() noexcept 
{
    if (m_Handle) m_Handle.destroy();
}

bool task::move_next() 
{ 
    if (m_Handle)
    {
        m_Handle.resume();
        return !m_Handle.done();

    }
    return false;
}

int task::current_value() 
{
    return m_Handle.promise().current_value;
}

// --------------------------------------------------------------

} // namespace base