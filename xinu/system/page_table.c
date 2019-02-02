/*page_table.c - This file contains functions for handling the page table operations */

#include <xinu.h>

syscall init_pg_table(pt_t * p_tab){
	intmask mask;
	mask = disable();
	int i;
	for(i = 0;i<NPTENT; i++){
		p_tab->pt_pres = 0;
		p_tab->pt_write = 0;
		p_tab->pt_user = 0;
		p_tab->pt_base = 0;
		p_tab->pt_dirty = 0;
	}
	restore(mask);
	return OK;

}

syscall add_pgs_to_pg_table(pt_t *p_tab_ent, int dir_num){
	intmask mask;
	mask = disable();
	int i;
	for(i = 0; i<NPTENT; i++){
		add_page(p_tab_ent, dir_num,i);
		p_tab_ent ++;
		
	}
	restore(mask);
	return OK;
}

syscall add_page(pt_t *p_tab_ent, int dir_num, int p_num){
	intmask mask;
	mask = disable();
	
	p_tab_ent->pt_pres = 1;
	p_tab_ent->pt_write = 1;
	p_tab_ent->pt_user = 0;
	p_tab_ent->pt_pwt = 0;
	p_tab_ent->pt_pcd = 0;
	p_tab_ent->pt_acc = 0;
	p_tab_ent->pt_dirty = 0;
	p_tab_ent->pt_mbz = 0;
	p_tab_ent->pt_global = 0;
	p_tab_ent->pt_avail = 1;
	p_tab_ent->pt_base = dir_num * NPDENT + p_num;
	
	restore(mask);
	return OK;


}

syscall add_v_page(pt_t * p_tab_ent, int frame_num){
	
	intmask mask;
	mask = disable();
	
	p_tab_ent->pt_pres = 1;
	p_tab_ent->pt_write = 1;
	p_tab_ent->pt_user = 0;
	p_tab_ent->pt_pwt = 0;
	p_tab_ent->pt_pcd = 0;
	p_tab_ent->pt_acc = 0;
	p_tab_ent->pt_dirty = 0;
	p_tab_ent->pt_mbz = 0;
	p_tab_ent->pt_global = 0;
	p_tab_ent->pt_avail = 1;
	p_tab_ent->pt_base = frame_num;
	
	restore(mask);
	return OK;

}
