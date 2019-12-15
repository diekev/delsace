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

#include "arbre_syntactic.h"

#include "biblinternes/outils/conditions.h"

/* ************************************************************************** */

static void imprime_tab(std::ostream &os, int tab)
{
	for (int i = 0; i < tab; ++i) {
		os << ' ' << ' ';
	}
}

char caractere_echape(char const *sequence)
{
	switch (sequence[0]) {
		case '\\':
			switch (sequence[1]) {
				case '\\':
					return '\\';
				case '\'':
					return '\'';
				case 'a':
					return '\a';
				case 'b':
					return '\b';
				case 'f':
					return '\f';
				case 'n':
					return '\n';
				case 'r':
					return '\r';
				case 't':
					return '\t';
				case 'v':
					return '\v';
				case '0':
					return '\0';
				default:
					return sequence[1];
			}
		default:
			return sequence[0];
	}
}

/* ************************************************************************** */

const char *chaine_type_noeud(type_noeud type)
{
#define CAS_TYPE(x) case x: return #x;

	switch (type) {
		CAS_TYPE(type_noeud::RACINE)
		CAS_TYPE(type_noeud::DECLARATION_FONCTION)
		CAS_TYPE(type_noeud::DECLARATION_COROUTINE)
		CAS_TYPE(type_noeud::LISTE_PARAMETRES_FONCTION)
		CAS_TYPE(type_noeud::APPEL_FONCTION)
		CAS_TYPE(type_noeud::VARIABLE)
		CAS_TYPE(type_noeud::ACCES_MEMBRE_DE)
		CAS_TYPE(type_noeud::ACCES_MEMBRE_POINT)
		CAS_TYPE(type_noeud::ASSIGNATION_VARIABLE)
		CAS_TYPE(type_noeud::NOMBRE_REEL)
		CAS_TYPE(type_noeud::NOMBRE_ENTIER)
		CAS_TYPE(type_noeud::OPERATION_BINAIRE)
		CAS_TYPE(type_noeud::OPERATION_UNAIRE)
		CAS_TYPE(type_noeud::RETOUR)
		CAS_TYPE(type_noeud::RETOUR_MULTIPLE)
		CAS_TYPE(type_noeud::RETOUR_SIMPLE)
		CAS_TYPE(type_noeud::CHAINE_LITTERALE)
		CAS_TYPE(type_noeud::BOOLEEN)
		CAS_TYPE(type_noeud::CARACTERE)
		CAS_TYPE(type_noeud::SI)
		CAS_TYPE(type_noeud::BLOC)
		CAS_TYPE(type_noeud::POUR)
		CAS_TYPE(type_noeud::CONTINUE_ARRETE)
		CAS_TYPE(type_noeud::BOUCLE)
		CAS_TYPE(type_noeud::REPETE)
		CAS_TYPE(type_noeud::TANTQUE)
		CAS_TYPE(type_noeud::TRANSTYPE)
		CAS_TYPE(type_noeud::MEMOIRE)
		CAS_TYPE(type_noeud::NUL)
		CAS_TYPE(type_noeud::TAILLE_DE)
		CAS_TYPE(type_noeud::PLAGE)
		CAS_TYPE(type_noeud::DIFFERE)
		CAS_TYPE(type_noeud::NONSUR)
		CAS_TYPE(type_noeud::TABLEAU)
		CAS_TYPE(type_noeud::CONSTRUIT_TABLEAU)
		CAS_TYPE(type_noeud::CONSTRUIT_STRUCTURE)
		CAS_TYPE(type_noeud::INFO_DE)
		CAS_TYPE(type_noeud::LOGE)
		CAS_TYPE(type_noeud::DELOGE)
		CAS_TYPE(type_noeud::RELOGE)
		CAS_TYPE(type_noeud::DECLARATION_STRUCTURE)
		CAS_TYPE(type_noeud::DECLARATION_ENUM)
		CAS_TYPE(type_noeud::ASSOCIE)
		CAS_TYPE(type_noeud::PAIRE_ASSOCIATION)
		CAS_TYPE(type_noeud::SAUFSI)
		CAS_TYPE(type_noeud::RETIENS)
		CAS_TYPE(type_noeud::ACCES_TABLEAU)
		CAS_TYPE(type_noeud::OPERATION_COMP_CHAINEE)
		CAS_TYPE(type_noeud::ASSOCIE_UNION)
		CAS_TYPE(type_noeud::ACCES_MEMBRE_UNION)
		CAS_TYPE(type_noeud::SINON)
	}

	return "erreur : type_noeud inconnu";
#undef CAS_TYPE
}

/* ************************************************************************** */

namespace noeud {

base::base(ContexteGenerationCode &/*contexte*/, DonneesMorceau const &morceau_)
	: morceau{morceau_}
{}

dls::vue_chaine_compacte const &base::chaine() const
{
	return morceau.chaine;
}

DonneesMorceau const &base::donnees_morceau() const
{
	return morceau;
}

base *base::dernier_enfant() const
{
	if (this->enfants.est_vide()) {
		return nullptr;
	}

	return this->enfants.back();
}

void base::ajoute_noeud(base *noeud)
{
	this->enfants.pousse(noeud);
}

void base::imprime_code(std::ostream &os, int tab)
{
	imprime_tab(os, tab);

	os << chaine_type_noeud(this->type) << ' ';

	if (dls::outils::possede_drapeau(this->drapeaux, EST_CALCULE)) {
		if (this->type == type_noeud::NOMBRE_ENTIER) {
			os << std::any_cast<long>(this->valeur_calculee);
		}
		else if (this->type == type_noeud::NOMBRE_REEL) {
			os << std::any_cast<double>(this->valeur_calculee);
		}
		else if (this->type == type_noeud::BOOLEEN) {
			os << ((std::any_cast<bool>(this->valeur_calculee)) ? "vrai" : "faux");
		}
		else if (this->type == type_noeud::CHAINE_LITTERALE) {
			os << this->chaine_calculee();
		}
	}
	else if (this->type == type_noeud::TRANSTYPE) {
		os << this->index_type;
	}
	else if (this->type == type_noeud::TAILLE_DE) {
		os << this->index_type;
	}
	else if (this->type != type_noeud::RACINE) {
		os << morceau.chaine;
	}

	os << ":\n";

	for (auto enfant : this->enfants) {
		enfant->imprime_code(os, tab + 1);
	}
}

dls::chaine base::chaine_calculee() const
{
	return std::any_cast<dls::chaine>(this->valeur_calculee);
}

id_morceau base::identifiant() const
{
	return morceau.identifiant;
}

/* ************************************************************************** */

void rassemble_feuilles(
		base *noeud_base,
		dls::tableau<base *> &feuilles)
{
	if (noeud_base->identifiant() != id_morceau::VIRGULE) {
		feuilles.pousse(noeud_base);
		return;
	}

	for (auto enfant : noeud_base->enfants) {
		if (enfant->identifiant() == id_morceau::VIRGULE) {
			rassemble_feuilles(enfant, feuilles);
		}
		else {
			feuilles.pousse(enfant);
		}
	}
}

void ajoute_nom_argument(base *b, const dls::vue_chaine_compacte &nom)
{
	auto noms_arguments = std::any_cast<dls::liste<dls::vue_chaine_compacte>>(&b->valeur_calculee);
	noms_arguments->pousse(nom);
}

}  /* namespace noeud */
