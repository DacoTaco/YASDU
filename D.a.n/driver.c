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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/xattr.h>
#include "driver.h"
#include "aes.h"
#include "ccrypto.c"

fs_state state = { 
	//Raw Partition
	{ RAWNAND, NULL, NULL, 0, 0,{} },
	//User Partitions
	{
		{ PRODINFO, NULL, NULL, 0, 0,{} },
		{ PRODINFOF, NULL, NULL, 0, 0,{} },
		{ SAFE, NULL, NULL, 0, 0,{} },
		{ SYSTEM, NULL, NULL, 0, 0,{} },
		{ USER, NULL, NULL, 0, 0,{} },
	}
};

struct fuse_operations nand_oper = {
	.getattr	= nand_getattr,
#if FUSE_USE_VERSION < 26
	.getdir		= nand_getdir,
	.open		= nand_open_compat,
	.read		= nand_read_compat,
	.write		= nand_write_compat,
	.release	= nand_release_compat,
	.setxattr	= nand_setxattr,
	.getxattr	= nand_getxattr
#else
	//.setattr	= nand_setattr,
	.readdir	= nand_readdir,
	.open		= nand_open,
	.read		= nand_read,
	.write		= nand_write,
	.release	= nand_release
#endif
};
const int read_size = 0x400; //NAND_SECTOR_SIZE
const int write_size = 0x10;

int nand_getPartitionInfo(const char* path, partition_info** info)
{	
	if(path == NULL || info == NULL)
		return -1;
	
	if(path[0] == '/')
		path++;
	
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		if(state.userPartitions[i].partition.name == NULL ||
		   state.userPartitions[i].active == 0 )
			continue;

		if (strcmp(path, state.userPartitions[i].partition.name) == 0) 
		{	
			*info = &state.userPartitions[i].partition;
			return i;
		} 
	}	
	return -2;
}

int nand_getPartitionFileInfo(const char* path, file_info** info)
{	
	if(path == NULL || info == NULL)
		return -1;
	
	if(path[0] == '/')
		path++;
	
	if(state.rawInfo.active == 1)
	{
		*info = &state.rawInfo;
		return ID_NAND;
	}
	
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		if(state.userPartitions[i].partition.name == NULL ||
		   state.userPartitions[i].active == 0 )
			continue;

		if (strcmp(path, state.userPartitions[i].partition.name) == 0) 
		{	
			*info = &state.userPartitions[i];
			return i;
		} 
	}	
	return -2;
}

int nand_getattr(const char *path, struct stat *stbuf) 
{
	printf("attrib of '%s' requested\n\r",path);
	//clear output buffer
	memset(stbuf, 0, sizeof(struct stat));
	//set root info & access rights
	int ret = 0;

	if (strcmp(path, "/") == 0) 
	{
		// root directory
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
		stbuf->st_uid = getuid();
		stbuf->st_gid = getgid();
		stbuf->st_atime = stbuf->st_mtime = stbuf->st_ctime = time(NULL);
	} 
	else
	{
		partition_info* partitionInfo = NULL;
		file_info* fileInfo = NULL;
		struct stat fileStat;
		if(nand_getPartitionInfo(path, &partitionInfo) >= 0 && 
		   nand_getPartitionFileInfo(path, &fileInfo) >= 0 && 
		   stat(fileInfo->file_path, &fileStat) >= 0 )
		{
			// the exposed files!
			//stbuf->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
			stbuf->st_dev = fileStat.st_dev;
			stbuf->st_mode = fileStat.st_mode;
			stbuf->st_ino = fileStat.st_ino;
			stbuf->st_nlink = 1;
			stbuf->st_uid = fileStat.st_uid;//getuid();
			stbuf->st_gid = fileStat.st_gid;//getgid();
			stbuf->st_mtime = fileStat.st_mtime;
			stbuf->st_atime = fileStat.st_atime;//stbuf->st_mtime = stbuf->st_ctime = time(NULL);
			stbuf->st_size = partitionInfo->partition_size;
			stbuf->st_blksize = 0x10;
			stbuf->st_blocks = partitionInfo->partition_size / 0x10;
			//device type - only used when block device
			stbuf->st_rdev = fileStat.st_rdev;
		}
		else
		{
			//we dont know what its asking attributes off. so piss off y0
			printf("failed to retrieve file attributes\r\n");
			ret = -ENOENT;
		}
	}

	return ret;
}

int nand_getxattr(const char* path,const char* attrib_name,char* buf, size_t size)
{
	printf("getxattr of %s requested\n\r",path);
	// can only open exposed file
	file_info* info = NULL;
	int i = nand_getPartitionFileInfo(path, &info);
	if (i <= 0) 
	{
		return -ENOENT;
	}	

	//call xattr of the file and return that
	int ret = getxattr(info->file_path, attrib_name, buf, size);
	//ssize_t getxattr (char* path,char* name,void* value,size_t size)
	//printf("%s - 0x%x - %ld - %d\n\r",attrib_name,buf,size,ret);
	return ret;
}

int nand_setxattr(const char* path,const char* attrib_name,const char *value, size_t size, int flags)
{
	printf("setxattr of %s requested\n\r",path);
	
	file_info* info = NULL;
	int i = nand_getPartitionFileInfo(path, &info);
	
	if (i < 0) 
		return -ENOENT;	

	//sure sure,we did it xD
	return 0;
}

int nand_open(const char *path, struct fuse_file_info *fi)
{
	printf("open of %s requested\n\r",path);
	// can only open exposed file

	file_info* info = NULL;
	if (nand_getPartitionFileInfo(path, &info) < 0) 
		return -ENOENT;

	if(info->fp != NULL)
	{
		//we already have the file open!
		printf("file already open!\n\r");
		return 0;
	}

	//state[i].fp = open(state[i].file_path,O_RDWR);
	info->fp = fopen(info->file_path,"rb+");

	return 0;
}

int nand_open_compat(const char* path, int i)
{
	return nand_open(path,NULL);	
}

int nand_release(const char *path, struct fuse_file_info *fi) 
{
	printf("release of %s requested\n\r",path);

	file_info* info = NULL;
	if (nand_getPartitionFileInfo(path, &info) < 0) 
		return -ENOENT;


	//closing not needed, when app closes, file handles get closed
	/*if(state[i].fp == NULL)
		return 0;


	pthread_mutex_lock(&state[i].lock);

	//close(state[i].fp);
	fclose(state[i].fp);
	state[i].fp = NULL;

	pthread_mutex_unlock(&state[i].lock);*/
	return 0;
}

int nand_release_compat(const char* path, int i)
{
	return nand_release(path,NULL);
}

int nand_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{	
	(void) offset;
	(void) fi;
	if(strcmp(path,"/") != 0)
		return -ENOENT;

	filler(buf,".",NULL,0);
	filler(buf,"..",NULL,0);
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		if (state.userPartitions[i].active == 1)
		{
			filler(buf,state.userPartitions[i].partition.name,NULL,0);
		}
	}

	return 0;
}


int nand_getdir(const char *path,fuse_dirh_t hd,fuse_dirfil_t filler)
{
	if(strcmp(path,"/") != 0)
		return -ENOENT;

#if FUSE_USE_VERSION < 26

	filler(hd,".",0);
	filler(hd,"..",0);
	//add a +1 to not add the extra	/ in the file names
	for(int i = 0;i < PARTITION_COUNT;i++)
	{
		if (state.userPartitions[i].active == 1)
		{
			filler(hd,state.userPartitions[i].partition.name,0);
		}
#endif

	return 0;
}

int nand_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
	//check if we actually have this file
	file_info* fileInfo = NULL;
	partition_info* partitionInfo = NULL;
	
	if (nand_getPartitionInfo(path, &partitionInfo) < 0) 
	{
		printf("failed to retrieve the PartitionInfo for file %s\r\n",path);
		return -ENOENT;
	}
	
	if (nand_getPartitionFileInfo(path, &fileInfo) < 0) 
	{
		printf("failed to retrieve the fileInfo for file %s\r\n",path);
		return -ENOENT;
	}

	//DONT READ TO MUCH Y0
	if (size + offset > partitionInfo->partition_size) 
		size = partitionInfo->partition_size - offset;
	
	if(size <= 0)
		return -EINVAL;

	int block_size = read_size;
	unsigned char* read_buf = (unsigned char*)malloc(block_size);
	if(read_buf == NULL)
		return -ENOMEM;

	int read = 0;
	unsigned long long decrypt_offset = offset - (offset % 0x10);
	unsigned long long partition_start = 0;
	unsigned long long sector_offset = offset % NAND_SECTOR_SIZE;
	unsigned long long read_skip = offset % 0x10;
	unsigned long long tweakHI,tweakLO = 0;	
	if(fileInfo->partition.ID == ID_NAND)
		partition_start = RAW_USERPARTITION_BASE + partitionInfo->partition_offset;
	
	printf("read %s : 0x%lx(+0x%llx) -> 0x%lx\n\r",path, offset, partition_start, offset+size);
	
	while (read < size)
	{	
		unsigned long long sector = ( decrypt_offset / NAND_SECTOR_SIZE);
		tweakHI = (sector >> 63) & 0xFFFFFFFFFFFFFFFF;
		tweakLO = sector & 0xFFFFFFFFFFFFFFFF;

		memset(read_buf, 0, block_size);
		if( block_size > size-read )
			block_size = (size - read);

		//read in blocks of 0x400 bytes
		//should be relatively fast to read & decrypt
		pthread_mutex_lock(&fileInfo->lock);
		if( fseek(fileInfo->fp, partition_start + decrypt_offset, SEEK_SET) < 0)
		{
			printf("fseek returned < 0!\n\r");
			free(read_buf);
			return -EINVAL;
		}
		
		int enc_read = fread(read_buf, 1, block_size, fileInfo->fp);
		pthread_mutex_unlock(&fileInfo->lock);
		if(enc_read <= 0)
		{
			printf("pread returned %d!\n\r",enc_read);
			free(read_buf);
			return -EFAULT;
		}

		if(partitionInfo->encrypted)
		{
			//void aes_xtsn_decrypt(u8 *buffer, u64 len, u8 *key, u8 *tweakin, u64 sectoroffsethi, u64 sectoroffsetlo, u32 sector_size, u64 sectoroffset) 
			aes_xtsn_decrypt(	read_buf,
								enc_read,
								partitionInfo->crypt_key,
								partitionInfo->tweak_key,
								tweakHI,
								tweakLO,
								NAND_SECTOR_SIZE,
								sector_offset);
		}

		//copy the decrypted data to the buffer!
		int toCopySize = (size-read > enc_read-read_skip)? enc_read-read_skip : size-read;
		memcpy(buf+read,read_buf+read_skip,toCopySize);
		read += toCopySize;

		//setup for next decrypt	
		decrypt_offset += enc_read;
		sector_offset += toCopySize;
		if (sector_offset >= NAND_SECTOR_SIZE)
			sector_offset = sector_offset % NAND_SECTOR_SIZE;
		read_skip = 0;
	}

	if(read != size)
		printf("!!!!!WARNING!!!!!! READ : %d , ASKED SIZE : %ld\n\r",read,size);
	free(read_buf);	
	return read;
}

int nand_read_compat(const char *path, char *buf,size_t size,off_t offset)
{
	return nand_read(path,buf,size,offset,NULL);
}

int nand_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) 
{
	//check if we actually have this file
	file_info* fileInfo = NULL;
	partition_info* partitionInfo = NULL;
	
	if (nand_getPartitionInfo(path, &partitionInfo) < 0) 
	{
		printf("failed to retrieve the PartitionInfo for file %s\r\n",path);
		return -ENOENT;
	}
	
	if (nand_getPartitionFileInfo(path, &fileInfo) < 0) 
	{
		printf("failed to retrieve the fileInfo for file %s\r\n",path);
		return -ENOENT;
	}
	
	//DONT WRITE TO MUCH Y0
	if (size + offset > partitionInfo->partition_size) 
		size = partitionInfo->partition_size - offset;
	
	if(size <= 0)
		return -EINVAL;
	
	
	int written = 0;	
	unsigned char* write_buf = (unsigned char*)malloc(write_size);
	if(write_buf == NULL)
		return -ENOMEM;

	unsigned long long decrypt_offset = offset - (offset % 0x10);
	unsigned long long sector_offset = offset % NAND_SECTOR_SIZE;
	unsigned long long write_skip = offset % 0x10;
	unsigned long long tweakHI,tweakLO = 0;	
	unsigned long long partition_start = 0;
	
	if(fileInfo->partition.ID == ID_NAND)
		partition_start = RAW_USERPARTITION_BASE + partitionInfo->partition_offset;
	
	printf("write %s: 0x%lx(+0x%llx) -> 0x%lx\n\r", path, offset, partition_start, offset+size);
	while (written < size)
	{	
		memset(write_buf, 0, write_size);
		unsigned long long sector = ( decrypt_offset / NAND_SECTOR_SIZE);
		tweakHI = (sector >> 63) & 0xFFFFFFFFFFFFFFFF;
		tweakLO = sector & 0xFFFFFFFFFFFFFFFF;

		//read decrypted data
		int dec_read = nand_read(path, (char*)write_buf, write_size, decrypt_offset, fi);
		if ( dec_read != write_size)
		{
			printf("nand_read returned %d!\n\r", dec_read);
			free(write_buf);
			return -EFAULT;
		}

		//write data to buf
		int toCopySize = (size-written > dec_read-write_skip)? dec_read-write_skip : size-written;
		memcpy(write_buf+write_skip,buf+written,toCopySize);
		written += toCopySize;

		//encrypt & write data back to NAND		
		if(partitionInfo->encrypted)
		{
			//void aes_xtsn_encrypt(u8 *buffer, u64 len, u8 *key, u8 *tweakin, u64 sectoroffsethi, u64 sectoroffsetlo, u32 sector_size, u64 sectoroffset) 
			aes_xtsn_encrypt(	write_buf, 
								write_size,
								partitionInfo->crypt_key,
								partitionInfo->tweak_key,
								tweakHI,
								tweakLO,
								NAND_SECTOR_SIZE,
								sector_offset);
		}
			
		//get lock, write data and release lock.
		pthread_mutex_lock(&fileInfo->lock);
		fseek(fileInfo->fp, partition_start+decrypt_offset, SEEK_SET);
		int enc_write = fwrite(write_buf, 1, write_size, fileInfo->fp);		
		pthread_mutex_unlock(&fileInfo->lock);
		if(enc_write <= 0)
		{
			printf("pwrite returned %d!!!!!!!\n\r",enc_write);
			free(write_buf);
			return -EFAULT;
		}
		
		//setup for next write	
		decrypt_offset += write_size;
		sector_offset += toCopySize;
		if (sector_offset >= NAND_SECTOR_SIZE)
			sector_offset = sector_offset % NAND_SECTOR_SIZE;
		write_skip = 0;		
	}	

	if(written != size)
		printf("!!!!!WARNING!!!!!! WRITE : %d , ASKED SIZE : %ld\n\r", written, size);

	return written;
}

int nand_write_compat(const char *path, const char *buf,size_t size,off_t offset)
{
	printf("nand write called\n\r");
	return nand_write(path,buf,size,offset,NULL);	
}

