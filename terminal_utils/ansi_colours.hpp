// ansi_colours.hpp

#pragma once

#include <string_view>
#include <type_traits>
#include <ostream>
#include <array>

#include "../platform/platform.hpp"
#include "config.hpp"
#include "output.hpp"

namespace terminal_utils
{
    enum class Colour
    {
        None,
        Reset,
        ResetColour,
        Black, Red, Green, Yellow, Blue, Magenta, Cyan, White
    };
    
    // bitmask enum
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

    using style_ut = std::underlying_type_t<Style>;

    // Allow bitwise operations on Style
    inline constexpr Style operator|(Style a, Style b)
    {
        return static_cast<Style>(static_cast<style_ut>(a) | static_cast<style_ut>(b));
    }

    inline constexpr Style& operator|=(Style& a, Style b)
    {
        a = a | b;
        return a;
    }

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

    // small value object that bundles text with the formatting metadata (colour + style).
    // stores the text internally & keeps it for later rendering
    // does not depend on caller "lifetime"
    class ColouredText
    {
        public:
            ColouredText(std::string text, Colour colour = Colour::None, Style style = Style::None)
                : _text(std::move(text)), _colour(colour), _style(style)
                {}

            ColouredText() = default;

            friend std::ostream& operator<<(std::ostream& os, const ColouredText& ct);

            template<typename T>
            friend std::ostream& output::operator<<(std::ostream&, const output::Aligned<T>&);
        
            // accumulates multiple styles via bitwise OR
            inline constexpr void add_style(Style s)
            {
                _style |= s;
            }

            // sets a single colour
            inline constexpr void set_colour(Colour c)
            {
                _colour = c;
            }

        private:
            std::string _text;
            Colour _colour;
            Style _style;

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

            static void apply_styles(std::ostream& os, Style styles)
            {
                using st = Style; // Type alias
                constexpr std::array<Style, 8> all_styles_array{st::Dim, st::Bold, st::Italic, st::Underline,
                                                                st::Blink, st::Reverse, st::Hidden, st::Strikethrough};

                for(const Style s : all_styles_array)
                    if(has_style(styles, s))
                        os << get_style_code(s);
            }
    };

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
        // template specialization for ColouredText
        template<>
        inline std::ostream& operator<<(std::ostream& os, const Aligned<ColouredText>& a)
        {
            FormatGuard guard(os);

            // Direct access to _text since we're a friend
            const std::string& text = a.value._text;

            if (a.align == Alignment::Center)
            {
                int padding = (a.width - 2 - static_cast<int>(text.length())) / 2;
                int left_pad = std::max(0, padding);
                int right_pad = std::max(0, a.width - 2 - left_pad - static_cast<int>(text.length()));

                os << std::string(left_pad, a.fill) 
                << a.value  // Outputs text stored in ColouredText with ANSI codes
                << std::string(right_pad, a.fill);
            }

            else
            {
                int padding = a.width - static_cast<int>(text.length());

                if (a.align == Alignment::Left)
                    os << a.value << std::string(std::max(0, padding), a.fill);

                else // Right
                    os << std::string(std::max(0, padding), a.fill) << a.value;
            }

            return os;
        }
    }
} // namespace terminal_utils
