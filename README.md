# Linux eBPF JIT verification

This repository contains a tool for formally verifying
the Linux BPF JITs that builds
on our verification framework [Serval].

We used this tool to help develop the RV32 BPF JIT
(in `arch/riscv/net/bpf_jit_comp32.c`).

We also used this tool to find the bugs in the RV64 BPF JIT
([1e692f09e091]),
and also to review the patches that add support for far jumps and branching:

- [[PATCH bpf-next 2/8] riscv, bpf: add support for far branching](https://lore.kernel.org/bpf/20191209173136.29615-3-bjorn.topel@gmail.com/T/#u)
- [[PATCH bpf-next 3/8] riscv, bpf: add support for far jumps and exits](https://lore.kernel.org/bpf/20191209173136.29615-4-bjorn.topel@gmail.com/T/#u)

## How to install dependencies

- [Racket] (tested on v7.6 and v7.6-cs)
- [Serval]

After installing Racket, install a good version of Serval with

```sh
git clone --recursive 'https://github.com/uw-unsat/bpf-jit-verif.git'
cd bpf-jit-verif
raco pkg install --auto ./serval
```

## Directory structure

`racket/lib/bpf-common.rkt` contains the BPF JIT correctness
specification and other common BPF functionality.

`racket/lib/riscv-common.rkt` provides features specific
to the JIT for the RISC-V ISA.

`arch/riscv/net/bpf_jit_comp.c` contains the C implementation
of the BPF JIT for rv64 from Linux.

`racket/rv64/bpf_jit_riscv64.rkt` is a manual translation
of the C implementation into Rosette for verification.

`racket/rv32/bpf_jit_comp32.rkt` is a Racket implementation
of a BPF JIT for rv32.

`racket/rv32/spec.rkt` contains the specification of the BPF JIT
for rv32.

`racket/rv32/lemmas.lean` contains the axiomatization of bitvector
operations in Lean.

`arch/riscv/net/bpf_jit_comp32.c` contains the C implementation
of the BPF JIT for rv32. It is generated from the
`racket/rv32/bpf_jit_comp32.c.tmpl` using the Racket implementation.

`racket/test/` contains verification test cases for
different classes of instructions.

## Running verification

```sh
raco test --jobs 8 racket/test
```

runs all of the verification test cases in parallel
using 8 jobs. You can also run
individual files for a specific class of instructions,
e.g.,

```sh
raco test racket/test/rv64/riscv64-alu32-x.rkt
```

## Finding bugs via verification

As an example, let's inject a bug fixed in commit [1e692f09e091].
Specifically, remove the zero extension for `BPF_ALU|BPF_ADD|BPF_X`
in `racket/rv64/bpf_jit_riscv64.rkt`:

```
    [(list (or 'BPF_ALU 'BPF_ALU64) 'BPF_ADD 'BPF_X)
     (emit (if is64 (rv_add rd rd rs) (rv_addw rd rd rs)) ctx)
     (when (! is64)
       (emit_zext_32 rd ctx))]
```

Now we have a buggy JIT implementation:

```
    [(list (or 'BPF_ALU 'BPF_ALU64) 'BPF_ADD 'BPF_X)
     (emit (if is64 (rv_add rd rd rs) (rv_addw rd rd rs)) ctx)]
```

Run the verification:

```sh
raco test racket/test/rv64/riscv64-alu32-x.rkt
```

Verification will fail with a counterexample:

```
Running test "VERIFY (BPF_ALU BPF_ADD BPF_X)"
--------------------
riscv64-alu32-x tests > VERIFY (BPF_ALU BPF_ADD BPF_X)
FAILURE
name:       check-unsat?
location:   [...]/bpf-common.rkt:218:4
params:
  '((model
   [x?$0 #f]
   [r0$0 (bv #x0000000080000000 64)]
   [r1$0 (bv #x0000000000000000 64)]
   [insn$0 (bv #x00800000 32)]
   [offsets$0 (fv (bitvector 32)~>(bitvector 32))]
   [target-pc-base$0 (bv #x0000000000000000 64)]
   [off$0 (bv #x8000 16)]))
--------------------
```

The counterexample produced by the tool gives
BPF register values that will trigger the bug:
it chooses r0 to be 0x0000000080000000
and r1 to be 0x0000000000000000. This demonstrates
the bug because the RISC-V instructions sign extend
the result of the 32-bit addition, where the BPF instruction
performs zero extension.

## Verification

The verification works by proving equivalence
between a BPF instruction and the RISC-V instructions
generated by the JIT for that instruction.

As a concrete example, consider verifying the `BPF_ALU|BPF_ADD|BPF_X`
instruction. The verification starts by constructing a _symbolic_ BPF
instruction where the destination and source register fields can
take on any legit value:

```
BPF_ALU32_REG(BPF_ADD, {{rd}}, {{rs}})
```

Next, the verifier symbolically evaluates the JIT on the BPF
instruction, producing a set of possible sequences of symbolic
RISC-V instructions:

```
addw {{rv_reg(rd)}} {{rv_reg(rd)}} {{rv_reg(rs)}}
slli {{rv_reg(rd)}} {{rv_reg(rd)}} 32
srli {{rv_reg(rd)}} {{rv_reg(rd)}} 32
```

Here, `rv_reg` denotes the RISC-V register associated
with a particular BPF register.

Next, the tool proves that every possible sequence of generated
RISC-V instructions has the equivalent behavior as the original BPF
instruction, for all possible choices of registers `rd` and `rs`,
and for all initial values of all RISC-V general-purpose registers.
To do so, it leverages automated verifiers provided by the [Serval]
verification framework, as follows.

The verifier starts with a symbolic BPF state and symbolic RISC-V
state, called `bpf-state` and `riscv-state`, assuming that the two
initial states match, e.g., `riscv-state[rv_reg(reg)] == bpf-state[reg]`
for all `reg`.  Next, it runs the Serval BPF and RISC-V verifiers
on the BPF instruction over `bpf-state` and every possible sequence
of generated RISC-V instructions over `riscv-state`, respectively.
It then proves that the final BPF
and RISC-V states still match.

Support for more complex instructions, such as jumps and branches,
requires additional care. For the details, you can see the JIT
correctness definition in `lib/bpf-common.rkt:verify-jit-refinement`.
This complexity comes from having to prove that the generated
instructions preserve a correspondence between the BPF program
counter and the RISC-V program counter.

## What is verified?

The test cases under `racket/test/` show which instructions
the JIT is currently verified for. For those instructions,
it proves that the JIT is correct for all possible initial
register values, for all jump offsets, for all immediate values, etc.

## Generating the RV32 JIT.

The RV32 JIT is split in two parts: the Racket implementation
in `racket/rv32/bpf_jit_comp32.rkt` contains the code required
for generating RV32 instruction sequences for a given BPF instruction,
and `racket/rv32/bpf_jit_comp32.c.tmpl` is a template containing
glue C code required to have the JIT interface compile in C and
interface with the rest of the kernel. The template
contains holes that are filled in by extracting the corresponding Racket
source code to C.

The final C code in `arch/riscv/net/bpf_jit_comp32.c` can be generated
from these two components via:

```sh
make arch/riscv/net/bpf_jit_comp32.c
```

This file can be copied to the Linux kernel source tree for building
and testing.

## Caveats / limitations

There are several properties of the JIT that
are currently not specified or verified:

- Memory instructions (e.g. `BPF_LD`, `BPF_ST`, etc.)
- JIT prologue and epilogue
- 64-bit immediate load (`BPF_LD | BPF_IMM | BPF_DW`)
- Call instructions (e.g., `BPF_CALL`, `BPF_TAIL_CALL`)
- `build_body`: the verification proves the JIT correct
  for individual BPF instructions at a time.
- The loop in `bpf_int_jit_compile`: verification assumes
  the `ctx->offset` mapping has already been correctly
  constructed by previous iterations.
- The verified JIT is a manual Rosette port of the C version:
  mismatches in this translation can mean there are bugs
  in the C version not present in the verified one.
- The verification assumes that the BPF program being compiled
  passed the kernel's BPF verifier: e.g., it assumes no
  divide-by-zero or out-of-range shifts.
- Verification makes assumptions on the total size of the
  compiled BPF program for jumps: it assumes that the number of BPF
  instructions is less than 16,777,216; and that the total number
  of generated RISC-V instructions is less than 134,217,728.
  These bounds can be increased but will increase overall
  verification time for jumps.

[Racket]: https://racket-lang.org
[Serval]: https://unsat.cs.washington.edu/projects/serval/
[1e692f09e091]: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=1e692f09e091
