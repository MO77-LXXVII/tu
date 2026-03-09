// file_cache.hpp

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <string_view>

#include "../tu/logger.hpp"


/**
 * @brief per-type lazy in-memory cache for flat-file persistence
 *
 * caches the contents of a single file in a `static std::vector`, populated
 * on the first `load()` call and **invalidated** *after any write operation*
 *
 * one cache instance exists per `Derived` type due to `inline static` storage
 * in a class template
 * 
 * `BankClient`, `BankUser`, and `CurrencyExchange` each maintain an independent cache slot
 *
 * @tparam Derived the entity type being cached; must implement:
 *   - `static std::string_view file_name()`
 *   - `static Derived decode(const std::string&)`
 */
template<typename Derived>
class FileCache
{
    public:

        /**
         * @brief returns a `const` reference to the cached records
         *
         * on a *cold cache* (first call or after `invalidate()`), reads the file
         * from disk and stores the result
         * 
         * subsequent calls return the cached data without any I/O
         *
         * @return `const` reference to the cached vector; empty if the file
         *         cannot be opened
         *
         * @warning the returned reference is only valid until the next call to
         *          `invalidate()`; iterate over it or copy immediately; **not safe store it**
         */
        [[nodiscard]] static const std::vector<Derived>& load()
        {
            if (s_cache.has_value())
            {
                LOG_INFO("Cached data used");
                return *s_cache;
            }

            LOG_INFO("Caching new file data");
            return m_populate();
        }


        /**
         * @brief clears the cache
         *
         * called by `PersistentEntity::_write_all()` before every write so that
         * the next `load()` re-reads the freshly written file.
         *
         * @note `std::optional::reset()` destroys the held `std::vector` and frees its memory
         */
        static void invalidate() noexcept
        {
            LOG_INFO("Cache no longer valid");
            s_cache.reset(); // std::optional calls std::vector ~destructor()
        }


    private:
        inline static std::optional<std::vector<Derived>> s_cache; ///< cached records; empty optional = cold cache

        /**
         * @brief reads the file from disk, stores the result in `s_cache`, and returns it
         * @return `const` reference to the newly populated cache
         */
        static const std::vector<Derived>& m_populate()
        {
            std::vector<Derived> records;

            std::ifstream file(Derived::file_name().data());

            if(file.is_open())
            {
                std::string line;
                while(std::getline(file, line))
                    if(!line.empty())
                        records.push_back(Derived::decode(line));
            }

            s_cache = std::move(records);
            return *s_cache;
        }
};
