/* directory.c - This file has functions handling the directory operations */

#include <xinu.h>

syscall create_pg_dir(pid32 pid, pd_t ** g_pg_dir){
	intmask mask;
	mask = disable();
	pd_t *page_dir;
	int return_code;
	int i;
	int avail_frame;
	return_code = get_frame(PDPT_R , &avail_frame);
	if(return_code == SYSERR){
		restore(mask);
		return SYSERR;
	}
	//kprintf("free frame %d \n\n",avail_frame);
	page_dir = (pd_t *)((PD_PT_START + avail_frame) * PAGE_SIZE);
	//kprintf("PAGE DIR ADDR: %d\n\n",page_dir);
	*g_pg_dir = page_dir;
	//kprintf("Inside function Gdir: %d\n",*g_pg_dir);
	proctab[pid].PDBR = (PD_PT_START + avail_frame) * PAGE_SIZE;
	//kprintf("PDBR : %lu \n",proctab[pid].PDBR);
	update_frame_tables(PDPT_R, avail_frame, PAGE_D, 0, 0);
	init_directory(page_dir);
	//kprintf("PAGE DIR ADDR after: %d\n\n",page_dir);
	restore(mask);
	return OK;
}

syscall init_directory(pd_t *dir){
	intmask mask;
	mask = disable();
	//pd_t *dir = p_dir;
	int i;
	for(i = 0; i<NPDENT; i++){
		dir->pd_pres = 0;
		dir->pd_base = 0;
		dir->pd_user = 0;
		dir->pd_write = 0;
		dir->pd_avail = 0;
		dir++;
	}
	restore(mask);
	return OK;
}

syscall add_to_dir(pd_t * pg_dir, uint32 base_address, int offset){
	intmask mask;
	mask = disable();
	
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
  	(pg_dir+offset)->pd_base = base_address;
  	
  	restore(mask);
  	return OK;	

}
