/*
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	@(#)vm.h	8.3 (Berkeley) 4/12/95
 */

#ifndef VM_H
#define VM_H

typedef int vm_inherit_t;		/* XXX: inheritance codes */

union vm_map_object;
typedef union vm_map_object vm_map_object_t;

struct vm_map_entry;
typedef struct vm_map_entry *vm_map_entry_t;

struct vm_map;
typedef struct vm_map *vm_map_t;

struct vm_object;
typedef struct vm_object *vm_object_t;

struct vm_page;
typedef struct vm_page  *vm_page_t;

struct pager_struct;
typedef struct pager_struct *vm_pager_t;

/*
 *	MACH VM locking primitives mappings to kernel locking primitives.
 */
typedef struct atomic_lk	simple_lock_data_t;
typedef struct atomic_lk	*simple_lock_t;
typedef struct lock		lock_data_t;
typedef struct lock		*lock_t;
#define	simple_lock(l)		atomic_lock(l)
#define	simple_lock_init(l)	atomic_lock_init(l)
#define	simple_lock_try(l)	atomic_lock_try(l)
#define	simple_unlock(l)	atomic_unlock(l)

#include <sys/vmmeter.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <vm/vm_param.h>
#include <vm/vm_prot.h>
#include <vm/vm_inherit.h>
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/pmap.h>
#include <vm/vm_extern.h>

/*
 * Shareable process virtual address space.
 * May eventually be merged with vm_map.
 * Several fields are temporary (text, data stuff).
 */
struct vmspace {
	struct	vm_map vm_map;	/* VM address map */
	struct	pmap vm_pmap;	/* private physical map */
	int	vm_refcnt;	/* number of references */
	caddr_t	vm_shm;		/* SYS5 shared memory private data XXX */
/* we copy from vm_startcopy to the end of the structure on fork */
#define vm_startcopy vm_rssize
	segsz_t vm_rssize; 	/* current resident set size in pages */
	segsz_t vm_swrss;	/* resident set size before last swap */
	segsz_t vm_tsize;	/* text size (pages) XXX */
	segsz_t vm_dsize;	/* data size (pages) XXX */
	segsz_t vm_ssize;	/* stack size (pages) */
	caddr_t	vm_taddr;	/* user virtual address of text XXX */
	caddr_t	vm_daddr;	/* user virtual address of data XXX */
	caddr_t vm_maxsaddr;	/* user VA at max stack growth */
};
#endif /* VM_H */
