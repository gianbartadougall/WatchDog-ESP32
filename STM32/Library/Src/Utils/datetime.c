/**
 * @file datetime.c
 * @author Gian Barta-Dougall ()
 * @brief
 * @version 0.1
 * @date 2023-01-30
 *
 * @copyright Copyright (c) 2023
 *
 */

/* C Library Includes */
#include <stdio.h>

/* Personal Includes */
#include "datetime.h"
#include "utils.h"
#include "chars.h"

#define JANUARY   1
#define FEBRUARY  2
#define MARCH     3
#define APRIL     4
#define MAY       5
#define JUNE      6
#define JULY      7
#define AUGUST    8
#define SEPTEMBER 9
#define OCTOBER   10
#define NOVEMEBER 11
#define DECEMBER  12
#define MIN_YEAR  2022
#define MAX_YEAR  2100

#define DT_DAY_MONDAY    1
#define DT_DAY_TUESDAY   2
#define DT_DAY_WEDNESDAY 3
#define DT_DAY_THURSDAY  4
#define DT_DAY_FRIDAY    5
#define DT_DAY_SATURDAY  6
#define DT_DAY_SUNDAY    7

#define YEAR_IS_LEAP_YEAR(year) ((year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0)))

const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

/* Function Prototypes */
void dt_datetime_add_days(dt_datetime_t* Datetime, uint16_t days);
void dt_datetime_add_hours(dt_datetime_t* Datetime, uint16_t hours);
void dt_datetime_add_minutes(dt_datetime_t* Datetime, uint16_t minutes);
void dt_datetime_add_seconds(dt_datetime_t* Datetime, uint16_t minutes);

uint8_t dt_time_is_valid(dt_time_t* Time) {
    return dt_time_valid(Time->second, Time->minute, Time->hour);
}

uint8_t dt_datetime_init(dt_datetime_t* datetime, uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t days,
                         uint8_t months, uint16_t years) {

    if (dt_time_init(&datetime->Time, seconds, minutes, hours) != TRUE) {
        return FALSE;
    }

    if (dt_date_init(&datetime->Date, days, months, years) != TRUE) {
        return FALSE;
    }

    return TRUE;
}

uint8_t dt_time_init(dt_time_t* Time, uint8_t second, uint8_t minute, uint8_t hour) {

    if (dt_time_valid(second, minute, hour) != TRUE) {
        return FALSE;
    }

    Time->second = second;
    Time->minute = minute;
    Time->hour   = hour;

    return TRUE;
}

uint8_t dt_date_init(dt_date_t* Date, uint8_t day, uint8_t month, uint16_t year) {

    if (dt_date_valid(day, month, year) != TRUE) {
        return FALSE;
    }

    Date->day   = day;
    Date->month = month;
    Date->year  = year;

    return TRUE;
}

uint8_t dt_date_is_valid(dt_date_t* Date) {
    return dt_date_valid(Date->day, Date->month, Date->year);
}

uint8_t dt_date_valid(uint8_t day, uint8_t month, uint16_t year) {

    if ((day == 0) || (month == 0) || (year == 0) || (month > 12)) {
        return FALSE;
    }

    uint8_t maxDays = daysInMonth[month - 1]; // Minus 1 because index starts from 0
    if ((month == FEBRUARY) && YEAR_IS_LEAP_YEAR(year) == TRUE) {
        maxDays = 29;
    }

    if (day > maxDays) {
        return FALSE;
    }

    return TRUE;
}

uint8_t dt_time_t1_leq_t2(dt_time_t* t1, dt_time_t* t2) {

    if (t1->hour < t2->hour) {
        return TRUE;
    }

    if ((t1->hour == t2->hour) && (t1->minute < t2->minute)) {
        return TRUE;
    }

    if ((t1->hour == t2->hour) && (t1->minute == t2->minute) && (t1->second <= t2->second)) {
        return TRUE;
    }

    return FALSE;
}

uint8_t dt_time_add_time(dt_time_t* Time, dt_time_t timeToAdd) {

    Time->second += timeToAdd.second;
    if (Time->second > 60) {
        Time->second -= 60;
        Time->minute++;
    }

    Time->minute += timeToAdd.minute;
    if (Time->minute > 60) {
        Time->minute -= 60;
        Time->hour++;
    }

    Time->hour += timeToAdd.hour;
    if (Time->minute > 23) {
        return FALSE;
    }

    return TRUE;
}

void dt_datetime_add_time(dt_datetime_t* Datetime, uint16_t seconds, uint16_t minutes, uint16_t hours, uint16_t days) {
    dt_datetime_add_seconds(Datetime, seconds);
    dt_datetime_add_minutes(Datetime, minutes);
    dt_datetime_add_hours(Datetime, hours);
    dt_datetime_add_days(Datetime, days);
}

uint8_t dt1_greater_than_dt2(dt_datetime_t* Dt1, dt_datetime_t* Dt2) {

    if (Dt2->Date.year > Dt1->Date.year) {
        return FALSE;
    }

    if (Dt2->Date.month > Dt1->Date.month) {
        return FALSE;
    }

    if (Dt2->Date.day > Dt1->Date.day) {
        return FALSE;
    }

    if (Dt2->Time.hour > Dt1->Time.hour) {
        return FALSE;
    }

    if (Dt2->Time.minute > Dt1->Time.minute) {
        return FALSE;
    }

    if (Dt2->Time.second >= Dt1->Time.second) {
        return FALSE;
    }

    return FALSE;
}

void dt_datetime_increment_day(dt_datetime_t* datetime) {

    uint8_t nextMonth = FALSE;

    if (datetime->Date.month == FEBRUARY && YEAR_IS_LEAP_YEAR(datetime->Date.year) == TRUE) {
        if (datetime->Date.day == 29) {
            nextMonth = TRUE;
        }
    }

    if (daysInMonth[datetime->Date.month] == datetime->Date.day) {
        nextMonth = TRUE;
    }

    if (nextMonth == TRUE) {
        datetime->Date.day = 1;
        datetime->Date.month++;
    } else {
        datetime->Date.day++;
    }

    // Correct for decemeber
    if (datetime->Date.month > DECEMBER) {
        datetime->Date.month = JANUARY;
    }
}

uint8_t dt_datetime_set_time(dt_datetime_t* datetime, dt_time_t Time) {

    // Confirm the time is valid
    if (dt_time_is_valid(&Time) != TRUE) {
        return FALSE;
    }

    datetime->Time.second = Time.second;
    datetime->Time.minute = Time.minute;
    datetime->Time.hour   = Time.hour;

    return TRUE;
}

uint8_t dt_time_valid(uint8_t second, uint8_t minute, uint8_t hour) {

    // Confirm seconds are valid
    if (second > 59) {
        return FALSE;
    }

    // Confirm minutes are valid
    if (minute > 59) {
        return FALSE;
    }

    // Confirm hours are valid
    if (hour > 23) {
        return FALSE;
    }

    return TRUE;
}

uint8_t is_leap_year(int year) {
    return ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0);
}

uint8_t dt_is_valid_date(char* date) {
    int day, month, year;
    if (chars_get_num_bytes(date) > 10) {
        return FALSE;
    }
    if (sscanf(date, "%d/%d/%d", &day, &month, &year) != 3) {
        return FALSE;
    }
    if (month < 1 || month > 12) {
        return FALSE;
    }
    if (day < 1 || day > 31) {
        return FALSE;
    }
    if ((month == 2) && (day > 29)) {
        return FALSE;
    }
    if ((month == 2) && (day == 29) && (!is_leap_year(year))) {
        return FALSE;
    }
    if ((month == 4 || month == 6 || month == 9 || month == 11) && (day > 30)) {
        return FALSE;
    }
    if (year < MIN_YEAR || year > MAX_YEAR) {
        return FALSE;
    }
    return TRUE;
}

uint8_t dt_is_valid_hour_min(char* time) {

    if (dt_time_format_is_valid(time) != TRUE) {
        return FALSE;
    }

    int hour, min;
    if (sscanf(time, "%d:%d", &hour, &min) != 2) {
        return FALSE;
    }

    if ((min < 0) || min >= 60) {
        return FALSE;
    }
    if ((hour <= 0) || hour > 23) {
        return FALSE;
    }
    return TRUE;
}

uint8_t dt_time_format_is_valid(char* time) {

    int colonCount = 0;
    int i;
    for (i = 0; time[i] != '\0'; i++) {

        if (i > 4) {
            return FALSE;
        }

        if (time[i] == ':' && colonCount < 1) {
            colonCount++;
            if (time[i + 1] == '\0' || time[i + 2] == '\0') {
                return FALSE;
            }
            continue;
        }

        if (time[i] < '0' || time[i] > '9') {
            return FALSE;
        }
    }
    return TRUE;
}

uint8_t dt_is_valid_hour_min_period(char* time) {
    if (chars_get_num_bytes(time) > 8) {
        return FALSE;
    }
    int hour, min;
    int firstZero = 0;
    int isSpace   = 0;
    char period[3];
    int num;
    if ((num = sscanf(time, "%d:%d %2s", &hour, &min, period)) != 3) {
        return FALSE;
    }
    if (time[0] == '0' || hour > 9) {
        firstZero = 1;
    }
    if (time[1 + firstZero] != ':') {
        return FALSE;
    }
    if (time[4 + firstZero] == ' ') {
        isSpace = 1;
    }
    if (chars_get_num_bytes(time) > (6 + isSpace + firstZero)) {
        return FALSE;
    }
    if ((min < 0) || min >= 60) {
        return FALSE;
    }
    if ((hour <= 0) || hour > 12) {
        return FALSE;
    }
    if (chars_same(period, "am\0") != TRUE && chars_same(period, "pm\0") != TRUE) {
        return FALSE;
    }
    return TRUE;
}

void dt_datetime_to_string(dt_datetime_t* datetime, char* string) {
    sprintf(string, "%i:%i:%i %i/%i/%i", datetime->Time.hour, datetime->Time.minute, datetime->Time.second,
            datetime->Date.day, datetime->Date.month, datetime->Date.year);
}

void dt_time_to_string(char* timeString, dt_time_t timeStruct, uint8_t hasPeriod) {
    uint8_t minute;
    minute = timeStruct.minute;
    uint8_t hour;
    hour = timeStruct.hour;
    char period[3];
    if (hour >= 12) {
        sprintf(period, "pm");
    } else {
        sprintf(period, "am");
    }
    if (hour > 12) {
        hour -= 12;
    }
    if (hour == 0) {
        hour += 12;
    }
    if (hasPeriod == TRUE) {
        sprintf(timeString, "%i:%s%i %s", hour, minute < 10 ? "0" : "", minute, period);
    } else if (hasPeriod == FALSE) {
        sprintf(timeString, "%i:%s%i", hour, minute < 10 ? "0" : "", minute);
    }
}

// void dt_datetime_to_string(dt_datetime_t* Datetime, char string[40], enum dt_format_e format) {

//     if (format == DT_FORMAT_FULL) {
//         sprintf(datetimeString, "%i%s%i%s%i_%s%i%s%i", DateTime.Date.year - 2000, DateTime.Date.month < 10 ? "0" :
//         "",
//                 DateTime.Date.month, DateTime.Date.day < 10 ? "0" : "", DateTime.Date.day,
//                 DateTime.Time.hour < 10 ? "0" : "", DateTime.Time.hour, DateTime.Time.minute < 10 ? "0" : "",
//                 DateTime.Time.minute);
//         return;
//     }

// }

uint8_t dt_calculate_day_of_week(dt_date_t* Date) {

    // Copy values across so the date data is not altered
    uint8_t day   = Date->day;
    uint8_t month = Date->month;
    uint16_t year = Date->year;

    /* Below is just a standard formula that I have gotten of the internet*/
    if (month < 3) {
        month += 12;
        year--;
    }

    int century         = year / 100;
    int year_of_century = year % 100;

    int day_of_week =
        (day + (13 * (month + 1) / 5) + year_of_century + (year_of_century / 4) + (century / 4) - (2 * century)) % 7;

    day_of_week--;

    if (day_of_week < 0) {
        day_of_week += 7;
    }

    if (day_of_week == 0) {
        return DT_DAY_SUNDAY;
    }

    return day_of_week;
}

void dt_datetime_add_days(dt_datetime_t* Datetime, uint16_t days) {

    while (1) {

        // Calculate number of days until
        uint8_t daysLeftInMonth = daysInMonth[Datetime->Date.month - 1] - Datetime->Date.day;
        if (days <= daysLeftInMonth) {
            Datetime->Date.day += days;
            return;
        }

        // Subtract extra 1 to get to the first day of the next month
        days = days - daysLeftInMonth - 1;

        Datetime->Date.day = 1;
        Datetime->Date.month++;

        if (Datetime->Date.month == 13) {
            Datetime->Date.month = 1;

            // increment the year by 1
            Datetime->Date.year++;
        }
    }
}

void dt_datetime_add_hours(dt_datetime_t* Datetime, uint16_t hours) {

    uint16_t daysToAdd = hours / 24;
    uint8_t hoursToAdd = hours % 24;

    if ((Datetime->Time.hour + hoursToAdd) < 24) {
        Datetime->Time.hour += hoursToAdd;
    } else {
        Datetime->Time.hour += hoursToAdd - 24;
        daysToAdd++;
    }

    // Add extra days
    dt_datetime_add_days(Datetime, daysToAdd);
}

void dt_datetime_add_minutes(dt_datetime_t* Datetime, uint16_t minutes) {

    uint32_t hoursToAdd   = minutes / 60;
    uint16_t minutesToAdd = minutes % 60;

    if ((Datetime->Time.minute + minutesToAdd) < 60) {
        Datetime->Time.minute += minutesToAdd;
    } else {
        Datetime->Time.minute += minutesToAdd - 60;
        hoursToAdd++;
    }

    // Add extra hours
    dt_datetime_add_hours(Datetime, hoursToAdd);
}

void dt_datetime_add_seconds(dt_datetime_t* Datetime, uint16_t seconds) {

    uint16_t minutesToAdd = seconds / 60;
    uint8_t secondsToAdd  = seconds % 60;

    if ((Datetime->Time.second + secondsToAdd) < 60) {
        Datetime->Time.second += secondsToAdd;
    } else {
        Datetime->Time.second += secondsToAdd - 60;
        minutesToAdd++;
    }

    // Add extra minutes
    dt_datetime_add_minutes(Datetime, minutesToAdd);
}

uint8_t dt_datetimes_are_equal(dt_datetime_t* dt1, dt_datetime_t* dt2) {

    if (dt1->Time.second != dt2->Time.second) {
        return FALSE;
    }

    if (dt1->Time.minute != dt2->Time.minute) {
        return FALSE;
    }

    if (dt1->Time.hour != dt2->Time.hour) {
        return FALSE;
    }

    if (dt1->Date.day != dt2->Date.day) {

        return FALSE;
    }

    if (dt1->Date.month != dt2->Date.month) {
        return FALSE;
    }

    if (dt1->Date.year != dt2->Date.year) {
        return FALSE;
    }

    return TRUE;
}

// void dt_datetime_subtract_seconds(dt_datetime_t* Datetime, uint16_t seconds) {

//     uint16_t minutesToRemove = seconds / 60;
//     uint8_t secondsToRemove  = seconds % 60;

//     if ((Datetime->Time.second - secondsToRemove) >= 0) {
//         Datetime->Time.second -= secondsToRemove;
//     } else {
//         Datetime->Time.second = (60 - (secondsToRemove - Datetime->Time.second));
//         minutesToRemove++;
//     }

//     if (minutesToRemove > 0) {
//         dt_datetime_subtract_minutes(Datetime, minutesToRemove);
//     }
// }

// void dt_datetime_subtract_minutes(dt_datetime_t* Datetime, uint16_t minutes) {

//     uint32_t hoursToRemove   = minutes / 60;
//     uint16_t minutesToRemove = minutes % 60;

//     if ((Datetime->Time.minute - minutesToRemove) >= 0) {
//         Datetime->Time.minute -= minutesToRemove;
//     } else {
//         Datetime->Time.minute = (60 - (minutesToRemove - Datetime->Time.minute));
//         hoursToRemove++;
//     }

//     if (hoursToRemove > 0) {
//         dt_datetime_subtract_hours(Datetime, hoursToRemove);
//     }
// }

// void dt_datetime_subtract_hours(dt_datetime_t* Datetime, uint16_t hours) {

//     uint16_t daysToRemove = hours / 24;
//     uint8_t hoursToRemove = hours % 24;

//     if ((Datetime->Time.hour - hoursToRemove) >= 0) {
//         Datetime->Time.hour -= hoursToRemove;
//     } else {
//         Datetime->Time.hour = (60 - (hoursToRemove - Datetime->Time.hour));
//         daysToRemove++;
//     }

//     // Add extra days
//     dt_datetime_subtract_days(Datetime, daysToRemove);
// }

// void dt_datetime_subtract_days(dt_datetime_t* Datetime, uint16_t days) {

//     while (1) {

//         if ((Datetime->Date.day - days) > 0) {
//             Datetime->Date.day -= days;
//             return;
//         }

//         // Subtract days then decrease the month
//         days -= Datetime->Date.day;

//         Datetime->Date.month--;

//         if (Datetime->Date.month == 0) {
//             Datetime->Date.month = 12;
//             Datetime->Date.year--;
//         }

//         Datetime->Date.day = daysInMonth[Datetime->Date.month - 1];
//     }
// }