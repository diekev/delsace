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

struct DonneesLexeme;
struct DonneesStructure;
struct ContexteGenerationCode;
struct Type;

namespace erreur {

enum class type_erreur : int {
	NORMAL,
	DECOUPAGE,
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

	AUCUNE_ERREUR,
};

using frappe = lng::erreur::frappe<type_erreur>;

[[noreturn]] void lance_erreur(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme &morceau,
		type_erreur type = type_erreur::NORMAL);

[[noreturn]] void lance_erreur_plage(
		const dls::chaine &quoi,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme &premier_morceau,
		const DonneesLexeme &dernier_morceau,
		type_erreur type = type_erreur::NORMAL);

[[noreturn]] void lance_erreur_type_arguments(
		const Type *type_arg,
		const Type *type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme &morceau_enfant,
		const DonneesLexeme &morceau);

[[noreturn]] void lance_erreur_type_retour(
		const Type *type_arg,
		const Type *type_enf,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme &morceau_enfant,
		const DonneesLexeme &morceau);

[[noreturn]] void lance_erreur_assignation_type_differents(
		const Type *type_gauche,
		const Type *type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme &morceau);

[[noreturn]] void lance_erreur_type_operation(
		const Type *type_gauche,
		const Type *type_droite,
		const ContexteGenerationCode &contexte,
		const DonneesLexeme &morceau);

[[noreturn]] void lance_erreur_fonction_inconnue(
		ContexteGenerationCode const &contexte,
		noeud::base *n,
		dls::tablet<DonneesCandidate, 10> const &candidates);

[[noreturn]] void lance_erreur_fonction_nulctx(
		ContexteGenerationCode const &contexte,
		noeud::base *appl_fonc,
		noeud::base *decl_fonc,
		noeud::base *decl_appel);

[[noreturn]] void lance_erreur_acces_hors_limites(
			ContexteGenerationCode const &contexte,
			noeud::base *b,
			long taille_tableau,
			Type const *type_tableau,
			long index_acces);

[[noreturn]] void lance_erreur_type_operation(
			ContexteGenerationCode const &contexte,
			noeud::base *b);

[[noreturn]] void lance_erreur_type_operation_unaire(
			ContexteGenerationCode const &contexte,
			noeud::base *b);

[[noreturn]] void membre_inconnu(
		ContexteGenerationCode &contexte,
		DonneesStructure &ds,
		noeud::base *acces,
		noeud::base *structure,
		noeud::base *membre);

[[noreturn]] void membre_inconnu_tableau(
			ContexteGenerationCode &contexte,
			noeud::base *acces,
			noeud::base *structure,
			noeud::base *membre);

[[noreturn]] void membre_inconnu_chaine(
			ContexteGenerationCode &contexte,
			noeud::base *acces,
			noeud::base *structure,
			noeud::base *membre);

[[noreturn]] void membre_inconnu_eini(
			ContexteGenerationCode &contexte,
			noeud::base *acces,
			noeud::base *structure,
			noeud::base *membre);

[[noreturn]] void membre_inactif(
			ContexteGenerationCode &contexte,
			noeud::base *acces,
			noeud::base *structure,
			noeud::base *membre);

[[noreturn]] void valeur_manquante_discr(
			ContexteGenerationCode &contexte,
			noeud::base *expression,
			dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes);

[[noreturn]] void fonction_principale_manquante();
}
