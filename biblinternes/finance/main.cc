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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

template <typename I, typename O>
void retours_investissement_ex(I first_in, I last_in, O first_out, O last_out)
{
	auto second_in = first_in + 1;

	while (second_in != last_in && first_out != last_out) {
		*first_out++ = 1.0 + (*second_in - *first_in) / *first_in;

		++first_in;
		++second_in;
	}
}

template <typename T>
dls::tableau<T> retours_investissement(const dls::tableau<T> &cours)
{
	auto retours = dls::tableau<T>(cours.taille() - 1);

	retours_investissement_ex(cours.debut(), cours.fin(),
							  retours.debut(), retours.fin());

	return retours;
}

template <typename T>
T retour_moyen(const dls::tableau<T> &retours)
{
	auto resultat = static_cast<T>(1);

	for (const auto &retour : retours) {
		resultat *= retour;
	}

	resultat = std::pow(resultat, static_cast<T>(1) / static_cast<T>(retours.taille()));

	return resultat;
}

template <typename T>
inline T carre(T t)
{
	return t * t;
}

template <typename T>
T risque(const dls::tableau<T> &retours, const T moyenne)
{
	auto resultat = static_cast<T>(0);

	for (const auto &retour : retours) {
		resultat += carre(retour - moyenne) / static_cast<T>(retours.taille());
	}

	return std::sqrt(resultat);
}

template <typename T>
T covariance(const dls::tableau<T> &retours1, const T moyenne1, const dls::tableau<T> &retours2, const T moyenne2)
{
	const auto n = std::min(retours1.taille(), retours2.taille());
	auto resultat = static_cast<T>(0);

	for (auto i = 0; i < n; ++i) {
		resultat += (retours1[i] - moyenne1) * (retours2[i] - moyenne2) / static_cast<T>(n);
	}

	return resultat;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "Utilisation : " << argv[0] << " fichier_cours.\n";
		return 1;
	}

	std::ifstream fichier_cours;
	fichier_cours.open(argv[1]);

	if (!fichier_cours.is_open()) {
		std::cerr << "Impossible d'ouvrir le fichier texte!\n";
		return 1;
	}

	dls::tableau<dls::chaine> actions;
	dls::tableau<dls::tableau<double>> valeurs_actions;

	std::string ligne;
	while (std::getline(fichier_cours, ligne)) {
		std::istringstream iss(ligne);

		/* Lis le nom de l'action. */
		dls::chaine action;
		iss >> action;

		actions.pousse(action);

		/* Lis les valeurs des actions. */
		double valeur;
		dls::tableau<double> valeurs;

		while (iss >> valeur) {
			valeurs.pousse(valeur);
		}

		valeurs_actions.pousse(valeurs);
	}

	dls::tableau<double> retours_geometrique;
	dls::tableau<dls::tableau<double>> retours_actions;
	dls::tableau<double> risques;

	for (const dls::tableau<double> &valeurs_action : valeurs_actions) {
		const auto &retours = retours_investissement(valeurs_action);
		const auto &retour = retour_moyen(retours);
		const auto &risque_ = risque(retours, retour);

		retours_actions.pousse(retours);
		retours_geometrique.pousse(retour);
		risques.pousse(risque_);
	}

	if (retours_geometrique.taille() != actions.taille()) {
		std::cerr << "Le nombre de retours est différent de celui des actions!\n";
	}

	for (auto i = 0; i < actions.taille(); ++i) {
		std::cerr << "Retour sur " << actions[i]
				  << " : " << retours_geometrique[i]
				  << ", risque : " << risques[i] << '\n';
	}

	dls::tableau<std::pair<dls::chaine, float>> covariances;

	for (auto i = 0; i < actions.taille(); ++i) {
		for (auto j = i + 1; j < actions.taille(); ++j) {
			auto cov = covariance(retours_actions[i], retours_geometrique[i],
								  retours_actions[j], retours_geometrique[j]);

			covariances.pousse(std::make_pair(actions[i] + "-" + actions[j], cov));
		}
	}

	std::sort(covariances.debut(), covariances.fin(),
			  [](const std::pair<dls::chaine, float> pair1, const std::pair<dls::chaine, float> pair2)
	{
		return pair1.second > pair2.second;
	});

	for (const auto &pair : covariances) {
		std::cerr << "Covariance : " << pair.first << " : " << pair.second <<'\n';
	}

	return 0;
}
