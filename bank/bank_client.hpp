// bank_client.hpp

#pragma once
#include <vector>
#include <fstream>
#include <limits>

#include "person.hpp"
#include "../terminal_utils/config.hpp"
#include "../terminal_utils/output.hpp"
#include "../terminal_utils/style_wrappers.hpp"
#include "../terminal_utils/input.hpp"
#include "../platform/platform.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/utils.hpp"
#include "bank/persistent_entity.hpp"

// ============================================================
// BankClient
// Inherits shared persistence from PersistentEntity<BankClient>
// Inherits shared person fields from Person
// Only implements the 5 required CRTP hooks + its own business logic
// ============================================================


/**
 * @brief represents a bank client with an account, PIN, and balance
 * 
 * each client has a unique account number used for lookups and transactions
 * supports deposit, withdrawal, and transfer operations with balance cap enforcement
 * 
 * use `find()` with account number and PIN for ATM-style authentication,
 * or `find()` with account number only for admin lookups
 */
class BankClient : public PersistentEntity<BankClient>, public Person
{
    public:

        // ignore IntelliSense scoping issue
        // it gets confused about the scope of nested enums inside CRTP templates
        /**
         * @brief return type for `save_with_result()`; indicates success or the exact failure reason
         * 
         * - `succeeded`              record was saved successfully
         * - `failed_empty_object`    attempted to save an empty record
         * - `failed_account_exists`  account number already exists(add mode only)
         * - `failed_cannot_update`   underlying I/O failure
         */
        enum class SaveResult
        {
            succeeded,
            failed_empty_object,
            failed_account_exists,
            failed_cannot_update
        };


        // ignore IntelliSense scoping issue
        // it gets confused about the scope of nested enums inside CRTP templates
        /**
         * @brief return type for `transfer()`: indicates success or the exact failure reason
         * 
         * - `success`                   transfer completed successfully
         * - `recipient_cap_exceeded`    transfer would exceed recipient's maximum balance
         * - `save_failed`               I/O failure during withdraw or deposit
         * - `critical_rollback_failed`  withdraw succeeded but deposit failed and rollback also failed
         */
        enum class TransferResult
        {
            success,
            recipient_cap_exceeded,
            save_failed,
            critical_rollback_failed
        };


        // =========================
        //         Data
        // =========================

    protected:
        std::string m_account_number;        ///< unique account identifier
        std::string m_pin_code;              ///< PIN for ATM authentication
        double      m_account_balance = 0.0; ///< current account balance


    public:

        // =========================
        //     Constructors
        // =========================

        /** @brief default constructor: creates an empty sentinel with `Mode::empty_mode` */
        BankClient()
            : PersistentEntity(Mode::empty_mode)
            , Person("", "", "", "")
        {}


        /**
         * @brief full constructor used by the UI, `decode()`, and factory helpers
         * @param mode             persistence mode
         * @param first_name       first name
         * @param last_name        last name
         * @param email            email address
         * @param phone_num        phone number
         * @param account_number   unique account identifier
         * @param pin_code         PIN for authentication
         * @param account_balance  initial account balance
         */
        BankClient(Mode           mode,
                   std::string    first_name,
                   std::string    last_name,
                   std::string    email,
                   std::string    phone_num,
                   std::string    account_number,
                   std::string    pin_code,
                   double         account_balance)
            : PersistentEntity(mode)
            , Person(std::move(first_name), std::move(last_name), std::move(email), std::move(phone_num))
            , m_account_number (std::move(account_number))
            , m_pin_code       (std::move(pin_code))
            , m_account_balance(account_balance)
        {}


        // =========================
        //     Factory helpers
        // =========================


        /** @brief creates an empty sentinel object equivalent to default construction */
        static BankClient make_empty()
        {
            return BankClient{};
        }

        /**
         * @brief creates a shell in `add_mode` with only the account number set
         * @param account_number unique account identifier for the new client
         * @note caller fills in the remaining fields via `read_client_info()`
         */
        static BankClient make_new(std::string account_number)
        {
            return BankClient(Mode::add_mode, "", "", "", "", std::move(account_number), "", 0.0);
        }


        // =========================
        //    Required CRTP hooks
        // =========================


        /** @brief returns the file path for client records */
        [[nodiscard]] static std::string_view file_name()
        {
            return terminal_utils::config::CLIENTS_FILE_NAME;
        }


        /**
         * @brief deserialize a line from the file into a `BankClient` object
         * @note line format: `first#//#last#//#email#//#phone#//#account#//#pin#//#balance`
         */
        [[nodiscard]] static BankClient decode(const std::string& line)
        {
            auto cd = stutl::String::split(line, SEPARATOR);

            return BankClient(
                Mode::update_mode,                                                   // loaded from file -> ready to update
                cd[0],                                                               // first_name
                cd[1],                                                               // last_name
                cd[2],                                                               // email
                cd[3],                                                               // phone
                cd[4],                                                               // account_number
                utils::decrypt_text(cd[5], terminal_utils::config::CIPHER_SHIFT),    // pin(decrypted)
                std::stod(cd[6])                                                     // balance
            );
        }


        /** @brief serialize this object into a file line, encrypting the PIN */
        [[nodiscard]] std::string encode() const
        {
            return m_first_name                                                            + std::string(SEPARATOR)
                 + m_last_name                                                             + std::string(SEPARATOR)
                 + m_email                                                                 + std::string(SEPARATOR)
                 + m_phone_num                                                             + std::string(SEPARATOR)
                 + m_account_number                                                        + std::string(SEPARATOR)
                 + utils::encrypt_text(m_pin_code, terminal_utils::config::CIPHER_SHIFT) + std::string(SEPARATOR)
                 + std::to_string(m_account_balance);
        }


        /** @brief returns the account number as the unique key */
        [[nodiscard]] std::string key() const
        {
            return m_account_number;
        }


        /** @brief returns `true` if this record's account number matches `k` */
        [[nodiscard]] bool matches_key(const std::string& k) const
        {
            return m_account_number == k;
        }


        /**
         * @brief sort records by account number ascending
         * @param records the records to sort in place
         */
        static void sort(std::vector<BankClient>& records)
        {
            // `stable_sort()` preserves the relative order of records with equal rates,
            // allowing users to rely on consistent ordering between saves
            std::stable_sort(records.begin(), records.end(), [](const BankClient& a, const BankClient& b)
            {
                return a.m_account_number < b.m_account_number;
            });
        }


        // =========================
        //         find()
        // =========================


        /**
         * @brief find a client by account number
         * @param account_number the account number to search for
         * @return the matching client, or `std::nullopt` if not found
         */
        [[nodiscard]] static std::optional<BankClient> find(const std::string& account_number)
        {
            for(auto& c : load_all())
                if(c.m_account_number == account_number)
                    return c;

            return std::nullopt;
        }


        /**
         * @brief find a client by account number and PIN; used for ATM authentication
         * @param account_number the account number to search for
         * @param pin_code       the PIN to match
         * @return the matching client, or `std::nullopt` if credentials do not match
         */
        [[nodiscard]] static std::optional<BankClient> find(const std::string& account_number, const std::string& pin_code)
        {
            for(auto& c : load_all())
                if(c.m_account_number == account_number && c.m_pin_code == pin_code)
                    return c;

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

            if(_mode == Mode::add_mode && BankClient::exists(m_account_number))
                return SaveResult::failed_account_exists;

            // Delegate to PersistentEntity::save() for the actual I/O
            if(!save())
                return SaveResult::failed_cannot_update;

            return SaveResult::succeeded;
        }


        // =========================
        //     Business logic
        // =========================


        /**
         * @brief deposit an amount into this account
         * @param amount the amount to deposit (must be positive and within balance cap)
         * @return `true` on success, `false` if amount is invalid or save fails
         */
        bool deposit(double amount)
        {
            if(amount <= 0 || amount > terminal_utils::config::MAXIMUM_ALLOWED_BALANCE_PER_CLIENT - m_account_balance)
                return false;

            m_account_balance += amount;
            set_mode(Mode::update_mode);
            return save_with_result() == SaveResult::succeeded;
        }


        /**
         * @brief withdraw an amount from this account
         * @param amount the amount to withdraw (must be positive and not exceed balance)
         * @return `true` on success, `false` if amount is invalid or save fails
         */
        bool withdraw(double amount)
        {
            if(amount <= 0 || amount > m_account_balance)
                return false;

            m_account_balance -= amount;
            set_mode(Mode::update_mode);
            return save_with_result() == SaveResult::succeeded;
        }


        /**
         * @brief transfer an amount from this account to a destination account
         * @param amount      the amount to transfer
         * @param destination the recipient client
         * @return `TransferResult` indicating success or the exact failure reason
         * @note if deposit to destination fails, attempts to roll back the withdrawal
         */
        TransferResult transfer(double amount, BankClient& destination)
        {
            if(destination.m_account_balance + amount > terminal_utils::config::MAXIMUM_ALLOWED_BALANCE_PER_CLIENT)
                return TransferResult::recipient_cap_exceeded;

            if(!withdraw(amount))
                return TransferResult::save_failed;

            if(!destination.deposit(amount))
            {
                // Attempt rollback
                if(!deposit(amount))
                {
                    const std::string message = "CRITICAL: Transfer rollback failed for client("
                                                + full_name()
                                                + "), Amount: "
                                                + std::to_string(amount);
                    LOG_EXCEPTION(message);
                    return TransferResult::critical_rollback_failed;
                }

                return TransferResult::save_failed;
            }

            return TransferResult::success;
        }

    // ----------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------
    public:

        [[nodiscard]] static constexpr std::string_view mode_name(Mode m) noexcept
        {
            switch(m)
            {
                case Mode::empty_mode:  return "empty";
                case Mode::add_mode:    return "add";
                case Mode::update_mode: return "update";
                case Mode::delete_mode: return "delete";
                default:                return "";
            }
        }

    // ----------------------------------------------------------
    // UI: input
    // ----------------------------------------------------------
    public:

        // Prompt for a unique(non-existing) account number
        static std::string get_unique_account_num()
        {
            std::string account_num = terminal_utils::input::get_string("Enter client account number: ", "");

            while(BankClient::exists(account_num))
            {
                std::cout << "\nAccount number already in use, choose another.\n";
                account_num = terminal_utils::input::get_string("Enter client account number: ", "");
            }

            return account_num;
        }

        // Prompt for an existing account number
        static std::string get_valid_account_num()
        {
            std::string account_num = terminal_utils::input::get_string("Enter client account number: ", "");

            while(!BankClient::exists(account_num))
            {
                std::cout << "\nNot a valid account number.\n";
                account_num = terminal_utils::input::get_string("Enter client account number: ", "");
            }

            return account_num;
        }

        static void read_client_info(BankClient& client, std::string_view header)
        {
            std::cout << "\n\n" << header;
            std::cout << "\n---------------------\n";

            std::cout << "\nEnter First Name: ";
            client.m_first_name = terminal_utils::input::get_string();

            std::cout << "\nEnter Last Name: ";
            client.m_last_name = terminal_utils::input::get_string();

            std::cout << "\nEnter Email: ";
            client.m_email = terminal_utils::input::get_string();

            std::cout << "\nEnter Phone: ";
            client.m_phone_num = terminal_utils::input::get_string();

            std::cout << "\nEnter Pin Code: ";
            client.m_pin_code = terminal_utils::input::get_string();

            std::cout << "\nEnter Account Balance: ";
            client.m_account_balance = terminal_utils::input::get_number<double>();
        }

    // ----------------------------------------------------------
    // UI: display
    // ----------------------------------------------------------
    public:

        void print_client_details() const
        {
            terminal_utils::output::Table table;
            table.add_row({"first_name", "last_name", "email", "phone_num", "account_number", "pin_code", "account_balance", "mode"});
            table.add_row({m_first_name, m_last_name, m_email, m_phone_num, m_account_number, m_pin_code,
                           std::to_string(m_account_balance), std::string(mode_name(_mode))});
            table.print(terminal_utils::output::Alignment::Center);
        }

        static void list_clients()
        {
            auto clients = load_all();

            if(clients.empty())
            {
                std::cout << "No clients in the system.\n";
                return;
            }

            terminal_utils::output::Table table;
            table.add_row({"first_name", "last_name", "email", "phone_num", "account_number", "pin_code", "account_balance", "mode"});

            for(const BankClient& c : clients)
                table.add_row_owned({c.m_first_name, c.m_last_name, c.m_email, c.m_phone_num,
                                     c.m_account_number, c.m_pin_code,
                                     std::to_string(c.m_account_balance),
                                     std::string(mode_name(c._mode))});

            const std::string label = "List of(" + std::to_string(clients.size()) + ") Client(s):";
            constexpr size_t  PADDING = 4;
            std::cout << terminal_utils::output::print_aligned(label, label.size() + PADDING,
                                                               terminal_utils::output::Alignment::Right) << "\n";
            table.print(terminal_utils::output::Alignment::Center);
        }

    // ----------------------------------------------------------
    // UI: CRUD actions
    // ----------------------------------------------------------
    public:

        static void add_client()
        {
            BankClient client = BankClient::make_new(get_unique_account_num());

            terminal_utils::platform::clear_terminal();
            read_client_info(client, "Add Client Info:");

            switch(client.save_with_result())
            {
                case SaveResult::succeeded:
                    terminal_utils::platform::clear_terminal();
                    std::cout << "Account added successfully:\n";
                    client.print_client_details();
                    break;

                default:
                    terminal_utils::platform::clear_terminal();
                    std::cout << bold(underline(terminal_utils::red("An error occurred"))) << " while saving the file.\n";
                    break;
            }
        }

        static void update_client()
        {
            auto opt = BankClient::find(get_valid_account_num());
            if(!opt) return;

            BankClient client = *opt;

            terminal_utils::platform::clear_terminal();
            client.print_client_details();
            read_client_info(client, "Update Client Info:");

            client.set_mode(Mode::update_mode);

            switch(client.save_with_result())
            {
                case SaveResult::succeeded:
                    terminal_utils::platform::clear_terminal();
                    std::cout << "Account updated successfully:\n";
                    client.print_client_details();
                    break;

                default:
                    terminal_utils::platform::clear_terminal();
                    std::cout << bold(underline(terminal_utils::red("An error occurred"))) << " couldn't update the client.\n";
                    break;
            }
        }

        static void delete_client()
        {
            std::string account_num = get_valid_account_num();
            auto        opt         = BankClient::find(account_num);

            if(!opt)
                return;

            BankClient client = *opt;

            terminal_utils::platform::clear_terminal();
            client.print_client_details();

            if(!terminal_utils::input::get_yes_no("\n\nAre you sure you want to delete this client?(y/n):"))
            {
                std::cout << "\nDelete operation canceled.\n";
                return;
            }

            client.set_mode(Mode::delete_mode);
            client.save();

            terminal_utils::platform::clear_terminal();
            std::cout << "Account(" << account_num << ") deleted successfully.\n";
        }

    // ----------------------------------------------------------
    // UI: transactions
    // ----------------------------------------------------------
    public:

        static void ui_deposit()
        {
            auto opt = BankClient::find(get_valid_account_num());
            if(!opt) return;

            BankClient client = *opt;

            terminal_utils::platform::clear_terminal();
            client.print_client_details();

            double amount = terminal_utils::input::get_number_in_range<double>(
                "Enter amount to deposit: ",
                "Invalid amount.",
                0.001,
                terminal_utils::config::MAXIMUM_ALLOWED_BALANCE_PER_CLIENT - client.m_account_balance);

            std::cout << "New balance will be: " << client.m_account_balance + amount << "\n";

            if(!terminal_utils::input::get_yes_no("Confirm? "))
            {
                std::cout << "Operation aborted.\n";
                return;
            }

            terminal_utils::platform::clear_terminal();
            if(client.deposit(amount))
            {
                std::cout << "Deposit successful.\n";
                client.print_client_details();
            }
            else
                std::cout << bold(underline(terminal_utils::red("Deposit failed.\n")));
        }

        static void ui_withdraw()
        {
            auto opt = BankClient::find(get_valid_account_num());
            if(!opt) return;

            BankClient client = *opt;

            terminal_utils::platform::clear_terminal();
            client.print_client_details();

            double amount = terminal_utils::input::get_number_in_range<double>(
                "Enter amount to withdraw: ",
                "Invalid amount.",
                0.001,
                client.m_account_balance);

            std::cout << "New balance will be: " << client.m_account_balance - amount << "\n";

            if(!terminal_utils::input::get_yes_no("Confirm? "))
            {
                std::cout << "Operation aborted.\n";
                return;
            }

            terminal_utils::platform::clear_terminal();
            if(client.withdraw(amount))
            {
                std::cout << "Withdrawal successful.\n";
                client.print_client_details();
            }
            else
                std::cout << bold(underline(terminal_utils::red("Withdrawal failed.\n")));
        }

        static void ui_transfer()
        {
            std::cout << "From account:\n";
            auto from_opt = BankClient::find(get_valid_account_num());
            if(!from_opt) return;

            std::cout << "To account:\n";
            auto to_opt = BankClient::find(get_valid_account_num());
            if(!to_opt) return;

            BankClient from = *from_opt;
            BankClient to   = *to_opt;

            if(from.m_account_number == to.m_account_number)
            {
                std::cout << underline(terminal_utils::red(terminal_utils::bold("Can't transfer to the same account.\n")));
                return;
            }

            terminal_utils::platform::clear_terminal();
            from.print_client_details();
            to.print_client_details();

            double amount = terminal_utils::input::get_number_in_range<double>(
                "Enter amount to transfer: ",
                "Invalid amount.",
                0.001,
                from.m_account_balance);

            std::cout << "(" << from.full_name() << ") new balance: "
                      << terminal_utils::red(std::to_string(from.m_account_balance - amount)) << "\n";
            std::cout << "(" << to.full_name() << ") new balance: "
                      << terminal_utils::green(std::to_string(to.m_account_balance + amount)) << "\n";

            if(!terminal_utils::input::get_yes_no("Confirm transfer?(y/n)"))
            {
                std::cout << "Transfer canceled.\n";
                return;
            }

            terminal_utils::platform::clear_terminal();

            switch(from.transfer(amount, to))
            {
                case TransferResult::success:
                    std::cout << terminal_utils::green("Transfer successful.\n");
                    break;

                case TransferResult::recipient_cap_exceeded:
                    std::cout << terminal_utils::red("Transfer failed: would exceed recipient's maximum balance.\n");
                    break;

                case TransferResult::critical_rollback_failed:
                    std::cout << bold(terminal_utils::red("CRITICAL: rollback failed: contact customer support.\n"));
                    break;

                case TransferResult::save_failed:
                    std::cout << bold(underline(terminal_utils::red("Transfer failed: could not save transaction.\n")));
                    break;
            }
        }
};
