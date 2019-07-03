;;noyau.asm
bits 32			; directive nasm - 32 bit

section .text
	; spec multiboot
	align 4
	dd 0x1BADB002           ;nombre magic
	dd 0x00                 ;drapeaux
	dd -(0x1BADB002 + 0x00) ; somme de controle. magic + drapeaux + somme = 0

global debut
extern debut_noyau    ; debut_noyau is defined in the c file

debut:
  cli 			; bloc les interruptions
  mov esp, stack_space	; set stack pointer
  call debut_noyau
  hlt		 	; halt the CPU

section .bss
resb 8192		; 8Ko pour la pile
stack_space:
