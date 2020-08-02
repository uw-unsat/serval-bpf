#include <linux/bpf.h>
#include <linux/filter.h>

#include "arch/riscv/net/bpf_jit.h"

/* Force static inline functions to be emitted. */
void *funcs[] = {
    &rv_add,
    &rv_addi,
    &rv_sub,
    &rv_and,
    &rv_andi,
    &rv_or,
    &rv_ori,
    &rv_xor,
    &rv_xori,
    &rv_mul,
    &rv_mulhu,
    &rv_sll,
    &rv_slli,
    &rv_srl,
    &rv_srli,
    &rv_sra,
    &rv_srai,
    &rv_divu,
    &rv_remu,
    &rv_sltu,
    &rv_lui,
    &rv_auipc,
    &rv_jal,
    &rv_jalr,
    &rv_beq,
    &rv_bltu,
    &rv_bgtu,
    &rv_bgeu,
    &rv_bleu,
    &rv_bne,
    &rv_blt,
    &rv_bgt,
    &rv_bge,
    &rv_ble,
    &rv_sb,
    &rv_sh,
    &rv_sw,
    &rv_lbu,
    &rv_lhu,
    &rv_lw,
    &rv_amoadd_w,
    // &rvc_beqz,
    // &rvc_bnez,
    &rvc_mv,
    &rvc_add,
    &rvc_jalr,
    &rvc_jr,
    &rvc_addi,
    &rvc_addiw,
    &rvc_li,
    &rvc_lui,
    &rvc_addi16sp,
    &rvc_addi4spn,
    &rvc_lwsp,
    &rvc_swsp,
    &rvc_ld,
    &rvc_lw,
    &rvc_sd,
    &rvc_sw,
    &rvc_ldsp,
    &rvc_sdsp,
    &rvc_slli,
    &rvc_srli,
    &rvc_srai,
    &rvc_andi,
    &rvc_sub,
    &rvc_xor,
    &rvc_or,
    &rvc_and,
    &rvc_subw,
    &rv_slli,
    &rv_srli,
    &rv_srai,
    &rv_addiw,
    &rv_addw,
    &rv_subw,
    &rv_mulw,
    &rv_divuw,
    &rv_remuw,
    &rv_sllw,
    &rv_srlw,
    &rv_sraw,
    &rv_slliw,
    &rv_srliw,
    &rv_sraiw,
    &rv_sd,
    &rv_lwu,
    &rv_ld,
    &rv_amoadd_d,
};
