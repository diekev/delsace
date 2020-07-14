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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "graphe_dependance.hh"

#include "validation_expression_appel.hh"

struct Compilatrice;
struct NoeudDeclarationFonction;
struct NoeudExpression;

namespace erreur {
enum class type_erreur : int;
}

struct ContexteValidationCode {
	Compilatrice &m_compilatrice;
	NoeudDeclarationFonction *fonction_courante = nullptr;

	/* Les données des dépendances d'un noeud syntaxique. */
	DonneesDependance donnees_dependance{};

	UniteCompilation *unite = nullptr;

	using paire_union_membre = std::pair<dls::vue_chaine_compacte, dls::vue_chaine_compacte>;
	dls::tableau<paire_union_membre> membres_actifs{};

	ContexteValidationCode(Compilatrice &compilatrice);

	COPIE_CONSTRUCT(ContexteValidationCode);

	void commence_fonction(NoeudDeclarationFonction *fonction);

	void termine_fonction();

	/* gestion des membres actifs des unions :
	 * cas à considérer :
	 * -- les portées des variables
	 * -- les unions dans les structures (accès par '.')
	 */
	dls::vue_chaine_compacte trouve_membre_actif(dls::vue_chaine_compacte const &nom_union);

	void renseigne_membre_actif(dls::vue_chaine_compacte const &nom_union, dls::vue_chaine_compacte const &nom_membre);

	bool valide_semantique_noeud(NoeudExpression *);
	bool valide_acces_membre(NoeudExpressionMembre *expression_membre);

	bool valide_type_fonction(NoeudDeclarationFonction *);
	bool valide_fonction(NoeudDeclarationFonction *);
	bool valide_operateur(NoeudDeclarationFonction *);
	bool valide_enum(NoeudEnum *);
	bool valide_structure(NoeudStruct *);
	bool resoud_type_final(NoeudExpression *expression_type, Type *&type_final);

	void rapporte_erreur(const char *message, NoeudExpression *noeud);
	void rapporte_erreur(const char *message, NoeudExpression *noeud, erreur::type_erreur type_erreur);
	void rapporte_erreur_redefinition_symbole(NoeudExpression *decl, NoeudDeclaration *decl_prec);
	void rapporte_erreur_redefinition_fonction(NoeudDeclarationFonction *decl, NoeudDeclaration *decl_prec);
	void rapporte_erreur_type_arguments(NoeudExpression *type_arg, NoeudExpression *type_enf);
	void rapporte_erreur_type_retour(const Type *type_arg, const Type *type_enf, NoeudExpression *racine);
	void rapporte_erreur_assignation_type_differents(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud);
	void rapporte_erreur_type_operation(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud);
	void rapporte_erreur_type_operation(NoeudExpression *noeud);
	void rapporte_erreur_type_indexage(NoeudExpression *noeud);
	void rapporte_erreur_type_operation_unaire(NoeudExpression *noeud);
	void rapporte_erreur_acces_hors_limites(NoeudExpression *b, TypeTableauFixe *type_tableau, long index_acces);
	void rapporte_erreur_membre_inconnu(NoeudExpression *acces, NoeudExpression *structure, NoeudExpression *membre, TypeCompose *type);
	void rapporte_erreur_membre_inactif(NoeudExpression *acces, NoeudExpression *structure, NoeudExpression *membre);
	void rapporte_erreur_valeur_manquante_discr(NoeudExpression *expression, dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes);
	void rapporte_erreur_fonction_inconnue(NoeudExpression *b, dls::tablet<DonneesCandidate, 10> const &candidates);
	void rapporte_erreur_fonction_nulctx(NoeudExpression const *appl_fonc, NoeudExpression const *decl_fonc, NoeudExpression const *decl_appel);
};
