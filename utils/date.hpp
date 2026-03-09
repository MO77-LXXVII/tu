// date.hpp

#pragma once

#include <string>
#include <string_view>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <algorithm>

#include "../tu/output.hpp"

namespace utils
{
    /**
     * @brief Comprehensive date manipulation class with calendar operations.
     * 
     * Design philosophy:
     * - Dual interface: static methods (pure functions) + instance methods
     * - Validation on construction (invalid dates default to 2000-01-01)
     * - Value semantics (dates are comparable and copyable)
     * - Rich comparison operators
     * - Business day calculations
     */
    class Date
    {
        public:
            // -----------------
            // Constructors
            // -----------------

            // Default: 2000-01-01 (epoch)
            Date() = default;

            // From year, month, day (validates and adjusts if invalid)
            Date(int year, int month, int day)
                : _year(year), _month(month), _day(day)
            {
                if(!is_date_valid())
                {
                    _year = 2000;
                    _month = 1;
                    _day = 1;
                }
            }

            // From string "dd/mm/yyyy" or "yyyy-mm-dd"
            explicit Date(std::string_view date_str)
            {
                *this = parse(date_str);
            }










            // -----------------
            // Static Factory Methods
            // -----------------

            [[nodiscard]] static Date parse(std::string_view str)
            {
                // Try "DD/MM/YYYY"
                if(str.find('/') != std::string_view::npos)
                {
                    int day, month, year;
                    char sep1, sep2;
                    std::istringstream iss(str.data());

                    if(iss >> day >> sep1 >> month >> sep2 >> year)
                        return Date(year, month, day);
                }


                // Try "YYYY-MM-DD"
                if(str.find('-') != std::string_view::npos)
                {
                    int day, month, year;
                    char sep1, sep2;
                    std::istringstream iss(str.data());

                    if(iss >> year >> sep1 >> month >> sep2 >> day)
                        return Date(year, month, day);
                }

                return Date(); // Invalid format -> epoch
            }


            [[nodiscard]] static Date today() noexcept
            {
                std::time_t now = std::time(nullptr);
                std::tm* local  = std::localtime(&now);

                return Date(
                    local->tm_year + 1900,
                    local->tm_mon + 1,
                    local->tm_mday
                );
            }


            // day N of year
            // converts a day position inside a year into an actual calendar date.
            // year = 2024 day_index_in_year = 60 ---> 2024-02-29
            [[nodiscard]] static Date from_ordinal_date(int year, int day_index_in_year) noexcept
            {
                Date d(year, 1, 1);
                
                for (int m = 1; m <= 12; ++m)
                {
                    int month_length = Date::month_length(year, m);

                    if(day_index_in_year <= month_length)
                    {
                        d._month = m;
                        d._day = day_index_in_year;
                        return d;
                    }

                    day_index_in_year -= month_length;
                }
                return d;
            }


            [[nodiscard]] Date from_ordinal_date(int day_index_in_year) const noexcept
            {
                return from_ordinal_date(this->_year, day_index_in_year);
            }


            [[nodiscard]] static int total_days_from_the_begininng_of_the_year(const Date& d) noexcept
            {
                int total_days{};

                for (int month = 1; month <= (d._month - 1); ++month)
                    total_days += month_length(d._year , month);

                return (total_days + d._day);
            }


            [[nodiscard]] int total_days_from_the_begininng_of_the_year() const noexcept
            {
                return total_days_from_the_begininng_of_the_year(*this);
            }


            /**
             * @brief returns the current date and time as a formatted timestamp string
             * @return e.g. "2000-01-01 12:00:00"
             */
            [[nodiscard]] static std::string timestamp() noexcept
            {
                auto now  = std::chrono::system_clock::now();
                auto time = std::chrono::system_clock::to_time_t(now);

                std::ostringstream oss;
                oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
                return oss.str();
            }





            // -----------------
            // Accessors
            // -----------------

            [[nodiscard]] int year() const noexcept
            {
                return _year;
            }

            [[nodiscard]] int month() const noexcept
            {
                return _month;
            }

            [[nodiscard]] int day() const noexcept
            {
                return _day;
            }

            Date& set_year(int year) noexcept
            {
                _year = year; return *this;
            }

            Date& set_month(int month) noexcept
            {
                _month = month; return *this;
            }

            Date& set_day(int day) noexcept
            {
                _day = day; return *this;
            }






            // -----------------
            // Validation & Properties
            // -----------------

            [[nodiscard]] bool is_date_valid() const noexcept
            {
                if(_month < 1 || _month > 12)
                    return false;

                if(_day < 1 || _day > month_length(_year, _month))
                    return false;

                return true;
            }

            [[nodiscard]] static bool is_leap_year(int year) noexcept
            {
                return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            }

            [[nodiscard]] bool is_leap_year() const noexcept
            {
                return is_leap_year(_year);
            }

            [[nodiscard]] static int month_length(int year, int month) noexcept
            {
                static constexpr int days[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

                if(month < 1 || month > 12)
                    return 0;

                if(month == 2 && is_leap_year(year))
                    return 29;

                return days[month];
            }

            [[nodiscard]] int month_length() const noexcept
            {
                return month_length(_year, _month);
            }

            [[nodiscard]] static int total_days_in_year(int year) noexcept
            {
                return is_leap_year(year) ? 366 : 365;
            }

            [[nodiscard]] int total_days_in_year() const noexcept
            {
                return total_days_in_year(_year);
            }










            // -----------------
            // Day of Week Calculations (Zeller's Congruence)
            // -----------------

            // computes which day of the week a date falls on
            [[nodiscard]] static int day_of_week(const Date& d) noexcept
            {
                int year = d._year, month = d._month, day = d._day;
                // Zeller's congruence for Gregorian calendar
                int m = month;
                int y = year;

                if(m < 3)
                {
                    m += 12;
                    --y;
                }

                int h = (day + (13 * (m + 1)) / 5 + y + y / 4 - y / 100 + y / 400) % 7;

                // Convert: 0=Sat, 1=Sun, 2=Mon, ..., 6=Fri
                // To: 0=Sun, 1=Mon, ..., 6=Sat
                return (h + 6) % 7;
            }

            [[nodiscard]] int day_of_week() const noexcept
            {
                return day_of_week(*this);
            }

            [[nodiscard]] static bool is_end_of_week(const Date& d) noexcept
            {
                return day_of_week(d) == 0;
            }

            [[nodiscard]] bool is_end_of_week() const noexcept
            {
                return day_of_week(*this) == 0;
            }

            [[nodiscard]] static std::string_view day_name(int day_index) noexcept
            {
                static constexpr std::string_view names[] =
                {
                    "Sunday", "Monday", "Tuesday", "Wednesday", 
                    "Thursday", "Friday", "Saturday"
                };

                return names[day_index % 7];
            }

            [[nodiscard]] std::string_view day_name() const noexcept
            {
                return day_name(day_of_week());
            }

            [[nodiscard]] static std::string_view day_name_short(int day_index) noexcept
            {
                static constexpr std::string_view names[] =
                {
                    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
                };

                return names[day_index % 7];
            }

            [[nodiscard]] std::string_view day_name_short() const noexcept
            {
                return day_name_short(day_of_week());
            }

            [[nodiscard]] static std::string_view month_name(int month) noexcept
            {
                static constexpr std::string_view names[] =
                {
                    "January", "February", "March", "April", "May", "June",
                    "July", "August", "September", "October", "November", "December"
                };

                if(month < 1 || month > 12)
                    return "";

                return names[month - 1];
            }

            [[nodiscard]] std::string_view month_name() const noexcept
            {
                return month_name(_month);
            }

            [[nodiscard]] static std::string_view month_name_short(int month) noexcept
            {
                static constexpr std::string_view names[] =
                {
                    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                };

                if(month < 1 || month > 12)
                    return "";

                return names[month - 1];
            }

            [[nodiscard]] std::string_view month_name_short() const noexcept
            {
                return month_name_short(_month);
            }







            // -----------------
            // Print calendar
            // -----------------

            void print_month_calendar(Date d)
            {
                tu::output::FormatGuard fg;

                std::cout << "________________________" << month_name(d._month) << "________________________" << std::endl;

                for (size_t i = 1; i <= 7; i++)
                    std::cout << std::setw(8) << std::left << day_name(i);
                std::cout << std::endl;

                d._day = 1;
                int start_day = day_of_week(d);

                for (int i = 0; i < start_day; ++i)
                    std::cout << std::setw(8) << ""; // 8 spaces per cell

                int day_counter  = 1;
                for (int i = start_day; day_counter <= month_length(d._year, d._month); ++i)
                {
                    std::cout << std::setw(3) << std::right << day_counter << std::setw(5) << "";

                    if (i % 7 == 6) // end of week
                        std::cout << std::endl;

                    ++day_counter;
                }

                std::cout << std::endl;

                std::cout << "________________________" << "___" << "________________________" << std::endl;
            }

            void print_year_calendar(Date d)
            {
                d._day = 1;

                for (size_t i = 1; i <= 12; i++)
                {
                    d._month = i;
                    print_month_calendar(d);
                    std::cout << std::endl;
                }
            }














            // -----------------
            // Date Arithmetic
            // -----------------

            [[nodiscard]] static Date add_days(Date date, int days) noexcept
            {
                // forward shifts
                while(days > 0)
                {
                    int remaining_in_month = month_length(date._year, date._month) - date._day;

                    if(days <= remaining_in_month)
                    {
                        date._day += days;
                        return date;
                    }

                    // finish current month + first day of next month
                    days -= remaining_in_month + 1;
                    date._day = 1;

                    if((++date._month) > 12)
                    {
                        date._month = 1;
                        ++date._year;
                    }
                }

                // backward shifts
                while(days < 0)
                {
                    if(date._day > 1)
                    {
                        // if today is day 10; earliest day in month = 1, max backward steps inside month = 9 (not 10)
                        const int remaining_days_before_month_start = date._day - 1;
                        int can_subtract = std::min(-days, remaining_days_before_month_start);

                        date._day -= can_subtract;
                        days += can_subtract;
                    }
                    else
                    {
                        // 2024-01-01 → 2023-12-?
                        if((--date._month) < 1)
                        {
                            date._month = 12;
                            --date._year;
                        }

                        // 2023-12-31
                        date._day = month_length(date._year, date._month);

                        // moved 1 day backward
                        ++days;
                    }
                }

                return date;
            }

            [[nodiscard]] Date add_days(int days) const noexcept
            {
                return add_days(*this, days);
            }

            [[nodiscard]] Date add_months(int months) const noexcept
            {
                Date result = *this;

                // total months passed in all years up to this date.
                const int months_passed = result._year * 12;

                // converts 1-based months (Jan = 1 ... Dec = 12) to 0-based index.
                const int month_index_zero_based = result._month - 1;

                // add the requested shift, can be negative.
                const int shift = months;

                int total_months =  months_passed + month_index_zero_based + shift;

                /*
                    Date: 2024-02-15
                    months = 5
                    total_months = (2024 * 12) + (2-1) + (5) = (2024 * 12) + 1 + 5 = 24294
                */

                if(total_months < 0)
                    total_months = 0;

                // full years passed
                result._year = total_months / 12; // 24294 / 12 = 2024(.5) --> truncated

                // remaining month in 1–12 range (+1 shifts back from 0-based index)
                result._month = (total_months % 12) + 1; // (24294 % 12) = 6; 6 + 1 = 7 

                // Adjust day if it exceeds the new month's max
                int max_day = month_length(result._year, result._month);

                /*
                    Date: 2024-01-31
                    add_months(1) → February
                    Feb 2024 has 29 days → adjust 31 → 29
                */
                if(result._day > max_day)
                    result._day = max_day;

                return result;
            }

            [[nodiscard]] Date add_years(int years) const noexcept
            {
                Date result = *this;
                result._year += years;

                // Adjust for leap year edge case (Feb 29)
                if(result._month == 2 && result._day == 29 && !is_leap_year(result._year))
                    result._day = 28;

                return result;
            }














            // -----------------
            // Date Differences
            // -----------------

            int is_date1_after_date2(Date d1, Date d2)
            {
                if(d1._year != d2._year)
                    return (d1._year > d2._year ? 1 : -1);

                int total_passed1 = total_days_from_the_begininng_of_the_year(d1);

                int total_passed2 = total_days_from_the_begininng_of_the_year(d2);

                return (total_passed1 != total_passed2 ? (total_passed1 > total_passed2 ? 1 : -1) : 0);
            }
            








            [[nodiscard]] static int difference_in_days(const Date& from, const Date& to) noexcept
            {
                Date start = from;
                Date end = to;
                bool negative = start > end;

                if(negative)
                    std::swap(start, end);

                int days = 0;

                while(start < end)
                {
                    start = add_days(start, 1);
                    ++days;
                }

                return negative ? -days : days;
            }

            int deference_between_two_dates_in_same_year(Date d, Date d2)
            {
                int date1_days = total_days_from_the_begininng_of_the_year(d);
                int date2_days = total_days_from_the_begininng_of_the_year(d2);

                return date2_days - date1_days;
            }

            int deference_between_2_dates(Date d, Date d2, bool include_end_date = false)
            {
                bool inverted = false;
                if (is_date1_after_date2(d, d2) == 1)
                {
                    std::swap(d, d2);
                    inverted = true;
                }

                if(d._year == d2._year)
                    return deference_between_two_dates_in_same_year(d, d2) + (include_end_date? 1 : 0);

                int total{};
                {
                    Date temp = d;
                    temp._month = 12;
                    temp._day = 31;

                    total = deference_between_two_dates_in_same_year(d, temp);
                }

                int next_year = d._year + 1;

                while (next_year <= d2._year)
                {
                    if(d2._year == next_year)
                    {
                        total += total_days_from_the_begininng_of_the_year(d2);
                        break;
                    }

                    total += total_days_in_year(next_year++);
                }

                return (total + (include_end_date? 1 : 0)) * (inverted ? -1 : 1);
            }

            [[nodiscard]] int difference_in_days(const Date& other) const noexcept
            {
                return difference_in_days(*this, other);
            }

            [[nodiscard]] static int age_in_days(const Date& other) noexcept
            {
                return difference_in_days(other, Date::today());
            }

            [[nodiscard]] int age_in_days() const noexcept
            {
                return difference_in_days(*this, Date::today());
            }
























            // -----------------
            // Business Day Operations
            // -----------------

            [[nodiscard]] bool is_weekend() const noexcept
            {
                int dow = day_of_week();
                return dow == 0 || dow == 6; // friday & saturday
                // ~~> consider sunday or saturday
            }

            [[nodiscard]] bool is_business_day() const noexcept
            {
                return !is_weekend();
            }

            [[nodiscard]] static int days_until_end_of_week(const Date& d) noexcept
            {
                int h = day_of_week(d);
                return (7 - h) % 7; // 0 if Saturday, 1 if Friday, etc.
            }

            [[nodiscard]] int days_until_end_of_week() const noexcept
            {
                return days_until_end_of_week(*this);
            }


            [[nodiscard]] static int days_until_end_of_month(const Date& d) noexcept
            {
                return month_length(d._year, d._month) - d._day;
            }

            [[nodiscard]] int days_until_end_of_month() const noexcept
            {
                return days_until_end_of_month(*this);
            }

            [[nodiscard]] static int days_until_end_of_year(const Date& d) noexcept
            {
                return (is_leap_year(d._year)? 366 : 365) - total_days_from_the_begininng_of_the_year(d);
            }

            [[nodiscard]] int days_until_end_of_year() const noexcept
            {
                return days_until_end_of_year(*this);
            }






            [[nodiscard]] static int count_business_days(const Date& from, const Date& to) noexcept
            {
                Date current = from;
                int count = 0;

                while(current <= to)
                {
                    if(current.is_business_day())
                        ++count;

                    current = current.add_days(1);
                }

                return count;
            }

            [[nodiscard]] int count_business_days(const Date& to) const noexcept
            {
                return count_business_days(*this, to);
            }

            /**
             * @brief Returns a new Date advanced forward by a given number of business days.
             *
             * Advances one calendar day at a time using add_days(1) and counts only days
             * for which is_business_day() returns true. Weekends / non-working days are skipped.
             *
             * @param business_days Number of business days to add (forward-only).
             * @return New Date after advancing the specified number of business days.
             *
             * @note Does not modify the current object.
             * @note Negative values are not supported.
             */
            [[nodiscard]] Date add_business_days(int business_days) const noexcept
            {
                Date result = *this;
                int added = 0;

                while(added < business_days)
                {
                    result = result.add_days(1);

                    if(result.is_business_day())
                        ++added;
                }

                return result;
            }










            // -----------------
            // Day index in a year
            // -----------------

            [[nodiscard]] static int day_index_in_year(int year, int month, int day) noexcept
            {
                int total = 0;

                for (int m = 1; m < month; ++m)
                    total += month_length(year, m);

                return total + day;
            }

            [[nodiscard]] int day_index_in_year() const noexcept
            {
                return day_index_in_year(_year, _month, _day);
            }










            // -----------------
            // Boundary Checks
            // -----------------

            [[nodiscard]] bool is_first_day_of_month() const noexcept
            {
                return _day == 1;
            }

            [[nodiscard]] bool is_last_day_of_month() const noexcept
            {
                return _day == month_length(_year, _month);
            }

            [[nodiscard]] bool is_first_month_of_year() const noexcept
            {
                return _month == 1;
            }

            [[nodiscard]] bool is_last_month_of_year() const noexcept
            {
                return _month == 12;
            }

            [[nodiscard]] int days_until_month_end() const noexcept
            {
                return month_length(_year, _month) - _day;
            }

            [[nodiscard]] int days_until_year_end() const noexcept
            {
                return total_days_in_year(_year) - day_index_in_year();
            }

























            // -----------------
            // Formatting
            // -----------------
            
            [[nodiscard]] std::string to_string() const
            {
                std::ostringstream oss;
                oss << std::setfill('0')
                    << std::setw(2) << _day << '/'
                    << std::setw(2) << _month << '/'
                    << std::setw(4) << _year;
                return oss.str();
            }
            
            [[nodiscard]] std::string format(std::string_view fmt = "dd/mm/yyyy") const
            {
                std::string result(fmt);
                
                // Helper to replace in string
                auto replace_all = [](std::string& str, std::string_view from, std::string_view to)
                {
                    size_t pos = 0;
                    while((pos = str.find(from, pos)) != std::string::npos)
                    {
                        str.replace(pos, from.length(), to);
                        pos += to.length();
                    }
                };
                
                // Year
                replace_all(result, "yyyy", std::to_string(_year));
                replace_all(result, "yy", std::to_string(_year % 100));
                
                // Month
                std::ostringstream month_padded;
                month_padded << std::setfill('0') << std::setw(2) << _month;
                replace_all(result, "mm", month_padded.str());
                replace_all(result, "m", std::to_string(_month));
                
                // Day
                std::ostringstream day_padded;
                day_padded << std::setfill('0') << std::setw(2) << _day;
                replace_all(result, "dd", day_padded.str());
                replace_all(result, "d", std::to_string(_day));
                
                return result;
            }
            







            // -----------------
            // Stream Output
            // -----------------

            friend std::ostream& operator<<(std::ostream& os, const Date& date)
            {
                return os << date.to_string();
            }

            // reads dd/mm/yyyy
            friend std::istream& operator>>(std::istream& is, Date& date)
            {
                int day{}, month{}, year{};
                char sep1{}, sep2{};

                if (is >> day >> sep1 >> month >> sep2 >> year && sep1 == sep2 && (sep1 == '/' || sep1 == '-'))
                    date = Date(year, month, day);

                else
                    is.setstate(std::ios::failbit); // 25@12@2024 → fail

                return is;
            }











            // -----------------
            // Comparison Operators
            // -----------------
            
            [[nodiscard]] bool operator==(const Date& other) const noexcept
            {
                return _year == other._year 
                    && _month == other._month 
                    && _day == other._day;
            }
            
            [[nodiscard]] bool operator!=(const Date& other) const noexcept
            {
                return !(*this == other);
            }
            
            [[nodiscard]] bool operator<(const Date& other) const noexcept
            {
                if(_year != other._year)
                    return _year < other._year;
                if(_month != other._month)
                    return _month < other._month;
                return _day < other._day;
            }
            
            [[nodiscard]] bool operator>(const Date& other) const noexcept
            {
                return other < *this;
            }
            
            [[nodiscard]] bool operator<=(const Date& other) const noexcept
            {
                return !(other < *this);
            }
            
            [[nodiscard]] bool operator>=(const Date& other) const noexcept
            {
                return !(*this < other);
            }





        private:
            // fixed reference point in time (Epoch = "2000-01-01")
            int _year = 2000;
            int _month = 1;
            int _day = 1;
    };

} // namespace utils
