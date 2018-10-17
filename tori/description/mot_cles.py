# -*- coding:utf-8 -*-

import io

mot_cles = [
	u'si',
	u'sinon',
	u'finsi',
	u'pour',
	u'finpour',
	u'étend',
	u'dans',
]

mot_cles = sorted(mot_cles)

id_extra = [
	u'DEBUT_VARIABLE',
	u'DEBUT_EXPRESSION',
	u'FIN_VARIABLE',
	u'FIN_EXPRESSION',
	u'CHAINE_CARACTERE',
	u'INCONNU',
]


def enleve_accent(mot):
	mot = mot.replace(u'é', 'e')
	mot = mot.replace(u'è', 'e')
	mot = mot.replace(u'â', 'a')
	mot = mot.replace(u'ê', 'e')
	mot = mot.replace(u'ô', 'o')
	mot = mot.replace(u'î', 'i')

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

	return tableaux


def constuit_enumeration():
	enumeration = u'enum {\n'

	for mot in mot_cles + id_extra:
		m = enleve_accent(mot)
		m = m.upper()

		enumeration += u'\tID_{},\n'.format(m)

	enumeration += u'};\n'

	return enumeration


def construit_fonction_chaine_identifiant():
	fonction = u'const char *chaine_identifiant(int id)\n{\n'
	fonction += u'\tswitch (id) {\n'

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
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_mots_cles[i] = false;
	}

	for (const auto &iter : paires_mots_cles) {
		tables_mots_cles[static_cast<unsigned char>(iter.first[0])] = true;
	}
}

int id_chaine(const std::string_view &chaine)
{
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

int id_chaine(const std::string_view &chaine);
"""

with io.open(u"../coeur/decoupage/morceaux.hh", u'w') as entete:
	entete.write(license_)
	entete.write(u'\n#pragma once\n\n')
	entete.write(u'#include <string>\n\n')
	entete.write(structures)
	entete.write(enumeration)
	entete.write(declaration_fonctions)


with io.open(u'../coeur/decoupage/morceaux.cc', u'w') as source:
	source.write(license_)
	source.write(u'\n#include "morceaux.hh"\n\n')
	source.write(u'#include <map>\n\n')
	source.write(tableaux)
	source.write(fonction)
	source.write(fonctions)
