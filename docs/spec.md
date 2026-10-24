# vm-x86 — Architecture Specification

## Overview

A software emulator of a 32-bit IA-32 (x86) CPU subset, written in C.
Goal: eventually boot and run a custom OS kernel.

Development is staged. This document tracks what is implemented per phase.

---

## Register Set (IA-32)

### General-Purpose Registers (32-bit)

| Register | Alias | Convention |
|----------|-------|------------|
| EAX | AX/AH/AL | Accumulator |
| EBX | BX/BH/BL | Base |
| ECX | CX/CH/CL | Counter |
| EDX | DX/DH/DL | Data |
| ESI | SI | Source index |
| EDI | DI | Destination index |
| ESP | SP | Stack pointer |
| EBP | BP | Base pointer |

### Special Registers

| Register | Width | Purpose |
|----------|-------|---------|
| EIP | 32-bit | Instruction pointer |
| EFLAGS | 32-bit | Flags (CF, ZF, SF, OF, PF, AF, DF, IF, TF) |

### Segment Registers (stubbed in Phase 1)

CS, DS, SS, ES, FS, GS — 16-bit each. Flat model in Phase 1 (base=0, limit=4GB).

---

## Memory Model

### Phase 1: Flat

- Address space: 32-bit (4 GB addressable)
- Physical RAM: 4 MB (configurable at init)
- Layout:
  ```
  0x00000000 – 0x000003FF   Real-mode IVT (reserved)
  0x00000400 – 0x000004FF   BIOS Data Area (reserved)
  0x00000500 – 0x0007FFFF   Free for use (programs, stack)
  0x000A0000 – 0x000BFFFF   VGA framebuffer (future)
  0x000C0000 – 0x000FFFFF   BIOS ROM (future)
  ```
- Access: byte, word (16-bit), dword (32-bit)
- Out-of-range access: raises fault (halts in Phase 1)

---

## Instruction Set Subset

### Phase 1 — Core Instructions

| Opcode | Mnemonic | Description |
|--------|----------|-------------|
| 0x90 | NOP | No operation |
| 0xF4 | HLT | Halt execution |
| 0xB8–0xBF | MOV r32, imm32 | Load immediate into register |
| 0x89 | MOV r/m32, r32 | Register to register/memory |
| 0x8B | MOV r32, r/m32 | Register/memory to register |
| 0x01 | ADD r/m32, r32 | Add |
| 0x29 | SUB r/m32, r32 | Subtract |
| 0x50–0x57 | PUSH r32 | Push register onto stack |
| 0x58–0x5F | POP r32 | Pop from stack into register |
| 0xE9 | JMP rel32 | Near relative jump |
| 0xFF /4 | JMP r/m32 | Near absolute indirect jump |
| 0xCD | INT imm8 | Software interrupt |
| 0xCF | IRET | Interrupt return (stub) |

### ModRM Byte (used by 0x89, 0x8B, 0x01, 0x29)

```
7  6  | 5  4  3 | 2  1  0
  mod |   reg   |   r/m
```

Phase 1 supports: mod=11 (register direct) only. Memory addressing deferred to Phase 2.

---

## Fetch / Decode / Execute Cycle

```
while (!halted) {
    fetch:   opcode = mem[EIP]; EIP++
    decode:  determine instruction length, read operands
    execute: perform operation, update EFLAGS, update EIP
}
```

---

## EFLAGS Bits (Phase 1)

| Bit | Name | Set when |
|-----|------|---------|
| 0 | CF | Carry |
| 2 | PF | Parity |
| 4 | AF | Auxiliary carry |
| 6 | ZF | Result is zero |
| 7 | SF | Result is negative |
| 8 | TF | Trap (single-step) |
| 9 | IF | Interrupts enabled |
| 11 | OF | Signed overflow |

---

## Phase Roadmap

| Phase | Focus |
|-------|-------|
| 1 (current) | Registers, flat memory, core instructions, HLT |
| 2 | Full ModRM (memory addressing), LEA, CMP, Jcc, CALL/RET |
| 3 | Stack frames, segment registers, protected mode switch |
| 4 | Interrupts, IDT, PIC emulation |
| 5 | Paging, CR0/CR3, page fault |
| 6 | Load and run a minimal OS kernel |
