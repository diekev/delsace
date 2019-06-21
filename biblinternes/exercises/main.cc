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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <csignal>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <numeric>
#include <stack>
#include <unordered_map>

#include "curry.h"
#include "divers.h"
#include "postfix.h"
#include "range.h"

#include "../math/outils.hh"
#include "../outils/definitions.h"
#include "../tests/test_unitaire.hh"

/* ************************************************************************** */

template <typename T>
static auto debug_queue(std::queue<T> queue)
{
	while (!queue.empty()) {
		std::cerr << queue.front() << '\n';
		queue.pop();
	}
}

static auto test_unitaire_postfix(dls::test_unitaire::Controleuse &controlleur)
{
	{
		const auto expression = "2 + 2";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 4.0);
	}

	{
		const auto expression = "2 * 2";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 4.0);
	}

	{
		const auto expression = "2 - 2";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 0.0);
	}

	{
		const auto expression = "2 / 2";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 1.0);
	}

	{
		const auto expression = "2 * 2 + 9";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 13.0);
	}

	{
		const auto expression = "2 * 2 + 9 * 2";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 22.0);
	}

	{
		const auto expression = "10!";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 3628800.0);
	}

	{
		const auto expression = "sqrt 25";
		auto sortie = postfix(expression);
		const auto resultat = evaluate_postfix(sortie);

		CU_VERIFIE_EGALITE(controlleur, resultat, 5.0);
	}
}

static auto test_postfix(std::ostream &os, std::istream &is)
{
	os << "Lancement des tests unitaires\n";

	dls::test_unitaire::Controleuse controlleur(os);
	controlleur.ajoute_fonction(test_unitaire_postfix);
	controlleur.performe_controles();
	controlleur.imprime_resultat();

	os << '\n';

	std::string expression;
	os << "Infix to postfix compiler, type an expression, then press enter\n";

	os << std::fixed;

	while (true) {
		os << ">>>> ";
		std::getline(is, expression);

		if (expression == "-1") {
			break;
		}

		auto output = postfix(expression);
		//		debug_queue(output);
		auto result = evaluate_postfix(output);

		os << result << '\n';
	}
}

/* ************************************************************************** */

static auto test_hangman(std::ostream &os, std::istream &is)
{
	os << "Who should guess?\n";
	os << "Enter 0 for the computer to guess, 1 to be the guesser\n";

	int guesser;
	is >> guesser;

	hangman(os, is, static_cast<hangman_player>(guesser));
}

/* ************************************************************************** */

static auto test_to_words(std::ostream &os, std::istream &is)
{
	os << "Translate a number to a sentence, type a number, then press enter\n";
	int number;

	while (true) {
		os << ">>>> ";
		is >> number;

		try {
			auto result = to_words(number);
			os << result << '\n';
		}
		catch (const InvalidNumberExcetion &c) {
			std::cerr << c.what() << '\n';
		}
	}
}

/* ************************************************************************** */

template <typename Container>
static auto all(const Container &container)
{
	return range<typename Container::const_iterator>(container.cbegin(), container.cend() - 1);
}

template <typename RangeType>
static auto retro(const RangeType &r)
{
	return reverse_range<RangeType>(r);
}

template <typename RangeType, typename ValueType>
static auto find(RangeType r, ValueType v)
{
	for (; !r.empty(); r.pop_front()) {
		if (r.front() == v) {
			break;
		}
	}

	return r;
}

static auto test_range(std::ostream &os, std::istream &)
{
	std::vector<int> v = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	auto r0 = all(v);

	auto r = retro(find(r0, 5));

	while (!r.empty()) {
		os << r.front() << '\n';
		r.pop_front();
	}
}

/* ************************************************************************** */

using pointer_relocation_map = std::unordered_map<void *, void *>;

static constexpr auto redo_filename = "/home/kevin/redo.txt";
static constexpr auto undo_filename = "/home/kevin/undo.txt";

static auto read_prm(std::ifstream &file, pointer_relocation_map &prm)
{
	if (file.is_open()) {
		return;
	}

	auto size = pointer_relocation_map::size_type{0};
	file >> size;

	while (size-- != 0) {
		auto key = pointer_relocation_map::key_type{};
		auto value = pointer_relocation_map::key_type{};

		file >> key >> value;

		//	prm[key] = value;
		prm.insert(std::pair<void *, void *>{ key, value });
	}

	file.close();
}

static auto write_prm(std::ofstream &file, pointer_relocation_map &prm)
{
	if (!file.is_open()) {
		return;
	}

	file << prm.size() << '\n';

	for (auto entry : prm) {
		file << entry.first << ' ' << entry.second << '\n';
	}
}

static auto save_prm(std::ofstream &undo_file, pointer_relocation_map &prm)
{
	write_prm(undo_file, prm);
	prm.clear();

	std::ofstream redo_file;
	redo_file.open(redo_filename, std::ofstream::out | std::ofstream::trunc);
	redo_file.close();
}

static auto load_prm(std::ifstream &file, pointer_relocation_map &prm)
{
	auto tmp_prm = pointer_relocation_map{};

	read_prm(file, prm);

	for (auto entry : tmp_prm) {
		auto C = prm[entry.second];
		tmp_prm[entry.first] = C;

		if (C != entry.second) {
			auto iter = prm.find(entry.second);
			prm.erase(iter);
		}
	}

	for (auto entry : tmp_prm) {
		prm[entry.first] = entry.second;
	}
}

using object_t = int;

using undo_stack_t = std::stack<std::string>;
undo_stack_t undo_stack;

static auto change_object_value(std::ofstream &undo_file,
                                pointer_relocation_map &prm,
                                object_t *object, bool was_created)
{
	save_prm(undo_file, prm);

	undo_file << "change_object_value" << ' ' << object << ' ' << was_created << '\n';

	/* serialize object */
	if (!was_created) {
		undo_file << *object << '\n';
	}

	*object += 5;
}

object_t *create_object(pointer_relocation_map &prm)
{
	std::ofstream undo_file;
	undo_file.open(undo_filename, std::ios::app);

	save_prm(undo_file, prm);

	object_t *object = new object_t;
	*object = 0;

	undo_file << "Create" << ' ' << object << '\n';

	change_object_value(undo_file, prm, object, true);

	return object;
}

static auto delete_object(std::ofstream &undo_file,
                          pointer_relocation_map &prm,
                          object_t *object)
{
	save_prm(undo_file, prm);

	change_object_value(undo_file, prm, object, false);

	undo_file << "delete_object" << ' ' << object << ' ' << "object_t" << '\n';

	delete object;
}

static auto revert_delete_object(std::ofstream &redo_file,
                                 std::ifstream &undo_file,
                                 pointer_relocation_map &prm)
{
	/* read delete_object record */
	load_prm(undo_file, prm);

	object_t *object = new object_t;

	redo_file << "create" << ' ' << object << ' ' << "object_t" << '\n';

	/* nullptr here should be the ptr read from the delete_object record */
	prm[nullptr] = object;

	return object;
}

static auto test_prm(std::ostream &, std::istream &)
{
	std::ofstream undo_file;
	undo_file.open(undo_filename, std::ofstream::out | std::ofstream::trunc);

	std::ofstream redo_file;
	redo_file.open(redo_filename, std::ofstream::out | std::ofstream::trunc);
	redo_file.close();

	pointer_relocation_map prm;

	auto ob = create_object(prm);

	undo_file.open(undo_filename, std::ofstream::out | std::ofstream::app);

	delete_object(undo_file, prm, ob);
	undo_file.close();

	std::ifstream undo_file_in;
	undo_file_in.open(undo_filename);

	ob = revert_delete_object(redo_file, undo_file_in, prm);

	undo_file.open(undo_filename, std::ofstream::out | std::ofstream::trunc);
	delete_object(undo_file, prm, ob);
}

/* ************************************************************************** */

static auto calcul_partage_dette(
		const std::vector<double> &dettes,
		const double capital,
		std::vector<double> &partages)
{
	auto total_dettes = std::accumulate(dettes.begin(), dettes.end(), 0.0);

	/* S'il y a suffisament de capitaux, distribue à chacun leurs dûs. */
	if (total_dettes <= capital) {
		partages = dettes;
		return;
	}

	partages.resize(dettes.size());
	std::fill(partages.begin(), partages.end(), 0.0);

	auto reste = capital;

	for (size_t i = 0; i < dettes.size() - 1; ++i) {
		/* Calcul parts contestées. */
		auto contestee = std::min(dettes[i], dettes[i + 1]);

		if (contestee > reste) {
			contestee = reste;
		}

		/* Divise également les parts contestées. */
		auto divisee = contestee / 2.0;
		partages[i] += divisee;
		partages[i + 1] += divisee;

		reste -= contestee;
	}

	/* Assigne les parts non-contestées. */
	partages[dettes.size() - 1] += reste;
}

static void test_unitaire_dette(dls::test_unitaire::Controleuse &controlleur)
{
	{
		std::vector<double> dettes = { 50.0 };
		auto capital = 100.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 50.0);
	}

	{
		std::vector<double> dettes = { 100.0 };
		auto capital = 75.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 75.0);
	}

	{
		std::vector<double> dettes = { 50.0, 100.0 };
		auto capital = 100.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 25.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 75.0);
	}

	{
		std::vector<double> dettes = { 100.0, 300.0 };
		auto capital = 66.0 + 2.0 / 3.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], (33.0 + 1.0 / 3.0));
		CU_VERIFIE_EGALITE(controlleur, partages[1], (33.0 + 1.0 / 3.0));
	}

	{
		std::vector<double> dettes = { 100.0, 300.0 };
		auto capital = 125;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 50.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 75.0);
	}

	{
		std::vector<double> dettes = { 100.0, 300.0 };
		auto capital = 200;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 50.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 150.0);
	}

	{
		std::vector<double> dettes = { 100.0, 200.0 };
		auto capital = 66.0 + 2.0 / 3.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], (33.0 + 1.0 / 3.0));
		CU_VERIFIE_EGALITE(controlleur, partages[1], (33.0 + 1.0 / 3.0));
	}

	{
		std::vector<double> dettes = { 100.0, 200.0 };
		auto capital = 125.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 50.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 75.0);
	}

	{
		std::vector<double> dettes = { 100.0, 200.0 };
		auto capital = 150.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 50.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 100.0);
	}

	{
		std::vector<double> dettes = { 200.0, 300.0 };
		auto capital = 66.0 + 2.0 / 3.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], (33.0 + 1.0 / 3.0));
		CU_VERIFIE_EGALITE(controlleur, partages[1], (33.0 + 1.0 / 3.0));
	}

	{
		std::vector<double> dettes = { 200.0, 300.0 };
		auto capital = 150.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 75.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 75.0);
	}

	{
		std::vector<double> dettes = { 200.0, 300.0 };
		auto capital = 250.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 100.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 150.0);
	}

	{
		std::vector<double> dettes = { 100.0, 200.0, 300.0 };
		auto capital = 100;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], (33.0 + 1.0 / 3.0));
		CU_VERIFIE_EGALITE(controlleur, partages[1], (33.0 + 1.0 / 3.0));
		CU_VERIFIE_EGALITE(controlleur, partages[2], (33.0 + 1.0 / 3.0));
	}

	{
		std::vector<double> dettes = { 100.0, 200.0, 300.0 };
		auto capital = 200.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 50.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 75.0);
		CU_VERIFIE_EGALITE(controlleur, partages[2], 75.0);
	}

	{
		std::vector<double> dettes = { 100.0, 200.0, 300.0 };
		auto capital = 300.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 50.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 100.0);
		CU_VERIFIE_EGALITE(controlleur, partages[2], 150.0);
	}

	{
		std::vector<double> dettes = { 100.0, 200.0, 300.0 };
		auto capital = 600.0;

		std::vector<double> partages;
		calcul_partage_dette(dettes, capital, partages);

		CU_VERIFIE_EGALITE(controlleur, partages[0], 100.0);
		CU_VERIFIE_EGALITE(controlleur, partages[1], 200.0);
		CU_VERIFIE_EGALITE(controlleur, partages[2], 300.0);
	}
}

static auto test_dette(std::ostream &, std::istream &)
{
	dls::test_unitaire::Controleuse controlleur;
	controlleur.ajoute_fonction(test_unitaire_dette);
	controlleur.performe_controles();
	controlleur.imprime_resultat();
}

/* ************************************************************************** */

static auto proteine(char valeur)
{
	switch (valeur) {
		default:
		case 0:
			return 'A';
		case 1:
			return 'C';
		case 2:
			return 'G';
		case 3:
			return 'T';
	}
}

static auto proteine_paire(char valeur)
{
	switch (valeur) {
		default:
		case 'A':
			return 'T';
		case 'G':
			return 'C';
		case 'C':
			return 'G';
		case 'T':
			return 'A';
	}
}

auto brin_pair(const std::string &brin)
{
	std::string pair;
	pair.resize(brin.size());

	for (size_t i = 0; i < brin.size(); ++i) {
		pair[i] = proteine_paire(brin[i]);
	}

	return pair;
}

static auto converti_vers_adn_ex(const char *ptr, const size_t taille, bool inverse)
{
	std::string tampon;
	tampon.resize(taille * 4);

	if (inverse) {
		for (size_t i = taille - 1; i != -1; --i) {
			tampon[i * 4 + 0] = proteine((*ptr >> 6) & 0b00000011);
			tampon[i * 4 + 1] = proteine((*ptr >> 4) & 0b00000011);
			tampon[i * 4 + 2] = proteine((*ptr >> 2) & 0b00000011);
			tampon[i * 4 + 3] = proteine((*ptr) & 0b00000011);

			++ptr;
		}
	}
	else {
		for (size_t i = 0; i < taille; ++i) {
			tampon[i * 4 + 0] = proteine((*ptr >> 6) & 0b00000011);
			tampon[i * 4 + 1] = proteine((*ptr >> 4) & 0b00000011);
			tampon[i * 4 + 2] = proteine((*ptr >> 2) & 0b00000011);
			tampon[i * 4 + 3] = proteine((*ptr) & 0b00000011);

			++ptr;
		}
	}

	return tampon;
}

template <typename T>
auto converti_vers_adn(T objet)
{
	constexpr auto taille = sizeof(T);
	auto *ptr = reinterpret_cast<const char *>(&objet);

	return converti_vers_adn_ex(ptr, taille, true);
}

template <>
auto converti_vers_adn(const char *objet)
{
	const auto taille = std::strlen(objet);

	return converti_vers_adn_ex(objet, taille, false);
}

template <>
auto converti_vers_adn(const std::string &objet)
{
	return converti_vers_adn_ex(objet.c_str(), objet.size(), false);
}

static auto tests_conversion_adn(dls::test_unitaire::Controleuse &controlleur)
{
	{
		auto adn = converti_vers_adn(0);

		CU_VERIFIE_EGALITE(controlleur, adn, std::string("AAAAAAAAAAAAAAAA"));
	}

	{
		auto adn = converti_vers_adn(1);

		CU_VERIFIE_EGALITE(controlleur, adn, std::string("AAAAAAAAAAAAAAAC"));
	}

	{
		auto adn = converti_vers_adn("ADN");

		CU_VERIFIE_EGALITE(controlleur, adn, std::string("CAACCACACATG"));
	}

	{
		auto adn = converti_vers_adn("Better Call Saul");

		CU_VERIFIE_EGALITE(controlleur,
						   adn,
						   std::string("CAAGCGCCCTCACTCACGCCCTAGAGAACAATCGACCGTACGTAAGAACCATCGACCTCCCGTA"));
	}
}

static auto test_adn(std::ostream &os, std::istream &)
{
	dls::test_unitaire::Controleuse controlleur(os);
	controlleur.ajoute_fonction(tests_conversion_adn);
	controlleur.performe_controles();
	controlleur.imprime_resultat();
}

/* ************************************************************************** */

static auto test_constexpr_if_ex(dls::test_unitaire::Controleuse &controlleur)
{
	CU_VERIFIE_EGALITE(controlleur, dls::math::est_pair(3), false);
	CU_VERIFIE_EGALITE(controlleur, dls::math::est_pair(3.0), false);
	CU_VERIFIE_EGALITE(controlleur, dls::math::est_pair(2), true);
	CU_VERIFIE_EGALITE(controlleur, dls::math::est_pair(2.0), false);

	CU_VERIFIE_EGALITE(controlleur, dls::math::tolerance<int>(), 0);
	CU_VERIFIE_EGALITE(controlleur, dls::math::tolerance<float>(), 1e-5f);
	CU_VERIFIE_EGALITE(controlleur, dls::math::tolerance<double>(), 1e-8);
	CU_VERIFIE_EGALITE(controlleur, dls::math::tolerance<std::string>(), std::string{""});
}

static auto test_constexpr_if(std::ostream &os, std::istream &)
{
	dls::test_unitaire::Controleuse controlleur(os);
	controlleur.ajoute_fonction(test_constexpr_if_ex);
	controlleur.performe_controles();
	controlleur.imprime_resultat();
}

/* ************************************************************************** */

struct Donnees {
	const int nombre_candidats = 6;
	int ensemble_candidats[6] = { 1, 2, 5, 10, 20, 50 };
	std::vector<int> ensemble_resultat{};
	int resultat_courant = 0;
	int objectif = 0;

	Donnees() = default;
};

static auto choisis_meilleur_candidat(Donnees &donnees)
{
	for (int i = donnees.nombre_candidats - 1; i >= 0; --i) {
		if (donnees.ensemble_candidats[i] <= donnees.resultat_courant) {
			return donnees.ensemble_candidats[i];
		}
	}

	return -1;
}

static auto candidat_peut_contribuer(int candidat)
{
	return candidat != -1;
}

static auto assigne_candidat(Donnees &donnees, int candidat)
{
	donnees.ensemble_resultat.push_back(candidat);
	donnees.resultat_courant -= candidat;
}

static auto solution_trouve(const Donnees &donnees)
{
	return donnees.resultat_courant == 0;
}

static auto test_algorithm_glouton(std::ostream &os, std::istream &is)
{
	int nombre;

	os << "Tapez un nombre (zéro pour quitter) : ";
	is >> nombre;

	while (nombre != 0) {
		Donnees donnees;
		donnees.objectif = nombre;
		donnees.resultat_courant = donnees.objectif;

		while (!solution_trouve(donnees)) {
			auto candidat = choisis_meilleur_candidat(donnees);

			if (candidat_peut_contribuer(candidat)) {
				assigne_candidat(donnees, candidat);
			}
		}

		os << "Pour distribuer " << donnees.objectif << " centimes, on peut donner "
			  "les pièces suivantes :";

		for (int piece : donnees.ensemble_resultat) {
			os << " " << piece;
		}

		os << ".\n";

		os << "Tapez un nombre (zéro pour quitter) : ";
		is >> nombre;
	}
}

#include <regex>

namespace chaine_caractere {

static const char *mots_vides[] = {
	"alors",
	"au",
	"aucuns",
	"aussi",
	"autre",
	"avant",
	"avec",
	"avoir",
	"bon",
	"car",
	"ce",
	"cela",
	"ces",
	"ceux",
	"chaque",
	"ci",
	"comme",
	"comment",
	"dans",
	"des",
	"du",
	"dedans",
	"dehors",
	"depuis",
	"deux",
	"devrait",
	"doit",
	"donc",
	"dos",
	"droite",
	"début",
	"elle",
	"elles",
	"en",
	"encore",
	"essai",
	"est",
	"et",
	"eu",
	"fait",
	"faites",
	"fois",
	"font",
	"force",
	"haut",
	"hors",
	"ici",
	"il",
	"ils",
	"je",
	"juste",
	"la",
	"le",
	"les",
	"leur",
	"là",
	"ma",
	"maintenant",
	"mais",
	"mes",
	"mine",
	"moins",
	"mon",
	"mot",
	"même",
	"ni",
	"nommés",
	"notre",
	"nous",
	"nouveaux",
	"ou",
	"où",
	"par",
	"parce",
	"parole",
	"pas",
	"personnes",
	"peut",
	"peu",
	"pièce",
	"plupart",
	"pour",
	"pourquoi",
	"quand",
	"que",
	"quel",
	"quelle",
	"quelles",
	"quels",
	"qui",
	"sa",
	"sans",
	"ses",
	"seulement",
	"si",
	"sien",
	"son",
	"sont",
	"sous",
	"soyez",
	"sujet",
	"sur",
	"ta",
	"tandis",
	"tellement",
	"tels",
	"tes",
	"ton",
	"tous",
	"tout",
	"trop",
	"très",
	"tu",
	"valeur",
	"voie",
	"voient",
	"vont",
	"votre",
	"vous",
	"vu",
	"ça",
	"étaient",
	"état",
	"étions",
	"été",
	"être"
};

enum {
	IGNORE_APOSTROPHES = (1 << 0),
	IGNORE_TIRETS      = (1 << 1),
	IGNORE_MOTS_VIDES  = (1 << 2),
};

struct InformationsTexte {
	int nombre_mots = 0;
	int nombre_caracteres_avec_espaces = 0;
	int nombre_caracteres_sans_espaces = 0;
	int occurence_mots_cles = 0;

	int lignes_ordinateur = 0;
	int lignes_papier = 0;
	int pages_ordinateur = 0;
	int pages_papier = 0;

	double temps_lecture = 0.0;
	double temps_frappe = 0.0;
	double temps_manuscrit = 0.0;
	double temps_elocution = 0.0;

	InformationsTexte() = default;
};

auto rogne(const std::string &texte, const std::string &espaces_blancs = " \n\t")
{
	const auto debut = texte.find_first_not_of(espaces_blancs);

	if (debut == std::string::npos) {
		return std::string{""};
	}

	const auto fin = texte.find_last_not_of(espaces_blancs);
	const auto plage = fin - debut + 1;

	return texte.substr(debut, plage);
}

auto decoupe(const std::string &texte, const char delimiteur)
{
	std::vector<std::string> morceaux;
	std::string bout;
	std::stringstream ss(texte);

	while (std::getline(ss, bout, delimiteur)) {
		morceaux.push_back(bout);
	}

	return morceaux;
}

auto nombre_caractere(bool espaces, std::string texte)
{
	if (!espaces) {
		texte = std::regex_replace(texte, std::regex("/\\s+/g"), " ");
	}

	return texte.size();
}

enum {
	TEMPS_LECTURE   = 0,
	TEMPS_FRAPPE    = 1,
	TEMPS_MANUSCRIT = 2,
	TEMPS_ELOCUTION = 3,
};

auto temps(int nombre_mot, int type)
{
	auto n = 0.0;

	switch (type) {
		case TEMPS_LECTURE:
			n = std::ceil(nombre_mot * 60.0 / 250.0);
			break;
		case TEMPS_FRAPPE:
			n = std::ceil(nombre_mot * 60.0 / 40.0);
			break;
		case TEMPS_MANUSCRIT:
			n = std::ceil(nombre_mot * 60.0 / 25.0);
			break;
		case TEMPS_ELOCUTION:
			n = std::ceil(nombre_mot * 60.0 / 150.0);
			break;
	}

	return n;
}

auto informations_texte(const std::string &texte, int drapeaux, const std::string &mots_cles)
{
	auto informations = InformationsTexte();

	if (texte.empty()) {
		return informations;
	}

	auto copie_texte1 = texte;
	auto copie_texte2 = texte;

	if ((drapeaux & IGNORE_APOSTROPHES) != 0) {
		const auto regex = std::regex("[\\'\\’]");  // "gi"
		copie_texte1 = std::regex_replace(copie_texte1, regex, "");
	}
	else {
		const auto regex = std::regex("[\\'\\’]");  // "gi"
		copie_texte1 = std::regex_replace(copie_texte1, regex, " ");
	}

	if ((drapeaux & IGNORE_TIRETS) != 0) {
		const auto regex = std::regex("[\\-]");  // "gi"
		copie_texte1 = std::regex_replace(copie_texte1, regex, "");
	}
	else {
		const auto regex = std::regex("[\\-]");  // "gi"
		copie_texte1 = std::regex_replace(copie_texte1, regex, " ");
	}

	if ((drapeaux & IGNORE_MOTS_VIDES) != 0) {
		std::string motif;
		std::regex regex;

		for (const auto &mot_vide : mots_vides) {
			motif = std::string{"^\\s*"} + mot_vide + "\\s*$";
			motif += std::string{"|^\\s*"} + mot_vide + "\\s+";
			motif += std::string{"|\\s+"} + mot_vide + "\\s*$";
			motif += std::string{"|\\s+"} + mot_vide + "\\s+";

			regex = std::regex(motif);  // "gi"

			copie_texte1 = std::regex_replace(copie_texte1, regex, " ");
			copie_texte2 = std::regex_replace(copie_texte2, regex, " ");
		}
	}

	auto regex = std::regex("[^\\w^àáâãäåçèéêëìíîïðòóôõöùúûüýÿ]");  // "gi"
	copie_texte1 = std::regex_replace(copie_texte1, regex," ");
	copie_texte1 = std::regex_replace(copie_texte1, std::regex("/\\s+/g"), " ");
	copie_texte1 = rogne(copie_texte1);

	auto atext = decoupe(texte, ' ');

	regex = std::regex("(\r\n|\r|\n)");  // "g"
	copie_texte2 = std::regex_replace(copie_texte2, regex, "");
	copie_texte2 = rogne(copie_texte2);

	if (!mots_cles.empty()) {
		regex = std::regex("\\b" + mots_cles + "\\b");  // g
		const auto iterateur_debut = std::sregex_iterator(copie_texte1.begin(), copie_texte1.end(), regex);
		const auto iterateur_fin = std::sregex_iterator();

		informations.occurence_mots_cles = static_cast<int>(std::distance(iterateur_debut, iterateur_fin));
	}

	if (mots_cles.empty() && (informations.occurence_mots_cles != 0 || atext[atext.size() - 1] == "")){
		informations.occurence_mots_cles = 0;
	}

	informations.nombre_mots = static_cast<int>((atext[atext.size() - 1ul]=="") ? atext.size() - 1ul : atext.size());

	const auto nombre_caracteres_sans_espace = nombre_caractere(false, copie_texte2);
	const auto lignes_ordinateur = static_cast<int>(static_cast<double>(nombre_caracteres_sans_espace) / 78.0 + 1.0);
	const auto lignes_papier = static_cast<int>(static_cast<double>(nombre_caracteres_sans_espace) / 41.0 + 1.0);

	informations.nombre_caracteres_sans_espaces = static_cast<int>(nombre_caracteres_sans_espace);
	informations.nombre_caracteres_avec_espaces = static_cast<int>(nombre_caractere(true, copie_texte2));
	informations.lignes_ordinateur = lignes_ordinateur;
	informations.lignes_papier = lignes_papier;
	informations.pages_ordinateur = static_cast<int>(lignes_ordinateur / 42.0 + 1.0);
	informations.pages_papier = static_cast<int>(lignes_papier / 31.0 + 1.0);

	informations.temps_elocution = temps(informations.nombre_mots, TEMPS_ELOCUTION);
	informations.temps_frappe = temps(informations.nombre_mots, TEMPS_FRAPPE);
	informations.temps_manuscrit = temps(informations.nombre_mots, TEMPS_MANUSCRIT);
	informations.temps_lecture = temps(informations.nombre_mots, TEMPS_LECTURE);

	return informations;
}

}  /* namespace chaine_caractere */

auto test_nombre_mot(std::ostream &os, std::istream &)
{
	auto information = chaine_caractere::informations_texte("", 0, "");

	os << "La phrase vide a " << information.nombre_mots << " mots\n";

	auto phrase = "Deux mots";

	information = chaine_caractere::informations_texte(phrase, 0, "");

	os << "La phrase '" << phrase << "' a " << information.nombre_mots << " mots\n";
}

/* ************************************************************************** */

int main(int /*argc*/, char **/*argv*/)
{
	const int i = 10;

	switch (i) {
		case 0:
			test_to_words(std::cout, std::cin);
			break;
		case 1:
			test_postfix(std::cout, std::cin);
			break;
		case 3:
			test_hangman(std::cout, std::cin);
			break;
		case 4:
			test_prm(std::cout, std::cin);
			break;
		case 5:
			test_range(std::cout, std::cin);
			break;
		case 6:
			test_dette(std::cout, std::cin);
			break;
		case 7:
			test_adn(std::cout, std::cin);
			break;
		case 8:
			test_constexpr_if(std::cout, std::cin);
			break;
		case 9:
			test_algorithm_glouton(std::cout, std::cin);
			break;
		case 10:
			test_nombre_mot(std::cout, std::cin);
			break;
	}
}
