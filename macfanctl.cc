/*
 *  macfanctl.c -  Fan control daemon for MacBook
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

#include <iostream>
#include <vector>

extern "C" {
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

#include "control.h"
#include "config.h"
extern void daemonize();
};

#include "macfanctl.h"

//------------------------------------------------------------------------------

int running = 1;
int lock_fd = -1;
int reload = 0;

//------------------------------------------------------------------------------

void signal_handler(int sig)
{
	switch (sig)
	{
	case SIGHUP:
		reload = 1;
		break;
	case SIGINT:
	case SIGTERM:
		running = 0;
		break;
	}
}

//-----------------------------------------------------------------------------

void usage()
{
	std::cout << "usage: macfanctld [-f]\n"
			     "       -f  run in foregound\n";
	exit(-1);
}

void setup_sighandler()
{
	// setup daemon
	signal(SIGCHLD, SIG_IGN); 			// ignore child
	signal(SIGTSTP, SIG_IGN); 			// ignore tty signals
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGINT, signal_handler); 	// catch Ctrl-C signal (terminating in foreground mode)
	signal(SIGHUP, signal_handler); 	// catch hangup signal (reload config)
	signal(SIGTERM, signal_handler); 	// catch kill signal
}

//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	int i;
	int daemon = 1;

	setup_sighandler();

	int opt;
	while ((opt = getopt(argc, argv, "f")) != -1)
	{
		switch(opt) {
			case 'f':
				daemon = 0;
				break;
			default:
				usage();
				break;
		}
	}

	if(daemon)
	{
		daemonize();
	}
	else
	{
		std::cout << "Running in foreground, log to stdout.\n";
	}

	// main loop

	read_cfg(CFG_FILE);

	find_applesmc();
	scan_sensors();

	running = 1;
	while(running)
	{
		adjust();

		logger();

		if(reload)
		{
			read_cfg(CFG_FILE);
			scan_sensors();
			reload = 0;
		}

		sleep(update_time);
	}

	// close pid file and delete it

	if(lock_fd != -1)
	{
		close(lock_fd);
		unlink(PID_FILE);
	}

	std::cout << "Exiting.\n";

	return 0;
}

//-----------------------------------------------------------------------------

