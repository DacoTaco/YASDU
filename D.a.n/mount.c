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
#include <sys/mount.h>
#include "fuse.h"
#include "driver.h"
#include "mount.h"

char fuseMountState = Disabled;
struct fuse_chan* chan;

void* mount_fuse(void * args)
{
	fuseMountState = Init;
	fuse_mount_args* fuse_args = args;
	if(fuse_args == NULL || fuse_args->mountPoint == NULL)
	{
		printf("Invalid fuse arguments.\r\n");
		fuseMountState = Disabled;
		return NULL;
	}	
	
	const char* mountPoint = fuse_args->mountPoint;	
	struct fuse_args _args = FUSE_ARGS_INIT(fuse_args->argc, fuse_args->argv);
	chan = fuse_mount(mountPoint, &_args);
	if (chan == NULL) {
		printf("error: failed to mount fuse\n");
		fuseMountState = Disabled;
		return NULL;
	}

	struct fuse *fuse = fuse_new(chan, &_args, &nand_oper, sizeof(nand_oper), &state);
	if (fuse == NULL) {
		fuse_unmount(mountPoint, chan);
		printf("error: failed to create fuse\n");
		fuseMountState = Disabled;
		return NULL;
	}
	
	struct fuse_session *se = fuse_get_session(fuse);
	if (fuse_set_signal_handlers(se) != 0) {
		fuse_unmount(mountPoint, chan);
		printf("error: failed to set signal handlers\n");
		fuseMountState = Disabled;
		return NULL;
	}
	
	//start fuse loop in foreground
	//this will loop untill an (fuse)unmount is called
	fuse_daemonize(1);
	//fuse_loop(fuse);
	fuseMountState = Running;
	int ret = fuse_loop_mt(fuse);
	fuse_remove_signal_handlers(se);
	fuseMountState = Disabled;
	return &ret;
}

void unmount_fuse()
{
	if(fuseMountState != Running)
		return;
	
	fuse_unmount(fuseMountPoint, chan);
}