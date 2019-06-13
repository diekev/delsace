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

#pragma once

#include <experimental/filesystem>
#include <unordered_map>

namespace arachne {

/**
 * La struct magasin_chaine offre le stockage et l'indexage de chaîne de
 * caractère de sorte que chaque chaîne ne soit stockée et indexée qu'une seule
 * fois.
 */
struct magasin_chaine {
	std::unordered_map<std::string, int> m_tableau = {};
	unsigned int m_nombre_chaines = 0;
	size_t m_taille_chaines = 0ul;

public:
	magasin_chaine() = default;
	~magasin_chaine() = default;

	using iterateur = std::unordered_map<std::string, int>::iterator;
	using iterateur_const = std::unordered_map<std::string, int>::const_iterator;

	/**
	 * Ajoute la chaîne spécifiée en paramètre dans le magasin si elle n'y est
	 * pas déjà et retourne son index.
	 */
	unsigned int ajoute_chaine(const std::string &chaine);

	/**
	 * Retourne l'index de la chaîne spécifiée. Si la chaîne n'a pas été
	 * indexée, retourne 0.
	 */
	unsigned int index_chaine(const std::string &chaine) const;

	/**
	 * Retourne vrai si le nombre de chaîne dans l'index du magasin est égal au
	 * nombre unique de chaîne qui lui a été passé.
	 */
	bool est_valide() const;

	/**
	 * Retourne le nombre de chaînes dans le magasin.
	 */
	size_t taille() const;

	/**
	 * Retourne la taille concaténée de toutes les chaînes dans le magasin.
	 */
	size_t taille_chaines() const;

	/**
	 * Retourne un itérateur pointant vers le début du tableau de chaînes du
	 * magasin.
	 */
	iterateur debut();

	/**
	 * Retourne un itérateur pointant vers la fin du tableau de chaînes du
	 * magasin.
	 */
	iterateur fin();

	/**
	 * Retourne un itérateur pointant vers le début du tableau de chaînes du
	 * magasin.
	 */
	iterateur begin();

	/**
	 * Retourne un itérateur pointant vers la fin du tableau de chaînes du
	 * magasin.
	 */
	iterateur end();

	/**
	 * Retourne un itérateur constant pointant vers le début du tableau de
	 * chaînes du magasin.
	 */
	iterateur_const begin() const;

	/**
	 * Retourne un itérateur constant pointant vers la fin du tableau de chaînes
	 * du magasin.
	 */
	iterateur_const end() const;
};

/**
 * Écris le contenu du magasin de chaînes spécifié dans le fichier pointé par
 * le chemin donné.
 */
void ecris_magasin_chaine(
		const magasin_chaine &magasin,
		const std::experimental::filesystem::path &chemin);

/**
 * Écris le contenu d'un magasin de chaînes depuis le fichier pointé par le
 * chemin donné.
 */
void lis_magasin_chaine(const std::experimental::filesystem::path &chemin);

}  /* namespace arachne */
