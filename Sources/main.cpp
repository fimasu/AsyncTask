#include "base/base_Task.h"
#include "base/base_TaskAwaiter.h"

base::task<int> child_func(int a)
{
    std::cout << std::format("child_func({})\n", a);
    std::cout << std::format("child_func : co_return ({})\n", a + 1);
    co_return (a + 1);
}

base::task<int> root_func(int a, int b) 
{ 
    std::cout << std::format("root_func({}, {})\n", a,  b);
    int result = a;
    for(int i = 0; i < b; ++i)
    {
        result += co_await child_func(result);
    }
    std::cout << std::format("root_func : co_return ({})\n", result);
    co_return result;
}

int main()
{
    auto root_coro = root_func(2, 4);
    const bool move_next_result = root_coro.move_next();
    std::cout << "move_next : " << move_next_result << std::endl;
    std::cout << "result :" << root_coro.consume_result() << std::endl;
}
