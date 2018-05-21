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

#define FUSE_USE_VERSION 26
#define _DRIVER_H_

#include <fuse.h>

#define KEY_SIZE 32
//there are 5 encrypted partitions on the nand
#define PARTITION_COUNT 5
#define PARTITION_SIZE 0x00400000
#define NAND_SECTOR_SIZE 0x4000
#define NAND_SECTOR_COUNT (PARTITION_SIZE / NAND_SECTOR_SIZE)

struct partition_state {
	char *name;
	char *file_path;
	size_t partition_size;
	int fp;
	char report;
	unsigned char tweak_key[KEY_SIZE];
	unsigned char crypt_key[KEY_SIZE];
	pthread_mutex_t lock;
};

//compatability stuff
//replace with array of states
struct partition_state state[PARTITION_COUNT];

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


static struct fuse_operations nand_oper = {
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




#endif