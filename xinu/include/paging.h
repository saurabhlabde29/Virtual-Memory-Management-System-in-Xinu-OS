/* paging.h */
#ifndef _PAGING_H_
#define _PAGING_H_

typedef unsigned int	 bsd_t;
typedef unsigned long    pdbr_t;
/* Structure for a page directory entry */

typedef struct {

  unsigned int pd_pres	: 1;		/* page table present?		*/
  unsigned int pd_write : 1;		/* page is writable?		*/
  unsigned int pd_user	: 1;		/* is use level protection?	*/
  unsigned int pd_pwt	: 1;		/* write through cachine for pt?*/
  unsigned int pd_pcd	: 1;		/* cache disable for this pt?	*/
  unsigned int pd_acc	: 1;		/* page table was accessed?	*/
  unsigned int pd_mbz	: 1;		/* must be zero			*/
  unsigned int pd_fmb	: 1;		/* four MB pages?		*/
  unsigned int pd_global: 1;		/* global (ignored)		*/
  unsigned int pd_avail : 3;		/* for programmer's use		*/
  unsigned int pd_base	: 20;		/* location of page table?	*/
} pd_t;

/* Structure for a page table entry */

typedef struct {

  unsigned int pt_pres	: 1;		/* page is present?		*/
  unsigned int pt_write : 1;		/* page is writable?		*/
  unsigned int pt_user	: 1;		/* is use level protection?	*/
  unsigned int pt_pwt	: 1;		/* write through for this page? */
  unsigned int pt_pcd	: 1;		/* cache disable for this page? */
  unsigned int pt_acc	: 1;		/* page was accessed?		*/
  unsigned int pt_dirty : 1;		/* page was written?		*/
  unsigned int pt_mbz	: 1;		/* must be zero			*/
  unsigned int pt_global: 1;		/* should be zero in 586	*/
  unsigned int pt_avail : 3;		/* for programmer's use		*/
  unsigned int pt_base	: 20;		/* location of page?		*/
} pt_t;

typedef struct{
  unsigned int pg_offset : 12;		/* page offset			*/
  unsigned int pt_offset : 10;		/* page table offset		*/
  unsigned int pd_offset : 10;		/* page directory offset	*/
} virt_addr_t;

typedef struct{
  unsigned int fm_offset : 12;		/* frame offset			*/
  unsigned int fm_num : 20;		/* frame number			*/
} phy_addr_t;

// Struct to model each frame in the FSS , PD_PT and the swap region
typedef struct{
  unsigned int pid; // PID of the holder of the frame
  unsigned int virtual_pg_no; // Virtual page no of this frame
  unsigned status; // If this frame is mapped or unmapped
  unsigned int type_of_entry; // Whether this frame stores a page, a page_directory or a page_table
  unsigned int is_dirty; // Whether this frame has been written or not.
} frame_t;

typedef struct vmemblk_t{
  struct vmemblk_t *mnext;
  unsigned long vmaddr;
  unsigned long mlength;
}vmemblk_t;

#define	roundpage(x)	(char *)( (4095 + (uint32)(x)) & (~4095) )
#define	truncpage(x)	(char *)( ((uint32)(x)) & (~4095) )

/* Macros */

#define PAGE_SIZE       4096    /* number of bytes per page		 		 */
#define MAX_HEAP_SIZE   4096    /* max number of frames for virtual heap		 */
#define MAX_SWAP_SIZE   4096    /* size of swap space (in frames) 			 */
#define MAX_FSS_SIZE    2048    /* size of FSS space  (in frames)			 */
#define MAX_PT_SIZE	256	/* size of space used for page tables (in frames)	 */

#define NPDENT		1024
#define NPTENT		1024
#define STATIC_START	0
#define STATIC_END	8191
#define PD_PT_START	(STATIC_END+1)
#define PD_PT_END	(PD_PT_START+MAX_PT_SIZE-1)
#define FFS_START	(PD_PT_END+1)
#define FFS_END		(FFS_START+MAX_FSS_SIZE-1)
#define DSS_START	(FFS_END+1)
#define DSS_END		(DSS_START+MAX_SWAP_SIZE-1)
#define V_HEAP_START	8192

#define PAGE		1
#define PAGE_T		2
#define PAGE_D		3
#define NO_TYPE 	0
#define NOT_ASSIGNED	0

#define CLEAN 		0
#define DIRTY		1

#define MAPPED		1
#define UNMAPPED	0

#define NONE		150

#define PDPT_R 		0
#define FSS_R		1
#define DSS_R		2

#define NTAB		15
#define NSTATIC		8

extern frame_t pdpt_tab[];
extern frame_t fss_tab[];
extern frame_t dss_tab[];
extern int base_addrs[];

extern int num_of_free_ffs_frames;
extern int avail_heap_space;

#endif
