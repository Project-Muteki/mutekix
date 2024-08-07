    .arm
    .cpu arm7tdmi
    .global mutekix_thread_wrapper
    .type mutekix_thread_wrapper, %function
@ This is a modified version of the stack fixer in CRT0. It uses stack tip to save the state instead of relying on global variables.
mutekix_thread_wrapper:
    @ Align to 8-bytes ourselves
    push {r4, lr}

    @ Check for alignment
    tst sp, #7

    @ If not aligned, the dummy value flag itself will ensure the alignment, otherwise push a dummy value to maintain the alignment.
    mov r1, #0
    subeq sp, sp, #4
    @ Flag is set when dummy value is pushed.
    moveq r1, #1
    @ Push the flag value
    stmfd sp!, {r1}

    @ Run the actual thread
    ldr r1, [r0]      @ arg->func
    ldr r0, [r0, #4]  @ arg->user_data
    @ PC will be 2 inst later which is right after the bx call
    mov lr, pc
    bx r1

    @ After finished, pop the dummy value flag and check it
    ldmfd sp!, {r1}
    cmp r1, #0
    @ If set, pop the dummy value.
    addne sp, sp, #4

    @ Return
    pop {r4, pc}
