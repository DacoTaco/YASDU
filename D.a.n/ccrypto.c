/* AES-XTS by DacoTaco , altered version from @luigoalma, based on @plutooo's gist*/

#include <stdbool.h>

#define ECB 1
#define CBC 0
#define CTR 0

#include "aes.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

union {
    u16 foo;
    u8 islittle;
} endian = {.foo = 1};

union bigint128 {
    u8 value8[16];
    u64 value64[2];
};

inline static union bigint128 geniv(u64 poshi, u64 poslo) {
    union bigint128 out = {0};
    if (endian.islittle) {
        u8 *foo = (u8 *) &poslo;
        int i;
        for (i = 0; i < sizeof(u64); i++) out.value8[15 - i] = foo[i];
        foo = (u8 *) &poshi;
        for (i = 0; i < sizeof(u64); i++) out.value8[7 - i] = foo[i];
    } else {
        out.value64[1] = poslo;
        out.value64[0] = poshi;
    }
    return out;
}

inline static void xor128(u8 *foo, u8 *bar) {
    int i;
    for (i = 0; i < 16; i++) foo[i] ^= bar[i];
}

inline static void shift128(u8 *foo) {
    int i;
    for (i = 15; i >= 0; i--) {
        if (i != 0) foo[i] = (foo[i] << 1) | (foo[i - 1] >> 7);
        else foo[i] = (foo[i] << 1);
    }
}

void aes_xtsn_process(u8 direction, u8* buffer, u64 len, u8 *key, u8 *tweakin, u64 sectoroffsethi, u64 sectoroffsetlo, u32 sector_size, u64 sectoroffset) 
{
	if(len == 0)
		return;
	if(sectoroffset > sector_size)
		sectoroffset = 0;
	
    u64 i;	
    struct AES_ctx _key, _tweak;
    AES_init_ctx(&_key, key);
    AES_init_ctx(&_tweak, tweakin);
    u64 position[2] = {sectoroffsethi, sectoroffsetlo};
	u64 sectorsToRead = (len < sector_size) ? 1 : (len / (u64)sector_size);
	
    for (i = 0; i < sectorsToRead; i++) 
	{
        if (position[1] > (position[1] + 1LLU)) 
			position[0] += 1LLU; //if overflow, we gotta
        union bigint128 tweak = geniv(position[0], position[1]);
        AES_ECB_encrypt(&_tweak, tweak.value8);
		u64 j = 0;
		
		while( ( j < len / 0x10))// && ( j < sector_size / 0x10 ) )
		{
			//decrypt if we aren't at a skipped block
			if(sectoroffset/0x10 > j)
			{
				if(sectoroffset > 0x10)
					sectoroffset -= 0x10;
				else
					sectoroffset = 0;
			}
			else //encrypt or decrypt !
			{
				xor128(buffer, tweak.value8);
				if(direction)
					AES_ECB_encrypt(&_key, buffer);
				else
					AES_ECB_decrypt(&_key, buffer);
				xor128(buffer, tweak.value8);
				buffer += 0x10;			
				j++;
			}				
			
			//set the tweak value for the next 0x10 block
			bool flag = tweak.value8[15] & 0x80;
            shift128(tweak.value8);
            if (flag) 
				tweak.value8[0] ^= 0x87;
			
			//end of a sector. this can only work if we decrypted from address 0 of a sector though
			//kinda unreliable , so commented
			/*if(j > 0 && j%(sector_size / 0x10) == 0)
			{
				//printf("end of sector : %d\r\n",j);
				break;
			}*/
		}
        position[1] += 1LLU;
    }
}

void aes_xtsn_decrypt(u8 *buffer, u64 len, u8 *key, u8 *tweakin, u64 sectoroffsethi, u64 sectoroffsetlo, u32 sector_size, u64 sectoroffset) 
{
	return aes_xtsn_process(0,buffer,len,key,tweakin,sectoroffsethi,sectoroffsetlo,sector_size,sectoroffset);
}

void aes_xtsn_encrypt(u8 *buffer, u64 len, u8 *key, u8 *tweakin, u64 sectoroffsethi, u64 sectoroffsetlo, u32 sector_size, u64 sectoroffset) 
{
	return aes_xtsn_process(1,buffer,len,key,tweakin,sectoroffsethi,sectoroffsetlo,sector_size,sectoroffset);
}