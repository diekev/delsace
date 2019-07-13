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

#include "divers.h"
#include "string_utils.h"

#include <iostream>
#include <random>

dls::chaine words[] = {
    "apopathodiaphulatophobe", // 23
    "chemin",  // 6
    "deliquescence", // 13
    "diatribe",  // 8
    "disparate", // 9
    "entrainement", // 12
    "inverser",  // 8
    "masque",  // 6
    "matinal",  // 7
    "relatif",  // 7
    "transformation", //14
    "utiliser",  // 8
};

auto find_best_match(const dls::chaine &to_guess)
{
	dls::chaine best_match, tmp_match;
	auto prev_match = 0;

	for (const auto &word : words) {
		auto match = compte_commun(to_guess, word);

		if (match > prev_match) {
			prev_match = match;
			best_match = tmp_match;
		}
	}

	return best_match;
}

auto hangman_player_human(std::istream &is, std::ostream &os)
{
	const auto num_words = std::end(words) - std::begin(words);

	std::random_device rd;
	std::uniform_int_distribution<int> dist(0, num_words - 1);

	const auto &word = words[dist(rd)];
	dls::chaine tmp(word.taille(), '_');

	int max_guesses = 10;
	char letter;
	dls::chaine guess_word;

	os << tmp << '\n';

	while (max_guesses) {
		os << "Pick a letter: ";
		is >> letter;

		auto index = word.trouve(letter);
		if (index == dls::chaine::npos) {
			os << "Letter not present in word!\n";
			--max_guesses;
			continue;
		}

		tmp[index] = letter;

		while ((index = word.trouve(letter, index + 1)) != dls::chaine::npos)
			tmp[index] = letter;

		if (compte(tmp, '_') == 0) {
			os << "Congratulation! You guessed correctly!\n";
			os << tmp << '\n';
			break;
		}

		os << tmp << '\n';

		os << "Guess a word? (y/n): ";
		is >> guess_word;

		if (guess_word == "n") {
			continue;
		}
		else if (guess_word == "y") {
			os << "Enter guess: ";
			is >> guess_word;

			if (guess_word == word) {
				os << "Congratulation! You guessed correctly!\n";
				os << tmp << '\n';
				break;
			}
			else {
				os << "Wrong guess...\n";
			}
		}
	}
}

auto hangman_player_computer(std::istream &is, std::ostream &os)
{
	std::random_device rng;
	std::uniform_int_distribution<char> rnd_letter(97, 122);

	os << "How many letters are there? ";
	int letters;
	is >> letters;

	dls::chaine word(letters, '_');
	bool available[26];
	std::fill(std::begin(available), std::end(available), true);

	char guess;
	auto max_guesses = 11;

	while (max_guesses--) {
		guess = rnd_letter(rng);
		while (!available[(guess - 'a') - 1]) {
			guess = rnd_letter(rng);
		}

		available[(guess - 'a') - 1] = false;

		os << "Is there a " << guess << "? (y/n) ";
		char yesno;
		is >> yesno;

		if (yesno != 'y') {
			continue;
		}

		os << "Please enter the letters:\n" << word << '\n';
		is >> word;

		auto word_guess = find_best_match(word);
		os << "Is it: " << word_guess << "? (y/n): ";
		is >> word_guess;

		if (word_guess == "y") {
			break;
		}
	}
}

auto hangman(std::ostream &os, std::istream &is, hangman_player player) -> void
{
	switch (player) {
		case hangman_player::computer:
			hangman_player_computer(is, os);
			break;
		case hangman_player::human:
			hangman_player_human(is, os);
			break;
	}
}
