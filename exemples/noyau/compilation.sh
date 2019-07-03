#nasm noyau.asm -f elf64 -o noyau_asm.o
#kuri noyau.kuri --objet_seul
#gcc -c noyau.c -o noyau.o
#ld -m elf_x86_64 -T link.ld -o noyau noyau_asm.o noyau.o

nasm noyau.asm -f elf32 -o noyau_asm.o
gcc -m32 -c noyau.c -o noyau.o
ld -m elf_i386 -T link.ld -o noyau noyau_asm.o noyau.o

# compile en mode 32-bit
#!option arch 32

# défini le type de sortie à créer (executable, objet)
#!sortie obj "noyau.o"

# les commandes sont executés après la compilation
#!commande "nasm noyau.asm -f elf32 -o noyau_asm.o"
#!commande "ld -m elf_i386 -T link.ld -o noyau noyau_asm.o noyau.o"
