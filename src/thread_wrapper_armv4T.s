    .arm
    .cpu arm7tdmi
    .global mutekix_thread_wrapper
@ This is a modified version of the stack fixer in CRT0. It uses stack tip to save the state instead of relying on global variables.
mutekix_thread_wrapper:
    @ Align to 8-bytes ourselves
    push {r4, lr}

    @ Check for alignment
    tst sp, #7

    @ Only push 1 element if not aligned, otherwise push 2 and set the flag.
    mov r1, #0
    subeq sp, sp, #4
    moveq r1, #1
    stmfd sp!, {r1}

    @ Run the actual thread
    ldr r1, [r0]      @ arg->func
    ldr r0, [r0, #4]  @ arg->user_data
    @ PC will be 2 inst later which is right after the bx call
    mov lr, pc
    bx r1

    @ After finished, check for the fixed flag and undo the fix if set
    ldmfd sp!, {r1}
    cmp r1, #0
    addne sp, sp, #4

    @ Return
    pop {r4, pc}
