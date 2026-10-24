#include "cpu.h"
#include <string.h>

void cpu_init(CPU *cpu) {
    memset(cpu, 0, sizeof(*cpu));
    cpu->eflags = 0x00000002;  /* bit 1 always 1 per IA-32 spec */
    /* CS = 0xF000 in real HW reset, but we start flat */
    cpu->cs = 0x0000;
    cpu->ds = cpu->ss = cpu->es = cpu->fs = cpu->gs = 0x0000;
}

void cpu_reset(CPU *cpu) {
    cpu_init(cpu);
}

void cpu_update_zsp(CPU *cpu, uint32_t result) {
    /* Zero flag */
    if (result == 0)
        cpu_set_flag(cpu, FLAG_ZF);
    else
        cpu_clr_flag(cpu, FLAG_ZF);

    /* Sign flag — bit 31 of result */
    if (result & 0x80000000u)
        cpu_set_flag(cpu, FLAG_SF);
    else
        cpu_clr_flag(cpu, FLAG_SF);

    /* Parity flag — set if low byte has even number of set bits */
    uint8_t b = (uint8_t)(result & 0xFF);
    b ^= b >> 4;
    b ^= b >> 2;
    b ^= b >> 1;
    if ((b & 1) == 0)
        cpu_set_flag(cpu, FLAG_PF);
    else
        cpu_clr_flag(cpu, FLAG_PF);
}
