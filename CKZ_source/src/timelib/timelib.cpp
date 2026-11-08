// timelib.cpp - unix timestamp to broken down GMT time (from PHP's timelib_unixtime2gmt).
#include "timelib.h"

#define SECS_PER_DAY    86400
#define DAYS_PER_YEAR   365
#define DAYS_PER_LYEAR  366

#define timelib_is_leap(y) ((y) % 4 == 0 && ((y) % 100 != 0 || (y) % 400 == 0))

// Day of the year before the first of each month (leap table shifted by one, see below).
static const int month_tab_leap[12] = { -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
static const int month_tab[12] =      {  0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };

void timelib_time2struc(timelib_time* tm, timelib_sll ts)
{
    timelib_sll days, remainder, tmp_days;
    timelib_sll cur_year = 1970;
    timelib_sll i;
    timelib_sll hours, minutes, seconds;
    const int* months;

    days = ts / SECS_PER_DAY;
    remainder = ts - (days * SECS_PER_DAY);
    if (ts < 0 && remainder == 0) {
        days++;
        remainder -= SECS_PER_DAY;
    }

    if (ts >= 0) {
        tmp_days = days + 1;
        while (tmp_days >= DAYS_PER_LYEAR) {
            cur_year++;
            if (timelib_is_leap(cur_year)) {
                tmp_days -= DAYS_PER_LYEAR;
            } else {
                tmp_days -= DAYS_PER_YEAR;
            }
        }
    } else {
        tmp_days = days;

        while (tmp_days <= 0) {
            if (tmp_days < -1460970) {
                // 4000 years at once
                cur_year -= 4000;
                tmp_days += 1460970;
            } else {
                cur_year--;
                if (timelib_is_leap(cur_year)) {
                    tmp_days += DAYS_PER_LYEAR;
                } else {
                    tmp_days += DAYS_PER_YEAR;
                }
            }
        }
        remainder += SECS_PER_DAY;
    }

    if (timelib_is_leap(cur_year)) {
        months = month_tab_leap;
    } else {
        months = month_tab;
    }
    if (timelib_is_leap(cur_year) && cur_year < 1970) {
        tmp_days--;
    }
    i = 11;
    while (i > 0) {
        if (tmp_days > months[i]) {
            break;
        }
        i--;
    }

    // That was the date, now the time
    hours = remainder / 3600;
    minutes = (remainder - hours * 3600) / 60;
    seconds = remainder % 60;

    tm->y = cur_year;
    tm->m = i + 1;
    tm->d = tmp_days - months[i];
    tm->h = hours;
    tm->i = minutes;
    tm->s = seconds;
    tm->z = 0;
    tm->dst = 0;
    tm->sse = ts;
    tm->sse_uptodate = 1;
    tm->tim_uptodate = 1;
    tm->is_localtime = 0;
}
