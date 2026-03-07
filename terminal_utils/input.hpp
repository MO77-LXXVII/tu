// input.hpp

#pragma once

#include <iostream>
#include <limits>
#include <string_view>
#include <optional>
#include <cctype>
#include <type_traits>

namespace terminal_utils::input
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


    // Get single character for menu navigation (handles buffering internally)
    inline std::optional<char> get_menu_key(terminal_utils::ColouredText prompt) noexcept
    {
        std::cout << prompt;
        char c;

        if(std::cin.get(c))
        {
            // Only discard if we didn't read a newline
            if (c != '\n')
                discard_rest_of_line();

            return c;
        }

        return std::nullopt;
    }


    // For "press Enter to continue" - waits for actual Enter
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


    // Get input with validation predicate
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
     * @brief reads a full line from `stdin`, **skipping leading whitespace**
     *
     * @param prompt     optional prompt to display before reading
     * @param error_msg  optional message to display on recoverable input failure
     *
     * @return the line read, or `std::nullopt` on `EOF`
     *
     * @note `std::ws` skips leading whitespace including leftover newlines from previous input
     */
    inline std::optional<std::string> get_line(std::string_view prompt = "", std::string_view error_msg = "")
    {
        while (true)
        {
            if(!prompt.empty())
                std::cout << prompt;

            std::string line;
            if(std::getline(std::cin >> std::ws, line))
                return line;

            if(std::cin.eof())
                return std::nullopt;

            std::cout << error_msg << "\n";

            // clear failbit to allow retry if reached
            // `eofbit` is already handled at this point
            std::cin.clear();
        }
    }

    // Get number in range
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

    // Get yes/no decision
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
            std::cout << "Please enter " << "'y'" <<  " for yes or " << "'n'" << " for no.\n";
        }
    }
} // namespace terminal_utils::input
