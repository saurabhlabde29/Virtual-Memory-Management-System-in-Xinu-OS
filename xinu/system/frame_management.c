/*frame_management.c - This file has functions maning the frames  */

#include <xinu.h>
#include <stdlib.h>
syscall check_dss(uint32 vpn, pid32 pid, int *frame_no){
	int i;
	intmask mask;
	mask = disable();
	*frame_no = -1;
	for(i = 0; i<MAX_SWAP_SIZE; i++){
		if(dss_tab[i].virtual_pg_no == vpn && dss_tab[i].pid == pid){
			*frame_no = i;
			restore(mask);
			return OK;
		}
	}
	restore(mask);
	return SYSERR;
}

int32 get_frame_from_dss(){
	int i;
	for(i = 0; i<MAX_SWAP_SIZE; i++){
		if(dss_tab[i].status == UNMAPPED){
			return i;
		}
	}
	return -1;
}

int32 find_clean_pg_fss(uint32 swap_victim, uint32 vpn, pid32 pid){
	int i;
	for(i = 0; i<MAX_FSS_SIZE; i++){
		if(fss_tab[i].virtual_pg_no == vpn && fss_tab[i].pid == pid){
			unsigned long pdbr_fss_pg = proctab[fss_tab[i].pid].PDBR;
			pd_t * fss_pg_dir_ent = (pd_t *) (pdbr_fss_pg);
			unsigned long temp_fss = fss_tab[i].virtual_pg_no << 12;
			virt_addr_t * v_addr_fss = (virt_addr_t *)&temp_fss; //FIXME
			uint32 pd_offset_fss = v_addr_fss->pd_offset;
			uint32 pt_offset_fss = v_addr_fss->pt_offset;

			pt_t * fss_pg_table_ent;
			fss_pg_table_ent = (pt_t *)(((fss_pg_dir_ent+pd_offset_fss)->pd_base * PAGE_SIZE))+ pt_offset_fss;
			if(fss_pg_table_ent->pt_dirty == 0)
			{
				return swap_victim;
			}
		}
	}
	return -1;
}

int32 find_swap_victim(){
	int32 i;
	int32 swap_victim;
	for(i = 0; i<MAX_SWAP_SIZE; i++){
		swap_victim = find_clean_pg_fss(i, dss_tab[i].virtual_pg_no, dss_tab[i].pid);
		if(swap_victim != -1){
			return swap_victim;
		}
	}
	return -1;
}

syscall get_frame(uint32 region, int *free_frame_no){
int i;
intmask mask;
mask = disable();
*free_frame_no = -1;
if(region == PDPT_R){
	for(i = 0;i<MAX_PT_SIZE; i++){
		if(pdpt_tab[i].status == UNMAPPED){
			*free_frame_no = i;
			restore(mask);
			return OK;
		}
	}
	
	kprintf("Out of space in PDPT region");
	restore(mask);
	return SYSERR;
}
else if(region == FSS_R){
	for(i = 0;i<MAX_FSS_SIZE; i++){
		if(fss_tab[i].status == UNMAPPED){
			*free_frame_no = i;
			restore(mask);
			return OK;
		}
	}
	
	//SWAP LOGIC
	// Select random frame from FSS to be evicted
	uint32 victim = rand() % MAX_FSS_SIZE;

	unsigned long pdbr_victim = proctab[fss_tab[victim].pid].PDBR;
	pd_t * victim_pg_dir_ent = (pd_t *) (pdbr_victim);
	unsigned long temp = fss_tab[victim].virtual_pg_no << 12;
	virt_addr_t * v_addr = (virt_addr_t *)&temp; //FIXME
	uint32 pd_offset = v_addr->pd_offset;
	uint32 pt_offset = v_addr->pt_offset;

	pt_t * pg_table_ent;
	pg_table_ent = (pt_t *)(((victim_pg_dir_ent+pd_offset)->pd_base * PAGE_SIZE))+ pt_offset;

	// Check Swap space for the same page that is selected from the FSS region for eviction
    int32 swap_pg; 
    check_dss(fss_tab[victim].virtual_pg_no, fss_tab[victim].pid, &swap_pg);

    int32 free_swap_pg;

    int32 swap_victim, swap_victim_temp;

    if(swap_pg == -1){ // Page does not exist in SWAP space
    	free_swap_pg = get_frame_from_dss();
    	if(free_swap_pg != -1){
    		bcopy((FFS_START + victim)<<12, (DSS_START + free_swap_pg)<<12, PAGE_SIZE);
    		pg_table_ent->pt_pres = 0;
    		pg_table_ent->pt_base = DSS_START + free_swap_pg;
    		update_frame_tables(DSS_R, free_swap_pg, PAGE, fss_tab[victim].virtual_pg_no, fss_tab[victim].pid);
    		*free_frame_no = victim;
			restore(mask);
			return OK;
    	}
    	else{ // SWAP space is full
    		swap_victim_temp = find_swap_victim();
    		if(swap_victim_temp != -1){
    			swap_victim = swap_victim_temp;
    		}
    		/*
    		else{
    			swap_victim = rand() % MAX_SWAP_SIZE;
    			unsigned long pdbr_swap_pg = proctab[dss_tab[swap_victim].pid].PDBR;
				pd_t * swap_pg_dir_ent = (pd_t *) (pdbr_swap_pg);
				unsigned long temp_swap = dss_tab[swap_victim].virtual_pg_no << 12;
				virt_addr_t * v_addr_swap = (virt_addr_t *)&temp_swap; //FIXME
				uint32 pd_offset_swap = v_addr_swap->pd_offset;
				uint32 pt_offset_swap = v_addr_swap->pt_offset;

				pt_t * swap_pg_table_ent;
				swap_pg_table_ent = (pt_t *)(((swap_pg_dir_ent+pd_offset_swap)->pd_base * PAGE_SIZE))+ pt_offset_swap;
				swap_pg_table_ent->pt_pres = 0;
				swap_pg_table_ent->pt_dirty = 0;
				swap_pg_table_ent->pt_base = 0;
    		}
    		
		*/
		bcopy((FFS_START + victim)<<12, (DSS_START + swap_victim)<<12, PAGE_SIZE);
    		pg_table_ent->pt_pres = 0;
    		pg_table_ent->pt_base = DSS_START + swap_victim;
    		update_frame_tables(DSS_R, swap_victim, PAGE, fss_tab[victim].virtual_pg_no, fss_tab[victim].pid);
    		*free_frame_no = victim;
			restore(mask);
			return OK;
    	}
    }
    else{
    	if(pg_table_ent->pt_dirty == 1){
    		bcopy((FFS_START + victim)<<12, (DSS_START + swap_pg)<<12, PAGE_SIZE);
    		pg_table_ent->pt_pres = 0;
    		pg_table_ent->pt_base = DSS_START + swap_pg;
    		update_frame_tables(DSS_R, swap_pg, PAGE, fss_tab[victim].virtual_pg_no, fss_tab[victim].pid);
    		*free_frame_no = victim;
			restore(mask);
			return OK;
    	}
    	else{
    		pg_table_ent->pt_pres = 0;
    		pg_table_ent->pt_base = DSS_START + swap_pg;
    		update_frame_tables(DSS_R, swap_pg, PAGE, fss_tab[victim].virtual_pg_no, fss_tab[victim].pid);
    		*free_frame_no = victim;
			restore(mask);
			return OK;
    	}
    }

	kprintf("Out of space in FSS region");
	restore(mask);
	return SYSERR;
}
restore(mask);
return OK;

}

syscall clean_frames(pid32 pid){
	intmask mask;
	mask = disable();
	int i;
	
	for(i = 0; i<MAX_PT_SIZE; i++){
		if(pdpt_tab[i].pid == pid){
			pdpt_tab[i].pid = NONE;
			pdpt_tab[i].virtual_pg_no = NOT_ASSIGNED;
			pdpt_tab[i].status = UNMAPPED;
			pdpt_tab[i].is_dirty = CLEAN;
			pdpt_tab[i].type_of_entry = NO_TYPE;		
		}	
	}
	
	
	
	for(i = 0; i<MAX_FSS_SIZE; i++){
		if(fss_tab[i].pid == pid){
			fss_tab[i].pid = NONE;
			fss_tab[i].virtual_pg_no = NOT_ASSIGNED;
			fss_tab[i].status = UNMAPPED;
			fss_tab[i].is_dirty = CLEAN;
			fss_tab[i].type_of_entry = NO_TYPE;		
		}	
	}
	
	for(i = 0; i<MAX_SWAP_SIZE; i++){
		if(dss_tab[i].pid == pid){
			dss_tab[i].pid = NONE;
			dss_tab[i].virtual_pg_no = NOT_ASSIGNED;
			dss_tab[i].status = UNMAPPED;
			dss_tab[i].is_dirty = CLEAN;
			dss_tab[i].type_of_entry = NO_TYPE;		
		}	
	}
	
	
	restore(mask);
	return OK;


}

syscall clean_selected_frames(pid32 pid, int v_pg_num){
	intmask mask;
	mask = disable();
	int i ;
	
	for(i = 0; i<MAX_FSS_SIZE; i++){
		if(fss_tab[i].pid == pid && fss_tab[i].virtual_pg_no == v_pg_num){
			fss_tab[i].pid = NONE;
			fss_tab[i].virtual_pg_no = NOT_ASSIGNED;
			fss_tab[i].status = UNMAPPED;
			fss_tab[i].is_dirty = CLEAN;
			fss_tab[i].type_of_entry = NO_TYPE;		
		}	
	}
	
	for(i = 0; i<MAX_SWAP_SIZE; i++){
		if(dss_tab[i].pid == pid && dss_tab[i].virtual_pg_no == v_pg_num){
			dss_tab[i].pid = NONE;
			dss_tab[i].virtual_pg_no = NOT_ASSIGNED;
			dss_tab[i].status = UNMAPPED;
			dss_tab[i].is_dirty = CLEAN;
			dss_tab[i].type_of_entry = NO_TYPE;		
		}	
	}
	
	restore(mask);
	return OK;	

}
