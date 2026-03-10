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
#include "../tu/config.hpp"
#include "file_cache.hpp"  

static constexpr std::string_view SEPARATOR = "#//#";

// ============================================================
//    CRTP Base: all shared persistence logic lives here
// ============================================================

namespace bank
{
    template<typename Derived>
    class PersistentEntity
    {
        public:
            /** @brief controls the behaviour of `save()` */
            enum class Mode
            {
                empty_mode,     ///< no-op, `save()` does nothing
                add_mode,       ///< appends this record to the file
                update_mode,    ///< overwrites the matching record in the file
                delete_mode     ///< removes the matching record from the file
            };


            /** @brief return type for `save_with_result()`; indicates success or the exact failure reason */
            enum class SaveResult
            {
                succeeded,                ///< record was saved successfully
                failed_empty_object,      ///< attempted to save an empty record
                failed_invalid_fields,    ///< attempted to save an empty record
                failed_key_exists,        ///< key already exists (`add_mode` only)
                failed_io                 ///< underlying I/O failure
            };

            // =========================
            //       Public API 
            // =========================

            /**
             * @brief returns true if the string contains the field separator
             * @note any field containing "#//#" would corrupt the file on encode/decode
             */
            inline static bool contains_separator(std::string_view str)
            {
                return str.find(SEPARATOR) != std::string_view::npos;
            }


            /**
            * @brief returns true if any of the given fields contain the separator sequence
            * @note pass all encoded string fields to guard against file corruption before saving
            */
            template<typename... Fields>
            [[nodiscard]] static bool any_field_corrupt(const Fields&... fields) noexcept
            {
                return (contains_separator(fields) || ...);
            }


            /**
             * @brief load all records from the entity's file
             * @return `const` reference to the cached records, empty vector if the file cannot be opened
             * @note calls `Derived::file_name()` and `Derived::decode()` internally
             * 
             * @warning the returned reference is only valid until the next mutating operation (`add()`, `update()`, `delete()`)
             * do not store this reference; iterate or copy immediately
             */
            [[nodiscard]] static const std::vector<Derived>& load_all()
            {
                // Uses cached data now
                return FileCache<Derived>::load();
            }


            /**
             * @brief  find a single record by `key`
             * @param  key the unique identifier to search for
             * @return the matching record, or `std::nullopt` if not found
             * @note   calls `Derived::matches_key()` to compare
             */
            [[nodiscard]] static std::optional<Derived> find(const std::string& key)
            {
                for(const auto& record : load_all())
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
             * @brief wraps `save()` and returns a `SaveResult` instead of a plain `bool`
             * @return `SaveResult` indicating success or the exact failure reason
             * @see SaveResult
             */
            SaveResult save_with_result()
            {
                if(is_empty())
                    return SaveResult::failed_empty_object;

                if(self().has_corrupt_fields())
                    return SaveResult::failed_invalid_fields;

                if(_mode == Mode::add_mode && Derived::exists(self().key()))
                    return SaveResult::failed_key_exists;

                if(!save())
                    return SaveResult::failed_io;

                return SaveResult::succeeded;
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
                    case Mode::add_mode:    return m_add();
                    case Mode::update_mode: return m_update();
                    case Mode::delete_mode: return m_remove();
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
             * @return  `true` on success, `false` if the file cannot be opened or a write fails
             * @note    non-const: Derived::sort() mutates records before writing
             */
            static bool m_write_all(std::vector<Derived>& records)
            {
                Derived::sort(records);

                // cache will be invalid after editing the file
                FileCache<Derived>::invalidate();

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
            bool m_add()
            {
                auto records = load_all();

                records.push_back(self());

                return m_write_all(records);
            }


            /** @brief overwrite the matching record in the file with this object's current state */
            bool m_update()
            {
                auto records = load_all();

                for (Derived& r : records)
                    if(r.matches_key(self().key()))
                    {
                        r        = self();
                        r._mode  = Mode::update_mode;
                        break;
                    }

                return m_write_all(records);
            }


            /** @brief remove the matching record from the file */
            bool m_remove()
            {
                auto records = load_all();

                records.erase(
                    std::remove_if(records.begin(), records.end(), [&](const Derived& r)
                    {
                        return r.matches_key(self().key());
                    }),
                    records.end()
                );

                return m_write_all(records);
            }


        protected:
            /**
             * @brief atomically updates multiple records in a single write
             * intended for operations that must persist changes to **more than one record in one operation**
             */
            static bool save_all(std::vector<Derived>& records)
            {
                return m_write_all(records);
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
    //   bool has_corrupt_fields() const noexcept    → separator guard
    // ============================================================
} // namespace bank
