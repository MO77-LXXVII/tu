// terminal_utils/style_wrappers.hpp

#pragma once

#include "ansi_colours.hpp"

namespace terminal_utils
{
    // ========================================================================
    // Colour factory functions
    // ========================================================================

    // Initial creation from string (owns string)
    [[nodiscard]] inline ColouredText red(std::string text)
    {
        return ColouredText{std::move(text), Colour::Red};
    }

    [[nodiscard]] inline ColouredText green(std::string text)
    {
        return ColouredText{std::move(text), Colour::Green};
    }

    [[nodiscard]] inline ColouredText blue(std::string text)
    {
        return ColouredText{std::move(text), Colour::Blue};
    }

    [[nodiscard]] inline ColouredText cyan(std::string text)
    {
        return ColouredText{std::move(text), Colour::Cyan};
    }

    [[nodiscard]] inline ColouredText magenta(std::string text)
    {
        return ColouredText{std::move(text), Colour::Magenta};
    }

    [[nodiscard]] inline ColouredText yellow(std::string text)
    {
        return ColouredText{std::move(text), Colour::Yellow};
    }

    [[nodiscard]] inline ColouredText white(std::string text)
    {
        return ColouredText{std::move(text), Colour::White};
    }

    [[nodiscard]] inline ColouredText black(std::string text)
    {
        return ColouredText{std::move(text), Colour::Black};
    }

    /*
        Chaining on existing object:
        note: `base` is a function parameter, NRVO is excluded (C++ standard, [class.copy.elision])
            all functions in this section use implicit move on return
    */
    [[nodiscard]] inline ColouredText red(ColouredText base)
    {
        base.set_colour(Colour::Red);
        return base;
    }

    [[nodiscard]] inline ColouredText green(ColouredText base)
    {
        base.set_colour(Colour::Green);
        return base;
    }

    [[nodiscard]] inline ColouredText blue(ColouredText base)
    {
        base.set_colour(Colour::Blue);
        return base;
    }

    [[nodiscard]] inline ColouredText cyan(ColouredText base)
    {
        base.set_colour(Colour::Cyan);
        return base;
    }

    [[nodiscard]] inline ColouredText magenta(ColouredText base)
    {
        base.set_colour(Colour::Magenta);
        return base;
    }

    [[nodiscard]] inline ColouredText yellow(ColouredText base)
    {
        base.set_colour(Colour::Yellow);
        return base;
    }

    [[nodiscard]] inline ColouredText white(ColouredText base)
    {
        base.set_colour(Colour::White);
        return base;
    }

    [[nodiscard]] inline ColouredText black(ColouredText base)
    {
        base.set_colour(Colour::Black);
        return base;
    }

    // ========================================================================
    // Style factory functions
    // ========================================================================

    // Initial creation (owns string)
    [[nodiscard]] inline ColouredText bold(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Bold};
    }

    [[nodiscard]] inline ColouredText dim(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Dim};
    }

    [[nodiscard]] inline ColouredText underline(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Underline};
    }

    [[nodiscard]] inline ColouredText italic(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Italic};
    }

    [[nodiscard]] inline ColouredText blink(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Blink};
    }

    [[nodiscard]] inline ColouredText reverse(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Reverse};
    }

    [[nodiscard]] inline ColouredText hidden(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Hidden};
    }

    [[nodiscard]] inline ColouredText strikethrough(std::string text)
    {
        return ColouredText{std::move(text), Colour::None, Style::Strikethrough};
    }

    /*
        Chaining (stack styles)
        note: `base` is a function parameter, NRVO is excluded (C++ standard, [class.copy.elision])
              all functions in this section use implicit move on return
    */
    [[nodiscard]] inline ColouredText bold(ColouredText base)
    {
        base.add_style(Style::Bold);
        return base;
    }

    [[nodiscard]] inline ColouredText dim(ColouredText base)
    {
        base.add_style(Style::Dim);
        return base;
    }

    [[nodiscard]] inline ColouredText underline(ColouredText base)
    {
        base.add_style(Style::Underline);
        return base;
    }

    [[nodiscard]] inline ColouredText italic(ColouredText base)
    {
        base.add_style(Style::Italic);
        return base;
    }

    [[nodiscard]] inline ColouredText blink(ColouredText base)
    {
        base.add_style(Style::Blink);
        return base;
    }

    [[nodiscard]] inline ColouredText reverse(ColouredText base)
    {
        base.add_style(Style::Reverse);
        return base;
    }

    [[nodiscard]] inline ColouredText hidden(ColouredText base)
    {
        base.add_style(Style::Hidden);
        return base;
    }

    [[nodiscard]] inline ColouredText strikethrough(ColouredText base)
    {
        base.add_style(Style::Strikethrough);
        return base;
    }
} // namespace terminal_utils
