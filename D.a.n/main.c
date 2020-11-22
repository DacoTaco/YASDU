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
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mount.h>
#include "driver.h"
#include "mount.h"

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

int getPartitionInfo(char* partition, partition_info** info)
{
	if(partition == NULL || info == NULL)
		return -1;
	
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		if(state.userPartitions[i].partition.name == NULL )
			continue;

		if (strcmp(partition, state.userPartitions[i].partition.name) == 0) 
		{	
			*info = &state.userPartitions[i].partition;
			return i;
		} 
	}		
	return -2;
}

void interrupt_handler(int dummy) 
{
	printf("INT HANDLER : SIGINT RECEIVED\n");
	
	//cleanup
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		file_info* partitionFile = &state.userPartitions[i];
		if(partitionFile->active <= 0 || partitionFile->mounted <= 0)
			continue;

		unMountDecryptedPartition(partitionFile->partition.name);			
	}
	unmount_fuse();
}

int readBisKeys()
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
		partition_info* info = NULL;
		char crypt[33];
		char tweak[33];

		crypt[32] = 0;
		tweak[32] = 0;

		//printf("%s\n",line);
		if(strncmp(line,"BIS KEY ",8) != 0)
			continue;
		
		//read crypt line
		char* split = line;
		for(int i = 0;i <= 1;i++)
		{
			split = split+10-i;
			strncpy(crypt,split,32);
		}

		//read tweak line
		if(getline(&line,&len,fp) < 0)
			break;
		split = line;
		for(int i = 0;i <= 1;i++)
		{
			split = split+10-i;
			strncpy(tweak,split,32);
		}

		printf("crypt(%d) : %s\n\r",key_nr,crypt);
		printf("tweak(%d) : %s\n\r",key_nr,tweak);

		switch(key_nr)
		{
			case 0:
			case 1:
				getPartitionInfo(PRODINFO.name, &info);
				break;
			case 2:
				getPartitionInfo(SAFE.name, &info);
				break;
			case 3:
				getPartitionInfo(SYSTEM.name, &info);
				break;
			case 4:
				getPartitionInfo(USER.name, &info);
				break;
			default:
				break;
		}

		if(info == NULL)
		{
			printf("unknown usage\r\n");
			key_nr++;
			continue;
		}

		//convert keys
		for(unsigned int y = 0;y*2 < strlen(crypt);y++)
		{
			char number[3] = { 0 };
			memcpy(number,crypt+(y*2),2);
			info->crypt_key[y] = (unsigned char)strtol(number,NULL,16);

			memset(number,0,3);
			memcpy(number,tweak+(y*2),2);
			info->tweak_key[y] = (unsigned char)strtol(number,NULL,16);
		}

		//the same key gets used for the first 2 partitions. hence a memcpy
		if(strcmp(info->name, PRODINFO.name) == 0 || strcmp(info->name, PRODINFOF.name) == 0)
		{
			partition_info* copy = NULL;
			char* copyName = NULL;

			if(strcmp(info->name, PRODINFO.name) == 0)
				copyName = PRODINFOF.name;
			else
				copyName = PRODINFO.name;

			if(getPartitionInfo(copyName, &copy) < 0 || copy == NULL)
			{
				printf("unknown partition\r\n");
				key_nr++;
				continue;
			}

			memcpy(copy->crypt_key,
				   info->crypt_key,
				   KEY_SIZE);

			memcpy(copy->tweak_key,
				   info->tweak_key,
				   KEY_SIZE);
			key_nr++;
		}
		key_nr++;

		//we read all keys
		if(key_nr >= PARTITION_COUNT)
			break;
	}
	
	fclose(fp);
	if(key_nr < 3)
	{
		printf("unable to retrieve all BIS keys!");
		return -EFAULT;
	}
	
	return 1;
}

int main(int argc, char *argv[]) 
{
	//loop trough all parameters and check if we have '-l' for local in there
	//we also setup the fuse arguments in the process
	const int fuse_arg_start = 2;
	int fuse_argc = fuse_arg_start;
	char* fuse_argv[fuse_argc+argc];
	char local_files = 0;
	char read_only = 1;

	for(int i = 0;i < argc;i++)
	{		
		if(strncmp(argv[i],"-l",2) == 0)
			local_files = 1;
		else if(strncmp(argv[i],"-rw",3) == 0)
			read_only = 0;
		else
		{
			printf("adding '%s' to fuse's arguments\r\n",argv[i]);
			fuse_argv[fuse_argc-fuse_arg_start] = argv[i];
			fuse_argc++;
		}

	}

	if(read_only)
	{
		printf("going in read only mode\r\n");
		fuse_argv[fuse_argc-fuse_arg_start] = "-o";
		fuse_argc++;
		fuse_argv[fuse_argc-fuse_arg_start] = "ro";
		fuse_argc++;
	}
	
	fuse_argv[fuse_argc-2] = "-o";
	fuse_argv[fuse_argc-1] = "allow_other";
	fuse_mount_args fuse_args;
	fuse_args.mountPoint = fuseMountPoint;
	fuse_args.argv = fuse_argv;
	fuse_args.argc = fuse_argc;

	//read bis keys
	int readKeys = readBisKeys();
	if(readKeys < 0)
		return 0;

	//set up device & look for files...
	printf("searching partitions");
	if(local_files > 0)
		printf(" using local files");
	printf("...\r\n");
	
	//check for the raw nand dump and then the partition files.
	char partitionsFound = 0;
	for(int i = -1;i < PARTITION_COUNT;i++)
	{		
		file_info* partitionFile = NULL;
		
		if(i < 0)
			partitionFile = &state.rawInfo;
		else
			partitionFile = &state.userPartitions[i];
			
		partitionFile->file_path = NULL;
		
		char* path = malloc(32);
		if(path == NULL)
		{
			printf("failed to alloc memory for path\n\r");
			return -ENOMEM;
		}
		
		// p1,p2,p9,p10,p11
		if(local_files == 1)
		{
			snprintf(path,32,"./%s",partitionFile->partition.name);
		}
		else
		{
			snprintf(path,32,"/dev/%s",partitionFile->partition.switch_name);
		}

		printf("searching for %s...\n\r",path);		
		if(state.rawInfo.active)
		{
			partitionsFound = 1;
			printf("found in NAND image %s\r\n", state.rawInfo.file_path);
			partitionFile->active = 1;
		}
		else if(file_exists(path))
		{
			printf("%s added as -> %s \n\r", path, partitionFile->partition.name);
			partitionFile->file_path = path;
			partitionFile->active = 1;
			partitionsFound = 1;
		}
		else
		{
			partitionFile->active = 0;	
		}
		
		//free memory if unused
		if(partitionFile->file_path == NULL)
			free(path);	
		
		//init lock
		pthread_mutex_init(&partitionFile->lock, NULL);
	}	
	
	if(!partitionsFound)
	{
		printf("no partitions found!\r\n");
		return 0;
	}

	//if the mounting point doesn't exists -> create it
	struct stat st = {0};
	if (stat(fuseMountPoint, &st) < 0 ) 
	{
		mkdir(fuseMountPoint, 0755);
	}
	
	//and setup fuse device :')
	printf("LEGGO\n\r");

	pthread_t fuseThread = 0;
	int err = pthread_create(&fuseThread, NULL, &mount_fuse, &fuse_args);
	if (err != 0)
		printf("can't create thread : %s\r\n", strerror(err));
	else
	{
		printf("Thread created successfully\r\n");
		usleep(500);
	}
	
	//wait for fuse to be done initialising
	while(fuseMountState == Init) { }
	
	void* ret = NULL;
	if(fuseMountState != Running)
	{
		printf("failed to initialise fuse \r\n");
		unmount_fuse();	
		pthread_join(fuseThread , &ret);
		pthread_cancel(fuseThread);
		exit(0);		
	}
	
	printf("fuse initialised!\r\n");
	printf("mounting decrypted partitions...\r\n");
	int mounted = 0;
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		file_info* partitionFile = &state.userPartitions[i];
		if(partitionFile->active <= 0 || partitionFile->mounted > 0)
			continue;
		
		if(mountPartition(partitionFile->partition.name, read_only) < 0)
			printf("failed to mount %s!\r\n", partitionFile->file_path);
		else
		{
			mounted++;
			partitionFile->mounted = 1;
		}
			
	}
	
	if(mounted <= 0)
	{
		printf("failed to mount decrypted partitions\r\n");
		unmount_fuse();	
		pthread_join(fuseThread , &ret);
		exit(0);
	}

	//set handler of sigint(ctrl-c) and wait for fuse to stop.
	signal(SIGINT, interrupt_handler);
	pthread_join(fuseThread , &ret);	
	
	printf("THE END\n");
	exit(0);
}
