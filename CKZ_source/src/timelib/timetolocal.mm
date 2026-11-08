// timetolocal.mm - unix timestamp to broken down local time, using the device time zone.
#import <Foundation/Foundation.h>
#include "timelib.h"

void timelib_time2local(timelib_time* tm, timelib_sll ts)
{
    timelib_time2struc(tm, ts + [[NSTimeZone localTimeZone] secondsFromGMT]);
    tm->is_localtime = 1;
    tm->have_zone = 1;
}
