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
#include <vector>

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
std::vector<T> retours_investissement(const std::vector<T> &cours)
{
	auto retours = std::vector<T>(cours.size() - 1);

	retours_investissement_ex(cours.begin(), cours.end(),
							  retours.begin(), retours.end());

	return retours;
}

template <typename T>
T retour_moyen(const std::vector<T> &retours)
{
	auto resultat = static_cast<T>(1);

	for (const auto &retour : retours) {
		resultat *= retour;
	}

	resultat = std::pow(resultat, static_cast<T>(1) / static_cast<T>(retours.size()));

	return resultat;
}

template <typename T>
inline T carre(T t)
{
	return t * t;
}

template <typename T>
T risque(const std::vector<T> &retours, const T moyenne)
{
	auto resultat = static_cast<T>(0);

	for (const auto &retour : retours) {
		resultat += carre(retour - moyenne) / static_cast<T>(retours.size());
	}

	return std::sqrt(resultat);
}

template <typename T>
T covariance(const std::vector<T> &retours1, const T moyenne1, const std::vector<T> &retours2, const T moyenne2)
{
	const auto n = std::min(retours1.size(), retours2.size());
	auto resultat = static_cast<T>(0);

	for (size_t i = 0; i < n; ++i) {
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

	std::vector<std::string> actions;
	std::vector<std::vector<double>> valeurs_actions;

	std::string ligne;
	while (std::getline(fichier_cours, ligne)) {
		std::istringstream iss(ligne);

		/* Lis le nom de l'action. */
		std::string action;
		iss >> action;

		actions.push_back(action);

		/* Lis les valeurs des actions. */
		double valeur;
		std::vector<double> valeurs;

		while (iss >> valeur) {
			valeurs.push_back(valeur);
		}

		valeurs_actions.push_back(valeurs);
	}

	std::vector<double> retours_geometrique;
	std::vector<std::vector<double>> retours_actions;
	std::vector<double> risques;

	for (const std::vector<double> &valeurs_action : valeurs_actions) {
		const auto &retours = retours_investissement(valeurs_action);
		const auto &retour = retour_moyen(retours);
		const auto &risque_ = risque(retours, retour);

		retours_actions.push_back(retours);
		retours_geometrique.push_back(retour);
		risques.push_back(risque_);
	}

	if (retours_geometrique.size() != actions.size()) {
		std::cerr << "Le nombre de retours est différent de celui des actions!\n";
	}

	for (size_t i = 0; i < actions.size(); ++i) {
		std::cerr << "Retour sur " << actions[i]
				  << " : " << retours_geometrique[i]
				  << ", risque : " << risques[i] << '\n';
	}

	std::vector<std::pair<std::string, float>> covariances;

	for (size_t i = 0; i < actions.size(); ++i) {
		for (size_t j = i + 1; j < actions.size(); ++j) {
			auto cov = covariance(retours_actions[i], retours_geometrique[i],
								  retours_actions[j], retours_geometrique[j]);

			covariances.push_back(std::make_pair(actions[i] + "-" + actions[j], cov));
		}
	}

	std::sort(covariances.begin(), covariances.end(),
			  [](const std::pair<std::string, float> pair1, const std::pair<std::string, float> pair2)
	{
		return pair1.second > pair2.second;
	});

	for (const auto &pair : covariances) {
		std::cerr << "Covariance : " << pair.first << " : " << pair.second <<'\n';
	}

	return 0;
}
