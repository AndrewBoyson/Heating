#include "mbed.h"

#define STD_OFFSET 0
#define DST_OFFSET 1

int TimeScanUs = 0;

// Convert compile time to system time 
int TimeAsciiDateTimeToTm(const char* pDate, const char* pTime, struct tm* ptm)
{
    //Pull out the year, month, day, hour, minute and second from the compiler DATE and TIME macro constants
    char s_month[5];
    sscanf(pDate, "%s %d %d", s_month, &ptm->tm_mday, &ptm->tm_year); ptm->tm_year -= 1900;
    sscanf(pTime, "%2d %*c %2d %*c %2d", &ptm->tm_hour, &ptm->tm_min, &ptm->tm_sec);
    
    // Find where is s_month in month_names. Deduce month value. 
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    ptm->tm_mon = (strstr(month_names, s_month) - month_names) / 3;
        
    //Normalises the day of week and the day of year part of the tm structure as well as returning a time_t
    mktime(ptm); 
    
    return 0;
}
int TimeMain()
{
    //Establish the scan time
    static Timer scanTimer;
    int scanUs = scanTimer.read_us();
    scanTimer.reset();
    scanTimer.start();
    if (scanUs > TimeScanUs) TimeScanUs++;
    if (scanUs < TimeScanUs) TimeScanUs--;
    return 0;
}
static int isLeapYear(int year)
{
    int leapYear = false;
    if (year %   4 == 0) leapYear =  true;
    if (year % 100 == 0) leapYear = false;
    if (year % 400 == 0) leapYear =  true;
    return leapYear;
}
static int monthLength(int year, int month)
{
    static char monthlengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int daysInMonth = monthlengths[month-1];
    if (month == 2 && isLeapYear(year)) daysInMonth++;
    return daysInMonth;
}
static int isDst(int year, int month, int dayOfMonth, int dayOfWeek, int hours)
{
    //Find the last Sunday in the month
    int lastSunday = monthLength(year, month);
    while (lastSunday > 0)
    {
        int weekday = (dayOfWeek + lastSunday - dayOfMonth) % 7;
        if (weekday == 0) break; //Stop when weekday is Sunday
        lastSunday--;
    }

    //Check each month
    if (month <= 2) return false;                  //Jan, Feb
    if (month == 3)                                //Mar - DST true after 1am UTC on the last Sunday in March
    {
        if (dayOfMonth <  lastSunday) return false;
        if (dayOfMonth == lastSunday) return hours >= 1;
        if (dayOfMonth >  lastSunday) return true;
    }
    if (month >= 4 && month <= 9)     return true; //Apr, May, Jun, Jul, Aug, Sep
    if (month == 10)                               //Oct - DST false after 1am UTC on the last Sunday in October
    {
        if (dayOfMonth <  lastSunday) return true;
        if (dayOfMonth == lastSunday) return hours < 1;
        if (dayOfMonth >  lastSunday) return false;
    }
    if (month >= 11) return false;                  //Nov, Dec
    return false;
}
static void normalise(int* pHours, int* pDayOfWeek, int* pDayOfMonth, int* pMonth, int * pDayOfYear, int* pYear)
{
    if (*pHours > 23)
    {
        *pHours -= 24;
        ++*pDayOfWeek;
        if (*pDayOfWeek > 6) *pDayOfWeek = 0;
        ++*pDayOfMonth;
        if (*pDayOfMonth > monthLength(*pYear, *pMonth))
        {
            ++*pMonth;
            if (*pMonth > 12)
            {
                ++*pYear;
                *pDayOfYear = 1;
                *pMonth = 1;
            }
            *pDayOfMonth = 1;
        }
    }
    
    if (*pHours < 0)
    {
        *pHours += 24;
        --*pDayOfWeek;
        if (*pDayOfWeek < 0) *pDayOfWeek = 6;
        --*pDayOfMonth;
        if (*pDayOfMonth < 1)
        {
            --*pMonth;
            if (*pMonth < 1)
            {
                --*pYear;
                *pDayOfYear = isLeapYear(*pYear) ? 366 : 365;
                *pMonth = 12;
            }
            *pDayOfMonth = monthLength(*pYear, *pMonth);
        }
    }
}
static void addYears(int* pYear, int* pDayOfWeek, int* pDaysLeft)
{
    while(1)
    {
        //See if it is a leap year
        int leapYear = isLeapYear(*pYear);
        
        //Find the number of days in this year
        int daysInYear = leapYear ? 366 : 365;
        
        //Stop if this is the final year
        if (*pDaysLeft < daysInYear) break;
        
        //Calculate the current day of the week at the start of the year
        *pDayOfWeek += leapYear ? 2 : 1;
        if (*pDayOfWeek >= 7) *pDayOfWeek -= 7;
        
        //Move on to the next year
        *pDaysLeft -= daysInYear;
        ++*pYear;
    }
}
static void addMonths(int year, int* pMonth, int* pDaysLeft)
{
    while(1)
    {
        int daysInMonth = monthLength(year, *pMonth);
        
        //Stop if this is the last month
        if (*pDaysLeft < daysInMonth) break;
        
        //Move onto next month
        *pDaysLeft -= daysInMonth;
        ++*pMonth;
    }
}
static void timeToTm(time_t time, struct tm* ptm, int local)
{
    int seconds  = time % 60; time /= 60;
    int minutes  = time % 60; time /= 60;
    int hours    = time % 24; time /= 24;
    int daysLeft = time;
    
    //Add a year at a time while there is more than a year of days left
    int year      = 1970; //Unix epoch is 1970
    int dayOfWeek = 4;    //1 Jan 1970 is a Thursday
    addYears(&year, &dayOfWeek, &daysLeft);
    
    //Days left contains the days left from the start (1 Jan) of the current year
    int dayOfYear = daysLeft + 1;
    dayOfWeek += daysLeft;
    dayOfWeek %= 7;

    //Add a month at a time while there is more than a month of days left
    int month = 1;
    addMonths(year, &month, &daysLeft);
    
    //Days left contains the days left from the start (1st) of the current month
    int dayOfMonth = daysLeft + 1;
          
    //Deal with local time offsets
    int dst;
    if (local)
    {
        //Work out if Daylight Saving Time applies
        dst = isDst(year, month, dayOfMonth, dayOfWeek, hours);
        
        //Adjust for the timezone
        hours += dst ? DST_OFFSET : STD_OFFSET;
        normalise(&hours, &dayOfWeek, &dayOfMonth, &month, &dayOfYear, &year);
    }
    else
    {
        dst = false;
    }
    
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

void TimeToTmUtc(time_t time, struct tm* ptm)
{
    timeToTm(time, ptm, false);
}

void TimeToTmLocal(time_t time, struct tm* ptm)
{
    timeToTm(time, ptm, true);
}
