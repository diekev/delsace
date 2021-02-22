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

#include "modules.hh"

#include "compilatrice.hh"
#include "portee.hh"

template <>
const int FichierExistant::tag = tags++;

template <>
const int FichierNeuf::tag = tags++;

/* ************************************************************************** */

bool Fichier::importe_module(IdentifiantCode *nom_module) const
{
	return modules_importes.possede(nom_module);
}

DonneesConstantesModule *SystemeModule::trouve_ou_cree_module(IdentifiantCode *nom, kuri::chaine_statique chemin)
{
	auto chemin_normalise = kuri::chaine(chemin);

	if (chemin_normalise.taille() > 0 && chemin_normalise[chemin_normalise.taille() - 1] != '/') {
		chemin_normalise = enchaine(chemin_normalise, '/');
	}

	POUR_TABLEAU_PAGE (donnees_modules) {
		if (it.chemin == chemin_normalise) {
			return &it;
		}
	}

	return cree_module(nom, chemin_normalise);
}

DonneesConstantesModule *SystemeModule::cree_module(IdentifiantCode *nom, kuri::chaine_statique chemin)
{
	auto dm = donnees_modules.ajoute_element();
	dm->nom = nom;
	dm->chemin = chemin;

	return dm;
}

DonneesConstantesFichier *SystemeModule::trouve_ou_cree_fichier(kuri::chaine_statique nom, kuri::chaine_statique chemin)
{
	POUR_TABLEAU_PAGE (donnees_fichiers) {
		if (it.chemin == chemin) {
			return &it;
		}
	}

	return cree_fichier(nom, chemin);
}

DonneesConstantesFichier *SystemeModule::cree_fichier(kuri::chaine_statique nom, kuri::chaine_statique chemin)
{
	auto df = donnees_fichiers.ajoute_element();
	df->nom = nom;
	df->chemin = chemin;
	df->id = donnees_fichiers.taille() - 1;

	return df;
}

void SystemeModule::rassemble_stats(Statistiques &stats) const
{
	stats.nombre_modules = donnees_modules.taille();

	auto &stats_fichiers = stats.stats_fichiers;
	POUR_TABLEAU_PAGE (donnees_fichiers) {
		auto entree = EntreeFichier();
		entree.nom = it.nom;
		entree.nombre_lignes = it.tampon.nombre_lignes();
		entree.memoire_tampons = it.tampon.taille_donnees();
		entree.memoire_lexemes = it.lexemes.taille() * taille_de(Lexeme);
		entree.nombre_lexemes = it.lexemes.taille();
		entree.temps_chargement = it.temps_chargement;
		entree.temps_tampon = it.temps_tampon;
		entree.temps_lexage = it.temps_decoupage;

		stats_fichiers.fusionne_entree(entree);
	}
}

long SystemeModule::memoire_utilisee() const
{
	auto memoire = 0l;
	memoire += donnees_modules.memoire_utilisee();
	memoire += donnees_modules.memoire_utilisee();

	POUR_TABLEAU_PAGE (donnees_fichiers) {
		memoire += it.nom.taille();
		memoire += it.chemin.taille();
		memoire += it.tampon.chaine().taille();
	}

	POUR_TABLEAU_PAGE (donnees_modules) {
		memoire += it.chemin.taille();
	}

	return memoire;
}

/* ************************************************************************** */

PositionLexeme position_lexeme(Lexeme const &lexeme)
{
	auto pos = PositionLexeme{};
	pos.pos = lexeme.colonne;
	pos.numero_ligne = lexeme.ligne + 1;
	pos.index_ligne = lexeme.ligne;
	return pos;
}
