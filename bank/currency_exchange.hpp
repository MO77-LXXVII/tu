// currency_exchange.hpp

#pragma once
#include <vector>
#include <fstream>
#include <limits>

#include "person.hpp"
#include "../terminal_utils/output.hpp"
#include "../terminal_utils/config.hpp"
#include "../terminal_utils/style_wrappers.hpp"
#include "../terminal_utils/input.hpp"
#include "../platform/platform.hpp"
#include "../utils/string_utils.hpp"
#include "../utils/utils.hpp"
#include "bank/persistent_entity.hpp"


// ============================================================
// CurrencyExchange
// Inherits shared persistence from PersistentEntity<CurrencyExchange>
// Only implements the 5 required CRTP hooks + its own business logic
// ============================================================


/**
 * @brief represents a currency exchange rate entry
 * 
 * stores the exchange rate of a currency relative to USD, along with
 * the country and currency name. multiple countries can share the same
 * currency code (e.g. "USD" for both the US and Ecuador).
 * 
 * use `find_all()` instead of `find()` when multiple matches are possible,
 * and `select_from_matches()` to let the user pick one.
 */
class CurrencyExchange: public PersistentEntity<CurrencyExchange>
{
    public:

        // ignore IntelliSense scoping issue 
        // it gets confused about the scope of nested enums inside CRTP templates
        /**
         * @brief return type for `save_with_result()` indicates success or the exact failure reason
         * 
         * - `succeeded`                      record was saved successfully
         * 
         * - `failed_empty_object`            attempted to save an empty record
         * 
         * - `failed_currency_exists`         currency code already exists (add mode only)
         * 
         * - `failed_cannot_update_currency`  underlying I/O failure
        */
        enum class SaveResult
        {
            succeeded,
            failed_empty_object,
            failed_currency_exists,
            failed_cannot_update_currency
        };


        // =========================
        //         Data
        // =========================

    protected:
        std::string m_country;         ///< country name
        std::string m_currency_code;   ///< ISO currency code  (e.g. "EUR")
        std::string m_currency_name;   ///< full currency name (e.g. "Euro")
        double      m_rate{};          ///< exchange rate relative to USD


    public:

        // =========================
        //     Constructors
        // =========================

        /** @brief default constructor creates an empty sentinel object with `Mode::empty_mode` */
        CurrencyExchange()
            : PersistentEntity(Mode::empty_mode)
        {}


        /**
         * @brief full constructor used by the UI, `decode()`, and factory helpers
         * @param mode          persistence mode
         * @param country       country name
         * @param currency_code ISO currency code
         * @param currency_name full currency name
         * @param rate          exchange rate relative to USD
         */
        CurrencyExchange(
                Mode        mode,
                std::string country,
                std::string currency_code,
                std::string currency_name,
                double       rate):
                PersistentEntity(mode),
                m_country(country),
                m_currency_code(currency_code),
                m_currency_name(currency_name),
                m_rate(rate)
        {}


        // =========================
        //     Factory helpers
        // =========================


        /** @brief creates an empty sentinel object: equivalent to default construction */
        static CurrencyExchange make_empty()
        {
            return CurrencyExchange{};
        }


        /**
         * @brief creates a shell in `add_mode` with only the currency code set
         * @param currency_code ISO currency code for the new entry
         * @note caller fills in the remaining fields via `read_currency_info()`
         */
        static CurrencyExchange make_new(std::string currency_code)
        {
            return CurrencyExchange(Mode::add_mode, "", std::move(currency_code), "", 0);
        }


        // =========================
        //    Required CRTP hooks
        // =========================


        /** @brief returns the file path for currency exchange records */
        [[nodiscard]] static std::string_view file_name()
        {
            return terminal_utils::config::CURRENCY_EXCHANGE_FILE_NAME;
        }


        /**
         * @brief deserialize a line from the file into a `CurrencyExchange` object
         * @note line format: `country#//#currency_code#//#currency_name#//#rate`
         */
        [[nodiscard]] static CurrencyExchange decode(const std::string& line)
        {
            auto cd = stutl::String::split(line, SEPARATOR);

            return CurrencyExchange(
                Mode::update_mode,      // loaded from file → ready to update
                cd[0],                  // m_country
                cd[1],                  // m_currency_code
                cd[2],                  // m_currency_name
                std::stod(cd[3])        // m_rate
            );
        }


        /** @brief serialize this object into a file line */
        [[nodiscard]] std::string encode() const
        {
            return m_country              + std::string(SEPARATOR)
                 + m_currency_code        + std::string(SEPARATOR)
                 + m_currency_name        + std::string(SEPARATOR)
                 + std::to_string(m_rate);
        }


        /** @brief returns the currency code as the unique key */
        [[nodiscard]] std::string key() const
        {
            return m_currency_code;
        }


        /** @brief returns `true` if this record's currency code matches `k` */
        [[nodiscard]] bool matches_key(std::string_view k) const
        {
            return m_currency_code == k;
        }


        /**
         * @brief sort records by exchange rate ascending, with currency code as tie-breaker
         * @param records the records to sort in place
         */
        static void sort(std::vector<CurrencyExchange>& records)
        {
            constexpr double epsilon = 1e-6;

            // `stable_sort()` preserves the relative order of records with equal rates,
            // allowing users to rely on consistent ordering between saves
            std::stable_sort(records.begin(), records.end(), [](const CurrencyExchange& a, const CurrencyExchange& b)
            {
                double diff = a.m_rate - b.m_rate;

                if(fabs(diff) > epsilon)
                    return diff < 0.0;

                // tie-breaker to preserve strict ordering
                return a.m_currency_code < b.m_currency_code;
            });
        }


        // =========================
        //         find()
        // =========================

        /**
         * @brief find a single currency by code
         * @param currency_code ISO currency code to search for
         * @return the first matching record, or `std::nullopt` if not found
         */
        [[nodiscard]] static std::optional<CurrencyExchange> find(const std::string& currency_code)
        {
            for(const auto& u : load_all())
                if(u.m_currency_code == currency_code)
                    return u;

            return std::nullopt;
        }


        /**
         * @brief find all currencies sharing the same code (multiple countries can share a code)
         * @param currency_code ISO currency code to search for
         * @return all matching records, or `std::nullopt` if none found
         */
        [[nodiscard]] static std::optional<std::vector<CurrencyExchange>> find_all(const std::string& currency_code)
        {
            std::vector<CurrencyExchange> currencies_with_same_code;
            for(const auto& u : load_all())
                if(u.m_currency_code == currency_code)
                    currencies_with_same_code.push_back(u);

            if(currencies_with_same_code.empty())
                return std::nullopt;

            return currencies_with_same_code;
        }


        // =========================
        // Helpers
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

            if(_mode == Mode::add_mode && CurrencyExchange::exists(m_currency_code))
                return SaveResult::failed_currency_exists;

            // Delegate to PersistentEntity::save() for the actual I/O
            if(!save())
                return SaveResult::failed_cannot_update_currency;

            return SaveResult::succeeded;
        }


        /** @brief returns the string representation of a `Mode` value */
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


        /** @brief returns the ISO currency code of this record */
        std::string get_currency_code() const
        {
            return m_currency_code;
        }


        // =========================
        //    UI input methods
        // =========================


        /**
         * @brief prompts the user for a currency code, loops until a valid one is entered
         * @param msg prompt message
         * @return a valid, uppercased ISO currency code that exists in the system
         */
        static std::string get_valid_currency_code(std::string_view msg = "Enter currency code: ")
        {
            while(true)
            {
                std::string currency_code = terminal_utils::input::get_string(msg, "");

                std::transform(currency_code.begin(), currency_code.end(), currency_code.begin(), [](unsigned char ch)
                {
                    return static_cast<char>(std::toupper(ch));
                });

                if(CurrencyExchange::exists(currency_code))
                    return currency_code;

                std::cout << "\nNot a valid currency code.\n";
            }
        }


        /**
         * @brief prompts the user to fill in all fields of a `CurrencyExchange` object
         * @param currency the object to populate
         * @param header   header text to display above the input form
         */
        static void read_currency_info(CurrencyExchange& currency, std::string_view header)
        {
            std::cout << "\n\n" << header;
            std::cout << "\n---------------------\n";

            std::cout << "\nEnter Country Name: ";
            currency.m_country = terminal_utils::input::get_string();

            std::cout << "\nEnter the Country's Currency Name: ";
            currency.m_currency_name = terminal_utils::input::get_string();

            std::cout << "\nEnter The Exchange Rate To USD: ";
            currency.m_rate = terminal_utils::input::get_number<double>();
        }


        /**
         * @brief prompts the user to enter a new exchange rate for an existing currency
         * @param currency the object whose rate will be updated
         * @param header   header text to display above the input form
         */
        static void read_new_currency_rate(CurrencyExchange& currency, std::string_view header)
        {
            std::cout << "\nEnter The Exchange Rate To USD: ";
            currency.m_rate = terminal_utils::input::get_number<double>();
        }


        // =========================
        //    UI display methods
        // =========================


        /**
         * @brief print this record's details as a formatted table
         * @param show_index whether to include an index column
         * @param index      index value to display (only used if `show_index` is `true`)
         */
        void print_currency_details(bool show_index = false, int index = 0) const
        {
            terminal_utils::output::Table table;

            if(show_index)
            {
                table.add_row({"Index", "Country", "Currency Code", "Currency Name", "Exchange Rate"});
                table.add_row({std::to_string(index), m_country, m_currency_code, m_currency_name, std::to_string(m_rate)});
            }
            else
            {
                table.add_row({"Country", "Currency Code", "Currency Name", "Exchange Rate"});
                table.add_row({m_country, m_currency_code, m_currency_name, std::to_string(m_rate)});
            }

            table.print(terminal_utils::output::Alignment::Center);
        }


        /**
         * @brief print a list of currency records as a formatted table
         * @param currencies_with_same_code records to display
         * @param show_index                whether to include an index column
         */
        static void print_currency_details(std::vector<CurrencyExchange> currencies_with_same_code, bool show_index = false)
        {
            terminal_utils::output::Table table;

            if(show_index)
                table.add_row({"Index", "Country", "Currency Code", "Currency Name", "Exchange Rate"});
            else
                table.add_row({"Country", "Currency Code", "Currency Name", "Exchange Rate"});

            for(size_t i = 0; i < currencies_with_same_code.size(); ++i)
            {
                const auto& c = currencies_with_same_code[i];
                if(show_index)
                    table.add_row_owned({std::to_string(i + 1), c.m_country, c.m_currency_code, c.m_currency_name, std::to_string(c.m_rate)});
                else
                    table.add_row_owned({c.m_country, c.m_currency_code, c.m_currency_name, std::to_string(c.m_rate)});
            }

            const std::string label = "List of (" + std::to_string(currencies_with_same_code.size()) + (currencies_with_same_code.size() == 1 ? ") currency:" : ") currencies:");

            constexpr size_t PADDING = 4;
            std::cout << terminal_utils::output::print_aligned(label, label.size() + PADDING, terminal_utils::output::Alignment::Right) << "\n";
            table.print(terminal_utils::output::Alignment::Center);
        }

        /** @brief print all currency records in the system, or a message if none exist */
        static void list_available_currencies()
        {
            const auto& currencies = load_all();

            if(currencies.empty())
            {
                std::cout << "No currencies found in the system.\n";
                return;
            }

            print_currency_details(currencies);
        }


        // =========================
        //    UI CRUD operations
        // =========================


        /** @brief prompt the user to add a new currency entry and save it */
        static void add_currency()
        {
            CurrencyExchange user = CurrencyExchange::make_new(get_valid_currency_code());

            terminal_utils::platform::clear_terminal();
            read_currency_info(user, "Add Currency Data:");

            switch(user.save_with_result())
            {
                case SaveResult::succeeded:
                    terminal_utils::platform::clear_terminal();
                    std::cout << "Currency data added successfully:\n";
                    user.print_currency_details();
                    break;

                default:
                    terminal_utils::platform::clear_terminal();
                    std::cout << bold(underline(terminal_utils::red("An error occurred"))) << " while saving the file.\n";
                    break;
            }
        }


        /**
         * @brief if multiple records match, prompt the user to pick one; otherwise return the only match
         * @param matches list of matching records
         * @return the selected `CurrencyExchange` record
        */
        static CurrencyExchange select_from_matches(const std::vector<CurrencyExchange>& matches)
        {
            if(matches.size() == 1) // single element
                return matches[0];

            // multiple elements
            print_currency_details(matches, true);

            const int choice = terminal_utils::input::get_number_in_range<int>(
                "Choose which country: ", "Invalid option, try again: ", 1, matches.size());

            return matches[choice - 1];
        }


        /** @brief prompt the user to select and update an existing currency entry */
        static void update_currency()
        {
            auto from_matches = CurrencyExchange::find_all(get_valid_currency_code());

            terminal_utils::platform::clear_terminal();
            CurrencyExchange selected = select_from_matches(*from_matches);

            selected.set_mode(Mode::update_mode);

            switch(selected.save_with_result())
            {
                case SaveResult::succeeded: // ignore IntelliSense false positive (error 2373)
                    terminal_utils::platform::clear_terminal();
                    std::cout << "currency data updated successfully:\n";
                    selected.print_currency_details();
                    break;

                default:
                    terminal_utils::platform::clear_terminal();
                    std::cout << bold(underline(terminal_utils::red("An error occurred"))) << " couldn't update the currency data.\n";
                    break;
            }
        }


        /**
         * @brief convert an amount from this currency to USD
         * @param amount the amount to convert
         * @return equivalent value in USD
         */
        double to_USD(double amount) const
        {
            return amount / this->m_rate;
        }


        /**
         * @brief convert an amount from USD to this currency
         * @param amount the amount in USD
         * @return equivalent value in this currency
         */
        double from_USD(double amount) const
        {
            return amount * this->m_rate;
        }


        /** @brief prompt the user to convert an amount between two currencies */
        static void convert_currency()
        {
            // `get_valid_currency_code()` guarantees existence
            auto from_matches = CurrencyExchange::find_all(get_valid_currency_code("currency code to convert from: "));
            auto to_matches = CurrencyExchange::find_all(get_valid_currency_code("currency code to convert to: "));

            terminal_utils::platform::clear_terminal();
            CurrencyExchange source = select_from_matches(*from_matches);

            terminal_utils::platform::clear_terminal();
            CurrencyExchange target = select_from_matches(*to_matches);

            terminal_utils::platform::clear_terminal();

            double input_amount = terminal_utils::input::get_number<double>("Enter amount to exchange: ", "Invalid number: ");

            terminal_utils::platform::clear_terminal();

            const double result = target.from_USD(source.to_USD(input_amount));

            std::cout << input_amount << " " << source.m_currency_code << " = "
                      << result << " " << target.m_currency_code << "\n";
        }
};
