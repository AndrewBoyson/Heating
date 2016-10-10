#include "mbed.h"

#define STD_OFFSET 0
#define DST_OFFSET 1

static bool isLeapYear(int year)
{
    year += 1900;
    bool leapYear = !(year & 0x3);
    if (year >= 2100)
    {
        if (year % 100 == 0) leapYear = false;
        if (year % 400 == 0) leapYear =  true;
    }
    return leapYear;

}
static int monthLength(int year, int month)
{
    static char monthlengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int daysInMonth = monthlengths[month];
    if (month == 1 && isLeapYear(year)) daysInMonth++; //February is month 1 of months 0 to 11
    return daysInMonth;
}
static int isDst(int year, int month, int dayOfMonth, int dayOfWeek, int hours)
{
    //Find the last Sunday in the month
    int lastDayOfMonth = monthLength(year, month);
    int daysToEndOfMonth = lastDayOfMonth - dayOfMonth;
    int dayOfWeekOfLastDayOfMonth = (dayOfWeek + daysToEndOfMonth) % 7;
    int lastSundayDayOfMonth = lastDayOfMonth - dayOfWeekOfLastDayOfMonth;

    //Check each month
    if (month <= 1) return false;                  //Jan, Feb
    if (month == 2)                                //Mar - DST true after 1am UTC on the last Sunday in March
    {
        if (dayOfMonth <  lastSundayDayOfMonth) return false;
        if (dayOfMonth == lastSundayDayOfMonth) return hours >= 1;
        if (dayOfMonth >  lastSundayDayOfMonth) return true;
    }
    if (month >= 3 && month <= 8)     return true; //Apr, May, Jun, Jul, Aug, Sep
    if (month == 9)                                //Oct - DST false after 1am UTC on the last Sunday in October
    {
        if (dayOfMonth <  lastSundayDayOfMonth) return true;
        if (dayOfMonth == lastSundayDayOfMonth) return hours < 1;
        if (dayOfMonth >  lastSundayDayOfMonth) return false;
    }
    if (month >= 10) return false;                  //Nov, Dec
    return false;
}
static void calculateDayOfYearAndWeek(int thisYear, int thisMonth, int thisMonthDay, int* pDayOfYear, int* pDayOfWeek)
{
    int dayOfYear = 0;    //1 Jan is day 0
    int dayOfWeek = 4;    //1 Jan 1970 is a Thursday
    
    //Add days of each whole year
    for (int y = 70; y < thisYear; y++)
    {
        int lengthOfYear = isLeapYear(y) ? 366 : 365;
        dayOfWeek += lengthOfYear;
    }
    
    //Add days of each whole month
    for (int m = 0; m < thisMonth; m++)
    {
        int lengthOfMonth = monthLength(thisYear, m);
        dayOfYear += lengthOfMonth;
        dayOfWeek += lengthOfMonth;
    }
    
    //Add days of part month
    thisMonthDay--; //thisMonthDay is 01 to 31 where we need 00 to 30
    dayOfYear += thisMonthDay;
    dayOfWeek += thisMonthDay;
    
    //Update the day of year and day of week parts of the struct tm
    *pDayOfYear = dayOfYear;     // 0 --> 365
    *pDayOfWeek = dayOfWeek % 7; // 0 --> 6
}
static void normalise(int* pHours, int* pDayOfWeek, int* pDayOfMonth, int* pMonth, int * pDayOfYear, int* pYear)
{
    if (*pHours > 23)
    {
        *pHours -= 24;
        ++*pDayOfWeek;
        if (*pDayOfWeek > 6) *pDayOfWeek = 0;
        ++*pDayOfYear;
        ++*pDayOfMonth;
        if (*pDayOfMonth > monthLength(*pYear, *pMonth))
        {
            ++*pMonth;
            if (*pMonth > 11)
            {
                ++*pYear;
                *pDayOfYear = 0;
                *pMonth = 0;
            }
            *pDayOfMonth = 1;
        }
    }
    
    if (*pHours < 0)
    {
        *pHours += 24;
        --*pDayOfWeek;
        if (*pDayOfWeek < 0) *pDayOfWeek = 6;
        --*pDayOfYear;
        --*pDayOfMonth;
        if (*pDayOfMonth < 1)
        {
            --*pMonth;
            if (*pMonth < 0)
            {
                --*pYear;
                *pDayOfYear = isLeapYear(*pYear) ? 365 : 364;
                *pMonth = 11;
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
static void timeToTm(time_t t, struct tm* ptm, int local)
{
    //Extract the seconds, minutes, hours and days from the time_t t
    div_t divres;
    divres = div(          t, 60);    int seconds  = divres.rem;
    divres = div(divres.quot, 60);    int minutes  = divres.rem;
    divres = div(divres.quot, 24);    int hours    = divres.rem;
                                      int daysLeft = divres.quot;
    
    //Add a year at a time while there is more than a year of days left
    int year      = 70; //Unix epoch is 1970
    int dayOfWeek = 4;  //1 Jan 1970 is a Thursday
    addYears(&year, &dayOfWeek, &daysLeft);
    
    //Days left contains the days left from the start (1 Jan) of the current year
    int dayOfYear = daysLeft;
    dayOfWeek += daysLeft;
    dayOfWeek %= 7;

    //Add a month at a time while there is more than a month of days left
    int month = 0;
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
        dst = -1;
    }
    
    //Set up the broken time TM structure
    ptm->tm_sec   = seconds;       // 00 --> 59
    ptm->tm_min   = minutes;       // 00 --> 59
    ptm->tm_hour  = hours;         // 00 --> 23
    ptm->tm_mday  = dayOfMonth;    // 01 --> 31
    ptm->tm_mon   = month;         // 00 --> 11
    ptm->tm_year  = year;          // Years since 1900
    ptm->tm_wday  = dayOfWeek;     // 0 --> 6 where 0 == Sunday
    ptm->tm_yday  = dayOfYear;     // 0 --> 365
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
time_t TimeFromTmUtc(struct tm* ptm)
{
    int days = 0;
    
    for (int y = 70; y < ptm->tm_year; y++) days += isLeapYear(y) ? 366 : 365;
    
    days += ptm->tm_yday;
    
    return days         * 86400 + 
           ptm->tm_hour *  3600 + 
           ptm->tm_min  *    60 + 
           ptm->tm_sec;
}
int TimePeriodBetween(struct tm* ptmLater, struct tm* ptmEarlier)
{
    int days = 0;
    
    if (ptmLater->tm_year > ptmEarlier->tm_year) for (int y = ptmEarlier->tm_year; y < ptmLater->tm_year; y++) days += isLeapYear(y) ? 366 : 365;
    else                                         for (int y = ptmEarlier->tm_year; y > ptmLater->tm_year; y--) days -= isLeapYear(y) ? 366 : 365;
    
    days += ptmLater->tm_yday - ptmEarlier->tm_yday;
    
    return  days                                     * 86400 + 
           (ptmLater->tm_hour - ptmEarlier->tm_hour) *  3600 + 
           (ptmLater->tm_min  - ptmEarlier->tm_min ) *    60 + 
           (ptmLater->tm_sec  - ptmEarlier->tm_sec );
}

void TimeTmUtcToLocal(struct tm* ptm)
{    
    //Establish DST
    ptm->tm_isdst = isDst(ptm->tm_year, ptm->tm_mon, ptm->tm_mday, ptm->tm_wday, ptm->tm_hour);
        
    //Adjust for the timezone
    ptm->tm_hour += ptm->tm_isdst ? DST_OFFSET : STD_OFFSET;
    normalise(&ptm->tm_hour, &ptm->tm_wday, &ptm->tm_mday, &ptm->tm_mon, &ptm->tm_yday, &ptm->tm_year);
}

void TimeAsciiDateTimeToTm(const char* pDate, const char* pTime, struct tm* ptm) // Convert compile time to system time 
{
    //__DATE__ The string constant contains eleven characters and looks like "Feb 12 1996". If the day of the month is less than 10, it is padded with a space on the left. 
    char month[5];
    sscanf(pDate, "%s %d %d", month, &ptm->tm_mday, &ptm->tm_year); ptm->tm_year -= 1900;
    
    // Find where month is in month_names. Deduce month value. 
    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    ptm->tm_mon = (strstr(month_names, month) - month_names) / 3;
        
    //__TIME__ The string constant contains eight characters and looks like "23:59:01".     
    sscanf(pTime, "%2d %*c %2d %*c %2d", &ptm->tm_hour, &ptm->tm_min, &ptm->tm_sec);

    //Fill the day of week and the day of year part of the tm structure
    calculateDayOfYearAndWeek(ptm->tm_year, ptm->tm_mon, ptm->tm_mday, &ptm->tm_yday, &ptm->tm_wday);
}
