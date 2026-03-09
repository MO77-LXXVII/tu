// tu/style_wrappers.hpp

#pragma once

#include "ansi_colors.hpp"

namespace tu
{
    // ========================================================================
    // Color factory functions
    // ========================================================================

    // Initial creation from string (owns string)
    [[nodiscard]] inline ColoredText red(std::string text)
    {
        return ColoredText{std::move(text), Color::Red};
    }

    [[nodiscard]] inline ColoredText green(std::string text)
    {
        return ColoredText{std::move(text), Color::Green};
    }

    [[nodiscard]] inline ColoredText blue(std::string text)
    {
        return ColoredText{std::move(text), Color::Blue};
    }

    [[nodiscard]] inline ColoredText cyan(std::string text)
    {
        return ColoredText{std::move(text), Color::Cyan};
    }

    [[nodiscard]] inline ColoredText magenta(std::string text)
    {
        return ColoredText{std::move(text), Color::Magenta};
    }

    [[nodiscard]] inline ColoredText yellow(std::string text)
    {
        return ColoredText{std::move(text), Color::Yellow};
    }

    [[nodiscard]] inline ColoredText white(std::string text)
    {
        return ColoredText{std::move(text), Color::White};
    }

    [[nodiscard]] inline ColoredText black(std::string text)
    {
        return ColoredText{std::move(text), Color::Black};
    }

    /*
        Chaining on existing object:
        note: `base` is a function parameter, NRVO is excluded (C++ standard, [class.copy.elision])
            all functions in this section use implicit move on return
    */
    [[nodiscard]] inline ColoredText red(ColoredText base)
    {
        base.set_color(Color::Red);
        return base;
    }

    [[nodiscard]] inline ColoredText green(ColoredText base)
    {
        base.set_color(Color::Green);
        return base;
    }

    [[nodiscard]] inline ColoredText blue(ColoredText base)
    {
        base.set_color(Color::Blue);
        return base;
    }

    [[nodiscard]] inline ColoredText cyan(ColoredText base)
    {
        base.set_color(Color::Cyan);
        return base;
    }

    [[nodiscard]] inline ColoredText magenta(ColoredText base)
    {
        base.set_color(Color::Magenta);
        return base;
    }

    [[nodiscard]] inline ColoredText yellow(ColoredText base)
    {
        base.set_color(Color::Yellow);
        return base;
    }

    [[nodiscard]] inline ColoredText white(ColoredText base)
    {
        base.set_color(Color::White);
        return base;
    }

    [[nodiscard]] inline ColoredText black(ColoredText base)
    {
        base.set_color(Color::Black);
        return base;
    }

    // ========================================================================
    // Style factory functions
    // ========================================================================

    // Initial creation (owns string)
    [[nodiscard]] inline ColoredText bold(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Bold};
    }

    [[nodiscard]] inline ColoredText dim(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Dim};
    }

    [[nodiscard]] inline ColoredText underline(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Underline};
    }

    [[nodiscard]] inline ColoredText italic(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Italic};
    }

    [[nodiscard]] inline ColoredText blink(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Blink};
    }

    [[nodiscard]] inline ColoredText reverse(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Reverse};
    }

    [[nodiscard]] inline ColoredText hidden(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Hidden};
    }

    [[nodiscard]] inline ColoredText strikethrough(std::string text)
    {
        return ColoredText{std::move(text), Color::None, Style::Strikethrough};
    }

    /*
        Chaining (stack styles)
        note: `base` is a function parameter, NRVO is excluded (C++ standard, [class.copy.elision])
              all functions in this section use implicit move on return
    */
    [[nodiscard]] inline ColoredText bold(ColoredText base)
    {
        base.add_style(Style::Bold);
        return base;
    }

    [[nodiscard]] inline ColoredText dim(ColoredText base)
    {
        base.add_style(Style::Dim);
        return base;
    }

    [[nodiscard]] inline ColoredText underline(ColoredText base)
    {
        base.add_style(Style::Underline);
        return base;
    }

    [[nodiscard]] inline ColoredText italic(ColoredText base)
    {
        base.add_style(Style::Italic);
        return base;
    }

    [[nodiscard]] inline ColoredText blink(ColoredText base)
    {
        base.add_style(Style::Blink);
        return base;
    }

    [[nodiscard]] inline ColoredText reverse(ColoredText base)
    {
        base.add_style(Style::Reverse);
        return base;
    }

    [[nodiscard]] inline ColoredText hidden(ColoredText base)
    {
        base.add_style(Style::Hidden);
        return base;
    }

    [[nodiscard]] inline ColoredText strikethrough(ColoredText base)
    {
        base.add_style(Style::Strikethrough);
        return base;
    }
} // namespace tu
