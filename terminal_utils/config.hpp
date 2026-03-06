// config.hpp

#pragma once

namespace terminal_utils::config
{
    // Compile-time configurations
    inline constexpr bool ENABLE_COLOURS = true;

    inline constexpr std::string_view PROGRAM_NAME = "bank_app.log";

    inline constexpr const std::string_view CLIENTS_FILE_NAME           = "bank/data/clients.txt";
    inline constexpr const std::string_view CURRENCY_EXCHANGE_FILE_NAME = "bank/data/currencies.txt";
    inline constexpr const std::string_view USERS_FILE_NAME             = "bank/data/users.txt";
    inline constexpr double MAXIMUM_ALLOWED_BALANCE_PER_CLIENT = 1000000.0;
    
    inline constexpr int DEFAULT_MENU_WIDTH = 40;
    inline constexpr int DEFAULT_PADDING = 4;
    
    inline constexpr double ENCRYPTION_KEY = 2;

} // namespace terminal_utils::config
