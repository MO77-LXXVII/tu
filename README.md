# tu: Terminal Utilities

A C++ library for building interactive terminal applications.
`tu` provides a cross-platform ANSI abstraction layer, a fluent keyboard-driven menu system, typed input validation, aligned table rendering, and a structured logger: everything needed to build polished command-line interfaces without depending on external UI frameworks.

A fully functional bank management system is included as a demonstration of the library's capabilities.

---
## Table of Contents

- [Library Overview](#library-overview)
- [Components](#components)
  - [Platform](#platform-tuplatform)
  - [Menu System](#menu-system-tumenu-tumenunavigator)
  - [Input](#input-tuinput)
  - [Output](#output-tuoutput)
  - [Colors and Styles](#colors-and-styles-tucoloredtext)
  - [Logger](#logger-tulogger)
  - [Date](#date-utilsdate)
  - [String Utilities](#string-utilities-stutlstring)
- [Demo Application: Bank Management System](#demo-application-bank-management-system)
  - [Domain Features](#domain-features)
  - [Persistence](#persistence)
- [Project Structure](#project-structure)
- [Building](#building)
- [Configuration](#configuration)

---
## Library Overview

`tu` is organized into self-contained components under the `tu::` namespace. Each component can be used independently, though they compose naturally.

The platform layer is separated into `platform/linux.cpp` and `platform/windows.cpp`, compiled conditionally, so all ANSI and terminal state logic is handled transparently regardless of the target OS.

### Namespace Map

|Namespace|Purpose|
|---|---|
|`tu::`|Top-level: colors, styles, and shared types|
|`tu::platform`|Cross-platform terminal initialization, ANSI enable/disable, signal handling|
|`tu::input`|Typed and validated console input with RAII stream guards|
|`tu::output`|Aligned text printing, bordered table rendering, stream format guards|
|`tu::config`|Compile-time configuration constants, set via CMake|
|`utils::`|General-purpose utilities: `Date`, basic encryption|
|`stutl::`|String manipulation with static and fluent instance methods|

---
## Components

### Platform (`tu::platform`)

Abstracts terminal state across Windows (Win 10/11) and Linux/Unix. On Windows, ANSI processing is controlled via the `ENABLE_VIRTUAL_TERMINAL_PROCESSING` console mode flag and the original mode is cached and restored on exit.
On Linux, an internal flag simulates the disabled state since terminal emulators cannot have ANSI processing disabled at the application level.

#### Key facilities:
- `AnsiGuard`: RAII singleton that initializes the terminal on construction and restores it on destruction. Throws `std::logic_error` if a second instance is created.
- `install_handler()`: registers `SIGINT`/`SIGTERM` handlers on Linux and `SetConsoleCtrlHandler` on Windows to guarantee terminal cleanup on abnormal exit.
- `is_terminal()`: detects whether `stdout` is a TTY or is redirected, allowing graceful fallback to plain text output.
- `enable_ansi()` / `disable_ansi()` / `is_ansi_enabled()`: runtime ANSI state management, respecting the `NO_COLOR` and `TERM` environment variables on Linux.

### Menu System (`tu::Menu`, `tu::MenuNavigator`)

Menus are constructed through a fluent builder and navigated entirely from the keyboard (W/S or J/K to move, Enter to select, Q to go back). Each menu renders with a configurable title, optional date display, persistent global subtitles (such as a logged-in username), and a configurable highlight color.

`MenuItem` supports dynamic visibility via an optional callback, so items can be shown or hidden at runtime based on any condition: permissions, state, or user type: without rebuilding the menu structure.

`MenuNavigator<Key>` provides stack-based navigation between named menus. It accepts either `std::string` or a custom `enum class` as the key type. Menus registered with `add()` are rebuilt fresh from a factory function on every iteration, ensuring that visibility predicates and runtime state are always current.

```cpp
tu::MenuNavigator<> nav;
nav.add("main", [&](tu::MenuNavigator<>& n)
{
    return tu::Menu::create("Main Menu")
        .set_width(40)
        .add_item("Go somewhere", [&]{ n.push("other"); })
        .add_item("Quit",         [&]{ n.exit(); });
});
av.run("main");
```

### Input (`tu::input`)

Typed input functions that validate, reprompt on failure, and clean up stream state via `InputGuard`: a RAII wrapper that restores `std::cin` and discards any remaining line content on destruction.

Available primitives include `get_number<T>()`, `get_number_in_range<T>()`, `get_string()`, `get_yes_no()`, and `get_menu_key()`. All functions accept a prompt string and an error message and loop until valid input is received with `get_string()` and `get_line()` allowing users to abort the operation when pressing *Enter*.

### Output (`tu::output`)

The output component provides two main tools.

`print_aligned()` is a factory function that returns an `Aligned<T>` descriptor, printed via `operator<<` with left, right, or center alignment and a configurable fill character. A template specialization handles `ColoredText` correctly by measuring the raw text length rather than the ANSI-encoded length, so padding is always accurate.

`Table` stores rows as `std::vector<std::string_view>` and computes column widths automatically from cell contents. Rows can be added as views (caller manages lifetime) or with `Table::own` to transfer string ownership into the table's internal storage. Printing supports left, right, or center alignment and configurable cell padding.

`FormatGuard` saves and restores `std::ostream` format flags and the fill character via RAII, preventing formatting state from leaking across output calls.

### Colors and Styles (`tu::ColoredText`)

`ColoredText` carries a text string, a `Color`, and a `Style` bitmask. It is written to a stream via `operator<<`, which applies ANSI escape sequences only when the output stream is `std::cout` or `std::cerr`, ANSI is enabled, and `config::ENABLE_COLORS` is `true`. All formatting is reset after each write to prevent bleed into subsequent output.

`Style` is a bitmask enum supporting bold, dim, italic, underline, blink, reverse, hidden, and strikethrough. Styles compose with bitwise OR and can be combined with any color.
Convenience wrappers (`tu::red()`, `tu::bold()`, `tu::underline()`, etc.) allow concise inline styling at call sites.

### Logger (`tu::logger`)

A structured logger that writes timestamped entries to a log file configured at compile time. Macros: `LOG_INFO`, `LOG_WARNING`, `LOG_EXCEPTION`: capture the file, line, and function name automatically. The logger is used internally throughout the library and is available to application code.

### Date (`utils::Date`)

A full-featured calendar date class with value semantics. Supports construction from year/month/day integers or from "dd/mm/yyyy" and "yyyy-mm-dd" strings. Provides day-of-week calculation via Zeller's congruence, leap year detection, ordinal date conversion, business day arithmetic, and flexible formatting via a pattern string.

### String Utilities (`stutl::String`)

A string manipulation class offering both static methods (pure functions on `std::string_view`) and instance methods that mutate an internal string and return `*this` for method chaining. Operations include trimming, splitting, case conversion, word counting, word reversal, and more.

---
## Demo Application: Bank Management System

The included bank application demonstrates `tu` in a realistic, multi-screen terminal program. It is a direct port and extension of an earlier standalone project, rebuilt on top of the `tu` library.

It exercises nearly every library component: `AnsiGuard` for terminal lifecycle management, `MenuNavigator` for multi-level navigation, dynamic `MenuItem` visibility driven by a permission bitmask, `Table` for displaying records, `input` primitives for validated data entry, and `ColoredText` for status feedback.

### Domain Features

The application supports full CRUD operations on both clients and bank system users, role-based access control via a `uint32_t` bitmask permission system covering 13 individual operations, deposits, withdrawals, and fund transfers with confirmation flows, an append-only transaction audit log, and a currency exchange converter using a USD pivot rate.

### Persistence

All data is stored in plain text files using `#//#` as a field delimiter. The `PersistentEntity<Derived>` [CRTP base class](https://en.cppreference.com/w/cpp/language/crtp.html) provides `load_all()`, `find()`, `exists()`, and `save()` for all entity types without virtual dispatch.

A companion `FileCache<Derived>` template maintains one lazily-populated `std::optional<std::vector<Derived>>` per entity type, invalidated automatically after every write.

|Entity|File|Key Field|
|---|---|---|
|`BankUser`|`bank/data/users.txt`|username|
|`BankClient`|`bank/data/clients.txt`|account number|
|`CurrencyExchange`|`bank/data/currencies.txt`|currency code|
|`TransactionLog`|`bank/data/transactions.txt`|timestamp + source account|

Passwords are encrypted with a [Caesar cipher](https://en.wikipedia.org/wiki/Caesar_cipher) (shift configured at compile time) before being written to disk.

---
## Project Structure

```
.
├── bank/                          # Bank library
│   ├── data/
│   │   ├── clients.txt
│   │   ├── users.txt
│   │   ├── currencies.txt
│   │   └── transactions.txt
│   ├── entities/
│   │   ├── person.hpp
│   │   ├── bank_client.hpp
│   │   └── bank_user.hpp
│   ├── persistence/
│   │   ├── file_cache.hpp
│   │   └── persistent_entity.hpp
│   └── services/
│       └── currency_exchange.hpp
├── demo/                          # Demo application
│   ├── bank_demo_raw.hpp
│   └── bank_demo.hpp
├── platform/                      # tu: platform abstraction
│   ├── platform.hpp
│   ├── linux.cpp
│   └── windows.cpp
├── tu/                            # tu: core library
│   ├── config.hpp
│   ├── input.hpp
│   ├── output.hpp
│   ├── logger.hpp
│   ├── ansi_colors.hpp
│   ├── style_wrappers.hpp
│   ├── tu.hpp
│   └── menu/
│       ├── menu.hpp
│       ├── menu_item.hpp
│       └── menu_navigator.hpp
├── utils/                         # General utilities
│   ├── date.hpp
│   ├── string_utils.hpp
│   └── utils.hpp
├── logs/
│   └── bank_app.log
├── CMakeLists.txt
└── main.cpp
```

---
## Building

The project uses CMake and requires a C++17-capable compiler.

```bash
git clone <repository-url>
cd <repository-directory>
cmake -S . -B build
cmake --build build
./build/<executable-name>
```

On Windows, use the CMake build command with the appropriate generator or open the generated solution in Visual Studio.

---
## Configuration

All compile-time settings are defined in `tu/config.hpp` and populated by CMake at configure time.

|Constant|Type|Default|Description|
|---|---|---|---|
|`LOG_FILE`|`std::string`|`logs/bank_app.log`|Runtime log file path|
|`USERS_FILE_NAME`|`std::string`|`bank/data/users.txt`|Users data file|
|`CLIENTS_FILE_NAME`|`std::string`|`bank/data/clients.txt`|Clients data file|
|`CURRENCY_EXCHANGE_FILE_NAME`|`std::string`|`bank/data/currencies.txt`|Currency rates file|
|`TRANSACTIONS_FILE_NAME`|`std::string`|`bank/data/transactions.txt`|Transaction log file|
|`ENABLE_COLORS`|`bool`|`true`|Enable ANSI color output|
|`MAXIMUM_ALLOWED_BALANCE_PER_CLIENT`|`double`|`1,000,000.0`|Per-client deposit cap (USD)|
|`DEFAULT_MENU_WIDTH`|`int`|`40`|Menu box width in characters|
|`DEFAULT_PADDING`|`int`|`4`|Horizontal padding inside menus|
|`CIPHER_SHIFT`|`uint8_t`|`2`|Caesar cipher shift for password encryption|

Static assertions in the header guard against invalid values at compile time.