// terminal_utils/menu.hpp
#pragma once

#include <string>
#include <vector>
#include <csignal>
#include <functional>
#include <optional>
#include <string_view>

#include "menu_item.hpp"
#include "../input.hpp"
#include "../ansi_colours.hpp"
#include "../style_wrappers.hpp"

#include "../../platform/platform.hpp"
#include "../../utils/date.hpp"

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
     * @note `m_global_subtitles` and `m_date` are shared across all `Menu` instances via `inline static`
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
                : m_title(std::move(title)), m_selected_index(-1) {}


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
                m_width = width; 
                return *this; 
            }


            /**
             * @brief enables the date display in the menu title area
             *
             * sets `m_date` to today's date and enables rendering it below the title.
             * shared across all `Menu` instances via `inline static`
             *
             * @return reference to this `Menu` for chaining
             */
            Menu& set_date() 
            { 
                m_date = utils::Date::today();
                m_show_date = true;
                return *this; 
            }


            /**
             * @brief disables the date display in the menu title area
             * @return reference to this `Menu` for chaining
             */
            Menu& reset_date() 
            {
                m_show_date = false;
                return *this; 
            }


            /**
             * @brief sets the colour used to highlight the currently selected item
             * @param colour the highlight colour (default: `Colour::Cyan`)
             * @return reference to this `Menu` for chaining
             */
            Menu& set_highlight_colour(Colour colour)
            {
                m_highlight_colour = colour;
                return *this;
            }


            /**
             * @brief adds a subtitle displayed below the title, shared across all `Menu` instances
             *
             * silently ignores duplicates: if the same text already exists it will not be added again.
             * useful for persistent info like *username* or *role*.
             *
             * @param text the subtitle text to add
             * @return reference to this `Menu` for chaining
             */
            Menu& add_global_subtitle(std::string text)
            {
                auto& subtitles = m_global_subtitles; // local alias
    
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
                m_local_subtitles.emplace_back(std::move(text));
                return *this;
            }


            /**
             * @brief clears all global subtitles shared across `Menu` instances
             * @return reference to this `Menu` for chaining
             */
            Menu& reset_global_subtitles()
            {
                m_global_subtitles.clear();
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
                m_items.emplace_back(std::move(label), std::move(action));
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
                m_items.emplace_back(std::move(label), std::move(action),
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
                m_items.emplace_back("-", nullptr, true);
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


            /* 
                ======================
                Diagnostic methods
                ======================
            */


            /**
             * @brief returns the number of items that are currently selectable
             * 
             * @note **diagnostic function**: useful for debugging menu state and validation.
             * 
             * Example debug usage:
             * 
             * @code
             * Menu m = Menu::create("Test Menu")
             *          .add_separator();
             *
             * if(m.get_selectable_count() == 0)
             *     std::cout << "Menu has no selectable items\n";
             * @endcode
             * 
             * @return number of selectable items in the menu
             */
            [[nodiscard]] int get_selectable_count() const;

        private:
            /*
                ======================
                UI rendering methods
                ======================
            */


            /** @brief clears the terminal and renders title, items, and footer in order */
            void m_render() const;

            /** @brief renders the title, global/local subtitles, and optional date, wrapped in borders */
            void m_render_title() const;

            /** @brief renders all menu items, highlighting the selected one and dimming invisible items */
            void m_render_items() const;

            /** @brief renders the bottom border and navigation key hints */
            void m_render_footer() const;


            /* 
                =======================
                UI navigation methods
                =======================
            */


            /**
             * @brief returns the index of the next selectable item after `current`
             *
             * @note wraps around to the first selectable item if the end is reached
             *
             * @param current the currently selected index
             * @return index of the next selectable item, or `current` if none exist
             */
            [[nodiscard]] int m_next_selectable(int current) const;


            /**
             * @brief returns the index of the previous selectable item before `current`
             *
             * @note wraps around to the last selectable item if the beginning is reached
             *
             * @param current the currently selected index
             * @return index of the previous selectable item, or `current` if none exist
             */
            [[nodiscard]] int m_prev_selectable(int current) const;


            /* 
                =======================
                Data Organization
                =======================
                
                - SHARED (inline static - same for all menus):
                    m_global_subtitles  - Persistent info (username, role, system status)
                    _date              - Current date/time
                    m_show_date         - Whether to show date in all menus

                - PER MENU (instance members):
                    m_title             - This menu's title (e.g., "Main Menu", "Settings")
                    m_local_subtitles   - This menu's dynamic info (e.g., "Page 2/5")
                    _items             - Menu options
                    m_selected_index    - Current selection
                    m_width             - Menu width
                    m_highlight_colour  - Selection highlight color
                =======================
            */


            // Shared across all instances
            inline static std::vector<std::string> m_global_subtitles;   ///< persistent info shown in all menus (e.g. username, role)
            inline static utils::Date m_date{};                          ///< current date displayed in the title area
            inline static bool m_show_date = false;                      ///< whether to show the date in all menus

            // Per-menu instance
            std::string m_title;                                         ///< text title displayed at the top of the menu
            std::vector<std::string> m_local_subtitles;                  ///< dynamic info local to this menu instance (e.g. "Page 2/5")
            std::vector<MenuItem> m_items;                               ///< list of menu items and separators
            int m_selected_index = -1;                                   ///< index of the currently selected item
            int m_width = config::DEFAULT_MENU_WIDTH;                    ///< total character width of the menu
            Colour m_highlight_colour = Colour::Cyan;                    ///< colour used to highlight the selected item
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
            std::cout << "+" << std::string(width - MENU_BORDER_WIDTH, fill_char) << "+\n";
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


    /* 
        ======================================
                UI rendering methods
        ======================================
    */


    /**
     * @brief renders the menu title area including subtitles and optional date
     *
     * output structure:
     * 
     * @code
     * +--------------------------------------+   <- top border
     * |            Bank System               |   <- title (bold, underlined)
     * |        Logged in as: admn            |   <- global subtitles (bold, green, underlined)
     * |           Date: 08/03/2026           |   <- date (if enabled) (bold, underlined)
     * +======================================+   <- section separator
     * @endcode
     */
    inline void Menu::m_render_title() const
    {
        using namespace terminal_utils;

        render_horizontal_border(m_width); // Top border

        // subtract border_padding to account for the '|' characters we manually print on each side
        constexpr int border_padding = 2;
        
        std::cout << "|" << output::print_aligned(bold(underline(m_title)), m_width - border_padding, output::Alignment::Center) << "|" << std::endl;

        for(const auto& subtitle : m_global_subtitles)
            std::cout << "|" << output::print_aligned(bold(green(underline(subtitle))), m_width - border_padding, output::Alignment::Center) << "|" << std::endl;
        
        for(const auto& subtitle : m_local_subtitles)
            std::cout << "|" << output::print_aligned(bold(green(underline(subtitle))), m_width - border_padding, output::Alignment::Center) << "|" << std::endl;

        if(m_show_date)
                std::cout << "|" << output::print_aligned(bold(underline("Date: " + m_date.format())), m_width - border_padding, output::Alignment::Center) << "|" << std::endl;

        render_horizontal_border(m_width, '='); // Separator
    }


    /**
     * @brief renders all menu items with selection highlighting and visibility dimming
     *
     * each item is rendered as one of:
     *
     * @code
     * | ------------------------------------ |   <- separator
     * | > Accounts                           |   <- selected item (highlighted)
     * |   Transactions                       |   <- normal item
     * |   Restricted Option                  |   <- invisible item (dimmed)
     * @endcode
     *
     * @note *labels longer* than the available width are **truncated** with `...`
     * @note *invisible items* are rendered *dimmed* and cannot be navigated to
     * 
     * @code
     * | > Accounts$$$$$$$$$$$$$$$$$$$$$$$... |
     *                                    ^^^
     *                                Truncated
     * @endcode
     */
    inline void Menu::m_render_items() const
    {
        // iterate through all menu items (including separators)
        for(std::size_t i = 0; i < m_items.size(); ++i)
        {
            const auto& item = m_items[i];
            bool is_selected = (static_cast<int>(i) == m_selected_index);

            // left border: every row starts with "| "
            std::cout << "| ";

            // --- SEPARATOR HANDLING ---
            if(item.is_separator())
                render_separator(m_width); // render a horizontal line across the menu width

            // --- MENU ITEM HANDLING (not a separator) ---
            else
            {
                std::string label = item.label();

                /*
                    calculate available space for the label:
                    m_width                  : total menu width
                    MENU_BORDER_WIDTH       : accounts for left and right "|" and "|" (2 characters)
                    MENU_SELECTOR_WIDTH     : space for "> " or "  " (2 characters)
                    MENU_PADDING            : extra padding spaces (2 characters)

                    example: if m_width = 40, border = 2, selector = 2, padding = 2
                    max_label_width = 40 - (2 + 2 + 2) = 34 characters for the label
                */
                int max_label_width = m_width - (MENU_BORDER_WIDTH + MENU_SELECTOR_WIDTH + MENU_PADDING);

                if(static_cast<int>(label.length()) > max_label_width)
                    label = label.substr(0, max_label_width - 3) + "..."; // truncate and add ellipsis: "Very Long Menu Option..." 

                /*
                    calculate right padding to maintain consistent menu width
                    if `label` is shorter than `max_label_width`, add spaces on the right
                    Example: "  Accounts    " (with extra spaces to fill to `max_label_width`)
                 */
                int padding = max_label_width - static_cast<int>(label.length());

                // --- VISIBILITY STATES (3 possible render modes) ---

                // 1. INVISIBLE ITEM: dimmed, cursor cannot select
                if(!item.is_visible())
                {
                    // Render with tu:: dim()
                    // Uses "  " (no selector) + dimmed label
                    std::cout << dim("  " + label);
                    std::cout << std::string(std::max(0, padding), ' ');
                }

                // 2. SELECTED VISIBLE ITEM: highlighted with cursor
                else if(is_selected)
                {
                    // Render with highlight colour (e.g., Cyan)
                    // Uses "> " as selector + highlighted label
                    std::cout << ColouredText(("> " + label), m_highlight_colour);
                    std::cout << std::string(std::max(0, padding), ' ');
                }

                // 3. NORMAL VISIBLE ITEM: plain, no highlight, no cursor
                else
                {
                    // Uses "  " (no selector) + plain label
                    std::cout << "  " << label;
                    std::cout << std::string(std::max(0, padding), ' ');
                }
            }
            // Right border - every row ends with " |" and newline
            std::cout << " |\n";
        }
    }


    /**
     * @brief renders the bottom border and navigation key hints
     *
     * @code
     * +--------------------------------------+
     * W/K: Up  S/J: Down  E: Select  q: Quit
     * @endcode
     */
    inline void Menu::m_render_footer() const
    {
        // Bottom border
        render_horizontal_border(m_width);

        // Help text
        std::cout << dim("W/K: Up  S/J: Down  E: Select  q: Quit") << "\n";
    }
    

    /**
     * @brief clears the terminal and renders the full menu interface
     *
     * renders in order: title → items → footer
     */
    inline void Menu::m_render() const
    {
        platform::clear_terminal();
        m_render_title();
        m_render_items();
        m_render_footer();
    }


    /* 
        ======================
         Diagnostic methods
        ======================
    */


    inline int Menu::get_selectable_count() const
    {
        return std::count_if(m_items.begin(), m_items.end(),[](const MenuItem& item)
        {
            return item.is_selectable();
        });
    }


    /* 
        ======================================
        UI navigation methods implementation
        ======================================
    */


    inline int Menu::m_next_selectable(int current) const
    {
        for(int i = current + 1; i < m_items.size(); ++i)
            if(m_items[i].is_selectable())
                return i;

        // Wrap to first selectable
        for(int i = 0; i < m_items.size(); ++i)
            if(m_items[i].is_selectable())
                return i;

        return current;
    }


    inline int Menu::m_prev_selectable(int current) const
    {
        for(int i = current - 1; i >= 0; --i)
            if(m_items[i].is_selectable())
                return i;

        // Wrap to last selectable
        for(int i = static_cast<int>(m_items.size()) - 1; i >= 0; --i)
            if(m_items[i].is_selectable())
                return i;

        return current;
    }


    inline MenuResult Menu::run()
    {
        if(!platform::is_terminal())
            return MenuResult::Error;

        // initialize or reset `m_selected_index` if it's unset, out of bounds, or pointing to a non-selectable item
        if(m_selected_index == -1 || m_selected_index >= static_cast<int>(m_items.size()) || !m_items[m_selected_index].is_selectable())
            m_selected_index = m_next_selectable(-1);

        if(m_selected_index < 0)
            return MenuResult::Error; // m_next_selectable() returns -1 when no selectable items exist

        while(true)
        {
            m_render();
            hide_cursor();

            auto key_opt = input::get_menu_key();

            if(!key_opt)
            {
                // clear any pending input before retrying
                input::clear_failed_input();
                continue;
            }

            char key = static_cast<char>(std::tolower(static_cast<unsigned char>(*key_opt)));

            switch(key)
            {
                case 'j': // vim-style down
                    [[fallthrough]];
                case 's':
                    m_selected_index = m_next_selectable(m_selected_index);
                    break;

                case 'k': // vim-style up
                    [[fallthrough]];
                case 'w':
                    m_selected_index = m_prev_selectable(m_selected_index);
                    break;

                case '\n': // Enter key (various terminals)
                    [[fallthrough]];
                case '\r':
                case 'e':
                    if(m_selected_index >= 0 && m_selected_index < static_cast<int>(m_items.size()))
                    {
                        const auto& item = m_items[m_selected_index];
                        if(item.is_selectable())
                        {
                            // execute() calls show_cursor() internally before invoking the action
                            item.execute();
                            return MenuResult::Selected;
                        }
                    }
                    break;

                case 'q':
                    show_cursor(); // restore cursor before exiting
                    return MenuResult::Quit;

                default:
                    break;
            }
        }
        return MenuResult::Quit; // unreachable; silences -Wreturn-type warning
    }


    inline void Menu::display() const
    {
        m_render();
    }

} // namespace terminal_utils
