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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <any>

#include "biblinternes/math/rectangle.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/iterateurs.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

#include "danjo/manipulable.h"

#include "graphe.hh"

struct Noeud;
struct PriseEntree;
struct PriseSortie;

enum class type_prise : int {
	DECIMAL,
	ENTIER,
	VEC2,
	VEC3,
	VEC4,
	MAT3,
	MAT4,
	COULEUR,
	CORPS,
	IMAGE,
	OBJET,
	TABLEAU,
	POLYMORPHIQUE,
	CHAINE,
	INVALIDE,
};

/* ************************************************************************** */

struct PriseSortie {
	Noeud *parent = nullptr;
	dls::tableau<PriseEntree *> liens{};
	dls::chaine nom = "";
	type_prise type{};

	/* inférence de type pour les noeuds dont les entrées sont polymorphiques */
	type_prise type_infere{};

	/* décalage dans la pile d'une CompileuseGraphe */
	long decalage_pile = 0;

	/* position et taille dans l'entreface */
	Rectangle rectangle{};

	explicit PriseSortie(dls::chaine const &nom_prise);

	COPIE_CONSTRUCT(PriseSortie);
};

/* ************************************************************************** */

struct PriseEntree {
	Noeud *parent = nullptr;
	dls::tableau<PriseSortie *> liens{};
	//PriseSortie *lien = nullptr;
	dls::chaine nom = "";
	type_prise type{};
	bool multiple_connexions = false;

	/* position et taille dans l'entreface */
	Rectangle rectangle{};

	explicit PriseEntree(dls::chaine const &nom_prise);

	COPIE_CONSTRUCT(PriseEntree);
};

/* ************************************************************************** */

enum class type_noeud : int {
	COMPOSITE,
	OBJET,
	OPERATRICE,
	NUANCEUR,
	RENDU,
	INVALIDE,
};

struct Noeud : public danjo::Manipulable {
	dls::chaine nom = "";
	type_noeud type = type_noeud::INVALIDE;

	dls::tableau<PriseEntree *> entrees = {};
	dls::tableau<PriseSortie *> sorties = {};

	std::any donnees = nullptr;

	/* exécutions */
	bool besoin_execution = true;
	float temps_execution = 0.0f;
	int executions = 0;

	/* interface */
	Rectangle rectangle{};

	/* autres */
	int degree = 0; /* pour les algorithmes de tri topologique entre autre */

	bool est_sortie = false;
	bool peut_avoir_graphe = false;

	Noeud *parent = nullptr;
	dls::tableau<Noeud *> enfants{};

	/* NOTE : le graphe doit être libéré avant les enfants, donc il doit être
	 * situé dans la structure après les enfants puisque C++ utilise une pile
	 * pour appeler les destructeurs. */
	Graphe graphe;

	/* constrution/destruction */

	Noeud();

	~Noeud();

	COPIE_CONSTRUCT(Noeud);

	/* accès/informations */

	/**
	 * Retourne la position horizontale du noeud dans l'éditeur.
	 */
	float pos_x() const;

	/**
	 * Ajourne la position horizontale du noeud dans l'éditeur.
	 */
	void pos_x(float x);

	/**
	 * Retourne la position verticale du noeud dans l'éditeur.
	 */
	float pos_y() const;

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
	 * Ajoute une prise d'entrée au noeud.
	 */
	void ajoute_entree(dls::chaine const &nom, const type_prise type_p, bool connexions_multiples);

	/**
	 * Ajoute une prise de sortie au noeud.
	 */
	void ajoute_sortie(dls::chaine const &nom, const type_prise type_p);

	/**
	 * Retourne l'entrée selon l'index spécifié.
	 */
	PriseEntree *entree(long index) const;

	/**
	 * Retourne l'entrée selon le nom spécifié.
	 */
	PriseEntree *entree(dls::chaine const &nom_entree) const;

	/**
	 * Retourne la sortie selon l'index spécifié.
	 */
	PriseSortie *sortie(long index) const;

	/**
	 * Retourne la sortie selon le nom spécifié.
	 */
	PriseSortie *sortie(dls::chaine const &nom_sortie) const;

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

	dls::chaine chemin() const;
};

/* ************************************************************************** */

void marque_surannee(Noeud *noeud, std::function<void(Noeud *, PriseEntree *)> const &rp);

void marque_parent_surannee(Noeud *noeud, std::function<void(Noeud *, PriseEntree *)> const &rp);
