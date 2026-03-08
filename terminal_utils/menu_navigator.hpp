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
     * @brief Manages navigation between named menus using an internal stack.
     * 
     * Abstracts stack-based navigation so users of the library
     * don't need to manage a navigation stack manually.
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
        /** @brief Factory function type — builds and returns a Menu given the navigator. */
        using MenuFactory = std::function<Menu(MenuNavigator&)>;

        /**
         * @brief Register a named menu via a factory function.
         * @param name   Unique name identifying this menu.
         * @param factory Function that builds and returns the Menu.
         * @return Reference to this navigator (allows chaining).
         */
        MenuNavigator& add(std::string name, MenuFactory factory)
        {
            _registry[std::move(name)] = std::move(factory);
            return *this;
        }

        /**
         * @brief Push a named menu onto the navigation stack.
         * @param name The registered name of the menu to navigate to.
         */
        void push(const std::string& name)
        {
            if(_registry.find(name) == _registry.end())
                throw std::runtime_error("MenuNavigator: unknown menu '" + name + "'");

            _stack.push(name);
        }

        /**
         * @brief Pop the current menu — go back to the previous one.
         */
        void pop()
        {
            if(!_stack.empty())
                _stack.pop();
        }

        /**
         * @brief Exit the navigator entirely.
         */
        void exit()
        {
            while(!_stack.empty())
                _stack.pop();

            _running = false;
        }

        /**
         * @brief Start navigation from a named menu.
         * @param start_menu Name of the initial menu to display.
         */
        void run(const std::string& start_menu)
        {
            push(start_menu);
            _running = true;

            while(_running && !_stack.empty())
            {
                const std::string& current = _stack.top();

                auto it = _registry.find(current);
                if(it == _registry.end())
                    break;

                // Build the menu fresh each iteration (date updates, visibility predicates, etc.)
                Menu menu = it->second(*this);
                auto result = menu.run();

                // 'q' on any menu = go back one level
                if(result == MenuResult::Quit)
                    pop();
            }
        }

    private:
        std::unordered_map<std::string, MenuFactory> _registry;
        std::stack<std::string>                      _stack;
        bool                                         _running = false;
    };

} // namespace terminal_utils
