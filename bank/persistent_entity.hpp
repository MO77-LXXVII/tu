// persistent_entity.hpp

/*
    ===========================================================
        Curiously Recurring Template Pattern (CRTP)
            PersistentEntity<Derived>
    ===========================================================

    ?MOTIVATION:
    BankClient, BankUser, and CurrencyExchange share identical logic:
      - File I/O (reading and writing)
      - Line parsing and encoding
      - save() with mode switching
      - load_all(), find(), exists(), remove()

    An alternative approach using a virtual Interface would incur runtime
    overhead; CRTP avoids this entirely by resolving all dispatch at compile time.

    ?DESIGN:
    PersistentEntity<Derived> extracts all shared logic into a single
    base class template.
    
    each derived class implements only the parts
    specific to its type:
      - encode()       -> serialize this object to a line
      - decode()       -> parses a line into this object
      - file_name()    -> which file to read/write
      - key()          -> unique identifier (e.g. account number)
      - matches_key()  -> how to compare keys

    the base handles everything else automatically.

    ?CRTP MECHANICS:
    template<typename Derived>
    class PersistentEntity
    {
        // Base accesses derived members without virtual dispatch
        Derived& self() { return static_cast<Derived&>(*this); }
    };

    class BankClient   : public PersistentEntity<BankClient>   {};
    class BankUser     : public PersistentEntity<BankUser>     {};
    class CurrencyRate : public PersistentEntity<CurrencyRate> {};

    All dispatch is resolved at compile time
    no virtual functions, no runtime overhead.
*/

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <optional>
#include <algorithm>

#include "../utils/string_utils.hpp"
#include "../utils/utils.hpp"
#include "../terminal_utils/config.hpp"

static constexpr std::string_view SEPARATOR = "#//#";

// ============================================================
// CRTP Base — all shared persistence logic lives here
// ============================================================

template<typename Derived>
class PersistentEntity
{
    public:

        // ----------------------------------------------------------
        // Mode — controls what save() does
        // ----------------------------------------------------------
        enum class Mode { empty_mode, add_mode, update_mode, delete_mode };

        // ----------------------------------------------------------
        // Public API — same for ALL entities, written once
        // ----------------------------------------------------------

        /**
         * @brief Load all records from the entity's file.
         * Calls Derived::file_name() and Derived::decode() internally.
         */
        [[nodiscard]] static std::vector<Derived> load_all()
        {
            std::vector<Derived> records;

            std::ifstream file(Derived::file_name().data());
            if (!file.is_open())
                return records;

            std::string line;
            while (std::getline(file, line))
                if (!line.empty())
                    records.push_back(Derived::decode(line));

            return records;
        }

        /**
         * @brief Find a single record by key.
         * Calls Derived::matches_key() to compare.
         * @return std::nullopt if not found.
         */
        [[nodiscard]] static std::optional<Derived> find(const std::string& key)
        {
            for(auto& record : load_all())
                if(record.matches_key(key))
                    return record;

            return std::nullopt;
        }

        /**
         * @brief Check if a record with the given key exists.
         */
        [[nodiscard]] static bool exists(const std::string& key)
        {
            return find(key).has_value();
        }

        /**
         * @brief Save, update, or delete based on current Mode.
         * No parameters needed — mode is stored in the object.
         * @return true on success, false on I/O failure
         */
        bool save()
        {
            switch (_mode)
            {
                case Mode::add_mode:    return _add();
                case Mode::update_mode: return _update();
                case Mode::delete_mode: return _remove();
                case Mode::empty_mode:  return true;
            }
            return false;
        }

        /**
         * @brief Set the mode of this record.
         * Allows: entity.set_mode(Mode::update_mode).save();
         */
        Derived& set_mode(Mode mode)
        {
            _mode = mode;
            return self();
        }

        [[nodiscard]] Mode mode() const noexcept { return _mode; }

        [[nodiscard]] bool is_empty() const noexcept
        {
            return _mode == Mode::empty_mode;
        }

    protected:
        Mode _mode = Mode::empty_mode;

        // Protected constructor so derived classes set the mode
        explicit PersistentEntity(Mode mode = Mode::empty_mode) : _mode(mode) {}

    private:

        // ----------------------------------------------------------
        // Internal helpers — call Derived's encode/decode/sort
        // ----------------------------------------------------------

        Derived& self()
        {
            return static_cast<Derived&>(*this);
        }

        const Derived& self() const
        {
            return static_cast<const Derived&>(*this);
        }

        static bool _write_all(std::vector<Derived>& records)
        {
            Derived::sort(records);

            std::ofstream file(Derived::file_name().data(), std::ios::trunc);

            if(!file.is_open())
                return false;

            for(const Derived& r : records)
            {
                file << r.encode() << "\n";

                if(!file.good())
                    return false;
            }
            return true;
        }

        bool _add()
        {
            std::vector<Derived> records = load_all();

            records.push_back(self());

            return _write_all(records);
        }

        bool _update()
        {
            auto records = load_all();

            for (Derived& r : records)
                if(r.matches_key(self().key()))
                {
                    r        = self();
                    r._mode  = Mode::update_mode;
                    break;
                }

            return _write_all(records);
        }

        bool _remove()
        {
            auto records = load_all();

            records.erase(
                std::remove_if(records.begin(), records.end(),
                    [&](const Derived& r) { return r.matches_key(self().key()); }),
                records.end()
            );

            return _write_all(records);
        }
};

// ============================================================
// What each Derived class MUST implement:
//
//   static std::string_view file_name()         → file path
//   static Derived decode(const std::string&)   → parse line → object
//   std::string encode() const                  → object → line
//   std::string key() const                     → unique identifier
//   bool matches_key(const std::string&) const  → comparison
//   static void sort(std::vector<Derived>&)     → custom sort before write
// ============================================================
