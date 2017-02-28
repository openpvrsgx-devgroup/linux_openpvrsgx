
.text

msg: .asciz "%08x %08x %u\n"
err: .asciz "bad result after %u iterations\n"

.align 2

.globl main
main:
    sub        sp, #8

    ldr        r12, =1000001
    cmp        r0, #1
    ble        0f
    ldr        r0, [r1, #4] @ argv[1]
    bl         atoi
    mov        r12, r0

0:
    ldr        r10, =2000000011
    mov        r0,  r10
    mov        r1,  r10
    mov        r2,  r10
    mov        r3,  r10
    mov        r4,  r10
    mov        r5,  r10
    mov        r6,  r10
    mov        r7,  r10
    mov        r8,  r10
    mov        r9,  r10
    vdup.32    q15, r10
    vmov       q14, q15
    vmov       q13, q15
    vmov       q12, q15
    vmov       q11, q15
    vmov       q10, q15
    vmov       q9,  q15
    vmul.u32   q8,  q15, q15
    vmov       q7,  q15
    vmul.u32   q6,  q15, q15
    vmov       q5,  q15
    vmov       q4,  q15
    vmov       q3,  q15
    vmov       q2,  q15
    vmvn       q1,  q15
    vmvn       q0,  q15

.if 0
    mov        r11, #1<<31
    mcr        p15, 0, r11, c9, c12, 1
    mrc        p15, 0, r11, c9, c13, 0
    str        r11, [sp]
.endif
    mov        r11, #0
    str        r11, [sp, #4]
    mov        r11, r12

@@@

.align 6
666:
    vbic.i32   q10, #0x38000000                @ no vmul.f64 overflow

                                 mul   r0, r10
                                 sub   r2, r10, r2, ror #1
    vadd.i32   q14, q15
    vadd.i32   q13, q15
                                 mul   r1, r10
                                 sub   r3, r10, r3, ror #1
    vmul.u32   q12, q15
    vmul.u32   q11, q15
                                 smlal r8, r9, r0, r10
                                 rsb   r0, #0
    vmul.f64   d21, d31
    vmul.f64   d20, d30
                                 sxtab r4, r4, r10, ror #8
                                 sxtab r5, r5, r10, ror #8
    vzip.8     q9, q8
    vzip.8     q7, q6
                                 eor r6, r6, rrx
                                 eor r7, r7, rrx
    vrsra.s32  q5, #1
    vrsra.s32  q4, #1
                                 rbit r2, r2
                                 rbit r3, r3
    vpadal.u32 q3, q15
    vpadal.u32 q2, q15
                                 smlal r8, r9, r0, r10
                                 rsb   r0, #0
    vmull.u32  q1, d31, d2[0]
    vmull.u32  q0, d31, d0[0]

    subs       r11, #1
    bgt        666b

    ldr        r11, [sp, #4]
    add        r11, #1

.if 1 @ check
    eor        r0, r1
    eor        r2, r3
    eor        r4, r5
    eor        r6, r7
    eor        r8, r10
    eor        r9, r10
    orr        r0, r2
    orr        r0, r4
    orr        r0, r6
    orr        r0, r8
    orr        r0, r9
    veor       q14, q13
    veor       q12, q11
    veor       d21, d20
    veor       q9,  q7
    veor       q8,  q6
    veor       q5,  q4
    veor       q3,  q2
    veor       q1,  q0
    vorr       q14, q12
    vorr       q9,  q8
    vorr       d28, d29
    vorr       d19, d18
    vorr       d11, d10
    vorr       d7,  d6
    vorr       d3,  d2
    vorr       d28, d21
    vorr       d28, d18
    vorr       d28, d11
    vorr       d28, d7
    vorr       d28, d3
    vpmax.u32  d28, d28
    vmov.32    r2, d28[0]
    orr        r0, r2
    tst        r0, r0
    beq        0f

    ldr        r0, =err
    mov        r1, r11
    bl         printf
    b          abort

0:
    @ restore
    mov        r0, r1
    mov        r2, r3
    mov        r4, r5
    mov        r6, r7
    mov        r8, r10
    mov        r9, r10
    vmov       q14, q13
    vmov       q12, q11
    vmov       d21, d20
    vmov       q9,  q7
    vmov       q8,  q6
    vmov       q5,  q4
    vmov       q3,  q2
    vmov       q1,  q0
    str        r11, [sp, #4]
    mov        r11, r12
    b          666b
.endif

    ldr        r11, [sp]
    mrc        p15, 0, r12, c9, c13, 0
    mov        r1, r6
    mov        r2, r7
@    vmov.32    r1, d0[0]
@    vmov.32    r2, d1[1]
    ldr        r0, =msg
    sub        r3, r12, r11
    bl         printf
    b          exit

.pool

@ vim:filetype=armasm
