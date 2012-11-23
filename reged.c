/*
 * reged.c - Simple Registry Edit Utility for NT 3.51 4.0 5.0 5.1 6.0 registry hives.
 *
 * This program is a command line utility to export of registry to .reg files
 * Import from .reg is also planned
 * Also, registry editor stuff should be moved into own library or something
 * 
 * 
 * 2008-mar: First version. Call reg edit library. Export to .reg file.
 * See HISTORY.txt for more detailed info on history.
 *
 *****
 *
 * Copyright (c) 1997-2007 Petter Nordahl-Hagen.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * See file GPL.txt for the full license.
 * 
 *****
 */


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


#include "ntreg.h"


const char reged_version[] = "reged version 0.1 080526, (c) Petter N Hagen";


/* Global verbosity flag */
int gverbose = 0;

/* Array of loaded hives */
#define MAX_HIVES 10
struct hive *hive[MAX_HIVES+1];
int no_hives = 0;


void usage(void)
{
  printf("-x <registryhivefile> <prefixstring> <key> <output.reg>\n"
	 "   Xport. Where <prefixstring> for example is HKEY_LOCAL_MACHINE\n"
	 "   <key> is key to dump (recursively), . means all keys in hive\n"
	 "\n"
	 "-e <registryhive> ...\n"
	 "   Interactive edit one or more of registry files\n"
	 "-v : More verbose messages\n"
	 );
}


int main(int argc, char **argv)
{
   
  int export = 0, edit = 0;
  int d = 0;
  int update = 0;
  int logchange = 0, mode = 0, dd = 0;
  int il;
  extern int optind;
  extern char* optarg;
  char *hivename, *prefix, *key, *outputname;
  char c;
  char yn[10];
  FILE *ch;
  
  char *options = "vhxe";
  
  printf("%s\n",reged_version);
  while((c=getopt(argc,argv,options)) > 0) {
    switch(c) {
    case 'e': edit = 1; break;
    case 'x': export = 1; break;
    case 'v': mode |= HMODE_VERBOSE; gverbose = 1; break;
    case 'h': usage(); exit(0); break;
    default: usage(); exit(1); break;
    }
  }
  if (!export && !edit) {
    usage();
    exit(1);
  }
  if (export && edit) {
    fprintf(stderr,"Edit and xport cannot be done at same time\n");
    usage();
    exit(1);
  }
  if (edit) {  /* Call editor. Rest of arguments are considered hives to load */
    hivename = argv[optind+no_hives];
    do {
      if (!(hive[no_hives] = openHive(hivename,
				      HMODE_RW|mode))) {
	printf("Unable to open/read a hive, exiting..\n");
	exit(1);
      }
      no_hives++;
      hivename = argv[optind+no_hives];
    } while (hivename && *hivename && no_hives < MAX_HIVES);
    regedit_interactive(hive, no_hives);
    update = 1;
  }
  if (export) { /* Call export. Works only on one hive at a time */
    hivename=argv[optind];
    prefix=argv[optind+1];
    key=argv[optind+2];
    outputname=argv[optind+3];
    if (gverbose) {
      printf("hivename: %s, prefix: %s, key: %s, output: %s\n",hivename,prefix,key,outputname);
    }
    
    if (!hivename || !*hivename || !prefix || !*prefix || !key || !*key || !outputname || !*outputname) {
      usage(); exit(1);
    }
    
    if (!(hive[no_hives] = openHive(hivename,HMODE_RO|mode))) {
      fprintf(stderr,"Unable to open/read hive %s, exiting..\n",hivename);
      exit(1);
    }
    
    export_key(hive[no_hives], 0, key, outputname, prefix);
    
    no_hives++;
    
  }
  
  if (update) {  /* run for functions that can have changed things */
    printf("\nHives that have changed:\n #  Name\n");
    for (il = 0; il < no_hives; il++) {
      if (hive[il]->state & HMODE_DIRTY) {
	if (!logchange) printf("%2d  <%s>\n",il,hive[il]->filename);
	d = 1;
      }
    }
    if (d) {
      /* Only prompt user if logging of changed files has not been set */
      /* Thus we assume confirmations are done externally if they ask for a list of changes */
      if (!logchange) fmyinput("Commit changes to registry? (y/n) [n] : ",yn,3);
      if (*yn == 'y' || logchange) {
	if (logchange) {
	  ch = fopen("/tmp/changed","w");
	}
	for (il = 0; il < no_hives; il++) {
	  if (hive[il]->state & HMODE_DIRTY) {
	    printf("%2d  <%s> - ",il,hive[il]->filename);
	    if (!writeHive(hive[il])) {
	      printf("OK\n");
	      if (logchange) fprintf(ch,"%s ",hive[il]->filename);
	      dd = 2;
	    }
	  }
	}
	if (logchange) {
	  fprintf(ch,"\n");
	  fclose(ch);
	}
      } else {
	printf("Not written!\n\n");
      }
    } else {
      printf("None!\n\n");
    }
  } /* list only check */
  return(dd);
}

