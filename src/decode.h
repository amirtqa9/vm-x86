#ifndef DECODE_H
#define DECODE_H

#include "cpu.h"
#include "memory.h"

typedef struct {
    CPU    *cpu;
    Memory *mem;
} VM;

void vm_init(VM *vm, CPU *cpu, Memory *mem);

/* Execute one instruction. Sets cpu->fault on unrecognised/bad instruction. */
void vm_step(VM *vm);

/* Run until cpu->halted or cpu->fault */
void vm_run(VM *vm);

#endif /* DECODE_H */
