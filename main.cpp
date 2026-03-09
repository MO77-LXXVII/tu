#include "bank_demo.hpp"
#include "bank_demo_raw.hpp"

int main()
{
    // an extra implementation that demonstrates `Menu` works standalone with any custom
    // navigation structure; uses a raw `std::stack<std::string>` to manually track and switch between menus
    // run_bank_demo_raw();

    // production entry point; uses `MenuNavigator` which wraps the `std::stack`
    // and abstracts away the navigation logic for the menus
    run_bank_demo();

    return 0;
}
