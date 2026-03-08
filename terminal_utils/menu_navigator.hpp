// terminal_utils/menu_navigator.hpp
#pragma once

#include <string>
#include <stack>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include "menu.hpp"

namespace terminal_utils
{
    /**
     * @brief Manages navigation between named menus using an internal stack
     * 
     * Abstracts stack-based navigation so users of the library
     * don't need to manage a navigation stack manually
     * 
     * @code
     * MenuNavigator nav;
     * nav.add("main", [&](MenuNavigator& n)
     * {
     *     return Menu::create("Bank System")
     *         .add_item("Accounts", [&]{ n.push("accounts"); })
     *         .add_item("Quit",     [&]{ n.exit(); });
     * });
     * nav.add("accounts", [&](MenuNavigator& n)
     * {
     *     return Menu::create("Accounts")
     *         .add_item("Back", [&]{ n.pop(); });
     * });
     * nav.run("main");
     * @endcode
     */
    class MenuNavigator
    {
        public:
            /** @brief factory function type: builds and returns a `Menu` given the navigator */
            using MenuFactory = std::function<Menu(MenuNavigator&)>;

            /**
             * @brief register a named `menu` via a factory function
             * @param name    unique name identifying this menu
             * @param factory function that builds and returns the `Menu`
             * @return reference to this navigator (allows chaining)
             */
            MenuNavigator& add(std::string name, MenuFactory factory)
            {
                m_registry[std::move(name)] = std::move(factory);
                return *this;
            }

            /**
             * @brief push a named menu onto the navigation stack
             * 
             * The pushed `menu` becomes the active one on the next iteration of `run()`
             * 
             * @param name the registered name of the menu to navigate to
             * 
             * @throws `std::runtime_error` if `name` has not been registered via `add()`
             */
            void push(const std::string& name)
            {
                if(m_registry.find(name) == m_registry.end())
                    throw std::runtime_error("MenuNavigator: unknown menu '" + name + "'");

                m_stack.push(name);
            }

            /**
             * @brief pop the current `menu`: go back to the previous one
             */
            void pop()
            {
                if(!m_stack.empty())
                    m_stack.pop();
            }

            /**
             * @brief exit the navigator entirely and clears the navigation stack and stops the `run()` loop
             */
            void exit()
            {
                while(!m_stack.empty())
                    m_stack.pop();

                m_running = false;
            }

            /**
             * @brief start navigation from a named menu
             * 
             * `push()` `start_menu` onto the stack and enters the navigation loop
             * each iteration builds the current menu fresh via its factory, runs it,
             * and `pop()` the stack if the user quits
             * the loop exits when the stack is empty or `exit()` is called
             * 
             * @param start_menu Name of the initial menu to display
             * @throws `std::runtime_error` if `start_menu` has not been registered via `add()`
             */
            void run(const std::string& start_menu)
            {
                push(start_menu);
                m_running = true;

                while(m_running && !m_stack.empty())
                {
                    const std::string& current = m_stack.top();

                    auto it = m_registry.find(current);

                    // future-proofing safety guard: registry entry may have been removed after push()
                    if(it == m_registry.end())
                        break;

                    // Build the menu fresh each iteration (date updates, visibility predicates, etc...)
                    Menu menu = it->second(*this);
                    auto result = menu.run();

                    // 'q' on any menu = go back one level
                    if(result == MenuResult::Quit)
                        pop();
                }
            }


        private:
            std::unordered_map<std::string, MenuFactory> m_registry;         ///< maps menu names to their factory functions
            std::stack<std::string>                      m_stack;            ///< navigation history, current `menu` on top
            bool                                         m_running = false;  ///< controls the `run()` loop
    };

} // namespace terminal_utils
