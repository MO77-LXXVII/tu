// person.hpp

#pragma once
#include <string>

/**
 * @brief base class representing a person with basic contact information
 */
class Person
{
    protected:
        std::string m_first_name; ///< first name
        std::string m_last_name;  ///< last name
        std::string m_email;      ///< email address
        std::string m_phone_num;  ///< phone number

    public:
        /**
         * @brief construct a person with full contact details
         * @param first_name first name
         * @param last_name  last name
         * @param email      email address
         * @param phone_num  phone number
         */
        Person(std::string first_name, std::string last_name, std::string email, std::string phone_num):
            m_first_name(first_name), m_last_name(last_name), m_email(email), m_phone_num(phone_num){}

        Person() = default;

        /**
         * @brief returns the full name as `"first last"`
         */
        std::string full_name()
        {
            return m_first_name + " " + m_last_name;
        }
};
