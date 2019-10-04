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

#include "biblinternes/structures/pile.hh"

#include "execution_pile.hh"
#include "fonctions.hh"

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
	dls::tableau<donnees_propriete *> donnees{};
	dls::tableau<donnees_propriete *> requetes{};

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

template <typename T>
auto remplis_donnees(
		lcc::pile &donnees,
		gestionnaire_propriete &gest_props,
		dls::chaine const &nom,
		T const &v)
{
	auto idx = gest_props.pointeur_donnees(nom);

	if (idx == -1) {
		/* À FAIRE : erreur */
		return;
	}

	donnees.stocke(idx, v);
}

/* ************************************************************************** */

struct donnees_variables {
	int type{};
	lcc::type_var donnees_type{};
};

struct donnees_boucles {
	dls::tableau<int> arretes{};
	dls::tableau<int> continues{};
};

/* pour les paramètres déclarés dans les scripts via #!param */
struct DonneesDeclarationParam {
	dls::chaine nom{};
	lcc::type_var type{};
	dls::math::vec3f min{};
	dls::math::vec3f max{};
	dls::math::vec3f valeur{};
};

using conteneur_locales = dls::tableau<std::pair<dls::vue_chaine, donnees_variables>>;

struct ContexteGenerationCode {
	assembleuse_arbre *assembleuse = nullptr;

	dls::tableau<DonneesModule *> modules{};

	dls::ensemble<lcc::req_fonc> requetes{};

	dls::pile<donnees_boucles *> boucles{};

	dls::tableau<DonneesDeclarationParam> params_declare{};

	lcc::magasin_fonctions fonctions{};

	gestionnaire_propriete gest_props{};
	gestionnaire_propriete gest_attrs{};

	/* les chaines qui seront transférées au contexte globale */
	dls::tableau<dls::chaine> chaines{};

	ContexteGenerationCode() = default;

	~ContexteGenerationCode();

	/* ********************************************************************** */

	/* La copie devrait être désactivée, car il ne peut y avoir qu'un seul
	 * contexte par compilation, mais nous l'avons besoin pour retourner des
	 * contextes par valeur. */
	ContexteGenerationCode(const ContexteGenerationCode &) = default;
	ContexteGenerationCode &operator=(const ContexteGenerationCode &) = default;

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

private:
	conteneur_locales m_locales{};
};
