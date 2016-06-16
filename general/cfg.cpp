#include "mbed.h"
#include "log.h"
#define MAX_NAME_OR_VALUE 100
#define FILE_NAME "/local/cfg.txt"


int    CfgBaud;
char  *CfgSsid;
char  *CfgPassword;
char  *CfgNtpIp;

int    CfgClockSetInterval;
int    CfgClockSetRetryInterval;
int    CfgClockOffsetMs;
int    CfgClockCalibration;

char   CfgTankRom[8];
char   CfgInletRom[8];

static void saveString(char *value, char  **dest) {
    *dest = (char*)realloc(*dest, strlen(value) + 1); //strlen does not include the null so add 1
    if (*dest == NULL)
    {
        LogCrLf("Error reallocating memory in saveString");
        return;
    }
    *dest = strcpy(*dest, value);
}
static void saveInt   (char *value, int    *dest) {
    *dest = atoi(value);
}
static void saveRom (char *value, char* dest) {
    char* p = value;
    for (int i = 0; i < 8; i++) dest[i] = (char)strtoul(p, &p, 16);
}
static void rtrim     (int i, char *s) { //i is the length of the thing to trim
    while(1)
    {
        s[i] = '\0';
        if (--i < 0) break;
        if (s[i] != ' ' && s[i] != '\t') break;
    }
}

static void handleLine(int n, int v, char *name, char *value)
{
    rtrim(n, name);
    rtrim(v, value);
    if (strcmp(name, "baud"                    ) == 0) saveInt   (value, &CfgBaud);
    if (strcmp(name, "ssid"                    ) == 0) saveString(value, &CfgSsid);
    if (strcmp(name, "password"                ) == 0) saveString(value, &CfgPassword);
    if (strcmp(name, "ntp ip"                  ) == 0) saveString(value, &CfgNtpIp);
    if (strcmp(name, "clock set interval"      ) == 0) saveInt   (value, &CfgClockSetInterval);
    if (strcmp(name, "clock set retry interval") == 0) saveInt   (value, &CfgClockSetRetryInterval);
    if (strcmp(name, "clock offset ms"         ) == 0) saveInt   (value, &CfgClockOffsetMs);
    if (strcmp(name, "clock calibration"       ) == 0) saveInt   (value, &CfgClockCalibration);
    if (strcmp(name, "tank"                    ) == 0) saveRom   (value,  CfgTankRom);
    if (strcmp(name, "inlet"                   ) == 0) saveRom   (value,  CfgInletRom);
}
static void resetValues(void)
{
    
                       CfgBaud       = 0;
    free(CfgSsid);     CfgSsid       = NULL;
    free(CfgPassword); CfgPassword   = NULL;
    free(CfgNtpIp);    CfgNtpIp      = NULL;
                       CfgClockSetInterval      = 0;
                       CfgClockSetRetryInterval = 0;
                       CfgClockOffsetMs         = 0;
                       CfgClockCalibration      = 0;
                       memset(CfgTankRom,  0, 8);
                       memset(CfgInletRom, 0, 8);
}
int CfgInit()   {

    FILE *fp = fopen(FILE_NAME, "r");
    if (fp == NULL)
    {
        LogF("Error opening file %s for reading", FILE_NAME);
        return -1;
    }
    resetValues();
    char  name[MAX_NAME_OR_VALUE];
    char value[MAX_NAME_OR_VALUE];
    int isName  = 1;
    int isValue = 0;
    int isStart = 0; //Used to trim starts
    int n = 0;
    int v = 0;
    while (1)
    {
        int c = fgetc(fp);
        if (c == '\r') continue; //Ignore windows <CR> characters
        if (c == EOF)  { handleLine(n, v, name, value); break;}
        if (c == '\0') { handleLine(n, v, name, value); break;}
        if (c == '\n') { handleLine(n, v, name, value); isName = 1; isValue = 0; isStart = 0; n = 0; v = 0; continue;}
        if (c == '=' && isName) {                       isName = 0; isValue = 1; isStart = 0;               continue;}
        if (c == '#')  {                                isName = 0; isValue = 0;                            continue;}
        if (c != ' ' && c != '\t') isStart = -1;
        if (isName  && isStart && n < MAX_NAME_OR_VALUE - 1)  name[n++] = (char)c; //n can never exceed MAX_NAME_OR_VALUE - 1 leaving room for a null at length n
        if (isValue && isStart && v < MAX_NAME_OR_VALUE - 1) value[v++] = (char)c; //v can never exceed MAX_NAME_OR_VALUE - 1 leaving room for a null at length v
    }
        
    fclose(fp);
        
    return 0;
}
