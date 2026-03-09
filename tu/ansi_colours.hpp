// ansi_colours.hpp

#pragma once

#include <string_view>
#include <type_traits>
#include <ostream>
#include <array>

#include "../platform/platform.hpp"
#include "config.hpp"
#include "output.hpp"

namespace tu
{
    /**
     * @brief ANSI terminal colour codes for foreground text
     * 
     * These correspond to standard ANSI escape sequences for
     * setting text colour in compatible terminals.
     *
     * When `ENABLE_COLOURS` is true and terminal supports ANSI, these
     * will be rendered as coloured text.
     */
    enum class Colour
    {
        None,         ///< No colour change (use default)
        Reset,        ///< Reset all attributes (colour, style, etc.)
        ResetColour,  ///< Reset only colour, keep other styles
        Black, Red, Green, Yellow, Blue, Magenta, Cyan, White
    };


    /**
     * @brief Text style attributes as a bitmask enum
     * 
     * This enum uses power-of-two values so multiple styles can be combined
     * using bitwise OR (|). For example: `Style::Bold | Style::Underline`
     * 
     * Maps to standard ANSI style codes (1-9) for terminal text formatting.
     * 
     * @see `has_style()` to check if a particular style is set
     * @see `operator|()` for combining styles
     */
    enum class Style
    {
        None = 0,
        Bold = 1 << 0,
        Dim = 1 << 1,
        Italic = 1 << 2,
        Underline = 1 << 3,
        Blink = 1 << 4,
        Reverse = 1 << 5,
        Hidden = 1 << 6,
        Strikethrough = 1 << 7
    };


    // Underlying integer type for `Style enum` (used for bitwise operations)
    // `style_ut` is a shorthand for `style underlying type`
    using style_ut = std::underlying_type_t<Style>;


    /**
     * @brief Bitwise OR operator for combining `Style` flags
     * 
     * Allows multiple styles to be combined naturally:
     * @code
     * Style my_style = Style::Bold | Style::Underline;
     * @endcode
     * 
     * @param a First `Style` value
     * @param b Second `Style` value
     * @return Combined `Style` with bits from both operands
     * 
     * @note this operator allow `Style` to behave like a proper bitmask type,
     * enabling intuitive syntax while maintaining type safety through
     * explicit casts to/from the underlying type
     */
    inline constexpr Style operator|(Style a, Style b)
    {
        return static_cast<Style>(static_cast<style_ut>(a) | static_cast<style_ut>(b));
    }


    /**
     * @brief Compound assignment OR operator for Style flags
     * 
     * @param a `Style` to modify (left-hand side)
     * @param b `Style` to combine in (right-hand side)
     * @return Reference to the modified `Style`
     */
    inline constexpr Style& operator|=(Style& a, Style b)
    {
        a = a | b;
        return a;
    }


    /**
     * @brief check if specific style flag is set in a `Style` bitmask
     * 
     * tests whether any bit in 'check' is present in 'styles'
     * useful for conditionally applying logic based on styles
     * 
     * @param styles the combined `Style` bitmask to check
     * @param check the specific `style` flag to look for
     * @return `true` **if any bit** in 'check' is set in 'styles'
     * 
     * @note should be called with a single style flag
     * 
     * @code
     * Style my_style = Style::Bold | Style::Underline;
     * if(has_style(my_style, Style::Bold))                 // true  (Bold is set)
     * if(has_style(my_style, Style::Italic))               // false (Italic not set)
     * if(has_style(my_style, Style::Bold | Style::Italic)) // true  (even though Italic is not set, Bold is)
     * @endcode
     */
    inline constexpr bool has_style(Style styles, Style check)
    {
        return (static_cast<style_ut>(styles) & static_cast<style_ut>(check)) != 0;
    }


    // Grant friendship to Aligned in the output namespace
    // to break circular dependency 
    namespace output
    {
        template<typename T> struct Aligned;
        template<typename T> std::ostream& operator<<(std::ostream&, const Aligned<T>&);
    }


    /**
     * @brief small value object that bundles text with formatting metadata (colour + style)
     * 
     * stores the text internally, so it does not depend on the caller's lifetime.
     * safe to store and render later without dangling reference concerns.
     */
    class ColouredText
    {
        public:
            /**
             * @brief constructs a `ColouredText` with the given text and optional formatting
             * 
             * @param text   the text to display: ownership is taken via `std::move`
             * @param colour foreground colour (default: `Colour::None`)
             * @param style  text style bitmask (default: `Style::None`)
             */
            ColouredText(std::string text, Colour colour = Colour::None, Style style = Style::None)
                : _text(std::move(text)), _colour(colour), _style(style)
                {}

            /** @brief default constructor: initializes `_colour` to `Colour::None` and `_style` to `Style::None` */
            ColouredText() = default;

            friend std::ostream& operator<<(std::ostream& os, const ColouredText& ct);

            template<typename T>
            friend std::ostream& output::operator<<(std::ostream&, const output::Aligned<T>&);


            /**
             * @brief accumulates a `Style` flag into the current `_style` bitmask via bitwise OR
             * 
             * @param s the `Style` flag to add
             */
            inline constexpr void add_style(Style s)
            {
                _style |= s;
            }


            /**
             * @brief set the text `colour`
             * 
             * @param `c` the `colour` to apply
             */
            inline constexpr void set_colour(Colour c)
            {
                _colour = c;
            }


        private:
            std::string _text;                ///< The actual text content to display
            Colour _colour   = Colour::None;  ///< Current text colour (default: no colour)
            Style _style     = Style::None;   ///< Current text style flags (default: no styles)

            /**
             * @brief returns the ANSI escape sequence string corresponding to a Colour enum
             * @note  returns `std::string_view` to string literals, which have static storage duration.
             *        this is safe because string literals live for the entire program lifetime.
             */
            [[nodiscard]] static constexpr std::string_view get_colour_code(Colour c)
            {
                switch(c)
                {
                    case Colour::None:
                        return "";

                    case Colour::Reset:
                        return "\033[0m"; // the nuclear option.

                    case Colour::ResetColour:
                        return "\033[39m"; // resets colour only

                    case Colour::Black:
                        return "\033[30m";

                    case Colour::Red:
                        return "\033[31m";

                    case Colour::Green:
                        return "\033[32m";

                    case Colour::Yellow:
                        return "\033[33m";

                    case Colour::Blue:
                        return "\033[34m";

                    case Colour::Magenta:
                        return "\033[35m";

                    case Colour::Cyan:
                        return "\033[36m";

                    case Colour::White:
                        return "\033[37m";

                    default: // unreachable as all enum values are handled above (suppressing compiler warnings)
                        return "";
                }
            }


            /**
             * @brief returns the ANSI escape sequence string corresponding to a Style enum
             * @note  returns `std::string_view` to string literals, which have static storage duration.
             *        this is safe because string literals live for the entire program lifetime.
             */
            [[nodiscard]] static constexpr std::string_view get_style_code(Style s)
            {
                switch(s)
                {
                    case Style::None:
                        return "";

                    case Style::Bold:
                        return "\033[1m";

                    case Style::Dim:
                        return "\033[2m";

                    case Style::Italic:
                        return "\033[3m";

                    case Style::Underline:
                        return "\033[4m";

                    case Style::Blink:
                        return "\033[5m";

                    case Style::Reverse:
                        return "\033[7m";

                    case Style::Hidden:
                        return "\033[8m";

                    case Style::Strikethrough:
                        return "\033[9m";    

                    default:  // unreachable as all enum values are handled above (suppressing compiler warnings)
                        return "";
                }
            }


            /**
             * @brief applies all active style flags in `styles` to the output stream
             * 
             * iterates over all known `Style` flags and writes the corresponding
             * ANSI escape sequence for each one that is set in `styles`
             * 
             * @param os     the output stream to write to
             * @param styles the combined `Style` bitmask to apply
             */
            static void apply_styles(std::ostream& os, Style styles)
            {
                constexpr std::array<Style, 8> all_styles_array{Style::Dim, Style::Bold, Style::Italic, Style::Underline,
                                                                Style::Blink, Style::Reverse, Style::Hidden, Style::Strikethrough};

                for(const Style s : all_styles_array)
                    if(has_style(styles, s))
                        os << get_style_code(s);
            }
    };

    /**
     * @brief stream insertion operator for `ColouredText`
     * 
     * writes the text to `os` with ANSI colour and style codes applied,
     * but only if `os` is `std::cout` or `std::cerr` and **ANSI is enabled**
     * 
     * @note always resets formatting after writing to avoid bleeding into subsequent output
     * 
     * @param os the output stream to write to
     * @param ct the `ColouredText` object to render
     * 
     * @return reference to `os` for chaining
     */
    inline std::ostream& operator<<(std::ostream& os, const ColouredText& ct)
    {
        // Only colorize cout and cerr
        const bool should_colourize = (&os == &std::cout || &os == &std::cerr) && config::ENABLE_COLOURS && platform::is_ansi_enabled();

        if(should_colourize)
        {
            ColouredText::apply_styles(os, ct._style);
            os << ColouredText::get_colour_code(ct._colour);
        }

        os << ct._text;

        if(should_colourize)
            os << ColouredText::get_colour_code(Colour::Reset);

        return os;
    }


    // Specialization must be in output namespace 
    namespace output    
    {
        /**
         * @brief template specialization of `operator<<` for `Aligned<ColouredText>`
         * 
         * handles alignment padding correctly for `ColouredText` by using the raw
         * text length (excluding ANSI escape codes) for padding calculations,
         * then outputs the `ColouredText` with its ANSI codes intact.
         * 
         * @note without this specialization, padding would be calculated correctly,
         * but if the generic version of `Aligned` tried to measure length through `operator<<`
         * it would include ANSI codes in the length calculation, making the padding wrong
         *
         * @param os the output stream to write to
         * @param a  the `Aligned<ColouredText>` object containing alignment settings
         * @return reference to `os` for chaining
         */
        template<>
        inline std::ostream& operator<<(std::ostream& os, const Aligned<ColouredText>& a)
        {
            // FormatGuard restores stream flags and fill character on destruction (RAII)
            FormatGuard guard(os);

            // Direct access to _text since we're a friend
            const std::string& text = a.value._text;
            if(a.align == Alignment::Center)
            {
                /*
                    `/ 2`: splits padding equally left and right
                    in case of odd padding integer division truncates
                    so the remainder goes to `right_pad`
                */
                const int padding = (a.width - static_cast<int>(text.length())) / 2;

                int left_pad = std::max(0, padding);
                int right_pad = std::max(0, a.width - left_pad - static_cast<int>(text.length()));

                os << std::string(left_pad, a.fill) 
                << a.value  // Outputs text stored in ColouredText with ANSI codes
                << std::string(right_pad, a.fill);
            }

            else
            {
                const int padding = a.width - static_cast<int>(text.length());

                if (a.align == Alignment::Left)
                    os << a.value << std::string(std::max(0, padding), a.fill);

                else // Alignment::Right
                    os << std::string(std::max(0, padding), a.fill) << a.value;
            }

            return os;
        }
    }
} // namespace terminal_utils
