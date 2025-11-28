#include "rtc.h"
#include "port.h"

unsigned char rtc_read(unsigned char reg) {
    outb(0x70, reg);
    return inb(0x71);
}

void rtc_get_time(int *hour, int *min, int *sec) {
    *sec  = rtc_read(0x00);
    *min  = rtc_read(0x02);
    *hour = rtc_read(0x04);

    // BCD → Binär
    *sec  = (*sec & 0x0F) + ((*sec / 16) * 10);
    *min  = (*min & 0x0F) + ((*min / 16) * 10);
    *hour = (*hour & 0x0F) + ((*hour / 16) * 10);
}
