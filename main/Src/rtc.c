/* Public Includes */
#include <stdio.h> // Required for sprintf() function

/* Private Includes */
#include "rtc.h"
#include <time.h> // Dummy time for the moment until rtc is connected

void rtc_get_formatted_date_time(char* str) {

    // TODO: Read the time from the real time clock

    // Store the time into the given string
    int day   = 1;
    int month = 1;
    int year  = 2023;
    // int minute       = 30;
    // int hour12hrTime = 9;
    // int am           = 1;

    // sprintf(str, "%d/%d/%d %d:%d%s", day, month, year, hour12hrTime, minute, am == 1 ? "am" : "pm");

    sprintf(str, "%d/%d/%d %ld", day, month, year, time(NULL));
}