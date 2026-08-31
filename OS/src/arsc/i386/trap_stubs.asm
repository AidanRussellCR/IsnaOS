BITS 32

GLOBAL trap_stub_table

EXTERN trap_dispatch


KERNEL_DATA_SELECTOR equ 0x10


; Exceptions for which the CPU does not push an error code
%macro TRAP_NOERR 1
trap_stub_%1:
    push dword 0
    push dword %1
    jmp trap_common
%endmacro


; Exceptions for which the CPU already pushed an error code
%macro TRAP_ERR 1
trap_stub_%1:
    push dword %1
    jmp trap_common
%endmacro


; CPU exceptions
TRAP_NOERR 0
TRAP_NOERR 1
TRAP_NOERR 2
TRAP_NOERR 3
TRAP_NOERR 4
TRAP_NOERR 5
TRAP_NOERR 6
TRAP_NOERR 7

TRAP_ERR   8

TRAP_NOERR 9

TRAP_ERR   10
TRAP_ERR   11
TRAP_ERR   12
TRAP_ERR   13
TRAP_ERR   14

TRAP_NOERR 15
TRAP_NOERR 16

TRAP_ERR   17

TRAP_NOERR 18
TRAP_NOERR 19
TRAP_NOERR 20

TRAP_ERR   21

TRAP_NOERR 22
TRAP_NOERR 23
TRAP_NOERR 24
TRAP_NOERR 25
TRAP_NOERR 26
TRAP_NOERR 27
TRAP_NOERR 28

TRAP_ERR   29
TRAP_ERR   30

TRAP_NOERR 31

TRAP_NOERR 32
TRAP_NOERR 33
TRAP_NOERR 34
TRAP_NOERR 35
TRAP_NOERR 36
TRAP_NOERR 37
TRAP_NOERR 38
TRAP_NOERR 39
TRAP_NOERR 40
TRAP_NOERR 41
TRAP_NOERR 42
TRAP_NOERR 43
TRAP_NOERR 44
TRAP_NOERR 45
TRAP_NOERR 46
TRAP_NOERR 47


trap_common:
    cld
    pushad

    xor eax, eax
    mov ax, ds
    push eax

    xor eax, eax
    mov ax, es
    push eax

    xor eax, eax
    mov ax, fs
    push eax

    xor eax, eax
    mov ax, gs
    push eax

    mov ax, KERNEL_DATA_SELECTOR

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ebx, esp

    and esp, 0xFFFFFFF0
    sub esp, 12

    push ebx
    call trap_dispatch

    mov esp, ebx

    pop eax
    mov gs, ax

    pop eax
    mov fs, ax

    pop eax
    mov es, ax

    pop eax
    mov ds, ax

    popad

    add esp, 8

    iretd


SECTION .rodata
ALIGN 4

trap_stub_table:
    dd trap_stub_0
    dd trap_stub_1
    dd trap_stub_2
    dd trap_stub_3
    dd trap_stub_4
    dd trap_stub_5
    dd trap_stub_6
    dd trap_stub_7

    dd trap_stub_8
    dd trap_stub_9
    dd trap_stub_10
    dd trap_stub_11
    dd trap_stub_12
    dd trap_stub_13
    dd trap_stub_14
    dd trap_stub_15

    dd trap_stub_16
    dd trap_stub_17
    dd trap_stub_18
    dd trap_stub_19
    dd trap_stub_20
    dd trap_stub_21
    dd trap_stub_22
    dd trap_stub_23

    dd trap_stub_24
    dd trap_stub_25
    dd trap_stub_26
    dd trap_stub_27
    dd trap_stub_28
    dd trap_stub_29
    dd trap_stub_30
    dd trap_stub_31

    dd trap_stub_32
    dd trap_stub_33
    dd trap_stub_34
    dd trap_stub_35
    dd trap_stub_36
    dd trap_stub_37
    dd trap_stub_38
    dd trap_stub_39

    dd trap_stub_40
    dd trap_stub_41
    dd trap_stub_42
    dd trap_stub_43
    dd trap_stub_44
    dd trap_stub_45
    dd trap_stub_46
    dd trap_stub_47
