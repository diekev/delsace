# -*- coding:utf-8 -*-

import io

mot_cles = [
	u'dyn',
	u'fonc',
	u'corout',
	u'boucle',
	u'pour',
	u'dans',
	u'arrête',
	u'continue',
	u'discr',
	u'si',
	u'sinon',
	u'énum',
	u'énum_drapeau',
	u'struct',
	u'retourne',
	u'diffère',
	u'vrai',
	u'faux',
	u'taille_de',
	u'type_de',
	u'info_de',
	u'mémoire',
	u'empl',
	u'n8',
	u'n16',
	u'n32',
	u'n64',
	u'z8',
	u'z16',
	u'z32',
	u'z64',
	u'r16',
	u'r32',
	u'r64',
	u'bool',
	u'rien',
	u'nul',
	u'sansarrêt',
	u'externe',
	u'importe',
	u'nonsûr',
	u'eini',
	u'chaine',
	u'tantque',
	u'octet',
	u'garde',
	u'saufsi',
	u'retiens',
	u'répète',
	u'union',
	u'charge',
	u'opérateur',
	u'pousse_contexte',
	u'tente',
	u'piège',
	u'nonatteignable',
	u'erreur',
	u'eini_erreur',
	u'comme',
	u'init_de',
	u'type_de_données',
	u'définis',
	u'reprends',
]

mot_cles = sorted(mot_cles)

caracteres_simple = [
	[u'+', u'PLUS'],
	[u'-', u'MOINS'],
	[u'/', u'DIVISE'],
	[u'*', u'FOIS'],
	[u'%', u'POURCENT'],
	[u'=', u'EGAL'],
	[u'@', u'AROBASE'],
	[u'#', u'DIRECTIVE'],
	[u'(', u'PARENTHESE_OUVRANTE'],
	[u')', u'PARENTHESE_FERMANTE'],
	[u'{', u'ACCOLADE_OUVRANTE'],
	[u'}', u'ACCOLADE_FERMANTE'],
	[u'[', u'CROCHET_OUVRANT'],
	[u']', u'CROCHET_FERMANT'],
	[u'.', u'POINT'],
	[u',', u'VIRGULE'],
	[u';', u'POINT_VIRGULE'],
	[u':', u'DOUBLE_POINTS'],
	[u'!', u'EXCLAMATION'],
	[u'&', u'ESPERLUETTE'],
	[u'|', u'BARRE'],
	[u'^', u'CHAPEAU'],
	[u'~', u'TILDE'],
	[u"'", u'APOSTROPHE'],
	[u'"', u'GUILLEMET'],
	[u'<', u'INFERIEUR'],
	[u'>', u'SUPERIEUR'],
	[u'$', u'DOLLAR'],
]

caracteres_simple = sorted(caracteres_simple)

digrammes = [
	[u'<=', u'INFERIEUR_EGAL'],
	[u'>=', u'SUPERIEUR_EGAL'],
	[u'==', u'EGALITE'],
	[u'!=', u'DIFFERENCE'],
	[u'&&', u'ESP_ESP'],
	[u'||', u'BARRE_BARRE'],
	[u'<<', u'DECALAGE_GAUCHE'],
	[u'>>', u'DECALAGE_DROITE'],
	[u'+=', u'PLUS_EGAL'],
	[u'-=', u'MOINS_EGAL'],
	[u'/=', u'DIVISE_EGAL'],
	[u'*=', u'MULTIPLIE_EGAL'],
	[u'%=', u'MODULO_EGAL'],
	[u'&=', u'ET_EGAL'],
	[u'|=', u'OU_EGAL'],
	[u'^=', u'OUX_EGAL'],
	[u':=', u'DECLARATION_VARIABLE'],
	[u'->', u'RETOUR_TYPE'],
	[u'::', u'DECLARATION_CONSTANTE'],
	[u'//', u'DEBUT_LIGNE_COMMENTAIRE'],
	[u'/*', u'DEBUT_BLOC_COMMENTAIRE'],
	[u'*/', u'FIN_BLOC_COMMENTAIRE'],
]

digrammes = sorted(digrammes)

trigrammes = [
	[u'<<=', u'DEC_GAUCHE_EGAL'],
	[u'>>=', u'DEC_DROITE_EGAL'],
	[u'...', u'TROIS_POINTS'],
	[u'---', u'NON_INITIALISATION'],
]

trigrammes = sorted(trigrammes)

id_extra = [
	[u'1.234', u'NOMBRE_REEL'],
	[u'123', u'NOMBRE_ENTIER'],
	[u'-', u'PLUS_UNAIRE'],
	[u'+', u'MOINS_UNAIRE'],
	[u'*', u'FOIS_UNAIRE'],
	[u'&', u'ESP_UNAIRE'],
    [u'chaine_de_caractère', u"CHAINE_CARACTERE"],
    [u'chaine littérale', u"CHAINE_LITTERALE"],
    [u'a', u"CARACTERE"],
    [u'*', u"POINTEUR"],
    [u'[]', u"TABLEAU"],
    [u'&', u"REFERENCE"],
	[u'inconnu', u'INCONNU'],
	[u' ', u'CARACTERE_BLANC'],
	[u'// commentaire', u'COMMENTAIRE'],
	[u'...', u'EXPANSION_VARIADIQUE'],
]


def enleve_accent(mot):
	mot = mot.replace(u'é', 'e')
	mot = mot.replace(u'è', 'e')
	mot = mot.replace(u'â', 'a')
	mot = mot.replace(u'ê', 'e')
	mot = mot.replace(u'ô', 'o')
	mot = mot.replace(u'û', 'u')
	mot = mot.replace(u'î', 'i')

	return mot


def construit_structures():
	structures = u''
	structures += u'\n#pragma GCC diagnostic push\n'
	structures += u'#pragma GCC diagnostic ignored "-Wpedantic"\n'
	structures += u'struct Lexeme {\n'
	structures += u'\tusing type = GenreLexeme;\n'
	structures += u'\tstatic constexpr type INCONNU = GenreLexeme::INCONNU;\n'
	structures += u'\tdls::vue_chaine_compacte chaine;\n'
	structures += u'\n'
	structures += u'\tunion {\n'
	structures += u'\t\tunsigned long long valeur_entiere;\n'
	structures += u'\t\tdouble valeur_reelle;\n'
	structures += u'\t\tlong index_chaine;\n'
	structures += u'\t\tIdentifiantCode *ident;\n'
	structures += u'\t};\n'
	structures += u'\n'
	structures += u'\tGenreLexeme genre;\n'
	structures += u'\tint fichier = 0;\n'
	structures += u'\tint ligne = 0;\n'
	structures += u'\tint colonne = 0;\n'
	structures += u'};\n'
	structures += u'#pragma GCC diagnostic pop\n'

	return structures

def constuit_enumeration():
	enumeration = u'enum class GenreLexeme : unsigned int {\n'

	for car in caracteres_simple:
		enumeration += u'\t{},\n'.format(car[1])

	for car in digrammes:
		enumeration += u'\t{},\n'.format(car[1])

	for car in trigrammes:
		enumeration += u'\t{},\n'.format(car[1])

	for car in id_extra:
		enumeration += u'\t{},\n'.format(car[1])

	for mot in mot_cles:
		m = enleve_accent(mot)
		m = m.upper()

		enumeration += u'\t{},\n'.format(m)

	enumeration += u'};\n'

	return enumeration


def construit_fonction_chaine_du_genre_de_lexeme():
	fonction = u'const char *chaine_du_genre_de_lexeme(GenreLexeme id)\n{\n'
	fonction += u'\tswitch (id) {\n'

	for car in caracteres_simple:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "GenreLexeme::{}";\n'.format(car[1])

	for car in digrammes:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "GenreLexeme::{}";\n'.format(car[1])

	for car in trigrammes:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "GenreLexeme::{}";\n'.format(car[1])

	for car in id_extra:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "GenreLexeme::{}";\n'.format(car[1])

	for mot in mot_cles:
		m = enleve_accent(mot)
		m = m.upper()

		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(m)
		fonction += u'\t\t\treturn "GenreLexeme::{}";\n'.format(m)

	fonction += u'\t}\n'
	fonction += u'\n\treturn "ERREUR";\n'
	fonction += u'}\n'

	return fonction


def construit_fonction_chaine_du_lexeme():
	fonction = u'const char *chaine_du_lexeme(GenreLexeme genre)\n{\n'
	fonction += u'\tswitch (genre) {\n'

	for car in caracteres_simple:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])

		if car[0] == u'"':
		    fonction += u'\t\t\treturn "\{}";\n'.format(car[0])
		else:
		    fonction += u'\t\t\treturn "{}";\n'.format(car[0])

	for car in digrammes:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "{}";\n'.format(car[0])

	for car in trigrammes:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "{}";\n'.format(car[0])

	for car in id_extra:
		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "{}";\n'.format(car[0])

	for mot in mot_cles:
		m = enleve_accent(mot)
		m = m.upper()

		fonction += u'\t\tcase GenreLexeme::{}:\n'.format(m)
		fonction += u'\t\t\treturn "{}";\n'.format(mot)

	fonction += u'\t}\n'
	fonction += u'\n\treturn "ERREUR";\n'
	fonction += u'}\n'

	return fonction


license_ = u"""/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

 /* Ce fichier est généré automatiquement. NE PAS ÉDITER ! */
 """

with io.open(u"../parsage/lexemes.hh", u'w') as entete:
	entete.write(license_)
	entete.write(u'\n#pragma once\n\n')
	entete.write(u'struct IdentifiantCode;\n\n')
	entete.write(u'#include "biblinternes/structures/chaine.hh"\n\n')
	entete.write(constuit_enumeration())
	entete.write(construit_structures())
	entete.write(u"\n")
	entete.write(u"const char *chaine_du_genre_de_lexeme(GenreLexeme id);\n")
	entete.write(u"const char *chaine_du_lexeme(GenreLexeme genre);")


with io.open(u'../parsage/lexemes.cc', u'w') as source:
	source.write(license_)
	source.write(u'\n#include "lexemes.hh"\n\n')
	source.write(construit_fonction_chaine_du_genre_de_lexeme())
	source.write(construit_fonction_chaine_du_lexeme())
