// bank_user.hpp

#pragma once
#include <vector>
#include <fstream>
#include <limits>
#include <cstdint>

#include "person.hpp"
#include "../tu/output.hpp"
#include "../tu/config.hpp"
#include "../tu/style_wrappers.hpp"
#include "../tu/input.hpp"
#include "../platform/platform.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/utils.hpp"
#include "bank/persistent_entity.hpp"

namespace bank
{
    /**
     * @brief bitmask enum representing individual user permissions
     * 
     * @attention
     * Read as: has permision to `...`
     * 
     * each value is a distinct bit, allowing combinations via bitwise OR.
     * use `has_permission()` to test if a permission is granted.
     * 
     */
    enum class Permission : uint32_t
    {
        None            = 0,

        // -- Clients --
        ShowClients  = 1 << 0,
        AddClient    = 1 << 1,
        UpdateClient = 1 << 2,
        DeleteClient = 1 << 3,
        FindClient   = 1 << 4,

        // -- Users --
        ShowUsers    = 1 << 5,
        AddUser      = 1 << 6,
        UpdateUser   = 1 << 7,
        DeleteUser   = 1 << 8,
        FindUser     = 1 << 9,

        // -- Transactions --
        Deposit      = 1 << 10,
        Withdraw     = 1 << 11,
        Transfer     = 1 << 12,

        All =
            ShowClients   |
            AddClient     |
            UpdateClient  |
            DeleteClient  |
            FindClient    |
            ShowUsers     |
            AddUser       |
            UpdateUser    |
            DeleteUser    |
            FindUser      |
            Deposit       |
            Withdraw      |
            Transfer
    };


    /** @brief combines two permissions */
    inline constexpr Permission operator|(Permission a, Permission b)
    {
        return static_cast<Permission>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }


    /** @brief adds a permission in place */
    inline constexpr Permission& operator|=(Permission& a, Permission b)
    {
        a = a | b;
        return a;
    }


    /** @brief intersects two permissions */
    inline constexpr Permission operator&(Permission a, Permission b)
    {
        return static_cast<Permission>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }


    /** @brief intersects a permission in place */
    inline constexpr Permission& operator&=(Permission& a, Permission b)
    {
        a = a & b;
        return a;
    }


    /**
     * @brief checks if `user` has at least one bit of `required` set
     * @param user     the user's permission bitmask
     * @param required the permission(s) to check for
     * @return `true` if any bit in `required` is set in `user`
     */
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


    /**
     * @brief represents a bank system user with login credentials and access permissions
     * 
     * each user has a username/password pair for authentication and a `Permission`
     * bitmask that controls which operations they can perform in the system
     * 
     * use `login()` to authenticate, `save_with_result()` to persist changes,
     * and `has_permission()` to check access before performing operations
     */
    class BankUser : public PersistentEntity<BankUser>, public Person
    {
        public:

            /**
             * @brief return type for `save_with_result()`; indicates success or the exact failure reason
             * 
             * - `succeeded`              record was saved successfully
             * - `failed_empty_object`    attempted to save an empty record
             * - `failed_username_exists` username already exists (add mode only)
             * - `failed_cannot_update`   underlying I/O failure
             */
            enum class SaveResult
            {
                succeeded,
                failed_empty_object,
                failed_username_exists,
                failed_cannot_update
            };

            // =========================
            //         Data
            // =========================

        protected:
            std::string m_username;                          ///< unique login identifier
            std::string m_password;                          ///< password for authentication
            Permission  m_permissions = Permission::None;    ///< bitmask of granted permissions


        public:
            // =========================
            //     Constructors
            // =========================

            /** @brief default constructor: creates an empty sentinel with `Mode::empty_mode` */
            BankUser()
                : PersistentEntity(Mode::empty_mode)
                , Person("", "", "", "")
            {}


            /**
             * @brief full constructor used by the UI, `decode()`, and factory helpers
             * @param mode        persistence mode
             * @param first_name  first name
             * @param last_name   last name
             * @param email       email address
             * @param phone_num   phone number
             * @param username    unique login identifier
             * @param password    password for authentication
             * @param permissions bitmask of granted permissions
             */
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


            // =========================
            //     Factory helpers
            // =========================


            /** @brief creates an empty sentinel object equivalent to default construction */
            static BankUser make_empty()
            {
                return BankUser{};
            }


            /**
             * @brief creates a shell in `add_mode` with only the username set
             * @param username unique login identifier for the new user
             * @note caller fills in the remaining fields via `read_user_info()`
             */
            static BankUser make_new(std::string username)
            {
                return BankUser(Mode::add_mode, "", "", "", "", std::move(username), "", 0);
            }


            // =========================
            //    Required Cache hooks
            // =========================


            [[nodiscard]] static constexpr std::string_view class_name() { return "BankUser"; }


            // =========================
            //    Required CRTP hooks
            // =========================


            /** @brief returns the file path for user records */
            [[nodiscard]] static std::string_view file_name()
            {
                return tu::config::USERS_FILE_NAME;
            }


            /**
             * @brief deserialize a line from the file into a `BankUser` object
             * @note line format: `first#//#last#//#email#//#phone#//#username#//#password#//#permissions`
             */
            [[nodiscard]] static BankUser decode(const std::string& line)
            {
                auto ud = stutl::String::split(line, SEPARATOR);

                return BankUser(
                    Mode::update_mode,                                                      // loaded from file -> ready to update
                    ud[0],                                                                  // first_name
                    ud[1],                                                                  // last_name
                    ud[2],                                                                  // email
                    ud[3],                                                                  // phone
                    ud[4],                                                                  // username
                    utils::decrypt_text(ud[5], tu::config::CIPHER_SHIFT),                   // password (decrypted)
                    static_cast<uint32_t>(std::stoi(ud[6]))                                 // permissions
                );
            }


            /** @brief serialize this object into a file line, encrypting the password */
            [[nodiscard]] std::string encode() const
            {
                return m_first_name                                                           + std::string(SEPARATOR)
                    + m_last_name                                                             + std::string(SEPARATOR)
                    + m_email                                                                 + std::string(SEPARATOR)
                    + m_phone_num                                                             + std::string(SEPARATOR)
                    + m_username                                                              + std::string(SEPARATOR)
                    + utils::encrypt_text(m_password, tu::config::CIPHER_SHIFT)               + std::string(SEPARATOR)
                    + std::to_string(static_cast<uint32_t>(m_permissions));
            }


            /** @brief returns the username as the unique key */
            [[nodiscard]] std::string key() const
            {
                return m_username;
            }


            /** @brief returns `true` if this record's username matches `k` */
            [[nodiscard]] bool matches_key(std::string_view k) const
            {
                return m_username == k;
            }


            /**
             * @brief sort records by permission level ascending
             * @param records the records to sort in place
             */
            static void sort(std::vector<BankUser>& records)
            {
                // `stable_sort()` preserves the relative order of records with equal permissions,
                // allowing users to rely on consistent ordering between saves
                std::stable_sort(records.begin(), records.end(), [](const BankUser& a, const BankUser& b)
                {
                    return a.m_permissions < b.m_permissions;
                });
            }


            // =========================
            //         find()
            // =========================

            using PersistentEntity<BankUser>::find;

            /**
             * @brief find a user by username and password; used for login authentication
             * @param username the username to search for
             * @param password the password to match
             * @return the matching user, or `std::nullopt` if credentials do not match
             */
            [[nodiscard]] static std::optional<BankUser> find(const std::string& username,
                                                            const std::string& password)
            {
                for(const auto& u : load_all())
                    if(u.m_username == username && u.m_password == password)
                        return u;

                return std::nullopt;
            }


            // =========================
            //         Helpers
            // =========================


            /**
             * @brief wraps `save()` and returns a `SaveResult` instead of a plain `bool`
             * @return `SaveResult` indicating success or the exact failure reason
             * @see SaveResult
             */
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


            // =========================
            //   Helpers for Permission
            // =========================


            /**
             * @brief grants an additional permission to this user
             * @param p the permission to add
             */
            inline constexpr void add_permission(Permission p)
            {
                m_permissions |= p;
            }


            /**
             * @brief returns the user's current permission bitmask
             * @return the combined `Permission` value
             */
            [[nodiscard]] Permission get_permissions() const noexcept
            {
                return m_permissions;
            }


            /**
             * @brief returns the display name for a single permission value
             * @param p a single (non-combined) `Permission` value
             * @return a human-readable string view, or `""` for unknown values
             */
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


            /**
             * @brief formats a permission bitmask as a pipe-separated string
             * @param p the combined `Permission` value to format
             * @return e.g. `"Show Clients | Add Client | Deposit"`, or `"None"` if empty
             */
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
                for(Permission pn : all_permissions)
                    if(has_permission(p, pn))
                    {
                        if(!result.empty())
                            result += " | ";
                        result += get_permission_name(pn);
                    }

                return result.empty() ? "None" : result;
            }


            /** @brief formats this user's permissions as a pipe-separated string */
            std::string permissions_to_string() const
            {
                return permissions_to_string(m_permissions);
            }


            /**
             * @brief interactively prompts to grant or deny each permission
             * @return the resulting `Permission` bitmask after all prompts
             */
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

                for(Permission p : all_permissions)
                {
                    std::cout << "Should this user have access to [ "
                            << underline(red(tu::bold(std::string(get_permission_name(p)))))
                            << " ]? (y/n): ";

                    if(tu::input::get_yes_no(""))
                        granted |= p;
                }

                return granted;
            }

            [[nodiscard]] bool has_any_client_permission() const noexcept
            {
                using P = Permission;
                return has_permission(m_permissions,
                    P::ShowClients  | P::AddClient  | P::UpdateClient |
                    P::DeleteClient | P::FindClient);
            }

            [[nodiscard]] bool has_any_user_permission() const noexcept
            {
                using P = Permission;
                return has_permission(m_permissions,
                    P::ShowUsers  | P::AddUser  | P::UpdateUser |
                    P::DeleteUser | P::FindUser);
            }


            /** @brief returns the string representation of a `Mode` value */
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


            /** @brief returns the username of this user */
            std::string get_username() const
            {
                return m_username;
            }


            // =========================
            // Business logic (Login / Logout)
            // =========================


            /**
             * @brief interactively prompts for credentials and authenticates this user
             * 
             * allows up to 3 attempts before locking out and returning `false`.
             * on success, populates `*this` with the matching record from disk.
             * 
             * @return `true` on successful login, `false` after too many failed attempts
             */
            bool login()
            {
                tu::platform::clear_terminal();

                int attempts = 0;
                constexpr int MAX_ATTEMPTS = 3;

                while (true)
                {
                    std::string username = tu::input::get_string("Enter Username: ", "");
                    std::string password = tu::input::get_string("Enter Password: ", "");

                    auto result = find(username, password);

                    if(result)
                    {
                        *this = *result;
                        return true;
                    }

                    attempts++;

                    if(attempts >= MAX_ATTEMPTS)
                    {
                        tu::platform::clear_terminal();
                        std::cout << underline(bold(tu::red("Account locked, too many failed attempts\n")));
                        tu::input::get_menu_key(tu::dim("Press Enter To Continue..."));
                        tu::platform::clear_terminal();
                        return false;
                    }

                    std::cout << underline(bold(tu::red("\nInvalid Username/Password"))) << ", try again...\n\n";
                }
            }


            /** @brief resets this object to the empty sentinel, effectively logging out */
            void logout()
            {
                *this = BankUser{};
            }


            // =========================
            //           UI
            // =========================


            /** @brief prompts for a unique (non-existing) username */
            static std::string get_unique_username()
            {
                std::string username = tu::input::get_string("Enter user username: ", "");

                while (BankUser::exists(username))
                {
                    std::cout << "\nUsername already in use, choose another.\n";
                    username = tu::input::get_string("Enter user username: ", "");
                }

                return username;
            }


            /** @brief prompts for an existing username, loops until valid */
            static std::string get_valid_username()
            {
                std::string username = tu::input::get_string("Enter username: ", "");

                while (!BankUser::exists(username))
                {
                    std::cout << "\nNot a valid username.\n";
                    username = tu::input::get_string("Enter username: ", "");
                }

                return username;
            }


            /**
             * @brief prompts the user to fill in all fields of a `BankUser` object
             * @param user   the object to populate
             * @param header header text to display above the input form
             */
            static void read_user_info(BankUser& user, std::string_view header)
            {
                std::cout << "\n\n" << header;
                std::cout << "\n---------------------\n";

                std::cout << "\nEnter First Name: ";
                user.m_first_name = tu::input::get_string();

                std::cout << "\nEnter Last Name: ";
                user.m_last_name = tu::input::get_string();

                std::cout << "\nEnter Email: ";
                user.m_email = tu::input::get_string();

                std::cout << "\nEnter Phone: ";
                user.m_phone_num = tu::input::get_string();

                std::cout << "\nEnter Password: ";
                user.m_password = tu::input::get_string();

                std::cout << "\nEnter Permissions: ";
                user.m_permissions = set_user_permission();
            }


            /** @brief print this user's details as a formatted table */
            void print_user_details() const
            {
                tu::output::Table table;
                table.add_row({"first_name", "last_name", "email", "phone_num", "username", "password", "permissions", "mode"});
                table.add_row({m_first_name, m_last_name, m_email, m_phone_num, m_username, m_password,
                            std::to_string(static_cast<uint32_t>(m_permissions)), std::string(mode_name(_mode))});
                table.print(tu::output::Alignment::Center);
            }


            /** @brief print all users in the system, or a message if none exist */
            static void list_users()
            {
                const auto& users = load_all();

                if(users.empty())
                {
                    std::cout << "No users in the system.\n";
                    return;
                }

                tu::output::Table table;
                table.add_row({"first_name", "last_name", "email", "phone_num", "username", "password", "permissions", "mode"});

                for(const BankUser& u : users)
                    table.add_row_owned({u.m_first_name, u.m_last_name, u.m_email, u.m_phone_num,
                                        u.m_username, u.m_password,
                                        std::to_string(static_cast<uint32_t>(u.m_permissions)),
                                        std::string(mode_name(u._mode))});

                const std::string label = "List of (" + std::to_string(users.size()) + ") User(s):";
                constexpr size_t  PADDING = 4;
                std::cout << tu::output::print_aligned(label, label.size() + PADDING,
                                                                tu::output::Alignment::Right) << "\n";
                table.print(tu::output::Alignment::Center);
            }


            // =========================
            //     UI: CRUD actions
            // =========================


            /** @brief prompt the user to add a new user and save it */
            static void add_user()
            {
                BankUser user = BankUser::make_new(get_unique_username());

                tu::platform::clear_terminal();
                read_user_info(user, "Add User Info:");

                switch (user.save_with_result())
                {
                    case SaveResult::succeeded:
                        tu::platform::clear_terminal();
                        std::cout << "User added successfully:\n";
                        user.print_user_details();
                        break;

                    default:
                        tu::platform::clear_terminal();
                        std::cout << bold(underline(tu::red("An error occurred"))) << " while saving the file.\n";
                        break;
                }
            }


            /** @brief prompt the user to find and update an existing user */
            static void update_user()
            {
                auto opt = BankUser::find(get_valid_username());
                if(!opt) return;

                BankUser user = *opt;

                tu::platform::clear_terminal();
                user.print_user_details();
                read_user_info(user, "Update User Info:");

                user.set_mode(Mode::update_mode);

                switch (user.save_with_result())
                {
                    case SaveResult::succeeded:
                        tu::platform::clear_terminal();
                        std::cout << "User updated successfully:\n";
                        user.print_user_details();
                        break;

                    default:
                        tu::platform::clear_terminal();
                        std::cout << bold(underline(tu::red("An error occurred"))) << " couldn't update the user.\n";
                        break;
                }
            }


            /** @brief prompt the user to find and delete an existing user */
            static void delete_user()
            {
                std::string username = get_valid_username();
                auto        opt      = BankUser::find(username);
                if(!opt) return;

                BankUser user = *opt;

                tu::platform::clear_terminal();
                user.print_user_details();

                if(!tu::input::get_yes_no("\n\nAre you sure you want to delete this user? (y/n):"))
                {
                    std::cout << "\nDelete operation canceled.\n";
                    return;
                }

                user.set_mode(Mode::delete_mode);
                user.save();

                tu::platform::clear_terminal();
                std::cout << "User (" << username << ") deleted successfully.\n";
            }
    };

} // namespace bank
