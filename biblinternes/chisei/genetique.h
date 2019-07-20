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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <algorithm>
#include <iostream>
#include <random>
#include "biblinternes/structures/chaine.hh"

#if defined __cpp_concepts && __cpp_concepts >= 201507
template <typename C>
concept bool ConceptConteneur()
{
	return requires(C a, C b)
	{
		C();
		C(a);
		a = b;
		(&a)->~C();
		a.debut();
		a.fin();
		a.cbegin();
		a.cend();
		a.swap(b);
		std::swap(a, b);
		a == b;
		a != b;
		a.taille();
		a.max_size();
		a.est_vide();
	};
}

template <typename I>
concept bool ConceptIterateur()
{
	return requires(I i, I j)
	{
		*i;
		++i;
	};
}

template <typename I>
concept bool ConceptIterateurEntree()
{
	return requires(I i, I j)
	{
		*i;
		i != j;
		//i->m;
		++i;
		(void)i++;
		(*i)++;
	};
}

template <typename I>
concept bool ConceptIterateurSortie()
{
	return requires(I r)
	{
		++r;
		r++;
		(*r)++;
		//*r = o;
		//*r++ = o;
	};
}

template <typename C, typename... Args>
concept bool ConceptFonction = requires(C op, Args... args)
{
	op(args...);
};


/**
 * Concept pour définir le type de problème à résoudre à l'aide de l'algorithme
 * génétique.
 */
template <typename T>
concept bool ConceptTypeProbleme = requires(
	typename T::type_donnees donnees,
	typename T::type_chromosome chromosome,
	std::mt19937 &rng,
	std::ostream &os)
{
	/**
	 * Le type de chromosome.
	 */
	typename T::type_chromosome;

	/**
	 * Le type de données à se passer entre l'algorithme et le programme
	 * appelant l'algorithme.
	 */
	typename T::type_donnees;

	/**
	 * Le nombre maximum d'itérations ou de générations de l'algorithme.
	 */
	T::GENERATIONS_MAX;

	/**
	 * Le nombre d'itérations à effectuer avant d'imprimer des informations
	 * de débogage.
	 */
	T::ITERATIONS_ETAPE;

	/**
	 * La taille maximum de la population.
	 */
	T::TAILLE_POPULATION;

	/**
	 * La taille de l'élite de la population. L'algorithme fonctionne par
	 * élitisme : les premiers TAILLE_ELITE chromosomes sont utilisés pour
	 * les croisements, et ceux-ci ne font l'objet d'aucune mutation.
	 */
	T::TAILLE_ELITE;

	/**
	 * La probabilité qu'un croisement soit effectué. La probabilité qu'une
	 * mutation ait lieu est de 1.0 - PROB_CROISEMENT.
	 */
	T::PROB_CROISEMENT;

	/**
	 * Créer la population à partir des données principales et d'un générateur
	 * de nombre aléatoire de type std::mt19937. Cette fonction doit retourner
	 * un dls::tableau<typename T::type_chromosome.
	 */
	T::cree_population(donnees, rng);

	/**
	 * Créer un chromosome à partir des données principales et d'un générateur
	 * de nombre aléatoire de type std::mt19937. Cette fonction doit retourner
	 * un typename T::type_chromosome, mais n'est pas appeler directement par
	 * l'algorithme : elle doit être appelé par la fonction cree_population.
	 */
	T::cree_chromosome(donnees, rng);

	/**
	 * Calcule l'aptitude du chromosome spécifié, qui est passé par référence,
	 * avec les données fournies.
	 */
	T::calcule_aptitude(donnees, chromosome);

	/**
	 * Croise deux chromosome spécifié. La fonction doit retourner un nouveau
	 * chromosome. Un générateur de nombre aléatoire de type std::mt19937
	 * est fourni.
	 */
	T::croise(donnees, chromosome, chromosome, rng);

	/**
	 * Mute le chromosome passé par référence. Un générateur de nombre aléatoire
	 * de type std::mt19937 est également passé par référence.
	 */
	T::mute(chromosome, rng);

	/**
	 * Compare les aptitudes de deux chromosomes spécifiés. Cette fonction est
	 * utilisé pour trié les chromosomes selon leurs aptitudes.
	 */
	T::compare_aptitude(chromosome, chromosome);

	/**
	 * Retourne l'aptitude du chromosome spécifié.
	 */
	T::aptitude(chromosome);

	/**
	 * Retourne vrai ou faux si le chromosome passé en paramètre est le meilleur
	 * pour résoudre le problème. Si la fonction retourne vrai, l'algorithme
	 * s'arrête et le chromosome est retourné.
	 */
	T::meilleur_trouve(chromosome);

	/**
	 * Fonction de rappel qui est appelé à chaque génération avec le meilleur
	 * chromosome de la génération passé en paramètre.
	 */
	T::rappel_pour_meilleur(donnees, chromosome);
};
#else
#	define ConceptConteneur typename
#	define ConceptIterateur typename
#	define ConceptIterateurEntree typename
#	define ConceptIterateurSortie typename
#	define ConceptFonction typename
#	define ConceptTypeProbleme typename
#endif

namespace dls {
namespace chisei {

/* ************************************************************************** */

/**
 * Créé une chaine de type dls::chaine de la longueur spécifiée contenant des
 * caractères aléatoire générés par le générateur donnée.
 */
dls::chaine chaine_aleatoire(std::mt19937 &generateur, long longueur);

/**
 * Croise les valeurs pointées par deux itérateurs entre debut1-fin1 et
 * debut2-fin2 selon la fonction spécifiée. À chaque itération, la fonction est
 * appelée, et selon son résultat la valeur de l'un ou l'autre itérateur est
 * choisie et mise dans le conteneur pointé par l'itérateur sortie.
 */
template <
		ConceptIterateurEntree IterEntree1,
		ConceptIterateurEntree IterEntree2,
		ConceptIterateurSortie IterSortie1,
		ConceptFonction Fonction>
inline auto croise_si(
		IterEntree1 debut1,
		IterEntree1 fin1,
		IterEntree2 debut2,
		IterEntree2 fin2,
		IterSortie1 sortie,
		Fonction fonction)
{
	while ((debut1 != fin1) && (debut2 != fin2)) {
		if (fonction()) {
			*sortie++ = *debut1;
		}
		else {
			*sortie++ = *debut2;
		}

		++debut1;
		++debut2;
	}
}

/**
 * Croise les valeurs de deux conteneurs, conteneur1 et conteneur2, selon la
 * fonction spécifiée. À chaque itération, la fonction est appelée, et selon son
 * résultat la valeur de l'un ou l'autre conteneur est choisie et mise dans le
 * conteneur sortie.
 */
template <
		ConceptConteneur Cont1,
		ConceptConteneur Cont2,
		ConceptConteneur ContSortie1,
		ConceptFonction Fonction>
inline auto croise_si(
		const Cont1 &conteneur1,
		const Cont2 &conteneur2,
		ContSortie1 &conteneur_sortie,
		Fonction fonction)
{
	croise_si(conteneur1.debut(), conteneur1.fin(),
			  conteneur2.debut(), conteneur2.fin(),
			  conteneur_sortie.debut(),
			  fonction);
}

/**
 * Compte le nombre de correspondances entre les valeurs pointés par deux
 * itérateurs.
 */
template <
		ConceptIterateurEntree IterEntree1,
		ConceptIterateurEntree IterEntree2>
inline auto compte_correspondances(
		IterEntree1 debut1,
		IterEntree1 fin1,
		IterEntree2 debut2,
		IterEntree2 fin2)
{
	auto compte = 0;

	while ((debut1 != fin1) && (debut2 != fin2)) {
		if (*debut1++ == *debut2++) {
			++compte;
		}
	}

	return compte;
}

/**
 * Compte le nombre de correspondances entre les valeurs de deux conteneurs.
 */
template <ConceptConteneur Cont1, ConceptConteneur Cont2>
inline auto compte_correspondances(Cont1 conteneur1, Cont2 conteneur2)
{
	return compte_correspondances(
				conteneur1.debut(), conteneur1.fin(),
				conteneur2.debut(), conteneur2.fin());
}

/* ************************************************************************** */

/**
 * Simple structure utilisée pour retourner des informations de l'exécution de
 * l'algorithme génétique.
 */
struct DonneesAlgorithme {
	int generations = 0;
	double meilleur_aptitude = 0.0;
	double aptitude_moyenne = 0.0;

	DonneesAlgorithme() = default;
};

/**
 * Lance un algorithme génétique pour résoudre le problème définit par le
 * TypeProbleme, avec les données spécifiées.
 *
 * Quand l'algorithme s'achevé, soit en ayant atteint le nombre maximal
 * d'itérations défini par le TypeProbleme, soit en ayant trouvé le meilleur
 * chromosome définit par le TypeProbleme, il retourne une copie du meilleur
 * choromosome au moment de son achèvement, ainsi qu'une instance de la
 * structure DonneesAlgorithme, qui rassemble certaines données de l'algorithme
 * au moment de sa fin.
 */
template <ConceptTypeProbleme TypeProbleme>
auto lance_algorithme_genetique(std::ostream &os, const typename TypeProbleme::type_donnees &donnees)
{
	using type_chromosome = typename TypeProbleme::type_chromosome;

	std::mt19937 rng(19937);
	std::mt19937 rng_crp(19937);
	std::uniform_real_distribution<double> dist(0.0, 1.0);

	auto population = TypeProbleme::cree_population(donnees, rng);

	auto generation = 0;
	auto moyenne = 0.0;
	auto meilleur_aptitude = 0.0;
	auto meilleur_chromosome = type_chromosome();

	while (generation++ < TypeProbleme::GENERATIONS_MAX) {
		std::sort(population.debut(), population.fin(), TypeProbleme::compare_aptitude);

		moyenne = 0.0;
		auto parent = 0;
		auto meilleur_index = 0;

		for (size_t i = 0; i < TypeProbleme::TAILLE_POPULATION; ++i) {
			/* Élitisme : on ne s'occupe que des chromosomes discriminés. */
			if (i >= TypeProbleme::TAILLE_ELITE) {
				if (dist(rng_crp) < TypeProbleme::PROB_CROISEMENT) {
					/* Croisement. */

					/* Choix des parents. */
					const auto index1 = parent + static_cast<int>(dist(rng_crp) * 10);
					const auto index2 = parent + 1 + static_cast<int>(dist(rng_crp) * 10);

					population[i] = TypeProbleme::croise(donnees, population[index1], population[index2], rng);
					TypeProbleme::calcule_aptitude(donnees, population[i]);
					parent += 1;
				}
				else {
					/* Mutation. */
					TypeProbleme::mute(population[i], rng);
					TypeProbleme::calcule_aptitude(donnees, population[i]);
				}
			}

			const auto aptitude_i = TypeProbleme::aptitude(population[i]);
			moyenne += aptitude_i;

			if (aptitude_i > meilleur_aptitude) {
				meilleur_aptitude = aptitude_i;
				meilleur_index = i;
			}

			if (TypeProbleme::meilleur_trouve(population[i])) {
				return std::pair<type_chromosome, DonneesAlgorithme>{
					population[i],
					DonneesAlgorithme{ generation, meilleur_aptitude, moyenne / i }
				};
			}
		}

		meilleur_chromosome = population[meilleur_index];

		TypeProbleme::rappel_pour_meilleur(donnees, meilleur_chromosome);

		if ((generation % TypeProbleme::ITERATIONS_ETAPE) == 0) {
			os << "Meilleur : " << meilleur_aptitude << '\n';
			os << "Moyenne : " << moyenne / TypeProbleme::TAILLE_POPULATION << '\n';
		}
	}

	return std::pair<type_chromosome, DonneesAlgorithme>{
		meilleur_chromosome,
		DonneesAlgorithme{ generation, meilleur_aptitude, moyenne / TypeProbleme::TAILLE_POPULATION }
	};
}

}  /* namespace chisei */
}  /* namespace dls */
