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

#ifndef _DRIVER_H_

#define _DRIVER_H_
#define _FILE_OFFSET_BITS 64
#define KEY_SIZE 32

#include "fuse.h"

typedef struct {
	char* name;
	char* switch_name;
	size_t partition_offset;
	size_t partition_size;
	char encrypted;
	char ID;
	unsigned char tweak_key[KEY_SIZE];
	unsigned char crypt_key[KEY_SIZE];
} partition_info;

//there are 5 encrypted user partitions on the nand
#define ID_NAND		0
#define PARTITION_COUNT 5
#define RAWNAND		(partition_info){"NAND"		, "mmcblk1p666"	, 0x00000000, 0xFFFFFFFFFFFF, 	0, ID_NAND		, {}, {} }
#define PRODINFO	(partition_info){"PRODINFO"	, "mmcblk1p1"	, 0x00004400, 0x003FBC00, 		1, ID_NAND+1	, {}, {} }
#define PRODINFOF	(partition_info){"PRODINFOF", "mmcblk1p2"	, 0x00400000, 0x00400000, 		1, ID_NAND+2	, {}, {} }
#define SAFE		(partition_info){"SAFE"		, "mmcblk1p9"	, 0x03800000, 0x04000000, 		1, ID_NAND+3	, {}, {} }
#define SYSTEM	 	(partition_info){"SYSTEM"	, "mmcblk1p10"	, 0x07800000, 0xA0000000, 		1, ID_NAND+4	, {}, {} }
#define USER		(partition_info){"USER"  	, "mmcblk1p11"	, 0xA7800000, 0x680000000, 		1, ID_NAND+5	, {}, {} }
#define NAND_SECTOR_SIZE 0x4000
#define RAW_USERPARTITION_BASE 0x01800000

typedef struct {
	partition_info partition;
	char *file_path;
	FILE* fp;
	char active;
	char mounted;
	pthread_mutex_t lock;
} file_info;

typedef struct {
	file_info rawInfo;
	file_info userPartitions[PARTITION_COUNT];
} fs_state;

extern fs_state state;
extern struct fuse_operations nand_oper;

int nand_getattr(const char *path, struct stat *stbuf);
int nand_getxattr(const char* path,const char* attrib_name,char* buf, size_t size);
int nand_setxattr(const char* path,const char* attrib_name,const char *value, size_t size, int flags);

int nand_open_compat(const char* path, int i);
int nand_open(const char *path, struct fuse_file_info *fi);

int nand_release(const char *path, struct fuse_file_info *fi);
int nand_release_compat(const char* path, int i);

int nand_getdir(const char *path,fuse_dirh_t hd,fuse_dirfil_t filler);
int nand_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi);

int nand_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int nand_read_compat(const char *path, char *buf,size_t size,off_t offset);

int nand_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int nand_write_compat(const char *path, const char *buf,size_t size,off_t offset);

#endif
