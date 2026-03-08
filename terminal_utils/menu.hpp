// terminal_utils/menu.hpp
#pragma once

#include <string>
#include <vector>
#include <csignal>
#include <functional>
#include <optional>
#include <string_view>

#include "menu_item.hpp"
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
     * Indicates whether the user **selected an item**, **Quit the menu**,
     * or if an **error occurred during input**.
     */
    enum class MenuResult
    {
        /** User **selected** a menu item. */
        Selected,

        /** User **quit** the menu (e.g., pressed 'q'). */
        Quit,

        /** An **input error** occurred. */
        Error
    };


     /**
     * @brief a terminal menu with keyboard navigation, selectable items, and visual separators
     *
     * supports building menus via a fluent interface, with configurable width, highlight colour,
     * title, subtitles, and an optional date display.
     * items can have dynamic visibility callbacks to show or hide them based on runtime conditions.
     *
     * navigation uses W/S (or J/K) to move, E/Enter to select, and Q to quit.
     *
     * @note `_global_subtitles` and `_date` are shared across all `Menu` instances via `inline static`
     *
     * @code
     * Menu::create("Main Menu")
     *     .set_width(40)
     *     .add_item("Option A", []{ do_a(); })
     *     .add_separator()
     *     .add_item("Option B", []{ do_b(); })
     *     .run();
     * @endcode
     */
    class Menu
    {
        public:
            /**
             * @brief constructs a `Menu` with the given title
             * @param title text displayed at the top of the menu
             * @note prefer using the `Menu::create()` factory for a more expressive fluent interface
             */
            explicit Menu(std::string title) 
                : _title(std::move(title)), _selected_index(-1) {}


            /**
             * @brief factory function to create a `Menu` with the given title
             *
             * preferred over the constructor for a more expressive fluent interface:
             * 
             * @code
             * Menu::create("Main Menu")
             *     .set_width(40)
             *     .add_item("Option A", []{ do_a(); })
             *     .run();
             * @endcode
             *
             * @param title text displayed at the top of the menu
             * @return a new `Menu` instance
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
             * @brief sets the total character width of the menu
             * @param width desired width in characters (default: `config::DEFAULT_MENU_WIDTH`)
             * @return reference to this `Menu` for chaining
             */
            Menu& set_width(int width) 
            {
                _width = width; 
                return *this; 
            }


            /**
             * @brief enables the date display in the menu title area
             *
             * sets `_date` to today's date and enables rendering it below the title.
             * shared across all `Menu` instances via `inline static`
             *
             * @return reference to this `Menu` for chaining
             */
            Menu& set_date() 
            { 
                _date = utils::Date::today();
                _view_date = true;
                return *this; 
            }


            /**
             * @brief disables the date display in the menu title area
             * @return reference to this `Menu` for chaining
             */
            Menu& reset_date() 
            {
                _view_date = false;
                return *this; 
            }


            /**
             * @brief sets the colour used to highlight the currently selected item
             * @param colour the highlight colour (default: `Colour::Cyan`)
             * @return reference to this `Menu` for chaining
             */
            Menu& set_highlight_colour(Colour colour)
            {
                _highlight_colour = colour;
                return *this;
            }


            /**
             * @brief adds a subtitle displayed below the title, shared across all `Menu` instances
             *
             * silently ignores duplicates: if the same text already exists it will not be added again.
             * useful for persistent info like *username* or *role*.
             *
             * @param text the subtitle text to add
             * @return reference to this `Menu` for chai    ning
             */
            Menu& add_global_subtitle(std::string text)
            {
                auto& subtitles = _global_subtitles; // local alias
    
                if(std::find(subtitles.begin(), subtitles.end(), text) == subtitles.end())
                    subtitles.emplace_back(std::move(text));
                return *this;
            }


            /**
             * @brief adds a subtitle displayed below the title, local to this `Menu` instance only
             * @param text the subtitle text to add
             * @return reference to this `Menu` for chaining
             */
            Menu& add_subtitle(std::string text)
            {
                _local_subtitles.emplace_back(std::move(text));
                return *this;
            }


            /**
             * @brief clears all global subtitles shared across `Menu` instances
             * @return reference to this `Menu` for chaining
             */
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
             * @brief adds a selectable item to the menu
             * @param label  text displayed in the menu
             * @param action callable to execute when the item is selected
             * @return reference to this `Menu` for chaining
             */
            Menu& add_item(std::string label, std::function<void()> action)
            {
                _items.emplace_back(std::move(label), std::move(action));
                return *this;
            }


            /**
             * @brief adds a selectable item with a dynamic visibility callback
             *
             * the item is only shown and selectable when the visibility callback returns `true`
             *
             * @param label      text displayed in the menu
             * @param action     callable to execute when the item is selected
             * @param visibility callback that controls whether the item is visible at runtime
             * @return reference to this `Menu` for chaining
             */
            Menu& add_item(std::string label, std::function<void()> action, std::function<bool()> visibility)
            {
                _items.emplace_back(std::move(label), std::move(action),
                                    false /* not a separator, this is a selectable item */, std::move(visibility));
                return *this;
            }


            /**
             * @brief adds a visual separator to the menu
             *
             * separators cannot be selected or executed and are skipped during navigation
             *
             * @return reference to this `Menu` for chaining
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
             * @brief runs the menu interactively until the user selects an item or quits
             *
             * handles keyboard navigation (W/S or J/K to move, E/Enter to select, Q to quit),
             * re-renders on each keypress, and executes the selected item's action before returning.
             *
             * @return `MenuResult::Selected` if an item was executed,
             *         `MenuResult::Quit`     if the user pressed 'q',
             *         `MenuResult::Error`    if not running in a terminal or no selectable items exist
             */
            [[nodiscard]] MenuResult run();


            /**
             * @brief renders the menu to `std::cout` without any interaction
             *
             * useful for previewing or debugging the menu layout
             */
            void display() const;


        private:
            /* 
                ======================
                UI rendering functions
                ======================
            */


            /** @brief clears the terminal and renders title, items, and footer in order */
            void render() const;

            /** @brief renders the title, global/local subtitles, and optional date, wrapped in borders */
            void render_title() const;

            /** @brief renders all menu items, highlighting the selected one and dimming invisible items */
            void render_items() const;

            /** @brief renders the bottom border and navigation key hints */
            void render_footer() const;


            /* 
                =======================
                UI navigation functions
                =======================
            */


            /** @brief returns the number of items that are currently selectable */
            [[nodiscard]] int _get_selectable_count() const;


            /**
             * @brief returns the index of the next selectable item after `current`
             *
             * @note wraps around to the first selectable item if the end is reached
             *
             * @param current the currently selected index
             * @return index of the next selectable item, or `current` if none exist
             */
            [[nodiscard]] int _next_selectable(int current) const;


            /**
             * @brief returns the index of the previous selectable item before `current`
             *
             * @note wraps around to the last selectable item if the beginning is reached
             *
             * @param current the currently selected index
             * @return index of the previous selectable item, or `current` if none exist
             */
            [[nodiscard]] int _prev_selectable(int current) const;


            /* 
                =======================
                Data Organization
                =======================
                
                - SHARED (inline static - same for all menus):
                    _global_subtitles  - Persistent info (username, role, system status)
                    _date              - Current date/time
                    _show_date         - Whether to show date in all menus

                - PER MENU (instance members):
                    _title             - This menu's title (e.g., "Main Menu", "Settings")
                    _local_subtitles   - This menu's dynamic info (e.g., "Page 2/5")
                    _items             - Menu options
                    _selected_index    - Current selection
                    _width             - Menu width
                    _highlight_colour  - Selection highlight color
                    _is_running        - Menu state
                =======================
            */


            // Shared across all instances
            inline static std::vector<std::string> _global_subtitles;   ///< persistent info shown in all menus (e.g. username, role)
            inline static utils::Date _date{};                          ///< current date displayed in the title area
            inline static bool _view_date = false;                      ///< whether to show the date in all menus

            // Per-menu instance
            std::string _title;                                         ///< text title displayed at the top of the menu
            std::vector<std::string> _local_subtitles;                  ///< dynamic info local to this menu instance (e.g. "Page 2/5")
            std::vector<MenuItem> _items;                               ///< list of menu items and separators
            int _selected_index = -1;                                   ///< index of the currently selected item
            int _width = config::DEFAULT_MENU_WIDTH;                    ///< total character width of the menu
            Colour _highlight_colour = Colour::Cyan;                    ///< colour used to highlight the selected item
            bool _is_running = false;                                   ///< whether the menu loop is currently active
    };


    namespace
    {
        constexpr int MENU_BORDER_WIDTH = 2;   ///< width of the `|` border characters on each side
        constexpr int MENU_SELECTOR_WIDTH = 2; ///< width of the `> ` selector prefix on selected items
        constexpr int MENU_PADDING = 2;        ///< empty space between text and border (`| txt |`)
                                               ///<                                           ^ ^


        /**
         * @brief renders a full-width horizontal border line
         * @param width     total menu width in characters
         * @param fill_char character to fill the border with (default: `'-'`, use `'='` for section separator)
         */
        void render_horizontal_border(int width, char fill_char = '-')
        {
            std::cout << "+" << std::string(width - MENU_BORDER_WIDTH, fill_char) << "+\n"; // Top border
        }


        /**
         * @brief renders a dimmed inline separator inside a menu row
         *
         * shorter than the full border to account for the `| ` padding on each side
         *
         * @param width     total menu width in characters
         * @param fill_char character to fill the separator with (default: `'-'`)
         */
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
        if(!platform::is_terminal())
            return MenuResult::Error;

        // Find first selectable item
        // validate index per-instance state:
        if(_selected_index == -1 || _selected_index >= static_cast<int>(_items.size()) || !_items[_selected_index].is_selectable())
            _selected_index = _next_selectable(-1);

        if(_selected_index < 0)
            return MenuResult::Error; // No selectable items

        _is_running = true;

        while(_is_running)
        {
            render();

            hide_cursor();

            auto key_opt = input::get_menu_key();

            if(!key_opt)
            {
                // Clear any pending input
                input::clear_failed_input();
                continue;
            }

            char key = static_cast<char>(std::tolower(static_cast<unsigned char>(*key_opt)));

            switch(key)
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
                    return MenuResult::Quit;

                default:
                    break;
            }
        }
        return MenuResult::Quit;
    }


    inline void Menu::display() const
    {
        render();
    }

} // namespace terminal_utils
