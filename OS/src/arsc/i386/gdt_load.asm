BITS 32

GLOBAL gdt_load

GDT_KERNEL_CODE equ 0x08
GDT_KERNEL_DATA equ 0x10


; void gdt_load(const gdt_ptr_t* ptr)
gdt_load:
    mov eax, [esp + 4]

    lgdt [eax]

    push dword GDT_KERNEL_CODE
    push dword .reload_cs
    retf

.reload_cs:
    mov ax, GDT_KERNEL_DATA

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ret
