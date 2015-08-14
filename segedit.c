/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
 * The segedit(1) program. This program extracts sections from an object
 * file, and takes the following options:
 *   -extract <segname> <sectname> <filename>
 *
 * Adapted from Apple sources for easier compilation on Linux.
 * (c)2015 wvengen <dev-generic@willem.engen.nl>
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <asm/byteorder.h>

#include "mach-o-loader.h"
#include "mach-o-fat.h"
#include "bytesex.h"

#define error(...) { \
  fprintf(stderr, __VA_ARGS__); \
  fprintf(stderr, "\n"); \
}
#define fatal(...) { \
  error(__VA_ARGS__); \
  exit(1); \
}

/* These variables are set from the command line arguments */
char *progname = NULL;	/* name of the program for error messages (argv[0]) */

static char *input;	/* object file to extract/replace sections from */

/* structure for holding -extract's arguments */
struct extract {
    char *segname;		/* segment name */
    char *sectname;		/* section name */
    char *filename;		/* file to put the section contents in */
    int32_t found;		/* set when the section is found */
    struct extract *next;	/* next extract structure, NULL if last */
} *extracts;			/* first extract structure, NULL if none */

/* These variables are set in the routine map_input() */
static void *input_addr;	/* address of where the input file is mapped */
static uint32_t input_size;	/* size of the input file */
static uint32_t input_mode;	/* mode of the input file */
static struct mach_header *mhp;	/* pointer to the input file's mach header */
static struct mach_header_64
			*mhp64;	/* pointer to the input file's mach header for
				   64-bit files */
static uint32_t mh_ncmds;	/* number of load commands */
static struct load_command
		*load_commands;	/* pointer to the input file's load commands */
static char swapped;	/* 1 if the input is to be swapped */
static enum byte_sex host_byte_sex = UNKNOWN_BYTE_SEX;
static enum byte_sex target_byte_sex = UNKNOWN_BYTE_SEX;

/* Internal routines */
static void map_input(
    void);
static void extract_sections(
    void);
static void extract_section(
    char *segname,
    char *sectname,
    uint32_t flags,
    uint32_t offset,
    uint32_t size);
static void *allocate(
    size_t size);
static void usage(
    void);

int
main(
int argc,
char *argv[],
char *envp[])
{
    int i;
    struct extract *ep;

	progname = argv[0];
	host_byte_sex = get_host_byte_sex();

	for (i = 1; i < argc; i++) {
	    if(argv[i][0] == '-'){
		switch(argv[i][1]){
		case 'e':
		    if(i + 4 > argc){
			error("missing arguments to %s option", argv[i]);
			usage();
		    }
		    ep = allocate(sizeof(struct extract));
		    ep->segname =  argv[i + 1];
		    ep->sectname = argv[i + 2];
		    ep->filename = argv[i + 3];
		    ep->found = 0;
		    ep->next = extracts;
		    extracts = ep;
		    i += 3;
		    break;
		default:
		    error("unrecognized option: %s", argv[i]);
		    usage();
		}
	    }
	    else{
		if(input != NULL){
		    fatal("only one input file can be specified");
		    usage();
		}
		input = argv[i];
	    }
	}

	if(input == NULL){
	    error("no input file specified");
	    usage();
	}

	if(extracts == NULL){
	    error("no -extract option specified");
	    usage();
	}

	map_input();

	if(extracts != NULL)
	    extract_sections();

	return(0);
}

/*
 * map_input maps the input file into memory.  The address it is mapped at is
 * left in input_addr and the size is left in input_size.  The input file is
 * checked to be an object file and that the headers are checked to be correct
 * enough to loop through them.  The pointer to the mach header is left in mhp
 * and the pointer to the load commands is left in load_commands.
 */
static
void
map_input(void)
{
    int fd;
    uint32_t i, magic, mh_sizeofcmds;
    struct stat stat_buf;
    struct load_command l, *lcp;
    struct segment_command *sgp;
    struct segment_command_64 *sgp64;
    struct section *sp;
    struct section_64 *sp64;
    struct symtab_command *stp;
    struct symseg_command *ssp;

	/* Open the input file and map it in */
	if((fd = open(input, O_RDONLY)) == -1)
	    fatal("can't open input file: %s", input);
	if(fstat(fd, &stat_buf) == -1)
	    fatal("Can't stat input file: %s", input);
	input_size = stat_buf.st_size;
	input_mode = stat_buf.st_mode;
	input_addr = mmap(0, input_size, PROT_READ|PROT_WRITE,
			  MAP_FILE|MAP_PRIVATE, fd, 0);
	if((intptr_t)input_addr == -1)
	    error("Can't map input file: %s", input);
	close(fd);

	if(sizeof(uint32_t) > input_size)
	    fatal("truncated or malformed object (mach header would extend "
		  "past the end of the file) in: %s", input);
	magic = *(uint32_t *)input_addr;
#ifdef __BIG_ENDIAN
	if(magic == FAT_MAGIC)
#endif /* __BIG_ENDIAN */
#ifdef __LITTLE_ENDIAN
	if(magic == SWAP_INT(FAT_MAGIC))
#endif /* __LITTLE_ENDIAN */
	    fatal("file: %s is a fat file (%s only operates on Mach-O files, "
		  "use lipo(1) on it to get a Mach-O file)", input, progname);

	mh_ncmds = 0;
	mh_sizeofcmds = 0;
	host_byte_sex = get_host_byte_sex();
	if(magic == SWAP_INT(MH_MAGIC) || magic == MH_MAGIC){
	    if(sizeof(struct mach_header) > input_size)
		fatal("truncated or malformed object (mach header would extend "
		      "past the end of the file) in: %s", input);
	    mhp = (struct mach_header *)input_addr;
	    if(magic == SWAP_INT(MH_MAGIC)){
		swapped = 1;
		target_byte_sex = host_byte_sex == BIG_ENDIAN_BYTE_SEX ?
				  LITTLE_ENDIAN_BYTE_SEX : BIG_ENDIAN_BYTE_SEX;
		swap_mach_header(mhp, host_byte_sex);
	    }
	    else{
		swapped = 0;
		target_byte_sex = host_byte_sex;
	    }
	    if(mhp->sizeofcmds + sizeof(struct mach_header) > input_size)
		fatal("truncated or malformed object (load commands would "
		      "extend past the end of the file) in: %s", input);
	    load_commands = (struct load_command *)((char *)input_addr +
				sizeof(struct mach_header));
	    mh_ncmds = mhp->ncmds;
	    mh_sizeofcmds = mhp->sizeofcmds;
	}
	else if(magic == SWAP_INT(MH_MAGIC_64) || magic == MH_MAGIC_64){
	    if(sizeof(struct mach_header_64) > input_size)
		fatal("truncated or malformed object (mach header would extend "
		      "past the end of the file) in: %s", input);
	    mhp64 = (struct mach_header_64 *)input_addr;
	    if(magic == SWAP_INT(MH_MAGIC_64)){
		swapped = 1;
		target_byte_sex = host_byte_sex == BIG_ENDIAN_BYTE_SEX ?
				  LITTLE_ENDIAN_BYTE_SEX : BIG_ENDIAN_BYTE_SEX;
		swap_mach_header_64(mhp64, host_byte_sex);
	    }
	    else{
		swapped = 0;
		target_byte_sex = host_byte_sex;
	    }
	    if(mhp64->sizeofcmds + sizeof(struct mach_header_64) > input_size)
		fatal("truncated or malformed object (load commands would "
		      "extend past the end of the file) in: %s", input);
	    load_commands = (struct load_command *)((char *)input_addr +
				sizeof(struct mach_header_64));
	    mh_ncmds = mhp64->ncmds;
	    mh_sizeofcmds = mhp64->sizeofcmds;
	}
	else
	    fatal("bad magic number (file is not a Mach-O file) in: %s", input);

	lcp = load_commands;
	for(i = 0; i < mh_ncmds; i++){
	    l = *lcp;
	    if(swapped)
		swap_load_command(&l, host_byte_sex);
	    if(l.cmdsize % sizeof(uint32_t) != 0)
		error("load command %u size not a multiple of "
		      "sizeof(uint32_t) in: %s", i, input);
	    if(l.cmdsize <= 0)
		fatal("load command %u size is less than or equal to zero "
		      "in: %s", i, input);
	    if((char *)lcp + l.cmdsize >
	       (char *)load_commands + mh_sizeofcmds)
		fatal("load command %u extends past end of all load commands "
		      "in: %s", i, input);
	    switch(l.cmd){
	    case LC_SEGMENT:
		sgp = (struct segment_command *)lcp;
		sp = (struct section *)((char *)sgp +
					sizeof(struct segment_command));
		if(swapped)
		    swap_segment_command(sgp, host_byte_sex);
		if(swapped)
		    swap_section(sp, sgp->nsects, host_byte_sex);
		break;
	    case LC_SEGMENT_64:
		sgp64 = (struct segment_command_64 *)lcp;
		sp64 = (struct section_64 *)((char *)sgp64 +
					sizeof(struct segment_command_64));
		if(swapped)
		    swap_segment_command_64(sgp64, host_byte_sex);
		if(swapped)
		    swap_section_64(sp64, sgp64->nsects, host_byte_sex);
		break;
	    case LC_SYMTAB:
		stp = (struct symtab_command *)lcp;
		if(swapped)
		    swap_symtab_command(stp, host_byte_sex);
		break;
	    case LC_SYMSEG:
		ssp = (struct symseg_command *)lcp;
		if(swapped)
		    swap_symseg_command(ssp, host_byte_sex);
		break;
	    default:
		*lcp = l;
		break;
	    }
	    lcp = (struct load_command *)((char *)lcp + l.cmdsize);
	}
}

/*
 * This routine extracts the sections in the extracts list from the input file
 * and writes then to the file specified in the list.
 */
static
void
extract_sections(void)
{
    uint32_t i, j, errors;
    struct load_command *lcp;
    struct segment_command *sgp;
    struct segment_command_64 *sgp64;
    struct section *sp;
    struct section_64 *sp64;
    struct extract *ep;

	lcp = load_commands;
	for(i = 0; i < mh_ncmds; i++){
	    if(lcp->cmd == LC_SEGMENT){
		sgp = (struct segment_command *)lcp;
		sp = (struct section *)((char *)sgp +
					sizeof(struct segment_command));
		for(j = 0; j < sgp->nsects; j++){
		    extract_section(sp->segname, sp->sectname, sp->flags,
				    sp->offset, sp->size);
		    sp++;
		}
	    }
	    else if(lcp->cmd == LC_SEGMENT_64){
		sgp64 = (struct segment_command_64 *)lcp;
		sp64 = (struct section_64 *)((char *)sgp64 +
					sizeof(struct segment_command_64));
		for(j = 0; j < sgp64->nsects; j++){
		    extract_section(sp64->segname, sp64->sectname, sp64->flags,
				    sp64->offset, sp64->size);
		    sp64++;
		}
	    }
	    lcp = (struct load_command *)((char *)lcp + lcp->cmdsize);
	}

	errors = 0;
	ep = extracts;
	while(ep != NULL){
	    if(ep->found == 0){
		error("section (%s,%s) not found in: %s", ep->segname,
		      ep->sectname, input);
		errors = 1;
	    }
	    ep = ep->next;
	}
	if(errors != 0)
	    exit(1);
}

static
void
extract_section(
char *segname,
char *sectname,
uint32_t flags,
uint32_t offset,
uint32_t size)
{
    struct extract *ep;
    int fd;

	ep = extracts;
	while(ep != NULL){
	    if(ep->found == 0 &&
	       strncmp(ep->segname, segname, 16) == 0 &&
	       strncmp(ep->sectname, sectname, 16) == 0){
		if(flags == S_ZEROFILL || flags == S_THREAD_LOCAL_ZEROFILL)
		    fatal("meaningless to extract zero fill "
			  "section (%s,%s) in: %s", segname,
			  sectname, input);
		if(offset + size > input_size)
		    fatal("truncated or malformed object (section "
			  "contents of (%s,%s) extends past the "
			  "end of the file) in: %s", segname,
			  sectname, input);
		 if((fd = open(ep->filename, O_WRONLY | O_CREAT |
			       O_TRUNC, 0666)) == -1)
		    fatal("can't create: %s", ep->filename);
		 if(write(fd, (char *)input_addr + offset,
			 size) != (int)size)
		    fatal("can't write: %s", ep->filename);
		 if(close(fd) == -1)
		    fatal("can't close: %s", ep->filename);
		 ep->found = 1;
	    }
	    ep = ep->next;
	}
}

// misc/allocate.c
static
void *
allocate(
size_t size)
{
    void *p;

	if(size == 0)
	    return(NULL);
	if((p = malloc(size)) == NULL)
	    fatal("virtual memory exhausted (malloc failed)");
	return(p);
}

/*
 * Print the usage message and exit non-zero.
 */
static
void
usage(void)
{
	fprintf(stderr, "Usage: %s <input file> [-extract <segname> <sectname> "
			"<filename>] ...\n", progname);
	exit(1);
}
