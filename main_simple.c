
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"


#include "lz4.h"

enum {
    MESSAGE_MAX_BYTES   = 8192,
    RING_BUFFER_BYTES   = 4 * 256 + MESSAGE_MAX_BYTES,
    DICT_BYTES          = 8192,
};


char raw_buff[RING_BUFFER_BYTES];
int raw_offset=0;

char cmp_buff[LZ4_COMPRESSBOUND(MESSAGE_MAX_BYTES)];
int cmp_offset=0;

static uint32_t x=5238, y=234795, z=125805, w=165469;

static uint32_t xorshift128(void) {
    uint32_t t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = w ^ (w >> 19) ^ t ^ (t >> 8);
}

void test_compress()
{
    LZ4_stream_t lz4Stream_body = { 0 };
    LZ4_stream_t* lz4Stream = &lz4Stream_body;

    int i =0;

    SysCtlClockSet(SYSCTL_SYSDIV_4 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_INT);

    for (i=0; i<RING_BUFFER_BYTES; i++) {
    	raw_buff[i] = /*i;*/ xorshift128();
    }


    LZ4_resetStream(lz4Stream);

    for(i=0;;i++) {

        char* const rawPtr = &raw_buff[raw_offset];
    	//const int randomLength = (125) + 1;
    	const int randomLength = (xorshift128() % MESSAGE_MAX_BYTES) + 1;

    	if (randomLength > RING_BUFFER_BYTES - raw_offset) break;

        {
            const int cmpBytes = LZ4_compress_continue(lz4Stream,
            										   rawPtr,
													   cmp_buff+cmp_offset+sizeof(int),
													   randomLength);

            if(cmpBytes <= 0) break;

            memcpy(cmp_buff+cmp_offset, &cmpBytes, sizeof(int));

            cmp_offset += cmpBytes;


            raw_offset += randomLength;

            // Wraparound the ringbuffer offset
            //if(inpOffset >= RING_BUFFER_BYTES - MESSAGE_MAX_BYTES) inpOffset = 0;
            //if (raw_offset >= RING_BUFFER_BYTES - MESSAGE_MAX_BYTES) raw_offset = 0; /* Don't need this since raw is already populated */
        }
    }

    __asm("    nop\n");


}

int main_simple(void) {
	
	test_compress();

	while (1) {}

	return 0;
}
