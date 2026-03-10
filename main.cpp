#include "demo/bank_demo.hpp"
#include "demo/bank_demo_raw.hpp"



int main()
{
    // alternate implementation that demonstrates that `Menu` works standalone
    // with any custom navigation structure; uses a raw `std::stack<std::string>`
    // to manually track and switch menus
    // run_bank_demo_raw();

    // production entry point; uses `MenuNavigator` which wraps the `std::stack`
    // and abstracts away the manual navigation logic for the menus
    run_bank_demo();

    return 0;
}
