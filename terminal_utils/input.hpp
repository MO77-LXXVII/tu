// input.hpp

#pragma once

#include <iostream>
#include <limits>
#include <string_view>
#include <optional>
#include <cctype>
#include <type_traits>

#include "ansi_colours.hpp"
#include "style_wrappers.hpp"

namespace terminal_utils::input
{
    namespace
    {
        // RAII wrapper for input state management
        class InputGuard
        {
            public:
                explicit InputGuard(bool discard_buffer = true)
                    : old_state_(std::cin.rdstate()), discard_(discard_buffer)
                {}

                ~InputGuard()
                {
                    std::cin.clear(old_state_);

                    if(discard_)
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                }

                InputGuard(const InputGuard&) = delete;
                InputGuard& operator=(const InputGuard&) = delete;

            private:
                std::ios::iostate old_state_;
                bool discard_;
        };
    }

    // Clear input buffer after failed extraction
    inline void clear_failed_input() noexcept
    {
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // Discard rest of line after successful extraction
    inline void discard_rest_of_line() noexcept
    {
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    // Get single character for menu navigation (handles buffering internally)
    inline std::optional<char> get_menu_key() noexcept
    {
        char c;

        if(std::cin.get(c))
        {
            // Only discard if we didn't just read a newline
            if (c != '\n')
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
            // Only discard if we didn't just read a newline
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
    T get_validated_input(const std::string& prompt, const std::string& error_msg, Predicate&& validator)
    {
        while(true)
        {
            if(!prompt.empty())
                std::cout << prompt;


            auto result = try_get_input<T>("");

            if(result && validator(*result))
                return *result;

            std::cout << bold(red(underline(error_msg))) << "\n";
        }
    }

    template <typename T>
    T get_number(std::string_view prompt = "", std::string error_msg = "")
    {
        while (true)
        {
            if(!prompt.empty())
                std::cout << prompt;

            if (auto result = try_get_input<T>(""))
                return *result;
            std::cout << bold(red(underline(error_msg)));
        }
    }

    inline std::string get_string(std::string_view prompt = "", std::string error_msg = "")
    {
        while (true)
        {
            if(!prompt.empty())
                std::cout << prompt;

            if(auto result = try_get_input<std::string>(""))
                return *result;
            std::cout << bold(red(underline(error_msg)));
        }
    }

    // skips leading whitespace
    inline std::string get_line(std::string_view prompt = "", std::string error_msg = "")
    {
        while (true)
        {
            if(!prompt.empty())
                std::cout << prompt;

            std::string line;
            if (std::getline(std::cin >> std::ws, line))
                return line;

            std::cout << bold(red(underline(error_msg)));
            std::cin.clear();
        }
    }

    // Get number in range
    template<typename T>
    T get_number_in_range(const std::string& prompt, const std::string& error_msg, T min, T max)
    {
        return get_validated_input<T>(prompt, error_msg,
            [min, max](const T& value)
            {
                return value >= min && value <= max;
            }
        );
    }

    // Get yes/no decision
    inline bool get_yes_no(const std::string& prompt = "")
    {
        if(!prompt.empty())
            std::cout << magenta(bold(underline(prompt)));

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
            std::cout << "Please enter " << underline(blue("'y'")) <<  " for yes or " << underline(blue("'n'")) << " for no.\n";
        }
    }
} // namespace terminal_utils::input
