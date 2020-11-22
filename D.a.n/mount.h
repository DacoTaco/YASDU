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

#ifndef _MOUNT_H_
#define _MOUNT_H_

typedef struct {
	char **argv;
	int argc;
	char* mountPoint;
} fuse_mount_args;

enum fuseState
{ 
	Disabled = 0, 
	Init = 1, 
	Running = 2
};

static char* fuseMountPoint = "./decrypted/";
static char* mountDestination = "/mnt/nand/";
extern char fuseMountState;


void* mount_fuse(void * args);
void unmount_fuse();
int mountPartition(const char* input, char read_only);
int unMountDecryptedPartition(const char* name);

#endif