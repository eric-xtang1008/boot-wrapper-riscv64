/*
 * Copyright (c) 2021. Xing Tang. All Rights Reserved.
 */
#include <stdint.h>
#include "htif_uart.h"
#include "bits.h"

#define GET_RD(insn) \
    (uintptr_t *)((uintptr_t)regs + ((insn >> 7) & 0x1F) * REGBYTES)

static inline uintptr_t mstaus_readset(uintptr_t new)
{
	uintptr_t __mstatus;

	asm("csrrs %[mstatus], mstatus, %[mprv]\n"
	    : [mstatus] "=&r"(__mstatus)
	    : [mprv] "r"(new)
	    : );

	return __mstatus;
}

static inline void mstaus_restore(uintptr_t old)
{
	asm("csrw mstatus, %[mstatus]\n"::[mstatus] "r"(old):);
}

static uintptr_t get_insn(uintptr_t mepc)
{
	register uintptr_t __mstatus_adjust = MSTATUS_MPRV | MSTATUS_MXR;
	register uintptr_t __mstatus;
	register uintptr_t __mepc = mepc;
	uint8_t i, val[4] = { 0 };
	uintptr_t insn = 0;
	__mstatus = mstaus_readset(__mstatus_adjust);
	asm("lbu %0, 0(%[addr])\n"
	    "lbu %1, 1(%[addr])\n"
	    "lbu %2, 2(%[addr])\n"
	    "lbu %3, 3(%[addr])\n"
	    : "=r"(val[0]), "=r"(val[1]), "=r"(val[2]), "=r"(val[3])
	    : [addr] "r"(__mepc)
	    : );
	mstaus_restore(__mstatus);

	for (i = 0; i < 4; i++)
		insn = insn | ((val[i] & 0xFF) << i * 8);
	return insn;
}

static inline uint8_t load_byte(const uintptr_t addr)
{
	uintptr_t __mstatus_adjust = MSTATUS_MPRV;
	uintptr_t __mstatus;
	uint8_t val;
	__mstatus = mstaus_readset(__mstatus_adjust);
	asm("lbu %0, 0(%1)\n":"=&r"(val) : "r"(addr) :);
	mstaus_restore(__mstatus);
	return val;
}

static inline uint8_t store_byte(const uintptr_t addr, uint8_t val)
{
	uintptr_t __mstatus_adjust = MSTATUS_MPRV;
	uintptr_t __mstatus;
	__mstatus = mstaus_readset(__mstatus_adjust);
	asm("sb %0, 0(%1)\n" : :"r"(val), "r"(addr) : );
	mstaus_restore(__mstatus);
	return val;
}

union endian_byte {
	uint8_t bytes[8];
	uintptr_t intx;
	uint32_t int32;
	uint64_t int64;
};

void load_misaligned_handler(uintptr_t mcause,
			     uintptr_t mepc, 
			     uintptr_t tval, 
			     uintptr_t * regs)
{
	uintptr_t insn = get_insn(mepc);
	union endian_byte val;
	int len, i;
	int shift = 0;				/* for signed externed */
	switch (insn & MASK_LOAD) {
	case MATCH_LW:
		len = 4;
		shift = 8 * (sizeof(uintptr_t) - len);
		break;
	case MATCH_LWU:
		len = 4;
		break;
	case MATCH_LD:
		len = 8;
		shift = 8 * (sizeof(uintptr_t) - len);
		break;
	case MATCH_LH:
		len = 2;
		shift = 8 * (sizeof(uintptr_t) - len);
		break;
	case MATCH_LHU:
		len = 2;
		break;
	default:
		rv_printf(" unhandlable trap mcause: 0x%x\n", mcause);
		return;
	}

	val.int64 = 0;
	for (i = 0; i < len; i++)
		val.bytes[i] = load_byte(tval + i);

	*GET_RD(insn) = (intptr_t) val.intx << shift >> shift;

	/*
	   rv_printf("misaligned_load_trap epc:0x%x insn:0x%x, val:0x%lx\n",
	   mepc, get_insn(mepc), val.int64);
	 */
}

void store_misaligned_handler(uintptr_t mcause,
			      uintptr_t mepc, uintptr_t tval, uintptr_t * regs)
{
	uintptr_t insn = get_insn(mepc);
	union endian_byte val;
	int len, i;
	val.intx =
		*(uintptr_t *) ((uintptr_t) regs + ((insn >> 20) & 0x1F) * REGBYTES);

	switch (insn & MASK_STORE) {
	case MATCH_SH:
		len = 2;
		break;
	case MATCH_SW:
		len = 4;
		break;
	case MATCH_SD:
		len = 8;
		break;
	default:
		rv_printf(" unhandlable trap mcause: 0x%x\n", mcause);
		return;
	}

	for (i = 0; i < len; i++)
		store_byte(tval + i, val.bytes[i]);
}

void illegal_insn_handler(uintptr_t cause,
			  uintptr_t epc, uintptr_t tval, uintptr_t * regs)
{
	uintptr_t insn = tval;
	if ((insn & 0x7F) == 0x73) {
		switch ((insn >> 12) & 0x3) {
		case 2:
			/* only process intstrution: rdtime rd */
			if (((insn >> 20) & 0xFFF) == 0xC01) {
				*GET_RD(insn) = *(uintptr_t *) (0x0200bff8);
				//rv_printf("rd val: 0x%x\n", *GET_RD(insn));
			} else
				rv_printf("unhandler csr instrution: 0x%x\n", insn);
			return;
		case 1:
		case 3:
		case 5:
		case 6:
		case 7:
			rv_printf("unhandler csr instrution: 0x%x\n", insn);
			return;
		default:
			rv_printf(" unknown instruction: 0x%x\n", tval);
			return;
		}
	}

	rv_printf(" unknown instruction: 0x%x\n", tval);
}

static uintptr_t ecall_set_timer(uint64_t time)
{
	*(uintptr_t *) (0x02004000) = time;
	clear_csr(mip, 1 << 5);		/* Supervisor timer interrupt */
	set_csr(mie, 1 << 7);		/* Machine timer interrupt */
	return 0;
}

/* reference linux/arch/riscv/include/asm/sbi.h */
void ecall_handler(uintptr_t mcause,
		   uintptr_t epc, uintptr_t tval, uintptr_t * regs)
{
	uintptr_t type = regs[17], arg0 = regs[10];
	uintptr_t retval = 0;
	switch (type) {
	case SBI_CONSOLE_PUTCHAR:
		htif_console_putchar(arg0);
		break;
	case SBI_CONSOLE_GETCHAR:
		retval = htif_console_getchar();
		break;
	case SBI_SET_TIMER:
		retval = ecall_set_timer(arg0);
		break;
	default:
		retval = -1;
		break;
	}
	regs[10] = retval;
}

static void mtimer_interrupt_handler(uintptr_t epc)
{
	write_csr(mepc, epc - 4);
	clear_csr(mie, 1 << 7);		/* Machine timer interrupt */
	set_csr(mip, 1 << 5);		/* Supervisor timer interrupt */
}

void mhandle_trap(uintptr_t cause,
		  uintptr_t epc, uintptr_t tval, uintptr_t * regs)
{

	if (cause & MCAUSE_INT) {

		switch (cause << 1) {
		case (7 << 1):
			//rv_printf("timer: 0x%lx\n", cause);
			mtimer_interrupt_handler(epc);
			break;
		default:
			rv_printf(" Unknown interrupt type: 0x%x\n", cause);
			break;
		}
	} else {
		switch (cause) {
		case 0:
			//print("0: Instruction address misaligned\n");
			break;
		case 1:
			//print("1: Instruction access fault\n");
			break;
		case 2:
			//rv_printf("2: Illegal instruction - 0x%x\n", tval);
			illegal_insn_handler(cause, epc, tval, regs);
			break;
		case 3:
			//print("3: Breakpoint\n");
			break;
		case 4:
			//rv_printf("4: Load address misaligned - 0x%x\n", tval);
			load_misaligned_handler(cause, epc, tval, regs);
			break;
		case 5:
			//print("5: Load access fault\n");
			break;
		case 6:
			//print("6: Store/AMO address misaligned\n");
			store_misaligned_handler(cause, epc, tval, regs);
			break;
		case 7:
			//print("7: Store/AMO access fault\n");
			break;
		case 8:
			//print("8: Environment call from U-mode\n");
			break;
		case 9:
			//print("9: Environment call from S-mode\n");
			ecall_handler(cause, epc, tval, regs);
			break;
		case 10:
			//print("10: reserved\n");
			break;
		case 11:
			//print("11: Environment call from M-mode\n");
			break;
		case 12:
			//print("12: Instruction page fault\n");
			break;
		case 13:
			//print("13: Load page fault\n");
			break;
		default:
			rv_printf("unexpected Exception: 0x%lx\n", cause);
			break;
		}
	}
}
