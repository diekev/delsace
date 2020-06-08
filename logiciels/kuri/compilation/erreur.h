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

#pragma once

#include "modules.hh"

#include "biblinternes/langage/erreur.hh"

#include "validation_expression_appel.hh"

struct Compilatrice;
struct Lexeme;
struct NoeudBase;
struct Type;

namespace erreur {

#define ENUMERE_TYPES_ERREUR \
	ENUMERE_TYPE_ERREUR_EX(AUCUNE_ERREUR) \
	ENUMERE_TYPE_ERREUR_EX(NORMAL) \
	ENUMERE_TYPE_ERREUR_EX(LEXAGE) \
	ENUMERE_TYPE_ERREUR_EX(SYNTAXAGE) \
	ENUMERE_TYPE_ERREUR_EX(NOMBRE_ARGUMENT) \
	ENUMERE_TYPE_ERREUR_EX(TYPE_ARGUMENT) \
	ENUMERE_TYPE_ERREUR_EX(ARGUMENT_INCONNU) \
	ENUMERE_TYPE_ERREUR_EX(ARGUMENT_REDEFINI) \
	ENUMERE_TYPE_ERREUR_EX(VARIABLE_INCONNUE) \
	ENUMERE_TYPE_ERREUR_EX(VARIABLE_REDEFINIE) \
	ENUMERE_TYPE_ERREUR_EX(FONCTION_INCONNUE) \
	ENUMERE_TYPE_ERREUR_EX(FONCTION_REDEFINIE) \
	ENUMERE_TYPE_ERREUR_EX(ASSIGNATION_RIEN) \
	ENUMERE_TYPE_ERREUR_EX(TYPE_INCONNU) \
	ENUMERE_TYPE_ERREUR_EX(TYPE_DIFFERENTS) \
	ENUMERE_TYPE_ERREUR_EX(STRUCTURE_INCONNUE) \
	ENUMERE_TYPE_ERREUR_EX(STRUCTURE_REDEFINIE) \
	ENUMERE_TYPE_ERREUR_EX(MEMBRE_INCONNU) \
	ENUMERE_TYPE_ERREUR_EX(MEMBRE_INACTIF) \
	ENUMERE_TYPE_ERREUR_EX(MEMBRE_REDEFINI) \
	ENUMERE_TYPE_ERREUR_EX(ASSIGNATION_INVALIDE) \
	ENUMERE_TYPE_ERREUR_EX(ASSIGNATION_MAUVAIS_TYPE) \
	ENUMERE_TYPE_ERREUR_EX(CONTROLE_INVALIDE) \
	ENUMERE_TYPE_ERREUR_EX(MODULE_INCONNU) \
	ENUMERE_TYPE_ERREUR_EX(APPEL_INVALIDE)

enum class type_erreur : int {
#define ENUMERE_TYPE_ERREUR_EX(type) type,
	ENUMERE_TYPES_ERREUR
#undef ENUMERE_TYPE_ERREUR_EX
};

const char *chaine_erreur(type_erreur te);
std::ostream &operator<<(std::ostream &os, type_erreur te);

using frappe = lng::erreur::frappe<type_erreur>;

void imprime_ligne_avec_message(
		dls::flux_chaine &flux,
		Fichier *fichier,
		Lexeme *lexeme,
		const char *message);

[[noreturn]] void lance_erreur(
		const dls::chaine &quoi,
		const Compilatrice &compilatrice,
		const Lexeme *morceau,
		type_erreur type = type_erreur::NORMAL);

[[noreturn]] void redefinition_fonction(
		const Compilatrice &compilatrice,
		const Lexeme *lexeme_redefinition,
		const Lexeme *lexeme_original);

[[noreturn]] void redefinition_symbole(
		const Compilatrice &compilatrice,
		const Lexeme *lexeme_redefinition,
		const Lexeme *lexeme_original);

[[noreturn]] void lance_erreur_type_arguments(
		const Type *type_arg,
		const Type *type_enf,
		const Compilatrice &compilatrice,
		const Lexeme *morceau_enfant,
		const Lexeme *morceau);

[[noreturn]] void lance_erreur_type_retour(
		const Type *type_arg,
		const Type *type_enf,
		const Compilatrice &compilatrice,
		NoeudBase *racine);

[[noreturn]] void lance_erreur_assignation_type_differents(
		const Type *type_gauche,
		const Type *type_droite,
		const Compilatrice &compilatrice,
		const Lexeme *morceau);

[[noreturn]] void lance_erreur_type_operation(
		const Type *type_gauche,
		const Type *type_droite,
		const Compilatrice &compilatrice,
		const Lexeme *morceau);

[[noreturn]] void lance_erreur_fonction_inconnue(
		Compilatrice const &compilatrice,
		NoeudBase *n,
		dls::tablet<DonneesCandidate, 10> const &candidates);

[[noreturn]] void lance_erreur_fonction_nulctx(
		Compilatrice const &compilatrice,
		NoeudBase const *appl_fonc,
		NoeudBase const *decl_fonc,
		NoeudBase const *decl_appel);

[[noreturn]] void lance_erreur_acces_hors_limites(
		Compilatrice const &compilatrice,
		NoeudBase *b,
		long taille_tableau,
		Type *type_tableau,
		long index_acces);

[[noreturn]] void lance_erreur_type_operation(
			Compilatrice const &compilatrice,
			NoeudBase *b);

[[noreturn]] void lance_erreur_type_operation_unaire(
			Compilatrice const &compilatrice,
			NoeudBase *b);

[[noreturn]] void membre_inconnu(Compilatrice &compilatrice,
		NoeudBase *acces,
		NoeudBase *structure,
		NoeudBase *membre,
		TypeCompose *type);

[[noreturn]] void membre_inactif(
			Compilatrice &compilatrice,
			ContexteValidationCode &contexte,
			NoeudBase *acces,
			NoeudBase *structure,
			NoeudBase *membre);

[[noreturn]] void valeur_manquante_discr(
			Compilatrice &compilatrice,
			NoeudBase *expression,
			dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes);

[[noreturn]] void fonction_principale_manquante();
}
