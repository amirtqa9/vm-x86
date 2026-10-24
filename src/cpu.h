#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

/* EFLAGS bit positions */
#define FLAG_CF  (1u << 0)
#define FLAG_PF  (1u << 2)
#define FLAG_AF  (1u << 4)
#define FLAG_ZF  (1u << 6)
#define FLAG_SF  (1u << 7)
#define FLAG_TF  (1u << 8)
#define FLAG_IF  (1u << 9)
#define FLAG_DF  (1u << 10)
#define FLAG_OF  (1u << 11)

/* Register indices — used for ModRM and opcode +rd encoding */
typedef enum {
    REG_EAX = 0, REG_ECX, REG_EDX, REG_EBX,
    REG_ESP, REG_EBP, REG_ESI, REG_EDI,
    REG_COUNT
} RegIndex;

typedef struct {
    uint32_t gpr[REG_COUNT];  /* EAX–EDI in RegIndex order */
    uint32_t eip;
    uint32_t eflags;

    uint16_t cs, ds, ss, es, fs, gs;  /* segment regs — flat in Phase 1 */

    bool halted;
    bool fault;               /* set on unrecoverable error */
    const char *fault_msg;
} CPU;

void cpu_init(CPU *cpu);
void cpu_reset(CPU *cpu);

/* Convenience accessors */
static inline uint32_t cpu_get_reg(const CPU *cpu, RegIndex r) { return cpu->gpr[r]; }
static inline void     cpu_set_reg(CPU *cpu, RegIndex r, uint32_t v) { cpu->gpr[r] = v; }

static inline bool cpu_flag(const CPU *cpu, uint32_t bit) { return (cpu->eflags & bit) != 0; }
static inline void cpu_set_flag(CPU *cpu, uint32_t bit)   { cpu->eflags |= bit; }
static inline void cpu_clr_flag(CPU *cpu, uint32_t bit)   { cpu->eflags &= ~bit; }

/* Update ZF, SF, PF from a 32-bit result */
void cpu_update_zsp(CPU *cpu, uint32_t result);

#endif /* CPU_H */
