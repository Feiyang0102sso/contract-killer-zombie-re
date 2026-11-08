// CStdUtil_Cocoa.cpp - iOS standard utilities: wall clock / uptime, wide string compare, printf forwarding.
#include "CStdUtil_Cocoa.h"

#include <ctype.h>
#include <sys/time.h>
#include <mach/mach_time.h>

#include "bvsprintf.h"

long CStdUtil_Cocoa::GetTimeSeconds()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec;
}

int CStdUtil_Cocoa::GetUpTimeMS()
{
    static uint64_t start = 0;
    static mach_timebase_info_data_t sTimebaseInfo;

    if (start == 0) {
        start = mach_absolute_time();
        mach_timebase_info(&sTimebaseInfo);
    }

    uint64_t elapsed = mach_absolute_time() - start;

    // Large values: divide first so the 64-bit product cannot overflow
    if (sTimebaseInfo.numer > 1000000 && elapsed > 1000000) {
        return (elapsed / 1000) * (sTimebaseInfo.numer / 1000) / sTimebaseInfo.denom;
    }
    return elapsed * sTimebaseInfo.numer / ((uint64_t)sTimebaseInfo.denom * 1000000);
}

int CStdUtil_Cocoa::WcsICmp(const wchar_t* left, const wchar_t* right)
{
    while (*left != 0 && *right != 0) {
        // Only ASCII letters are folded
        wchar_t leftChar = *left;
        if (isascii(leftChar) && isupper(leftChar)) {
            leftChar = tolower(leftChar);
        }
        wchar_t rightChar = *right;
        if (isascii(rightChar) && isupper(rightChar)) {
            rightChar = tolower(rightChar);
        }
        if (leftChar != rightChar) {
            if (leftChar > rightChar) {
                return 1;
            }
            return -1;
        }
        left++;
        right++;
    }

    if (*left == *right) {
        return 0;
    }
    if (*left > *right) {
        return 1;
    }
    return -1;
}

int CStdUtil_Cocoa::VSPrintF(char* buffer, const char* format, void* arguments)
{
    return bvsprintf(buffer, format, (va_list)arguments);
}

int CStdUtil_Cocoa::VSPrintF_S(char* buffer, int bufferSize, const char* format, void* arguments)
{
    return bvsprintf_s(buffer, bufferSize, format, (va_list)arguments);
}

int CStdUtil_Cocoa::VSWPrintF(wchar_t* buffer, const wchar_t* format, void* arguments)
{
    return bwsprintf(buffer, format, (va_list)arguments);
}
