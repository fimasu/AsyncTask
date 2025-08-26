#include "base/base_Task.h"

base::task<int> child_func(int a)
{
    std::cout << std::format("child_func({})\n", a);

    if(a >= 3)
    {
        std::cout << "[child_func] co_yield base::cancellation\n";
        co_yield base::cancellation;
    }
    std::cout << std::format("[child_func] co_return ({} + 3)\n", a);
    co_return (a + 3);
}

base::task<int> root_func(int a, int b) 
{ 
    std::cout << std::format("root_func({}, {})\n", a,  b);
    int result = a;
    for(int i = 0; i < b; ++i)
    {
        const auto val = co_await child_func(result);
        if (val.has_value())
        {
            std::cout << "[root_func] child_func returned : " << val.value() << "\n";
            result += val.value();
        }
        else
        {
            std::cout << "[root_func] child_func returned no value\n";
        }
    }

    if (result >= 5)
    {
        std::cout << "[root_func] co_yield base::cancellation\n";
        co_yield base::cancellation;
    }

    std::cout << std::format("[root_func] co_return ({})\n", result);
    co_return result;
}

int main()
{
    auto root_coro = root_func(-2, 5);

    std::cout << "[main] initial state\n";
    const bool move_next_result = root_coro.move_next();

    std::cout << "[main] move_next : " << std::boolalpha << move_next_result << std::endl;

    const auto result = root_coro.consume_result();
    if(result.has_value())
    {
        std::cout << "[main] result value :" << result.value() << std::endl;
    }
    else
    {
        std::cout << "[main] result has no value" << std::endl;
    }
}
