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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_cycles.hh"

#include "../rendu/cycles.hh"

#include "coeur/noeud.hh"

/* ************************************************************************** */

static type_prise convertis_type_prise(ccl::SocketType::Type type)
{
	switch (type) {
		case ccl::SocketType::BOOLEAN:
		{
			return type_prise::INVALIDE;
		}
		case ccl::SocketType::FLOAT:
		{
			return type_prise::DECIMAL;
		}
		case ccl::SocketType::INT:
		case ccl::SocketType::UINT:
		{
			return type_prise::ENTIER;
		}
		case ccl::SocketType::COLOR:
		{
			return type_prise::COULEUR;
		}
		case ccl::SocketType::VECTOR:
		case ccl::SocketType::POINT:
		case ccl::SocketType::NORMAL:
		{
			return type_prise::VEC3;
		}
		case ccl::SocketType::POINT2:
		{
			return type_prise::VEC2;
		}
		case ccl::SocketType::CLOSURE:
		{
			return type_prise::COULEUR;
		}
		case ccl::SocketType::TRANSFORM:
		{
			return type_prise::MAT4;
		}
		case ccl::SocketType::STRING:
		case ccl::SocketType::ENUM:
		case ccl::SocketType::NODE:
		case ccl::SocketType::BOOLEAN_ARRAY:
		case ccl::SocketType::FLOAT_ARRAY:
		case ccl::SocketType::INT_ARRAY:
		case ccl::SocketType::COLOR_ARRAY:
		case ccl::SocketType::VECTOR_ARRAY:
		case ccl::SocketType::POINT_ARRAY:
		case ccl::SocketType::NORMAL_ARRAY:
		case ccl::SocketType::POINT2_ARRAY:
		case ccl::SocketType::STRING_ARRAY:
		case ccl::SocketType::TRANSFORM_ARRAY:
		case ccl::SocketType::NODE_ARRAY:
		case ccl::SocketType::UNDEFINED:
		{
			return type_prise::INVALIDE;
		}
	}

	return type_prise::INVALIDE;
}

/* ************************************************************************** */

OperatriceCycles::OperatriceCycles(Graphe &graphe_parent, Noeud &noeud_, const ccl::NodeType *type_noeud_)
	: OperatriceImage(graphe_parent, noeud_)
{
	type_noeud = type_noeud_;

	noeud_cycles = static_cast<ccl::ShaderNode *>(type_noeud->create(type_noeud_));

	entrees(static_cast<int>(noeud_cycles->inputs.size()));
	sorties(static_cast<int>(noeud_cycles->outputs.size()));

	cree_proprietes();
}

OperatriceCycles *OperatriceCycles::cree(Graphe &graphe_, Noeud &noeud_, dls::chaine const &nom_type)
{
	auto type_noeud = ccl::NodeType::find(ccl::ustring(nom_type.c_str()));

	if (type_noeud == nullptr) {
		return nullptr;
	}

	return memoire::loge<OperatriceCycles>("OperatriceCycles", graphe_, noeud_, type_noeud);
}

const char *OperatriceCycles::nom_classe() const
{
	return NOM;
}

const char *OperatriceCycles::texte_aide() const
{
	return AIDE;
}

const char *OperatriceCycles::nom_entree(int i)
{
	return noeud_cycles->inputs[static_cast<size_t>(i)]->name().c_str();
}

const char *OperatriceCycles::nom_sortie(int i)
{
	return noeud_cycles->outputs[static_cast<size_t>(i)]->name().c_str();
}

int OperatriceCycles::type() const
{
	return OPERATRICE_DETAIL;
}

type_prise OperatriceCycles::type_entree(int i) const
{
	return convertis_type_prise(noeud_cycles->inputs[static_cast<size_t>(i)]->type());
}

type_prise OperatriceCycles::type_sortie(int i) const
{
	return convertis_type_prise(noeud_cycles->outputs[static_cast<size_t>(i)]->type());
}

res_exec OperatriceCycles::execute(const ContexteEvaluation &contexte, DonneesAval *donnees_aval)
{
	return res_exec::REUSSIE;
}

template <typename T>
T valeur_defaut(ccl::ShaderInput *entree)
{
	return *reinterpret_cast<const T *>(entree->socket_type.default_value);
}

void OperatriceCycles::cree_proprietes()
{
	for (auto i = 0; i < entrees(); ++i) {
		auto nom_propriete = nom_entree(i);

		auto prop = danjo::Propriete();

		switch (type_entree(i)) {
			case type_prise::DECIMAL:
			{
				prop.type = danjo::TypePropriete::DECIMAL;
				prop.valeur = valeur_defaut<float>(noeud_cycles->inputs[static_cast<size_t>(i)]);
				break;
			}
			case type_prise::ENTIER:
			{
				prop.type = danjo::TypePropriete::ENTIER;
				prop.valeur = valeur_defaut<int>(noeud_cycles->inputs[static_cast<size_t>(i)]);
				break;
			}
			case type_prise::VEC2:
			case type_prise::VEC3:
			{
				prop.type = danjo::TypePropriete::VECTEUR;
				auto f3 = valeur_defaut<ccl::float3>(noeud_cycles->inputs[static_cast<size_t>(i)]);
				prop.valeur = dls::math::vec3f(f3.x, f3.y, f3.z);
				break;
			}
			case type_prise::COULEUR:
			{
				prop.type = danjo::TypePropriete::COULEUR;
				auto f3 = valeur_defaut<ccl::float3>(noeud_cycles->inputs[static_cast<size_t>(i)]);
				prop.valeur = dls::phys::couleur32(f3.x, f3.y, f3.z, f3.w);
				break;
			}
			case type_prise::MAT4:
			{
				prop.type = danjo::TypePropriete::ENTIER;
				prop.valeur = valeur_defaut<int>(noeud_cycles->inputs[static_cast<size_t>(i)]);
				break;
			}
			default:
			{
				continue;
			}
		}

		ajoute_propriete_extra(nom_propriete, prop);
	}
}

/* ************************************************************************** */

dls::chaine genere_menu_noeuds_cycles()
{
	dls::chaine resultat;

	resultat += "menu \"Menu Cycles\" {\n";

	for (auto &type : ccl::NodeType::types()) {
		if (type.second.type == ccl::NodeType::NONE) {
			continue;
		}

		resultat += "    action(valeur=\"";
		resultat += type.first.c_str();
		resultat += "\"; attache=ajouter_noeud_cycles; métadonnée=\"";
		resultat += type.first.c_str();
		resultat += "\")\n";
	}

	resultat += "}\n";

	return resultat;
}
