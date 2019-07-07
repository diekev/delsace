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

#include <any>

#include "biblinternes/phys/couleur.hh"
#include "biblinternes/math/vecteur.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/pile.hh"

#include "donnees_type.h"
#include "execution_pile.hh"
#include "fonctions.hh"
#include "tampon_source.h"

class assembleuse_arbre;

struct DonneesModule;

struct compileuse_lng {
	template <typename T>
	void ajoute_instructions(T inst)
	{
		m_instructions.pousse(inst);
	}

	lcc::pile &donnees()
	{
		return m_donnees;
	}

	lcc::pile const &donnees() const
	{
		return m_donnees;
	}

	lcc::pile &instructions()
	{
		return m_instructions;
	}

	lcc::pile const &instructions() const
	{
		return m_instructions;
	}

private:
	lcc::pile m_donnees{};
	lcc::pile m_instructions{};
};

/* ************************************************************************** */

struct Metriques {
	size_t nombre_modules = 0ul;
	size_t nombre_lignes = 0ul;
	size_t nombre_morceaux = 0ul;
	size_t memoire_tampons = 0ul;
	size_t memoire_morceaux = 0ul;
	double temps_chargement = 0.0;
	double temps_analyse = 0.0;
	double temps_tampon = 0.0;
	double temps_decoupage = 0.0;
	double temps_validation = 0.0;
	double temps_generation = 0.0;
};

/* ************************************************************************** */

struct donnees_propriete {
	dls::chaine nom = "";
	lcc::type_var type{};
	bool est_requis = false;
	bool pad = false;
	int ptr = 0;
	std::any ptr_donnees = nullptr;

	donnees_propriete(
			dls::chaine const &_nom_,
			lcc::type_var _type_,
			bool _est_requis_,
			int _ptr_)
		: nom(_nom_)
		, type(_type_)
		, est_requis(_est_requis_)
	    , ptr(_ptr_)
	{}
};

struct gestionnaire_propriete {
	dls::tableau<donnees_propriete *> donnees;
	dls::tableau<donnees_propriete *> requetes;

	~gestionnaire_propriete()
	{
		for (auto &d : donnees) {
			memoire::deloge("donnees_propriete", d);
		}
	}

	void ajoute_propriete(dls::chaine const &nom, lcc::type_var type, int idx)
	{
		donnees.pousse(memoire::loge<donnees_propriete>("donnees_propriete", nom, type, false, idx));
	}

	void requiers_attr(dls::chaine const &nom, lcc::type_var type, int idx)
	{
		auto prop = memoire::loge<donnees_propriete>("donnees_propriete", nom, type, true, idx);
		donnees.pousse(prop);
		requetes.pousse(prop);
	}

	bool propriete_existe(dls::vue_chaine const &nom)
	{
		for (auto const &donnee : donnees) {
			if (donnee->nom == nom) {
				return true;
			}
		}

		return false;
	}

	lcc::type_var type_propriete(dls::vue_chaine const &nom)
	{
		for (auto const &donnee : donnees) {
			if (donnee->nom == nom) {
				return donnee->type;
			}
		}

		return lcc::type_var::INVALIDE;
	}

	int pointeur_donnees(dls::vue_chaine const &nom) const
	{
		for (auto &donnee : donnees) {
			if (donnee->nom == nom) {
				return donnee->ptr;
			}
		}

		return -1;
	}
};

/* ************************************************************************** */

struct donnees_variables {
	int type{};
	lcc::type_var donnees_type{};
};

using conteneur_locales = dls::tableau<std::pair<dls::vue_chaine, donnees_variables>>;

struct ContexteGenerationCode {
	assembleuse_arbre *assembleuse = nullptr;

	dls::tableau<DonneesModule *> modules{};

	lcc::magasin_fonctions fonctions{};

	gestionnaire_propriete gest_props{};
	gestionnaire_propriete gest_attrs{};

	ContexteGenerationCode() = default;

	~ContexteGenerationCode();

	/* ********************************************************************** */

	/* Désactive la copie, car il ne peut y avoir qu'un seul contexte par
	 * compilation. */
	ContexteGenerationCode(const ContexteGenerationCode &) = delete;
	ContexteGenerationCode &operator=(const ContexteGenerationCode &) = delete;

	/* ********************************************************************** */

	/**
	 * Crée un module avec le nom spécifié, et retourne un pointeur vers le
	 * module ainsi créé. Aucune vérification n'est faite quant à la présence
	 * d'un module avec un nom similaire pour l'instant.
	 */
	DonneesModule *cree_module(const dls::chaine &nom);

	/**
	 * Retourne un pointeur vers le module à l'index indiqué. Si l'index est
	 * en dehors de portée, le programme crashera.
	 */
	DonneesModule *module(size_t index) const;

	/**
	 * Retourne un pointeur vers le module dont le nom est spécifié. Si aucun
	 * module n'a ce nom, retourne nullptr.
	 */
	DonneesModule *module(const dls::vue_chaine &nom) const;

	/**
	 * Retourne vrai si le module dont le nom est spécifié existe dans la liste
	 * de module de ce contexte.
	 */
	bool module_existe(const dls::vue_chaine &nom) const;

	/* ********************************************************************** */

	void pousse_locale(dls::vue_chaine const &nom, int valeur, lcc::type_var donnees_type);

	int valeur_locale(dls::vue_chaine const &nom);

	lcc::type_var donnees_type(dls::vue_chaine const &nom);

	/**
	 * Retourne vrai s'il existe une locale dont le nom correspond au spécifié.
	 */
	bool locale_existe(const dls::vue_chaine &nom);

	/**
	 * Retourne les données de la locale dont le nom est spécifié en paramètre.
	 * Si aucune locale ne portant ce nom n'existe, des données vides sont
	 * retournées.
	 */
	size_t type_locale(const dls::vue_chaine &nom);

	/**
	 * Retourne vrai si la variable locale dont le nom est spécifié peut être
	 * assignée.
	 */
	bool peut_etre_assigne(const dls::vue_chaine &nom);

	/**
	 * Indique que l'on débute un nouveau bloc dans la fonction, et donc nous
	 * enregistrons le nombre de variables jusqu'ici. Voir 'pop_bloc_locales'
	 * pour la gestion des variables.
	 */
	void empile_nombre_locales();

	/**
	 * Indique que nous sortons d'un bloc, donc toutes les variables du bloc
	 * sont 'effacées'. En fait les variables des fonctions sont tenues dans un
	 * vecteur. À chaque fois que l'on rencontre un bloc, on pousse le nombre de
	 * variables sur une pile, ainsi toutes les variables se trouvant entre
	 * l'index de début du bloc et l'index de fin d'un bloc sont des variables
	 * locales à ce bloc. Quand on sort du bloc, l'index de fin des variables
	 * est réinitialisé pour être égal à ce qu'il était avant le début du bloc.
	 * Ainsi, nous pouvons réutilisé la mémoire du vecteur de variable d'un bloc
	 * à l'autre, d'une fonction à l'autre, tout en faisant en sorte que peut
	 * importe le bloc dans lequel nous nous trouvons, on a accès à toutes les
	 * variables jusqu'ici déclarées.
	 */
	void depile_nombre_locales();

	/**
	 * Imprime le nom des variables locales dans le flux précisé.
	 */
	void imprime_locales(std::ostream &os);

	/**
	 * Retourne vrai si la variable est un argument variadic. Autrement,
	 * retourne faux.
	 */
	bool est_locale_variadique(const dls::vue_chaine &nom);

	conteneur_locales::iteratrice iter_locale(const dls::vue_chaine &nom);

	conteneur_locales::iteratrice fin_locales();

	/* ********************************************************************** */

	size_t memoire_utilisee() const;

	/**
	 * Retourne les métriques de ce contexte. Les métriques sont calculées à
	 * chaque appel à cette fonction, et une structure neuve est retournée à
	 * chaque fois.
	 */
	Metriques rassemble_metriques() const;

	/* ********************************************************************** */

	/**
	 * Définie si oui ou non le contexte est non-sûr, c'est-à-dire que l'on peut
	 * manipuler des objets dangereux.
	 */
	void non_sur(bool ouinon);

	/**
	 * Retourne si oui ou non le contexte est non-sûr.
	 */
	bool non_sur() const;

private:
	conteneur_locales m_locales{};
	dls::pile<size_t> m_pile_nombre_locales{};
	size_t m_nombre_locales = 0;

	bool m_non_sur = false;

public:
	/* À FAIRE : bouge ça d'ici. */
	double temps_validation = 0.0;
	double temps_generation = 0.0;
};
