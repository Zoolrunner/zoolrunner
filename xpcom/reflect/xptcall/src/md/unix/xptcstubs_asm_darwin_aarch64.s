#define SYM(x) _ ## x

        .set NGPREGS, 8
        .set NFPREGS, 8

        .text
        .align 2
        .globl SharedStub
SharedStub:
        .cfi_startproc
        stp     x29, x30, [sp, #-16]!
        .cfi_adjust_cfa_offset 16
        .cfi_rel_offset x29, 0
        .cfi_rel_offset x30, 8
        mov     x29, sp
        .cfi_def_cfa_register x29

        sub     sp, sp, #128
        stp     x0, x1, [sp, #64]
        stp     x2, x3, [sp, #80]
        stp     x4, x5, [sp, #96]
        stp     x6, x7, [sp, #112]
        stp     d0, d1, [sp, #0]
        stp     d2, d3, [sp, #16]
        stp     d4, d5, [sp, #32]
        stp     d6, d7, [sp, #48]

        mov     w1, w17
        add     x2, sp, #144
        add     x3, sp, #64
        mov     x4, sp
        bl      SYM(PrepareAndDispatch)

        add     sp, sp, #128
        .cfi_def_cfa_register sp
        ldp     x29, x30, [sp], #16
        .cfi_adjust_cfa_offset -16
        .cfi_restore x29
        .cfi_restore x30
        ret
        .cfi_endproc
