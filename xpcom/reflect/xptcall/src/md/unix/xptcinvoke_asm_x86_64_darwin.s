#define SYM(x) _ ## x

        .section __TEXT,__text,regular,pure_instructions
        .p2align 4
        .globl SYM(xptc_invoke_x86_64_darwin)

/*
 * nsresult xptc_invoke_x86_64_darwin(const XPTCInvokeData* data)
 *
 * The C++ half classifies XPCOM parameters and supplies register homes plus
 * an already padded stack argument array.  This routine creates the actual
 * Darwin x86-64 call frame; merely assigning C register variables does not
 * make an alloca buffer part of that frame under Clang.
 */
SYM(xptc_invoke_x86_64_darwin):
        .cfi_startproc
        pushq   %rbp
        .cfi_def_cfa_offset 16
        .cfi_offset %rbp, -16
        movq    %rsp, %rbp
        .cfi_def_cfa_register %rbp
        pushq   %rbx
        pushq   %r12
        .cfi_offset %rbx, -24
        .cfi_offset %r12, -32

        movq    %rdi, %rbx
        movl    128(%rbx), %ecx
        testl   %ecx, %ecx
        jz      1f
        leaq    (,%rcx,8), %r12
        subq    %r12, %rsp
        movq    120(%rbx), %rsi
        movq    %rsp, %rdi
        rep movsq
1:
        movq    0(%rbx), %r11

        movq    56(%rbx), %xmm0
        movq    64(%rbx), %xmm1
        movq    72(%rbx), %xmm2
        movq    80(%rbx), %xmm3
        movq    88(%rbx), %xmm4
        movq    96(%rbx), %xmm5
        movq    104(%rbx), %xmm6
        movq    112(%rbx), %xmm7

        movq    16(%rbx), %rsi
        movq    24(%rbx), %rdx
        movq    32(%rbx), %rcx
        movq    40(%rbx), %r8
        movq    48(%rbx), %r9
        movq    8(%rbx), %rdi
        callq   *%r11

        leaq    -16(%rbp), %rsp
        popq    %r12
        .cfi_restore %r12
        popq    %rbx
        .cfi_restore %rbx
        popq    %rbp
        .cfi_def_cfa %rsp, 8
        .cfi_restore %rbp
        retq
        .cfi_endproc
