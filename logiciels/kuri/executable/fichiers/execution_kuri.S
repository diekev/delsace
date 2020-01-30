.text

.globl _start

_start:                    # Le point d'entrée connue du lieur
    mov %rsp, %rbp         # construit une nouvelle pile
    mov 0(%rbp), %rdi      # obtient le nombre d'arguments depuis la pile
    lea 8(%rbp), %rsi      # obtient le pointeur vers les arguments depuis la pile
	call vraie_principale  # %rdi et %rsi sont les deux arguments de vraie_principale

	mov %rax, %rdi         # bouge le retour de vraie_principale du premier argument
    call exit              # termine le programme
