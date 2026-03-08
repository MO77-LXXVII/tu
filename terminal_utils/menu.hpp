// terminal_utils/menu.hpp
#pragma once

#include <string>
#include <vector>
#include <csignal>
#include <functional>
#include <optional>
#include <string_view>

#include "input.hpp"
#include "ansi_colours.hpp"
#include "style_wrappers.hpp"
#include "../platform/platform.hpp"
#include "../utils/date.hpp"

namespace terminal_utils
{
    /**
     * @brief Represents the result of a menu interaction.
     * 
     * Indicates whether the user selected an item, cancelled the menu,
     * or if an error occurred during input.
     */
    enum class MenuResult
    {
        /** User **selected** a menu item. */
        Selected,

        /** User **cancelled** the menu (e.g., pressed 'q'). */
        Cancelled,

        /** An **input error** occurred. */
        Error
    };





    namespace
    {
        void hide_cursor()
        {
            std::cout << "\033[?25l"; // Hide cursor (ANSI)
        }

        void show_cursor()
        {
            // Show cursor (ANSI)
            std::cout << "\033[?25h";
        }
    }







    /**
     * @brief Represents a single `Menu` entry.
     * 
     * Can be a *selectable item* with an associated action, or a *visual separator*.
     */
    class MenuItem
    {
        public:
            /**
             * @brief Construct a new MenuItem.
             * 
             * @param label Text displayed in the menu.
             * @param action Callable function to execute when selected. (Ignored if this is a separator).
             * @param is_separator If true, this item is a *visual separator* and cannot be selected.
             */
            MenuItem(std::string label, std::function<void()> action, bool is_separator = false, std::function<bool()> visibility = nullptr)
                : _label(std::move(label)), _action(std::move(action)), _is_separator(is_separator), _visibility(std::move(visibility))
                {}

            /**
             * @brief Get the label of the menu item.
             * 
             * @return `const std::string&` The text label.
             */
            [[nodiscard]] const std::string& label() const noexcept
            {
                return _label;
            }

            /**
             * @brief Check if the item is a separator.
             * 
             * @return `true` If the item is a separator.
             * @return `false` Otherwise.
             */
            [[nodiscard]] bool is_separator() const noexcept
            {
                return _is_separator;
            }

            [[nodiscard]] bool is_visible() const noexcept
            {
                if(!_visibility)
                    return true;

                return _visibility();
            }

            /**
             * @brief Check if the item can be selected.
             * 
            * @return `true`
            *         If the item is selectable (not a separator and has an action).
            * @return `false`
            *         Otherwise.

             */
            [[nodiscard]] bool is_selectable() const noexcept
            {
                return !_is_separator && _action && is_visible();
            }

            /**
             * @brief Execute the item's associated action.
             * 
             * Safe to call even if the item is not selectable; nothing happens in that case.
             */
            void execute() const 
            { 
                if(_action)
                {
                    show_cursor();
                    _action(); 
                }
            }

        private:
            std::string _label;               /**< Text label of the menu item */
            std::function<void()> _action;    /**< Action to execute when selected */
            bool _is_separator = false;       /**< `true` if item is a separator */
            std::function<bool()> _visibility = nullptr;
    };










    /**
     * @brief A terminal menu system with selectable items and separators.
     * 
     * This class allows building, displaying, and interacting with a simple
     * text-based menu in the terminal. Items can be added with actions, and
     * separators can be included for visual grouping. Supports highlighting
     * the selected item with configurable colour.
     */
    class Menu
    {
        public:
            /**
             * @brief Construct a menu with a given title.
             * @param title The text title displayed at the top of the menu.
             */
            explicit Menu(std::string title) 
                : _title(std::move(title)), _selected_index(-1) {}

            /**
             * @brief Create a menu via a static factory.
             * @param title The text title displayed at the top of the menu.
             * @return A new Menu instance with the specified title.
             */
            [[nodiscard]] static Menu create(std::string title)
            {
                return Menu(std::move(title));
            }

            /* 
                ==============
                Configurations
                ==============
            */

            /**
             * @brief Set the width of the menu (total characters per line).
             * @param width The desired width of the menu.
             * @return Reference to this menu (allows chaining).
             */
            Menu& set_width(int width) 
            { 
                _width = width; 
                return *this; 
            }

            Menu& set_date() 
            { 
                _date = utils::Date::today();
                _view_date = true;
                return *this; 
            }

            Menu& reset_date() 
            {
                _view_date = false;
                return *this; 
            }

            /**
             * @brief Set the colour used to highlight the selected item.
             * @param colour The highlight colour.
             * @return Reference to this menu (allows chaining).
             */
            Menu& set_highlight_colour(Colour colour)
            {
                _highlight_colour = colour;
                return *this;
            }

            Menu& add_global_subtitle(std::string text)
            {
                for(const auto& s: _global_subtitles)
                    if(s == text)
                        return *this; // already exists, skip

                _global_subtitles.emplace_back(std::move(text));
                return *this;
            }

            Menu& add_subtitle(std::string text)
            {
                _local_subtitles.emplace_back(std::move(text));
                return *this;
            }

            Menu& reset_global_subtitles()
            {
                _global_subtitles.clear();
                return *this;
            }

            /* 
                ===============
                Adding elements
                ===============
            */

            /**
             * @brief Add a selectable item to the menu.
             * @param label The text label of the menu item.
             * @param action The function to execute when the item is selected.
             * @return Reference to this menu (allows chaining).
             */
            Menu& add_item(std::string label, std::function<void()> action)
            {
                _items.emplace_back(std::move(label), std::move(action));
                return *this;
            }


            Menu& add_item(std::string label, std::function<void()> action, std::function<bool()> visibility)
            {
                _items.emplace_back(std::move(label), std::move(action), false, std::move(visibility));
                return *this;
            }

            /**
             * @brief Add a visual separator to the menu.
             * @return Reference to this menu (allows chaining).
             */
            Menu& add_separator()
            {
                _items.emplace_back("-", nullptr, true);
                return *this;
            }



            /* 
                ==========================
                Run the menu interactively
                ==========================
            */


            /**
             * @brief Run the menu interactively.
             * Allows the user to navigate, select items, or cancel.
             * @return The result of the menu interaction (Selected, Cancelled, or Error).
             */
            [[nodiscard]] MenuResult run();

            /**
             * @brief Display the menu without interaction.
             * Useful for rendering the menu for preview purposes.
             */
            void display() const;

            /**
             * @brief Run the menu once and return the selected item index.
             * @return Optional index of the selected item, or std::nullopt if cancelled or error.
             */
            [[nodiscard]] std::optional<std::size_t> run_once();


        private:
            /* 
                ======================
                UI rendering functions
                ======================
            */

            /** @brief Render the full menu interface. */
            void render() const;

            /** @brief Render only the menu title at the top. */
            void render_title() const;

            /** @brief Render all menu items with highlighting. */
            void render_items() const;

            /** @brief Render the menu footer with instructions. */
            void render_footer() const;

            /* 
                =======================
                UI navigation functions
                =======================
            */

            /** @brief Count how many items are selectable. */
            [[nodiscard]] int _get_selectable_count() const;

            /** @brief Get the next selectable item after the current index.
             *  Wraps around to the first selectable item if needed.
             *  @param current Current selected index.
             *  @return Index of the next selectable item.
             */
            [[nodiscard]] int _next_selectable(int current) const;

            /** @brief Get the previous selectable item before the current index.
             *  Wraps around to the last selectable item if needed.
             *  @param current Current selected index.
             *  @return Index of the previous selectable item.
             */
            [[nodiscard]] int _prev_selectable(int current) const;

            std::string _title;                        /**< Text title displayed at the top of the menu */
            inline static std::vector<std::string> _subtitles;       /**< Text title displayed below `_title` */

            inline static std::vector<std::string> _global_subtitles;   // persists across all menus (username, role)
            std::vector<std::string> _local_subtitles;                  // per-menu instance only

            std::vector<MenuItem> _items;              /**< List of menu items and separators */
            int _selected_index = -1;                  /**< Index of the currently selected item */
            int _width = config::DEFAULT_MENU_WIDTH;   /**< Width of the menu in characters */
            Colour _highlight_colour = Colour::Cyan;   /**< Colour used to highlight selected item */
            bool _is_running = false;                  /**< Whether the menu is currently running */

            // shared among all instances to avoid calling `set_date()` per instance
            inline static utils::Date _date{};
            inline static bool _view_date = false;
    };

    namespace
    {
        constexpr int MENU_BORDER_WIDTH = 2;   // "|"
        constexpr int MENU_SELECTOR_WIDTH = 2; // "> "
        constexpr int MENU_PADDING = 2;  // empty space between txt & | ("| txt |")
                                                //                                ^   ^

        void render_horizontal_border(int width, char fill_char = '-')
        {
            std::cout << "+" << std::string(width - MENU_BORDER_WIDTH, fill_char) << "+\n"; // Top border
        }

        void render_separator(int width, char fill_char = '-')
        {
            std::cout << dim(std::string(width - (MENU_BORDER_WIDTH + MENU_PADDING), fill_char));
        }
    }








    inline void Menu::render_title() const
    {
        using namespace terminal_utils;

        render_horizontal_border(_width); // Top border

        constexpr int border_padding = 2; // account for '|' we manually print on each side
        
        std::cout << "|" << output::print_aligned(bold(underline(_title)), _width - border_padding, output::Alignment::Center) << "|" << std::endl;

        for(const auto& subtitle : _global_subtitles)
            std::cout << "|" << output::print_aligned(bold(green(underline(subtitle))), _width - border_padding, output::Alignment::Center) << "|" << std::endl;
        
        for(const auto& subtitle : _local_subtitles)
            std::cout << "|" << output::print_aligned(bold(green(underline(subtitle))), _width - border_padding, output::Alignment::Center) << "|" << std::endl;

        if(_view_date)
                std::cout << "|" << output::print_aligned(bold(underline("Date: " + _date.format())), _width - border_padding, output::Alignment::Center) << "|" << std::endl;

        render_horizontal_border(_width, '='); // Separator
    }
    










    inline void Menu::render_items() const
    {
        for(std::size_t i = 0; i < _items.size(); ++i)
        {
            const auto& item = _items[i];
            bool is_selected = (static_cast<int>(i) == _selected_index);

            std::cout << "| ";

            if(item.is_separator())
                render_separator(_width);

            // else
            // {
            //     std::string label = item.label();

            //     int max_label_width = _width - (MENU_BORDER_WIDTH + MENU_SELECTOR_WIDTH + MENU_PADDING);

            //     /*
            //         Truncate if too long
            //         Example:
            //         | > Accounts$$$$$$$$$$$$$$$$$$$$$$$... |
            //                                            ^^^
            //                                            Truncated                
            //     */
            //     if(static_cast<int>(label.length()) > max_label_width)
            //         label = label.substr(0, max_label_width - 3) + "...";

            //     if(int padding = max_label_width - static_cast<int>(label.length()); is_selected)
            //     {
            //         // if selected
            //         // | > Accounts                           |
            //         std::cout << ColouredText(("> " + label), _highlight_colour);

            //         // Pad to align right border
            //         std::cout << std::string(std::max(0, padding), ' ');
            //     }

            //     else
            //     {
            //         // if not selected
            //         // |   Transactions                       |
            //         std::cout << "  " << label;
            //         std::cout << std::string(std::max(0, padding), ' ');
            //     }
            // }

            else
            {
                std::string label = item.label();

                int max_label_width = _width - (MENU_BORDER_WIDTH + MENU_SELECTOR_WIDTH + MENU_PADDING);

                if(static_cast<int>(label.length()) > max_label_width)
                    label = label.substr(0, max_label_width - 3) + "...";

                int padding = max_label_width - static_cast<int>(label.length());

                if(!item.is_visible())
                {
                    // restricted — always dim, cursor never lands here
                    std::cout << dim("  " + label);
                    std::cout << std::string(std::max(0, padding), ' ');
                }
                else if(is_selected)
                {
                    std::cout << ColouredText(("> " + label), _highlight_colour);
                    std::cout << std::string(std::max(0, padding), ' ');
                }
                else
                {
                    std::cout << "  " << label;
                    std::cout << std::string(std::max(0, padding), ' ');
                }
            }
            std::cout << " |\n";
        }
    }



/*


if(item.is_separator())
{
    buf << dim(std::string(_width - (MENU_BORDER_WIDTH + MENU_PADDING), '-'));
}
else if(!item.is_visible())
{
    buf << dim("  " + label);
    buf << std::string(std::max(0, padding), ' ');
}
else if(is_selected)
{
    buf << ColouredText(("> " + label), _highlight_colour);
    buf << std::string(std::max(0, padding), ' ');
}
else
{
    buf << "  " << label;
    buf << std::string(std::max(0, padding), ' ');
}


*/







    inline void Menu::render_footer() const
    {
        // Bottom border
        render_horizontal_border(_width);

        // Help text
        std::cout << dim("W: Up  S: Down  E: Select  q: Quit") << "\n";
    }
    










    inline void Menu::render() const
    {
        platform::clear_terminal();
        render_title();
        render_items();
        render_footer();
    }
    










    inline int Menu::_get_selectable_count() const
    {
        int count = 0;
        for(const auto& item : _items)
            if(item.is_selectable())
                ++count;

        return count;
    }
    










    inline int Menu::_next_selectable(int current) const
    {
        for(int i = current + 1; i < _items.size(); ++i)
            if(_items[i].is_selectable())
                return i;

        // Wrap to first selectable
        for(int i = 0; i < _items.size(); ++i)
            if(_items[i].is_selectable())
                return i;

        return current;
    }
    










    inline int Menu::_prev_selectable(int current) const
    {
        for(int i = current - 1; i >= 0; --i)
            if(_items[i].is_selectable())
                return i;

        // Wrap to last selectable
        for(int i = static_cast<int>(_items.size()) - 1; i >= 0; --i)
            if(_items[i].is_selectable())
                return i;

        return current;
    }








    inline MenuResult Menu::run()
    {
        if (!platform::is_terminal())
            return MenuResult::Error;

        // Find first selectable item
        // validate index per-instance state:
        if(_selected_index == -1 || _selected_index >= static_cast<int>(_items.size()) || !_items[_selected_index].is_selectable())
            _selected_index = _next_selectable(-1);

        if (_selected_index < 0)
            return MenuResult::Error; // No selectable items

        _is_running = true;

        while (_is_running)
        {
            render();

            hide_cursor();

            auto key_opt = input::get_menu_key();

            if (!key_opt)
            {
                // Clear any pending input
                input::clear_failed_input();
                continue;
            }

            char key = static_cast<char>(std::tolower(static_cast<unsigned char>(*key_opt)));

            switch (key)
            {
                case 'j':
                    [[fallthrough]];
                case 's':
                    _selected_index = _next_selectable(_selected_index);
                    break;

                case 'k':
                    [[fallthrough]];
                case 'w':
                    _selected_index = _prev_selectable(_selected_index);
                    break;

                case '\n':
                    [[fallthrough]];
                case '\r':
                case 'e':
                    if(_selected_index >= 0 && _selected_index < static_cast<int>(_items.size()))
                    {
                        const auto& item = _items[_selected_index];
                        if(item.is_selectable())
                        {
                            item.execute();
                            return MenuResult::Selected;
                        }
                    }
                    break;

                case 'q':
                    _is_running = false;
                    show_cursor();
                    return MenuResult::Cancelled;

                default:
                    break;
            }
        }
        return MenuResult::Cancelled;
    }







    inline void Menu::display() const
    {
        render();
    }

    inline std::optional<std::size_t> Menu::run_once()
    {
        auto result = run();
        if (result == MenuResult::Selected)
            return static_cast<std::size_t>(_selected_index);

        return std::nullopt;
    }

} // namespace terminal_utils
