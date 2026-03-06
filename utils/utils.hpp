// utils.hpp

#pragma once

#include <string>
#include <limits>
#include <cmath>
#include <random>
#include <iostream>
#include <functional>
#include <type_traits>
#include <utility>

#include <utils/date.hpp>

namespace utils
{
    [[nodiscard]] inline int random_number(int from = 0, int to = 100)
    {
        if (from == to)
            return from;

        static std::random_device rd;
        static std::mt19937 gen(rd());

        if(from > to)
            std::swap(from, to);

        if (from == to)
            return from;

        std::uniform_int_distribution<int> dis(from, to);
        return dis(gen);
    }

    enum class char_type
    {
        small_letter, capital_letter, digits, mixed_chars, special_char
    };

    inline char get_random_character(char_type category)
    {
        // pick a random character either capital/small/digits
        if (category == char_type::mixed_chars)
            category = static_cast<char_type>(random_number(0, 2));

        switch (category)
        {
            case char_type::capital_letter: // A - Z
                return static_cast<char>(random_number('A', 'Z'));

            case char_type::special_char: // ! - /
                return static_cast<char>(random_number('!', '/'));
            
            case char_type::digits: // 0 - 9
                return static_cast<char>(random_number('0', '9'));

            case char_type::small_letter: // a - z
                [[fallthrough]];

            default: // set default to small letters
                return static_cast<char>(random_number('a', 'z'));
        }
    }

    inline std::string generate_word(char_type category, size_t length)
    {
        std::string word(length, '\0');

        for (size_t i = 0; i < length; i++)
            word[i] = get_random_character(category);

        return word;        
    }

    inline std::string generate_key(char_type category = char_type::capital_letter, size_t fields = 4, size_t char_per_field = 4)
    {
        if(fields == 0 || char_per_field == 0)
            return "";

        // "6jLu-U5a4-9SI1-RSi1" for example have 4 fields and 3 delimiters (-)
        const size_t delim = fields - 1;

        const size_t total_size = (fields * char_per_field) + delim;

        std::string word(total_size, '\0');

        for(size_t i = 1; i <= total_size; i++)
        {
            if(i % (char_per_field + 1) == 0)
                word[i-1] = '-';
            else
                word[i-1] = get_random_character(category);
        }

        return word;
    }

    inline void generate_keys(int num_of_keys, char_type category = char_type::capital_letter, size_t fields = 4, size_t char_per_field = 4)
    {
        for(size_t i = 1; i <= num_of_keys; i++)
            std::cout << "Key [" << i << "]: " << generate_key(category, fields, char_per_field) << std::endl;
    }

    // apply static polymorphism via templates and SFINAE to ensure vector<>, array<>, deque<>
    template<typename container, typename generator,
            typename value_type = typename container::value_type,
            typename = std::enable_if_t<std::is_assignable_v<value_type&, decltype(std::declval<generator>()())>>>
    inline void fill_array(container& arr, generator gen)
    {
        for (auto& elem : arr)
            elem = gen();
    }

    // template specialization for C-style arrays
    template<typename T, size_t N, typename generator>
    inline void fill_array(T (&arr)[N], generator gen)
    {
        for (size_t i = 0; i < N; ++i)
            arr[i] = gen();
    }

    /*
        array<string, 10> v;

        utils::fill_array(v, []
        {
            return utils::generate_key(utils::char_type::mixed_chars);
        });

        for (auto& elem : v)
            cout << elem << endl;

        vector<string> v1(10);

        utils::fill_array(v1, []
        {
            return utils::generate_word(utils::char_type::mixed_chars, 10);
        });

        for (auto& elem : v1)
            cout << elem << endl;

        vector<char> v2(10);

        utils::fill_array(v2, []
        {
            return utils::get_random_character(utils::char_type::small_letter);
        });

        for (auto& elem : v2)
            cout << elem << endl;

        deque<char> v3(10);

        utils::fill_array(v3, []
        {
            return utils::get_random_character(utils::char_type::capital_letter);
        });
    
        for (auto& elem : v3)
            cout << elem << endl;

        int array[10];

        utils::fill_array(array, []
        {
            return utils::random_number();
        });

        for (auto& elem : array)
            cout << elem << endl;
    */

    // apply static polymorphism via templates and SFINAE to ensure vector<>, array<>, deque<>
    template<typename container,
         typename value_type = typename container::value_type,
         typename = std::enable_if_t<std::is_assignable<value_type&, value_type>::value>>
    inline void shuffle_array(container& arr)
    {
        const size_t arr_size = arr.size();
        for (size_t i = 0; i < arr_size; i++)
            std::swap(arr[random_number(0, arr_size - 1)], arr[random_number(0, arr_size - 1)]);
    }

    // template specialization for C-style arrays
    template<typename T, size_t N>
    inline void shuffle_array(T (&arr)[N])
    {
        for (size_t i = 0; i < N; ++i)
            std::swap(arr[random_number(0, N - 1)], arr[random_number(0, N - 1)]);
    }

    // no budget for cyber security? use Caesar Cipher, better than nothing ¯\_(ツ)_/¯
    inline std::string encrypt_text(std::string text, short encryption_key)
    {
        constexpr std::string_view CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        constexpr int RANGE = 62;

        for (size_t i = 0; i < text.size(); i++)
        {
            size_t pos = CHARS.find(text[i]);
            if (pos == std::string_view::npos)
                continue; // leave non-alphanumeric characters unchanged

            int shifted = (static_cast<int>(pos) + encryption_key) % RANGE;

            if (shifted < 0)
                shifted += RANGE;

            text[i] = CHARS[shifted];
        }

        return text;
    }

    inline std::string decrypt_text(std::string text, short encryption_key)
    {
        constexpr std::string_view CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        constexpr int RANGE = 62;

        for (size_t i = 0; i < text.size(); i++)
        {
            size_t pos = CHARS.find(text[i]);
            if (pos == std::string_view::npos)
                continue;

            int shifted = (static_cast<int>(pos) - encryption_key) % RANGE;

            if (shifted < 0)
                shifted += RANGE;

            text[i] = CHARS[shifted];
        }

        return text;
    }

    // Generic template for integral types only
    template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
    bool is_number_between(T num, T from, T to)
    {
        return num >= from && num <= to;
    }

    // overload for floating-points
    template<typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    bool is_number_between(T num, T from, T to)
    {
        const T eps = std::numeric_limits<T>::epsilon();
        return (num + eps >= from) && (num - eps <= to);
    }

    bool is_date_between(Date date, Date from, Date to)
    {
        if(from < to)
            std::swap(from, to);

        return date >= from && date <= to;
    }











} // namespace utils
