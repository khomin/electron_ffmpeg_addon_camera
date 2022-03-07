#ifndef VIDEO_STATS_H
#define VIDEO_STATS_H

#include <stdint.h>

struct VideStats
{
    bool is_active;
    uint32_t packet_cnt;
    uint32_t err_cnt;
};

#endif // VIDEO_STATS_H
