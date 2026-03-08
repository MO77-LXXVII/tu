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
//    CRTP Base: all shared persistence logic lives here
// ============================================================

template<typename Derived>
class PersistentEntity
{
    public:
        /**
         * @brief controls the behaviour of `save()`
         * 
         * - `empty_mode`:  no-op, `save()` does nothing
         * 
         * - `add_mode`:    appends this record to the file
         * 
         * - `update_mode`: overwrites the matching record in the file
         * 
         * - `delete_mode`: removes the matching record from the file
         */
        enum class Mode { empty_mode, add_mode, update_mode, delete_mode };


        // =========================
        //       Public API 
        // =========================


        /**
         * @brief load all records from the entity's file
         * @return vector of all decoded records, empty if the file cannot be opened
         * @note calls `Derived::file_name()` and `Derived::decode()` internally
         */
        [[nodiscard]] static std::vector<Derived> load_all()
        {
            std::vector<Derived> records;

            std::ifstream file(Derived::file_name().data());
            if(!file.is_open())
                return records;

            std::string line;
            while(std::getline(file, line))
                if(!line.empty())
                    records.push_back(Derived::decode(line));

            return records;
        }


        /**
         * @brief  find a single record by `key`
         * @param  key the unique identifier to search for
         * @return the matching record, or `std::nullopt` if not found
         * @note   calls `Derived::matches_key()` to compare
         */
        [[nodiscard]] static std::optional<Derived> find(const std::string& key)
        {
            for(auto& record : load_all())
                if(record.matches_key(key))
                    return record;

            return std::nullopt;
        }


        /**
         * @brief check if a record with the given key exists
         * @param key the unique identifier to search for
         * @return `true` if found, `false` otherwise
         */
        [[nodiscard]] static bool exists(const std::string& key)
        {
            return find(key).has_value();
        }


        /**
         * @brief persist this record according to its current `Mode`
         * 
         * - `add_mode`    → appends this record
         * - `update_mode` → overwrites the matching record
         * - `delete_mode` → removes the matching record
         * - `empty_mode`  → no-op
         * 
         * @return `true` on success, `false` on I/O failure
         * @see set_mode()
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
         * @brief set the persistence mode of this record
         * @param mode the `Mode` to apply on the next `save()` call
         * @return reference to the derived object (allows chaining)
         * @note usage: `entity.set_mode(Mode::update_mode).save()`
         */
        Derived& set_mode(Mode mode)
        {
            _mode = mode;
            return self();
        }


        /** @brief returns the current persistence `Mode` of this record */
        [[nodiscard]] Mode mode() const noexcept { return _mode; }


        /** @brief returns `true` if this record has no pending persistence operation */
        [[nodiscard]] bool is_empty() const noexcept
        {
            return _mode == Mode::empty_mode;
        }


    protected:
        Mode _mode = Mode::empty_mode; ///< current persistence mode, determines behaviour of `save()`


        /**
         * @brief protected constructor derived classes set the initial mode
         * @param mode initial `Mode`, defaults to `empty_mode`
         */
        explicit PersistentEntity(Mode mode = Mode::empty_mode): _mode(mode) {}


    private:
        // =========================
        //     Internal helpers
        // =========================


        /** @brief downcast `this` to the derived type for CRTP dispatch */
        Derived& self()
        {
            return static_cast<Derived&>(*this);
        }


        /** @brief const overload of `self()` for use in const member functions */
        const Derived& self() const
        {
            return static_cast<const Derived&>(*this);
        }


        /**
         * @brief   sort and write all records to the entity's file, overwriting existing content
         * @param   records the records to write
         * @return `true` on success, `false` if the file cannot be opened or a write fails
         */
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


        /** @brief append this record to the file */
        bool _add()
        {
            std::vector<Derived> records = load_all();

            records.push_back(self());

            return _write_all(records);
        }


        /** @brief overwrite the matching record in the file with this object's current state */
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


        /** @brief remove the matching record from the file */
        bool _remove()
        {
            auto records = load_all();

            records.erase(
                std::remove_if(records.begin(), records.end(), [&](const Derived& r)
                {
                    return r.matches_key(self().key());
                }),
                records.end()
            );

            return _write_all(records);
        }
};

// ============================================================
// What each Derived class MUST implement:
//
//   static std::string_view file_name()         → file path
//   static Derived decode(const std::string&)   → parse line        → object
//   std::string encode() const                  → convert object    → line
//   std::string key() const                     → unique identifier
//   bool matches_key(const std::string&) const  → comparison
//   static void sort(std::vector<Derived>&)     → custom sort before write
// ============================================================
