/*
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

#include "analyseuse_grammaire.hh"

#include <iostream>
#include <set>

#include "erreur.hh"
#include "nombres.hh"

/* ************************************************************************** */

static bool est_drapeaux(DonneesMorceaux::type identifiant)
{
	switch (identifiant) {
		case ID_VARIABLE:
		case ID_AUTO_INCREMENTE:
		case ID_CLE_PRIMAIRE:
			return true;
		default:
			return false;
	}
}

static void initialise_colonne(Colonne &colonne)
{
	switch (colonne.type) {
		case ID_ENTIER:
			colonne.octet = 4;
			colonne.taille = 10;
			break;
		case ID_BIT:
			colonne.octet = 1;
			break;
		case ID_CHAINE:
			break;
		case ID_BINAIRE:
			break;
		case ID_TEMPS:
			break;
		case ID_TEMPS_DATE:
			break;
		case ID_TEXTE:
			break;
	}
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(dls::tableau<DonneesMorceaux> &identifiants, lng::tampon_source const &tampon)
	: lng::analyseuse<DonneesMorceaux>(identifiants)
	, m_tampon(tampon)
{}

void analyseuse_grammaire::lance_analyse(std::ostream &os)
{
	m_position = 0;

	if (m_identifiants.taille() == 0) {
		return;
	}

	this->analyse_schema();
}

const Schema *analyseuse_grammaire::schema() const
{
	return &m_schema;
}

void analyseuse_grammaire::analyse_schema()
{
	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("Le script doit commencer par le nom de la base de données");
	}

	m_schema.nom = donnees().chaine;

	if (!requiers_identifiant(ID_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante '{' après le nom de la base de données");
	}

	this->analyse_declaration_table();

	if (!requiers_identifiant(ID_ACCOLADE_FERMANTE)) {
		lance_erreur("Le script doit terminer par une accolade fermante '}'");
	}
}

void analyseuse_grammaire::analyse_declaration_table()
{
	if (est_identifiant(ID_ACCOLADE_FERMANTE)) {
		return;
	}

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("La déclaration d'une table doit commencer par un nom");
	}

	auto table = Table{};
	table.nom = donnees().chaine;

	for (const auto &t : m_schema.tables) {
		if (t.nom == table.nom) {
			/* À FAIRE : position de la première déclaration. */
			lance_erreur("Plusieurs tables ont le même nom");
		}
	}

	if (!requiers_identifiant(ID_ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante '{' après le nom de la table");
	}

	this->analyse_declaration_colonne(table);

	if (!requiers_identifiant(ID_ACCOLADE_FERMANTE)) {
		lance_erreur("La déclaration d'une table doit terminer par une accolade fermante '}'");
	}

	m_schema.tables.push_back(table);

	this->analyse_declaration_table();
}

void analyseuse_grammaire::analyse_declaration_colonne(Table &table)
{
	auto colonne = Colonne{};

	if (!requiers_identifiant(ID_CHAINE_CARACTERE)) {
		lance_erreur("La déclaration d'une colonne doit commencer par un nom");
	}

	/* À FAIRE : vérifie que la colonne est unique. */
	colonne.nom = donnees().chaine;

	for (const auto &c : table.colonnes) {
		if (c.nom == colonne.nom) {
			/* À FAIRE : position de la première déclaration. */
			lance_erreur("Plusieurs colonnes ont le même nom");
		}
	}

	if (!requiers_identifiant(ID_DOUBLE_POINTS)) {
		lance_erreur("Attendu un double point ':' après le nom de la colonne");
	}

	if (!requiers_type(identifiant_courant())) {
		lance_erreur("Attendu la déclaration d'un type après ':");
	}

	colonne.type = donnees().identifiant;
	initialise_colonne(colonne);

	if (!requiers_identifiant(ID_PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une paranthèse ouvrante '(' après le type de la colonne");
	}

	while (!est_identifiant(ID_PARENTHESE_FERMANTE)) {
		analyse_propriete_colonne(colonne);
	}

	if (!requiers_identifiant(ID_PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une paranthère fermante ')' à la fin de la déclaration de la colonne");
	}

	/* À FAIRE : validation des propriétés. */
	table.colonnes.push_back(colonne);

	if (est_identifiant(ID_CHAINE_CARACTERE)) {
		this->analyse_declaration_colonne(table);
	}
}

bool analyseuse_grammaire::requiers_type(DonneesMorceaux::type identifiant)
{
	avance();

	switch (identifiant) {
		case ID_ENTIER:
		case ID_BIT:
		case ID_CHAINE:
		case ID_BINAIRE:
		case ID_CLE:
		case ID_TEMPS:
		case ID_TEMPS_DATE:
		case ID_TEXTE:
			return true;
		default:
			return false;
	}
}

static void initialise_propriete(
		Colonne &colonne,
		size_t id_propriete,
		const DonneesMorceaux &donnees)
{
	switch (id_propriete) {
		case ID_SIGNE:
			colonne.signee = (donnees.identifiant == ID_VRAI);
			break;
		case ID_SUPPRIME:
			colonne.suppression = donnees.identifiant;
			break;
		case ID_NUL:
			colonne.peut_etre_nulle = (donnees.identifiant == ID_VRAI);
			break;
		case ID_DEFAUT:
			colonne.a_valeur_defaut = true;
			colonne.id_valeur_defaut = donnees.identifiant;
			colonne.defaut = donnees.chaine;
			break;
		case ID_TAILLE:
			colonne.taille = std::atoi(std::string(donnees.chaine).c_str());
			break;
		case ID_OCTET:
			colonne.octet = std::atoi(std::string(donnees.chaine).c_str());
			break;
		case ID_TABLE:
			colonne.table = donnees.chaine;
			break;
		case ID_REFERENCE:
			colonne.ref = donnees.chaine;
			break;
	}
}

void analyseuse_grammaire::analyse_propriete_colonne(Colonne &colonne)
{
	if (est_drapeaux(identifiant_courant())) {
		switch (identifiant_courant()) {
			case ID_VARIABLE:
				colonne.variable = true;
				break;
			case ID_AUTO_INCREMENTE:
				colonne.auto_inc = true;
				break;
			case ID_CLE_PRIMAIRE:
				colonne.cle_primaire = true;
				break;
		}

		avance();
	}
	else {
		if (!requiers_propriete(identifiant_courant())) {
			lance_erreur("Attendu la déclaration d'une propriété");
		}

		auto id_propriete = donnees().identifiant;

		if (!requiers_identifiant(ID_DOUBLE_POINTS)) {
			lance_erreur("Attendu un double point ':' après la propriété");
		}

		if (!requiers_valeur(identifiant_courant())) {
			lance_erreur("Attendu une valeur après le double point ':'");
		}

		initialise_propriete(colonne, id_propriete, donnees());
	}

	if (est_identifiant(ID_VIRGULE)) {
		avance();
	}
}

bool analyseuse_grammaire::requiers_propriete(DonneesMorceaux::type identifiant)
{
	avance();

	switch (identifiant) {
		case ID_SIGNE:
		case ID_SUPPRIME:
		case ID_NUL:
		case ID_DEFAUT:
		case ID_TAILLE:
		case ID_OCTET:
		case ID_TABLE:
		case ID_REFERENCE:
			return true;
		default:
			return false;
	}
}

bool analyseuse_grammaire::requiers_valeur(DonneesMorceaux::type identifiant)
{
	avance();

	switch (identifiant) {
		case ID_NOMBRE_ENTIER:
		case ID_NOMBRE_HEXADECIMAL:
		case ID_NOMBRE_BINAIRE:
		case ID_NOMBRE_OCTAL:
		case ID_NUL:
		case ID_FAUX:
		case ID_VRAI:
		case ID_TEMPS_COURANT:
		case ID_CHAINE_CARACTERE:
		case ID_CASCADE:
			return true;
		default:
			return false;
	}
}

void analyseuse_grammaire::lance_erreur(const std::string &quoi, int type)
{
	erreur::lance_erreur(quoi, m_tampon, donnees(), type);
}
