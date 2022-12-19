#ifndef RTC_H
#define RTC_H

/* Public Macros */
#define RTC_DATE_TIME_CHAR_LENGTH 20

/**
 * @brief Requests the time from the real time clock, formats
 * the time into dd/mm/yy hh:mm format and then stores that as
 * a string into the given char array.
 *
 * @param str The array for the formatted time to be stored in.
 * Ensure that this array is at least 15 characters long
 */
void rtc_get_formatted_date_time(char* str);

#endif // RTC_H