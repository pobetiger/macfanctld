/*
 *  daemonize.c -  Fan control daemon for MacBook
 *
 *  Copyright (C) 2010  Mikael Strom <mikael@sesamiq.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  Note:
 *  This was written for MacBook Pro 5,1 and Ubutnu 10.04
 *  Requires applesmc-dkms
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "macfanctl.h"

void daemonize()
{
	if (getppid() == 1)
		return; // already a daemon

	// fork of new process
	pid_t pid = fork();

	if(pid < 0)
		exit(1); 		// fork error

	if(pid > 0)
		exit(0);		// parent exits

	// child (daemon) continues

#ifdef DEBUG
	sleep(20);			// time to attach debugger to this process
#endif

	setsid(); 			// create a new session

#ifdef DEBUG
	umask(0);
#else
	umask(022); // set createfile permissions
#endif

	// keep the daemonizing code slightly more C
	freopen(LOG_FILE, "w", stdout);
	freopen("/dev/null", "r", stdin);

	chdir("/");

	// create lockfile
	int lock_fd = open(PID_FILE, O_RDWR | O_CREAT, 0640);

	if(lock_fd < 0)
		exit(1); 		// open failed, we're a duplicate

	if(lockf(lock_fd, F_TLOCK, 0) < 0)
		exit(0); 		// lock failed - no idea what this means...

	// first instance continues...
	// write pid to file, and leave file open (blocking duplicates)

	char str[32];
	snprintf(str, 32, "%d\n", getpid());
	write(lock_fd, str, strlen(str));
}
