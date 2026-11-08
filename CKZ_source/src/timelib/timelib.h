// timelib.h - unix timestamp to broken down time, trimmed down from PHP's timelib
// (timelib_unixtime2gmt / timelib_unixtime2local). Struct layout from the field offsets
// written by timelib_time2struc (64-bit members are 4-byte aligned on iOS armv7).
#ifndef TIMELIB_H
#define TIMELIB_H

typedef long long timelib_sll;

struct timelib_tzinfo;

/** Relative time (unused here, kept for the layout of timelib_time). */
struct timelib_rel_time {
    timelib_sll y, m, d;                // 0x00 years, months, days
    timelib_sll h, i, s;                // 0x18 hours, minutes, seconds
    int weekday;                        // 0x30
    int weekday_behavior;               // 0x34
    struct {
        unsigned int type;
        timelib_sll amount;
    } special;                          // 0x38
    unsigned int have_weekday_relative; // 0x44
    unsigned int have_special_relative; // 0x48
    int first_last_day_of;              // 0x4c
    int invert;                         // 0x50
    timelib_sll days;                   // 0x54
};

struct timelib_time {
    timelib_sll y, m, d;                // 0x00 year, month, day
    timelib_sll h, i, s;                // 0x18 hour, minute, second
    double f;                           // 0x30 fraction
    int z;                              // 0x38 GMT offset in minutes
    char* tz_abbr;                      // 0x3c
    timelib_tzinfo* tz_info;            // 0x40
    signed int dst;                     // 0x44
    timelib_rel_time relative;          // 0x48
    timelib_sll sse;                    // 0xa4 seconds since epoch
    unsigned int have_time;             // 0xac
    unsigned int have_date;             // 0xb0
    unsigned int have_zone;             // 0xb4
    unsigned int have_relative;         // 0xb8
    unsigned int have_weeknr_day;       // 0xbc
    unsigned int sse_uptodate;          // 0xc0
    unsigned int tim_uptodate;          // 0xc4
    unsigned int is_localtime;          // 0xc8
    unsigned int zone_type;             // 0xcc
};

/** Fill `tm` with the GMT date and time of the unix timestamp `ts`. */
void timelib_time2struc(timelib_time* tm, timelib_sll ts);

/** Fill `tm` with the local (device time zone) date and time of the unix timestamp `ts`. */
void timelib_time2local(timelib_time* tm, timelib_sll ts);

#endif
