/*
 * copy guest blob from start of bss to its intermmediate location
 *
 * NOTE: this code runs at the very start: no stack is available 
 *       and BSS has not been initialized yet
 */

    .global arm_move_guest_blob
    .extern guests_db
    .code 32
    .align  0

#include "guest_blob.h"
#include "arm_common.h"


////////////////////////////////////////////////
//arm_move_guest_blob:

    ldr r0, =__data_end__ + HAL_OFFSET
    ldr r2, [r0], #4
    ldr r2, [r0], #4

    add r1, r0, r2, LSL # 4
    ldr r3, [r0]
    add r0, r3
    ldr r3, =(HAL_PHYS_START+HAL_PHYS_SIZE)

    ldr r10, =guests_db + HAL_OFFSET
    str r2, [r10], #8
    str r3, [r10], #4       @@ pend

    ldr r4, [r1], #-4      @@ VADR
    ldr r5, [r1], #-4      @@ PSIZE
    ldr r6, [r1], #-4      @@ FWSIZE
    ldr r7, [r1], #-4	   @@ OFFSET

    sub r3, r3, r5
    mov r3, r3, LSR #20
    mov r3, r3, LSL #20
    
    stmia r10!, {r3-r7}

    sub r0, r6  @@ start of this fw
    mov r4, r0  @@ copy it!

//Pre-calculations above.
#####################################################################
//initial adress där gästen sparas:

	sub r10, r10, #20	//pstart
	str r4, [r10]		//initial address where guest is stored at pstart.
//	str r6, [r10]		//fwsize.

######################################################################
//initial adress kopiera till.

	add r5, r3, r7		//r5 = physical memory start address + offset.
	str r5, [r10, #4]	//initial address where guest is copied to at vstart.

######################################################################
//final address to copy to.

	add r6, r6, r5		//r6 = physical memory start address + guest byte count + offset.
	add r6, r7			//r6=r6+r7= physical memory start address + guest byte count + offset + offset.
	str r6, [r10, #8]	//final address where guest is copied to at psize.

######################################################################
//amount of bytes to copy.

	sub r7, r6, r5
	add r7, r7, #8
	str r7, [r10, #12]	//amount of bytes to copy at fwsize.

    mov pc, lr
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

arm_move_guest_blob:

    /* get guest header and see if it is valid */
	//__data_end__ is va = 0xf0012eac, adding HAL_OFFSET gives pa (mmu off) = 0x80012EAC
	//giving first physical address after hypervisor data, 
    ldr r0, =__data_end__ + HAL_OFFSET	//r0 = 0x80012EAC = physical address after hypervisor = first address of guest_data.S
    ldr r2, [r0], #4					//r0 updated to address of  second word of guest_data.S storing number of guests.
    ldr r3, =GUESTS_MAGIC 
    cmp r3, r2
    bne fail

    /* get guest count an veryfy */
    ldr r2, [r0], #4		//r0 updated to address of third word of guest_data.S storing __guests_data_end - __guests_table_start
    cmp r2, #0 
    ble fail

    /* r0=binary end, r1=binary start, r2=binary count, r3=pmem end */        
	//r0 = address of third word of guest_data.S. LSL #4 shifts r2 left 4 bits: 16 bytes.
	//r2 = number of guests * 16 (which covers 4 32-bit words (offset, fwsize, psize and vadr).
    add r1, r0, r2, LSL # 4		//r1 = address of VADR in guest_data.S.	
    ldr r3, [r0]				//r3 = size of guest 1 plus its four fields (offset, fwsize, psize, vadr) in bytes = 0x005DD2F8.
								//r3 = 0x005DD2F8 = 0x005DD300 - 0x00000008 = __guests_data_end - __guests_table_start
    add r0, r3					//r0 = 0x80012EAC + 0x8 + 0x005DD2F8 = 0x805F01AC = address of first byte after guest.
    ldr r3, =(HAL_PHYS_START+HAL_PHYS_SIZE)	//r3 = 0x88000000 = 0x80000000 + 0x08000000 = End physical address of RAM.

    /* save guest count and p start*/     
    ldr r10, =guests_db + HAL_OFFSET		//Physical address of guests_db structure.
    str r2, [r10], #8						//Store number of guests in guests_db.count, and then increment r10 by 8.
    str r3, [r10], #4       @@ pend			//Stores physical end address of RAM in guests_db.pend, and increments r10 by 4.
											//r10 now points to first field (pstart) of guest_binary of first guest.
foreach_guest:
    /* get size & padr. make sure size is 4 alligned */
	//r1 points to first byte after current guest's four fields (see guest_data.S).
    ldr r4, [r1], #-4      @@ VADR			//r4 = virtual address = 0xC0000000, and afterwards decrement r1 by 4.
    ldr r5, [r1], #-4      @@ PSIZE			//r5 = physical memory size = 0x07000000, and afterwards decrement r1 by 4.
    ldr r6, [r1], #-4      @@ FWSIZE		//r6 = raw binary guest byte size = 0x005DD2E4, and afterwards decrement r1 by 4.
											//r6 = 0x005DD2E4 = 0x005DD300 - 0x0000001C = __guests_data_end - __guests_data_start 
    ldr r7, [r1], #-4	   @@ OFFSET		//r7 = offset = 0x00010000, and afterwards decrement r1 by 4.

    /* calc pstart, align to MB */
    sub r3, r3, r5	//r3 = 0x81000000 = 0x88000000 - 0x07000000 =
					//guest physical memory start address = physical end memory address - size of guest physical memory.
    mov r3, r3, LSR #20						//Clear lower 20 bits.
    mov r3, r3, LSL #20						//r3 = 0x8100_0000

    /* update guest db */
	//r10 points to first field (pstart) of guest_binary of first guest.
	//Store physical start address, virtual start address, physical memory size,
	//guest size, offset, to the fields pstart, vstart, psize, fwsize and
	//offset.
    stmia r10!, {r3-r7}

    /* copy guest fw to start of the physical memory of this guest */
	//r4=r0= r0 - r6 = physical address of first byte after guest - byte size of guest = physical address of first byte of guest.
    sub r0, r6  @@ start of this fw			//r0 = 0x80012EC8 = 0x805F01AC - 0x005DD2E4
    mov r4, r0  @@ copy it!					//r4 = 0x80012EC8 = physical address of first byte of guest.
    add r5, r3, r7  @@ pmem start + offset	//r5 = 0x81010000 = 0x81000000 + 0x00010000
											//guest physical memory start address +                    offset.
    add r6, r6, r5  @@ pmem end				//r6 = 0x815ED2E4 = 0x005DD2E4 + 0x81010000 = 
											//guest physical memory start address + guest byte count + offset.
    add r6, r7		@@ pmem + offset		//r6 = 0x815FD2E4 = 0x815ED2E4 + 0x00010000
											//r6=r6+r7= guest physical memory start address + guest byte count + offset + offset.
///////
/*
number of bytes to copy = guest byte count =
r6 - r5 + 8 = 0x815FD2E4 - 0x81010000 = 0x005ED2E4
guest byte count =                      0x005DD2E4
                                        0x00010000
ett offset fel.
varför två offset?
antal kopior är 8 byte till.

Correct code is blt copy_one, and remove 'add r6, r7		@@ pmem + offset' to avoid 2 offsets.
*/

//ldr r11, =guests_db + HAL_OFFSET		//Physical address of guests_db structure.
//str r6, [r11, #12]
//mov pc, lr
///////
        
copy_one:				//Move guest from physical address of guest to physical memory start address + offset.
    ldmia r4!, {r8, r9}	//Load words starting at address of first byte of guest into r8 and r9, and increment address by 8.
    stmia r5!, {r8, r9} //Store r8 and r9 into memory starting at address physical memory start address + offset, and increment 8.
    cmp r5, r6			//destination address <= physical memory start address + guest byte count + offset + offset.
    ble copy_one		//Copy more if <=.
    
    /* more guests? */
    subs r2, r2, #1
    bne foreach_guest

    /* save pstart */
    ldr r7, =guests_db + HAL_OFFSET   //Physical address of guests_db structure.
    str r3, [r7, #4]       @@ pstart //Stores physical memory start address (MB aligned) into pstart field of guests_db structure.

done:    
    mov pc, lr
    
fail:
    b .    
