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

/* ************************************************************************** */

const char *chaine_genre_noeud(GenreNoeud genre)
{
#define CAS_GENRE(x) case x: return #x;

	switch (genre) {
		CAS_GENRE(GenreNoeud::RACINE)
		CAS_GENRE(GenreNoeud::DECLARATION_FONCTION)
		CAS_GENRE(GenreNoeud::DECLARATION_COROUTINE)
		CAS_GENRE(GenreNoeud::DECLARATION_PARAMETRES_FONCTION)
		CAS_GENRE(GenreNoeud::EXPRESSION_APPEL_FONCTION)
		CAS_GENRE(GenreNoeud::EXPRESSION_REFERENCE_DECLARATION)
		CAS_GENRE(GenreNoeud::EXPRESSION_REFERENCE_MEMBRE)
		CAS_GENRE(GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE)
		CAS_GENRE(GenreNoeud::DECLARATION_VARIABLE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER)
		CAS_GENRE(GenreNoeud::OPERATEUR_BINAIRE)
		CAS_GENRE(GenreNoeud::OPERATEUR_UNAIRE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETOUR)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETOUR_SIMPLE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_CHAINE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_CARACTERE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_SI)
		CAS_GENRE(GenreNoeud::INSTRUCTION_COMPOSEE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_POUR)
		CAS_GENRE(GenreNoeud::INSTRUCTION_CONTINUE_ARRETE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_BOUCLE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_REPETE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_TANTQUE)
		CAS_GENRE(GenreNoeud::EXPRESSION_TRANSTYPE)
		CAS_GENRE(GenreNoeud::EXPRESSION_MEMOIRE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LITTERALE_NUL)
		CAS_GENRE(GenreNoeud::EXPRESSION_TAILLE_DE)
		CAS_GENRE(GenreNoeud::EXPRESSION_PLAGE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_DIFFERE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_NONSUR)
		CAS_GENRE(GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES)
		CAS_GENRE(GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU)
		CAS_GENRE(GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE)
		CAS_GENRE(GenreNoeud::EXPRESSION_INFO_DE)
		CAS_GENRE(GenreNoeud::EXPRESSION_LOGE)
		CAS_GENRE(GenreNoeud::EXPRESSION_DELOGE)
		CAS_GENRE(GenreNoeud::EXPRESSION_RELOGE)
		CAS_GENRE(GenreNoeud::DECLARATION_STRUCTURE)
		CAS_GENRE(GenreNoeud::DECLARATION_ENUM)
		CAS_GENRE(GenreNoeud::INSTRUCTION_DISCR)
		CAS_GENRE(GenreNoeud::INSTRUCTION_PAIRE_DISCR)
		CAS_GENRE(GenreNoeud::INSTRUCTION_SAUFSI)
		CAS_GENRE(GenreNoeud::INSTRUCTION_RETIENS)
		CAS_GENRE(GenreNoeud::EXPRESSION_INDICE)
		CAS_GENRE(GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE)
		CAS_GENRE(GenreNoeud::INSTRUCTION_DISCR_ENUM)
		CAS_GENRE(GenreNoeud::INSTRUCTION_DISCR_UNION)
		CAS_GENRE(GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION)
		CAS_GENRE(GenreNoeud::INSTRUCTION_SINON)
		CAS_GENRE(GenreNoeud::EXPRESSION_PARENTHESE)
		CAS_GENRE(GenreNoeud::DIRECTIVE_EXECUTION)
	}

	return "erreur : GenreNoeud inconnu";
#undef CAS_GENRE
}

/* ************************************************************************** */

namespace noeud {

base::base(DonneesLexeme const &morceau_)
	: morceau{morceau_}
{}

dls::vue_chaine_compacte const &base::chaine() const
{
	return morceau.chaine;
}

DonneesLexeme const &base::donnees_morceau() const
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

	os << chaine_genre_noeud(this->genre) << ' ';

	if (dls::outils::possede_drapeau(this->drapeaux, EST_CALCULE)) {
		if (this->genre == GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER) {
			os << std::any_cast<long>(this->valeur_calculee);
		}
		else if (this->genre == GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL) {
			os << std::any_cast<double>(this->valeur_calculee);
		}
		else if (this->genre == GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN) {
			os << ((std::any_cast<bool>(this->valeur_calculee)) ? "vrai" : "faux");
		}
		else if (this->genre == GenreNoeud::EXPRESSION_LITTERALE_CHAINE) {
			os << this->chaine_calculee();
		}
	}
	else if (this->genre == GenreNoeud::EXPRESSION_TRANSTYPE) {
		os << this->index_type;
	}
	else if (this->genre == GenreNoeud::EXPRESSION_TAILLE_DE) {
		os << this->index_type;
	}
	else if (this->genre != GenreNoeud::RACINE) {
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

GenreLexeme base::identifiant() const
{
	return morceau.genre;
}

/* ************************************************************************** */

void rassemble_feuilles(
		base *noeud_base,
		dls::tableau<base *> &feuilles)
{
	if (noeud_base->identifiant() != GenreLexeme::VIRGULE) {
		feuilles.pousse(noeud_base);
		return;
	}

	for (auto enfant : noeud_base->enfants) {
		if (enfant->identifiant() == GenreLexeme::VIRGULE) {
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
