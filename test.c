/*
 * Copyright (C) 2019 Sergey Sergeev <adron@mstnt.com>
 *
 * Some structures and code has been taken from the U-Boot project.
 *	(C) Copyright 2008 Semihalf
 *	(C) Copyright 2000-2005
 *	Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include "types.h"
#include "fdt.h"

#define FDT_ALIGN(x, a)		(((x) + (a) - 1) & ~((a) - 1))
#define FDT_TAGALIGN(x)		(FDT_ALIGN((x), FDT_TAGSIZE))

#define FDT_FIRST_SUPPORTED_VERSION	0x10
#define FDT_LAST_SUPPORTED_VERSION	0x11

int fdt_streq(char *s1, char *s2){
	for(; *s1 != '\0' && *s2 != '\0'; s1++, s2++)
		if(*s1 != *s2)
			return 0;
	if(*s1 == '\0' && *s2 == '\0')
		return 1;
	return 0;
}

int fdt_strlen(char *s){
	int len = 0;
	for(; *s != '\0'; s++, len++);
	return len;
}

/*
	Checks fdt header variables for adequacy of values.
*/
int fdt_check_header(void *data, u32 data_size){
	struct fdt_header *image = (void*)data;
	u32 fdt_magic = ntohl(image->magic);
	u32 totalsize = ntohl(image->totalsize);
	u32 fdt_version = ntohl(image->version);
	u32 off_dt_struct = ntohl(image->off_dt_struct);
	u32 off_dt_strings = ntohl(image->off_dt_strings);
	u32 fdt_last_comp_version = ntohl(image->last_comp_version);
	if(fdt_magic != FDT_MAGIC)
		return -1;
	if(data_size < totalsize)
		return -2;
	if(fdt_version < FDT_FIRST_SUPPORTED_VERSION)
		return -3;
	if(fdt_last_comp_version > FDT_LAST_SUPPORTED_VERSION)
		return -4;
	if(off_dt_struct == 0 || off_dt_struct >= totalsize)
		return -5;
	if(off_dt_strings == 0 || off_dt_strings >= totalsize)
		return -6;
	return 0;
}

/*
	Returns: prop->data and prop->len for the prop with name prop_name
	whose parent node name is node_name.
*/
char *fdt_get_prop(void *data, char *node_name, char *prop_name, u32 *lenp_ret){
	struct fdt_header *image = (void*)data;
	u32 off_dt_struct = ntohl(image->off_dt_struct);
	u32 off_dt_strings = ntohl(image->off_dt_strings);
	u32 totalsize = ntohl(image->totalsize);
	u32 offset = 0;
	u32 nextoffset = 0;
	/* struct memory pointer */
	unsigned char *p = (void*)data + off_dt_struct;
	/* props names are stored separately, in a strings heap */
	unsigned char *n = (void*)data + off_dt_strings;
	/* data tail pointer */
	void *t = (void*)data + totalsize;
	int is_node_found = 0;
	struct fdt_property *fdt_prop;
	char *cur_prop_name;
	if(lenp_ret)
		*lenp_ret = 0;
	while(1){
		u32 *tagp = (void*)p + offset;
		u32 *lenp;
		u32 tag = ntohl(*tagp);
		/* out of range protection */
		if((void*)tagp + sizeof(*tagp) >= t)
			return NULL; /* offset is out of range ! */
		offset += FDT_TAGSIZE; /* skip tag var */
		if(tag == FDT_END)
			break; /* end of tags list is reached */
		switch(tag){
			case FDT_BEGIN_NODE: /* node tag. is the parent node for props and node tags. */
				if(fdt_streq(node_name, p + offset))
					is_node_found = 1;
				else
					is_node_found = 0;
				/* skip name var */
				offset += fdt_strlen(p + offset) + 1;
				break;
			case FDT_PROP: /* props of this node */
				lenp = (void*)p + offset;
				if(is_node_found){
					/* skip-name offset, length and value */
					fdt_prop = (void*)p + offset - FDT_TAGSIZE;
					/* out of range protection */
					if((void*)fdt_prop + sizeof(*fdt_prop) >= t)
						return NULL; /* fdt_prop is out of range ! */
					cur_prop_name = n + ntohl(fdt_prop->nameoff);
					/* out of range protection */
					if((void*)cur_prop_name >= t)
						return NULL; /* nameoff is out of range ! */
					if(fdt_streq(prop_name, cur_prop_name)){
						if(lenp_ret)
							*lenp_ret = ntohl(*lenp);
						return fdt_prop->data;
					}
				}
				offset += sizeof(struct fdt_property) - FDT_TAGSIZE + ntohl(*lenp);
				break;
			case FDT_END: /* end of tags list */
			case FDT_END_NODE: /* end of current node */
				is_node_found = 0;
			case FDT_NOP: /* nop ? */
				break;
			default:
				return NULL; /* unknown tag */
		}
		nextoffset = FDT_TAGALIGN(offset);
		offset = nextoffset;
	}
	return NULL; /* return NULL on error */
}

void fdt_print_all(void *data){
	struct fdt_header *image = (void*)data;
	u32 totalsize = ntohl(image->totalsize);
	u32 fdt_version = ntohl(image->version);
	u32 off_dt_struct = ntohl(image->off_dt_struct);
	u32 off_dt_strings = ntohl(image->off_dt_strings);
	u32 last_comp_version = ntohl(image->last_comp_version);

	printf("  size = %u\n", totalsize);
	printf("  fdt_version = %u\n", fdt_version);
	printf("  off_dt_struct = 0x%x\n", off_dt_struct);
	printf("  off_dt_strings = 0x%x\n", off_dt_strings);
	printf("  last_comp_version = %u\n", last_comp_version);
	printf("  totalsize = %u\n", totalsize);
	printf("! header1 len = %zu\n", sizeof(struct fdt_header));

	if(1){
		u32 offset = 0;
		u32 nextoffset = 0;
		int a;
		unsigned char *p = (void*)image + off_dt_struct;
		unsigned char *n = (void*)image + off_dt_strings;
		struct fdt_property *fdt_prop;
		while(1){
			u32 *tagp = (void*)p + offset;
			u32 *lenp;
			u32 tag = ntohl(*tagp);
			printf("tag = %u\n", tag);
			printf("  tot_offset = 0x%x\n", offset + off_dt_struct);
			offset += FDT_TAGSIZE;
			if(tag == FDT_END)
				break;
			switch(tag){
				case FDT_BEGIN_NODE:
					if(strlen(p + offset) > 0)
						printf("  name = '%s'\n", p + offset);
					else
						printf("  name = NULL !\n");
					/* skip name */
					offset += strlen(p + offset) + 1;
					break;
				case FDT_PROP:
					/* skip-name offset, length and value */
					fdt_prop = (void*)p + offset - FDT_TAGSIZE;
					lenp = (void*)p + offset;
					printf("  len = %u\n", ntohl(*lenp));
					printf("  nameoff = 0x%x\n", ntohl(fdt_prop->nameoff));
					{
						char *name = n + ntohl(fdt_prop->nameoff);
						printf("  name = !%s!\n", name);
					}
					if(ntohl(*lenp) < 8192){
						if(ntohl(*lenp) == 4)
							printf("  data = !0x%x!\n", ntohl(*(u32*)fdt_prop->data));
						else
							printf("  data = !%s!\n", fdt_prop->data);
					}
					offset += sizeof(struct fdt_property) - FDT_TAGSIZE + ntohl(*lenp);
					break;

				case FDT_END:
				case FDT_END_NODE:
				case FDT_NOP:
					break;

				default:
					printf("UNK! %d !\n", tag);
					return;
			}
			printf("  delta = %lu\n", FDT_TAGALIGN(offset) - nextoffset);
			nextoffset = FDT_TAGALIGN(offset);
			printf("nextoffset = %u\n", nextoffset);
			offset = nextoffset;
		}
	}
}
unsigned char data[10 * 1024 * 1024];
int main(void){
	unsigned char *p = data;
	int res;
	ssize_t len = 0;
	size_t rest = sizeof(data);
	int fd;
	fd = open("/home/prog/openwrt/lede-all/2019-openwrt-all/openwrt-ipq4xxx/bin/targets/ipq40xx/generic/openwrt-ipq40xx-meraki_mr33-initramfs-fit-uImage.itb", O_RDONLY);
	do{
		len = read(fd, p, rest);
		if(len <= 0)
			break;
		p += len;
		rest -= len;
	}while(rest > 0);
	close(fd);
	printf("%zd bytes readed\n", sizeof(data) - rest);

	res = fdt_check_header(data, sizeof(data) - rest);
	if(res){
		printf("fdt header contains errors. check res = %d\n", res);
		return res;
	}
	//fdt_print_all(data); /* dump all items */

	printf("FIT description: %s\n", fdt_get_prop(data, "", "description", NULL));
	printf("kernel description: %s\n", fdt_get_prop(data, "kernel@1", "description", NULL));
	printf("compression: %s\n", fdt_get_prop(data, "kernel@1", "compression", NULL));
	printf("Load Address: 0x%x\n", ntohl(*(u32*)fdt_get_prop(data, "kernel@1", "load", NULL)));
	printf("Entry Point: 0x%x\n", ntohl(*(u32*)fdt_get_prop(data, "kernel@1", "entry", NULL)));
	{
		u32 data_size = 0;
		fdt_get_prop(data, "kernel@1", "data", &data_size);
		printf("Kernel data Size: %d Bytes\n", data_size);
		printf("kernel description: %s\n", fdt_get_prop(data, "fdt@1", "description", NULL));
		fdt_get_prop(data, "fdt@1", "data", &data_size);
		printf("Kernel data Size: %d Bytes\n", data_size);
	}

	return 0;
}
