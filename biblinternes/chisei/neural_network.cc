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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <cmath>
#include <fstream>
#include <random>
#include <sstream>

#include "biblinternes/structures/tableau.hh"

#include "includes.h"

#include "entraineur_cerebral.h"
#include "reseau_neuronal.h"

/* ************************************************************************** */

#include "iomanip"

/**
 * Some guidelines:
 *  - normalise data (e.g. set { 38, 51.000, M, Pres } -> { 3.8, 5.1, -1.0, { 0.0, 1.0, 0.0 } })
 *  - if a piece of data can only have two values, make them -1.0 and 1.0 instead of 0.0 and 1.0
 *
 */

static constexpr auto SAMPLE_SIZE = 120;
static constexpr auto NUM_INPUT = 4;
static constexpr auto NUM_PERCEPTRON = 7;
static constexpr auto NUM_OUTPUT = 3;

struct NeuralNetwork {
	dls::tableau<dls::tableau<double>> inputs = {};
	dls::tableau<dls::tableau<double>> outputs = {};

	double input[NUM_INPUT];
	double target_output[NUM_OUTPUT];

	double hidden_layer[NUM_PERCEPTRON][NUM_INPUT];
	double hidden_layer_bias[NUM_INPUT];
	double hidden_layer_result[NUM_PERCEPTRON];

	double output_layer_weight[NUM_OUTPUT][NUM_PERCEPTRON];
	double output_layer_result[NUM_OUTPUT];
};

template <typename T>
static inline void print_vec(std::ostream &os, const dls::tableau<T> &v)
{
	for (const auto value : v) {
		os << value << ' ';
	}
}

void test_neural_net(std::ostream &os)
{
	auto path = "data/iris_train.dat";
	std::ifstream in(path);

	if (in.is_open()) {
		os << "File '" << path << "' is open!\n";
	}
	else {
		os << "File '" << path << "' is not open!\n";
	}

	NeuralNetwork network;
	/* -------------------------- read data -------------------------- */

	os << '\n';
	os << "Reading training data....\n";
	os << '\n';

	os << std::fixed << std::setprecision(2) << std::setfill('0');

	network.inputs.redimensionne(SAMPLE_SIZE);
	network.outputs.redimensionne(SAMPLE_SIZE);

	for (dls::tableau<double> &input : network.inputs) {
		input.redimensionne(NUM_INPUT);
	}

	for (dls::tableau<double> &output : network.outputs) {
		output.redimensionne(NUM_OUTPUT);
	}

	auto index = 0l;
	std::string line;

	while (std::getline(in, line)) {
		auto &input = network.inputs[index];
		auto &output = network.outputs[index];
		++index;

		std::stringstream ss(line);

		auto temp = 0.0;

		for (double &value : input) {
			ss >> temp;
			value = temp;
		}

		for (double &value : output) {
			ss >> temp;
			value = temp;
		}
	}

	os << "First 6 rows of training data:\n";

	for (auto i = 0l; i < 6; ++i) {
		os << "  " << i << ": ";
		print_vec(os, network.inputs[i]);
		os << '\n';
	}

	/* -------------------------- normalise data -------------------------- */

	os << '\n';
	os << "Normalising training data....\n";
	os << '\n';

	/* HOW-TO:
	 * - for each column, compute the mean
	 * - for each value x, the nomalised one is ((x - mean) / standard_deviation)
	 */

	/* compute means */
	double mean[NUM_INPUT] = { 0.0, 0.0, 0.0, 0.0 };

	for (const dls::tableau<double> &input : network.inputs) {
		for (auto i = 0l; i < NUM_INPUT; ++i) {
			mean[i] += input[i];
		}
	}

	const auto &size = network.inputs.taille();
	for (int i = 0; i < NUM_INPUT; ++i) {
		mean[i] /= static_cast<double>(size);
	}

	/* compute standard deviations */
	double stddev[NUM_INPUT] = { 0.0, 0.0, 0.0, 0.0 };

	auto sqr = [](double x) { return x * x; };

	for (const dls::tableau<double> &input : network.inputs) {
		for (auto i = 0l; i < NUM_INPUT; ++i) {
			stddev[i] += sqr(input[i] - mean[i]);
		}
	}

	for (auto i = 0l; i < NUM_INPUT; ++i) {
		stddev[i] /= static_cast<double>(size);
	}

	/* normalise */

	for (dls::tableau<double> &input : network.inputs) {
		for (auto i = 0l; i < NUM_INPUT; ++i) {
			input[i] = (input[i] - mean[i]) / stddev[i];
		}
	}

	os << "First 6 rows of normalised training data:\n";

	for (auto i = 0l; i < 6; ++i) {
		os << "  " << i << ": ";
		print_vec(os, network.inputs[i]);
		os << '\n';
	}

	/* Copy input */
	for (auto i = 0l; i < NUM_INPUT; ++i) {
		network.input[i] = network.inputs[0][i];
	}

	for (auto i = 0l; i < NUM_OUTPUT; ++i) {
		network.target_output[i] = network.outputs[0][i];
	}

#if 0
	ReseauNeuronal reseau_neuronal(NUM_INPUT, NUM_PERCEPTRON, NUM_OUTPUT);

	dls::tableau<DonneeFormation> donnees;

	for (size_t i = 0; i < 120; ++i) {
		DonneeFormation donnee;
		donnee.entrees = network.inputs[i];
		donnee.desirees = network.outputs[i];

		donnees.pousse(donnee);
	}

	EntraineurCerebral entraineur(reseau_neuronal);
	entraineur.execute_apprentissage(donnees);

	for (dls::tableau<double> &input : network.inputs) {
		auto output = reseau_neuronal.avance_entrees(input.data());

		os << "Input:  ";
		for (auto i = 0ul; i < NUM_INPUT; ++i) {
			os << input[i] << ' ';
		}
		os << '\n';

		os << "Output:  ";
		for (int i = 0; i < NUM_OUTPUT; ++i) {
			os << output[i] << ' ';
		}
		os << '\n';
	}
#endif
}
