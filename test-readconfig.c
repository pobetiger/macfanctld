/* 
 * test-readconfig.c
 *
 *   test code for reading config file into a dictionary
 *
 * Ben Sung Hsu, 2016 Nov 
 *
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "config.h"


// top-level used for testing
int main(int argc, char **argv) {

	int n =0;
	char *val_excl;

	read_cfg_file("macfanctl.conf");

	if (bConfigReady) {
		// print the config table
		printf("Mac Fan Control Daemon Configuration\n");
		for (int n=0; n<nConfigNbr; n++) {
			printf("[%d] Key=%s, Value=%s\n",
				n, ptConfigTable[n].key, ptConfigTable[n].val);
		}	
	}

	printf("Parsing config values\n");
	if (find_cfg_int("temp_avg_ceiling", &n))
		printf(" temp_avg_ceiling: %d\n", n);

	if ((val_excl = find_cfg_str("exclude"))) {
		//printf(" exclude: %s\n", val_excl);
		build_exclude_list(val_excl, find_cfg_strlen("exclude"));	

		printf(" sensors excluded: ");
		for (n=0; n<exclude_cnt; n++) {
			printf(" %d ", exclude_list[n]);
		}
		printf("\n");
	}

	return 0;
}






