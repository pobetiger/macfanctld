/*
 *  config.c -  Fan control daemon for MacBook
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "config.h"


/* Global Vars */


/* About Structures 
 *
 *  ptConfigTable and exclude_list are allocated in the heap
 *
 *  they must be freed when they are done being used or are being
 *  updated.
 *
 *  exclude_list is realloc()'ed when updated, so no problem there during
 *    reload
 *
 *  ptConfigTable is just a ptr to an array to a bunch of dynamically 
 *    allocated strings this means the obj.key and obj.val pointers must 
 *    be free()'ed prior to other addEntry to avoid duplicates.
 *
 */

struct config_entry *ptConfigTable = 0;
int nConfigNbr = 0;
bool bConfigReady = false;
int *exclude_list = 0; // array of sensors to exclude
int exclude_cnt = 0;

// default variables
// TODO: check these against the actual defaults in code later
float temp_avg_floor = 40;		// default values if no config file is found
float temp_avg_ceiling = 50;

float temp_max_ceiling = 255;
float temp_max_floor = 254;
int temp_max_fan_min = 4000;

float temp_TC0P_floor = 40;
float temp_TC0P_ceiling = 50;

float temp_TG0P_floor = 40;
float temp_TG0P_ceiling = 50;

float fan_min = 0;
float fan_max = 6200;			// fixed max value

// aux control variables
int log_level = 0;
int update_time = 5; // seconds


/* Prototypes */
static bool addEntry(struct config_entry *next);
static bool makeNewEntry(struct config_entry *entry, char *buf, int len);
static bool readConfigTable(FILE *fd);

/* Implementations */

//-----------------------------------------------------------------------------
static bool addEntry(struct config_entry *next) {
	// add new entry	
	struct config_entry *tmpEntry = realloc(ptConfigTable, 
			(nConfigNbr+1)*sizeof(struct config_entry));

	if (!tmpEntry) {
		printf("realloc failed\n");
		return false;
	}

	ptConfigTable = tmpEntry;

	// make deep copy
	memcpy(&ptConfigTable[nConfigNbr], next, sizeof(struct config_entry));
	nConfigNbr++;

	return true;
}

//-----------------------------------------------------------------------------
//TODO: pretty sure we can strtok_r this
static bool makeNewEntry(struct config_entry *entry, char *buf, int len) {

	int key_s, key_len, val_s, val_len;
	char *key_ptr, *val_ptr; // in the source buf must make deep copy
	char *ptr;
	char *colon = strchr(buf, ':');	// find colon

	// filter out comments and blank lines
	if ((buf[0] == '#') || (buf[0] == '\n'))
		return false;

	// check if valid kv pair (must contain ':')
	if (colon == 0) {
		printf("Ill formed line in config file: %s\n", buf);
		return false;
	}

	//
	// extract key
	//
	
	// walk to first non-blank
	key_s = 0;
	ptr = buf;
	while (isblank(*ptr++))
		key_s++;
	key_ptr = ptr-1;

	key_len = colon - key_ptr; 

	// deblank the end
	ptr = key_ptr + key_len - 1;
	while (isblank(*ptr--))
		key_len--;

	//
	// extract value
	// 
	
	val_s = colon+1 - buf;
	ptr = colon+1;
	// walk after the colon until first nonblank
	while (isblank(*ptr++))
		val_s++;
	val_ptr = ptr-1;

	// NOTE: this is index not ptr math
	val_len = len - val_s; 

	// deblank the end
	ptr = val_ptr + val_len - 1;
	while ( (isblank(*ptr)) || 
			(*ptr=='\n') ||
		 	(*ptr=='\r') ) {
		val_len--;
		ptr--;
	}


	if ((key_len > len) || (key_len<=0) ){ /*||
		(val_len > len) || (val_len <=0)) { */
		printf("Bad length in extraction: k:%d v:%d\n\tstr: [%s]\n",
			key_len, val_len, buf);
		return false;
	}


	// DEBUG, mind the newline in buf
	//printf("extraction: k:%d v:%d\n\tstr: [%s]\n",
	//	key_len, val_len, buf);

	//
	// do deep copy
	//

	entry->key_len = key_len;
	entry->val_len = val_len;

	entry->key = malloc(key_len+1);
	entry->val = malloc(val_len+1);
	memcpy(entry->key, key_ptr, key_len);
	memcpy(entry->val, val_ptr, val_len);
	// force terminating zero
	entry->key[key_len] = 0;
	entry->val[val_len] = 0;

	return true;
}

static void flushConfigTable() {
	int n =0;
	
	// NOTE: when we restart, we must flush the entire ptConfigTable
	//        that means go thru each entry and free the key,val
	//        but we don't have to free the entry since we will
	//        be realloc()'ed and extra space occupied is not a big problem

	for (n = 0; n < nConfigNbr; n++) {
		if (ptConfigTable[n].key) free(ptConfigTable[n].key);
		if (ptConfigTable[n].val) free(ptConfigTable[n].val);
	}
	nConfigNbr = 0;
}

//-----------------------------------------------------------------------------
static bool readConfigTable(FILE *fd) {

	bool bSuccess = true;
	char *lineptr = 0;
	size_t linebuflen = 0;
	int linelen = 0;
	struct config_entry new_item;

	// when we reload, we must flush/free the table elements
	// don't necessarily have to free the main table since
	// addEntry will realloc() for space.
	flushConfigTable();

	while (!feof(fd)) {

		// clear buffer to ensure no stale data in next round
		if (linelen>0)
			memset(lineptr, 0, linelen);

		// getline reallocs the same buffer, so just free once is fine
		// but do remember to clear the lineptr bu linelen char each time
		// to prevent stale data
		if ((linelen = getline(&lineptr, &linebuflen, fd)) > 0) {

			// extract key, value pair
			// if valid: 
			//   new_item will now have deep copy content of line
			//   now we can free/reuse lineptr
			if (!makeNewEntry(&new_item, lineptr, linelen))
				continue;

			// if all goes well, add to table
			//   addEntry will make copy of the new entry
			//   now we can let the new item be popped off stack/zero'ed
			if (!addEntry(&new_item)) {
				bSuccess = false;
				break;
			}

			// clean up after use
			memset(&new_item, 0, sizeof(struct config_entry));
		}
	}

	free(lineptr);
	linelen = 0;
	linebuflen = 0;

	return bSuccess;
}

//-----------------------------------------------------------------------------
void read_cfg_file(char *filename) {

	FILE *fd = fopen(filename, "r");
	if (!fd) {
		printf("Invalid file handle\n");
		exit(-1);
	}
	if (!readConfigTable(fd)) {
		printf("Error reading config file, quitting now\n");
		exit(-1);
	}

	bConfigReady = true;

	fclose(fd);
}

//-----------------------------------------------------------------------------
char* find_cfg_str(const char *search_key) {

	int n =0;
	bool bFound = false;

	if (!bConfigReady)
		return NULL;
	
	for (n=0; n<nConfigNbr; n++) {
		// if search_key is larger than the current, just fail
		if (strncmp(ptConfigTable[n].key, search_key, 
					ptConfigTable[n].key_len) == 0) {
			bFound = true;
			break;
		}
	}

	if (bFound) {
		return ptConfigTable[n].val;
	} else {
		return NULL;
	}
}

int find_cfg_strlen(const char *search_key) {

	int n =0;
	bool bFound = false;

	if (!bConfigReady) {
		return 0;
	}

	for (n=0; n<nConfigNbr; n++) {
		// if search_key is larger than the current, just fail
		if (strncmp(ptConfigTable[n].key, search_key, 
					ptConfigTable[n].key_len) == 0) {
			bFound = true;
			break;
		}
	}

	if (bFound) {
		return ptConfigTable[n].val_len;
	} else {
		return 0;
	}
}

bool strtoi(char *val_str, int *val) {

	long raw_val;
	char *endptr = 0;

	raw_val = strtol(val_str, &endptr, 10);

	if (
		((errno == ERANGE) && (raw_val == LONG_MAX || raw_val == LONG_MIN)) ||
		(errno != 0 && raw_val==0)
	) {
		// overflow/underflow
		return false;
	}

	if (endptr == val_str) {
		// no digits
		return false;
	}

	// cap back to 32-bits
	*val = (int) raw_val & INT_MAX ;

	return true;
}


//-----------------------------------------------------------------------------
bool find_cfg_int(const char *search_key, int *cfg_val) {

	char *val_str = find_cfg_str(search_key);
	if ((!val_str) || (!cfg_val))
		return false;

	return strtoi(val_str, cfg_val);
}

//-----------------------------------------------------------------------------
// format is: name : integer

int read_param(char* name, int min_val, int max_val, int def)
{
	int val;

	if (!find_cfg_int(name, &val)) {
		// not found, use default
		val = def;
	}

	// always clamp (maybe default can be wrong)
	val = min(max_val, val);
	val = max(min_val, val);

	return val;
}

//-----------------------------------------------------------------------------
bool build_exclude_list(char *val_excl, int val_len) {

	char *saveptr, *token;
	int tmpInt, *tmpList;
	char *val_excl_wip = malloc(val_len);
	bool bSuccess = true;

	// strtok will destroy the original string, make working copy
	memcpy(val_excl_wip, val_excl, val_len);
	exclude_cnt = 0; /* reset the list count */

	do {
		token = strtok_r((exclude_cnt ? NULL : val_excl_wip), " \t", &saveptr);
		
		//DEBUG:
		//printf("token: %s\n", token);

		if (token && strtoi(token, &tmpInt)) {
			tmpList = realloc(exclude_list, sizeof(int)*(exclude_cnt+1));
			if (!tmpList) {
				// realloc failed, out of memory?
				bSuccess = false;
				break;
			}

			exclude_list = tmpList;
			exclude_list[exclude_cnt] = tmpInt;
			exclude_cnt ++;
		}
	} while (token != NULL);

	free(val_excl_wip);

	return bSuccess;

}

//-----------------------------------------------------------------------------
void read_exclude_list()
{
	char *val_excl = 0;
	// find the entry and then build a dynamic list
	if ((val_excl = find_cfg_str("exclude"))) {
		build_exclude_list(val_excl, find_cfg_strlen("exclude"));
	} else {
		printf("No exclusion list specified\n");
	}
}
 
//-----------------------------------------------------------------------------

void read_cfg(char* name)
{
	int i = 0;

	read_cfg_file(name);

	if(bConfigReady)
	{
		// TODO: min_val=0 is that realistic? or max_val=90 is that realistic?
		//       most system will be >25C and anything beyond 75-80C will probably fry the chips
		//       so why are we using these lowball values?
		//
		// TODO2: are these def values the same as the ones at the top?
		//
		temp_avg_ceiling = read_param("temp_avg_ceiling",	0, 90, 50);
		temp_avg_floor = read_param("temp_avg_floor", 		0, temp_avg_ceiling - 1, 40);

		temp_TC0P_ceiling = read_param("temp_TC0P_ceiling",	0, 90, 65);
		temp_TC0P_floor = read_param("temp_TC0P_floor",		0, temp_TC0P_ceiling - 1, 50);

		temp_TG0P_ceiling = read_param("temp_TG0P_ceiling",	0, 90, 80);
		temp_TG0P_floor = read_param("temp_TG0P_floor",		0, temp_TG0P_ceiling - 1, 65);

		temp_max_ceiling = read_param("temp_max_ceiling", 0, 90, 80);
		temp_max_floor = read_param("temp_max_floor", 0, temp_max_ceiling - 1, 65);

		fan_min = read_param("fan_min", 0, fan_max, 0);
		temp_max_fan_min = read_param("temp_max_fan_min", fan_min, fan_max, 4000);

		log_level = read_param("log_level", 0, 2, 0);

		update_time = read_param("update_time",1,30,5);
		
		read_exclude_list();

	}
	else
	{
		printf("Could not open config file %s\n"
			   "Existing settings will be used\n", name);
	}

	printf("Using parameters:\n");

	printf("\ttemp_avg_floor: %.0f\n", temp_avg_floor);
	printf("\ttemp_avg_ceiling: %.0f\n", temp_avg_ceiling);

	printf("\ttemp_max_ceiling: %.0f\n", temp_max_ceiling);
	printf("\ttemp_max_floor: %.0f\n", temp_max_floor);
	printf("\ttemp_max_fan_min: %d\n", temp_max_fan_min);

	printf("\ttemp_TC0P_floor: %.0f\n", temp_TC0P_floor);
	printf("\ttemp_TC0P_ceiling: %.0f\n", temp_TC0P_ceiling);

	printf("\ttemp_TG0P_floor: %.0f\n", temp_TG0P_floor);
	printf("\ttemp_TG0P_ceiling: %.0f\n", temp_TG0P_ceiling);

	printf("\tfan_min: %.0f\n", fan_min);

	printf("\texclude: ");
	for (i=0; i<exclude_cnt; i++) {
		printf("temp%d_input ", exclude_list[i]);
	}
	printf("\n");
	
	printf("\tlog_level: %d\n", log_level);
	printf("\tupdate_time: %d\n", update_time);
}

//-----------------------------------------------------------------------------

