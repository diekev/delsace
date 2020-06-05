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

enum class type_erreur : int {
	AUCUNE_ERREUR,

	NORMAL,
	LEXAGE,
	SYNTAXAGE,
	NOMBRE_ARGUMENT,
	TYPE_ARGUMENT,
	ARGUMENT_INCONNU,
	ARGUMENT_REDEFINI,
	VARIABLE_INCONNUE,
	VARIABLE_REDEFINIE,
	FONCTION_INCONNUE,
	FONCTION_REDEFINIE,
	ASSIGNATION_RIEN,
	TYPE_INCONNU,
	TYPE_DIFFERENTS,
	STRUCTURE_INCONNUE,
	STRUCTURE_REDEFINIE,
	MEMBRE_INCONNU,
	MEMBRE_INACTIF,
	MEMBRE_REDEFINI,
	ASSIGNATION_INVALIDE,
	ASSIGNATION_MAUVAIS_TYPE,
	CONTROLE_INVALIDE,
	MODULE_INCONNU,
	APPEL_INVALIDE,
};

const char *chaine_erreur(type_erreur te);

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
