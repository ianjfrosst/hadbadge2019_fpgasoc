/*
 * Copyright 2019 Jeroen Domburg <jeroen@spritesmods.com>
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "custom_ops.S"

#include "../ipl/gloss/mach_defines.h"

	.global load_ipl
	.global run_ipl

	.section .text


reset_vec:
	j do_reset

.balign 16
irq_vec_entry:
	j irq_vec

.balign 16
//Interrupt handler table is at 0x20. Normally initialized with jumps to the panic handler.
//Can be modified by IPL/apps to real interrupt handlers later.
	.global gdb_panic_handler
	.global irq_table
irq_table:
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler
	.word gdb_panic_handler

.global irq_stack_ptr
irq_stack_ptr:
	.word 0x40002000

.global cart_boot_flag
cart_boot_flag:
	.word 0

do_reset:
	//see if we are the 2nd cpu
	li a1, MISC_OFFSET
	lw a2, MISC_CPU_NO(a1)
	bne a2, zero, secondcpu

	//Set up UART divider for 115200
	li a1, UART_OFFSET
	li a2, 414
	sw a2, UART_DIV_REG(a1)
	li a2, 24
	sw a2, UART_IRDA_DIV_REG(a1)

	//Check if we are running in a simulation. If so, immediately jump to app that the simulator already
	//preloaded in psram.
	li a2, MISC_OFFSET
	lw a2, MISC_SOC_VER(a2)
	srli a2, a2, 15 //mask out 15th bit
	andi a2, a2, 1
//	andi a2, a2, 0
	beq a2, zero, is_real_badge

	li a1, UART_OFFSET
	li a2, 64 //dummy for verilator
	sw a2, UART_DIV_REG(a1)

	//start lcd
	li a1, LCD_OFFSET
	li a2, LCD_CONTROL_BLEN | LCD_CONTROL_FBSTART
	sw a2, LCD_CONTROL_REG(a1)

	//irqs: enable 1 (ecall), 2 (unaligned mem), 3 (bus error) to bring us to gdbstub
	li a1, 0xffffffff-0xE
	picorv32_maskirq_insn(a1, a1)

	//Directly jump to IPL
	call run_ipl

kill:
	j kill


is_real_badge:

//Bitbang PSRAM port initialization command
psram_init:
	// Load base offset
	li a1, PSRAM_CMD_OFFSET

	// Request manual control
	li a2, 2
	sw a2, PSRAM_CMD_CSR(a1)

	// Send single command
	li a2, 0x3535
	sw a2, PSRAM_CMD_SPI_WR_16B(a1)

	// Release manual control
	li a2, 4
	sw a2, PSRAM_CMD_CSR(a1)

	// Wait for completion
1:
	lw a2, PSRAM_CMD_CSR(a1)
	andi a2, a2, 16
	beq zero, a2, 1b

	// Flush cache to psram
	li a1, MACH_FLUSH_REGION
	li a2, MACH_RAM_SIZE
	sw a2, 0(a1)

	//irqs: enable 1 (ecall), 2 (unaligned mem), 3 (bus error) to bring us to gdbstub
	li a1, 0xffffffff-0xE
	picorv32_maskirq_insn(a1, a1)

	call load_ipl
	call run_ipl

jtagwait:
	li a1, 0x40000000
	lw a3, 0(a1)
	li a2, 0xdeadbeef
	bne a3, a2, jtagwait

	li a1, 0x40002008 //ipl image has 2 words of header
	jalr zero, a1, 0

	//break to gdbstub
	li a1, 0x40002000
	li a2, 0x00100073 //ebreak
	sw a2, 0(a1)
	jalr zero, a1, 0


secondcpu:
secondcpuhang:
	j secondcpuhang
