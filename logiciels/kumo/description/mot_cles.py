# -*- coding:utf-8 -*-

import io

mot_cles = [
	u'clé',
	u'texte',
	u'table',
	u'référence',
	u'cascade',
	u'supprime',
	u'taille',
	u'nul',
	u'auto_incrémente',
	u'clé_primaire',
	u'temps_date',
	u'temps',
	u'temps_courant',
	u'défaut',
	u'faux',
	u'vrai',
	u'ajourne',
	u'variable',
	u'octet',
	u'signé',
	u'zerofill',
	u'bit',
	u'entier',
	u'réel',
	u'chaîne',
	u'binaire',
]

mot_cles = sorted(mot_cles)

caracteres_simple = [
	[u'(', u'PARENTHESE_OUVRANTE'],
	[u')', u'PARENTHESE_FERMANTE'],
	[u'{', u'ACCOLADE_OUVRANTE'],
	[u'}', u'ACCOLADE_FERMANTE'],
	[u'.', u'POINT'],
	[u',', u'VIRGULE'],
	[u';', u'POINT_VIRGULE'],
	[u':', u'DOUBLE_POINTS'],
]

caracteres_simple = sorted(caracteres_simple)

id_extra = [
	u'CHAINE_CARACTERE',
	u'NOMBRE_ENTIER',
	u'NOMBRE_REEL',
	u'NOMBRE_BINAIRE',
	u'NOMBRE_OCTAL',
	u'NOMBRE_HEXADECIMAL',
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

	structures += u'\nstruct DonneesMorceaux {\n'
	structures += u'\tusing type = id_morceau;\n'
	structures += u'\tstatic constexpr type INCONNU = id_morceau::INCONNU;\n\n'
	structures += u'\tdls::vue_chaine chaine;\n'
	structures += u'\tunsigned long ligne_pos;\n'
	structures += u'\tid_morceau identifiant;\n'
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
		tableaux += u'\tdls::paire{{ dls::vue_chaine("{}"), id_morceau::{} }}'.format(mot, m)
		virgule = ',\n'

	tableaux += u'\n);\n\n'

	tableaux += u'static auto paires_caracteres_speciaux = dls::cree_dico(\n'

	virgule = ''

	for c in caracteres_simple:
		tableaux += virgule
		if c[0] == "'":
			c[0] = "\\'"

		tableaux += u"\tdls::paire{{ '{}', id_morceau::{} }}".format(c[0], c[1])
		virgule = ',\n'

	tableaux += u'\n);\n\n'

	return tableaux


def constuit_enumeration():
	enumeration = u'enum class id_morceau : unsigned int {\n'

	for car in caracteres_simple:
		enumeration += u'\t{},\n'.format(car[1])

	for mot in mot_cles + id_extra:
		m = enleve_accent(mot)
		m = m.upper()

		enumeration += u'\t{},\n'.format(m)

	enumeration += u'};\n'

	return enumeration


def construit_fonction_chaine_identifiant():
	fonction = u'const char *chaine_identifiant(id_morceau id)\n{\n'
	fonction += u'\tswitch (id) {\n'

	for car in caracteres_simple:
		fonction += u'\t\tcase id_morceau::{}:\n'.format(car[1])
		fonction += u'\t\t\treturn "id_morceau::{}";\n'.format(car[1])

	for mot in mot_cles + id_extra:
		m = enleve_accent(mot)
		m = m.upper()

		fonction += u'\t\tcase id_morceau::{}:\n'.format(m)
		fonction += u'\t\t\treturn "id_morceau::{}";\n'.format(m)

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
static id_morceau tables_identifiants[256] = {};
static bool tables_mots_cles[256] = {};

void construit_tables_caractere_speciaux()
{
	for (int i = 0; i < 256; ++i) {
		tables_caracteres[i] = false;
		tables_mots_cles[i] = false;
		tables_identifiants[i] = id_morceau::INCONNU;
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
	    auto plg = paires_mots_cles.plage();

	    while (!plg.est_finie()) {
		    tables_mots_cles[static_cast<unsigned char>(plg.front().premier[0])] = true;
	   		plg.effronte();
	    }
	}
}

bool est_caractere_special(char c, id_morceau &i)
{
	if (!tables_caracteres[static_cast<int>(c)]) {
		return false;
	}

	i = tables_identifiants[static_cast<int>(c)];
	return true;
}

id_morceau id_chaine(const dls::vue_chaine &chaine)
{
	if (!tables_mots_cles[static_cast<unsigned char>(chaine[0])]) {
		return id_morceau::CHAINE_CARACTERE;
	}

	auto iterateur = paires_mots_cles.trouve_binaire(chaine);

	if (!iterateur.est_finie()) {
		return iterateur.front().second;
	}

	return id_morceau::CHAINE_CARACTERE;
}
"""

declaration_fonctions = u"""
const char *chaine_identifiant(id_morceau id);

void construit_tables_caractere_speciaux();

bool est_caractere_special(char c, id_morceau &i);

id_morceau id_chaine(const dls::vue_chaine &chaine);
"""

with io.open(u"../coeur/decoupage/morceaux.hh", u'w') as entete:
	entete.write(license_)
	entete.write(u'\n#pragma once\n\n')
	entete.write(u'#include "biblinternes/structures/vue_chaine.hh"\n\n')
	entete.write(enumeration)
	entete.write(structures)
	entete.write(declaration_fonctions)


with io.open(u'../coeur/decoupage/morceaux.cc', u'w') as source:
	source.write(license_)
	source.write(u'\n#include "morceaux.hh"\n\n')
	source.write(u'#include "biblinternes/structures/dico_fixe.hh"\n\n')
	source.write(tableaux)
	source.write(fonction)
	source.write(fonctions)

