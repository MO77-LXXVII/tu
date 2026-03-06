// person.hpp

#pragma once
#include <string>

class Person
{
    protected:
        std::string m_first_name; 
        std::string m_last_name;
        std::string m_email;
        std::string m_phone_num;

    public:
        Person(std::string first_name, std::string last_name, std::string email, std::string phone_num):
            m_first_name(first_name), m_last_name(last_name), m_email(email), m_phone_num(phone_num){}

        Person() = default;

        std::string full_name()
        {
            return m_first_name + " " + m_last_name;
        }
};
