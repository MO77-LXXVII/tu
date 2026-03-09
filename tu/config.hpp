// config.hpp

#pragma once

#include <string_view>
#include <cstdint>

// Compile-time configurations
namespace tu::config
{
    // Logging
    /** @brief Log output file path (relative to working directory) */
    inline constexpr std::string_view LOG_FILE = "logs/bank_app.log";


    // Control terminal coloring
    /** @brief Enable ANSI colour codes in terminal output */
    inline constexpr bool ENABLE_COLORS = true;


    // Data files (relative to the demo)
    /** @brief Path to the clients data file */
    inline constexpr std::string_view CLIENTS_FILE_NAME           = "bank/data/clients.txt";

    /** @brief Path to the currency exchange rates file */
    inline constexpr std::string_view CURRENCY_EXCHANGE_FILE_NAME = "bank/data/currencies.txt";

    /** @brief Path to the users file */
    inline constexpr std::string_view USERS_FILE_NAME             = "bank/data/users.txt";


    // Business rules
    /**
     * @brief Maximum allowed balance per client in dollars
     * @note Deposits that would exceed this limit are rejected
     */
    inline constexpr double MAXIMUM_ALLOWED_BALANCE_PER_CLIENT = 1000000.0;


    // UI
    /** @brief Default width of menu boxes in characters */
    inline constexpr int DEFAULT_MENU_WIDTH = 40;

    /** @brief Default horizontal padding inside menus */
    inline constexpr int DEFAULT_PADDING    = 4;


    // Security
    /**
     * @brief Caesar cipher shift value for basic encryption
     * @note for demonstration purposes only
     */
    inline constexpr uint8_t CIPHER_SHIFT = 2;

    // Ensure balance limit is a meaningful positive value
    static_assert(MAXIMUM_ALLOWED_BALANCE_PER_CLIENT > 0);

    // Ensure cipher shift is non-zero (no encryption) and within uint8_t range
    static_assert(CIPHER_SHIFT > 0 && CIPHER_SHIFT < 256);
} // namespace tu::config
