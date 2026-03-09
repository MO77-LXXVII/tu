// tu/menu_item.hpp
#pragma once

#include <iostream>
#include <string>
#include <functional>

namespace tu
{
    namespace
    {
        /** @brief hides the terminal cursor using ANSI escape codes */
        void hide_cursor()
        {
            // Hide cursor (ANSI)
            std::cout << "\033[?25l";
        }

        /** @brief restores the terminal cursor visibility using ANSI escape codes */
        void show_cursor()
        {
            // Show cursor (ANSI)
            std::cout << "\033[?25h";
        }
    }

    /**
     * @brief represents a single entry in a `Menu`
     *
     * a `MenuItem` is either a **selectable item** with an associated action,
     * or a **visual separator** that cannot be selected or executed.
     *
     * visibility can be controlled dynamically via an optional callback,
     * allowing items to be shown or hidden based on runtime conditions.
     *
     * @note **separators** are skipped during navigation and cannot be executed
     */
    class MenuItem
    {
        public:
            /**
             * @brief constructs a new `MenuItem`
             *
             * @param label        text displayed in the menu
             * @param action       callable to execute when the item is selected (ignored if `is_separator` is `true` or if the item is not visible)
             * @param is_separator if `true`, this item acts as a visual separator and cannot be selected or executed (default: `false`)
             * @param visibility   optional callback that controls dynamic visibility; if `nullptr`, item is always visible (default: `nullptr`)
             */
            MenuItem(std::string label, std::function<void()> action, bool is_separator = false, std::function<bool()> visibility = nullptr)
                : m_label(std::move(label)), m_action(std::move(action)), m_is_separator(is_separator), m_visibility(std::move(visibility))
                {}


            /** @brief returns the text label of this `MenuItem` */
            [[nodiscard]] const std::string& label() const noexcept
            {
                return m_label;
            }


            /**
             * @brief checks whether this item is a visual separator
             * @return `true` if the item is a separator, `false` otherwise
             */
            [[nodiscard]] bool is_separator() const noexcept
            {
                return m_is_separator;
            }


            /**
             * @brief checks whether this item is currently visible
             *
             * if no visibility callback is set, the item is always visible
             * otherwise, delegates to the callback to determine visibility dynamically
             *
             * @return `true` if the item should be shown, `false` otherwise
             */
            [[nodiscard]] bool is_visible() const noexcept
            {
                if(!m_visibility)
                    return true;

                return m_visibility();
            }


            /**
             * @brief checks whether this item can be selected and executed
             *
             * an item is selectable if it is not a separator, has an associated action, and is currently visible
             *
             * @return `true` if the item is selectable, `false` otherwise
             */
            [[nodiscard]] bool is_selectable() const noexcept
            {
                return !m_is_separator && m_action && is_visible();
            }


            /**
             * @brief executes the item's associated action
             *
             * restores cursor visibility before invoking the action, since the menu hides it during navigation
             *
             * @note safe to call on a separator or invisible item; nothing happens if no action is set
             */
            void execute() const 
            { 
                if(m_action)
                {
                    show_cursor();
                    m_action(); 
                }
            }

        private:
            std::string m_label;                          /**< text label of the menu item */
            bool m_is_separator = false;                  /**< `true` if item is a separator */
            std::function<void()> m_action = nullptr;     /**< action to execute when selected */
            std::function<bool()> m_visibility = nullptr; /**< optional callback to determine dynamic visibility */
    };
} // namespace tu
