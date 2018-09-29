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
	u'e8',
	u'e16',
	u'e32',
	u'e64',
	u'e8ns',
	u'e16ns',
	u'e32ns',
	u'e64ns',
	u'r16',
	u'r32',
	u'r64',
	u'bool',
	u'rien',
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
	structures += u'\tstd::string chaine;\n'
	structures += u'\tint identifiant;\n'
	structures += u'\tint ligne;\n'
	structures += u'\tint pos;\n'
	structures += u'};\n\n'

	return structures


def construit_tableaux():
	tableaux = u''

	tableaux += u'static paire_identifiant_chaine paires_mots_cles[] = {\n'

	for mot in mot_cles:
		m = enleve_accent(mot)
		m = m.upper()
		tableaux += u'\t{' + u'ID_{}, "{}"'.format(m, mot) + '},\n'

	tableaux += u'};\n\n'

	tableaux += u'static paire_identifiant_chaine paires_caracteres_double[] = {\n'

	for c in caracteres_double:
		tableaux += u'\t{' + u'ID_{}, "{}"'.format(c[1], c[0]) + '},\n'

	tableaux += u'};\n\n'

	tableaux += u'static paire_identifiant_caractere paires_caracteres_speciaux[] = {\n'

	for c in caracteres_simple:
		if c[0] == "'":
			c[0] = "\\'"

		tableaux += u'\t{' + u"ID_{}, '{}'".format(c[1], c[0]) + '},\n'

	tableaux += u'};\n'

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
	fonction += u'}\n\n'

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

structures_paires = u"""
struct paire_identifiant_chaine {
	int identifiant;
	std::string chaine;
};

struct paire_identifiant_caractere {
	int identifiant;
	char caractere;
};

"""

fonctions = u"""
static bool comparaison_paires_identifiant(
		const paire_identifiant_chaine &a,
		const paire_identifiant_chaine &b)
{
	return a.chaine < b.chaine;
}

static bool comparaison_paires_caractere(
		const paire_identifiant_caractere &a,
		const paire_identifiant_caractere &b)
{
	return a.caractere < b.caractere;
}

bool est_caractere_special(char c, int &i)
{
	auto iterateur = std::lower_bound(
						 std::begin(paires_caracteres_speciaux),
						 std::end(paires_caracteres_speciaux),
						 paire_identifiant_caractere{ID_INCONNU, c},
						 comparaison_paires_caractere);

	if (iterateur != std::end(paires_caracteres_speciaux)) {
		if ((*iterateur).caractere == c) {
			i = (*iterateur).identifiant;
			return true;
		}
	}

	return false;
}

int id_caractere_double(const std::string &chaine)
{
	auto iterateur = std::lower_bound(
						 std::begin(paires_caracteres_double),
						 std::end(paires_caracteres_double),
						 paire_identifiant_chaine{ID_INCONNU, chaine},
						 comparaison_paires_identifiant);

	if (iterateur != std::end(paires_caracteres_double)) {
		if ((*iterateur).chaine == chaine) {
			return (*iterateur).identifiant;
		}
	}

	return ID_INCONNU;
}

int id_chaine(const std::string &chaine)
{
	auto iterateur = std::lower_bound(
						 std::begin(paires_mots_cles),
						 std::end(paires_mots_cles),
						 paire_identifiant_chaine{ID_INCONNU, chaine},
						 comparaison_paires_identifiant);

	if (iterateur != std::end(paires_mots_cles)) {
		if ((*iterateur).chaine == chaine) {
			return (*iterateur).identifiant;
		}
	}

	return ID_CHAINE_CARACTERE;
}
"""

declaration_fonctions = u"""
const char *chaine_identifiant(int id);
bool est_caractere_special(char c, int &i);
int id_caractere_double(const std::string &chaine);
int id_chaine(const std::string &chaine);
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
	source.write(u'#include <algorithm>\n')
	source.write(structures_paires)
	source.write(tableaux)
	source.write(fonction)
	source.write(fonctions)

