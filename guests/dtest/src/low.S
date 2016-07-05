

    .global syscall_dmmu   
    .code 32
    .balign 4
    

syscall_dmmu:
    svc # 666
    bx lr

    .global syscall_dmmu_query   
    .code 32
    .balign 4

syscall_dmmu_query:
    svc # 1041
    bx lr

    .global syscall_monitor   
    .code 32
    .balign 4

syscall_monitor:
    svc # 1042
    bx lr

    .global syscall_dmmu_2   
    .code 32
    .balign 4
    

syscall_dmmu_2:
    svc # 1007
    bx lr


