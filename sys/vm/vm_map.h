/* 
 * Copyright (c) 1985, Avadis Tevanian, Jr., Michael Wayne Young
 * Copyright (c) 1987 Carnegie-Mellon University
 * Copyright (c) 1991 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * The Mach Operating System project at Carnegie-Mellon University.
 *
 * The CMU software License Agreement specifies the terms and conditions
 * for use and redistribution.
 *
 *	@(#)vm_map.h	7.1 (Berkeley) 12/5/90
 */

/*
 *	Virtual memory map module definitions.
 */

#ifndef	_VM_MAP_
#define	_VM_MAP_

#ifdef KERNEL
#include "types.h"
#include "lock.h"
#include "../vm/pmap.h"
#include "../vm/vm_prot.h"
#include "../vm/vm_inherit.h"
#include "../vm/vm_object.h"
#else
#include <sys/types.h>
#include <sys/lock.h>
#include <vm/pmap.h>
#include <vm/vm_prot.h>
#include <vm/vm_inherit.h>
#include <vm/vm_object.h>
#endif

/*
 *	Types defined:
 *
 *	vm_map_t		the high-level address map data structure.
 *	vm_map_entry_t		an entry in an address map.
 *	vm_map_version_t	a timestamp of a map, for use with vm_map_lookup
 */

/*
 *	Objects which live in maps may be either VM objects, or
 *	another map (called a "sharing map") which denotes read-write
 *	sharing with other maps.
 */

union vm_map_object {
	struct vm_object	*vm_object;	/* object object */
	struct vm_map		*share_map;	/* share map */
	struct vm_map		*sub_map;	/* belongs to another map */
};

typedef union vm_map_object	vm_map_object_t;

/*
 *	Address map entries consist of start and end addresses,
 *	a VM object (or sharing map) and offset into that object,
 *	and user-exported inheritance and protection information.
 *	Also included is control information for virtual copy operations.
 */
struct vm_map_entry {
	struct vm_map_entry	*prev;		/* previous entry */
	struct vm_map_entry	*next;		/* next entry */
	vm_offset_t		start;		/* start address */
	vm_offset_t		end;		/* end address */
	union vm_map_object	object;		/* object I point to */
	vm_offset_t		offset;		/* offset into object */
	boolean_t		is_a_map;	/* Is "object" a map? */
	boolean_t		is_sub_map;	/* Is "object" a submap? */
		/* Only in sharing maps: */
	boolean_t		copy_on_write;	/* is data copy-on-write */
	boolean_t		needs_copy;	/* does object need to be copied */
		/* Only in task maps: */
	vm_prot_t		protection;	/* protection code */
	vm_prot_t		max_protection;	/* maximum protection */
	vm_inherit_t		inheritance;	/* inheritance */
	int			wired_count;	/* can be paged if = 0 */
};

typedef struct vm_map_entry	*vm_map_entry_t;

#define	VM_MAP_ENTRY_NULL	((vm_map_entry_t) 0)

/*
 *	Maps are doubly-linked lists of map entries, kept sorted
 *	by address.  A single hint is provided to start
 *	searches again from the last successful search,
 *	insertion, or removal.
 */
struct vm_map {
	lock_data_t		lock;		/* Lock for map data */
	struct vm_map_entry	header;		/* List of entries */
	int			nentries;	/* Number of entries */
	pmap_t			pmap;		/* Physical map */
	vm_size_t		size;		/* virtual size */
	boolean_t		is_main_map;	/* Am I a main map? */
	int			ref_count;	/* Reference count */
	simple_lock_data_t	ref_lock;	/* Lock for ref_count field */
	vm_map_entry_t		hint;		/* hint for quick lookups */
	simple_lock_data_t	hint_lock;	/* lock for hint storage */
	vm_map_entry_t		first_free;	/* First free space hint */
	boolean_t		entries_pageable; /* map entries pageable?? */
	unsigned int		timestamp;	/* Version number */
#define	min_offset		header.start
#define max_offset		header.end
};

typedef	struct vm_map	*vm_map_t;

#define		VM_MAP_NULL	((vm_map_t) 0)

/*
 *	Map versions are used to validate a previous lookup attempt.
 *
 *	Since lookup operations may involve both a main map and
 *	a sharing map, it is necessary to have a timestamp from each.
 *	[If the main map timestamp has changed, the share_map and
 *	associated timestamp are no longer valid; the map version
 *	does not include a reference for the imbedded share_map.]
 */
typedef struct {
	int		main_timestamp;
	vm_map_t	share_map;
	int		share_timestamp;
} vm_map_version_t;

/*
 *	Macros:		vm_map_lock, etc.
 *	Function:
 *		Perform locking on the data portion of a map.
 */

#define		vm_map_lock(map)	{ lock_write(&(map)->lock); (map)->timestamp++; }
#define		vm_map_unlock(map)	lock_write_done(&(map)->lock)
#define		vm_map_lock_read(map)	lock_read(&(map)->lock)
#define		vm_map_unlock_read(map)	lock_read_done(&(map)->lock)

/*
 *	Exported procedures that operate on vm_map_t.
 */

void		vm_map_init();
vm_map_t	vm_map_create();
void		vm_map_deallocate();
void		vm_map_reference();
int		vm_map_find();
int		vm_map_remove();
int		vm_map_lookup();
void		vm_map_lookup_done();
int		vm_map_protect();
int		vm_map_inherit();
int		vm_map_copy();
vm_map_t	vm_map_fork();
void		vm_map_print();
void		vm_map_copy_entry();
boolean_t	vm_map_verify();
void		vm_map_verify_done();

/*
 *	Functions implemented as macros
 */
#define		vm_map_min(map)		((map)->min_offset)
#define		vm_map_max(map)		((map)->max_offset)
#define		vm_map_pmap(map)	((map)->pmap)

/* XXX: number of kernel maps and entries to statically allocate */
#define MAX_KMAP	10
#define	MAX_KMAPENT	500

#endif	_VM_MAP_
