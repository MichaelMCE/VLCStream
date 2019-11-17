

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>




// 32 bit magic FNV-1a prime
// Fowler/Noll/Vo hash
#define FNV_32_PRIME ((unsigned int)0x01000193)
#define FNV1_32_INIT ((unsigned int)0x811c9dc5)

static inline unsigned int fnv_32a_buf (const void *buf, const size_t len, unsigned int hval)
{
    unsigned char *restrict bp = (unsigned char*)buf;	/* start of buffer */
    unsigned char *restrict be = bp + len;				/* beyond end of buffer */

    /*
     * FNV-1a hash each octet in the buffer
     */
    while (bp < be){
		/* xor the bottom with the current octet */
		hval ^= (unsigned int)*bp++;

		/* multiply by the 32 bit FNV magic prime mod 2^32 */
#if defined(NO_FNV_GCC_OPTIMIZATION)
		hval *= FNV_32_PRIME;
#else
		hval += (hval<<1) + (hval<<4) + (hval<<7) + (hval<<8) + (hval<<24);
#endif
    }

    /* return our new hash value */
    return hval;
}

unsigned int generateHash (const void *data, const size_t dlen)
{
	if (!dlen)
		return 0;
	else
		return fnv_32a_buf(data, dlen, FNV_32_PRIME);
}




