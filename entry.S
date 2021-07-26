/*
 * Copyright (c) 2021. Xing Tang. All Rights Reserved.
 */

        .section .entry, "ax", @progbits
        .align 3
        .global _start
_start:
        li ra, 0
        call _reset_regs

        /* set mtvec and make sure it sticks */
        la t0, trap_entry
        csrw mtvec, t0

        /* mask all interrupts */
        csrw mie, zero

        /* disable multicore */
1:      csrr a0, mhartid
        bnez a0, 1b

#define RISCV_PGSHIFT 12
#define RISCV_PGSIZE (1 << RISCV_PGSHIFT)

        /* Setup temporary stack for M mode */
        la sp, _stack + RISCV_PGSIZE

     /* copy dtb frome 0x1020 to _payload_end*/
        lb a4, 4(a1)
        lb a5, 5(a1)
        lb a6, 6(a1)
        lb a7, 7(a1)

        slli a4, a4, 24
        slli a5, a5, 16
        slli a6, a6, 8
        add a4, a4, a5
        add a4, a4, a6
        add s3, a4, a7

        mv s4, a1
        la a0, _dtb_base
        mv s1, a0
1:      ld s2, 0(s4)
        addi s4, s4, 8
        sd s2, 0(s1)
        addi s1, s1, 8
        addi s3, s3, -8
        bgez s3, 1b

        /* send S-mode interrupts and most exceptions straight to S-mode */

        li  t1, (1 << 1 | 1 << 5 | 1 << 9)
        csrw mideleg, t1

        li  t1, (0x1 << 0 | 0x1 << 0x3 |0x1 << 0x8 | 0x1 << 0xc | 0x1 << 0xd | 0x1 << 0xf)
        csrw medeleg, t1


        /* change from M-mode to S-mode */

        csrr t0, mstatus
        li t1, 0x1
        slli t1, t1, 11 //MPP = 1
        or t0, t0, t1
        csrw mstatus, t0
        
        lla a0, _kernel_start
        csrw mepc, a0 
        csrw mscratch, sp /* save sp for M-mode */
        csrr a0, mhartid
        la a1, _dtb_base

        mret

        j _start_hang
        //call main

        .section .entry, "ax", %progbits
        .align 3
        .globl _reset_regs

_reset_regs:
        /* flush the instruction cache */
        fence.i
        /* Reset all registers except ra, a0, a1 and a2 */
        li sp, 0
        li gp, 0
        li tp, 0
        li t0, 0
        li t1, 0
        li t2, 0
        li s0, 0
        li s1, 0
        /* save a0 and a1 */
        li a2, 0
        li a3, 0
        li a4, 0
        li a5, 0
        li a6, 0
        li a7, 0
        li s2, 0
        li s3, 0
        li s4, 0
        li s5, 0
        li s6, 0
        li s7, 0
        li s8, 0
        li s9, 0
        li s10, 0
        li s11, 0
        li t3, 0
        li t4, 0
        li t5, 0
        li t6, 0
        csrw mscratch, 0

        ret

        .section .entry, "ax", %progbits
        .align 3
        .globl _start_hang
_start_hang:
        wfi
        j       _start_hang

  .bss
  .align RISCV_PGSHIFT
_stack:
  .skip 3*RISCV_PGSIZE 
