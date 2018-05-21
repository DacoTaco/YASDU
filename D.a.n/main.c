/* D.a.n - Driver to access Switch's encrypted partition
	major thanks to 
	https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os
	and
	https://github.com/euspectre/kedr/blob/master/sources/examples/sample_target/
	for their sample code & explanation. they explain it so well how modules work
	AND 
	tpw_rules for his FUSE code to look at. made alot more sence when i was looking at your code man
	
	Copyright (C) 2016-2017  DacoTaco
	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation version 2.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "driver.h"

char *DEVICE_NAME[] = {"/p1_PRODINFO","/p2_PRODINFOF",
					   "/p9_SAFE","/p10_SYSTEM",
					   "/p11_USER","/BUFF_OVERFLOW_DETECTED"};

//we should read these from the GPT but... eh
size_t sizes[]		= { 0x003FBC00,0x00400000,
					    0x04000000,0xA0000000,
					    0x680000000 };

char file_exists(const char* filename)
{
	FILE* fp = fopen(filename,"r");
	
	if(fp)
	{
		fclose(fp);
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[]) 
{
	
	//first load in the keys
	FILE* fp = fopen("biskeydump.txt","r");
	
	if(fp == NULL)
	{
		printf("unable to open the biskey dump file!\n\r");
		return -EFAULT;
	}
	
	char* line;
	size_t len = 0;
	int key_nr = 0;
	while(getline(&line,&len,fp) >= 0)
	{
		char crypt[33];
		char tweak[33];
		
		crypt[32] = 0;
		tweak[32] = 0;
		
		//printf("%s\n",line);
		if(strncmp(line,"BIS KEY ",8) == 0)
		{
			char* split = line;
			for(int i = 0;i <= 1;i++)
			{
				split = split+10-i;
				strncpy(crypt,split,32);
			}
			
			if(getline(&line,&len,fp) >= 0)
			{
				char* split = line;
				for(int i = 0;i <= 1;i++)
				{
					split = split+10-i;
					strncpy(tweak,split,32);
				}
			}
			
			printf("crypt(%d) : %s\n\r",key_nr,crypt);
			printf("tweak(%d) : %s\n\r",key_nr,tweak);
			
			//convert keys
			for(unsigned int y = 0;y*2 < strlen(crypt);y++)
			{
				char number[3] = { 0 };
				memcpy(number,crypt+(y*2),2);
				state[key_nr].crypt_key[y] = (unsigned char)strtol(number,NULL,16);

				memset(number,0,3);
				memcpy(number,tweak+(y*2),2);
				state[key_nr].tweak_key[y] = (unsigned char)strtol(number,NULL,16);
			}
			
			//the same key gets used for the first 2 partitions. hence a memcpy
			if(key_nr == 0)
			{
				memcpy(state[key_nr+1].crypt_key,state[key_nr].crypt_key,KEY_SIZE);
				memcpy(state[key_nr+1].tweak_key,state[key_nr].tweak_key,KEY_SIZE);
				key_nr++;
			}
			key_nr++;
			
			//we read all keys
			if(key_nr >= PARTITION_COUNT)
				break;
		}
	}
	fclose(fp);
	
	if(key_nr < 3)
	{
		printf("did not get all keys!");
		return -EFAULT;
	}
	
	//set up device
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		state[i].name = DEVICE_NAME[i];
		state[i].partition_size = sizes[i];
		
		char *path = malloc(32);
		if(path == NULL)
		{
			printf("failed to alloc memory for path\n\r");
			return -ENOMEM;
		}
		char part = 0;
		switch(i)
		{
			case 0:
			case 1:
				part = i+1;
				break;
			case 2:
			case 3:
			case 4:
				part = i+7;
				break;
		}
		// p1,p2,p9,p10,p11
		snprintf(path,32,"./mmcblk1p%d.bin",part);
		if(file_exists(path))
		{
			printf("%s added as -> %s \n\r",path,DEVICE_NAME[i]);
			state[i].file_path = path;
			state[i].report = 1;
		}
		else
		{
			state[i].report = 0;	
		}
		//init lock
		pthread_mutex_init(&state[i].lock, NULL);
	}
	
	//and setup fuse device :')
	printf("LEGGO\n\r");
	
#if FUSE_USE_VERSION < 26
	return fuse_main(argc, argv, &nand_oper);
#else
	return fuse_main(argc, argv, &nand_oper,&state);
#endif
	
}