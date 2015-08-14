/*
 * Copyright (c) 2004, Apple Computer, Inc. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Adapted from Apple sources for segedit compilation on Linux.
 * (c)2015 wvengen <dev-generic@willem.engen.nl>
 */
#include <string.h>
#include "mach-o-fat.h"
#include "mach-o-loader.h"
#include "bytesex.h"

__private_extern__
long long
SWAP_LONG_LONG(
long long ll)
{
	union {
	    char c[8];
	    long long ll;
	} in, out;
	in.ll = ll;
	out.c[0] = in.c[7];
	out.c[1] = in.c[6];
	out.c[2] = in.c[5];
	out.c[3] = in.c[4];
	out.c[4] = in.c[3];
	out.c[5] = in.c[2];
	out.c[6] = in.c[1];
	out.c[7] = in.c[0];
	return(out.ll);
}

__private_extern__
double
SWAP_DOUBLE(
double d)
{
	union {
	    char c[8];
	    double d;
	} in, out;
	in.d = d;
	out.c[0] = in.c[7];
	out.c[1] = in.c[6];
	out.c[2] = in.c[5];
	out.c[3] = in.c[4];
	out.c[4] = in.c[3];
	out.c[5] = in.c[2];
	out.c[6] = in.c[1];
	out.c[7] = in.c[0];
	return(out.d);
}

__private_extern__
float
SWAP_FLOAT(
float f)
{
	union {
	    char c[7];
	    float f;
	} in, out;
	in.f = f;
	out.c[0] = in.c[3];
	out.c[1] = in.c[2];
	out.c[2] = in.c[1];
	out.c[3] = in.c[0];
	return(out.f);
}

/*
 * get_host_byte_sex() returns the enum constant for the byte sex of the host
 * it is running on.
 */
__private_extern__
enum byte_sex
get_host_byte_sex(
void)
{
    uint32_t s;

	s = (BIG_ENDIAN_BYTE_SEX << 24) | LITTLE_ENDIAN_BYTE_SEX;
	return((enum byte_sex)*((char *)&s));
}

__private_extern__
void
swap_fat_header(
struct fat_header *fat_header,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif

	fat_header->magic     = SWAP_INT(fat_header->magic);
	fat_header->nfat_arch = SWAP_INT(fat_header->nfat_arch);
}

__private_extern__
void
swap_fat_arch(
struct fat_arch *fat_archs,
unsigned long nfat_arch,
enum byte_sex target_byte_sex)
{
    uint32_t i;
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif

	for(i = 0; i < nfat_arch; i++){
	    fat_archs[i].cputype    = SWAP_INT(fat_archs[i].cputype);
	    fat_archs[i].cpusubtype = SWAP_INT(fat_archs[i].cpusubtype);
	    fat_archs[i].offset     = SWAP_INT(fat_archs[i].offset);
	    fat_archs[i].size       = SWAP_INT(fat_archs[i].size);
	    fat_archs[i].align      = SWAP_INT(fat_archs[i].align);
	}
}

__private_extern__
void
swap_mach_header(
struct mach_header *mh,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	mh->magic = SWAP_INT(mh->magic);
	mh->cputype = SWAP_INT(mh->cputype);
	mh->cpusubtype = SWAP_INT(mh->cpusubtype);
	mh->filetype = SWAP_INT(mh->filetype);
	mh->ncmds = SWAP_INT(mh->ncmds);
	mh->sizeofcmds = SWAP_INT(mh->sizeofcmds);
	mh->flags = SWAP_INT(mh->flags);
}

__private_extern__
void
swap_mach_header_64(
struct mach_header_64 *mh,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	mh->magic = SWAP_INT(mh->magic);
	mh->cputype = SWAP_INT(mh->cputype);
	mh->cpusubtype = SWAP_INT(mh->cpusubtype);
	mh->filetype = SWAP_INT(mh->filetype);
	mh->ncmds = SWAP_INT(mh->ncmds);
	mh->sizeofcmds = SWAP_INT(mh->sizeofcmds);
	mh->flags = SWAP_INT(mh->flags);
	mh->reserved = SWAP_INT(mh->reserved);
}

__private_extern__
void
swap_load_command(
struct load_command *lc,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	lc->cmd = SWAP_INT(lc->cmd);
	lc->cmdsize = SWAP_INT(lc->cmdsize);
}

__private_extern__
void
swap_segment_command(
struct segment_command *sg,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	/* segname[16] */
	sg->cmd = SWAP_INT(sg->cmd);
	sg->cmdsize = SWAP_INT(sg->cmdsize);
	sg->vmaddr = SWAP_INT(sg->vmaddr);
	sg->vmsize = SWAP_INT(sg->vmsize);
	sg->fileoff = SWAP_INT(sg->fileoff);
	sg->filesize = SWAP_INT(sg->filesize);
	sg->maxprot = SWAP_INT(sg->maxprot);
	sg->initprot = SWAP_INT(sg->initprot);
	sg->nsects = SWAP_INT(sg->nsects);
	sg->flags = SWAP_INT(sg->flags);
}

__private_extern__
void
swap_segment_command_64(
struct segment_command_64 *sg,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	/* segname[16] */
	sg->cmd = SWAP_INT(sg->cmd);
	sg->cmdsize = SWAP_INT(sg->cmdsize);
	sg->vmaddr = SWAP_LONG_LONG(sg->vmaddr);
	sg->vmsize = SWAP_LONG_LONG(sg->vmsize);
	sg->fileoff = SWAP_LONG_LONG(sg->fileoff);
	sg->filesize = SWAP_LONG_LONG(sg->filesize);
	sg->maxprot = SWAP_INT(sg->maxprot);
	sg->initprot = SWAP_INT(sg->initprot);
	sg->nsects = SWAP_INT(sg->nsects);
	sg->flags = SWAP_INT(sg->flags);
}

__private_extern__
void
swap_section(
struct section *s,
unsigned long nsects,
enum byte_sex target_byte_sex)
{
    unsigned long i;
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif

	for(i = 0; i < nsects; i++){
	    /* sectname[16] */
	    /* segname[16] */
	    s[i].addr = SWAP_INT(s[i].addr);
	    s[i].size = SWAP_INT(s[i].size);
	    s[i].offset = SWAP_INT(s[i].offset);
	    s[i].align = SWAP_INT(s[i].align);
	    s[i].reloff = SWAP_INT(s[i].reloff);
	    s[i].nreloc = SWAP_INT(s[i].nreloc);
	    s[i].flags = SWAP_INT(s[i].flags);
	    s[i].reserved1 = SWAP_INT(s[i].reserved1);
	    s[i].reserved2 = SWAP_INT(s[i].reserved2);
	}
}

__private_extern__
void
swap_section_64(
struct section_64 *s,
unsigned long nsects,
enum byte_sex target_byte_sex)
{
    unsigned long i;
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif

	for(i = 0; i < nsects; i++){
	    /* sectname[16] */
	    /* segname[16] */
	    s[i].addr = SWAP_LONG_LONG(s[i].addr);
	    s[i].size = SWAP_LONG_LONG(s[i].size);
	    s[i].offset = SWAP_INT(s[i].offset);
	    s[i].align = SWAP_INT(s[i].align);
	    s[i].reloff = SWAP_INT(s[i].reloff);
	    s[i].nreloc = SWAP_INT(s[i].nreloc);
	    s[i].flags = SWAP_INT(s[i].flags);
	    s[i].reserved1 = SWAP_INT(s[i].reserved1);
	    s[i].reserved2 = SWAP_INT(s[i].reserved2);
	}
}

__private_extern__
void
swap_symtab_command(
struct symtab_command *st,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	st->cmd = SWAP_INT(st->cmd);
	st->cmdsize = SWAP_INT(st->cmdsize);
	st->symoff = SWAP_INT(st->symoff);
	st->nsyms = SWAP_INT(st->nsyms);
	st->stroff = SWAP_INT(st->stroff);
	st->strsize = SWAP_INT(st->strsize);
}

__private_extern__
void
swap_dysymtab_command(
struct dysymtab_command *dyst,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	dyst->cmd = SWAP_INT(dyst->cmd);
	dyst->cmdsize = SWAP_INT(dyst->cmdsize);
	dyst->ilocalsym = SWAP_INT(dyst->ilocalsym);
	dyst->nlocalsym = SWAP_INT(dyst->nlocalsym);
	dyst->iextdefsym = SWAP_INT(dyst->iextdefsym);
	dyst->nextdefsym = SWAP_INT(dyst->nextdefsym);
	dyst->iundefsym = SWAP_INT(dyst->iundefsym);
	dyst->nundefsym = SWAP_INT(dyst->nundefsym);
	dyst->tocoff = SWAP_INT(dyst->tocoff);
	dyst->ntoc = SWAP_INT(dyst->ntoc);
	dyst->modtaboff = SWAP_INT(dyst->modtaboff);
	dyst->nmodtab = SWAP_INT(dyst->nmodtab);
	dyst->extrefsymoff = SWAP_INT(dyst->extrefsymoff);
	dyst->nextrefsyms = SWAP_INT(dyst->nextrefsyms);
	dyst->indirectsymoff = SWAP_INT(dyst->indirectsymoff);
	dyst->nindirectsyms = SWAP_INT(dyst->nindirectsyms);
	dyst->extreloff = SWAP_INT(dyst->extreloff);
	dyst->nextrel = SWAP_INT(dyst->nextrel);
	dyst->locreloff = SWAP_INT(dyst->locreloff);
	dyst->nlocrel = SWAP_INT(dyst->nlocrel);
}

__private_extern__
void
swap_symseg_command(
struct symseg_command *ss,
enum byte_sex target_byte_sex)
{
#ifdef __MWERKS__
    enum byte_sex dummy;
        dummy = target_byte_sex;
#endif
	ss->cmd = SWAP_INT(ss->cmd);
	ss->cmdsize = SWAP_INT(ss->cmdsize);
	ss->offset = SWAP_INT(ss->offset);
	ss->size = SWAP_INT(ss->size);
}
