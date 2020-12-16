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

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#undef STATISTIQUES_DETAILLEES

#ifdef STATISTIQUES_DETAILLEES
#	define CHRONO_TYPAGE(entree_stats, nom) \
	dls::chrono::chrono_rappel_milliseconde VARIABLE_ANONYME(chrono)([&](double temps) { \
		entree_stats.fusionne_entree({ nom, temps }); \
	})
#else
#	define CHRONO_TYPAGE(entree_stats, nom)
#endif

#if defined __cpp_concepts && __cpp_concepts >= 201507
template <typename T>
concept TypeEntreesStats = requires(T a, T b)
{
    a += b;
};
#else
#   define TypeEntreesStats typename
#endif

struct EntreeNombreMemoire {
    dls::chaine nom = "";
    long compte = 0;
    long memoire = 0;

    EntreeNombreMemoire &operator += (EntreeNombreMemoire const &autre)
    {
        compte += autre.compte;
        memoire += autre.memoire;
        return *this;
    }
};

struct EntreeFichier {
    dls::chaine nom = "";
    long memoire_lexemes = 0;
    long nombre_lexemes = 0;
    long nombre_lignes = 0;
    long memoire_tampons = 0;
    double temps_lexage = 0.0;
    double temps_parsage = 0.0;
    double temps_chargement = 0.0;
    double temps_tampon = 0.0;

    EntreeFichier &operator += (EntreeFichier const &autre)
    {
        memoire_lexemes += autre.memoire_lexemes;
        nombre_lignes += autre.nombre_lignes;
        temps_lexage += autre.temps_lexage;
        temps_parsage += autre.temps_parsage;
        temps_chargement += autre.temps_chargement;
        temps_tampon += autre.temps_tampon;
        nombre_lexemes += autre.nombre_lexemes;
        memoire_tampons += autre.memoire_tampons;
        return *this;
    }
};

struct EntreeTemps {
	const char *nom{};
	double temps = 0.0;

	EntreeTemps &operator += (EntreeTemps const &autre)
	{
		temps += autre.temps;
		return *this;
	}
};

template <TypeEntreesStats T>
struct EntreesStats {
	dls::chaine nom{};
    dls::tableau<T> entrees{};
    T totaux{};

	EntreesStats(dls::chaine const &nom_)
		: nom(nom_)
	{}

    void ajoute_entree(T const &entree)
    {
        totaux += entree;
        entrees.ajoute(entree);
    }

    void fusionne_entree(T const &entree)
    {
        totaux += entree;

        for (auto &e : entrees) {
            if (e.nom == entree.nom) {
                e += entree;
                return;
            }
        }

        entrees.ajoute(entree);
    }
};

using StatistiquesFichiers = EntreesStats<EntreeFichier>;
using StatistiquesArbre = EntreesStats<EntreeNombreMemoire>;
using StatistiquesGraphe = EntreesStats<EntreeNombreMemoire>;
using StatistiquesTypes = EntreesStats<EntreeNombreMemoire>;
using StatistiquesOperateurs = EntreesStats<EntreeNombreMemoire>;
using StatistiquesNoeudCode = EntreesStats<EntreeNombreMemoire>;
using StatistiquesMessage = EntreesStats<EntreeNombreMemoire>;
using StatistiquesRI = EntreesStats<EntreeNombreMemoire>;

struct Statistiques {
    long nombre_modules = 0ul;
    long nombre_identifiants = 0ul;
    long nombre_metaprogrammes_executes = 0ul;
    long memoire_compilatrice = 0ul;
    long memoire_ri = 0ul;
    long memoire_mv = 0ul;
    double temps_generation_code = 0.0;
    double temps_fichier_objet = 0.0;
    double temps_executable = 0.0;
    double temps_nettoyage = 0.0;
    double temps_ri = 0.0;
    double temps_metaprogrammes = 0.0;
    double temps_scene = 0.0;
    double temps_lexage = 0.0;
    double temps_parsage = 0.0;
    double temps_typage = 0.0;
    double temps_chargement = 0.0;
    double temps_tampons = 0.0;

	StatistiquesFichiers stats_fichiers{"Fichiers"};
	StatistiquesArbre stats_arbre{"Arbre Syntaxique"};
	StatistiquesGraphe stats_graphe_dependance{"Graphe Dépendances"};
	StatistiquesTypes stats_types{"Types"};
	StatistiquesOperateurs stats_operateurs{"Opérateurs"};
	StatistiquesNoeudCode stats_noeuds_code{"Noeuds Code"};
	StatistiquesMessage stats_messages{"Messages"};
	StatistiquesRI stats_ri{"Représentation Intermédiaire"};
};

struct StatistiquesTypage {
	EntreesStats<EntreeTemps> validation_decl{"Déclarations Variables"};
	EntreesStats<EntreeTemps> validation_appel{"Appels"};
	EntreesStats<EntreeTemps> ref_decl{"Références Déclarations"};
	EntreesStats<EntreeTemps> operateurs_unaire{"Opérateurs Unaire"};
	EntreesStats<EntreeTemps> operateurs_binaire{"Opérateurs Binaire"};
	EntreesStats<EntreeTemps> fonctions{"Fonctions"};
	EntreesStats<EntreeTemps> enumerations{"Énumérations"};
	EntreesStats<EntreeTemps> structures{"Structures"};
	EntreesStats<EntreeTemps> assignations{"Assignations"};
};
