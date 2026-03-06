// linux.cpp

#include <string_view>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <csignal>

#include "platform.hpp"
#include "terminal_utils/config.hpp"

namespace terminal_utils::platform
{
    namespace
    {
        /*
            internal flag to simulate ANSI being disabled on linux
            Linux terminals always interpret ANSI escape sequences, and applications
            cannot disable that behavior at the terminal level
            true  = explicitly disabled via disable_ansi()
            false = not explicitly disabled; actual availability checked by is_ansi_enabled()
        */
        bool g_ansi_disabled = false;


        void install_POSIX_handler()
        {
            static bool installed = false;

            if (installed)
                return;

            installed = true;

            // \033[2J  : clear entire screen
            // \033[3J  : clear scrollback buffer
            // \033[H   : move cursor to home position (0,0)
            // \033[0m  : reset all text attributes
            // \033[?25h: show cursor
            static constexpr std::string_view RESTORE_SEQ = "\033[2J\033[3J\033[H\033[0m\033[?25h";

            // Signal handler for `SIGINT` and `SIGTERM`
            // - Always restores terminal state
            // - `write()` is async-signal-safe, so safe to call in signal context
            // - `_exit()` is used instead of `exit()`, because `exit()` is not async-signal-safe
            auto handler = +[](int sig)
            {
                [[maybe_unused]] auto _ = write(STDOUT_FILENO, RESTORE_SEQ.data(), RESTORE_SEQ.length());

                _exit(128 + sig); // Standard convention: 128 + signal number
            };

            // Install handler; if it fails, ignore (non-fatal)
            // Non-fatal: program continues normally, but Ctrl+C / termination may not restore terminal state
            if (std::signal(SIGINT, handler) == SIG_ERR || std::signal(SIGTERM, handler) == SIG_ERR)
                return; // Non-fatal: continue without handler
        }

        
        // cache env var lookups so `getenv()` isn't called on every `is_ansi_enabled()` call.
        // Both `NO_COLOR` and `TERM` are set at process start and never change at runtime.
        bool compute_ansi_env_supported()
        {        
            // Also check NO_COLOR env var (https://no-color.org/)
            // https://bixense.com/clicolors/
            if(getenv("NO_COLOR") != nullptr)
                return false;

            // Check TERM variable
            // getenv("TERM") fetches the environment variable TERM.

            // TERM tells programs what kind of terminal you are running in.
            // Example values: "xterm", "xterm-256color", "linux", "screen", "dumb".
            // term == nullptr --> the TERM variable is not set. If it's not set, we assume ANSI is not supported.
            const char* term = getenv("TERM");
            if(term == nullptr || strcmp(term, "dumb") == 0)
                return false;

            // ANSI is supported by default on Linux
            return true;
        }

        // evaluated once, result reused for the lifetime of the process
        const bool g_env_ansi_supported = compute_ansi_env_supported();
    }


    void install_handler()
    {
        install_POSIX_handler();
    }


    [[nodiscard]] bool is_terminal() noexcept
    {
        return isatty(STDOUT_FILENO) != 0;
    }


    // On Linux, we can't disable terminal's ANSI processing, but we can:
    // 1. Set a flag to track "disabled" state
    // 2. Send reset sequence to clear any active formatting
    // only applies when using `ColouredText`
    // using raw ansi colour codes will still work
    bool disable_ansi()
    {
        g_ansi_disabled = true;

        if(is_terminal())
        {
            // \033[0m  : reset all text attributes
            constexpr std::string_view RESET = "\033[0m";
            (void)write(STDOUT_FILENO, RESET.data(), RESET.length()); // Reset all terminal attributes
        }

        return true;
    }


    [[nodiscard]] bool enable_ansi()
    {
        if(!is_terminal())
            return false;

        // if user opted out don't enable ansi
        if(!config::ENABLE_COLOURS)
            return false;

        g_ansi_disabled = false;

        return g_env_ansi_supported;
    }


    [[nodiscard]] bool is_ansi_enabled()
    {
        if(!is_terminal())
            return false;

        if(g_ansi_disabled)  // Check flag first
            return false;

        // use cached result instead of calling `getenv()` every time
        return g_env_ansi_supported;
    }


    void clear_terminal()
    {
        if (!is_terminal())
            return;  // Don't try to clear if not a terminal

        // "\033[2J": Clear entire screen
        // "\033[H" : Move cursor to home position (0,0)
        // "\033[3J": Clear scrollback buffer
        constexpr std::string_view CLEAR_SEQ = "\033[2J\033[3J\033[H";
        (void)write(STDOUT_FILENO, CLEAR_SEQ.data(), CLEAR_SEQ.length());
    }


    bool restore_console_mode()
    {
        if(!is_terminal())
            return true;

        clear_terminal();

        return true;
    }


    bool init_terminal()
    {
        if(!is_terminal())
            return false;  // redirected: skip everything

        install_handler();

        static_cast<void>(enable_ansi());

        // Failure is non-fatal: app continues without coloured output
        return true;
    }


    void init_and_clear_terminal()
    {
        if(init_terminal())
            clear_terminal();
    }
} // namespace terminal_utils::platform
