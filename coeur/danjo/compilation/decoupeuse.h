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
#include "tampon_source.h"

namespace danjo {

/**
 * La classe Decoupeuse découpe une chaîne de caractère en morceaux selon les
 * mots-clés du langage.
 */
class Decoupeuse {
	size_t m_position{0};
	size_t m_position_ligne{0};
	size_t m_pos_mot{0};
	size_t m_ligne{0};
	size_t m_taille_mot_courant{0};
	bool m_fini{false};
	char m_caractere_courant{'\0'};
	const char *m_debut = nullptr;
	const char *m_debut_mot = nullptr;
	const char *m_fin = nullptr;

	std::vector<DonneesMorceaux> m_identifiants{};
	const TamponSource &m_tampon;

public:
	using iterateur = std::vector<DonneesMorceaux>::iterator;
	using iterateur_const = std::vector<DonneesMorceaux>::const_iterator;

	/**
	 * Construit une instance de Decoupeuse pour la chaîne spécifiée.
	 */
	explicit Decoupeuse(const TamponSource &tampon);

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
	void ajoute_identifiant(id_morceau identifiant, const std::string_view &contenu);

	/**
	 * Retourne la liste de morceaux découpés.
	 */
	const std::vector<DonneesMorceaux> &morceaux() const;

	/**
	 * Retourne un itérateur pointant vers le début de la liste d'identifiants.
	 */
	iterateur begin();

	/**
	 * Retourne un itérateur pointant vers la fin de la liste d'identifiants.
	 */
	iterateur end();

	/**
	 * Retourne un itérateur constant pointant vers le début de la liste
	 * d'identifiants.
	 */
	iterateur_const cbegin() const;

	/**
	 * Retourne un itérateur constant pointant vers la fin de la liste
	 * d'identifiants.
	 */
	iterateur_const cend() const;

private:
	/**
	 * Imprime un simple message de débogage contenant la ligne, la position, et
	 * le mot courant passé via le paramètre 'quoi'.
	 */
	/* cppcheck-suppress unusedPrivateFunction */
	void impression_debogage(const std::string &quoi);

	/**
	 * Avance le curseur sur le tampon du nombre de crans spécifié en paramètre.
	 */
	void avance(size_t compte = 1);

	/**
	 * Réinitialise la taille du mot courant, et copie la valeur du curseur
	 * courant vers le pointeur du début du mot courant.
	 */
	void enregistre_pos_mot();

	/**
	 * Ajoute le mot_courant() à la liste de morceaux de cette découpeuse en le
	 * couplant à l'identifiant spécifié.
	 */
	void pousse_mot(id_morceau identifiant);

	/**
	 * Incrémente la taille du mot courant.
	 */
	void pousse_caractere();

	/**
	 * Performe l'analyse d'un caractère UTF-8 encodé sur un seul octet. Ceci
	 * est la logique principale de découpage du tampon source.
	 */
	void analyse_caractere_simple();

	/**
	 * Retourne vrai si le tampon source a été consommé jusqu'au bout.
	 */
	[[nodiscard]] bool fini() const;

	/**
	 * Retourne le caractère couramment pointé par le curseur sur le tampon.
	 */
	[[nodiscard]] char caractere_courant() const;

	/**
	 * Retourne le caractère se trouvrant à la position (courante + n). Si le
	 * caractère à la position (courante + n) est hors de portée, retourne le
	 * caractère le plus proche de la position (courante + n).
	 */
	[[nodiscard]] char caractere_voisin(int n = 1) const;

	/**
	 * Retourne une string_view sur le mot dont le début à été marqué par le
	 * dernier appel à enregistre_pos_mot(), et dont la taille correspond au
	 * nombre d'appel à pousse_caractère() depuis l'enregistrement de la
	 * position.
	 */
	[[nodiscard]] std::string_view mot_courant() const;

	/**
	 * Lance une erreur dont le message correspond au paramètre 'quoi'.
	 */
	[[noreturn]] void lance_erreur(const std::string &quoi) const;
};

}  /* namespace danjo */
