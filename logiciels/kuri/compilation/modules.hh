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

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/structures/ensemble.hh"
#include "biblinternes/structures/tablet.hh"

#include "transformation_type.hh"
#include "typage.hh"
#include "structures.hh"

class assembleuse_arbre;

struct ContexteGenerationCode;
struct DonneesModule;
struct IdentifiantCode;
struct NoeudBase;
struct NoeudBloc;
struct NoeudDeclarationFonction;
struct NoeudExpression;

struct Fichier {
	double temps_analyse = 0.0;
	double temps_chargement = 0.0;
	double temps_decoupage = 0.0;
	double temps_tampon = 0.0;

	lng::tampon_source tampon{""};

	dls::tableau<DonneesLexeme> lexemes{};

	dls::ensemble<dls::vue_chaine_compacte> modules_importes{};

	DonneesModule *module = nullptr;

	size_t id = 0ul;
	dls::chaine nom{""};
	dls::chaine chemin{""};

	Fichier();

	COPIE_CONSTRUCT(Fichier);

	/**
	 * Retourne vrai si le fichier importe un module du nom spécifié.
	 */
	bool importe_module(dls::vue_chaine_compacte const &nom_module) const;
};

struct DonneesModule {
	/* utilisation d'un pointeur à cause de dépendances cycliques entre les entêtes */
	assembleuse_arbre *assembleuse{};
	NoeudBloc *bloc = nullptr;

	dls::tableau<Fichier *> fichiers{};
	dls::ensemble<dls::vue_chaine_compacte> fonctions_exportees{};
	size_t id = 0ul;
	dls::chaine nom{""};
	dls::chaine chemin{""};
	bool importe = false;

	DonneesModule(ContexteGenerationCode &contexte);

	~DonneesModule();

	COPIE_CONSTRUCT(DonneesModule);
};

dls::chaine charge_fichier(
		dls::chaine const &chemin,
		ContexteGenerationCode &contexte,
		DonneesLexeme const &lexeme);

void charge_fichier(std::ostream &os,
		DonneesModule *module,
		dls::chaine const &racine_kuri,
		dls::chaine const &nom,
		ContexteGenerationCode &contexte,
		DonneesLexeme const &lexeme);

/**
 * Charge le module dont le nom est spécifié.
 *
 * Le nom doit être celui d'un fichier s'appelant '<nom>.kuri' et se trouvant
 * dans le dossier du module racine.
 *
 * Les fonctions contenues dans le module auront leurs noms préfixés par le nom
 * du module, sauf pour le module racine.
 *
 * Le std::ostream est un flux de sortie où sera imprimé le nom du module ouvert
 * pour tenir compte de la progression de la compilation. Si un nom de module ne
 * pointe pas vers un fichier Kuri, ou si le fichier ne peut être ouvert, une
 * exception est lancée.
 *
 * Les DonneesLexeme doivent être celles du nom du module et sont utilisées
 * pour les erreurs lancées.
 *
 * Le paramètre est_racine ne doit être vrai que pour le module racine.
 */
void importe_module(
		std::ostream &os,
		dls::chaine const &racine_kuri,
		dls::chaine const &nom,
		ContexteGenerationCode &contexte,
		DonneesLexeme const &lexeme);

/* ************************************************************************** */

enum {
	FONCTION_TROUVEE,
	FONCTION_INTROUVEE,
};

enum {
	AUCUNE_RAISON,
	METYPAGE_ARG,
	MENOMMAGE_ARG,
	RENOMMAGE_ARG,
	MECOMPTAGE_ARGS,
	MANQUE_NOM_APRES_VARIADIC,
};

struct DonneesCandidate {
	int etat = FONCTION_INTROUVEE;
	int raison = AUCUNE_RAISON;
	double poids_args = 0.0;
	dls::vue_chaine_compacte nom_arg{};
	/* les expressions remises dans l'ordre selon les noms, si la fonction est trouvée. */
	dls::tablet<NoeudExpression *, 10> exprs{};
	Type *type1{};
	Type *type2{};
	NoeudBase *noeud_decl = nullptr;
	NoeudDeclarationFonction *decl_fonc = nullptr;
	bool arg_pointeur = false;
	dls::tableau<TransformationType> transformations{};
	dls::tableau<std::pair<dls::vue_chaine_compacte, Type *>> paires_expansion_gabarit{};
};

struct ResultatRecherche {
	dls::tablet<DonneesCandidate, 10> candidates{};
};

ResultatRecherche cherche_donnees_fonction(
		ContexteGenerationCode &contexte,
		IdentifiantCode *nom,
		kuri::tableau<IdentifiantCode *> &noms_arguments,
		kuri::tableau<NoeudExpression *> const &exprs,
		size_t index_fichier,
		size_t index_fichier_appel);


struct PositionLexeme {
	long index_ligne = 0;
	long numero_ligne = 0;
	long pos = 0;
};

PositionLexeme position_lexeme(DonneesLexeme const &lexeme);

/* ************************************************************************** */

NoeudDeclarationFonction *cherche_fonction_dans_module(
		ContexteGenerationCode &contexte,
		dls::vue_chaine_compacte const &nom_module,
		dls::vue_chaine_compacte const &nom_fonction);
