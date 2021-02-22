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

#include "identifiant.hh"

#include "biblinternes/outils/definitions.h"

TableIdentifiant::TableIdentifiant()
{
	initialise_identifiants(*this);
}

IdentifiantCode *TableIdentifiant::identifiant_pour_chaine(const dls::vue_chaine_compacte &nom)
{
	auto trouve = false;
	auto iter = table.trouve(nom, trouve);

	if (trouve) {
		return iter;
	}

	return ajoute_identifiant(nom);
}

IdentifiantCode *TableIdentifiant::identifiant_pour_nouvelle_chaine(kuri::chaine const &nom)
{
	auto trouve = false;
	auto iter = table.trouve(nom, trouve);

	if (trouve) {
		return iter;
	}

	auto tampon_courant = enchaineuse.tampon_courant;

	if (tampon_courant->occupe + nom.taille() > Enchaineuse::TAILLE_TAMPON) {
		enchaineuse.ajoute_tampon();
		tampon_courant = enchaineuse.tampon_courant;
	}

	auto ptr = &tampon_courant->donnees[tampon_courant->occupe];
	enchaineuse.ajoute(nom);

	auto vue_nom = dls::vue_chaine_compacte(ptr, nom.taille());
	return ajoute_identifiant(vue_nom);
}

long TableIdentifiant::taille() const
{
	return table.taille();
}

long TableIdentifiant::memoire_utilisee() const
{
	auto memoire = 0l;
	memoire += identifiants.memoire_utilisee();
	memoire += table.taille() * (taille_de(dls::vue_chaine_compacte) + taille_de(IdentifiantCode *));
	memoire += enchaineuse.nombre_tampons_alloues() * Enchaineuse::TAILLE_TAMPON;
	return memoire;
}

IdentifiantCode *TableIdentifiant::ajoute_identifiant(const dls::vue_chaine_compacte &nom)
{
	auto ident = identifiants.ajoute_element();
	ident->nom = { &nom[0], nom.taille() };

	table.insere(nom, ident);

	return ident;
}

/* ************************************************************************** */

namespace ID {

IdentifiantCode *chaine_vide;
IdentifiantCode *Kuri;
IdentifiantCode *contexte;
IdentifiantCode *ContexteProgramme;
IdentifiantCode *InfoType;
IdentifiantCode *InfoTypeEnum;
IdentifiantCode *InfoTypeStructure;
IdentifiantCode *InfoTypeUnion;
IdentifiantCode *InfoTypeMembreStructure;
IdentifiantCode *InfoTypeEntier;
IdentifiantCode *InfoTypeTableau;
IdentifiantCode *InfoTypePointeur;
IdentifiantCode *InfoTypeFonction;
IdentifiantCode *InfoTypeOpaque;
IdentifiantCode *PositionCodeSource;
IdentifiantCode *InfoFonctionTraceAppel;
IdentifiantCode *TraceAppel;
IdentifiantCode *BaseAllocatrice;
IdentifiantCode *InfoAppelTraceAppel;
IdentifiantCode *StockageTemporaire;
IdentifiantCode *panique;
IdentifiantCode *panique_hors_memoire;
IdentifiantCode *panique_depassement_limites_tableau;
IdentifiantCode *panique_depassement_limites_chaine;
IdentifiantCode *panique_membre_union;
IdentifiantCode *panique_erreur_non_geree;
IdentifiantCode *__rappel_panique_defaut;
IdentifiantCode *DLS_vers_r32;
IdentifiantCode *DLS_vers_r64;
IdentifiantCode *DLS_depuis_r32;
IdentifiantCode *DLS_depuis_r64;
IdentifiantCode *initialise_RC;
IdentifiantCode *it;
IdentifiantCode *index_it;
IdentifiantCode *principale;
IdentifiantCode *lance_execution;
IdentifiantCode *initialise_contexte;
IdentifiantCode *initialise_alloc;
IdentifiantCode *enligne;
IdentifiantCode *horsligne;
IdentifiantCode *nulctx;
IdentifiantCode *externe;
IdentifiantCode *sanstrace;
IdentifiantCode *interface;
IdentifiantCode *bibliotheque_dynamique;
IdentifiantCode *bibliotheque_statique;
IdentifiantCode *def;
IdentifiantCode *execute;
IdentifiantCode *chemin;
IdentifiantCode *creation_contexte;
IdentifiantCode *ajoute_chaine_a_la_compilation;
IdentifiantCode *ajoute_fichier_a_la_compilation;
IdentifiantCode *ajoute_chaine_au_module;
IdentifiantCode *compilatrice;
IdentifiantCode *compilatrice_obtiens_options;
IdentifiantCode *compilatrice_ajourne_options;
IdentifiantCode *compilatrice_attend_message;
IdentifiantCode *compilatrice_commence_interception;
IdentifiantCode *compilatrice_termine_interception;
IdentifiantCode *compilatrice_rapporte_erreur;
IdentifiantCode *compilatrice_lexe_fichier;
IdentifiantCode *compilatrice_espace_courant;
IdentifiantCode *demarre_un_espace_de_travail;
IdentifiantCode *fonction_test_variadique_externe;
IdentifiantCode *test;
IdentifiantCode *assert_;
IdentifiantCode *sansbroyage;
IdentifiantCode *racine;
IdentifiantCode *espace_defaut_compilation;
IdentifiantCode *malloc_;
IdentifiantCode *realloc_;
IdentifiantCode *free_;
IdentifiantCode *si;
IdentifiantCode *pointeur;
IdentifiantCode *corps_texte;
IdentifiantCode *cuisine;
IdentifiantCode *opaque;
IdentifiantCode *__point_d_entree_systeme;
IdentifiantCode *taille;
IdentifiantCode *capacite;
IdentifiantCode *anonyme;
IdentifiantCode *valeur;
IdentifiantCode *membre_actif;
IdentifiantCode *info;
IdentifiantCode *_0;
IdentifiantCode *_1;
IdentifiantCode *nombre_elements;
IdentifiantCode *min;
IdentifiantCode *max;
IdentifiantCode *valeurs_legales;
IdentifiantCode *valeurs_illegales;

}

void initialise_identifiants(TableIdentifiant &table)
{
	ID::chaine_vide = table.identifiant_pour_chaine("");
	ID::Kuri = table.identifiant_pour_chaine("Kuri");
	ID::contexte = table.identifiant_pour_chaine("contexte");
	ID::ContexteProgramme = table.identifiant_pour_chaine("ContexteProgramme");
	ID::InfoType = table.identifiant_pour_chaine("InfoType");
	ID::InfoTypeEnum = table.identifiant_pour_chaine("InfoTypeÉnum");
	ID::InfoTypeStructure = table.identifiant_pour_chaine("InfoTypeStructure");
	ID::InfoTypeUnion = table.identifiant_pour_chaine("InfoTypeUnion");
	ID::InfoTypeMembreStructure = table.identifiant_pour_chaine("InfoTypeMembreStructure");
	ID::InfoTypeEntier = table.identifiant_pour_chaine("InfoTypeEntier");
	ID::InfoTypeTableau = table.identifiant_pour_chaine("InfoTypeTableau");
	ID::InfoTypePointeur = table.identifiant_pour_chaine("InfoTypePointeur");
	ID::InfoTypeFonction = table.identifiant_pour_chaine("InfoTypeFonction");
	ID::InfoTypeOpaque = table.identifiant_pour_chaine("InfoTypeOpaque");
	ID::PositionCodeSource = table.identifiant_pour_chaine("PositionCodeSource");
	ID::InfoFonctionTraceAppel = table.identifiant_pour_chaine("InfoFonctionTraceAppel");
	ID::TraceAppel = table.identifiant_pour_chaine("TraceAppel");
	ID::BaseAllocatrice = table.identifiant_pour_chaine("BaseAllocatrice");
	ID::InfoAppelTraceAppel = table.identifiant_pour_chaine("InfoAppelTraceAppel");
	ID::StockageTemporaire = table.identifiant_pour_chaine("StockageTemporaire");
	ID::panique = table.identifiant_pour_chaine("panique");
	ID::panique_hors_memoire = table.identifiant_pour_chaine("panique_hors_mémoire");
	ID::panique_depassement_limites_tableau = table.identifiant_pour_chaine("panique_dépassement_limites_tableau");
	ID::panique_depassement_limites_chaine = table.identifiant_pour_chaine("panique_dépassement_limites_chaine");
	ID::panique_membre_union = table.identifiant_pour_chaine("panique_membre_union");
	ID::panique_erreur_non_geree = table.identifiant_pour_chaine("panique_erreur_non_gérée");
	ID::__rappel_panique_defaut = table.identifiant_pour_chaine("__rappel_panique_défaut");
	ID::DLS_vers_r32 = table.identifiant_pour_chaine("DLS_vers_r32");
	ID::DLS_vers_r64 = table.identifiant_pour_chaine("DLS_vers_r64");
	ID::DLS_depuis_r32 = table.identifiant_pour_chaine("DLS_depuis_r32");
	ID::DLS_depuis_r64 = table.identifiant_pour_chaine("DLS_depuis_r64");
	ID::initialise_RC = table.identifiant_pour_chaine("initialise_RC");
	ID::it = table.identifiant_pour_chaine("it");
	ID::index_it = table.identifiant_pour_chaine("index_it");
	ID::principale = table.identifiant_pour_chaine("principale");
	ID::lance_execution = table.identifiant_pour_chaine("lance_execution");
	ID::initialise_contexte = table.identifiant_pour_chaine("initialise_contexte");
	ID::initialise_alloc = table.identifiant_pour_chaine("initialise_alloc");
	ID::enligne = table.identifiant_pour_chaine("enligne");
	ID::horsligne = table.identifiant_pour_chaine("horsligne");
	ID::nulctx = table.identifiant_pour_chaine("nulctx");
	ID::externe = table.identifiant_pour_chaine("externe");
	ID::sanstrace = table.identifiant_pour_chaine("sanstrace");
	ID::interface = table.identifiant_pour_chaine("interface");
	ID::bibliotheque_dynamique = table.identifiant_pour_chaine("bibliothèque_dynamique");
	ID::bibliotheque_statique = table.identifiant_pour_chaine("bibliothèque_statique");
	ID::def = table.identifiant_pour_chaine("def");
	ID::execute = table.identifiant_pour_chaine("exécute");
	ID::chemin = table.identifiant_pour_chaine("chemin");
	ID::creation_contexte = table.identifiant_pour_chaine("création_contexte");
	ID::ajoute_chaine_a_la_compilation = table.identifiant_pour_chaine("ajoute_chaine_à_la_compilation");
	ID::ajoute_fichier_a_la_compilation = table.identifiant_pour_chaine("ajoute_fichier_à_la_compilation");
	ID::ajoute_chaine_au_module = table.identifiant_pour_chaine("ajoute_chaine_au_module");
	ID::compilatrice = table.identifiant_pour_chaine("compilatrice");
	ID::compilatrice_obtiens_options = table.identifiant_pour_chaine("compilatrice_obtiens_options");
	ID::compilatrice_ajourne_options = table.identifiant_pour_chaine("compilatrice_ajourne_options");
	ID::compilatrice_attend_message = table.identifiant_pour_chaine("compilatrice_attend_message");
	ID::compilatrice_commence_interception = table.identifiant_pour_chaine("compilatrice_commence_interception");
	ID::compilatrice_termine_interception = table.identifiant_pour_chaine("compilatrice_termine_interception");
	ID::compilatrice_rapporte_erreur = table.identifiant_pour_chaine("compilatrice_rapporte_erreur");
	ID::compilatrice_lexe_fichier = table.identifiant_pour_chaine("compilatrice_lèxe_fichier");
	ID::compilatrice_espace_courant = table.identifiant_pour_chaine("compilatrice_espace_courant");
	ID::demarre_un_espace_de_travail = table.identifiant_pour_chaine("démarre_un_espace_de_travail");
	ID::espace_defaut_compilation = table.identifiant_pour_chaine("espace_défaut_compilation");
	ID::fonction_test_variadique_externe = table.identifiant_pour_chaine("fonction_test_variadique_externe");
	ID::test = table.identifiant_pour_chaine("test");
	ID::assert_ = table.identifiant_pour_chaine("assert");
	ID::sansbroyage = table.identifiant_pour_chaine("sansbroyage");
	ID::racine = table.identifiant_pour_chaine("racine");
	ID::malloc_ = table.identifiant_pour_chaine("malloc");
	ID::realloc_ = table.identifiant_pour_chaine("realloc");
	ID::free_ = table.identifiant_pour_chaine("free");
	ID::si = table.identifiant_pour_chaine("si");
	ID::pointeur = table.identifiant_pour_chaine("pointeur");
	ID::corps_texte = table.identifiant_pour_chaine("corps_texte");
	ID::cuisine = table.identifiant_pour_chaine("cuisine");
	ID::opaque = table.identifiant_pour_chaine("opaque");
	ID::__point_d_entree_systeme = table.identifiant_pour_chaine("__point_d_entree_systeme");
	ID::taille = table.identifiant_pour_chaine("taille");
	ID::capacite = table.identifiant_pour_chaine("capacité");
	ID::anonyme = table.identifiant_pour_chaine("anonyme");
	ID::valeur = table.identifiant_pour_chaine("valeur");
	ID::membre_actif = table.identifiant_pour_chaine("membre_actif");
	ID::info = table.identifiant_pour_chaine("info");
	ID::_0 = table.identifiant_pour_chaine("0");
	ID::_1 = table.identifiant_pour_chaine("1");
	ID::nombre_elements = table.identifiant_pour_chaine("nombre_éléments");
	ID::min = table.identifiant_pour_chaine("min");
	ID::max = table.identifiant_pour_chaine("max");
	ID::valeurs_legales = table.identifiant_pour_chaine("valeurs_légales");
	ID::valeurs_illegales = table.identifiant_pour_chaine("valeurs_illégales");
}
