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

IdentifiantCode *TableIdentifiant::identifiant_pour_chaine(const dls::vue_chaine_compacte &nom)
{
	auto iter = table.trouve(nom);

	if (iter != table.fin()) {
		return iter->second;
	}

	return ajoute_identifiant(nom);
}

IdentifiantCode *TableIdentifiant::identifiant_pour_nouvelle_chaine(dls::chaine const &nom)
{
	auto iter = table.trouve(nom);

	if (iter != table.fin()) {
		return iter->second;
	}

	auto tampon_courant = enchaineuse.tampon_courant;

	if (tampon_courant->occupe + nom.taille() > Enchaineuse::TAILLE_TAMPON) {
		enchaineuse.ajoute_tampon();
		tampon_courant = enchaineuse.tampon_courant;
	}

	auto ptr = &tampon_courant->donnees[tampon_courant->occupe];
	enchaineuse.pousse(nom);

	auto vue_nom = dls::vue_chaine_compacte(ptr, nom.taille());
	return ajoute_identifiant(vue_nom);
}

long TableIdentifiant::taille() const
{
	return table.taille();
}

size_t TableIdentifiant::memoire_utilisee() const
{
	auto memoire = 0ul;
	memoire += identifiants.memoire_utilisee();
	memoire += static_cast<size_t>(table.taille()) * (sizeof (dls::vue_chaine_compacte) + sizeof(IdentifiantCode *));
	return memoire;
}

IdentifiantCode *TableIdentifiant::ajoute_identifiant(const dls::vue_chaine_compacte &nom)
{
	auto ident = identifiants.ajoute_element();
	ident->nom = nom;

	table.insere({ nom, ident });

	return ident;
}

/* ************************************************************************** */

namespace ID {

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
IdentifiantCode *inclus;
IdentifiantCode *bibliotheque_dynamique;
IdentifiantCode *bibliotheque_statique;
IdentifiantCode *def;
IdentifiantCode *execute;
IdentifiantCode *chemin;
IdentifiantCode *creation_contexte;
IdentifiantCode *ajoute_chaine_a_la_compilation;
IdentifiantCode *ajoute_fichier_a_la_compilation;
IdentifiantCode *compilatrice;
IdentifiantCode *compilatrice_obtiens_options;
IdentifiantCode *compilatrice_ajourne_options;
IdentifiantCode *fonction_test_variadique_externe;
IdentifiantCode *test;
IdentifiantCode *assert_;

}

void initialise_identifiants(TableIdentifiant &table)
{
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
	ID::inclus = table.identifiant_pour_chaine("inclus");
	ID::bibliotheque_dynamique = table.identifiant_pour_chaine("bibliothèque_dynamique");
	ID::bibliotheque_statique = table.identifiant_pour_chaine("bibliothèque_statique");
	ID::def = table.identifiant_pour_chaine("def");
	ID::execute = table.identifiant_pour_chaine("exécute");
	ID::chemin = table.identifiant_pour_chaine("chemin");
	ID::creation_contexte = table.identifiant_pour_chaine("création_contexte");
	ID::ajoute_chaine_a_la_compilation = table.identifiant_pour_chaine("ajoute_chaine_à_la_compilation");
	ID::ajoute_fichier_a_la_compilation = table.identifiant_pour_chaine("ajoute_fichier_à_la_compilation");
	ID::compilatrice = table.identifiant_pour_chaine("compilatrice");
	ID::compilatrice_obtiens_options = table.identifiant_pour_chaine("compilatrice_obtiens_options");
	ID::compilatrice_ajourne_options = table.identifiant_pour_chaine("compilatrice_ajourne_options");
	ID::fonction_test_variadique_externe = table.identifiant_pour_chaine("fonction_test_variadique_externe");
	ID::test = table.identifiant_pour_chaine("test");
	ID::assert_ = table.identifiant_pour_chaine("assert");
}
