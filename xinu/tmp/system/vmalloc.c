/* vmalloc.c - This file contains function vmalloc to allocate storage on virtual heap */

#include <xinu.h>

/*------------------------------------------------------------------------
 *  vmalloc  -  Allocate virtual heap storage, returning lowest word address
 *------------------------------------------------------------------------
 */
char  	*vmalloc(
	  uint32	nbytes		/* Size of memory requested	*/
	)
{
	intmask	mask;			/* Saved interrupt mask		*/
	vmemblk_t *prev, *curr, *leftover;

	mask = disable();
	if (nbytes == 0) {
		restore(mask);
		return (char *)SYSERR;
	}

	nbytes = (uint32) roundpage(nbytes);	/* Use memblk multiples	*/
	if(nbytes > proctab[currpid].vmemlist->mlength || nbytes > (avail_heap_space * PAGE_SIZE)){
		kprintf("Demanding more memory than available\n");
		restore(mask);
		return (char *)SYSERR;
		
	}

	prev = proctab[currpid].vmemlist;
	curr = proctab[currpid].vmemlist->mnext;
	while (curr != NULL) {			/* Search free list	*/

		if (curr->mlength == nbytes) {	/* Block is exact match	*/
			prev->mnext = curr->mnext;
			//memlist.mlength -= nbytes;
			proctab[currpid].vmemlist->mlength -= nbytes;
			restore(mask);
			avail_heap_space -= nbytes;
			proctab[currpid].allocated_space += nbytes;
			return (char *)(curr->vmaddr);

		} else if (curr->mlength > nbytes) { /* Split big block	*/
			leftover = getmem(sizeof(vmemblk_t));
			leftover->vmaddr = (uint32) (curr->vmaddr +
					nbytes);
			prev->mnext = leftover;
			leftover->mnext = curr->mnext;
			leftover->mlength = curr->mlength - nbytes;
			proctab[currpid].vmemlist->mlength -= nbytes;
			restore(mask);
			avail_heap_space -= nbytes;
			proctab[currpid].allocated_space += nbytes;
			return (char *)(curr->vmaddr);
		} else {			/* Move to next block	*/
			prev = curr;
			curr = curr->mnext;
		}
	}
	restore(mask);
	kprintf("No free memory block\n");
	return (char *)SYSERR;
}
