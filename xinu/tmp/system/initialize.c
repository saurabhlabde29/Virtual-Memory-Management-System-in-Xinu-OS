/* initialize.c - nulluser, sysinit */

/* Handle system initialization and become the null process */

#include <xinu.h>
#include <string.h>

extern	void	start(void);	/* Start of Xinu code			*/
extern	void	*_end;		/* End of Xinu code			*/

/* Function prototypes */

extern	void main(void);	/* Main is the first process created	*/
static	void sysinit(); 	/* Internal system initialization	*/
extern	void meminit(void);	/* Initializes the free memory list	*/
local	process startup(void);	/* Process to finish startup tasks	*/

/* Declarations of major kernel variables */

struct	procent	proctab[NPROC];	/* Process table			*/
struct	sentry	semtab[NSEM];	/* Semaphore table			*/
struct	memblk	memlist;	/* List of free memory blocks		*/
//******************************************************************************//
//			PAGING RELATED STRUCTURES
//******************************************************************************//
frame_t pdpt_tab[MAX_PT_SIZE];
frame_t fss_tab[MAX_FSS_SIZE];
frame_t dss_tab[MAX_SWAP_SIZE];
int base_addrs[NTAB];


/* Active system status */

int	prcount;		/* Total number of live processes	*/
pid32	currpid;		/* ID of currently executing process	*/

//******************************************************************************//
//			PAGING RELATED STRUCTURES
//******************************************************************************//
int num_of_free_ffs_frames = MAX_FSS_SIZE;
int avail_heap_space = MAX_HEAP_SIZE;


/* Control sequence to reset the console colors and cusor positiion	*/

#define	CONSOLE_RESET	" \033[0m\033[2J\033[;H"

/*------------------------------------------------------------------------
 * nulluser - initialize the system and become the null process
 *
 * Note: execution begins here after the C run-time environment has been
 * established.  Interrupts are initially DISABLED, and must eventually
 * be enabled explicitly.  The code turns itself into the null process
 * after initialization.  Because it must always remain ready to execute,
 * the null process cannot execute code that might cause it to be
 * suspended, wait for a semaphore, put to sleep, or exit.  In
 * particular, the code must not perform I/O except for polled versions
 * such as kprintf.
 *------------------------------------------------------------------------
 */

void	nulluser()
{	
	struct	memblk	*memptr;	/* Ptr to memory block		*/
	uint32	free_mem;		/* Total amount of free memory	*/
	
	/* Initialize the system */

	sysinit();

	/* Output Xinu memory layout */
	free_mem = 0;
	for (memptr = memlist.mnext; memptr != NULL;
						memptr = memptr->mnext) {
		free_mem += memptr->mlength;
	}
	kprintf("%10d bytes of free memory.  Free list:\n", free_mem);
	for (memptr=memlist.mnext; memptr!=NULL;memptr = memptr->mnext) {
	    kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)memptr, ((uint32)memptr) + memptr->mlength - 1);
	}

	kprintf("%10d bytes of Xinu code.\n",
		(uint32)&etext - (uint32)&text);
	kprintf("           [0x%08X to 0x%08X]\n",
		(uint32)&text, (uint32)&etext - 1);
	kprintf("%10d bytes of data.\n",
		(uint32)&ebss - (uint32)&data);
	kprintf("           [0x%08X to 0x%08X]\n\n",
		(uint32)&data, (uint32)&ebss - 1);

	/* Enable interrupts */

	enable();

	/* Initialize the network stack and start processes */

	net_init();

	/* Create a process to finish startup and start main */

	resume(create((void *)startup, INITSTK, INITPRIO,
					"Startup process", 0, NULL));

	/* Become the Null process (i.e., guarantee that the CPU has	*/
	/*  something to run when no other process is ready to execute)	*/

	while (TRUE) {
		;		/* Do nothing */
	}

}


/*------------------------------------------------------------------------
 *
 * startup  -  Finish startup takss that cannot be run from the Null
 *		  process and then create and resume the main process
 *
 *------------------------------------------------------------------------
 */
local process	startup(void)
{
	uint32	ipaddr;			/* Computer's IP address	*/
	char	str[128];		/* String used to format output	*/


	/* Use DHCP to obtain an IP address and format it */

	ipaddr = getlocalip();
	if ((int32)ipaddr == SYSERR) {
		kprintf("Cannot obtain an IP address\n");
	} else {
		/* Print the IP in dotted decimal and hex */
		ipaddr = NetData.ipucast;
		sprintf(str, "%d.%d.%d.%d",
			(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
			(ipaddr>>8)&0xff,        ipaddr&0xff);
	
		kprintf("Obtained IP address  %s   (0x%08x)\n", str,
								ipaddr);
	}

	/* Create a process to execute function main() */

	resume(create((void *)main, INITSTK, INITPRIO,
					"Main process", 0, NULL));

	/* Startup process exits at this point */

	return OK;
}


/*------------------------------------------------------------------------
 *
 * sysinit  -  Initialize all Xinu data structures and devices
 *
 *------------------------------------------------------------------------
 */
static	void	sysinit()
{
	int32	i;
	struct	procent	*prptr;		/* Ptr to process table entry	*/
	struct	sentry	*semptr;	/* Ptr to semaphore table entry	*/

	/* Reset the console */

	kprintf(CONSOLE_RESET);
	kprintf("\n%s\n\n", VERSION);

	/* Initialize the interrupt vectors */

	initevec();
	
	/* Initialize free memory list */
	
	meminit();

	/* Initialize system variables */

	/* Count the Null process as the first process in the system */

	prcount = 1;

	/* Scheduling is not currently blocked */

	Defer.ndefers = 0;

	/* Initialize process table entries free */

	for (i = 0; i < NPROC; i++) {
		prptr = &proctab[i];
		prptr->prstate = PR_FREE;
		prptr->prname[0] = NULLCH;
		prptr->prstkbase = NULL;
		prptr->prprio = 0;
		prptr->vheap_min = 0;
		prptr->vheap_max = 0;
		prptr->hsize = 0;

	}

	/* Initialize the Null process entry */	

	prptr = &proctab[NULLPROC];
	prptr->prstate = PR_CURR;
	prptr->prprio = 0;
	strncpy(prptr->prname, "prnull", 7);
	prptr->prstkbase = getstk(NULLSTK);
	prptr->prstklen = NULLSTK;
	prptr->prstkptr = 0;
	currpid = NULLPROC;
		
	/* Initialize semaphores */

	for (i = 0; i < NSEM; i++) {
		semptr = &semtab[i];
		semptr->sstate = S_FREE;
		semptr->scount = 0;
		semptr->squeue = newqueue();
	}

	/* Initialize buffer pools */

	bufinit();

	/* Create a ready list for processes */

	readylist = newqueue();


	/* initialize the PCI bus */

	pci_init();

	/* Initialize the real time clock */

	clkinit();

	for (i = 0; i < NDEVS; i++) {
		init(i);
	}
	
//*************************************************************************************************************//
//		 			PAGING RELATED OPERATION BEGINS HERE 				       //
//************************************************************************************************************//
	int return_code;
	pd_t *global_pg_dir;
	pt_t * pg_table;
	//int i;
	int ntabi = (8192 + MAX_PT_SIZE + MAX_FSS_SIZE + MAX_SWAP_SIZE) / 1024;
	float ntabf = ((float)8192 + (float)MAX_PT_SIZE + (float)MAX_FSS_SIZE + (float)MAX_SWAP_SIZE) / (float)1024;
	
	if(ntabf > (float)ntabi){
		ntabi = ntabi + 1;
	}
	
	init_pdpt_tab();
	init_fss_tab();
	init_dss_tab();
	
	return_code = create_pg_dir(NULLPROC, &global_pg_dir);
	if(return_code == SYSERR)
		kprintf("Error occured while creating directory\n");
	//kprintf("GLOBAL PAGE DIR ADDR: %d\n",global_pg_dir);
	for(i = 0; i<ntabi; i++){
		int free_frame_no;
		return_code = get_frame(PDPT_R, &free_frame_no);
		if(return_code == SYSERR)
			kprintf("Error occured while getting frames for directory\n");
		//kprintf("free_frame_no : %d\n",free_frame_no);
		pg_table = (pt_t *)((PD_PT_START + free_frame_no) * PAGE_SIZE);
		//kprintf("pg_table_addr: %d\n",pg_table);
		update_frame_tables(PDPT_R, free_frame_no, PAGE_T, 0, 0);
		init_pg_table(pg_table);
		//kprintf("pg_table_addr after: %d\n",pg_table);
		add_pgs_to_pg_table(pg_table, i);
		add_to_dir(global_pg_dir, (PD_PT_START + free_frame_no),i);
		base_addrs[i] = (PD_PT_START + free_frame_no);
	}
	
	write_cr3(proctab[NULLPROC].PDBR);
	//kprintf("\n\n\n");
	//printf("READ CR3: %d\n",read_cr3());
	set_evec(14,(unsigned long)pagefault_handler_disp);
	enable_paging();


	return;
}

int32	stop(char *s)
{
	kprintf("%s\n", s);
	kprintf("looping... press reset\n");
	while(1)
		/* Empty */;
}

int32	delay(int n)
{
	DELAY(n);
	return OK;
}
