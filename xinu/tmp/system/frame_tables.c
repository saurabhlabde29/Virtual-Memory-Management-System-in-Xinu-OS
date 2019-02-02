/* frame_tables.c - This file includes the functionality related to the frame_tables*/
#include <xinu.h>

syscall init_pdpt_tab(){
int i;
for(i = 0; i< MAX_PT_SIZE; i++){
	pdpt_tab[i].pid = NONE;
	pdpt_tab[i].virtual_pg_no = NOT_ASSIGNED;
	pdpt_tab[i].status = UNMAPPED;
	pdpt_tab[i].is_dirty = CLEAN;
	pdpt_tab[i].type_of_entry = NO_TYPE;
	
}
return OK;
}

syscall init_fss_tab(){
int i;
intmask mask;
mask = disable();
for(i = 0; i< MAX_FSS_SIZE; i++){
	fss_tab[i].pid = NONE;
	fss_tab[i].virtual_pg_no = NOT_ASSIGNED;
	fss_tab[i].status = UNMAPPED;
	fss_tab[i].is_dirty = CLEAN;
	fss_tab[i].type_of_entry = NO_TYPE;
}
restore(mask);
return OK;
}

syscall init_dss_tab(){
int i;
intmask mask;
mask = disable();
for(i = 0; i< MAX_SWAP_SIZE; i++){
	dss_tab[i].pid = NONE;
	dss_tab[i].virtual_pg_no = NOT_ASSIGNED;
	dss_tab[i].status = UNMAPPED;
	dss_tab[i].is_dirty = CLEAN;
	dss_tab[i].type_of_entry = NO_TYPE;
}
restore(mask);
return OK;
}

syscall update_frame_tables(int table, int index , int type, uint32 virtual_pg_num, pid32 pid){
intmask mask;
mask = disable();
	if(table == PDPT_R){
		pdpt_tab[index].pid = getpid();
		pdpt_tab[index].status = MAPPED;
		pdpt_tab[index].type_of_entry = type; 
		restore(mask);
		return OK;
	}
	else if(table == FSS_R){
		fss_tab[index].pid = getpid();
		fss_tab[index].status = MAPPED;
		fss_tab[index].type_of_entry = type;
		fss_tab[index].virtual_pg_no = virtual_pg_num;
		num_of_free_ffs_frames--;
		restore(mask);
		return OK;
	}
	else if(table == DSS_R){
		dss_tab[index].pid = pid;
		dss_tab[index].status = MAPPED;
		dss_tab[index].type_of_entry = type;
		dss_tab[index].virtual_pg_no = virtual_pg_num;
		restore(mask);
		return OK;
	}
	restore(mask);
	return OK;

	
}























