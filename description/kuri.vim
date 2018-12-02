" Vim syntax filr
" Language: Kuri
" Maintainer: Kévin Dietrich
" Latest Revision: 23 Octobre 2018

if exists("b:current_syntax")
	finish
endif

" Keywords
syn keyword mot_cles fonction retourne nul soit dyn gabarit employant diffère externe
syn keyword mot_cles_struct structure énum
syn keyword mot_cles_cond si sinon associe nonsûr
syn keyword mot_cles_boucle boucle pour dans arrête continue sansarrêt
syn keyword operateurs de mémoire transtype taille_de
syn keyword booleen vrai faux

" Matches
syn match pre_condition "#!.*$"
syn match commentaire "#.*$"
syn match identifiant "[a-zA-Z_âàéèêîïöôûü][a-zA-Z_0-9âàéèêîïôûüç]*"
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
hi def link pre_condition PreCondit
hi def link booleen Boolean
hi def link chaine_litterale String
hi def link chaine String
hi def link trois_point Operator
hi def link nombre_decimal Number
hi def link nombre_reel Float
hi def link caractere Character
hi def link identifiant Identifier
