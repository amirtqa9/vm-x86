#include "decode.h"
#include <stdio.h>

/* ── helpers ─────────────────────────────────────────────────────────────── */

static void fault(VM *vm, const char *msg) {
    vm->cpu->fault     = true;
    vm->cpu->fault_msg = msg;
}

/* Fetch one byte at EIP and advance EIP */
static bool fetch8(VM *vm, uint8_t *out) {
    if (!mem_read8(vm->mem, vm->cpu->eip, out)) {
        fault(vm, "fetch: out of range");
        return false;
    }
    vm->cpu->eip++;
    return true;
}

static bool fetch16(VM *vm, uint16_t *out) {
    if (!mem_read16(vm->mem, vm->cpu->eip, out)) {
        fault(vm, "fetch: out of range");
        return false;
    }
    vm->cpu->eip += 2;
    return true;
}

static bool fetch32(VM *vm, uint32_t *out) {
    if (!mem_read32(vm->mem, vm->cpu->eip, out)) {
        fault(vm, "fetch: out of range");
        return false;
    }
    vm->cpu->eip += 4;
    return true;
}

/* Decode ModRM byte — Phase 1: mod=11 (register direct) only */
static bool decode_modrm_reg_reg(VM *vm, RegIndex *reg, RegIndex *rm) {
    uint8_t modrm;
    if (!fetch8(vm, &modrm)) return false;

    uint8_t mod = (modrm >> 6) & 0x3;
    *reg = (RegIndex)((modrm >> 3) & 0x7);
    *rm  = (RegIndex)(modrm & 0x7);

    if (mod != 3) {
        fault(vm, "ModRM: memory addressing not yet implemented (Phase 2)");
        return false;
    }
    return true;
}

/* ── ADD/SUB helpers ─────────────────────────────────────────────────────── */

static uint32_t do_add(VM *vm, uint32_t a, uint32_t b) {
    uint64_t result64 = (uint64_t)a + b;
    uint32_t result   = (uint32_t)result64;

    cpu_update_zsp(vm->cpu, result);

    if (result64 > 0xFFFFFFFFu)
        cpu_set_flag(vm->cpu, FLAG_CF);
    else
        cpu_clr_flag(vm->cpu, FLAG_CF);

    /* Overflow: same-sign inputs produced opposite-sign output */
    if (!((a ^ b) & 0x80000000u) && ((a ^ result) & 0x80000000u))
        cpu_set_flag(vm->cpu, FLAG_OF);
    else
        cpu_clr_flag(vm->cpu, FLAG_OF);

    return result;
}

static uint32_t do_sub(VM *vm, uint32_t a, uint32_t b) {
    uint64_t result64 = (uint64_t)a - b;
    uint32_t result   = (uint32_t)result64;

    cpu_update_zsp(vm->cpu, result);

    if (a < b)
        cpu_set_flag(vm->cpu, FLAG_CF);
    else
        cpu_clr_flag(vm->cpu, FLAG_CF);

    if (((a ^ b) & 0x80000000u) && ((a ^ result) & 0x80000000u))
        cpu_set_flag(vm->cpu, FLAG_OF);
    else
        cpu_clr_flag(vm->cpu, FLAG_OF);

    return result;
}

/* ── INT handler (stub) ──────────────────────────────────────────────────── */

static void handle_int(VM *vm, uint8_t vector) {
    /* Phase 4 will implement a real IDT dispatch. For now just print. */
    printf("[INT 0x%02X] EAX=%08X EBX=%08X ECX=%08X EDX=%08X\n",
           vector,
           vm->cpu->gpr[REG_EAX], vm->cpu->gpr[REG_EBX],
           vm->cpu->gpr[REG_ECX], vm->cpu->gpr[REG_EDX]);
}

/* ── vm_step ─────────────────────────────────────────────────────────────── */

void vm_step(VM *vm) {
    uint8_t opcode;
    if (!fetch8(vm, &opcode)) return;

    switch (opcode) {

    /* NOP */
    case 0x90:
        break;

    /* HLT */
    case 0xF4:
        vm->cpu->halted = true;
        break;

    /* MOV r32, imm32 — opcodes B8..BF */
    case 0xB8: case 0xB9: case 0xBA: case 0xBB:
    case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
        RegIndex rd = (RegIndex)(opcode - 0xB8);
        uint32_t imm;
        if (!fetch32(vm, &imm)) return;
        cpu_set_reg(vm->cpu, rd, imm);
        break;
    }

    /* MOV r/m32, r32 — opcode 89 /r */
    case 0x89: {
        RegIndex reg, rm;
        if (!decode_modrm_reg_reg(vm, &reg, &rm)) return;
        cpu_set_reg(vm->cpu, rm, cpu_get_reg(vm->cpu, reg));
        break;
    }

    /* MOV r32, r/m32 — opcode 8B /r */
    case 0x8B: {
        RegIndex reg, rm;
        if (!decode_modrm_reg_reg(vm, &reg, &rm)) return;
        cpu_set_reg(vm->cpu, reg, cpu_get_reg(vm->cpu, rm));
        break;
    }

    /* ADD r/m32, r32 — opcode 01 /r */
    case 0x01: {
        RegIndex reg, rm;
        if (!decode_modrm_reg_reg(vm, &reg, &rm)) return;
        uint32_t result = do_add(vm, cpu_get_reg(vm->cpu, rm),
                                     cpu_get_reg(vm->cpu, reg));
        cpu_set_reg(vm->cpu, rm, result);
        break;
    }

    /* SUB r/m32, r32 — opcode 29 /r */
    case 0x29: {
        RegIndex reg, rm;
        if (!decode_modrm_reg_reg(vm, &reg, &rm)) return;
        uint32_t result = do_sub(vm, cpu_get_reg(vm->cpu, rm),
                                     cpu_get_reg(vm->cpu, reg));
        cpu_set_reg(vm->cpu, rm, result);
        break;
    }

    /* PUSH r32 — opcodes 50..57 */
    case 0x50: case 0x51: case 0x52: case 0x53:
    case 0x54: case 0x55: case 0x56: case 0x57: {
        RegIndex rs = (RegIndex)(opcode - 0x50);
        vm->cpu->gpr[REG_ESP] -= 4;
        if (!mem_write32(vm->mem, vm->cpu->gpr[REG_ESP],
                         cpu_get_reg(vm->cpu, rs))) {
            fault(vm, "PUSH: stack write fault");
        }
        break;
    }

    /* POP r32 — opcodes 58..5F */
    case 0x58: case 0x59: case 0x5A: case 0x5B:
    case 0x5C: case 0x5D: case 0x5E: case 0x5F: {
        RegIndex rd = (RegIndex)(opcode - 0x58);
        uint32_t val;
        if (!mem_read32(vm->mem, vm->cpu->gpr[REG_ESP], &val)) {
            fault(vm, "POP: stack read fault");
            return;
        }
        vm->cpu->gpr[REG_ESP] += 4;
        cpu_set_reg(vm->cpu, rd, val);
        break;
    }

    /* JMP rel32 — opcode E9 */
    case 0xE9: {
        uint32_t rel;
        if (!fetch32(vm, &rel)) return;
        vm->cpu->eip += (int32_t)rel;
        break;
    }

    /* INT imm8 — opcode CD */
    case 0xCD: {
        uint8_t vector;
        if (!fetch8(vm, &vector)) return;
        handle_int(vm, vector);
        break;
    }

    /* IRET — stub */
    case 0xCF:
        /* Phase 4 */
        fault(vm, "IRET: not yet implemented");
        break;

    default:
        fprintf(stderr, "[FAULT] Unknown opcode 0x%02X at EIP=0x%08X\n",
                opcode, vm->cpu->eip - 1);
        fault(vm, "unknown opcode");
        break;
    }
}

/* ── vm_init / vm_run ────────────────────────────────────────────────────── */

void vm_init(VM *vm, CPU *cpu, Memory *mem) {
    vm->cpu = cpu;
    vm->mem = mem;
}

void vm_run(VM *vm) {
    while (!vm->cpu->halted && !vm->cpu->fault) {
        vm_step(vm);
    }
    if (vm->cpu->fault) {
        fprintf(stderr, "[FAULT] %s (EIP=0x%08X)\n",
                vm->cpu->fault_msg, vm->cpu->eip);
    }
}
