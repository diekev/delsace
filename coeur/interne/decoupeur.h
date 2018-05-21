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

namespace danjo {

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
	const char *m_debut = nullptr;
	const char *m_fin = nullptr;

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
	void ajoute_identifiant(int identifiant, const std::string &contenu);

	/**
	 * Retourne la liste de morceaux découpés.
	 */
	const std::vector<DonneesMorceaux> &morceaux() const;

	void avance(int compte = 1);
private:
	/**
	 * Imprime un simple message de débogage contenant la ligne, la position, et
	 * le mot courant passé via le paramètre 'quoi'.
	 */
	/* cppcheck-suppress unusedPrivateFunction */
	void impression_debogage(const std::string &quoi);
};

}  /* namespace danjo */
