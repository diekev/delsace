" Vim syntax filr
" Language: Kuri
" Maintainer: Kévin Dietrich
" Latest Revision: 23 Octobre 2018

if exists("b:current_syntax")
	finish
endif

" Keywords
syn keyword mot_cles fonc retourne nul empl diffère externe eini chaine rien z8 z16 z32 z64 r16 r32 r64 n8 n16 n32 n64 importe charge
syn keyword mot_cles_struct struct union énum énum_drapeau
syn keyword mot_cles_cond si sinon saufsi discr nonsûr
syn keyword mot_cles_boucle boucle pour dans arrête continue sansarrêt tantque répète
syn keyword operateurs mémoire comme taille_de type_de info_de
syn keyword booleen vrai faux

" Matches
syn match commentaire "#.\+"
syn match identifiant "[a-zA-Z_][a-zA-Z_0-9]\+"
syn match trois_point "\.\.\."
syn match operateur_simple "[\[\]!+\-\*/@=<>\|&~]"
syn match chaine_litterale "\".*\""
syn match nombre_decimal '\d[0-9_]*'
syn match nombre_decimal '0[xX][0-9a-fA-F_]\+'
syn match nombre_decimal '0[oO][0-7_]\+'
syn match nombre_decimal '0[bB][0-1_]\+'
syn match nombre_reel '\d\+\.\d*'

" Régions
syn region bloc start='{' end='}' fold transparent
syn region caractere start='\'' end='\''
syn region chaine start="«" end="»"

let b:current_syntax = "kuri"

hi def link commentaire Comment
hi def link mot_cles Keyword
hi def link mot_cles_struct Structure
hi def link mot_cles_cond Conditional
hi def link mot_cles_boucle Repeat
hi def link operateurs Operator
hi def link operateur_simple Operator
hi def link booleen Boolean
hi def link chaine_litterale String
hi def link chaine String
hi def link trois_point Operator
hi def link nombre_decimal Number
hi def link nombre_reel Float
hi def link caractere Character
hi def link identifiant Identifier

let b:current_syntax = "kuri"
