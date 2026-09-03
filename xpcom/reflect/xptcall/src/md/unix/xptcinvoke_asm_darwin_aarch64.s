#define SYM(x) _ ## x

        .text
        .align 2
        .globl SYM(_XPTC_InvokeByIndex)

SYM(_XPTC_InvokeByIndex):
        .cfi_startproc
        stp     x29, x30, [sp, #-32]!
        .cfi_adjust_cfa_offset 32
        .cfi_rel_offset x29, 0
        .cfi_rel_offset x30, 8
        mov     x29, sp
        .cfi_def_cfa_register x29
        stp     x19, x20, [sp, #16]
        .cfi_rel_offset x19, 16
        .cfi_rel_offset x20, 24

        mov     w20, w1
        mov     x1, sp

        /* Eight bytes per parameter is an upper bound for Apple's compact
         * stack slots.  Round the allocation to the required 16 bytes. */
        add     w19, w2, #1
        and     w19, w19, #0xfffffffe
        sub     sp, sp, w19, uxth #3

        /* Temporary homes for x0-x7 and d0-d7. */
        sub     sp, sp, #128
        str     x0, [sp]
        mov     x0, sp
        bl      SYM(invoke_copy_to_stack)

        ldp     x6, x7, [sp, #48]
        ldp     x4, x5, [sp, #32]
        ldp     x2, x3, [sp, #16]
        ldp     x0, x1, [sp], #64
        ldp     d6, d7, [sp, #48]
        ldp     d4, d5, [sp, #32]
        ldp     d2, d3, [sp, #16]
        ldp     d0, d1, [sp], #64

        ldr     x16, [x0]
        add     x16, x16, w20, uxth #3
        ldr     x16, [x16]
        blr     x16

        add     sp, sp, w19, uxth #3
        .cfi_def_cfa_register sp
        ldp     x19, x20, [sp, #16]
        .cfi_restore x19
        .cfi_restore x20
        ldp     x29, x30, [sp], #32
        .cfi_adjust_cfa_offset -32
        .cfi_restore x29
        .cfi_restore x30
        ret
        .cfi_endproc
