
.text
.align 3

.globl main
main:
    @ try to do some dualissue here
                                       mul r4, r0
    vadd.i32    q15, q14
                                       mul r5, r1
    vmul.i32    d27, d26
    vsub.i32    q12, q11
                                       mul r6, r2
    veor.32     d20, d19
                                       mul r7, r3
    vorn.s32    d18, d17
    vshl.i32    q15, q14, #1
                                       smlal r0, r1, r2, r3
    vabs.f32    d29, d28
    vaba.s32    d27, d26, d25

    vzip.8      d0, d16
    vmla.s32    d1, d17
                                       adr r8, main
                                       sdiv r5, r4, lr
    vuzp.8      d2, d18
    vcvt.f32.s32 d3, d19
    vrev64.8    d4, d20
                                       ldrd r6, [r8]
    vmul.s32    d5, d21
    vext.8      d6, d22, #3
    vhadd.u32   d7, d23
    vzip.8      d8, d24
    veor.32     d9, d25
    vuzp.8      d10, d26
    vneg.s32    d11, d27
    vrev32.8    d12, d28
                                       strd r0, [sp, #-8]
    vmul.f32    d13, d29
    vext.8      d14, d30, #3
    vbsl        d15, d31, d21

    subs        r2, #1
    b           main

@ vim:filetype=armasm
