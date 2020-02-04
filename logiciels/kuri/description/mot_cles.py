# -*- coding:utf-8 -*-

import io

mot_cles = [
	u'soit',
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
	u'transtype',
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
	u'n128',
	u'z8',
	u'z16',
	u'z32',
	u'z64',
	u'z128',
	u'r16',
	u'r32',
	u'r64',
	u'r128',
	u'bool',
	u'rien',
	u'nul',
	u'sansarrêt',
	u'externe',
	u'importe',
	u'nonsûr',
	u'eini',
	u'chaine',
	u'loge',
	u'déloge',
	u'reloge',
	u'tantque',
	u'octet',
	u'garde',
	u'saufsi',
	u'retiens',
	u'répète',
	u'union',
	u'charge',
]

taille_max_mot_cles = max(len(m.encode('utf8')) for m in mot_cles)

mot_cles = sorted(mot_cles)

caracteres_simple = [
	[u'+', u'PLUS'],
	[u'-', u'MOINS'],
	[u'/', u'DIVISE'],
	[u'*', u'FOIS'],
	[u'%', u'POURCENT'],
	[u'=', u'EGAL'],
	[u'@', u'AROBASE'],
	[u'#', u'DIESE'],
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

digraphes = [
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
	[u'#!', u'DIRECTIVE'],
	[u':=', u'DECLARATION_VARIABLE'],
]

digraphes = sorted(digraphes)

trigraphes = [
	[u'<<=', u'DEC_GAUCHE_EGAL'],
	[u'>>=', u'DEC_DROITE_EGAL'],
	[u'...', u'TROIS_POINTS'],
]

trigraphes = sorted(trigraphes)

id_extra = [
	u'NOMBRE_REEL',
	u'NOMBRE_ENTIER',
	u'NOMBRE_HEXADECIMAL',
	u'NOMBRE_OCTAL',
	u'NOMBRE_BINAIRE',
	u'PLUS_UNAIRE',
	u'MOINS_UNAIRE',
    u"CHAINE_CARACTERE",
    u"CHAINE_LITTERALE",
    u"CARACTERE",
    u"POINTEUR",
    u"TABLEAU",
    u"REFERENCE",
	u'INCONNU',
	u'CARACTERE_BLANC',
	u'COMMENTAIRE',
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
	structures += u'\nstruct DonneesLexeme {\n'
	structures += u'\tusing type = TypeLexeme;\n'
	structures += u'\tstatic constexpr type INCONNU = TypeLexeme::INCONNU;\n'
	structures += u'\tdls::vue_chaine_compacte chaine;\n'
	structures += u'\tTypeLexeme identifiant;\n'
	structures += u'\tint fichier = 0;\n'
	structures += u'};\n'

	return structures


def construit_tableaux():
	tableaux = u''

	tableaux += u'static auto paires_mots_cles = dls::cree_dico(\n'

	virgule = ''

	for mot in mot_cles:
		tableaux += virgule
		m = enleve_accent(mot)
		m = m.upper()
		tableaux += u'\tdls::paire{{ dls::vue_chaine_compacte("{}"), TypeLexeme::{} }}'.format(mot, m)
		virgule = ',\n'

	tableaux += u'\n);\n\n'

	tableaux += u'static auto paires_digraphes = dls::cree_dico(\n'

	virgule = ''

	for c in digraphes:
		tableaux += virgule
		tableaux += u'\tdls::paire{{ dls::vue_chaine_compacte("{}"), TypeLexeme::{} }}'.format(c[0], c[1])
		virgule = ',\n'

	tableaux += u'\n);\n\n'

	tableaux += u'static auto paires_trigraphes = dls::cree_dico(\n'

	virgule = ''

	for c in trigraphes:
		tableaux += virgule
		tableaux += u'\tdls::paire{{ dls::vue_chaine_compacte("{}"), TypeLexeme::{} }}'.format(c[0], c[1])
		virgule = ',\n'

	tableaux += u'\n);\n\n'

	tableaux += u'static auto paires_caracteres_speciaux = dls::cree_dico(\n'

	virgule = ''

	for c in caracteres_simple:
		tableaux += virgule
		if c[0] == "'":
			c[0] = "\\'"

		tableaux += u"\tdls::paire{{ '{}', TypeLexeme::{} }}".format(c[0], c[1])
		virgule = ',\n'

	tableaux += u'\n);\n\n'

	return tableaux


def constuit_enumeration():
	enumeration = u'enum class TypeLexeme : unsigned int {\n'

	for car in caracteres_simple:
		enumeration += u'\t{},\n'.format(car[1])

	for car in digraphes:
		enumeration += u'\t{},\n'.format(car[1])

	for car in trigraphes:
		enumeration += u'\t{},\n'.format(car[1])

	for mot in mot_cles + id_extra:
		m = enleve_accent(mot)
		m = m.upper()

		enumeration += u'\t{},\n'.format(m)

	enumeration += u'};\n'

	return enumeration


def construit_fonction_chaine_identifiant():
	fonction = u'const char *chaine_identifiant(TypeLexeme id)\n{\n'
	fonction += u'\tswitch (id) {\n'

	for car in caracteres_simple:
		fonction += u'\t\tcase TypeLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "TypeLexeme::{}";\n'.format(car[1])

	for car in digraphes:
		fonction += u'\t\tcase TypeLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "TypeLexeme::{}";\n'.format(car[1])

	for car in trigraphes:
		fonction += u'\t\tcase TypeLexeme::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "TypeLexeme::{}";\n'.format(car[1])

	for mot in mot_cles + id_extra:
		m = enleve_accent(mot)
		m = m.upper()

		fonction += u'\t\tcase TypeLexeme::{}:\n'.format(m)
		fonction += u'\t\t\treturn "TypeLexeme::{}";\n'.format(m)

	fonction += u'\t};\n'
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

enumeration = constuit_enumeration()
fonction = construit_fonction_chaine_identifiant()
structures = construit_structures()
tableaux = construit_tableaux()

fonctions = u"""
static bool tables_caracteres[256] = {};
static TypeLexeme tables_identifiants[256] = {};
static bool tables_digraphes[256] = {};
static bool tables_trigraphes[256] = {};
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_digraphes[i] = false;
		tables_trigraphes[i] = false;
		tables_mots_cles[i] = false;
		tables_identifiants[i] = TypeLexeme::INCONNU;
	}

    {
	    auto plg = paires_caracteres_speciaux.plage();

	    while (!plg.est_finie()) {
		    tables_caracteres[int(plg.front().premier)] = true;
		    tables_identifiants[int(plg.front().premier)] = plg.front().second;
	   		plg.effronte();
	    }
	}

    {
	    auto plg = paires_digraphes.plage();

	    while (!plg.est_finie()) {
		    tables_digraphes[int(plg.front().premier[0])] = true;
	   		plg.effronte();
	    }
	}

    {
	    auto plg = paires_trigraphes.plage();

	    while (!plg.est_finie()) {
		    tables_trigraphes[int(plg.front().premier[0])] = true;
	   		plg.effronte();
	    }
	}

    {
	    auto plg = paires_mots_cles.plage();

	    while (!plg.est_finie()) {
		    tables_mots_cles[static_cast<unsigned char>(plg.front().premier[0])] = true;
	   		plg.effronte();
	    }
	}
}

bool est_caractere_special(char c, TypeLexeme &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

TypeLexeme id_digraphe(const dls::vue_chaine_compacte &chaine)
{
	if (!tables_digraphes[int(chaine[0])]) {
		return TypeLexeme::INCONNU;
	}

	auto iterateur = paires_digraphes.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return TypeLexeme::INCONNU;
}

TypeLexeme id_trigraphe(const dls::vue_chaine_compacte &chaine)
{
	if (!tables_trigraphes[int(chaine[0])]) {
		return TypeLexeme::INCONNU;
	}

	auto iterateur = paires_trigraphes.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return TypeLexeme::INCONNU;
}

TypeLexeme id_chaine(const dls::vue_chaine_compacte &chaine)
{
	if (chaine.taille() == 1 || chaine.taille() > TAILLE_MAX_MOT_CLE) {
		return TypeLexeme::CHAINE_CARACTERE;
	}

	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return TypeLexeme::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return TypeLexeme::CHAINE_CARACTERE;
}
"""

fonctions_enumeration = u"""
inline TypeLexeme operator&(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) & id2);
}

inline TypeLexeme operator|(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) | id2);
}

inline TypeLexeme operator|(TypeLexeme id1, TypeLexeme id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) | static_cast<int>(id2));
}

inline TypeLexeme operator<<(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) << id2);
}

inline TypeLexeme operator>>(TypeLexeme id1, int id2)
{
	return static_cast<TypeLexeme>(static_cast<int>(id1) >> id2);
}
"""

declaration_fonctions = u"""
const char *chaine_identifiant(TypeLexeme id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, TypeLexeme &i);

TypeLexeme id_digraphe(const dls::vue_chaine_compacte &chaine);

TypeLexeme id_trigraphe(const dls::vue_chaine_compacte &chaine);

TypeLexeme id_chaine(const dls::vue_chaine_compacte &chaine);
"""

with io.open(u"../compilation/lexemes.hh", u'w') as entete:
	entete.write(license_)
	entete.write(u'\n#pragma once\n\n')
	entete.write(u'#include "biblinternes/structures/chaine.hh"\n\n')
	entete.write(enumeration)
	entete.write(fonctions_enumeration)
	entete.write(structures)
	entete.write(declaration_fonctions)


with io.open(u'../compilation/lexemes.cc', u'w') as source:
	source.write(license_)
	source.write(u'\n#include "lexemes.hh"\n\n')
	source.write(u'#include "biblinternes/structures/dico_fixe.hh"\n\n')
	source.write(tableaux)
	source.write(fonction)
	source.write(u'\nstatic constexpr auto TAILLE_MAX_MOT_CLE = {};\n'.format(taille_max_mot_cles))
	source.write(fonctions)
