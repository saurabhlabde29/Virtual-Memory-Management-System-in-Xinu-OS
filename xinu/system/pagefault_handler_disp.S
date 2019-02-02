/* pagefault_handler_disp.S - The pagefault handler routine which calls a high level function */


      .text
pferrcode: .long 0
           .globl  pagefault_handler_disp,pferrcode 
pagefault_handler_disp:
    popl pferrcode 			/* store the error code     */
    pushfl         			/* store flag reg           */
    cli            			/* interrupt disabled       */
    pushal         			/* save all general regs    */
    call pagefault_handler		/* call paging fault ISR    */
    popal          			/* restore all general regs */
    popfl          			/* restore flag reg         */
    iret
