#include "utils.h"

namespace vcamshare {
    uint8_t *searchH264Head(uint8_t * const data, int max) {
        uint8_t *rs = data;
        while(rs[0] != 0 
                || rs[1] != 0
                || rs[2] != 0
                || rs[3] != 1) {

            rs ++;
            max --;

            if(max == 0) {
                return nullptr;
            }
        }
        return rs;
    }

    bool isNonIDR(uint8_t * const data) {
        return (data[4] & 0x1f) == 1;
    }
}
