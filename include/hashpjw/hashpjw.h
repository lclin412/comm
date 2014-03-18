
#ifndef HASH_PJW_H
#define HASH_PJW_H

#include <stdint.h>

inline uint32_t hashpjw(const char *psKey, uint32_t ulKeyLength)
{       
    uint32_t h = 0, g;
    const char *pEnd = psKey + ulKeyLength;
    while(psKey < pEnd)
    {   
        h = (h << 4) + *psKey++;
        if ((g = (h & 0xF0000000)))
        {
            h = h ^ (g >> 24);
            h = h ^ g;
        }
    }
    return h;
}    

#endif /* HASH_PJW_H */
