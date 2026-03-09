// file_cache.hpp

#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <optional>
#include <string_view>




template<typename Derived>
class FileCache
{
    public:

        [[nodiscard]] static std::vector<Derived> load()
        {
            if (s_cache.has_value())
                return *s_cache;

            return m_populate();
        }


        static void invalidate() noexcept
        {
            s_cache.reset(); // std::optional calls std::vector ~destructor()
        }


    private:
        // will carry Derived file data
        inline static std::optional<std::vector<Derived>> s_cache;


        static std::vector<Derived>& m_populate()
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
