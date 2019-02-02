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

#pragma once

#include <any>
#include <string>
#include <vector>

#include "bibliotheques/geometrie/rectangle.h"
#include "bibliotheques/outils/iterateurs.h"

class Noeud;
struct PriseEntree;
struct PriseSortie;

enum type_prise {
	DECIMAL = 0,
	ENTIER  = 1,
	VECTEUR = 2,
	COULEUR = 3,
	IMAGE   = 4,
	PIXEL   = 5,
	CORPS   = 6,
	OBJET   = 7,
	CAMERA  = 8,
	SCENE   = 9,
};

/* ************************************************************************** */

struct PriseSortie {
	Noeud *parent = nullptr;
	std::vector<PriseEntree *> liens{};
	std::string nom = "";
	int type = 0;

	/* décalage dans la pile d'une CompileuseGraphe */
	size_t decalage_pile = 0;

	/* position et taille dans l'entreface */
	Rectangle rectangle{};

	explicit PriseSortie(std::string const &nom_prise);

	/* À FAIRE : considère l'utilisation de shared_ptr */
	PriseSortie(PriseSortie const &) = default;
	PriseSortie &operator=(PriseSortie const &) = default;
};

/* ************************************************************************** */

struct PriseEntree {
	Noeud *parent = nullptr;
	PriseSortie *lien = nullptr;
	std::string nom = "";
	int type = 0;

	/* position et taille dans l'entreface */
	Rectangle rectangle{};

	explicit PriseEntree(std::string const &nom_prise);

	/* À FAIRE : considère l'utilisation de shared_ptr */
	PriseEntree(PriseEntree const &) = default;
	PriseEntree &operator=(PriseEntree const &) = default;
};

/* ************************************************************************** */

class Noeud {
	void (*supprime_donnees)(std::any) = nullptr;

	std::vector<PriseEntree *> m_entrees = {};
	std::vector<PriseSortie *> m_sorties = {};

	std::any m_donnees = nullptr;

	std::string m_nom = "";

	/* Interface utilisateur. */
	Rectangle m_rectangle{};

	int m_type = 0;
	bool m_besoin_traitement = true;

	float m_temps_execution{0.0f};

	float m_temps_execution_min{std::numeric_limits<float>::max()};

	int m_executions = 0;

public:
	Noeud() = default;
	explicit Noeud(void (*suppression_donnees)(std::any));
	~Noeud();

	/* À FAIRE : considère l'utilisation de shared_ptr */
	Noeud(Noeud const &) = default;
	Noeud &operator=(Noeud const &) = default;

	using plage_entrees = plage_iterable<std::vector<PriseEntree *>::const_iterator>;
	using plage_sorties = plage_iterable<std::vector<PriseSortie *>::const_iterator>;

	std::any donnees() const;
	void donnees(std::any pointeur);

	std::string const &nom() const;
	void nom(std::string const &nom);

	/**
	 * Retourne la position horizontale du noeud dans l'éditeur.
	 */
	double pos_x() const;

	/**
	 * Ajourne la position horizontale du noeud dans l'éditeur.
	 */
	void pos_x(float x);

	/**
	 * Retourne la position verticale du noeud dans l'éditeur.
	 */
	double pos_y() const;

	/**
	 * Ajourne la position verticale du noeud dans l'éditeur.
	 */
	void pos_y(float y);

	/**
	 * Retourne la hauteur du noeud dans l'éditeur.
	 */
	int hauteur() const;

	/**
	 * Ajourne la hauteur du noeud dans l'éditeur.
	 */
	void hauteur(int h);

	/**
	 * Retourne la largeur du noeud dans l'éditeur.
	 */
	int largeur() const;

	/**
	 * Ajourne la largeur du noeud dans l'éditeur.
	 */
	void largeur(int l);

	/**
	 * Retroune le rectangle englobant ce noeud.
	 */
	Rectangle const &rectangle() const;

	/**
	 * Ajoute une prise d'entrée au noeud.
	 */
	void ajoute_entree(std::string const &nom, const int type);

	/**
	 * Ajoute une prise de sortie au noeud.
	 */
	void ajoute_sortie(std::string const &nom, const int type);

	/**
	 * Retourne l'entrée selon l'index spécifié.
	 */
	PriseEntree *entree(size_t index);

	/**
	 * Retourne l'entrée selon le nom spécifié.
	 */
	PriseEntree *entree(std::string const &nom);

	/**
	 * Retourne la sortie selon l'index spécifié.
	 */
	PriseSortie *sortie(size_t index);

	/**
	 * Retourne la sortie selon le nom spécifié.
	 */
	PriseSortie *sortie(std::string const &nom);

	/**
	 * Retourne une plage itérable sur les entrées de ce noeud.
	 */
	plage_entrees entrees() const noexcept;

	/**
	 * Retourne une plage itérable sur les sorties de ce noeud.
	 */
	plage_sorties sorties() const noexcept;

	/**
	 * Retourne vrai si le noeud est lié à d'autres noeuds.
	 */
	bool est_lie() const;

	/**
	 * Retourne vrai si le noeud est lié par une de ses entrées à d'autres noeuds.
	 */
	bool a_des_entrees_liees() const;

	/**
	 * Retourne vrai si le noeud est lié par une de ses sorties à d'autres noeuds.
	 */
	bool a_des_sorties_liees() const;

	/**
	 * Retourne le type du noeud.
	 */
	int type() const;

	/**
	 * Renseigne le type du noeud.
	 */
	void type(int what);

	/**
	 * Retourne si oui ou non le noeud a besoin d'être exécuté.
	 */
	bool besoin_execution() const;

	/**
	 * Renseigne si oui ou non le noeud a besoin d'être exécuté.
	 */
	void besoin_execution(bool ouinon);

	/**
	 * Retourne le temps qu'il fallut pour exécuter l'opérateur du noeud.
	 */
	float temps_execution() const;

	/**
	 * Retourne le temps minimum qu'il fallut pour exécuter l'opérateur du noeud.
	 */
	float temps_execution_minimum() const;

	/**
	 * Renseigne le temps qu'il fallut pour exécuter l'opérateur du noeud.
	 */
	void temps_execution(float time);

	/**
	 * Incrémente le nombre d'exécutions de l'opératrice de noeud.
	 */
	void incremente_compte_execution();

	/**
	 * Retourne le nombre d'exécutions de l'opératrice de ce noeud.
	 */
	int compte_execution() const;

	int degre = 0;
};

void marque_surannee(Noeud *noeud);
