#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"
#include "memory.h"
#include "decode.h"

#define RAM_SIZE (4 * 1024 * 1024)  /* 4 MB */
#define LOAD_ADDR 0x00000500        /* first usable address */

static void print_registers(const CPU *cpu) {
    printf("EAX=%08X  EBX=%08X  ECX=%08X  EDX=%08X\n",
           cpu->gpr[REG_EAX], cpu->gpr[REG_EBX],
           cpu->gpr[REG_ECX], cpu->gpr[REG_EDX]);
    printf("ESI=%08X  EDI=%08X  ESP=%08X  EBP=%08X\n",
           cpu->gpr[REG_ESI], cpu->gpr[REG_EDI],
           cpu->gpr[REG_ESP], cpu->gpr[REG_EBP]);
    printf("EIP=%08X  EFLAGS=%08X\n", cpu->eip, cpu->eflags);
    printf("  ZF=%u SF=%u CF=%u OF=%u PF=%u\n",
           (cpu->eflags & FLAG_ZF) ? 1 : 0,
           (cpu->eflags & FLAG_SF) ? 1 : 0,
           (cpu->eflags & FLAG_CF) ? 1 : 0,
           (cpu->eflags & FLAG_OF) ? 1 : 0,
           (cpu->eflags & FLAG_PF) ? 1 : 0);
}

static void run_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(1); }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    uint8_t *code = malloc(fsize);
    if (!code) { fputs("OOM\n", stderr); exit(1); }
    fread(code, 1, fsize, f);
    fclose(f);

    CPU cpu;
    Memory mem;
    VM vm;

    cpu_init(&cpu);
    if (!mem_init(&mem, RAM_SIZE)) { fputs("mem_init failed\n", stderr); exit(1); }
    vm_init(&vm, &cpu, &mem);

    /* Stack grows down from 1 MB */
    cpu.gpr[REG_ESP] = 0x00100000;
    cpu.eip = LOAD_ADDR;

    if (!mem_load(&mem, LOAD_ADDR, code, fsize)) {
        fputs("Program too large\n", stderr);
        exit(1);
    }
    free(code);

    printf("=== vm-x86 | %ld bytes loaded at 0x%08X ===\n", fsize, LOAD_ADDR);
    vm_run(&vm);

    printf("\n=== Halted ===\n");
    print_registers(&cpu);

    mem_free(&mem);
}

/* Built-in smoke test — no file needed */
static void run_selftest(void) {
    /*
     * Hand-encoded program:
     *   B8 0A 00 00 00   MOV EAX, 10
     *   B9 14 00 00 00   MOV ECX, 20
     *   01 C8            ADD EAX, ECX   (EAX = 30)
     *   F4               HLT
     */
    static const uint8_t prog[] = {
        0xB8, 0x0A, 0x00, 0x00, 0x00,
        0xB9, 0x14, 0x00, 0x00, 0x00,
        0x01, 0xC8,
        0xF4,
    };

    CPU cpu;
    Memory mem;
    VM vm;

    cpu_init(&cpu);
    mem_init(&mem, RAM_SIZE);
    vm_init(&vm, &cpu, &mem);

    cpu.gpr[REG_ESP] = 0x00100000;
    cpu.eip = LOAD_ADDR;
    mem_load(&mem, LOAD_ADDR, prog, sizeof(prog));

    printf("=== Self-test ===\n");
    vm_run(&vm);
    print_registers(&cpu);

    int ok = (cpu.gpr[REG_EAX] == 30) && cpu.halted && !cpu.fault;
    printf("Result: %s (EAX expected 30, got %u)\n",
           ok ? "PASS" : "FAIL", cpu.gpr[REG_EAX]);

    mem_free(&mem);
}

int main(int argc, char **argv) {
    if (argc == 1) {
        run_selftest();
    } else if (argc == 2 && strcmp(argv[1], "--test") == 0) {
        run_selftest();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: %s [binary.bin | --test]\n", argv[0]);
        return 1;
    }
    return 0;
}
