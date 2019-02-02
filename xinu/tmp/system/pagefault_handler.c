/* pagefault_handler.c - The high level page fault handler */
#include <xinu.h>

syscall check_for_seg_fault(unsigned long fault_addr){
	//intmask mask;
	//mask = disable();
	vmemblk_t *vmemlist = proctab[currpid].vmemlist;
	vmemblk_t *curr = vmemlist->mnext;
	if(fault_addr > proctab[currpid].vheap_max){
		kprintf("EXCEEDING VIRTUAL HEAP SPACE\n");
		return SYSERR;
	}
	while(curr != NULL){
		if(fault_addr >= curr->vmaddr && fault_addr < (curr->vmaddr + curr->mlength)){
			kprintf("BLOCK NEVER ALLOCATED\n");
			return SYSERR;
		}
		curr = curr->mnext;
	}
	//kprintf("NO SEGMENTATION FAULT\n");
	return OK;
}

int check_dir_entry_exists(pd_t * dir, uint32 offset){
	if((dir+offset)->pd_pres == 0)
		return 0;
	else
		return 1;
}

interrupt pagefault_handler(){
	intmask mask;
	mask = disable();
	//kprintf("Yo whats up.? This is the pagefault handler. Nice to meet you.!!\n\n");
	
	write_cr3(proctab[NULLPROC].PDBR);
	
	unsigned long fault_address = read_cr2();
	virt_addr_t * virt_addr = (virt_addr_t *)&fault_address;
	uint32 pd_offset = virt_addr->pd_offset;
	uint32 pt_offset = virt_addr->pt_offset;
	uint32 pg_offset = virt_addr->pg_offset;
	uint32 virtual_pg_num = fault_address >> 12;
	//kprintf("VIrtual Page number : %d\n",virtual_pg_num);
	pt_t * pg_table;
	unsigned long pdbr = proctab[currpid].PDBR;
	pd_t * page_dir_ent = (pd_t *)(pdbr);
	int return_code;
	int pg_dir_exists;
	int free_frame;
	int free_ffs_frame;
	int swap_frame;
	
	
	
	return_code = check_for_seg_fault(fault_address);
	if(return_code == SYSERR){
		kprintf("SEGMENTATION_FAULT\n");
		restore(mask);
		kill(currpid);
		
	}
	pg_dir_exists = check_dir_entry_exists(page_dir_ent , pd_offset);
	
	if(pg_dir_exists == 0){
		
		return_code = get_frame(PDPT_R , &free_frame);
		if(return_code == SYSERR){
			kprintf("Could not get a frame for PT");
			restore(mask);
			return ;
		}
		pg_table = (pt_t *)((PD_PT_START + free_frame) * PAGE_SIZE);
		update_frame_tables(PDPT_R, free_frame, PAGE_T, 0, 0);
		init_pg_table(pg_table);
		return_code = get_frame(FSS_R, &free_ffs_frame);
		if(return_code == SYSERR) {
			kprintf("Could not get frame for virtual heap\n");
			restore(mask);
			return ;
		}
		update_frame_tables(FSS_R, free_ffs_frame, PAGE, virtual_pg_num, 0);
		add_v_page(pg_table , (FFS_START + free_ffs_frame));
		add_to_dir(page_dir_ent , (PD_PT_START + free_frame), pd_offset);
	}
	else{
		pg_table = (pt_t *)(((page_dir_ent+pd_offset)->pd_base * PAGE_SIZE))+ pt_offset;
		if(pg_table->pt_pres == 0 && pg_table->pt_dirty == 0){
			return_code = get_frame(FSS_R, &free_ffs_frame);
			if(return_code == SYSERR) {
				kprintf("Could not get frame for virtual heap\n");
				restore(mask);
				return ;
			}
			update_frame_tables(FSS_R, free_ffs_frame, PAGE, virtual_pg_num, 0);
			add_v_page(pg_table , (FFS_START + free_ffs_frame));
		}
		else if(pg_table->pt_pres == 0 && pg_table->pt_dirty == 1){
			check_dss(virtual_pg_num, currpid, &swap_frame);
			if(swap_frame == -1) {
				kprintf("Frame not found in SWAP SPACE\n");
				restore(mask);
				return ;
			}
			return_code = get_frame(FSS_R, &free_ffs_frame);
			if(return_code == SYSERR) {
				kprintf("Could not get frame for virtual heap\n");
				restore(mask);
				return ;
			}
			bcopy((DSS_START + swap_frame)<<12, (FFS_START + free_ffs_frame)<<12, PAGE_SIZE);
			update_frame_tables(FSS_R, free_ffs_frame, PAGE, virtual_pg_num, 0);
			add_v_page(pg_table , (FFS_START + free_ffs_frame));
		}

	}
	
	write_cr3(proctab[currpid].PDBR);
	restore(mask);
	//return OK;

}
