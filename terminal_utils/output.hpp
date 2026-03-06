// output.hpp

#pragma once

#include <vector>
#include <deque>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>

#include "config.hpp"

// Forward declaration instead of including ansi_colours.hpp
// to break circular dependency 
namespace terminal_utils
{
    class ColouredText;
}

namespace terminal_utils::output
{
    // RAII wrapper for sticky stream formatting state
    // Does NOT save width() because std::setw() is not sticky (resets after each <<)
    // this guarantees that the helpers don’t leak formatting into surrounding output
    class FormatGuard
    {
        public:
            explicit FormatGuard(std::ostream& os = std::cout)
                : stream_(os), 
                old_flags_(os.flags()),      // left/right/hex/scientific etc. (sticky)
                old_fill_(os.fill())         // fill character (sticky)
            {}

            ~FormatGuard()
            {
                stream_.flags(old_flags_);
                stream_.fill(old_fill_);
            }

            FormatGuard(const FormatGuard&) = delete;
            FormatGuard& operator=(const FormatGuard&) = delete;

        private:
            std::ostream& stream_;
            std::ios::fmtflags old_flags_;
            char old_fill_;
    };

    // Alignment enum
    enum class Alignment { Left, Right, Center };

    // format descriptor struct to hold formatting parameters
    // delay actual printing until `operator<<`
    template<typename T>
    struct Aligned
    {
        // storing const T& avoids copying large objects and works with "temporaries" passed through `print_aligned()`
        const T& value;
        int width;
        Alignment align;
        char fill;
    };

    // Factory function deduces T automatically (template argument deduction)
    template<typename T>
    Aligned<T> print_aligned(const T& value, int width, Alignment align = Alignment::Left, char fill = ' ')
    {
        return Aligned<T>{value, width, align, fill};
    }

    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Aligned<T>& a)
    {
        FormatGuard guard(os);

        if (a.align == Alignment::Center)
        {
            std::ostringstream oss;
            oss << a.value;
            const std::string str = oss.str();
            // int padding = std::max(0, a.width - static_cast<int>(str.length()));
            int padding = (((a.width - 2) -  static_cast<int>(str.length())) / 2);

            // int left_pad = padding / 2;
            int left_pad = std::max(0, padding);

            // int right_pad = padding - left_pad;
            int right_pad = std::max(0, a.width - 2 - left_pad - static_cast<int>(str.length()));

            os << std::string(left_pad, a.fill) << str 
               << std::string(right_pad, a.fill);
        }

        // // Title centered
        // int padding = (_width - 2 - static_cast<int>(_title.length())) / 2;
        // int left_pad = std::max(0, padding);
        // int right_pad = std::max(0, _width - 2 - left_pad - static_cast<int>(_title.length()));

        // std::cout << "|" << std::string(left_pad, ' ') 
        //         << bold(white(_title)) 
        //         << std::string(right_pad, ' ') << "|\n";

        else
        {
            os << std::setfill(a.fill) << std::setw(a.width);
            os << (a.align == Alignment::Left ? std::left : std::right) << a.value;
        }

        return os;
    }

    // Forward declare the specialization (definition will be in ansi_colours.hpp)
    template<>
    std::ostream& operator<<(std::ostream& os, const Aligned<terminal_utils::ColouredText>& a);

    // add ostream later to support file I/O
    class Table
    {
        public:
            // initializer_list allows: table.add_row({"id", "name", "score"});
            Table& add_row(std::initializer_list<std::string_view> cells)
            {
                _rows.emplace_back(cells);
                return *this; // allows chaining
            }

            Table& add_row_owned(std::initializer_list<std::string_view> cells)
            {
                std::vector<std::string_view> row;
                row.reserve(cells.size());

                for (auto& cell : cells)
                {
                    _owned_storage.emplace_back(cell); // copy into owned storage
                    row.emplace_back(_owned_storage.back());
                }

                _rows.emplace_back(std::move(row));
                return *this;
            }

            void print(Alignment alignment, int extra_padding = config::DEFAULT_PADDING) const
            {
                if (_rows.empty())
                    return;

                // stores each column width
                std::vector<size_t> col_widths;

                // total amount of columns
                size_t num_cols = 0;

                // Find total amountof columns
                for (const auto& row : _rows)
                    num_cols = std::max(num_cols, row.size());

                // Ensure `col_widths` has one entry per column (we got from `num_cols`)
                col_widths.resize(num_cols, 0);

                size_t total_words_count = 0;

                // Find maximum width for each column
                for(const auto& row : _rows)
                {
                    for(size_t i = 0; i < row.size(); ++i)
                        col_widths[i] = std::max(col_widths[i], row[i].length());
                }

                for (size_t i = 0; i < num_cols; ++i)
                    total_words_count += col_widths[i];

                auto print_border = [total_words_count, num_cols, extra_padding]()
                {
                    size_t border_width =
                        total_words_count +         // actual text widths
                        (2 * num_cols) +            // left + right padding per column (left/right padding per column)
                        (num_cols - 1);             // inner separators between columns (all '|' characters)

                    std::cout << "+"
                        << std::string(border_width, '-')
                        << "+\n";
                };

                // Top border
                print_border();

                // Print rows
                for (const auto& row : _rows)
                {
                    for (size_t i = 0; i < num_cols; ++i)  // Changed: iterate through num_cols instead of row.size()
                    {
                        // missing cells currently render as empty
                        // - could be "null"/"NULL"
                        constexpr std::string_view null_placeholder = "";
                        std::string_view cell = (i < row.size()) ? row[i] : null_placeholder;

                        std::cout << "|" << print_aligned(cell, col_widths[i] + extra_padding, alignment, ' ');
                    }

                    std::cout << "|" << std::endl;

                    // rows seperator + bottom border
                    print_border();
                }
            }

        private:
            std::vector<std::vector<std::string_view>> _rows;

            // takes ownership of std::string in case user cannot guarantee the view lifetime
            // uses std::deque as it doesn't invalidate references when growing up
            std::deque<std::string> _owned_storage;
    };
} // namespace terminal_utils::output



/*
    ! Use case:

    Table table;
    table.add_row({"id", "name", "score"});
    table.add_row({"id"});
    table.add_row({"id", "name", "score", "health", "age", "5555555"});
    table.add_row({"id", "name", "score", "health"});

    table.print(tu::output::Alignment::Center);

    +---------------------------------------------------------+
    |  id  |  name  |  score  |           |       |           |
    +---------------------------------------------------------+
    |  id  |        |         |           |       |           |
    +---------------------------------------------------------+
    |  id  |  name  |  score  |  health   |  age  |  5555555  |
    +---------------------------------------------------------+
    |  id  |  name  |  score  |  health   |       |           |
    +---------------------------------------------------------+
*/
