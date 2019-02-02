/* create.c - create, newpid */

#include <xinu.h>

local	int newpid();

/*------------------------------------------------------------------------
 *  vcreate  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32	vcreate(
	  void		*funcaddr,	/* Address of the function	*/
	  uint32	ssize,		/* Stack size in bytes		*/
	  uint32	hsize,
	  pri16		priority,	/* Process priority > 0		*/
	  char		*name,		/* Name (for debugging)		*/
	  uint32	nargs,		/* Number of args that follow	*/
	  ...
	)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr;		/* Stack address		*/

	mask = disable();
	
	if(hsize > MAX_HEAP_SIZE){
		kprintf("Process creation failed\n");
		return SYSERR;
	}
		
	//avail_heap_space -= hsize;
	
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundmb(ssize);
	if ( (priority < 1) || ((pid=newpid()) == SYSERR) ||
	     ((saddr = (uint32 *)getstk(ssize)) == (uint32 *)SYSERR) ) {
		restore(mask);
		return SYSERR;
	}

	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;
	prptr->hsize = hsize;
	//prptr->PDBR = proctab[NULLPROC].PDBR;

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	//*******************************************************************************************************//
	//					PAGING RELATED FUNCTIONALITY
	//******************************************************************************************************//
	
	pd_t *pg_dir;
	//pt_t * p_table;
	int return_code;
	int offset;
	
	return_code = create_pg_dir(pid, &pg_dir);
	if(return_code == SYSERR)
		return SYSERR;
		
	for(offset =0 ;offset< NSTATIC ; offset++){
		(pg_dir+offset)->pd_pres = 1;
		(pg_dir+offset)->pd_write = 1;
  		(pg_dir+offset)->pd_user = 0;	
   		(pg_dir+offset)->pd_pwt	= 0;
  		(pg_dir+offset)->pd_pcd	= 0;
  		(pg_dir+offset)->pd_acc = 0;	
  		(pg_dir+offset)->pd_mbz	= 0;
  		(pg_dir+offset)->pd_fmb	= 0;
  		(pg_dir+offset)->pd_global = 0;
  		(pg_dir+offset)->pd_avail = 1;
  		(pg_dir+offset)->pd_base = base_addrs[offset];

	}
	
	prptr->vheap_min = V_HEAP_START * PAGE_SIZE;
	prptr->vheap_max = prptr->vheap_min + (hsize * PAGE_SIZE);
	prptr->vmemlist = getmem(sizeof(vmemblk_t));
	prptr->vmemlist->vmaddr = V_HEAP_START * PAGE_SIZE;
	//prptr->vmemlist = ;
	prptr->vmemlist->mlength = hsize * PAGE_SIZE;
	prptr->vmemlist->mnext = getmem(sizeof(vmemblk_t));
	prptr->vmemlist->mnext->vmaddr = (V_HEAP_START *PAGE_SIZE);
	prptr->vmemlist->mnext->mnext = NULL;
	prptr->vmemlist->mnext->mlength = hsize * PAGE_SIZE;
		
	

	/* Initialize stack as if the process was called		*/

	*saddr = STACKMAGIC;
	savsp = (uint32)saddr;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--)	/* Machine dependent; copy args	*/
		*--saddr = *a--;	/* onto created process's stack	*/
	*--saddr = (long)INITRET;	/* Push on return address	*/

	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registers, and an old SP	*/

	*--saddr = (long)funcaddr;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--saddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
	*--saddr = 0x00000200;		/* New process runs with	*/
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;			/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = 0;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;			/* %esi */
	*--saddr = 0;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
	restore(mask);
	return pid;
}

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}
	return (pid32) SYSERR;
}
