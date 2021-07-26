/*
 * Copyright (c) 2021. Xing Tang. All Rights Reserved.
 */
#ifndef _RISCV_BITS_H
#define _RISCV_BITS_H

#if __riscv_xlen == 64
#define MCAUSE_INT  0x8000000000000000
#define STORE    sd
#define LOAD     ld
#define REGBYTES 8
#else
#define MCAUSE_INT  0x80000000
#define STORE    sw
#define LOAD     lw
#define REGBYTES 4
#endif

#define MSTATUS_MPRV    0x00020000
#define MSTATUS_MXR     0x00080000

#define MASK_LOAD   0x707F
#define MATCH_LB    0x3
#define MATCH_LH    0x1003
#define MATCH_LW    0x2003
#define MATCH_LD    0x3003
#define MATCH_LBU   0x4003
#define MATCH_LHU   0x5003
#define MATCH_LWU   0x6003

#define MASK_STORE  0x707F
#define MATCH_SB    0x23
#define MATCH_SH    0x1023
#define MATCH_SW    0x2023
#define MATCH_SD    0x3023


/*
 * reference linux/arch/riscv/include/asm/sbi.h
 *
 *      arg0 -> a0
 *      arg1 -> a1
 *      arg2 -> a2
 *      arg3 -> a3
 *      arg4 -> a7 (this is the the ecall type)
 */ 
#define SBI_SET_TIMER 0
#define SBI_CONSOLE_PUTCHAR 1
#define SBI_CONSOLE_GETCHAR 2
#define SBI_CLEAR_IPI 3
#define SBI_SEND_IPI 4
#define SBI_REMOTE_FENCE_I 5
#define SBI_REMOTE_SFENCE_VMA 6
#define SBI_REMOTE_SFENCE_VMA_ASID 7
#define SBI_SHUTDOWN 8

#define read_csr(reg) ({ unsigned long __tmp; \
  asm volatile ("csrr %0, " #reg : "=r"(__tmp)); \
  __tmp; })

#define write_csr(reg, val) ({ \
  asm volatile ("csrw " #reg ", %0" :: "rK"(val)); })

#define swap_csr(reg, val) ({ unsigned long __tmp; \
  asm volatile ("csrrw %0, " #reg ", %1" : "=r"(__tmp) : "rK"(val)); \
  __tmp; })

#define set_csr(reg, bit) ({ unsigned long __tmp; \
  asm volatile ("csrrs %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
  __tmp; })

#define clear_csr(reg, bit) ({ unsigned long __tmp; \
  asm volatile ("csrrc %0, " #reg ", %1" : "=r"(__tmp) : "rK"(bit)); \
  __tmp; })


#endif

