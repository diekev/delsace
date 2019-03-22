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

#include "operatrices_attributs.hh"

#include "bibliotheques/outils/gna.hh"

#include "../chef_execution.hh"
#include "../contexte_evaluation.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#include "normaux.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCreationAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Attribut";
	static constexpr auto AIDE = "Crée un attribut.";

	explicit OperatriceCreationAttribut(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_creation_attribut.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto const nom_attribut = evalue_chaine("nom_attribut");
		auto const chaine_type = evalue_enum("type_attribut");
		auto const chaine_portee = evalue_enum("portee_attribut");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		type_attribut type;

		if (chaine_type == "ent8") {
			type = type_attribut::ENT8;
		}
		else if (chaine_type == "ent32") {
			type = type_attribut::ENT32;
		}
		else if (chaine_type == "décimal") {
			type = type_attribut::DECIMAL;
		}
		else if (chaine_type == "chaine") {
			type = type_attribut::CHAINE;
		}
		else if (chaine_type == "vec2") {
			type = type_attribut::VEC2;
		}
		else if (chaine_type == "vec3") {
			type = type_attribut::VEC3;
		}
		else if (chaine_type == "vec4") {
			type = type_attribut::VEC4;
		}
		else if (chaine_type == "mat3") {
			type = type_attribut::MAT3;
		}
		else if (chaine_type == "mat4") {
			type = type_attribut::MAT4;
		}
		else {
			std::stringstream ss;
			ss << "Type d'attribut '" << chaine_type << "' invalide !";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		portee_attr portee;

		if (chaine_portee == "points") {
			portee = portee_attr::POINT;
		}
		else if (chaine_portee == "primitives") {
			portee = portee_attr::PRIMITIVE;
		}
		else if (chaine_portee == "vertex") {
			portee = portee_attr::VERTEX;
		}
		else if (chaine_portee == "groupe") {
			portee = portee_attr::GROUPE;
		}
		else if (chaine_portee == "corps") {
			portee = portee_attr::CORPS;
		}
		else {
			std::stringstream ss;
			ss << "Portée d'attribut '" << chaine_portee << "' invalide !";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		m_corps.ajoute_attribut(nom_attribut, type, portee);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSuppressionAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Suppression Attribut";
	static constexpr auto AIDE = "Supprime un attribut.";

	explicit OperatriceSuppressionAttribut(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_suppression_attribut.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto const nom_attribut = evalue_chaine("nom_attribut");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		m_corps.supprime_attribut(nom_attribut);

		return EXECUTION_REUSSIE;
	}

	void obtiens_liste(
			std::string const &attache,
			std::vector<std::string> &chaines) override
	{
		if (attache == "nom_attribut") {
			entree(0)->obtiens_liste_attributs(chaines);
		}
	}
};

/* ************************************************************************** */

enum class op_rand_attr {
	REMPLACE,
	AJOUTE,
	MULTIPLIE,
	MINIMUM,
	MAXIMUM,
};

template <typename T>
auto applique_op(op_rand_attr op, T const &a, T const &b)
{
	switch (op) {
		case op_rand_attr::REMPLACE:
		{
			return b;
		}
		case op_rand_attr::AJOUTE:
		{
			/* L'addition de 'char' convertie en 'int'. */
			return static_cast<T>(a + b);
		}
		case op_rand_attr::MULTIPLIE:
		{
			/* La multiplication de 'char' convertie en 'int'. */
			return static_cast<T>(a * b);
		}
		case op_rand_attr::MINIMUM:
		{
			return (a < b) ? a : b;
		}
		case op_rand_attr::MAXIMUM:
		{
			return (a > b) ? a : b;
		}
	}

	return b;
}

class OperatriceRandomisationAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Randomisation Attribut";
	static constexpr auto AIDE = "Randomise un attribut.";

	explicit OperatriceRandomisationAttribut(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_randomisation_attribut.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto const nom_attribut = evalue_chaine("nom_attribut");
		auto const graine = evalue_entier("graine", contexte.temps_courant);
		auto const distribution = evalue_enum("distribution");
		auto const constante = evalue_decimal("constante");
		auto const val_min = evalue_decimal("valeur_min", contexte.temps_courant);
		auto const val_max = evalue_decimal("valeur_max", contexte.temps_courant);
		auto const moyenne = evalue_decimal("moyenne", contexte.temps_courant);
		auto const ecart_type = evalue_decimal("écart_type", contexte.temps_courant);
		auto const enum_operation = evalue_enum("opération");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto attrib = m_corps.attribut(nom_attribut);

		if (attrib == nullptr) {
			ajoute_avertissement("Aucun attribut ne correspond au nom spécifié !");
			return EXECUTION_ECHOUEE;
		}

		op_rand_attr operation;

		if (enum_operation == "remplace") {
			operation = op_rand_attr::REMPLACE;
		}
		else if (enum_operation == "ajoute") {
			operation = op_rand_attr::AJOUTE;
		}
		else if (enum_operation == "multiplie") {
			operation = op_rand_attr::MULTIPLIE;
		}
		else if (enum_operation == "minimum") {
			operation = op_rand_attr::MINIMUM;
		}
		else if (enum_operation == "maximum") {
			operation = op_rand_attr::MAXIMUM;
		}
		else {
			std::stringstream ss;
			ss << "Opération '" << enum_operation << "' inconnue !";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		switch (attrib->type()) {
			case type_attribut::ENT8:
			{
				auto gna = GNA(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->ent8()) {
						v = applique_op(operation, v, static_cast<char>(constante));
					}
				}
				else if (distribution == "uniforme") {
					for (auto &v : attrib->ent8()) {
						v = applique_op(operation, v, static_cast<char>(gna.uniforme(-128, 127)));
					}
				}
				else if (distribution == "gaussienne") {
					for (auto &v : attrib->ent8()) {
						v = applique_op(operation, v, static_cast<char>(gna.normale(moyenne, ecart_type)));
					}
				}

				break;
			}
			case type_attribut::ENT32:
			{
				auto gna = GNA(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->ent32()) {
						v = applique_op(operation, v, static_cast<int>(constante));
					}
				}
				else if (distribution == "uniforme") {
					for (auto &v : attrib->ent32()) {
						v = applique_op(operation, v, gna.uniforme(0, std::numeric_limits<int>::max() - 1));
					}
				}
				else if (distribution == "gaussienne") {
					for (auto &v : attrib->ent32()) {
						v = applique_op(operation, v, static_cast<int>(gna.normale(moyenne, ecart_type)));
					}
				}

				break;
			}
			case type_attribut::DECIMAL:
			{
				auto gna = GNA(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->decimal()) {
						v = applique_op(operation, v, constante);
					}
				}
				else if (distribution == "uniforme") {
					for (auto &v : attrib->decimal()) {
						v = applique_op(operation, v, gna.uniforme(val_min, val_max));
					}
				}
				else if (distribution == "gaussienne") {
					for (auto &v : attrib->decimal()) {
						v = applique_op(operation, v, gna.normale(moyenne, ecart_type));
					}
				}

				break;
			}
			case type_attribut::CHAINE:
			{
				ajoute_avertissement("La randomisation d'attribut de type chaine n'est pas supportée !");
				break;
			}
			case type_attribut::VEC2:
			{
				auto gna = GNA(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->vec2()) {
						v.x = applique_op(operation, v.x, constante);
						v.y = applique_op(operation, v.y, constante);
					}
				}
				else if (distribution == "uniforme") {
					for (auto &v : attrib->vec2()) {
						v.x = applique_op(operation, v.x, gna.uniforme(val_min, val_max));
						v.y = applique_op(operation, v.y, gna.uniforme(val_min, val_max));
					}
				}
				else if (distribution == "gaussienne") {
					for (auto &v : attrib->vec2()) {
						v.x = applique_op(operation, v.x, gna.normale(moyenne, ecart_type));
						v.y = applique_op(operation, v.y, gna.normale(moyenne, ecart_type));
					}
				}

				break;
			}
			case type_attribut::VEC3:
			{
				auto gna = GNA(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->vec3()) {
						v.x = applique_op(operation, v.x, constante);
						v.y = applique_op(operation, v.y, constante);
						v.z = applique_op(operation, v.z, constante);
					}
				}
				else if (distribution == "uniforme") {
					for (auto &v : attrib->vec3()) {
						v.x = applique_op(operation, v.x, gna.uniforme(val_min, val_max));
						v.y = applique_op(operation, v.y, gna.uniforme(val_min, val_max));
						v.z = applique_op(operation, v.z, gna.uniforme(val_min, val_max));
					}
				}
				else if (distribution == "gaussienne") {
					for (auto &v : attrib->vec3()) {
						v.x = applique_op(operation, v.x, gna.normale(moyenne, ecart_type));
						v.y = applique_op(operation, v.y, gna.normale(moyenne, ecart_type));
						v.z = applique_op(operation, v.z, gna.normale(moyenne, ecart_type));
					}
				}

				break;
			}
			case type_attribut::VEC4:
			{
				auto gna = GNA(graine);

				if (distribution == "constante") {
					for (auto &v : attrib->vec4()) {
						v.x = applique_op(operation, v.x, constante);
						v.y = applique_op(operation, v.y, constante);
						v.z = applique_op(operation, v.z, constante);
						v.w = applique_op(operation, v.w, constante);
					}
				}
				else if (distribution == "uniforme") {
					for (auto &v : attrib->vec4()) {
						v.x = applique_op(operation, v.x, gna.uniforme(val_min, val_max));
						v.y = applique_op(operation, v.y, gna.uniforme(val_min, val_max));
						v.z = applique_op(operation, v.z, gna.uniforme(val_min, val_max));
						v.w = applique_op(operation, v.w, gna.uniforme(val_min, val_max));
					}
				}
				else if (distribution == "gaussienne") {
					for (auto &v : attrib->vec4()) {
						v.x = applique_op(operation, v.x, gna.normale(moyenne, ecart_type));
						v.y = applique_op(operation, v.y, gna.normale(moyenne, ecart_type));
						v.z = applique_op(operation, v.z, gna.normale(moyenne, ecart_type));
						v.w = applique_op(operation, v.w, gna.normale(moyenne, ecart_type));
					}
				}

				break;
			}
			case type_attribut::MAT3:
			{
				ajoute_avertissement("La randomisation d'attribut de type mat3 n'est pas supportée !");
				break;
			}
			case type_attribut::MAT4:
			{
				ajoute_avertissement("La randomisation d'attribut de type mat4 n'est pas supportée !");
				break;
			}
			case type_attribut::INVALIDE:
			{
				ajoute_avertissement("Type d'attribut invalide !");
				break;
			}
		}

		return EXECUTION_REUSSIE;
	}

	bool ajourne_proprietes() override
	{
#if 0 /* À FAIRE : ajournement de l'entreface. */
		auto const distribution = evalue_enum("distribution");

		rend_propriete_visible("constante", distribution == "constante");
		rend_propriete_visible("min_value", distribution == "uniforme");
		rend_propriete_visible("max_value", distribution == "uniforme");
		rend_propriete_visible("moyenne", distribution == "gaussienne");
		rend_propriete_visible("ecart_type", distribution == "gaussienne");
#endif
		return true;
	}

	void obtiens_liste(
			std::string const &attache,
			std::vector<std::string> &chaines) override
	{
		if (attache == "nom_attribut") {
			entree(0)->obtiens_liste_attributs(chaines);
		}
	}
};

/* ************************************************************************** */

class OperatriceAjoutCouleur final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Couleur";
	static constexpr auto AIDE = "Ajoute un attribut de couleur à la géométrie entrante.";

	explicit OperatriceAjoutCouleur(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_attr_couleur.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto const graine = evalue_entier("graine", contexte.temps_courant);
		auto const couleur_ = evalue_couleur("couleur_");
		auto const methode = evalue_enum("méthode");
		auto const chaine_portee = evalue_enum("portée");
		auto const nom_groupe = evalue_chaine("nom_groupe");

		auto attrib = static_cast<Attribut *>(nullptr);
		auto groupe_points = static_cast<GroupePoint *>(nullptr);
		auto groupe_prims  = static_cast<GroupePrimitive *>(nullptr);
		portee_attr portee;

		if (chaine_portee == "points") {
			portee = portee_attr::POINT;

			if (nom_groupe != "") {
				groupe_points = m_corps.groupe_point(nom_groupe);

				if (groupe_points == nullptr) {
					std::stringstream ss;
					ss << "Groupe '" << nom_groupe << "' inconnu !";
					ajoute_avertissement(ss.str());
					return EXECUTION_ECHOUEE;
				}
			}
		}
		else if (chaine_portee == "primitives") {
			portee = portee_attr::PRIMITIVE;

			if (nom_groupe != "") {
				groupe_prims = m_corps.groupe_primitive(nom_groupe);

				if (groupe_prims == nullptr) {
					std::stringstream ss;
					ss << "Groupe '" << nom_groupe << "' inconnu !";
					ajoute_avertissement(ss.str());
					return EXECUTION_ECHOUEE;
				}
			}
		}
		else if (chaine_portee == "devine_groupe") {
			if (nom_groupe == "") {
				ajoute_avertissement("Le nom du groupe est vide !");
				return EXECUTION_ECHOUEE;
			}

			groupe_points = m_corps.groupe_point(nom_groupe);

			if (groupe_points == nullptr) {
				groupe_prims = m_corps.groupe_primitive(nom_groupe);

				if (groupe_prims == nullptr) {
					std::stringstream ss;
					ss << "Groupe '" << nom_groupe << "' inconnu !";
					ajoute_avertissement(ss.str());
					return EXECUTION_ECHOUEE;
				}

				portee = portee_attr::PRIMITIVE;
			}
			else {
				portee = portee_attr::POINT;
			}
		}
		else {
			std::stringstream ss;
			ss << "Portée '" << chaine_portee << "' non-supportée !";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		attrib = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee);

		iteratrice_index iter;

		if (groupe_points != nullptr) {
			iter = iteratrice_index(groupe_points);
		}
		else if (groupe_prims != nullptr) {
			iter = iteratrice_index(groupe_prims);
		}
		else {
			iter = iteratrice_index(attrib->taille());
		}

		if (methode == "unique") {
			for (auto index : iter) {
				attrib->vec3(index, dls::math::vec3f(couleur_.r, couleur_.v, couleur_.b));
			}
		}
		else if (methode == "aléatoire") {
			auto gna = GNA(graine);

			for (auto index : iter) {
				attrib->vec3(index, gna.uniforme_vec3(0.0f, 1.0f));
			}
		}

		return EXECUTION_REUSSIE;
	}

	bool ajourne_proprietes() override
	{
#if 0 /* À FAIRE : ajournement de l'entreface. */
		auto const distribution = evalue_enum("distribution");

		rend_propriete_visible("constante", distribution == "constante");
		rend_propriete_visible("min_value", distribution == "uniforme");
		rend_propriete_visible("max_value", distribution == "uniforme");
		rend_propriete_visible("moyenne", distribution == "gaussienne");
		rend_propriete_visible("ecart_type", distribution == "gaussienne");
#endif
		return true;
	}
};

/* ************************************************************************** */

class OperatriceCreationNormaux final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Normaux";
	static constexpr auto AIDE = "Crée des normaux pour les maillages d'entrée.";

	explicit OperatriceCreationNormaux(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_3d_normaux.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto chaine_location = evalue_chaine("location");
		auto chaine_pesee = evalue_enum("pesée");
		auto inverse_normaux = evalue_bool("inverse_direction");

		auto liste_prims = m_corps.prims();
		auto nombre_prims = liste_prims->taille();

		if (nombre_prims == 0l) {
			ajoute_avertissement("Aucun polygone trouvé pour calculer les vecteurs normaux");
			return EXECUTION_ECHOUEE;
		}

		auto location = location_normal::POINT;

		if (chaine_location == "primitive") {
			location = location_normal::PRIMITIVE;
		}
		else if (chaine_location == "corps") {
			location = location_normal::CORPS;
		}
		else if (chaine_location != "point") {
			std::stringstream ss;
			ss << "Méthode de location '" << chaine_location << "' inconnue";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		auto pesee = pesee_normal::AIRE;

		if (chaine_pesee == "angle") {
			pesee = pesee_normal::ANGLE;
		}
		else if (chaine_pesee == "max") {
			pesee = pesee_normal::MAX;
		}
		else if (chaine_pesee == "moyenne") {
			pesee = pesee_normal::MOYENNE;
		}
		else if (chaine_pesee != "aire") {
			std::stringstream ss;
			ss << "Méthode de pesée '" << chaine_location << "' inconnue";
			ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		calcul_normaux(m_corps, location, pesee, inverse_normaux);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto copie_attribut(
		Attribut *attr_orig,
		long idx_orig,
		Attribut *attr_dest,
		long idx_dest)
{
	switch (attr_orig->type()) {
		case type_attribut::ENT8:
			attr_dest->ent8(idx_dest, attr_orig->ent8(idx_orig));
			break;
		case type_attribut::ENT32:
			attr_dest->ent32(idx_dest, attr_orig->ent32(idx_orig));
			break;
		case type_attribut::DECIMAL:
			attr_dest->decimal(idx_dest, attr_orig->decimal(idx_orig));
			break;
		case type_attribut::VEC2:
			attr_dest->vec2(idx_dest, attr_orig->vec2(idx_orig));
			break;
		case type_attribut::VEC3:
			attr_dest->vec3(idx_dest, attr_orig->vec3(idx_orig));
			break;
		case type_attribut::VEC4:
			attr_dest->vec4(idx_dest, attr_orig->vec4(idx_orig));
			break;
		case type_attribut::MAT3:
			attr_dest->mat3(idx_dest, attr_orig->mat3(idx_orig));
			break;
		case type_attribut::MAT4:
			attr_dest->mat4(idx_dest, attr_orig->mat4(idx_orig));
			break;
		case type_attribut::CHAINE:
			attr_dest->chaine(idx_dest, attr_orig->chaine(idx_orig));
			break;
		case type_attribut::INVALIDE:
			break;
	}
}

#include <mutex>
#include "bibliotheques/outils/parallelisme.h"

class OpTransfereAttributs final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Transfère Attributs";
	static constexpr auto AIDE = "";

	explicit OpTransfereAttributs(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_transfere_attribut.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

		auto corps_orig = entree(1)->requiers_corps(contexte, donnees_aval);

		if (corps_orig == nullptr) {
			this->ajoute_avertissement("Aucun corps d'origine trouvé");
			return EXECUTION_ECHOUEE;
		}

		auto points = m_corps.points();
		auto points_orig = corps_orig->points();

		if (points_orig->taille() == 0) {
			this->ajoute_avertissement("Aucun point dans le corps d'origine");
			return EXECUTION_ECHOUEE;
		}

		if (points->taille() == 0) {
			this->ajoute_avertissement("Aucun point dans le corps de destination");
			return EXECUTION_ECHOUEE;
		}

		auto const nom_attribut = evalue_chaine("nom_attribut");
		auto const distance = evalue_decimal("distance", contexte.temps_courant);

		auto attr_orig = corps_orig->attribut(nom_attribut);

		if (attr_orig == nullptr) {
			std::stringstream ss;
			ss << "Le corps d'origine ne possède pas l'attribut '"
			   << nom_attribut << "'";
			this->ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		if (attr_orig->portee != portee_attr::POINT) {
			std::stringstream ss;
			ss << "L'attribut '"
			   << nom_attribut << "' n'est pas sur les points\n";
			this->ajoute_avertissement(ss.str());
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("transfère attribut");

		/* À FAIRE : collision attribut. */
		auto attr_dest = m_corps.ajoute_attribut(
					nom_attribut,
					attr_orig->type(),
					attr_orig->portee);

		boucle_parallele(tbb::blocked_range<long>(0, points->taille()),
						 [&](tbb::blocked_range<long> const &plage)
		{
			for (auto i = plage.begin(); i < plage.end(); ++i) {
				auto const point = m_corps.point_transforme(i);
				auto dist_locale = distance;
				auto idx_point_plus_pres = -1;

				/* À FAIRE : structure accéleration. */
				/* Trouve l'index point le plus proche, À FAIRE : n-points. */
				for (auto j = 0; j < points_orig->taille(); ++j) {
					auto p0 = corps_orig->point_transforme(j);
					auto l = longueur(point - p0);

					if (l < dist_locale) {
						dist_locale = l;
						idx_point_plus_pres = j;
					}
				}

				if (idx_point_plus_pres >= 0) {
					copie_attribut(attr_orig, idx_point_plus_pres, attr_dest, i);
				}
			}

			auto delta = static_cast<float>(plage.end() - plage.begin()) * 100.0f;
			chef->indique_progression_parallele(delta / static_cast<float>(points->taille()));
		});

		return EXECUTION_REUSSIE;
	}

	void obtiens_liste(
			std::string const &attache,
			std::vector<std::string> &chaines) override
	{
		if (attache == "nom_attribut") {
			entree(1)->obtiens_liste_attributs(chaines);
		}
	}
};

/* ************************************************************************** */

void enregistre_operatrices_attributs(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationAttribut>());
	usine.enregistre_type(cree_desc<OperatriceAjoutCouleur>());
	usine.enregistre_type(cree_desc<OperatriceSuppressionAttribut>());
	usine.enregistre_type(cree_desc<OperatriceRandomisationAttribut>());
	usine.enregistre_type(cree_desc<OperatriceCreationNormaux>());
	usine.enregistre_type(cree_desc<OpTransfereAttributs>());
}

#pragma clang diagnostic pop
