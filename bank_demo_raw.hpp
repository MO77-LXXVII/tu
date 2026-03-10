// bank_demo_raw.hpp

#include <iostream>
#include <iomanip>
#include <limits>
#include <cassert>
#include <vector>
#include <stack>
#include <string>

#include "platform/platform.hpp"

#include "tu/ansi_colors.hpp"
#include "tu/style_wrappers.hpp"
#include "tu/input.hpp"
#include "tu/output.hpp"
#include "tu/logger.hpp"
#include "tu/menu/menu.hpp"

#include "bank/bank_client.hpp"
#include "bank/bank_user.hpp"
#include "bank/currency_exchange.hpp"

namespace
{
    /**
     * @brief Returns a visibility checker for the given permission.
     *
     * Usage:
     *
     *   `.add_item("Deposit", [&]{ ... }, visible_if(bank::Permission::Deposit, user))`
     */
    inline auto visible_if(bank::Permission required, const bank::BankUser& user)
    {
        return [user, required]() -> bool
        {
            return bank::has_permission(user.get_permissions(), required);
        };
    }
}

/**
 * @brief Curry-style: bind the user first, then use for multiple items.
 *
 * Usage:
 *
 *   `auto perm = for_user(current_user);`
 *
 *   `.add_item("Deposit", [&]{ ... }, perm(bank::Permission::Deposit))`
 */
inline auto for_user_raw(const bank::BankUser& user)
{
    return [&user](bank::Permission required) {
        return [&user, required]() -> bool {
            return bank::has_permission(user.get_permissions(), required);
        };
    };
}

void print_color_options_raw()
{
    std::cout << "\n";
    std::cout << "1. > Reset\n";
    std::cout << "2. " << tu::black  ("> Black")   << "\n";
    std::cout << "3. " << tu::red    ("> Red")     << "\n";
    std::cout << "4. " << tu::green  ("> Green")   << "\n";
    std::cout << "5. " << tu::yellow ("> Yellow")  << "\n";
    std::cout << "6. " << tu::blue   ("> Blue")    << "\n";
    std::cout << "7. " << tu::magenta("> Magenta") << "\n";
    std::cout << "8. " << tu::cyan   ("> Cyan")    << "\n";
    std::cout << "9. " << tu::white  ("> White")   << "\n\n";
}

void run_bank_raw()
{
    bank::BankUser bank_user{};

    if(!bank_user.login())
        return;

    const std::string USERNAME = bank_user.get_username();
    LOG_INFO("User (" + USERNAME + ") logged into the system");

    auto perm = for_user_raw(bank_user);

    std::stack<std::string> menu_stack;
    menu_stack.push("main");

    while(!menu_stack.empty())
    {
        std::string current = menu_stack.top();

        // === MAIN MENU ===
        if(current == "main")
        {
            auto result = tu::Menu::create("Bank System")
                .set_width(40)
                .set_date()
                .add_global_subtitle("Logged in as: " + USERNAME)
                .add_item("Manage Clients", [&]{ menu_stack.push("clients");      }, [&] ()-> bool { return bank_user.has_any_client_permission(); })
                .add_item("Manage Users",   [&]{ menu_stack.push("users");        }, [&] ()-> bool { return bank_user.has_any_user_permission();   })
                .add_item("Transactions",   [&]{ menu_stack.push("transactions"); })
                .add_item("Currency",       [&]{ menu_stack.push("currency");     })
                .add_item("Settings",       [&]{ menu_stack.push("settings");     })
                .add_separator()
                .add_item("Logout", [&]
                {
                    bank_user.logout();
                    LOG_INFO("User (" + USERNAME + ") logged out of the system");
                    while(!menu_stack.empty()) menu_stack.pop();
                })
                .run();

            if(result == tu::MenuResult::Quit)
            {
                bank_user.logout();
                LOG_INFO("User (" + USERNAME + ") logged out of the system");
                while(!menu_stack.empty()) menu_stack.pop();
            }
        }

        // === CLIENTS MENU ===
        else if(current == "clients")
        {
            auto result = tu::Menu::create("Manage Clients")
                .add_item("Show Clients List", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankClient::list_clients();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::ShowClients))
                .add_item("Add Client", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankClient::add_client();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::AddClient))
                .add_item("Update Client", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankClient::update_client();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::UpdateClient))
                .add_item("Delete Client", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankClient::delete_client();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::DeleteClient))
                .add_item("Find Client", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankClient found = bank::BankClient::find(bank::BankClient::get_valid_account_num()).value();
                    found.print_client_details();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::FindClient))
                .add_separator()
                .add_item("Back to Main", [&]{ menu_stack.pop(); })
                .run();

            if(result == tu::MenuResult::Quit)
                menu_stack.pop();
        }

        // === USERS MENU ===
        else if(current == "users")
        {
            auto result = tu::Menu::create("Manage Users")
                .add_item("Show Users List", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankUser::list_users();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::ShowUsers))
                .add_item("Add User", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankUser::add_user();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::AddUser))
                .add_item("Update User", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankUser::update_user();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::UpdateUser))
                .add_item("Delete User", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankUser::delete_user();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::DeleteUser))
                .add_item("Find User", [&]
                {
                    tu::platform::clear_terminal();
                    bank::BankUser found = bank::BankUser::find(bank::BankUser::get_valid_username()).value();
                    found.print_user_details();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::FindUser))
                .add_separator()
                .add_item("Back to Main", [&]{ menu_stack.pop(); })
                .run();

            if(result == tu::MenuResult::Quit)
                menu_stack.pop();
        }

        // === TRANSACTIONS MENU ===
        else if(current == "transactions")
        {
            auto result = tu::Menu::create("Transactions")
                .add_item("Transfer", [&]{ menu_stack.push("transfer"); })
                .add_item("History",  nullptr, [&]{ return false; })
                .add_separator()
                .add_item("Back to Main", [&]{ menu_stack.pop(); })
                .run();

            if(result == tu::MenuResult::Quit)
                menu_stack.pop();
        }

        // === TRANSFER MENU ===
        else if(current == "transfer")
        {
            auto result = tu::Menu::create("Transfer Money")
                .add_item("To Internal Account", [&]{ menu_stack.push("transfer_internal"); }) // internal: within this bank
                .add_item("To External Account", nullptr, [&]{ return false; })               // external: to another bank; YAGNI :P
                .add_separator()
                .add_item("Back to Transactions", [&]{ menu_stack.pop(); })
                .add_item("Back to Main Menu",    [&]{ menu_stack.pop(); menu_stack.pop(); })
                .run();

            if(result == tu::MenuResult::Quit)
                menu_stack.pop();
        }

        // === TRANSFER INTERNAL MENU ===
        else if(current == "transfer_internal")
        {
            auto result = tu::Menu::create("To Internal Account")
                .add_item("Transfer", [&]
                {
                    bank::BankClient::ui_transfer();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::Transfer))
                .add_item("Deposit", [&]
                {
                    bank::BankClient::ui_deposit();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::Deposit))
                .add_item("Withdraw", [&]
                {
                    bank::BankClient::ui_withdraw();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                }, perm(bank::Permission::Withdraw))
                .add_separator()
                .add_item("Back", [&]{ menu_stack.pop(); })
                .run();

            if(result == tu::MenuResult::Quit)
                menu_stack.pop();
        }

        // === SETTINGS MENU ===
        else if(current == "settings")
        {
            auto result = tu::Menu::create("Settings")
                .add_item("Language", nullptr, [&]{ return false; }) // YAGNI :P
                .add_item("Change Highlight Color", [&]
                {
                    print_color_options_raw();
                    tu::Menu::set_highlight_color_global(tu::ColoredText::get_color());
                })
                .add_separator()
                .add_item("Back to Main", [&]{ menu_stack.pop(); })
                .run();

            if(result == tu::MenuResult::Quit)
                menu_stack.pop();
        }

        // === CURRENCY MENU ===
        else if(current == "currency")
        {
            auto result = tu::Menu::create("Currency")
                .add_item("List Currencies", [&]
                {
                    tu::platform::clear_terminal();
                    bank::CurrencyExchange::list_available_currencies();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                })
                .add_item("Add Currency", [&]
                {
                    tu::platform::clear_terminal();
                    bank::CurrencyExchange::add_currency();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                })
                .add_item("Update Currency", [&]
                {
                    tu::platform::clear_terminal();
                    bank::CurrencyExchange::update_currency();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                })
                .add_item("Find Currency", [&]
                {
                    tu::platform::clear_terminal();
                    std::vector<bank::CurrencyExchange> currencies = bank::CurrencyExchange::find_all(bank::CurrencyExchange::get_valid_currency_code()).value();
                    bank::CurrencyExchange::print_currency_details(currencies);
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                })
                .add_item("Currency Calculator", [&]
                {
                    tu::platform::clear_terminal();
                    bank::CurrencyExchange::convert_currency();
                    tu::input::get_menu_key(tu::dim("Press Enter..."));
                })
                .add_separator()
                .add_item("Back to Main", [&]{ menu_stack.pop(); })
                .run();

            if(result == tu::MenuResult::Quit)
                menu_stack.pop();
        }
    }
}

void run_bank_demo_raw()
{
    // Configure logger
    utils::Logger::instance().set_level(utils::LogLevel::Debug).enable_file();

    LOG_INFO("Bank application starting");

    // scope ensures `AnsiGuard` restores the terminal before the final message is printed
    {
        tu::platform::AnsiGuard a;
        LOG_INFO("Terminal initialized");

        while(true)
        {
            run_bank_raw();

            if(tu::input::get_yes_no("Want to quit the program? (y/n)"))
                break;
        }
    }

    // the final message to be printed
    std::cout << tu::italic(tu::bold(tu::yellow("Bye User!"))) << std::endl;

    LOG_INFO("Bank application shutting down");
}
