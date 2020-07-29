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

#include "biblinternes/outils/enchaineuse.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/structures/vue_chaine_compacte.hh"

struct IdentifiantCode {
	dls::vue_chaine_compacte nom{};
};

struct TableIdentifiant {
private:
	// À FAIRE : il serait bien d'utiliser un dico simple car plus rapide, ne
	// nécissitant pas de hachage, mais dico échoue lors des comparaisons de
	// vue_chaine_compacte par manque de caractère nul à la fin des chaines
	dls::dico_desordonne<dls::vue_chaine_compacte, IdentifiantCode *> table{};
	tableau_page<IdentifiantCode, 1024> identifiants{};

	Enchaineuse enchaineuse{};

public:
	TableIdentifiant();

	IdentifiantCode *identifiant_pour_chaine(dls::vue_chaine_compacte const &nom);

	IdentifiantCode *identifiant_pour_nouvelle_chaine(dls::chaine const &nom);

	long taille() const;

	size_t memoire_utilisee() const;

private:
	IdentifiantCode *ajoute_identifiant(dls::vue_chaine_compacte const &nom);
};

namespace ID {

extern IdentifiantCode *contexte;
extern IdentifiantCode *ContexteProgramme;
extern IdentifiantCode *InfoType;
extern IdentifiantCode *InfoTypeEnum;
extern IdentifiantCode *InfoTypeStructure;
extern IdentifiantCode *InfoTypeUnion;
extern IdentifiantCode *InfoTypeMembreStructure;
extern IdentifiantCode *InfoTypeEntier;
extern IdentifiantCode *InfoTypeTableau;
extern IdentifiantCode *InfoTypePointeur;
extern IdentifiantCode *InfoTypeFonction;
extern IdentifiantCode *PositionCodeSource;
extern IdentifiantCode *InfoFonctionTraceAppel;
extern IdentifiantCode *TraceAppel;
extern IdentifiantCode *BaseAllocatrice;
extern IdentifiantCode *InfoAppelTraceAppel;
extern IdentifiantCode *StockageTemporaire;
extern IdentifiantCode *panique;
extern IdentifiantCode *panique_hors_memoire;
extern IdentifiantCode *panique_depassement_limites_tableau;
extern IdentifiantCode *panique_depassement_limites_chaine;
extern IdentifiantCode *panique_membre_union;
extern IdentifiantCode *panique_erreur_non_geree;
extern IdentifiantCode *__rappel_panique_defaut;
extern IdentifiantCode *DLS_vers_r32;
extern IdentifiantCode *DLS_vers_r64;
extern IdentifiantCode *DLS_depuis_r32;
extern IdentifiantCode *DLS_depuis_r64;
extern IdentifiantCode *initialise_RC;
extern IdentifiantCode *it;
extern IdentifiantCode *index_it;
extern IdentifiantCode *principale;
extern IdentifiantCode *lance_execution;
extern IdentifiantCode *initialise_contexte;
extern IdentifiantCode *initialise_alloc;
extern IdentifiantCode *enligne;
extern IdentifiantCode *horsligne;
extern IdentifiantCode *nulctx;
extern IdentifiantCode *externe;
extern IdentifiantCode *sanstrace;
extern IdentifiantCode *interface;
extern IdentifiantCode *bibliotheque_dynamique;
extern IdentifiantCode *bibliotheque_statique;
extern IdentifiantCode *def;
extern IdentifiantCode *execute;
extern IdentifiantCode *chemin;
extern IdentifiantCode *creation_contexte;
extern IdentifiantCode *ajoute_chaine_a_la_compilation;
extern IdentifiantCode *ajoute_fichier_a_la_compilation;
extern IdentifiantCode *ajoute_chaine_au_module;
extern IdentifiantCode *compilatrice;
extern IdentifiantCode *compilatrice_obtiens_options;
extern IdentifiantCode *compilatrice_ajourne_options;
extern IdentifiantCode *compilatrice_attend_message;
extern IdentifiantCode *compilatrice_commence_interception;
extern IdentifiantCode *compilatrice_termine_interception;
extern IdentifiantCode *demarre_un_espace_de_travail;
extern IdentifiantCode *espace_defaut_compilation;
extern IdentifiantCode *fonction_test_variadique_externe;
extern IdentifiantCode *test;
extern IdentifiantCode *assert_;
extern IdentifiantCode *sansbroyage;
extern IdentifiantCode *racine;
extern IdentifiantCode *malloc_;
extern IdentifiantCode *realloc_;
extern IdentifiantCode *free_;

}

void initialise_identifiants(TableIdentifiant &table);
