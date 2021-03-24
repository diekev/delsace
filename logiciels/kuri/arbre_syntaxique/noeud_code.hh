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

#pragma once

#include "biblinternes/structures/tableau_page.hh"

#include "structures/chaine.hh"
#include "structures/tableau.hh"

#include "infos_types.hh"

/* Structures utilisées pour passer les arbres syntaxiques aux métaprogrammes.
 * Nous utilisons des structures différents que celles de l'arbre syntaxique
 * afin de pouvoir cacher certaines informations propres au compilateur, et
 * également pour se prémunir de problèmes d'interface binaire dans le future si
 * nous changeons la structure de l'arbre syntaxique, pour ne pas briser les
 * métaprogrammes existants.
 *
 * Ces structures doivent être synchronisé avec celles du module Compilatrice.
 */

struct EspaceDeTravail;
struct InfoType;
struct NoeudCodeBloc;
struct NoeudCodeDeclaration;
struct NoeudCodeCorpsFonction;
struct NoeudExpression;
struct Type;

struct NoeudCode {
	int genre = 0;
	InfoType *info_type = nullptr;

	kuri::chaine_statique chemin_fichier{};
	kuri::chaine_statique nom_fichier{};
	int numero_ligne = 0;
	int numero_colonne = 0;

	POINTEUR_NUL(NoeudCode)
};

struct NoeudCodeEnteteFonction : public NoeudCode {
	kuri::chaine_statique nom{};

	kuri::tableau<NoeudCodeDeclaration *> params_entree{};
	kuri::tableau<NoeudCodeDeclaration *> params_sortie{};

	kuri::tableau<kuri::chaine_statique> annotations{};

	bool est_operateur = false;
	bool est_coroutine = false;
};

struct NoeudCodeCorpsFonction : public NoeudCode {
	NoeudCodeEnteteFonction *entete = nullptr;
	NoeudCodeBloc *bloc = nullptr;

	kuri::tableau<NoeudCode *> arbre_aplatis{};
};

struct NoeudCodeAssignation : public NoeudCode {
	NoeudCode *assigne = nullptr;
	NoeudCode *expression = nullptr;
};

struct NoeudCodeDeclaration : public NoeudCode {
	NoeudCode *valeur = nullptr;
	NoeudCode *expression = nullptr;
};

struct NoeudCodeOperationUnaire : public NoeudCode {
	NoeudCode *operande = nullptr;

	kuri::chaine_statique op{};
};

struct NoeudCodeOperationBinaire : public NoeudCode {
	NoeudCode *operande_gauche = nullptr;
	NoeudCode *operande_droite = nullptr;

	kuri::chaine_statique op{};
};

struct NoeudCodeBloc : public NoeudCode {
	kuri::tableau<NoeudCode *> expressions{};
	kuri::tableau<NoeudCodeDeclaration *> membres{};
};

struct NoeudCodeSi : public NoeudCode {
	NoeudCode *condition = nullptr;
	NoeudCodeBloc *bloc_si_vrai = nullptr;
	NoeudCodeBloc *bloc_si_faux = nullptr;
};

struct NoeudCodeBoucle : public NoeudCode {
	NoeudCode *condition = nullptr;
	NoeudCode *bloc = nullptr;
};

struct NoeudCodePour : public NoeudCode {
	NoeudCode *variable = nullptr;
	NoeudCode *expression = nullptr;
	NoeudCode *bloc = nullptr;
	NoeudCode *bloc_sansarret = nullptr;
	NoeudCode *bloc_sinon = nullptr;
};

struct NoeudCodeTente : public NoeudCode {
	NoeudCode *expression_appel = nullptr;
	NoeudCode *expression_piege = nullptr;
	NoeudCode *bloc = nullptr;
};

struct NoeudCodeDiscr : public NoeudCode {
	struct PaireDiscr {
		NoeudCode *expression = nullptr;
		NoeudCode *bloc = nullptr;
	};

	NoeudCode *expression = nullptr;
	NoeudCode *bloc_sinon = nullptr;
	kuri::tableau<PaireDiscr> paires_discr{};
};

struct NoeudCodePousseContexte : public NoeudCode {
	NoeudCode *expression = nullptr;
	NoeudCode *bloc = nullptr;
};

struct NoeudCodeReferenceMembre : public NoeudCode {
	NoeudCode *accede = nullptr;
	NoeudCode *membre = nullptr;
};

struct NoeudCodeAppel : public NoeudCode {
	NoeudCode *expression = nullptr;
	kuri::tableau<NoeudCode *> params{};
};

struct NoeudCodeVirgule : public NoeudCode {
	kuri::tableau<NoeudCode *> expressions{};
};

struct NoeudCodeDirective : public NoeudCode {
	kuri::chaine_statique ident{};
	NoeudCode *expression = nullptr;
};

struct NoeudCodeIdentifiant : public NoeudCode {
	kuri::chaine_statique ident{};
};

struct NoeudCodeLitteraleEntier : public NoeudCode {
	uint64_t valeur{};
};

struct NoeudCodeLitteraleReel : public NoeudCode {
	double valeur{};
};

struct NoeudCodeLitteraleCaractere : public NoeudCode {
	char valeur{};
};

struct NoeudCodeLitteraleChaine : public NoeudCode {
	kuri::chaine_statique valeur{};
};

struct NoeudCodeLitteraleBooleen : public NoeudCode {
	bool valeur{};
};

struct ConvertisseuseNoeudCode {
	tableau_page<NoeudCode> noeuds_codes{};
	tableau_page<NoeudCodeEnteteFonction> noeuds_entetes_fonctions{};
	tableau_page<NoeudCodeCorpsFonction> noeuds_corps_fonctions{};
	tableau_page<NoeudCodeAssignation> noeuds_assignations{};
	tableau_page<NoeudCodeDeclaration> noeuds_declarations{};
	tableau_page<NoeudCodeOperationUnaire> noeuds_operations_unaire{};
	tableau_page<NoeudCodeOperationBinaire> noeuds_operations_binaire{};
	tableau_page<NoeudCodeBloc> noeuds_blocs{};
	tableau_page<NoeudCodeSi> noeuds_sis{};
	tableau_page<NoeudCodeBoucle> noeuds_boucles{};
	tableau_page<NoeudCodePour> noeuds_pour{};
	tableau_page<NoeudCodeTente> noeuds_tente{};
	tableau_page<NoeudCodeDiscr> noeuds_discr{};
	tableau_page<NoeudCodePousseContexte> noeuds_pousse_contexte{};
	tableau_page<NoeudCodeReferenceMembre> noeuds_reference_membre{};
	tableau_page<NoeudCodeAppel> noeuds_appel{};
	tableau_page<NoeudCodeVirgule> noeuds_virgule{};
	tableau_page<NoeudCodeDirective> noeuds_directive{};
	tableau_page<NoeudCodeIdentifiant> noeuds_ident{};
	tableau_page<NoeudCodeLitteraleReel> noeuds_litterale_reel{};
	tableau_page<NoeudCodeLitteraleEntier> noeuds_litterale_entier{};
	tableau_page<NoeudCodeLitteraleChaine> noeuds_litterale_chaine{};
	tableau_page<NoeudCodeLitteraleCaractere> noeuds_litterale_caractere{};
	tableau_page<NoeudCodeLitteraleBooleen> noeuds_litterale_booleen{};

	AllocatriceInfosType allocatrice_infos_types{};

	NoeudCode *convertis_noeud_syntaxique(EspaceDeTravail *espace, NoeudExpression *noeud_expression);

	InfoType *cree_info_type_pour(Type *type);

	long memoire_utilisee() const;
};
