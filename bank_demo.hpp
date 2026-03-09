// bank_demo.hpp

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
#include "tu/menu/menu_navigator.hpp"

#include "bank/bank_client.hpp"
#include "bank/bank_user.hpp"
#include "bank/currency_exchange.hpp"

/**
 * @brief Returns a visibility checker for the given permission.
 * 
 * Usage:
 * 
 *   `.add_item("Deposit", [&]{ ... }, bank::visible_if(bank::Permission::Deposit))`
 */
inline auto visible_if(bank::Permission required, const bank::BankUser& user)
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
 * 
 *   `auto perm = bank::for_user(current_user);`
 * 
 *   `.add_item("Deposit", [&]{ ... }, perm(bank::Permission::Deposit))`
 */
inline auto for_user(const bank::BankUser& user)
{
    return [&user](bank::Permission required) {
        return [&user, required]() -> bool {
            return has_permission(user.get_permissions(), required);
        };
    };
}


void print_color_options()
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


void run_bank()
{
    bank::BankUser bank_user{};

    if(!bank_user.login())
        return;

    const std::string USERNAME = bank_user.get_username();
    LOG_INFO("User (" + USERNAME + ") logged into the system");

    auto perm = for_user(bank_user);

    enum class MenuID {main_menu, users_menu, clients_menu,
                       transaction_menu, transfer_menu, transfer_internal,
                       currency_menu, settings_menu};
    tu::MenuNavigator<MenuID> nav;

    // === MAIN MENU ===
    nav.add(MenuID::main_menu, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("Bank System")
                        .set_width(40)
                        .set_date()
                        .add_global_subtitle("Logged in as: " + USERNAME)
                        .add_item("Manage Clients", [&]{ n.push(MenuID::clients_menu);      }, perm(bank::Permission::ManageClients))
                        .add_item("Manage Users",   [&]{ n.push(MenuID::users_menu);        }, perm(bank::Permission::ManageUsers))
                        .add_item("Transactions",   [&]{ n.push(MenuID::transaction_menu);  })
                        .add_item("Currency",       [&]{ n.push(MenuID::currency_menu);     })
                        .add_item("Settings",       [&]{ n.push(MenuID::settings_menu);     })
                        .add_separator()
                        .add_item("Logout", [&]
                        {
                            bank_user.logout();
                            LOG_INFO("User (" + USERNAME + ") logged out of the system");
                            n.exit();
                        });
    });
    // === CLIENTS MENU ===
    nav.add(MenuID::clients_menu, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("Manage Clients")
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
                bank::BankClient found_user = bank::BankClient::find(bank::BankClient::get_valid_account_num()).value();
                found_user.print_client_details();
                tu::input::get_menu_key(tu::dim("Press Enter..."));
            }, perm(bank::Permission::FindClient))
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === USERS MENU ===
    nav.add(MenuID::users_menu, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("Manage Users")
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
                bank::BankUser found_user = bank::BankUser::find(bank::BankUser::get_valid_username()).value();
                found_user.print_user_details();
                tu::input::get_menu_key(tu::dim("Press Enter..."));
            }, perm(bank::Permission::FindUser))
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === TRANSACTIONS MENU ===
    nav.add(MenuID::transaction_menu, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("Transactions")
            .add_item("Transfer", [&]{ n.push(MenuID::transfer_menu); })
            .add_item("History", nullptr, [&]{ return false; })
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === TRANSFER MENU ===
    nav.add(MenuID::transfer_menu, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("Transfer Money")
                        .add_item("To Internal Account", [&]{ n.push(MenuID::transfer_internal); }) // internal: within this bank
                        .add_item("To External Account", nullptr, [&]{ return false; })       // external: to another bank (if added); YAGNI :P
                        .add_separator()
                        .add_item("Back to Transactions", [&]{ n.pop(); })
                        .add_item("Back to Main Menu", [&]{ n.pop(); n.pop(); });
    });

    // === TRANSFER INTERNAL MENU ===
    nav.add(MenuID::transfer_internal, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("To Internal Account")
            .add_item("Transfer", [&] 
            {
                // move money from account to another
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
            .add_item("Back", [&]{ n.pop(); });
    });

    // === SETTINGS MENU ===
    nav.add(MenuID::settings_menu, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("Settings")
            .add_item("Language", nullptr, [&]{ return false; }) // in case i ever touch locale.h or tinker with UTF-8 + yet another YAGNI :P
            .add_item("Change Highlight Color", [&]
            {
                print_color_options();
                tu::Menu::set_highlight_color_global(tu::ColoredText::get_color());
            })
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    // === CURRENCY MENU ===
    nav.add(MenuID::currency_menu, [&](tu::MenuNavigator<MenuID>& n)
    {
        return tu::Menu::create("Currency")
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
                std::vector<bank::CurrencyExchange> currencies_with_same_code = bank::CurrencyExchange::find_all(bank::CurrencyExchange::get_valid_currency_code()).value();

                bank::CurrencyExchange::print_currency_details(currencies_with_same_code);
                tu::input::get_menu_key(tu::dim("Press Enter..."));
            })
            .add_item("Currency Calculator", [&]
            {
                tu::platform::clear_terminal();
                bank::CurrencyExchange::convert_currency();
                tu::input::get_menu_key(tu::dim("Press Enter..."));
            })
            .add_separator()
            .add_item("Back to Main", [&]{ n.pop(); });
    });

    nav.run(MenuID::main_menu);
}

void run_bank_demo()
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
            run_bank();

            if(tu::input::get_yes_no("Want to quit the program? (y/n)"))
                break;
        }
    }

    // the final message to be printed
    std::cout << italic(bold(tu::yellow("Bye User!"))) << std::endl;

    LOG_INFO("Bank application shutting down");
}
