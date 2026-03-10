// bank/transaction_log.hpp
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <algorithm>

#include "../../tu/config.hpp"
#include "../../utils/date.hpp"
#include "../persistence/persistent_entity.hpp"

namespace bank
{
    /**
     * @brief represents a single transaction record in the audit log
     *
     * each record captures who did what, to which account(s), how much, and when
     * records are append-only; never updated or deleted
     *
     * file format:
     * `timestamp#//#type#//#source_account#//#destination_account#//#amount#//#performed_by`
     */
    class TransactionLog : public PersistentEntity<TransactionLog>
    {
        public:

            /**
             * @brief the type of transaction performed
             */
            enum class Type
            {
                Deposit,
                Withdraw,
                Transfer
            };


            // =========================
            //     Constructors
            // =========================


            /** @brief default constructor: creates an empty sentinel */
            TransactionLog()
                : PersistentEntity(Mode::empty_mode)
            {}

            /**
             * @brief full constructor used by `decode()` and `record()`
             * @param mode                 persistence mode
             * @param timestamp            date and time the transaction occurred
             * @param type                 deposit, withdraw, or transfer
             * @param source_account       account number funds were taken from
             * @param destination_account  account number funds were sent to (empty for deposit/withdraw)
             * @param amount               transaction amount
             * @param performed_by         username of the bank employee who registered the transaction
             */
            TransactionLog(Mode        mode,
                           std::string timestamp,
                           Type        type,
                           std::string source_account,
                           std::string destination_account,
                           double      amount,
                           std::string performed_by)
                : PersistentEntity(mode)
                , m_timestamp           (std::move(timestamp))
                , m_type                (type)
                , m_source_account      (std::move(source_account))
                , m_destination_account (std::move(destination_account))
                , m_amount              (amount)
                , m_performed_by        (std::move(performed_by))
            {}


            // =========================
            //     Factory helpers
            // =========================


            static constexpr std::string_view NO_DESTINATION = "N/A";

            /**
             * @brief creates and immediately saves a deposit record
             * @param source_account account that received the deposit
             * @param amount         deposited amount
             * @param performed_by   username of the employee
             */
            static void record_deposit(const std::string& source_account,
                                       double amount,
                                       const std::string& performed_by)
            {
                TransactionLog log(Mode::add_mode,
                    utils::Date::timestamp(),
                    Type::Deposit,
                    source_account,
                    std::string(NO_DESTINATION),
                    amount,
                    performed_by);
                log.save();
            }


            /**
             * @brief creates and immediately saves a withdraw record
             * @param source_account account that was withdrawn from
             * @param amount         withdrawn amount
             * @param performed_by   username of the employee
             */
            static void record_withdraw(const std::string& source_account,
                                        double amount,
                                        const std::string& performed_by)
            {
                TransactionLog log(Mode::add_mode,
                    utils::Date::timestamp(),
                    Type::Withdraw,
                    source_account,
                    std::string(NO_DESTINATION),
                    amount,
                    performed_by);
                log.save();
            }


            /**
             * @brief creates and immediately saves a transfer record
             * @param source_account      account funds were taken from
             * @param destination_account account funds were sent to
             * @param amount              transferred amount
             * @param performed_by        username of the employee
             */
            static void record_transfer(const std::string& source_account,
                                        const std::string& destination_account,
                                        double amount,
                                        const std::string& performed_by)
            {
                TransactionLog log(Mode::add_mode,
                    utils::Date::timestamp(),
                    Type::Transfer,
                    source_account,
                    destination_account,
                    amount,
                    performed_by);
                log.save();
            }

            // =========================
            //    Required Cache hooks
            // =========================


            [[nodiscard]] static constexpr std::string_view class_name() { return "TransactionLog"; }


            // =========================
            //    Required CRTP hooks
            // =========================


            /** @brief returns the file path for transaction records */
            [[nodiscard]] static std::string_view file_name()
            {
                return tu::config::TRANSACTIONS_FILE_NAME;
            }
            

            /**
             * @brief deserialize a line from the file into a `TransactionLog` object
             * @note format: `timestamp#//#type#//#source#//#destination#//#amount#//#performed_by`
             */
            [[nodiscard]] static TransactionLog decode(const std::string& line)
            {
                auto td = stutl::String::split(line, SEPARATOR);

                return TransactionLog(
                    Mode::update_mode,
                    td[0],                            // timestamp
                    type_from_string(td[1]),          // type
                    td[2],                            // source_account
                    td[3],                            // destination_account
                    std::stod(td[4]),                 // amount
                    td[5]                             // performed_by
                );
            }


            /** @brief serialize this record into a file line */
            [[nodiscard]] std::string encode() const
            {
                return m_timestamp                    + std::string(SEPARATOR)
                    + type_to_string(m_type)          + std::string(SEPARATOR)
                    + m_source_account                + std::string(SEPARATOR)
                    + m_destination_account           + std::string(SEPARATOR)
                    + std::to_string(m_amount)        + std::string(SEPARATOR)
                    + m_performed_by;
            }


            /** @brief returns the timestamp as the unique key */
            [[nodiscard]] std::string key() const
            {
                return m_timestamp + m_source_account; // timestamp + source makes a unique key
            }


            /** @brief matches by timestamp + source account */
            [[nodiscard]] bool matches_key(std::string_view k) const
            {
                return (m_timestamp + m_source_account) == k;
            }


            /** @brief transaction log is *append-only*; no sorting needed */
            static void sort(std::vector<TransactionLog>&) {}


            // =========================
            //         Helpers
            // =========================

            [[nodiscard]] static std::string type_to_string(Type t)
            {
                switch(t)
                {
                    case Type::Deposit:  return "Deposit";
                    case Type::Withdraw: return "Withdraw";
                    case Type::Transfer: return "Transfer";
                    default:             return "Unknown";
                }
            }


            [[nodiscard]] static Type type_from_string(std::string_view s)
            {
                if(s == "Deposit")  return Type::Deposit;
                if(s == "Withdraw") return Type::Withdraw;
                return Type::Transfer;
            }


            // =========================
            //            UI
            // =========================


            /** @brief print all transaction records */
            static void list_all()
            {
                const auto& logs = load_all();

                if(logs.empty())
                {
                    std::cout << "No transactions recorded.\n";
                    return;
                }

                m_print_table(logs);
            }


            /** @brief print all transactions for a specific account number */
            static void list_by_account(const std::string& account_number)
            {
                const auto& all = load_all();

                std::vector<const TransactionLog*> filtered;
                for(const auto& log : all)
                    if(log.m_source_account == account_number ||
                       log.m_destination_account == account_number)
                        filtered.push_back(&log);

                if(filtered.empty())
                {
                    std::cout << "No transactions found for account (" << account_number << ").\n";
                    return;
                }

                // collect into a temporary vector for printing
                std::vector<TransactionLog> result;
                result.reserve(filtered.size());
                for(const auto* p : filtered)
                    result.push_back(*p);

                m_print_table(result);
            }


        private:
            /** @brief renders a vector of transaction records as a formatted table */
            static void m_print_table(const std::vector<TransactionLog>& logs)
            {
                tu::output::Table table;
                table.add_row({"timestamp", "type", "source", "destination", "amount", "performed_by"});

                for(const auto& log : logs)
                    table.add_row(tu::output::Table::own,
                    {
                        log.m_timestamp,
                        type_to_string(log.m_type),
                        log.m_source_account,
                        log.m_destination_account,
                        std::to_string(log.m_amount),
                        log.m_performed_by
                    });

                const std::string label = "Transaction History (" + std::to_string(logs.size()) + "):";
                constexpr size_t PADDING = 4;
                std::cout << tu::output::print_aligned(label, label.size() + PADDING,
                                                       tu::output::Alignment::Right) << "\n";
                table.print(tu::output::Alignment::Center);
            }


            // =========================
            //           Data
            // =========================

            std::string m_timestamp;            ///< date and time the transaction was recorded
            Type        m_type;                 ///< deposit, withdraw, or transfer
            std::string m_source_account;       ///< account funds were taken from or deposited into
            std::string m_destination_account;  ///< account funds were sent to (empty for deposit/withdraw)
            double      m_amount = 0.0;         ///< transaction amount
            std::string m_performed_by;         ///< username of the bank employee who registered it
    };

} // namespace bank
