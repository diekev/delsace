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

#include <vector>

#include "morceaux.h"

class Analyseur;

/**
 * La classe Decoupeur découpe une chaîne de caractère en morceaux selon les
 * mots-clés du langage.
 */
class Decoupeur {
	std::string_view m_chaine;
	int m_position;
	int m_position_ligne;
	int m_ligne;
	bool m_fini;
	char m_caractere_courant;

	std::vector<std::string_view> m_lignes;
	std::vector<DonneesMorceaux> m_identifiants{};

public:
	/**
	 * Construit une instance de Decoupeur pour la chaîne spécifiée.
	 */
	explicit Decoupeur(const std::string_view &chaine);

	/**
	 * Lance la découpe de la chaîne de caractère spécifiée en paramètre du
	 * constructeur.
	 *
	 * La découpe s'effectue en coupant la chaîne en paire identifiant-contenu,
	 * paire qui est passée à l'analyseur syntactique. Si une erreur de frappe
	 * est détectée, une exception de type ErreurFrappe est lancée.
	 *
	 * À FAIRE : passage de la chaîne de caractère à cette méthode.
	 */
	void decoupe();

	/**
	 * Ajoute un identifiant au vecteur d'identifiants avec les données passées
	 * en paramètres.
	 */
	void ajoute_identifiant(int identifiant, const std::string_view &ligne, int numero_ligne, int position_ligne, const std::string &contenu);

	/**
	 * Retourne la liste de morceaux découpés.
	 */
	const std::vector<DonneesMorceaux> &morceaux() const;

private:
	/**
	 * Imprime un simple message de débogage contenant la ligne, la position, et
	 * le mot courant passé via le paramètre 'quoi'.
	 */
	/* cppcheck-suppress unusedPrivateFunction */
	void impression_debogage(const std::string &quoi);

	/**
	 * Avance la position du découpeur sur la chaîne de caractère tant que le
	 * caractère courant est un espace blanc ('\\n', '\\t', ' ').
	 *
	 * Si un caractère de nouvelle ligne apparaît, ajoute la ligne courante à la
	 * liste de lignes de la chaîne de caractère pour pouvoir l'imprimer en cas
	 * d'erreur.
	 */
	void saute_espaces_blancs();

	/**
	 * Retourne le caratère courant et avance la position du découpeur sur la
	 * chaîne de caractère.
	 */
	char caractere_suivant();

	/**
	 * Retourne le caractère courant sans modifier l'état du découpeur.
	 */
	char caractere_courant();

	/**
	 * Isole le contenu se trouvant entre deux guillemets (") et retourne-le
	 * sous forme de std::string.
	 */
	std::string decoupe_chaine_litterale();
};
