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

#include "erreur.hh"

/* ************************************************************************** */

static bool est_drapeaux(DonneesMorceaux::type identifiant)
{
	switch (identifiant) {
		case id_morceau::VARIABLE:
		case id_morceau::AUTO_INCREMENTE:
		case id_morceau::CLE_PRIMAIRE:
			return true;
		default:
			return false;
	}
}

static void initialise_colonne(Colonne &colonne)
{
	switch (colonne.type) {
		case id_morceau::ENTIER:
			colonne.octet = 4;
			colonne.taille = 10;
			break;
		case id_morceau::BIT:
			colonne.octet = 1;
			break;
		case id_morceau::CHAINE:
			break;
		case id_morceau::BINAIRE:
			break;
		case id_morceau::TEMPS:
			break;
		case id_morceau::TEMPS_DATE:
			break;
		case id_morceau::TEXTE:
			break;
		default:
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
	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
		lance_erreur("Le script doit commencer par le nom de la base de données");
	}

	m_schema.nom = donnees().chaine;

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante '{' après le nom de la base de données");
	}

	this->analyse_declaration_table();

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("Le script doit terminer par une accolade fermante '}'");
	}
}

void analyseuse_grammaire::analyse_declaration_table()
{
	if (est_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		return;
	}

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
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

	if (!requiers_identifiant(id_morceau::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante '{' après le nom de la table");
	}

	this->analyse_declaration_colonne(table);

	if (!requiers_identifiant(id_morceau::ACCOLADE_FERMANTE)) {
		lance_erreur("La déclaration d'une table doit terminer par une accolade fermante '}'");
	}

	m_schema.tables.pousse(table);

	this->analyse_declaration_table();
}

void analyseuse_grammaire::analyse_declaration_colonne(Table &table)
{
	auto colonne = Colonne{};

	if (!requiers_identifiant(id_morceau::CHAINE_CARACTERE)) {
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

	if (!requiers_identifiant(id_morceau::DOUBLE_POINTS)) {
		lance_erreur("Attendu un double point ':' après le nom de la colonne");
	}

	if (!requiers_type(identifiant_courant())) {
		lance_erreur("Attendu la déclaration d'un type après ':");
	}

	colonne.type = donnees().genre;
	initialise_colonne(colonne);

	if (!requiers_identifiant(id_morceau::PARENTHESE_OUVRANTE)) {
		lance_erreur("Attendu une paranthèse ouvrante '(' après le type de la colonne");
	}

	while (!est_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		analyse_propriete_colonne(colonne);
	}

	if (!requiers_identifiant(id_morceau::PARENTHESE_FERMANTE)) {
		lance_erreur("Attendu une paranthère fermante ')' à la fin de la déclaration de la colonne");
	}

	/* À FAIRE : validation des propriétés. */
	table.colonnes.pousse(colonne);

	if (est_identifiant(id_morceau::CHAINE_CARACTERE)) {
		this->analyse_declaration_colonne(table);
	}
}

bool analyseuse_grammaire::requiers_type(DonneesMorceaux::type identifiant)
{
	avance();

	switch (identifiant) {
		case id_morceau::ENTIER:
		case id_morceau::BIT:
		case id_morceau::CHAINE:
		case id_morceau::BINAIRE:
		case id_morceau::CLE:
		case id_morceau::TEMPS:
		case id_morceau::TEMPS_DATE:
		case id_morceau::TEXTE:
			return true;
		default:
			return false;
	}
}

static void initialise_propriete(
		Colonne &colonne,
		id_morceau id_propriete,
		const DonneesMorceaux &donnees)
{
	switch (id_propriete) {
		case id_morceau::SIGNE:
			colonne.signee = (donnees.genre == id_morceau::VRAI);
			break;
		case id_morceau::SUPPRIME:
			colonne.suppression = donnees.genre;
			break;
		case id_morceau::NUL:
			colonne.peut_etre_nulle = (donnees.genre == id_morceau::VRAI);
			break;
		case id_morceau::DEFAUT:
			colonne.a_valeur_defaut = true;
			colonne.id_valeur_defaut = donnees.genre;
			colonne.defaut = donnees.chaine;
			break;
		case id_morceau::TAILLE:
			colonne.taille = std::atoi(dls::chaine(donnees.chaine).c_str());
			break;
		case id_morceau::OCTET:
			colonne.octet = std::atoi(dls::chaine(donnees.chaine).c_str());
			break;
		case id_morceau::TABLE:
			colonne.table = donnees.chaine;
			break;
		case id_morceau::REFERENCE:
			colonne.ref = donnees.chaine;
			break;
		default:
			break;
	}
}

void analyseuse_grammaire::analyse_propriete_colonne(Colonne &colonne)
{
	if (est_drapeaux(identifiant_courant())) {
		switch (identifiant_courant()) {
			case id_morceau::VARIABLE:
				colonne.variable = true;
				break;
			case id_morceau::AUTO_INCREMENTE:
				colonne.auto_inc = true;
				break;
			case id_morceau::CLE_PRIMAIRE:
				colonne.cle_primaire = true;
				break;
			default:
				break;
		}

		avance();
	}
	else {
		if (!requiers_propriete(identifiant_courant())) {
			lance_erreur("Attendu la déclaration d'une propriété");
		}

		auto id_propriete = donnees().genre;

		if (!requiers_identifiant(id_morceau::DOUBLE_POINTS)) {
			lance_erreur("Attendu un double point ':' après la propriété");
		}

		if (!requiers_valeur(identifiant_courant())) {
			lance_erreur("Attendu une valeur après le double point ':'");
		}

		initialise_propriete(colonne, id_propriete, donnees());
	}

	if (est_identifiant(id_morceau::VIRGULE)) {
		avance();
	}
}

bool analyseuse_grammaire::requiers_propriete(DonneesMorceaux::type identifiant)
{
	avance();

	switch (identifiant) {
		case id_morceau::SIGNE:
		case id_morceau::SUPPRIME:
		case id_morceau::NUL:
		case id_morceau::DEFAUT:
		case id_morceau::TAILLE:
		case id_morceau::OCTET:
		case id_morceau::TABLE:
		case id_morceau::REFERENCE:
			return true;
		default:
			return false;
	}
}

bool analyseuse_grammaire::requiers_valeur(DonneesMorceaux::type identifiant)
{
	avance();

	switch (identifiant) {
		case id_morceau::NOMBRE_ENTIER:
		case id_morceau::NOMBRE_HEXADECIMAL:
		case id_morceau::NOMBRE_BINAIRE:
		case id_morceau::NOMBRE_OCTAL:
		case id_morceau::NUL:
		case id_morceau::FAUX:
		case id_morceau::VRAI:
		case id_morceau::TEMPS_COURANT:
		case id_morceau::CHAINE_CARACTERE:
		case id_morceau::CASCADE:
			return true;
		default:
			return false;
	}
}

void analyseuse_grammaire::lance_erreur(const dls::chaine &quoi, int type)
{
	erreur::lance_erreur(quoi, m_tampon, donnees(), type);
}
