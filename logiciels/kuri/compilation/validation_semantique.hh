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

#include "biblinternes/outils/definitions.h"

#include "graphe_dependance.hh"
#include "structures.hh"
#include "validation_expression_appel.hh"

struct Compilatrice;
struct Lexeme;
struct MetaProgramme;
struct NoeudAssignation;
struct NoeudBloc;
struct NoeudDeclarationCorpsFonction;
struct NoeudDeclarationEnteteFonction;
struct NoeudDirectiveExecution;
struct NoeudEnum;
struct NoeudExpression;
struct NoeudExpressionMembre;
struct NoeudExpressionUnaire;
struct NoeudRetour;
struct NoeudStruct;
struct Tacheronne;
struct TypeCompose;
struct TypeEnum;
struct TypeTableauFixe;
struct UniteCompilation;

namespace erreur {
enum class Genre : int;
}

struct ContexteValidationCode {
	Compilatrice &m_compilatrice;
	Tacheronne &m_tacheronne;
	NoeudDeclarationEnteteFonction *fonction_courante = nullptr;

	/* Les données des dépendances d'un noeud syntaxique. */
	DonneesDependance donnees_dependance{};

	UniteCompilation *unite = nullptr;
	EspaceDeTravail *espace = nullptr;

	using paire_union_membre = std::pair<dls::vue_chaine_compacte, dls::vue_chaine_compacte>;
	dls::tableau<paire_union_membre> membres_actifs{};

	double temps_chargement = 0.0;

	ContexteValidationCode(Compilatrice &compilatrice, Tacheronne &tacheronne, UniteCompilation &unite);

	COPIE_CONSTRUCT(ContexteValidationCode);

	void commence_fonction(NoeudDeclarationEnteteFonction *fonction);

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

	bool valide_type_fonction(NoeudDeclarationEnteteFonction *);
	bool valide_fonction(NoeudDeclarationCorpsFonction *);
	bool valide_operateur(NoeudDeclarationCorpsFonction *);

	template<int N>
	bool valide_enum_impl(NoeudEnum *decl, TypeEnum *type_enum);
	bool valide_enum(NoeudEnum *);

	bool valide_structure(NoeudStruct *);
	bool valide_declaration_variable(NoeudDeclarationVariable *decl);
	bool valide_assignation(NoeudAssignation *inst);
	bool valide_arbre_aplatis(NoeudExpression *declaration, kuri::tableau<NoeudExpression *> &arbre_aplatis);
	bool valide_expression_retour(NoeudRetour *inst_retour);
	bool valide_cuisine(NoeudExpressionUnaire *directive);
	bool resoud_type_final(NoeudExpression *expression_type, Type *&type_final);

	void rapporte_erreur(const char *message, NoeudExpression *noeud);
	void rapporte_erreur(const char *message, NoeudExpression *noeud, erreur::Genre genre);
	void rapporte_erreur_redefinition_symbole(NoeudExpression *decl, NoeudDeclaration *decl_prec);
	void rapporte_erreur_redefinition_fonction(NoeudDeclarationEnteteFonction *decl, NoeudDeclaration *decl_prec);
	void rapporte_erreur_type_arguments(NoeudExpression *type_arg, NoeudExpression *type_enf);
	void rapporte_erreur_assignation_type_differents(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud);
	void rapporte_erreur_type_operation(const Type *type_gauche, const Type *type_droite, NoeudExpression *noeud);
	void rapporte_erreur_acces_hors_limites(NoeudExpression *b, TypeTableauFixe *type_tableau, long index_acces);
	void rapporte_erreur_membre_inconnu(NoeudExpression *acces, NoeudExpression *structure, NoeudExpression *membre, TypeCompose *type);
	void rapporte_erreur_membre_inactif(NoeudExpression *acces, NoeudExpression *structure, NoeudExpression *membre);
	void rapporte_erreur_valeur_manquante_discr(NoeudExpression *expression, dls::ensemble<dls::vue_chaine_compacte> const &valeurs_manquantes);
	void rapporte_erreur_fonction_inconnue(NoeudExpression *b, dls::tablet<DonneesCandidate, 10> const &candidates);
	void rapporte_erreur_fonction_nulctx(NoeudExpression const *appl_fonc, NoeudExpression const *decl_fonc, NoeudExpression const *decl_appel);

	bool transtype_si_necessaire(NoeudExpression *&expression, Type *type_cible);
	bool transtype_si_necessaire(NoeudExpression *&expression, TransformationType const &transformation);

	MetaProgramme *cree_metaprogramme_corps_texte(NoeudBloc *bloc_corps_texte, NoeudBloc *bloc_parent, const Lexeme *lexeme);
};
