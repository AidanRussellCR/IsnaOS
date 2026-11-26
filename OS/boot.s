; GRUB Handling
BITS 32

section .multiboot
align 4
	dd 0x1BADB002
	dd 0x00000004
	dd -(0x1BADB002 + 0x00000004)

	; Optional address fields (unused)
	dd 0   ; header_addr
	dd 0   ; load_addr
	dd 0   ; load_end_addr
	dd 0   ; bss_end_addr
	dd 0   ; entry_addr

	; Graphics mode
	dd 0   ; linear framebuffer
	dd 800 ; width
	dd 600 ; height
	dd 32  ; bit depth

section .text
global _start
extern kmain

_start:
	cli

	cld

	; Set up stack
	mov esp, stack_top

	; Pass multiboot magic (EAX) and information address (EBX) then call kernel
	push ebx
	push eax
	call kmain

.hang:
	cli
	hlt
	jmp .hang

section .bss
align 16
stack_bottom:
	resb 16384 ; 16 Kb stack
stack_top:
