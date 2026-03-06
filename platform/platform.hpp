// platform.hpp

#pragma once

#include <stdexcept>
#include <cassert>

#include "../terminal_utils/logger.hpp"

namespace terminal_utils::platform
{

    /*
        Cross-platform ANSI escape sequence support

        Windows (Win 10/11):
        - ANSI support via `ENABLE_VIRTUAL_TERMINAL_PROCESSING` flag
        - Can be enabled/disabled by changing console mode
        - `disable_ansi()` actually prevents terminal from processing ANSI codes
        - `is_ansi_enabled()` checks the actual console mode flag

        Linux/Unix:
        - ANSI support is built into terminal emulators
        - Cannot disable terminal's ANSI processing at application level
        - `disable_ansi()` sets internal flag + sends reset sequence
        - `is_ansi_enabled()` checks internal flag + environment variables

        Testing:
        - `disable_ansi()` is useful on Windows to test if code gracefully
            handles ANSI being disabled (output falls back to plain text)
        - On Linux, it simulates the same behaviour using an internal flag
        - Your code should always check `is_ansi_enabled()` before outputting
            ANSI codes on both platforms
    */


    /*
        Platform behaviour:
        - Output stays in the main terminal buffer on both platforms

        Initialization:
        - enable_ansi() enables ANSI processing on both platforms

        Cleanup:
        - restore_console_mode() resets attributes (\033[0m) and shows the cursor
        - restore_console_mode() additionally restores the original console mode on Windows

        Result: No colour bleed into shell prompts on either platform
    */

    /**
     *  @brief Install platform-specific Ctrl-C / termination handler
     *
     *  Windows:
     *  - Calls `SetConsoleCtrlHandler()` to register a handler that resets
     *    text attributes, shows the cursor and terminates via `_Exit()` on Ctrl-C or console close.
     *
     *   Linux:
     *  - Installs `SIGINT` and `SIGTERM` handlers that reset text attributes
     *    and show the cursor before terminating via `_exit()`.
     *
     *  Note:
     *  - The handler is installed only once per process; subsequent calls are no-ops.
     *  - Call `install_handler()` once at program startup to register it.
     *  - You never call the handler directly.
    */
    void install_handler();


    /**
     * @brief Queries the underlying system to determine if `stdout` is a TTY.
     * @return true if the output stream is a terminal; false if redirected to a file/pipe.
     */
    [[nodiscard]] bool is_terminal() noexcept;


    /**
     * @brief Disable ANSI escape sequence output
     * Windows: Disables `ENABLE_VIRTUAL_TERMINAL_PROCESSING` flag
     * Linux: Sets internal flag, sends reset sequence (\033[0m)
    */
    bool disable_ansi();


    /**
     * @brief Enable ANSI escape sequence processing
     * Windows: Enables `ENABLE_VIRTUAL_TERMINAL_PROCESSING` flag, caches original console mode
     * Linux: Clears `g_ansi_disabled` flag, validates environment (`NO_COLOR`, `TERM`)
     *
     */
    [[nodiscard]] bool enable_ansi();


    /**
     * @brief Check if ANSI codes should be output
     * Windows: Queries console mode for `ENABLE_VIRTUAL_TERMINAL_PROCESSING`
     * Linux: Checks internal flag + `NO_COLOR` env var + `TERM` env var
     */
    [[nodiscard]] bool is_ansi_enabled();


    /**
     * @brief Clear the terminal screen and move cursor to top-left
     * Sends: `\033[2J` (clear screen), `\033[3J` (clear scrollback), `\033[H` (home cursor)
     */
    void clear_terminal();


    /**
     * @brief Performs a full reset of the terminal state and formatting.
     * Restores the original console mode (Windows).
     * Issues a SGR reset sequence (\033[0m) and shows the cursor (\033[?25h)
     * to prevent attribute leakage (color bleed) into the parent shell.
     */
    bool restore_console_mode();


    /**
     * @brief Configures the terminal environment for application execution.
     * On Windows: Enables `ENABLE_VIRTUAL_TERMINAL_PROCESSING` for ANSI sequence support.
     * On Linux: Enables ANSI if supported by the environment.
     * @return `true` if the environment was successfully initialized.
     */
    [[nodiscard]] bool init_terminal();


    /**
     * @brief Initialize terminal and clear the screen
     * 1. Checks if output is a terminal (not redirected)
     * 2. Enables ANSI processing via enable_ansi()
     * 3. Clears the screen
     */
    void init_and_clear_terminal();


    /**
     * @brief RAII guard for terminal ANSI support.
     * 
     * Initializes terminal on construction, restores on destruction.
     * 
     * @warning Only ONE AnsiGuard should exist at a time.
     *          Creating nested guards will throw `std::logic_error`.
     * 
     * @throws `std::logic_error`
     *         If another AnsiGuard instance already exists.
     */
    class AnsiGuard
    {
        public:
            AnsiGuard()
            {
                if(is_active())
                {
                    // ! invariant: only one AnsiGuard may exist at a time
                    // violation indicates a programmer error, so it logs and fail fast
                    LOG_EXCEPTION("a second AnsiGuard was initialized during runtime");
                    throw std::logic_error("AnsiGuard already active");
                }

                if(!init_terminal())  // false = output is redirected, not a terminal; degrade gracefully
                    LOG_WARNING("AnsiGuard: ANSI unavailable, falling back to plain output");

                m_active = true;
            }

            ~AnsiGuard() noexcept
            {
                restore_console_mode();
                m_active = false; // reset to reuse later if needed
            }

            // Deleted copy/move only ONE can exist
            AnsiGuard(const AnsiGuard&)            = delete;
            AnsiGuard(AnsiGuard&&)                 = delete;
            AnsiGuard& operator=(const AnsiGuard&) = delete;
            AnsiGuard& operator=(AnsiGuard&&)      = delete;

            [[nodiscard]] static bool is_active() noexcept
            {
                return m_active;
            }

        private:
            inline static bool m_active = false;
    };
} // namespace terminal_utils::platform
