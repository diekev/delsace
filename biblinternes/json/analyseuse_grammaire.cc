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

#include "biblinternes/langage/nombres.hh"

#include "erreur.hh"

namespace json {

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

void imprime_arbre(tori::Objet *racine, int tab, std::ostream &os)
{
	switch (racine->type) {
		case tori::type_objet::NUL:
		{
			os << "nul,\n";
			break;
		}
		case tori::type_objet::DICTIONNAIRE:
		{
			imprime_tab(os, tab);
			os << "{\n";

			auto obj_dico = static_cast<tori::ObjetDictionnaire *>(racine);

			for (auto &paire : obj_dico->valeur) {
				imprime_tab(os, tab + 1);
				os << '"' << paire.first << '"' << " : ";
				imprime_arbre(paire.second.get(), tab + 1, os);
			}

			imprime_tab(os, tab);
			os << "},\n";

			break;
		}
		case tori::type_objet::TABLEAU:
		{
			os << "[\n";

			auto obj_tabl = static_cast<tori::ObjetTableau *>(racine);

			for (auto &obj : obj_tabl->valeur) {
				imprime_arbre(obj.get(), tab + 1, os);
			}

			imprime_tab(os, tab);
			os << "],\n";
			break;
		}
		case tori::type_objet::CHAINE:
		{
			auto obj = static_cast<tori::ObjetChaine *>(racine);
			os << '"' << obj->valeur << '"' << ",\n";
			break;
		}
		case tori::type_objet::NOMBRE_ENTIER:
		{
			auto obj = static_cast<tori::ObjetNombreEntier *>(racine);
			os << obj->valeur << ",\n";
			break;
		}
		case tori::type_objet::NOMBRE_REEL:
		{
			auto obj = static_cast<tori::ObjetNombreReel *>(racine);
			os << obj->valeur << ",\n";
			break;
		}
	}
}

/* ************************************************************************** */

analyseuse_grammaire::analyseuse_grammaire(
		dls::tableau<DonneesMorceau> &identifiants,
		lng::tampon_source const &tampon)
	: lng::analyseuse<DonneesMorceau>(identifiants)
	, m_tampon(tampon)
{}

void analyseuse_grammaire::lance_analyse(std::ostream &os)
{
	m_position = 0;

	if (m_identifiants.taille() == 0) {
		return;
	}

	if (!this->requiers_identifiant(type_id::ACCOLADE_OUVRANTE)) {
		lance_erreur("Attendu une accolade ouvrante '{' au début du script");
	}

	analyse_objet();

	if (!this->requiers_identifiant(type_id::ACCOLADE_FERMANTE)) {
		lance_erreur("Attendu une accolade fermante '}' à la fin du script");
	}

	//imprime_arbre(m_assembleuse.racine.get(), 0, std::cerr);
}

assembleuse_objet::ptr_objet analyseuse_grammaire::objet() const
{
	return m_assembleuse.racine;
}

void analyseuse_grammaire::analyse_objet()
{
	if (!requiers_identifiant(type_id::CHAINE_CARACTERE)) {
		lance_erreur("Attendu une chaine de caractère");
	}

	auto nom_objet = donnees().chaine;

	if (!requiers_identifiant(type_id::DOUBLE_POINTS)) {
		lance_erreur("Attendu un double-point ':'");
	}

	analyse_valeur(nom_objet);

	if (est_identifiant(type_id::VIRGULE)) {
		avance();
		analyse_objet();
	}
}

void analyseuse_grammaire::analyse_valeur(dls::vue_chaine const &nom_objet)
{
	switch (identifiant_courant()) {
		case type_id::ACCOLADE_OUVRANTE:
		{
			avance();

			auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::DICTIONNAIRE);
			m_assembleuse.empile_objet(obj);

			analyse_objet();

			m_assembleuse.depile_objet();

			if (!this->requiers_identifiant(type_id::ACCOLADE_FERMANTE)) {
				lance_erreur("Attendu une accolade fermante '}' à la fin de l'objet");
			}

			break;
		}
		case type_id::CROCHET_OUVRANT:
		{
			avance();

			auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::TABLEAU);
			m_assembleuse.empile_objet(obj);

			while (true) {
				if (est_identifiant(type_id::CROCHET_FERMANT)) {
					break;
				}

				analyse_valeur("");

				if (est_identifiant(type_id::VIRGULE)) {
					avance();
				}
			}

			m_assembleuse.depile_objet();

			avance();

			break;
		}
		case type_id::NOMBRE_ENTIER:
		case type_id::NOMBRE_BINAIRE:
		case type_id::NOMBRE_HEXADECIMAL:
		case type_id::NOMBRE_OCTAL:
		{
			avance();

			using denombreuse = lng::decoupeuse_nombre<id_morceau>;

			auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::NOMBRE_ENTIER);
			auto obj_chaine = static_cast<tori::ObjetNombreEntier *>(obj.get());
			obj_chaine->valeur = denombreuse::converti_chaine_nombre_entier(
						donnees().chaine,
						donnees().genre);

			break;
		}
		case type_id::NOMBRE_REEL:
		{
			avance();

			using denombreuse = lng::decoupeuse_nombre<id_morceau>;

			auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::NOMBRE_REEL);
			auto obj_chaine = static_cast<tori::ObjetNombreReel *>(obj.get());
			obj_chaine->valeur = denombreuse::converti_chaine_nombre_reel(
						donnees().chaine,
						donnees().genre);

			break;
		}
		case type_id::CHAINE_CARACTERE:
		{
			avance();

			auto obj = m_assembleuse.cree_objet(nom_objet, tori::type_objet::CHAINE);
			auto obj_chaine = static_cast<tori::ObjetChaine *>(obj.get());

			auto res = dls::chaine();

			for (auto i = 0; i < donnees().chaine.taille(); ++i) {
				auto c = donnees().chaine[i];

				if (c == '\\') {
					continue;
				}

				res += c;
			}

			obj_chaine->valeur = res;

			break;
		}
		default:
		{
			avance();
			lance_erreur("Élément inattendu");
		}
	}
}

void analyseuse_grammaire::lance_erreur(const dls::chaine &quoi, int type)
{
	erreur::lance_erreur(quoi, m_tampon, donnees(), type);
}

}  /* namespace json */
