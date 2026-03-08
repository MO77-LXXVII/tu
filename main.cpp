#include <iostream>
#include <iomanip>
#include <limits>
#include <cassert>
#include <vector>
#include <stack>
#include <string>

#include "platform/platform.hpp"
#include "terminal_utils/ansi_colours.hpp"
#include "terminal_utils/style_wrappers.hpp"
#include "terminal_utils/input.hpp"
#include "terminal_utils/output.hpp"
#include "terminal_utils/logger.hpp"

#include "terminal_utils/menu/menu.hpp"
#include "terminal_utils/menu/menu_navigator.hpp"

#include "bank/bank_client.hpp"
#include "bank/bank_user.hpp"
#include "bank/currency_exchange.hpp"

namespace tu = terminal_utils;
// using namespace tu;

using namespace std;

struct Account
{
    std::string name;
    double balance = 0.0;
};

/**
 * @brief Returns a visibility checker for the given permission.
 * 
 * Usage:
 *   .add_item("Deposit", [&]{ ... }, bank::visible_if(Permission::Deposit))
 */
inline auto visible_if(Permission required, const BankUser& user)
{
    return[user = std::move(user), required]() -> bool
    {
        return has_permission(user.get_permissions(), required);
    };
}

/**
 * @brief Curry-style: bind the user first, then use for multiple items.
 * 
 * Usage:
 *   auto perm = bank::for_user(current_user);
 *   .add_item("Deposit", [&]{ ... }, perm(Permission::Deposit))
 */
inline auto for_user(const BankUser& user)
{
    return [&user](Permission required) {
        return [&user, required]() -> bool {
            return has_permission(user.get_permissions(), required);
        };
    };
}


// void run_bank2()
// {
//     using namespace terminal_utils;

//     // Data
//     // create dummy user
//     BankUser bank_user{};

//     // login with a specific user
//     bool running = bank_user.login();

//     const std::string USERNAME = bank_user.get_username();
//     LOG_INFO("User (" + USERNAME + ") logged into the system");

//     auto perm = for_user(bank_user);

//     // bank_user.print_user_details();

//     std::stack<std::string> menu_stack;
//     menu_stack.push("main");

//     // Main loop
//     while(running && !menu_stack.empty())
//     {
//         std::string current = menu_stack.top();

//         // === MAIN MENU ===
//         if(current == "main")
//         {
//             auto result = Menu::create("Bank System")
//                 .set_width(40)
//                 .set_date()
//                 .add_global_subtitle("Logged in as: " + USERNAME)
//                 .add_global_subtitle("Role: IDK")
//                 // .reset_subtitles()
//                 .add_item("Accounts", [&]{ menu_stack.push("accounts"); })
//                 .add_item("Manage Clients", [&]{ menu_stack.push("Clients"); }, perm(Permission::Deposit))
//                 .add_item("Manage Users", [&]{ menu_stack.push("Users"); }, perm(Permission::Withdraw))
//                 .add_item("Transactions", [&]{ menu_stack.push("transactions"); })
//                 .add_item("Settings", [&]{ menu_stack.push("settings"); })
//                 .add_separator()
//                 .add_item("Logout", [&]
//                 { 
//                     // terminates the menu
//                     running = false;

//                     // remove the current user data from memory
//                     bank_user.logout();

//                     LOG_INFO("User (" + USERNAME + ") logged out of the system");
//                 })
//                 .run();

//             // q on main menu = quit
//             if(result == MenuResult::Cancelled)
//             {
//                 // remove the current user data from memory
//                 bank_user.logout();

//                 LOG_INFO("User (" + USERNAME + ") logged out of the system");

//                 running = false;
//             }
//         }

//         // === ACCOUNTS MENU ===
//         else if (current == "accounts")
//         {
//             auto result = Menu::create("Accounts")
//                 .add_subtitle("bombastic")
//                 .add_item("Deposit", [&]
//                 {
//                     BankClient::deposit();
//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::Deposit))
//                 .add_item("Withdraw", [&]
//                 {
//                     BankClient::withdraw();
//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::Withdraw))
//                 .add_separator()
//                 .add_item("Back to Main", [&]
//                 {
//                     menu_stack.pop();
//                 })
//                 .run();

//             if (result == MenuResult::Cancelled)
//                 menu_stack.pop(); // ESC = back
//         }

//         // === CLIENTS MENU ===
//         else if (current == "Clients")
//         {
//             auto result = Menu::create("Accounts")
//                 .add_item("Show Clients List", [&]
//                 {
//                     platform::clear_terminal();

//                     BankClient::list_clients(); 

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::ShowClients))
//                 .add_item("Add Client", [&]
//                 {
//                     platform::clear_terminal();

//                     BankClient::add_client();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::AddClient))
//                 .add_item("Update Client", [&]
//                 {
//                     platform::clear_terminal();

//                     BankClient::update_client();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::UpdateClient))
//                 .add_item("Delete Client", [&]
//                 {
//                     platform::clear_terminal();

//                     BankClient::delete_client();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::DeleteClient))
//                 .add_item("Find Client", [&]
//                 {
//                     platform::clear_terminal();

//                     BankClient found_user = BankClient::find(BankClient::get_valid_account_num());

//                     found_user.print_client_details();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::FindClient))
//                 .add_separator()
//                 .add_item("Back to Main", [&]
//                 {
//                     menu_stack.pop();
//                 })
//                 .run();

//             if(result == MenuResult::Cancelled)
//                 menu_stack.pop(); // ESC = back
//         }

//         // === USERS MENU ===
//         else if (current == "Users")
//         {
//             auto result = Menu::create("Users")
//                 .add_item("Show Users List", [&]
//                 {
//                     platform::clear_terminal();

//                     BankUser::list_users(); 

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::ShowUsers))
//                 .add_item("Add User", [&]
//                 {
//                     platform::clear_terminal();

//                     BankUser::add_user();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::AddUser))
//                 .add_item("Update User", [&]
//                 {
//                     platform::clear_terminal();

//                     BankUser::update_user();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::UpdateUser))
//                 .add_item("Delete User", [&]
//                 {
//                     platform::clear_terminal();

//                     BankUser::delete_user();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::DeleteUser))
//                 .add_item("Find User", [&]
//                 {
//                     platform::clear_terminal();

//                     BankUser found_user = BankUser::find(BankUser::get_valid_user_name());

//                     found_user.print_user_details();

//                     input::get_menu_key(dim("Press Enter..."));
//                 }, perm(Permission::FindUser))
//                 .add_separator()
//                 .add_item("Back to Main", [&]
//                 {
//                     menu_stack.pop();
//                 })
//                 .run();

//             if(result == MenuResult::Cancelled)
//                 menu_stack.pop(); // ESC = back
//         }

//         // === TRANSACTIONS MENU ===
//         else if (current == "transactions")
//         {
//             auto result = Menu::create("Transactions")
//                 .add_item("Transfer", [&]{ menu_stack.push("transfer"); })
//                 .add_item("History", [&]{
//                     std::cout << yellow("\nNo history yet\n");
//                 })
//                 .add_separator()
//                 .add_item("Back to Main", [&]{ menu_stack.pop(); })
//                 .run();

//             if (result == MenuResult::Cancelled)
//                 menu_stack.pop();
//         }

//         // === TRANSFER SUB-MENU (3 levels deep) ===
//         else if (current == "transfer")
//         {
//             auto result = Menu::create("Transfer Money")
//                 .add_item("To Internal Account", [&]{
//                     BankClient::transfer();
//                     input::get_menu_key(dim("Press Enter..."));
//                 })
//                 .add_item("To External Account", nullptr, [&]{ return false; }) // totally YAGNI compliant :P
//                 .add_separator()
//                 .add_item("Back to Transactions", [&]{ menu_stack.pop(); })
//                 .add_item("Back to Main", [&]{ 
//                     // Pop twice: transfer -> transactions -> main
//                     menu_stack.pop(); 
//                     menu_stack.pop(); 
//                 })
//                 .run();

//             if (result == MenuResult::Cancelled)
//                 menu_stack.pop();
//         }

//         // === SETTINGS MENU ===
//         else if (current == "settings")
//         {
//             auto result = Menu::create("Settings")
//                 .add_item("Change PIN", [&]{ std::cout << "PIN changed\n"; })
//                 .add_item("Language", [&]{ std::cout << "Language set\n"; })
//                 .add_separator()
//                 .add_item("Back to Main", [&]{ menu_stack.pop(); })
//                 .run();

//             if (result == MenuResult::Cancelled)
//                 menu_stack.pop();
//         }
//     }

//     // bank_user.print_user_details();
// }

void run_bank()
{
    using namespace terminal_utils;

    BankUser bank_user{};

    if(!bank_user.login())
        return;

    const std::string USERNAME = bank_user.get_username();
    LOG_INFO("User (" + USERNAME + ") logged into the system");

    auto perm = for_user(bank_user);

    MenuNavigator nav;

    /*
    * ------------------------------------------
    ! ------------------------------------------
    ! ------------------------------------------
    ! ------------------------------------------
        Supporting enums would require templates
        in `nav.add("main", [&](MenuNavigator& n)`
        it only accepts std::string as of now
    */
    enum class MenuID { Main, Accounts, Clients };

    // === MAIN MENU ===
    nav.add("main", [&](MenuNavigator& n)
    {
        return Menu::create("Bank System")
            .set_width(40)
            .set_date()
            .add_global_subtitle("Logged in as: " + USERNAME)
            .add_item("Accounts",       [&]{ n.push("accounts"); })
            .add_item("Manage Clients", [&]{ n.push("clients"); },      perm(Permission::Deposit))
            .add_item("Manage Users",   [&]{ n.push("users"); },        perm(Permission::Withdraw))
            .add_item("Transactions",   [&]{ n.push("transactions"); })
            .add_item("Currency",       [&]{ n.push("currency"); })
            .add_item("Settings",       [&]{ n.push("settings"); })
            .add_separator()
            .add_item("Logout", [&]
            {
                bank_user.logout();
                LOG_INFO("User (" + USERNAME + ") logged out of the system");
                n.exit();
            });
    });

    // === ACCOUNTS MENU ===
    nav.add("accounts", [&](MenuNavigator& n)
    {
        return Menu::create("Accounts")
            .add_item("Deposit", [&]
            {
                BankClient::ui_deposit();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::Deposit))
            .add_item("Withdraw", [&]
            {
                BankClient::ui_withdraw();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::Withdraw))
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === CLIENTS MENU ===
    nav.add("clients", [&](MenuNavigator& n)
    {
        return Menu::create("Manage Clients")
            .add_item("Show Clients List", [&]
            {
                platform::clear_terminal();
                BankClient::list_clients();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::ShowClients))
            .add_item("Add Client", [&]
            {
                platform::clear_terminal();
                BankClient::add_client();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::AddClient))
            .add_item("Update Client", [&]
            {
                platform::clear_terminal();
                BankClient::update_client();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::UpdateClient))
            .add_item("Delete Client", [&]
            {
                platform::clear_terminal();
                BankClient::delete_client();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::DeleteClient))
            .add_item("Find Client", [&]
            {
                platform::clear_terminal();
                BankClient found_user = BankClient::find(BankClient::get_valid_account_num()).value();
                found_user.print_client_details();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::FindClient))
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === USERS MENU ===
    nav.add("users", [&](MenuNavigator& n)
    {
        return Menu::create("Manage Users")
            .add_item("Show Users List", [&]
            {
                platform::clear_terminal();
                BankUser::list_users();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::ShowUsers))
            .add_item("Add User", [&]
            {
                platform::clear_terminal();
                BankUser::add_user();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::AddUser))
            .add_item("Update User", [&]
            {
                platform::clear_terminal();
                BankUser::update_user();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::UpdateUser))
            .add_item("Delete User", [&]
            {
                platform::clear_terminal();
                BankUser::delete_user();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::DeleteUser))
            .add_item("Find User", [&]
            {
                platform::clear_terminal();
                BankUser found_user = BankUser::find(BankUser::get_valid_username()).value();
                found_user.print_user_details();
                input::get_menu_key(dim("Press Enter..."));
            }, perm(Permission::FindUser))
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === TRANSACTIONS MENU ===
    nav.add("transactions", [&](MenuNavigator& n)
    {
        return Menu::create("Transactions")
            .add_item("Transfer", [&]{ n.push("transfer"); })
            .add_item("History", [&]
            {
                std::cout << yellow("\nNo history yet\n");
            })
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === TRANSFER MENU ===
    nav.add("transfer", [&](MenuNavigator& n)
    {
        return Menu::create("Transfer Money")
            .add_item("To Internal Account", [&]
            {
                BankClient::ui_transfer();
                input::get_menu_key(dim("Press Enter..."));
            })
            .add_item("To External Account", nullptr, [&]{ return false; }) // YAGNI :P
            .add_separator()
            .add_item("Back to Transactions", [&]{ n.pop(); })
            .add_item("Back to Main", [&]{ n.pop(); n.pop(); });
    });

    // === SETTINGS MENU ===
    nav.add("settings", [&](MenuNavigator& n)
    {
        return Menu::create("Settings")
            .add_item("Change PIN", [&]{ std::cout << "PIN changed\n"; })
            .add_item("Language",   [&]{ std::cout << "Language set\n"; })
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === CURRENCY MENU ===
    nav.add("currency", [&](MenuNavigator& n)
    {
        return Menu::create("Currency")
            .add_item("List Currencies", [&]
            {
                platform::clear_terminal();
                CurrencyExchange::list_available_currencies();
                input::get_menu_key(dim("Press Enter..."));
            })
            .add_item("Add Currency", [&]
            {
                platform::clear_terminal();
                CurrencyExchange::add_currency();
                input::get_menu_key(dim("Press Enter..."));
            })
            .add_item("Update Currency", [&]
            {
                platform::clear_terminal();
                CurrencyExchange::update_currency();
                input::get_menu_key(dim("Press Enter..."));
            })
            .add_item("Find Currency", [&]
            {
                platform::clear_terminal();
                std::vector<CurrencyExchange> currencies_with_same_code = CurrencyExchange::find_all(CurrencyExchange::get_valid_currency_code()).value();

                CurrencyExchange::print_currency_details(currencies_with_same_code);
                input::get_menu_key(dim("Press Enter..."));
            })
            .add_item("Currency Calculator", [&]
            {
                platform::clear_terminal();
                CurrencyExchange::convert_currency();
                input::get_menu_key(dim("Press Enter..."));
            })
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

            //     }, perm(Permission::ShowUsers))
            // .add_item("Add User", [&]
            // {
            //     platform::clear_terminal();
            //     BankUser::add_user();
            //     input::get_menu_key(dim("Press Enter..."));
            // }, perm(Permission::AddUser))

    nav.run("main");
}

void run_bank_demo()
{
    // Configure logger
    utils::Logger::instance().set_level(utils::LogLevel::Debug).enable_file();

    LOG_INFO("Bank application starting");

    // The alternate buffer would hide output printed after the application exits
    // Introduce a scope to ensure RAII restores the main buffer early before printing
    {
        tu::platform::AnsiGuard a;
        LOG_INFO("Terminal initialized");

        while(true)
        {
            run_bank();

            if(terminal_utils::input::get_yes_no("Want to quit the program? (y/n)"))
                break;
        }
    }

    // the final message to be printed on the main buffer screen.
    std::cout << italic(bold(terminal_utils::yellow("Bye User!"))) << std::endl;

    LOG_INFO("Bank application shutting down");
}

int main()
{
    run_bank_demo();

    return 0;
}
