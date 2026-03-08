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


/**
 * Forward declaration of `ColouredText` to avoid including `ansi_colours.hpp` here
 * as `ansi_colours.hpp` already includes `output.hpp`, so including it here would
 * create a circular dependency
 */
namespace terminal_utils
{
    class ColouredText;
}


namespace terminal_utils::output
{
    /**
     * @brief RAII wrapper that saves and restores sticky stream formatting state.
     *
     * saves `flags()` and `fill()` on construction and restores them on destruction,
     * guaranteeing that helpers don't leak formatting into surrounding output.
     *
     * @note does NOT save `width()` because `std::setw()` is not sticky
     *       it resets automatically after each `operator<<`
     */
    class FormatGuard
    {
        /**
         * @brief constructs a `FormatGuard` and saves the current formatting state of `os`
         * @param os the stream to guard (default: `std::cout`)
         */
        public:
            explicit FormatGuard(std::ostream& os = std::cout)
                : stream_(os), 
                old_flags_(os.flags()),      // left/right/hex/scientific etc. (sticky)
                old_fill_(os.fill())         // fill character (sticky)
            {}

            /** @brief restores the saved formatting state to the guarded stream */
            ~FormatGuard()
            {
                stream_.flags(old_flags_);
                stream_.fill(old_fill_);
            }

            FormatGuard(const FormatGuard&) = delete;
            FormatGuard& operator=(const FormatGuard&) = delete;

        private:
            std::ostream& stream_;         ///< reference to the guarded stream
            std::ios::fmtflags old_flags_; ///< saved format flags
            char old_fill_;                ///< saved fill character
    };


    /** @brief text alignment options for use with `print_aligned()` and `Table::print()` */
    enum class Alignment { Left, Right, Center };


    /**
     * @brief format descriptor that bundles a value with its alignment parameters.
     *
     * delays actual printing until `operator<<` is called on the struct.
     *
     * @tparam T the type of the value to align
     *
     * @note `width` is a minimum, not a maximum: caller must ensure text fits or it will overflow.
     *       `const T&` avoids copying large objects and works with temporaries passed through `print_aligned()`
     */
    template<typename T>
    struct Aligned
    {
        // storing const T& avoids copying large objects and works with "temporaries" passed through `print_aligned()`
        const T& value;  ///< reference to the value to print
        int width;       ///< *minimum* field width
        Alignment align; ///< text alignment within the field
        char fill;       ///< fill character for padding
    };


    // Factory function deduces T automatically (template argument deduction)
    /**
     * @brief factory function that constructs an `Aligned<T>` with deduced type `T`
     *
     * @tparam T        the type of the value to align (deduced automatically)
     * 
     * @param value     the value to print
     * @param width     minimum field width
     * @param align     text alignment (default: `Alignment::Left`)
     * @param fill      fill character for padding (default: `' '`)
     * 
     * @return          an `Aligned<T>` descriptor ready for `operator<<`
     */
    template<typename T>
    Aligned<T> print_aligned(const T& value, int width, Alignment align = Alignment::Left, char fill = ' ')
    {
        return Aligned<T>{value, width, align, fill};
    }


    /**
     * @brief stream insertion operator for `Aligned<T>`
     *
     * applies alignment padding around the value and writes it to `os`
     * uses `FormatGuard` to ensure stream formatting state is not leaked
     *
     * @tparam T    the type of the wrapped value
     * @param os    the output stream to write to
     * @param a     the `Aligned<T>` descriptor containing the value and formatting parameters
     * @return      reference to `os` for chaining
     *
     * @note for `ColouredText`, use the explicit specialization which accounts
     *       for ANSI escape codes being excluded from padding calculations
     */
    template<typename T>
    std::ostream& operator<<(std::ostream& os, const Aligned<T>& a)
    {
        FormatGuard guard(os);

        if (a.align == Alignment::Center)
        {
            std::ostringstream oss;
            oss << a.value;

            const std::string str = oss.str();

            int padding = ((a.width -  static_cast<int>(str.length())) / 2);

            // int left_pad = padding / 2;
            int left_pad = std::max(0, padding);

            // int right_pad = padding - left_pad;
            int right_pad = std::max(0, a.width - left_pad - static_cast<int>(str.length()));

            os << std::string(left_pad, a.fill) << str 
               << std::string(right_pad, a.fill);
        }

        else
        {
            os << std::setfill(a.fill) << std::setw(a.width);
            os << (a.align == Alignment::Left ? std::left : std::right) << a.value;
        }

        return os;
    }


    /**
     * @brief specialization of `operator<<` for `Aligned<ColouredText>`
     *
     * the generic version streams the value into a temporary `std::ostringstream`
     * to measure its length for padding — this loses ANSI escape codes, printing plain text only
     * this specialization accesses `_text` directly for width calculation,
     * then outputs the full `ColouredText` object with its ANSI codes intact
     *
     * definition lives in `ansi_colours.hpp` where `ColouredText` is fully defined
     */
    template<>
    std::ostream& operator<<(std::ostream& os, const Aligned<terminal_utils::ColouredText>& a);


    class Table
    {
        public:
            /** @brief tag type used to select the owning overload of `add_row()` */
            struct own_t {};

            /** @brief tag instance: pass as first argument to take ownership of cell strings */
            static constexpr own_t own{};

            /**
             * @brief adds a row of cells as views. caller must ensure strings outlive the `Table`
             * @param cells the cell values for this row
             * @return reference to this `Table` for chaining
             */
            Table& add_row(std::initializer_list<std::string_view> cells)
            {
                _rows.emplace_back(cells);
                return *this;
            }

            /**
             * @brief adds a row of cells, copying strings into internal storage
             * safe to use with temporaries or strings with uncertain lifetime
             * @param cells the cell values for this row
             * @return reference to this `Table` for chaining
             */
            Table& add_row(own_t, std::initializer_list<std::string_view> cells)
            {
                std::vector<std::string_view> row;
                row.reserve(cells.size());

                for (auto& cell : cells)
                {
                    _owned_storage.emplace_back(cell);
                    row.emplace_back(_owned_storage.back());
                }

                _rows.emplace_back(std::move(row));
                return *this;
            }

            /** @brief alias for `add_row(Table::own, cells)`: kept for backwards compatibility */
            Table& add_row_owned(std::initializer_list<std::string_view> cells)
            {
                add_row(own, cells);
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

                        constexpr int border_padding = 2; // account for '|' we manually print on each side

                        std::string_view cell = (i < row.size()) ? row[i] : null_placeholder;

                        std::cout << "|" << print_aligned(cell, col_widths[i] + (extra_padding - border_padding), alignment, ' ');
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
