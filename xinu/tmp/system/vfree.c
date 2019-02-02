/*vfree.c - This file contains the function to free memory */

#include<xinu.h>

syscall vfree(char*blkaddr, uint32 nbytes){

	intmask	mask;			/* Saved interrupt mask		*/
	vmemblk_t	*next, *prev, *block;
	uint32	top;
	uint32 pdbr = proctab[currpid].PDBR;
	virt_addr_t *virt_addr = (virt_addr_t *)&blkaddr;
	uint32 pd_offset = virt_addr->pd_offset;
	uint32 pt_offset = virt_addr->pt_offset;
	uint32 virtual_pg_num = ((uint32)blkaddr)>>12;
	pd_t * pg_dir = (pd_t *)pdbr;
	pt_t * pg_tble;
	uint32 total_size = (uint32)blkaddr + nbytes;
	uint32 start_addr = (uint32)blkaddr;

	mask = disable();
	write_cr3(proctab[NULLPROC].PDBR);
	if ((nbytes == 0) || ((uint32) blkaddr < (uint32) proctab[currpid].vheap_min)
			  || ((uint32) blkaddr > (uint32) proctab[currpid].vheap_max)) {
		restore(mask);
		kprintf("Memory size to be freed is invalid\n");
		return SYSERR;
	}

	nbytes = (uint32) roundpage(nbytes);	/* Use page multiples	*/
	//virt_addr_t *virt_addr = (virt_addr_t *)&blkaddr;
	
	block = (vmemblk_t*) getmem(sizeof(vmemblk_t));  
	block->mnext = (vmemblk_t *)NULL;
	block->mlength = nbytes;
	block->vmaddr = (uint32)blkaddr;

	vmemblk_t *vmemlist = proctab[currpid].vmemlist;
	prev = vmemlist;			/* Walk along free list	*/
	next = vmemlist->mnext;
	
	while ((next != NULL) && (next->vmaddr < block->vmaddr)) {
		prev = next;
		next = next->mnext;
	}

	if (prev == vmemlist) {		/* Compute top of previous block*/
		top = (uint32) NULL;
	} else {
		top = (uint32) prev->vmaddr + prev->mlength;
	}

	/* Ensure new block does not overlap previous or next blocks	*/

	if (((prev != vmemlist) && (uint32) block < top)
	    || ((next != NULL)	&& (uint32) (block->vmaddr + nbytes) >(uint32)next->vmaddr)) {
		restore(mask);
		kprintf("Already Freed memory or not allocated at all\n");
		return SYSERR;
	}

	proctab[currpid].vmemlist->mlength += nbytes;

	/* Either coalesce with previous block or add to free list */

	if (top == (uint32) block->vmaddr) { 	/* Coalesce with previous block	*/
		prev->mlength += nbytes;
		block->vmaddr = prev->vmaddr;
	} else {			/* Link into list as new node	*/
		block->mnext = next;
		block->mlength = nbytes;
		prev->mnext = block;
	}

	/* Coalesce with next block if adjacent */

	if (((uint32) block->vmaddr + block->mlength) == (uint32) next->vmaddr) {
		block->mlength += next->mlength;
		block->mnext = next->mnext;
	}
	
	avail_heap_space += nbytes;
	proctab[currpid].allocated_space -= nbytes;
	
	while(start_addr < total_size){
		virt_addr = (virt_addr_t *)&start_addr;
		virtual_pg_num = start_addr >> 12;
		pd_offset = virt_addr->pd_offset;
		pt_offset = virt_addr->pt_offset;
		
		pg_tble = (pt_t*)((pg_dir + pd_offset)->pd_base * PAGE_SIZE ) + pt_offset;
		pg_tble -> pt_pres = 0;
		pg_tble -> pt_write = 0;
		pg_tble -> pt_base = 0;
		pg_tble -> pt_dirty = 0;
		
		clean_selected_frames(currpid, virtual_pg_num);
		start_addr += PAGE_SIZE;
	
	}
	
	write_cr3(pdbr);	
	restore(mask);
	return OK;

}

