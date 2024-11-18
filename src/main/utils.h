#ifndef VXMT_VCAM_SHARE_UTILS
#define VXMT_VCAM_SHARE_UTILS

#include <stdint.h>

namespace vcamshare {
    uint8_t *searchH264Head(uint8_t * const data, int max);
    bool isNonIDR(uint8_t * const data);
}

#endif