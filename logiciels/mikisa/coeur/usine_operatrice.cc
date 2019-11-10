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

#include "usine_operatrice.h"

#include <cassert>

#include "operatrice_corps.h"
#include "operatrice_image.h"

/* ************************************************************************** */

class OperatriceCorpsSE final : public OperatriceCorps {
public:
	type_operatrice_sans_entree m_fonction{};
	const char *m_nom_classe = "";
	const char *m_aide = "";
	const char *m_chemin_entreface = "";
	bool m_depend_sur_temps = false;
	char pad[7];

	OperatriceCorpsSE(Graphe &graphe_parent, Noeud &noeud_);

	OperatriceCorpsSE(OperatriceCorpsSE const &) = default;
	OperatriceCorpsSE &operator=(OperatriceCorpsSE const &) = default;

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	const char *chemin_entreface() const override;

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	bool depend_sur_temps() const override;
};

OperatriceCorpsSE::OperatriceCorpsSE(Graphe &graphe_parent, Noeud &noeud_)
	: OperatriceCorps(graphe_parent, noeud_)
{
	entrees(0);
	sorties(1);
}

const char *OperatriceCorpsSE::nom_classe() const
{
	return m_nom_classe;
}

const char *OperatriceCorpsSE::texte_aide() const
{
	return m_aide;
}

const char *OperatriceCorpsSE::chemin_entreface() const
{
	return m_chemin_entreface;
}

res_exec OperatriceCorpsSE::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	if (m_fonction == nullptr) {
		this->ajoute_avertissement("La fonction est nulle !");
		return res_exec::ECHOUEE;
	}

	return m_fonction(*this, contexte, donnees_aval);
}

bool OperatriceCorpsSE::depend_sur_temps() const
{
	return m_depend_sur_temps;
}

/* ************************************************************************** */

class OperatriceCorpsE0 final : public OperatriceCorps {
public:
	type_operatrice_entree0 m_fonction{};
	const char *m_nom_classe = "";
	const char *m_aide = "";
	const char *m_chemin_entreface = "";
	bool m_depend_sur_temps = false;
	char pad[7];

	OperatriceCorpsE0(Graphe &graphe_parent, Noeud &noeud_);

	OperatriceCorpsE0(OperatriceCorpsE0 const &) = default;
	OperatriceCorpsE0 &operator=(OperatriceCorpsE0 const &) = default;

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	const char *chemin_entreface() const override;

	res_exec execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	bool depend_sur_temps() const override;
};

OperatriceCorpsE0::OperatriceCorpsE0(Graphe &graphe_parent, Noeud &noeud_)
	: OperatriceCorps(graphe_parent, noeud_)
{
	entrees(1);
	sorties(1);
}

const char *OperatriceCorpsE0::nom_classe() const
{
	return m_nom_classe;
}

const char *OperatriceCorpsE0::texte_aide() const
{
	return m_aide;
}

const char *OperatriceCorpsE0::chemin_entreface() const
{
	return m_chemin_entreface;
}

res_exec OperatriceCorpsE0::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	if (m_fonction == nullptr) {
		this->ajoute_avertissement("La fonction est nulle !");
		return res_exec::ECHOUEE;
	}

	auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

	if (corps_entree == nullptr) {
		this->ajoute_avertissement("L'entrée 1 n'est pas connectée !");
		return res_exec::ECHOUEE;
	}

	return m_fonction(*this, contexte, donnees_aval, *corps_entree);
}

bool OperatriceCorpsE0::depend_sur_temps() const
{
	return m_depend_sur_temps;
}

/* ************************************************************************** */

DescOperatrice cree_desc(
		const char *nom,
		const char *aide,
		const char *chemin_entreface,
		type_operatrice_sans_entree &&fonction,
		bool depend_sur_temps)
{
	return DescOperatrice(
				nom,
				aide,
				[=](Graphe &graphe_parent, Noeud &noeud) -> OperatriceImage*
	{
		auto ptr = memoire::loge<OperatriceCorpsSE>(nom, graphe_parent, noeud);
		ptr->m_nom_classe = nom;
		ptr->m_aide = aide;
		ptr->m_chemin_entreface = chemin_entreface;
		ptr->m_fonction = fonction;
		ptr->m_depend_sur_temps = depend_sur_temps;

		return ptr;
	},
	[=](OperatriceImage *operatrice) -> void
	{
		auto derivee = dynamic_cast<OperatriceCorpsSE *>(operatrice);
		memoire::deloge(nom, derivee);
	});
}

DescOperatrice cree_desc(
		const char *nom,
		const char *aide,
		const char *chemin_entreface,
		type_operatrice_entree0 &&fonction,
		bool depend_sur_temps)
{
	return DescOperatrice(
				nom,
				aide,
				[=](Graphe &graphe_parent, Noeud &noeud) -> OperatriceImage*
	{
		auto ptr = memoire::loge<OperatriceCorpsE0>(nom, graphe_parent, noeud);
		ptr->m_nom_classe = nom;
		ptr->m_aide = aide;
		ptr->m_chemin_entreface = chemin_entreface;
		ptr->m_fonction = fonction;
		ptr->m_depend_sur_temps = depend_sur_temps;

		return ptr;
	},
	[=](OperatriceImage *operatrice) -> void
	{
		auto derivee = dynamic_cast<OperatriceCorpsE0 *>(operatrice);
		memoire::deloge(nom, derivee);
	});
}

/* ************************************************************************** */

long UsineOperatrice::enregistre_type(DescOperatrice const &desc)
{
	auto const iter = m_map.trouve(desc.name);
	assert(iter == m_map.fin());

	m_map[desc.name] = desc;
	return num_entries();
}

OperatriceImage *UsineOperatrice::operator()(dls::chaine const &name, Graphe &graphe_parent, Noeud &noeud_)
{
	auto const iter = m_map.trouve(name);
	assert(iter != m_map.fin());

	DescOperatrice const &desc = iter->second;

	auto operatrice = desc.build_operator(graphe_parent, noeud_);
	operatrice->usine(this);

	return operatrice;
}

void UsineOperatrice::deloge(OperatriceImage *operatrice)
{
	auto const iter = m_map.trouve(operatrice->nom_classe());
	assert(iter != m_map.fin());

	DescOperatrice &desc = iter->second;
	desc.supprime_operatrice(operatrice);
}

dls::tableau<DescOperatrice> UsineOperatrice::keys() const
{
	dls::tableau<DescOperatrice> v;
	v.reserve(num_entries());

	for (auto const &entry : m_map) {
		v.pousse(entry.second);
	}

	return v;
}

bool UsineOperatrice::registered(dls::chaine const &key) const
{
	return (m_map.trouve(key) != m_map.fin());
}
