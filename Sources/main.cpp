#include "base/base_Task.h"

base::task<int> f() 
{ 
    co_yield 1;
    co_yield 2;
}

int main()
{
    auto g = f();
    while (g.move_next()) std::cout << g.current_value() << std::endl;
}
