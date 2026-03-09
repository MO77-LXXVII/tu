// tu/menu_navigator.hpp
#pragma once

#include <string>
#include <stack>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include "menu.hpp"

namespace tu
{
     /**
     * @brief manages navigation between named menus using an internal stack
     *
     * abstracts stack-based navigation so callers don't need to manage a
     * navigation stack manually. menus are registered by name via `add()`,
     * then driven by `run()` which builds each menu fresh on every iteration.
     *
     * @tparam Key  type used to identify menus (default: `std::string`); can be
     *              an enum class for compile-time safety and typo prevention
     * @tparam Hash hash function for `Key` (default: `std::hash<Key>`)
     *
     * @code
     * // string keys (default)
     * tu::MenuNavigator<> nav;
     * nav.add("main", [&](tu::MenuNavigator<>& n)
     * {
     *     return tu::Menu::create("Bank System")
     *         .add_item("Accounts", [&]{ n.push("accounts"); })
     *         .add_item("Quit",     [&]{ n.exit(); });
     * });
     * nav.run("main");
     *
     * // enum keys
     * enum class MenuID { Main, Accounts };
     * tu::MenuNavigator<MenuID> nav;
     * nav.add(MenuID::Main, [&](tu::MenuNavigator<MenuID>& n)
     * {
     *     return tu::Menu::create("Bank System")
     *         .add_item("Accounts", [&]{ n.push(MenuID::Accounts); })
     *         .add_item("Quit",     [&]{ n.exit(); });
     * });
     * nav.run(MenuID::Main);
     * @endcode
     */
    template<typename Key = std::string, typename Hash = std::hash<Key>>
    class MenuNavigator
    {
        public:
            /** @brief factory function type: builds and returns a `Menu` given the navigator */
            using MenuFactory = std::function<Menu(MenuNavigator&)>;


            /**
             * @brief register a named menu with its factory function
             *
             * the factory is called every time the menu becomes active, ensuring
             * it reflects the latest runtime state (permissions, date, visibility, etc.)
             *
             * @param name    unique key identifying this menu
             * @param factory callable that builds and returns the `Menu`
             * @return        reference to this navigator for chaining
             */
            MenuNavigator& add(Key name, MenuFactory factory)
            {
                m_registry[std::move(name)] = std::move(factory);
                return *this;
            }


             /**
             * @brief push a registered `menu` onto the stack, making it the active menu
             *
             * the pushed `menu` becomes the active one on the next iteration of `run()`
             *
             * @param name the registered key of the menu to navigate to
             * @throws `std::runtime_error` if `name` has not been registered via `add()`
             */
            void push(const Key& name)
            {
                if(m_registry.find(name) == m_registry.end())
                    throw std::runtime_error("MenuNavigator: unknown menu");

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
             * pushes `start_menu` onto the stack and enters the navigation loop.
             * each iteration builds the current menu fresh via its factory and runs it.
             * if the user quits (`q`), the current menu is popped from the stack.
             * the loop exits when the stack is empty or `exit()` is called.
             *
             * @note menus are rebuilt on every iteration to reflect runtime state changes
             * such as date updates, permission changes, and visibility predicates.
             *
             * @param start_menu the registered key of the initial menu to display
             * @throws `std::runtime_error` if `start_menu` has not been registered via `add()`
             */
            void run(const Key& start_menu)
            {
                push(start_menu);
                m_running = true;

                while(m_running && !m_stack.empty())
                {
                    const Key& current = m_stack.top();

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
            std::unordered_map<Key, MenuFactory, Hash> m_registry;           ///< maps each registered menu name to its factory function; keyed by `Key`, hashed via `Hash`
            std::stack<Key>                            m_stack;              ///< navigation history, current `menu` on top
            bool                                       m_running = false;    ///< controls the `run()` loop
    };

} // namespace tu
