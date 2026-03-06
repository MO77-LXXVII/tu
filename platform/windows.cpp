// windows.cpp

// Excludes legacy subsystems
#define WIN32_LEAN_AND_MEAN 

// no win `min` and `max` macros
#define NOMINMAX

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <csignal>
#include <string_view>
#include <atomic>

#include "platform.hpp"

namespace terminal_utils::platform
{
    namespace
    {
        // Ctrl+C handler runs on a separate thread.
        // We signal the main thread to stop writing to the console to avoid
        // interleaving output with the cleanup sequence.
        std::atomic_bool g_exiting = false;

        DWORD original_mode = 0;
        WORD original_attributes = 0; // this stores original colours

        /*
            Windows only:
            `ENABLE_VIRTUAL_TERMINAL_PROCESSING` is a real console mode flag that must be
            set manually via `SetConsoleMode()` and explicitly restored on exit.

            We cache the original mode here so `restore_console_mode()` can return the console
            to exactly the state it was in before the app ran.

            read: https://github.com/curl/curl/pull/6226#issuecomment-758133146 for more context
        */
        bool original_mode_cached = false;

    
        // check both `INVALID_HANDLE_VALUE` and `NULL`
        // `GetStdHandle()` can return either on failure
        HANDLE get_stdout_handle()
        {
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            return (h != INVALID_HANDLE_VALUE && h != NULL) ? h : nullptr;
        }

        // Safe wrapper around WriteConsoleA
        bool write_console(HANDLE h, std::string_view text)
        {
            // main thread's next call to write_console() stops immediately.
            if(!h || g_exiting.load())
                return false;

            DWORD written = 0;

            // string_view::data() returns const char*, but WriteConsoleA expects LPCVOID (which is const void*).
            // https://learn.microsoft.com/en-us/windows/console/writeconsole
            BOOL success = WriteConsoleA(
                h,
                static_cast<const void*>(text.data()),
                static_cast<DWORD>(text.length()),
                &written,
                nullptr
            );

            return success != 0;
        }

        BOOL WINAPI console_ctrl_handler(DWORD ctrl_type)
        {
            switch (ctrl_type)
            {
                case CTRL_C_EVENT:
                    [[fallthrough]];
                case CTRL_CLOSE_EVENT:
                case CTRL_BREAK_EVENT:
                {
                    g_exiting.store(true);  // stop main thread output
                    Sleep(10);         // let any in-progress write finish

                    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
                    if(h != INVALID_HANDLE_VALUE && h != NULL)
                    {
                        // \033[2J  : clear entire screen
                        // \033[3J  : clear scrollback buffer
                        // \033[H   : move cursor to home position (0,0)
                        // \033[0m  : reset all text attributes
                        // \033[?25h: show cursor
                        constexpr std::string_view CLEANUP = "\033[2J\033[3J\033[H\033[0m\033[?25h";
                        WriteConsoleA(h, CLEANUP.data(), static_cast<DWORD>(CLEANUP.size()), nullptr, nullptr);

                        if(original_mode_cached)
                        {
                            SetConsoleTextAttribute(h, original_attributes);
                            SetConsoleMode(h, original_mode);
                        }
                    }

                    // Terminate immediately without cleanup to prevent main thread from executing further
                    _Exit(1);
                }
                default:
                    return FALSE;
            }
        }

        void install_ctrl_handler()
        {
            static bool installed = false;
            if(installed)
                return;

            installed = true;

            SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
        }
    }

    void install_handler()
    {
        install_ctrl_handler();
    }
    

    [[nodiscard]] bool is_terminal() noexcept
    {
        return _isatty(_fileno(stdout)) != 0;
    }

    bool disable_ansi()
    {
        if(!is_terminal())
            return false;

        HANDLE h_out = get_stdout_handle();
        if(!h_out)
            return false;

        DWORD mode = 0;
        if(!GetConsoleMode(h_out, &mode))
            return false;

        mode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING; // clear the flag
        return SetConsoleMode(h_out, mode) != 0;
    }


    [[nodiscard]] bool enable_ansi()
    {
        if(!is_terminal())
            return false;

        // if user opted out don't enable ansi
        if(!config::ENABLE_COLOURS)
            return false;

        HANDLE h_out = get_stdout_handle();
        if(!h_out)
            return false;

        DWORD mode = 0;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if(!GetConsoleMode(h_out, &mode) || !GetConsoleScreenBufferInfo(h_out, &csbi))
            return false;

        if(!original_mode_cached)
        {
            original_mode        = mode;             // cache original console state
            original_attributes  = csbi.wAttributes; // Cache the color here!
            original_mode_cached = true;
        }

        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING; // enable VT100/ANSI sequence processing
        return SetConsoleMode(h_out, mode) != 0;
    }


    [[nodiscard]] bool is_ansi_enabled()
    {
        if(!is_terminal())
            return false;

        HANDLE h_out = get_stdout_handle();
        if(!h_out)
            return false;

        DWORD mode = 0;
        if(!GetConsoleMode(h_out, &mode))
            return false;

        return (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    }


    void clear_terminal()
    {
        if(!is_terminal())
            return;  // Don't try to clear if not a terminal

        HANDLE h_out = get_stdout_handle();
        if(!h_out)
            return;

        // "\033[2J":   Clear entire screen
        // "\033[3J":   Clear scrollback buffer (Windows 10+)
        // "\033[H" :   Move cursor to home position (0,0)
        // "\033[?25h": show cursor
        constexpr std::string_view CLEAR_SEQ = "\033[2J\033[3J\033[H\033[?25h";

        write_console(h_out, CLEAR_SEQ);
    }


    bool restore_console_mode()
    {
        // failure, but it's really a no-op / "nothing to do" case.
        if(!is_terminal() || !original_mode_cached)
            return true;

        HANDLE h_out = get_stdout_handle();
        if(!h_out)
            return false;

        // !
        // Reset attributes and show cursor before restoring console mode
        clear_terminal();

        bool success = SetConsoleMode(h_out, original_mode) != 0;

        // Reset the cache console state flag
        if(success)
        {
            SetConsoleTextAttribute(h_out, original_attributes);
            original_mode_cached = false;   // restored successfully, cache no longer needed
            // if failed: keep cache intact, retry possible if console recovers
        }
        /*
            If restoration fails, the console environment may be temporarily
            unavailable or changed (redirection, detach, handle invalidation, etc.).

            Failure does NOT imply the cached original mode is invalid.
            It remains the only known pre-modification state.

            Therefore:
            - On success → clear cache (state restored, no longer needed)
            - On failure → keep cache (restoration may succeed later)

            Clearing the cache on failure would discard the only reversible state
            and make recovery impossible if the console becomes usable again.
        */

        return success;
    }


    bool init_terminal()
    {
        if(!is_terminal())
            return false;  // redirected: skip everything

        install_handler();

        // Enable ANSI support (good to enforce on Windows)
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
