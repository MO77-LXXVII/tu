// string_utils.hpp

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>

namespace stutl
{
    /**
     * @brief String manipulation utilities with both static and instance methods.
     * 
     * Design philosophy:
     * - Static methods for standalone operations (pure functions)
     * - Instance methods modify the internal string and return *this (fluent interface)
     * - Supports method chaining: str.trim().to_upper().reverse_words()
     */
    class String
    {
        public:
            // -----------------
            // Constructors
            // -----------------

            String() = default;

            String(std::string raw_str) 
                : _raw_str(std::move(raw_str)) {}

            String(std::string_view raw_str)
                : String(std::string(raw_str)) {}

            String(const char* raw_str)
                : String(std::string(raw_str)) {}

            // -----------------
            // Accessors (in case you wish to break encapsulation for some reason ¯\_(ツ)_/¯)
            // -----------------

            // ! read-only view
            [[nodiscard]] const std::string& raw_str() const noexcept 
            { 
                return _raw_str;
            }

            // ! read-write view
            [[nodiscard]] std::string& raw_str() noexcept 
            { 
                return _raw_str;
            }

            // ! structured mutation
            String& set_raw_str(std::string raw_str)
            {
                _raw_str = std::move(raw_str);
                return *this;
            }

            // -----------------
            // Length Operations
            // -----------------

            [[nodiscard]] static size_t length(std::string_view str) noexcept
            {
                return str.length();
            }

            [[nodiscard]] size_t length() const noexcept
            {
                return _raw_str.length();
            }

            [[nodiscard]] bool is_empty() const noexcept
            {
                return _raw_str.empty();
            }

            // -----------------
            // Word-level Operations
            // -----------------
            
            [[nodiscard]] static int count_words(std::string_view str)
            {
                if (str.empty())
                    return 0;

                int count = 0;
                bool in_word = false;

                for(const char ch : str)
                {
                    if(std::isspace(static_cast<unsigned char>(ch)))
                        in_word = false;

                    else if(!in_word)
                    {
                        in_word = true;
                        ++count;
                    }
                }
                return count;
            }

            [[nodiscard]] size_t count_words() const
            {
                return count_words(_raw_str);
            }

            // -----------------
            // Case Transformations (Static)
            // -----------------

            [[nodiscard]] static std::string to_upper(std::string str)
            {
                std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
                                                                    {
                                                                        return std::toupper(c);
                                                                    });
                return str;
            }

            [[nodiscard]] static std::string to_lower(std::string str)
            {
                std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c)
                                                                    {
                                                                        return std::tolower(c);
                                                                    });
                return str;
            }

            [[nodiscard]] static std::string capitalize_first_letter(std::string str)
            {
                if (!str.empty())
                    str[0] = std::toupper(static_cast<unsigned char>(str[0]));

                return str;
            }

            [[nodiscard]] static std::string capitalize_words(std::string str)
            {
                bool capitalize_next = true;

                for (char& ch : str)
                {
                    if(std::isspace(static_cast<unsigned char>(ch)))
                        capitalize_next = true;

                    else if(capitalize_next)
                    {
                        ch = std::toupper(static_cast<unsigned char>(ch));
                        capitalize_next = false;
                    }
                }

                return str;
            }

            [[nodiscard]] static std::string invert_case(std::string str)
            {
                std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) 
                                                                    {
                                                                        return std::isupper(c) ? std::tolower(c) : std::toupper(c);
                                                                    });
                return str;
            }

            // -----------------
            // Case Transformations (Instance - Fluent Interface)
            // -----------------
            
            String& to_upper()
            {
                _raw_str = to_upper(_raw_str);
                return *this;
            }

            String& to_lower()
            {
                _raw_str = to_lower(_raw_str);
                return *this;
            }

            String& capitalize_first_letter()
            {
                _raw_str = capitalize_first_letter(_raw_str);
                return *this;
            }
            
            String& capitalize_words()
            {
                _raw_str = capitalize_words(_raw_str);
                return *this;
            }
            
            String& invert_case()
            {
                _raw_str = invert_case(_raw_str);
                return *this;
            }

            // -----------------
            // Character Counting
            // -----------------

            [[nodiscard]] static size_t count_uppercase(std::string_view str)
            {
                return std::count_if(str.begin(), str.end(), [](const unsigned char c)
                                                            {
                                                                return std::isupper(c);
                                                            });
            }

            [[nodiscard]] static size_t count_lowercase(std::string_view str)
            {
                return std::count_if(str.begin(), str.end(), [](const unsigned char c)
                                                            {
                                                                return std::islower(c);
                                                            });
            }

            [[nodiscard]] static size_t count_char(std::string_view str, const char target, bool case_sensitive = true)
            {
                if (case_sensitive)
                    return std::count(str.begin(), str.end(), target);

                char target_lower = std::tolower(static_cast<unsigned char>(target));

                return std::count_if(str.begin(), str.end(), [target_lower](const unsigned char c) 
                                                            {
                                                                return std::tolower(c) == target_lower;
                                                            });
            }
            
            [[nodiscard]] static bool is_vowel(char ch)
            {
                ch = std::tolower(static_cast<unsigned char>(ch));
                return ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u';
            }
            
            [[nodiscard]] static size_t count_vowels(std::string_view str)
            {
                return std::count_if(str.begin(), str.end(), [](char c)
                                                            {
                                                                return is_vowel(c);
                                                            });
            }

            // -----------------
            // Character Counting (Instance)
            // -----------------

            [[nodiscard]] size_t count_uppercase() const
            {
                return count_uppercase(_raw_str);
            }

            [[nodiscard]] size_t count_lowercase() const
            {
                return count_lowercase(_raw_str);
            }

            [[nodiscard]] size_t count_char(char target, bool case_sensitive = true) const
            {
                return count_char(_raw_str, target, case_sensitive);
            }

            [[nodiscard]] size_t count_vowels() const
            {
                return count_vowels(_raw_str);
            }

            // -----------------
            // String Splitting & Joining
            // -----------------

            [[nodiscard]] static std::vector<std::string> split(std::string_view input, std::string_view delimiter)
            {
                std::vector<std::string> tokens;

                if(input.empty() || delimiter.empty())
                    return tokens;

                size_t start = 0;
                size_t end = input.find(delimiter);

                while(end != std::string::npos)
                {
                    // Skip empty tokens
                    if(end > start)
                        tokens.emplace_back(input.substr(start, end - start));

                    start = end + delimiter.length();
                    end = input.find(delimiter, start);
                }

                // Add remaining part
                if(start < input.length())
                    tokens.emplace_back(input.substr(start));

                return tokens;
            }

            [[nodiscard]] std::vector<std::string> split(std::string_view delimiter) const
            {
                return split(_raw_str, delimiter);
            }
            
            [[nodiscard]] static std::string join(const std::vector<std::string>& strings, std::string_view delimiter)
            {
                if(strings.empty())
                    return "";

                std::string result = strings[0];

                for(size_t i = 1; i < strings.size(); ++i)
                {
                    result += delimiter;
                    result += strings[i];
                }

                return result;
            }

            // -----------------
            // Trimming Operations
            // -----------------

            [[nodiscard]] static std::string trim_left(std::string str)
            {
                auto it = std::find_if(str.begin(), str.end(), [](unsigned char c)
                                                                {
                                                                    return !std::isspace(c);
                                                                });

                str.erase(str.begin(), it);
                return str;
            }

            [[nodiscard]] static std::string trim_right(std::string str)
            {
                auto it = std::find_if(str.rbegin(), str.rend(), [](unsigned char c)
                                                                {
                                                                    return !std::isspace(c);
                                                                });

                str.erase(it.base(), str.end());
                return str;
            }

            [[nodiscard]] static std::string trim(std::string str)
            {
                return trim_left(trim_right(std::move(str)));
            }

            String& trim_left()
            {
                _raw_str = trim_left(_raw_str);
                return *this;
            }

            String& trim_right()
            {
                _raw_str = trim_right(_raw_str);
                return *this;
            }

            String& trim()
            {
                _raw_str = trim(_raw_str);
                return *this;
            }

            // -----------------
            // String Manipulation
            // -----------------

            [[nodiscard]] static std::string reverse_words(std::string_view str)
            {
                auto words = split(str, " ");
                std::reverse(words.begin(), words.end());
                return join(words, " ");
            }

            String& reverse_words()
            {
                _raw_str = reverse_words(_raw_str);
                return *this;
            }

            [[nodiscard]] static std::string replace_word(std::string str, std::string_view target, 
                                                        std::string_view replacement, bool case_sensitive = true)
            {
                auto words = split(str, " ");

                for(auto& word : words)
                {
                    bool match = case_sensitive ? (word == target) : (to_lower(word) == to_lower(std::string(target)));
                    
                    if (match)
                        word = replacement;
                }

                return join(words, " ");
            }

            String& replace_word(std::string_view target, std::string_view replacement, bool case_sensitive = true)
            {
                _raw_str = replace_word(_raw_str, target, replacement, case_sensitive);
                return *this;
            }

            [[nodiscard]] static std::string remove_punctuation(std::string str)
            {
                str.erase(remove_if(str.begin(), str.end(), [](unsigned char c)
                                                            {
                                                                return std::ispunct(c);
                                                            }),
                                                            str.end());

                return str;
            }

            String& remove_punctuation()
            {
                _raw_str = remove_punctuation(_raw_str);
                return *this;
            }

            // -----------------
            // Builder Methods (wrappers for std::string methods)
            // -----------------

            String& append(std::string_view str)
            {
                _raw_str.append(str);
                return *this;
            }

            String& prepend(std::string_view str)
            {
                _raw_str.insert(0, str);
                return *this;
            }

            String& clear() noexcept
            {
                _raw_str.clear();
                return *this;
            }

            // -----------------
            // Comparison & Search
            // -----------------

            [[nodiscard]] bool equals(std::string_view other, bool case_sensitive = true) const
            {
                return (case_sensitive)? _raw_str == other :  to_lower(_raw_str) == to_lower(std::string(other));
            }

            [[nodiscard]] bool contains(std::string_view substring, bool case_sensitive = true) const noexcept
            {
                return (case_sensitive)? _raw_str.find(substring) != _raw_str.npos
                : to_lower(_raw_str).find(to_lower(std::string(substring))) != _raw_str.npos;
            }

            [[nodiscard]] bool starts_with(std::string_view prefix) const noexcept
            {
                if (prefix.length() > _raw_str.length())
                    return false;
                
                return _raw_str.compare(0, prefix.length(), prefix) == 0;
            }

            [[nodiscard]] bool ends_with(std::string_view suffix) const noexcept
            {
                if (suffix.length() > _raw_str.length())
                    return false;
                
                return _raw_str.compare(_raw_str.length() - suffix.length(), suffix.length(), suffix) == 0;
            }

            // -----------------
            // Conversion Operators
            // -----------------

            // Implicit conversion to std::string_view (safe, cheap)
            [[nodiscard]] operator std::string_view() const noexcept
            {
                return _raw_str;
            }

            // Explicit conversion to std::string
            [[nodiscard]] explicit operator std::string() const
            {
                return _raw_str;
            }

            // -----------------
            // Stream Output
            // -----------------

            friend std::ostream& operator<<(std::ostream& os, const String& str)
            {
                return os << str._raw_str;
            }

        private:
            std::string _raw_str;
    };
} // namespace stutl
