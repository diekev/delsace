# -*- coding:utf-8 -*-

import io

mot_cles = [
	u'soit',
	u'constante',
	u'variable',
	u'fonction',
	u'boucle',
	u'pour',
	u'dans',
	u'arrête',
	u'continue',
	u'associe',
	u'si',
	u'sinon',
	u'énum',
	u'structure',
	u'gabarit',
	u'de',
	u'retourne',
	u'défère',
	u'transtype',
	u'vrai',
	u'faux',
	u'taille_de',
	u'mémoire',
	u'type',
	u'employant',
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
]

caracteres_simple = sorted(caracteres_simple)

caracteres_double = [
	[u'<=', u'INFERIEUR_EGAL'],
	[u'>=', u'SUPERIEUR_EGAL'],
	[u'==', u'EGALITE'],
	[u'!=', u'DIFFERENCE'],
	[u'&&', u'ESP_ESP'],
	[u'||', u'BARRE_BARRE'],
	[u'<<', u'DECALAGE_GAUCHE'],
	[u'>>', u'DECALAGE_DROITE'],
]

caracteres_double = sorted(caracteres_double)

id_extra = [
	u'NOMBRE_REEL',
	u'NOMBRE_ENTIER',
	u'NOMBRE_HEXADECIMAL',
	u'NOMBRE_OCTAL',
	u'NOMBRE_BINAIRE',
	u'TROIS_POINTS',
    u"CHAINE_CARACTERE",
    u"CHAINE_LITTERALE",
    u"CARACTERE",
    u"POINTEUR",
    u"TABLEAU",
    u"REFERENCE",
	u'INCONNU',
]


def enleve_accent(mot):
	mot = mot.replace(u'é', 'e')
	mot = mot.replace(u'è', 'e')
	mot = mot.replace(u'â', 'a')
	mot = mot.replace(u'ê', 'e')
	mot = mot.replace(u'ô', 'o')

	return mot


def construit_structures():
	structures = u''

	structures += u'struct DonneesMorceaux {\n'
	structures += u'\tstd::string_view chaine;\n'
	structures += u'\tsize_t ligne_pos;\n'
	structures += u'\tsize_t identifiant;\n'
	structures += u'};\n\n'

	return structures


def construit_tableaux():
	tableaux = u''

	tableaux += u'static std::map<std::string_view, int> paires_mots_cles = {\n'

	for mot in mot_cles:
		m = enleve_accent(mot)
		m = m.upper()
		tableaux += u'\t{{ "{}", ID_{} }},\n'.format(mot, m)

	tableaux += u'};\n\n'

	tableaux += u'static std::map<std::string_view, int> paires_caracteres_double = {\n'

	for c in caracteres_double:
		tableaux += u'\t{{ "{}", ID_{} }},\n'.format(c[0], c[1])

	tableaux += u'};\n\n'

	tableaux += u'static std::map<char, int> paires_caracteres_speciaux = {\n'

	for c in caracteres_simple:
		if c[0] == "'":
			c[0] = "\\'"

		tableaux += u"\t{{ '{}', ID_{} }},\n".format(c[0], c[1])

	tableaux += u'};\n\n'

	return tableaux


def constuit_enumeration():
	enumeration = u'enum {\n'

	for car in caracteres_simple:
		enumeration += u'\tID_{},\n'.format(car[1])

	for car in caracteres_double:
		enumeration += u'\tID_{},\n'.format(car[1])

	for mot in mot_cles + id_extra:
		m = enleve_accent(mot)
		m = m.upper()

		enumeration += u'\tID_{},\n'.format(m)

	enumeration += u'};\n'

	return enumeration


def construit_fonction_chaine_identifiant():
	fonction = u'const char *chaine_identifiant(int id)\n{\n'
	fonction += u'\tswitch (id) {\n'

	for car in caracteres_simple:
		fonction += u'\t\tcase ID_{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "ID_{}";\n'.format(car[1])

	for car in caracteres_double:
		fonction += u'\t\tcase ID_{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "ID_{}";\n'.format(car[1])

	for mot in mot_cles + id_extra:
		m = enleve_accent(mot)
		m = m.upper()

		fonction += u'\t\tcase ID_{}:\n'.format(m)
		fonction += u'\t\t\treturn "ID_{}";\n'.format(m)

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
static int tables_identifiants[256] = {};
static bool tables_caracteres_double[256] = {};
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_caracteres_double[i] = false;
		tables_mots_cles[i] = false;
		tables_identifiants[i] = -1;
	}

	for (const auto &iter : paires_caracteres_speciaux) {
		tables_caracteres[int(iter.first)] = true;
		tables_identifiants[int(iter.first)] = iter.second;
	}

	for (const auto &iter : paires_caracteres_double) {
		tables_caracteres_double[int(iter.first[0])] = true;
	}

	for (const auto &iter : paires_mots_cles) {
		tables_mots_cles[static_cast<unsigned char>(iter.first[0])] = true;
	}
}

bool est_caractere_special(char c, int &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

int id_caractere_double(const std::string_view &chaine)
{
	if (!tables_caracteres_double[int(chaine[0])]) {
		return ID_INCONNU;
	}

	auto iterateur = paires_caracteres_double.find(chaine);

	if (iterateur != paires_caracteres_double.end()) {
		return (*iterateur).second;
	}

	return ID_INCONNU;
}

int id_chaine(const std::string_view &chaine)
{
	if (chaine.size() == 1 || chaine.size() > TAILLE_MAX_MOT_CLE) {
		return ID_CHAINE_CARACTERE;
	}

	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return ID_CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.find(chaine);

	if (iterateur != paires_mots_cles.end()) {
		return (*iterateur).second;
	}

	return ID_CHAINE_CARACTERE;
}
"""

declaration_fonctions = u"""
const char *chaine_identifiant(int id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, int &i);

int id_caractere_double(const std::string_view &chaine);

int id_chaine(const std::string_view &chaine);
"""

with io.open(u"../coeur/decoupage/morceaux.h", u'w') as entete:
	entete.write(license_)
	entete.write(u'\n#pragma once\n\n')
	entete.write(u'#include <string>\n\n')
	entete.write(structures)
	entete.write(enumeration)
	entete.write(declaration_fonctions)


with io.open(u'../coeur/decoupage/morceaux.cc', u'w') as source:
	source.write(license_)
	source.write(u'\n#include "morceaux.h"\n\n')
	source.write(u'#include <map>\n\n')
	source.write(tableaux)
	source.write(fonction)
	source.write(u'\nstatic constexpr auto TAILLE_MAX_MOT_CLE = {};\n'.format(taille_max_mot_cles))
	source.write(fonctions)

