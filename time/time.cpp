#include "mbed.h"

void TimeToTm(uint32_t time, struct tm* ptm)
{
    int seconds  = time % 60; time /= 60;
    int minutes  = time % 60; time /= 60;
    int hours    = time % 24; time /= 24;
    int daysLeft = time;
    
    //Add a year at a time while there is more than a year of days left
    int year      = 1970; //Unix epoch is 1970
    int dayOfWeek = 4;    //1 Jan 1970 is a Thursday
    int leapYear;
    while(1)
    {
        //See if it is a leap year
                             leapYear = false;
        if (year %   4 == 0) leapYear =  true;
        if (year % 100 == 0) leapYear = false;
        if (year % 400 == 0) leapYear =  true;
        
        //Find the number of days in this year
        int daysInYear = leapYear ? 366 : 365;
        
        //Stop if this is the final year
        if (daysLeft < daysInYear) break;
        
        //Calculate the current day of the week at the start of the year
        dayOfWeek += leapYear ? 2 : 1;
        if (dayOfWeek >= 7) dayOfWeek -= 7;
        
        //Move on to the next year
        daysLeft -= daysInYear;
        year++;
    }
    
    //Days left contains the days left from the start (1 Jan) of the current year
    int dayOfYear = daysLeft + 1;
    dayOfWeek += daysLeft;
    dayOfWeek %= 7;

    //Add a month at a time while there is more than a month of days left
    static char monthlengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int month = 1;
    while(1)
    {
        int daysInMonth = monthlengths[month-1];
        if (month == 2 && leapYear) daysInMonth++;
        
        //Stop if this is the last month
        if (daysLeft < daysInMonth) break;
        
        //Move onto next month
        daysLeft -= daysInMonth;
        month++;
    }
    
    //Days left contains the days left from the start (1st) of the current month
    int dayOfMonth = daysLeft + 1;
      
    //Find the last Sunday in the month
    int lastSunday = monthlengths[month-1];
    while (lastSunday > 0)
    {
        int weekday = (dayOfWeek + lastSunday - dayOfMonth) % 7;
        if (weekday == 0) break; //Stop when weekday is Sunday
        lastSunday--;
    }
    
    //Work out if Daylight Saving Time applies
    int dst;
    if (month <= 2) dst = false;              //Jan, Feb
    if (month == 3)                           //Mar - DST true after 1am UTC on the last Sunday in March
    {
        if (dayOfMonth <  lastSunday) dst = false;
        if (dayOfMonth == lastSunday) dst = hours >= 1;
        if (dayOfMonth >  lastSunday) dst = true;
    }
    if (month >= 4 && month <= 9) dst = true; //Apr, May, Jun, Jul, Aug, Sep
    if (month == 10)                          //Oct - DST false after 1am UTC on the last Sunday in October
    {
        if (dayOfMonth <  lastSunday) dst = true;
        if (dayOfMonth == lastSunday) dst = hours < 1;
        if (dayOfMonth >  lastSunday) dst = false;
    }
    if (month >= 11) dst = false;             //Nov, Dec
    
    
    //Set up the broken time TM structure
    ptm->tm_sec   = seconds;       // 00 --> 59
    ptm->tm_min   = minutes;       // 00 --> 59
    ptm->tm_hour  = hours;         // 00 --> 23
    ptm->tm_mday  = dayOfMonth;    // 01 --> 31
    ptm->tm_mon   = month - 1;     // 00 --> 11
    ptm->tm_year  = year - 1900;   // Years since 1900
    ptm->tm_wday  = dayOfWeek;     // 0 --> 6 where 0 == Sunday
    ptm->tm_yday  = dayOfYear - 1; // 0 --> 365
    ptm->tm_isdst = dst;           // +ve if DST, 0 if not DSTime, -ve if the information is not available. Note that 'true' evaluates to +1.
    
}
