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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "unite_compilation.hh"

#include "arbre_syntaxique.hh"
#include "compilatrice.hh"

static constexpr auto CYCLES_MAXIMUM = 10;

const char *chaine_etat_unite(UniteCompilation::Etat etat)
{
#define ENUMERE_ETAT_UNITE_EX(etat) \
	case UniteCompilation::Etat::etat: return #etat;
	switch (etat) {
		ENUMERE_ETATS_UNITE
	}
#undef ENUMERE_ETAT_UNITE_EX

	return "erreur";
}

std::ostream &operator<<(std::ostream &os, UniteCompilation::Etat etat)
{
	os << chaine_etat_unite(etat);
	return os;
}

bool UniteCompilation::est_bloquee() const
{
	switch (etat()) {
		case UniteCompilation::Etat::ATTEND_SUR_TYPE:
		{
			return false;
		}
		case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
		{
			return false;
		}
		case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
		{
			return cycle > CYCLES_MAXIMUM;
		}
		case UniteCompilation::Etat::ATTEND_SUR_OPERATEUR:
		{
			return cycle > CYCLES_MAXIMUM;
		}
		case UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME:
		{
			return false;
		}
		case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
		{
			return false;
		}
		case UniteCompilation::Etat::PRETE:
		{
			return cycle > CYCLES_MAXIMUM;
		}
	}

	return false;
}

dls::chaine UniteCompilation::commentaire() const
{
	switch (etat()) {
		case UniteCompilation::Etat::ATTEND_SUR_TYPE:
		{
			return chaine_type(type_attendu);
		}
		case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
		{
			return declaration_attendue->ident->nom;
		}
		case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
		{
			return symbole_attendu->ident->nom;
		}
		case UniteCompilation::Etat::ATTEND_SUR_OPERATEUR:
		{
			return "opérateur + " + operateur_attendu->lexeme->chaine;
		}
		case UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME:
		{
			dls::chaine resultat = "métaprogramme";

			if (metaprogramme_attendu->corps_texte) {
				resultat += " #corps_texte pour ";

				if (metaprogramme_attendu->corps_texte_pour_fonction) {
					resultat += metaprogramme->corps_texte_pour_fonction->ident->nom;
				}
				else if (metaprogramme_attendu->corps_texte_pour_structure) {
					resultat += metaprogramme_attendu->corps_texte_pour_structure->ident->nom;
				}
				else {
					resultat += " ERREUR COMPILATRICE";
				}
			}
			else {
				resultat += " ";
				resultat += dls::vers_chaine(metaprogramme_attendu);
			}

			return resultat;
		}
		case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
		{
			return fonction_interface_attendue;
		}
		case UniteCompilation::Etat::PRETE:
		{
			return "prête";
		}
	}

	return "";
}

UniteCompilation *UniteCompilation::unite_attendue() const
{
	switch (etat()) {
		case UniteCompilation::Etat::ATTEND_SUR_TYPE:
		{
			if (type_attendu->est_structure()) {
				auto type_structure = type_attendu->comme_structure();
				return type_structure->decl->unite;
			}
			else if (type_attendu->est_union()) {
				auto type_union = type_attendu->comme_union();
				return type_union->decl->unite;
			}
			else if (type_attendu->est_enum()) {
				auto type_enum = type_attendu->comme_enum();
				return type_enum->decl->unite;
			}
			else if (type_attendu->est_erreur()) {
				auto type_erreur = type_attendu->comme_erreur();
				return type_erreur->decl->unite;
			}

			return nullptr;
		}
		case UniteCompilation::Etat::ATTEND_SUR_DECLARATION:
		{
			return declaration_attendue->unite;
		}
		case UniteCompilation::Etat::ATTEND_SUR_OPERATEUR:
		{
			return nullptr;
		}
		case UniteCompilation::Etat::ATTEND_SUR_METAPROGRAMME:
		{
			return metaprogramme_attendu->unite;
		}
		case UniteCompilation::Etat::ATTEND_SUR_INTERFACE_KURI:
		case UniteCompilation::Etat::ATTEND_SUR_SYMBOLE:
		case UniteCompilation::Etat::PRETE:
		{
			return nullptr;
		}
	}

	return nullptr;
}

dls::chaine chaine_attentes_recursives(UniteCompilation *unite)
{
	if (!unite) {
		return "    L'unité est nulle !\n";
	}

	dls::flux_chaine fc;

	auto attendue = unite->unite_attendue();
	auto commentaire = unite->commentaire();

	if (!attendue) {
		fc << "    " << commentaire << " est bloquée !\n";
	}

	while (attendue) {
		if (attendue->etat() == UniteCompilation::Etat::PRETE) {
			break;
		}

		fc << "    " << commentaire << " attend sur ";
		commentaire = attendue->commentaire();
		fc << commentaire << '\n';

		attendue = attendue->unite_attendue();
	}

	return fc.chn();
}
