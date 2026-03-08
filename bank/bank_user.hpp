// bank_user.hpp

#pragma once
#include <vector>
#include <fstream>
#include <limits>
#include <cstdint>

#include "person.hpp"
#include "../terminal_utils/output.hpp"
#include "../terminal_utils/config.hpp"
#include "../terminal_utils/style_wrappers.hpp"
#include "../terminal_utils/input.hpp"
#include "../platform/platform.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/utils.hpp"
#include "bank/persistent_entity.hpp"

// max value: 8191
enum class Permission : uint32_t
{
    None            = 0,
    ShowClients     = 1 << 0,
    AddClient       = 1 << 1,
    UpdateClient    = 1 << 2,
    DeleteClient    = 1 << 3,
    FindClient      = 1 << 4,
    ShowUsers       = 1 << 5,
    AddUser         = 1 << 6,
    UpdateUser      = 1 << 7,
    DeleteUser      = 1 << 8,
    FindUser        = 1 << 9,
    Deposit         = 1 << 10,
    Withdraw        = 1 << 11,
    Transfer        = 1 << 12,

    All =
        ShowClients  |
        AddClient    |
        UpdateClient |
        DeleteClient |
        FindClient   |
        ShowUsers    |
        AddUser      |
        UpdateUser   |
        DeleteUser   |
        FindUser     |
        Deposit      |
        Withdraw     |
        Transfer
};

// Bitwise operators
inline constexpr Permission operator|(Permission a, Permission b)
{
    return static_cast<Permission>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr Permission& operator|=(Permission& a, Permission b)
{
    a = a | b;
    return a;
}

inline constexpr Permission operator&(Permission a, Permission b)
{
    return static_cast<Permission>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline constexpr Permission& operator&=(Permission& a, Permission b)
{
    a = a & b;
    return a;
}

inline constexpr bool has_permission(Permission user, Permission required)
{
    return static_cast<uint32_t>(user & required) != 0;
}


// ============================================================
// BankUser
// Inherits shared persistence from PersistentEntity<BankUser>
// Inherits shared person fields from Person
// Only implements the 6 required CRTP hooks + its own business logic
// ============================================================

class BankUser : public PersistentEntity<BankUser>, public Person
{
    // ----------------------------------------------------------
    // Types
    // ----------------------------------------------------------
    public:
        enum class SaveResult
        {
            succeeded,
            failed_empty_object,
            failed_username_exists,
            failed_cannot_update
        };

    // ----------------------------------------------------------
    // Data
    // ----------------------------------------------------------
    protected:
        std::string m_username;
        std::string m_password;
        Permission  m_permissions = Permission::None;

    // ----------------------------------------------------------
    // Constructors
    // ----------------------------------------------------------
    public:
        // Empty sentinel: returned when a lookup finds nothing
        BankUser()
            : PersistentEntity(Mode::empty_mode)
            , Person("", "", "", "")
        {}

        // Full constructor used everywhere (UI, decode, factory helpers)
        BankUser(Mode        mode,
                 std::string first_name,
                 std::string last_name,
                 std::string email,
                 std::string phone_num,
                 std::string username,
                 std::string password,
                 uint32_t    permissions)
            : PersistentEntity(mode)
            , Person(std::move(first_name), std::move(last_name), std::move(email), std::move(phone_num))
            , m_username    (std::move(username))
            , m_password    (std::move(password))
            , m_permissions (static_cast<Permission>(permissions))
        {}

    // ----------------------------------------------------------
    // Factory helpers
    // ----------------------------------------------------------
    public:
        static BankUser make_empty()
        {
            return BankUser{};
        }

        // Creates a shell in add_mode with only a username set;
        // caller fills in the rest via read_user_info()
        static BankUser make_new(std::string username)
        {
            return BankUser(Mode::add_mode, "", "", "", "", std::move(username), "", 0);
        }

    // ----------------------------------------------------------
    // Required CRTP hooks
    // ----------------------------------------------------------
    public:
        [[nodiscard]] static std::string_view file_name()
        {
            return terminal_utils::config::USERS_FILE_NAME;
        }

        // line format: first#//#last#//#email#//#phone#//#username#//#password#//#permissions
        [[nodiscard]] static BankUser decode(const std::string& line)
        {
            auto ud = stutl::String::split(line, SEPARATOR);

            return BankUser(
                Mode::update_mode,                                                      // loaded from file → ready to update
                ud[0],                                                                  // first_name
                ud[1],                                                                  // last_name
                ud[2],                                                                  // email
                ud[3],                                                                  // phone
                ud[4],                                                                  // username
                utils::decrypt_text(ud[5], terminal_utils::config::CIPHER_SHIFT),    // password (decrypted)
                static_cast<uint32_t>(std::stoi(ud[6]))                                // permissions
            );
        }

        [[nodiscard]] std::string encode() const
        {
            return m_first_name                                                            + std::string(SEPARATOR)
                 + m_last_name                                                             + std::string(SEPARATOR)
                 + m_email                                                                 + std::string(SEPARATOR)
                 + m_phone_num                                                             + std::string(SEPARATOR)
                 + m_username                                                              + std::string(SEPARATOR)
                //  + utils::encrypt_text(m_password, terminal_utils::config::ENCRYPTION_KEY) + std::string(SEPARATOR)
                 + std::to_string(static_cast<uint32_t>(m_permissions));
        }

        [[nodiscard]] std::string key() const
        {
            return m_username;
        }

        [[nodiscard]] bool matches_key(const std::string& k) const
        {
            return m_username == k;
        }

        static void sort(std::vector<BankUser>& records)
        {
            // `stable_sort()` preserves the relative order of records with equal rates,
            // allowing users to rely on consistent ordering between saves
            std::stable_sort(records.begin(), records.end(), [](const BankUser& a, const BankUser& b)
            {
                return a.m_permissions < b.m_permissions;
            });
        }

    // ----------------------------------------------------------
    // find() overload — by username + password (for login)
    // The base find(key) covers the username-only case.
    // ----------------------------------------------------------
    public:
        [[nodiscard]] static std::optional<BankUser> find(const std::string& username)
        {
            for (auto& u : load_all())
                if (u.m_username == username)
                    return u;

            return std::nullopt;
        }

        [[nodiscard]] static std::optional<BankUser> find(const std::string& username,
                                                          const std::string& password)
        {
            for (auto& u : load_all())
                if (u.m_username == username && u.m_password == password)
                    return u;

            return std::nullopt;
        }

    // ----------------------------------------------------------
    // Richer save() — wraps base save() and returns SaveResult
    // ----------------------------------------------------------
    public:
        SaveResult save_with_result()
        {
            if(is_empty())
                return SaveResult::failed_empty_object;

            if(_mode == Mode::add_mode && BankUser::exists(m_username))
                return SaveResult::failed_username_exists;

            // Delegate to PersistentEntity::save() for the actual I/O
            if(!save())
                return SaveResult::failed_cannot_update;

            return SaveResult::succeeded;
        }

    // ----------------------------------------------------------
    // Permission helpers
    // ----------------------------------------------------------
    public:
        inline constexpr void add_permission(Permission p)
        {
            m_permissions |= p;
        }

        [[nodiscard]] Permission get_permissions() const noexcept
        {
            return m_permissions;
        }

        [[nodiscard]] static constexpr std::string_view get_permission_name(Permission p)
        {
            switch (p)
            {
                case Permission::None:          return "";
                case Permission::ShowClients:   return "Show Clients";
                case Permission::AddClient:     return "Add Client";
                case Permission::UpdateClient:  return "Update Client";
                case Permission::DeleteClient:  return "Delete Client";
                case Permission::FindClient:    return "Find Client";
                case Permission::ShowUsers:     return "Show Users";
                case Permission::AddUser:       return "Add User";
                case Permission::UpdateUser:    return "Update User";
                case Permission::DeleteUser:    return "Delete User";
                case Permission::FindUser:      return "Find User";
                case Permission::Deposit:       return "Deposit";
                case Permission::Withdraw:      return "Withdraw";
                case Permission::Transfer:      return "Transfer";
                default:                        return "";
            }
        }

        static std::string permissions_to_string(Permission p)
        {
            constexpr Permission all_permissions[] =
            {
                Permission::ShowClients, Permission::AddClient,
                Permission::UpdateClient, Permission::DeleteClient,
                Permission::FindClient,
                Permission::ShowUsers, Permission::AddUser,
                Permission::UpdateUser, Permission::DeleteUser,
                Permission::FindUser,
                Permission::Deposit, Permission::Withdraw, Permission::Transfer
            };

            std::string result;
            for (Permission pn : all_permissions)
                if (has_permission(p, pn))
                {
                    if (!result.empty())
                        result += " | ";
                    result += get_permission_name(pn);
                }

            return result.empty() ? "None" : result;
        }

        std::string permissions_to_string() const
        {
            return permissions_to_string(m_permissions);
        }

        static Permission set_user_permission()
        {
            Permission granted = Permission::None;

            constexpr Permission all_permissions[] =
            {
                Permission::ShowClients, Permission::AddClient,
                Permission::UpdateClient, Permission::DeleteClient,
                Permission::FindClient,
                Permission::ShowUsers, Permission::AddUser,
                Permission::UpdateUser, Permission::DeleteUser,
                Permission::FindUser,
                Permission::Deposit, Permission::Withdraw, Permission::Transfer
            };

            for (Permission p : all_permissions)
            {
                std::cout << "Should this user have access to [ "
                          << underline(red(terminal_utils::bold(std::string(get_permission_name(p)))))
                          << " ]? (y/n): ";

                if (terminal_utils::input::get_yes_no(""))
                    granted |= p;
            }

            return granted;
        }

    // ----------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------
    public:
        [[nodiscard]] static constexpr std::string_view mode_name(Mode m) noexcept
        {
            switch (m)
            {
                case Mode::empty_mode:  return "empty";
                case Mode::add_mode:    return "add";
                case Mode::update_mode: return "update";
                case Mode::delete_mode: return "delete";
                default:                return "";
            }
        }

        std::string get_username() const
        {
            return m_username;
        }

    // ----------------------------------------------------------
    // Login / Logout
    // ----------------------------------------------------------
    public:
        bool login()
        {
            terminal_utils::platform::clear_terminal();

            int attempts = 0;
            constexpr int MAX_ATTEMPTS = 3;

            while (true)
            {
                std::string username = terminal_utils::input::get_string("Enter Username: ", "");
                std::string password = terminal_utils::input::get_string("Enter Password: ", "");

                auto result = find(username, password);

                if(result)
                {
                    *this = *result;
                    return true;
                }

                attempts++;

                if(attempts >= MAX_ATTEMPTS)
                {
                    terminal_utils::platform::clear_terminal();
                    std::cout << underline(bold(terminal_utils::red("Account locked, too many failed attempts\n")));
                    terminal_utils::input::get_menu_key(terminal_utils::dim("Press Enter To Continue..."));
                    terminal_utils::platform::clear_terminal();
                    return false;
                }

                std::cout << underline(bold(terminal_utils::red("\nInvalid Username/Password"))) << ", try again...\n\n";
            }
        }

        void logout()
        {
            *this = BankUser{};
        }

    // ----------------------------------------------------------
    // UI — input
    // ----------------------------------------------------------
    public:
        static std::string get_unique_username()
        {
            std::string username = terminal_utils::input::get_string("Enter user username: ", "");

            while (BankUser::exists(username))
            {
                std::cout << "\nUsername already in use, choose another.\n";
                username = terminal_utils::input::get_string("Enter user username: ", "");
            }

            return username;
        }

        static std::string get_valid_username()
        {
            std::string username = terminal_utils::input::get_string("Enter username: ", "");

            while (!BankUser::exists(username))
            {
                std::cout << "\nNot a valid username.\n";
                username = terminal_utils::input::get_string("Enter username: ", "");
            }

            return username;
        }

        static void read_user_info(BankUser& user, std::string_view header)
        {
            std::cout << "\n\n" << header;
            std::cout << "\n---------------------\n";

            std::cout << "\nEnter First Name: ";
            user.m_first_name = terminal_utils::input::get_string();

            std::cout << "\nEnter Last Name: ";
            user.m_last_name = terminal_utils::input::get_string();

            std::cout << "\nEnter Email: ";
            user.m_email = terminal_utils::input::get_string();

            std::cout << "\nEnter Phone: ";
            user.m_phone_num = terminal_utils::input::get_string();

            std::cout << "\nEnter Password: ";
            user.m_password = terminal_utils::input::get_string();

            std::cout << "\nEnter Permissions: ";
            user.m_permissions = set_user_permission();
        }

    // ----------------------------------------------------------
    // UI — display
    // ----------------------------------------------------------
    public:
        void print_user_details() const
        {
            terminal_utils::output::Table table;
            table.add_row({"first_name", "last_name", "email", "phone_num", "username", "password", "permissions", "mode"});
            table.add_row({m_first_name, m_last_name, m_email, m_phone_num, m_username, m_password,
                           std::to_string(static_cast<uint32_t>(m_permissions)), std::string(mode_name(_mode))});
            table.print(terminal_utils::output::Alignment::Center);
        }

        static void list_users()
        {
            auto users = load_all();

            if (users.empty())
            {
                std::cout << "No users in the system.\n";
                return;
            }

            terminal_utils::output::Table table;
            table.add_row({"first_name", "last_name", "email", "phone_num", "username", "password", "permissions", "mode"});

            for (const BankUser& u : users)
                table.add_row_owned({u.m_first_name, u.m_last_name, u.m_email, u.m_phone_num,
                                     u.m_username, u.m_password,
                                     std::to_string(static_cast<uint32_t>(u.m_permissions)),
                                     std::string(mode_name(u._mode))});

            const std::string label = "List of (" + std::to_string(users.size()) + ") User(s):";
            constexpr size_t  PADDING = 4;
            std::cout << terminal_utils::output::print_aligned(label, label.size() + PADDING,
                                                               terminal_utils::output::Alignment::Right) << "\n";
            table.print(terminal_utils::output::Alignment::Center);
        }

    // ----------------------------------------------------------
    // UI — CRUD actions
    // ----------------------------------------------------------
    public:
        static void add_user()
        {
            BankUser user = BankUser::make_new(get_unique_username());

            terminal_utils::platform::clear_terminal();
            read_user_info(user, "Add User Info:");

            switch (user.save_with_result())
            {
                case SaveResult::succeeded:
                    terminal_utils::platform::clear_terminal();
                    std::cout << "User added successfully:\n";
                    user.print_user_details();
                    break;

                default:
                    terminal_utils::platform::clear_terminal();
                    std::cout << bold(underline(terminal_utils::red("An error occurred"))) << " while saving the file.\n";
                    break;
            }
        }

        static void update_user()
        {
            auto opt = BankUser::find(get_valid_username());
            if (!opt) return;

            BankUser user = *opt;

            terminal_utils::platform::clear_terminal();
            user.print_user_details();
            read_user_info(user, "Update User Info:");

            user.set_mode(Mode::update_mode);

            switch (user.save_with_result())
            {
                case SaveResult::succeeded:
                    terminal_utils::platform::clear_terminal();
                    std::cout << "User updated successfully:\n";
                    user.print_user_details();
                    break;

                default:
                    terminal_utils::platform::clear_terminal();
                    std::cout << bold(underline(terminal_utils::red("An error occurred"))) << " couldn't update the user.\n";
                    break;
            }
        }

        static void delete_user()
        {
            std::string username = get_valid_username();
            auto        opt      = BankUser::find(username);
            if (!opt) return;

            BankUser user = *opt;

            terminal_utils::platform::clear_terminal();
            user.print_user_details();

            if (!terminal_utils::input::get_yes_no("\n\nAre you sure you want to delete this user? (y/n):"))
            {
                std::cout << "\nDelete operation canceled.\n";
                return;
            }

            user.set_mode(Mode::delete_mode);
            user.save();

            terminal_utils::platform::clear_terminal();
            std::cout << "User (" << username << ") deleted successfully.\n";
        }
};
