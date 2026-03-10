// input.hpp

#pragma once

#include <iostream>
#include <limits>
#include <string_view>
#include <optional>
#include <cctype>
#include <type_traits>

namespace tu::input
{
    namespace
    {
        /**
         * @brief RAII guard that restores `std::cin` state and optionally discards
         *        the remainder of the current line on destruction
         *
         * @note temporarily clears error state to allow `ignore()` to work,
         *       then restores the original stream state; which may include `failbit`
         */
        class InputGuard
        {
            public:
                explicit InputGuard(bool discard_buffer = true)
                    : old_state_(std::cin.rdstate()), discard_(discard_buffer)
                {}

                ~InputGuard()
                {
                    if(discard_)
                    {
                        // Clear any error state temporarily to allow `ignore()` to work
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    }

                    // Restore original state (which might include failbit)
                    std::cin.clear(old_state_);
                }

                InputGuard(const InputGuard&) = delete;
                InputGuard& operator=(const InputGuard&) = delete;

            private:
                std::ios::iostate old_state_;
                bool discard_;
        };
    }


    /**
     * @brief Clears `failbit` and discards the remainder of the current line
     *
     * @note Call after a failed extraction to reset the stream for the next read
     */
    inline void clear_failed_input() noexcept
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }


    /**
     * @brief Discards the remainder of the current line
     *
     * @note Call after a successful extraction to consume any trailing input
     */
    inline void discard_rest_of_line() noexcept
    {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }


    /**
     * @brief Reads a single character from `stdin` for menu navigation
     *
     * @return The character read, or `std::nullopt` on `EOF`
     *
     * @note Discards the remainder of the line unless the character read was `'\n'`
     */
    inline std::optional<char> get_menu_key() noexcept
    {
        char c;

        if(std::cin.get(c))
        {
            // Only discard if we didn't just read a newline
            if(c != '\n')
                discard_rest_of_line();

            return c;
        }

        return std::nullopt;
    }


    /**
     * @brief Reads a single character from `stdin` for menu navigation
     * 
     * @tparam T      any type with `operator<<`
     * @param prompt  Prompt to display before reading (any type with `operator<<`)
     *
     * @return The character read, or `std::nullopt` on `EOF`
     *
     * @note Discards the remainder of the line unless the character read was `'\n'`
     */
    template<typename T>
    inline std::optional<char> get_menu_key(const T& prompt) noexcept
    {
        std::cout << prompt;
        char c;

        if(std::cin.get(c))
        {
            if(c != '\n')
                discard_rest_of_line();

            return c;
        }

        return std::nullopt;
    }


    /**
     * @brief Blocks until the user presses Enter
     *
     * @note Any characters typed before Enter are discarded
     * 
     * @note For "press Enter to continue": waits for actual Enter
     */
    inline void wait_for_enter() noexcept
    {
        discard_rest_of_line();
    }


    /**
     * @brief Attempts to read a value of type T from stdin
     *
     * @note Always discards the remainder of the line after extraction,
     *       whether it succeeds or fails; callers do not need to manually clear the buffer
     */
    template<typename T>
    std::optional<T> try_get_input(std::string_view prompt = "")
    {
        if(!prompt.empty())
            std::cout << prompt;

        InputGuard guard;

        T value;

        if(std::cin >> value)
            return value;

        return std::nullopt;
    }


    /**
     * @brief reads a value of type `T` from `stdin`, retrying until the value satisfies the `Predicate`
     *
     * @tparam T          Type to read
     * @tparam Predicate  callable with signature `bool(const T&)`
     *
     * @param prompt     Optional prompt to display before each read
     * @param error_msg  message to display when validation fails
     * @param validator  predicate to validate the input
     *
     * @return The first value that satisfies the predicate
     */
    template<typename T, typename Predicate>
    T get_validated_input(std::string_view prompt, std::string_view error_msg, Predicate&& validator)
    {
        while(true)
        {
            if(!prompt.empty())
                std::cout << prompt;

            auto result = try_get_input<T>("");

            if(result && validator(*result))
                return *result;

            std::cout << error_msg << "\n";
        }
    }


    /**
     * @brief reads a number of type `T` from `stdin`, retrying until a valid value is read
     *
     * @tparam T numeric type to read
     *
     * @param prompt     optional prompt to display before each read
     * @param error_msg  message to display on invalid input
     *
     * @return The first successfully parsed value
     */
    template <typename T>
    T get_number(std::string_view prompt = "", std::string_view error_msg = "")
    {
        while (true)
        {
            if(!prompt.empty())
                std::cout << prompt;

            if(auto result = try_get_input<T>(""))
                return *result;

            std::cout << error_msg << "\n";
        }
    }


    /**
     * @brief reads a **whitespace-delimited word** from `stdin`, retrying until a valid value is read
     *
     * @param prompt     optional prompt to display before each read
     * @param error_msg  message to display on invalid input
     *
     * @return the first successfully parsed word
     */
    inline std::string get_string(std::string_view prompt = "", std::string_view error_msg = "")
    {
        while (true)
        {
            if(!prompt.empty())
                std::cout << prompt;

            if(auto result = try_get_input<std::string>(""))
                return *result;

            std::cout << error_msg << "\n";
        }
    }


    /**
     * @brief reads a full line from `stdin`
     *
     * @param prompt          optional prompt to display before reading
     * @param error_msg       optional message to display on recoverable input failure
     * @param cancel_on_empty if `true`, returns `std::nullopt` when the user enters only whitespace
     *
     * @return the line read, `std::nullopt` on EOF, or `std::nullopt` if `cancel_on_empty` and input is blank
     */
    inline std::optional<std::string> get_line(std::string_view prompt = "", std::string_view error_msg = "", bool cancel_on_empty = false)
    {
        while(true)
        {
            if(!prompt.empty())
                std::cout << prompt << std::flush;

            // flush leftover '\n' from any previous >> extraction
            // only ignore if there's something in the buffer
            if (std::cin.rdbuf()->in_avail() > 0)
                        tu::input::discard_rest_of_line();

            std::string line;
            if(std::getline(std::cin, line))
            {
                if(cancel_on_empty && line.find_first_not_of(" \t\r\n") == std::string::npos)
                    return std::nullopt;

                return line;
            }

            if(std::cin.eof())
                return std::nullopt;

            std::cout << error_msg << "\n";
            std::cin.clear();
        }
    }


    /**
     * @brief reads a number of type `T` from `stdin`, retrying until the value is within `[min, max]`
     *
     * @tparam T  numeric type to read
     *
     * @param prompt     optional prompt to display before each read
     * @param error_msg  message to display when the value is out of range
     * @param min        minimum accepted value (inclusive)
     * @param max        maximum accepted value (inclusive)
     *
     * @return the first value in the range `[min, max]`
     */
    template<typename T>
    T get_number_in_range(std::string_view prompt, std::string_view error_msg, T min, T max)
    {
        return get_validated_input<T>(prompt, error_msg,
            [min, max](const T& value)
            {
                return value >= min && value <= max;
            }
        );
    }


    /**
     * @brief prompts the user for a yes/no decision, retrying until `'y'` or `'n'` is entered
     *
     * @param prompt  optional prompt to display before reading
     *
     * @return `true` for `'y'`/`'Y'`, `false` for `'n'`/`'N'`
     */
    inline bool get_yes_no(std::string_view prompt = "")
    {
        if(!prompt.empty())
            std::cout << prompt;

        while(true)
        {
            auto result = try_get_input<char>("");

            if(result)
            {
                char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(*result)));

                if(upper == 'Y')
                    return true;

                if(upper == 'N')
                    return false;
            }
            std::cout << "Please enter " << "'y'" << " for yes or " << "'n'" << " for no.\n";
        }
    }
} // namespace tu::input
