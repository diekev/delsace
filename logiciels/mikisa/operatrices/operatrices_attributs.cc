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

#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "normaux.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceCreationAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Attribut";
	static constexpr auto AIDE = "Crée un attribut.";

	OperatriceCreationAttribut(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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
		auto const chaine_dims = evalue_enum("dimensions_attr");
		auto const chaine_prec = evalue_enum("précision_attr");

		if (nom_attribut == "") {
			ajoute_avertissement("Le nom de l'attribut est vide !");
			return EXECUTION_ECHOUEE;
		}

		auto dico_precisions = dls::cree_dico(
					dls::paire{ dls::chaine("8"), 8 },
					dls::paire{ dls::chaine("16"), 16 },
					dls::paire{ dls::chaine("32"), 32 },
					dls::paire{ dls::chaine("64"), 64 });

		auto dico_dimensions = dls::cree_dico(
					dls::paire{ dls::chaine("1"), 1 },
					dls::paire{ dls::chaine("2"), 2 },
					dls::paire{ dls::chaine("3"), 3 },
					dls::paire{ dls::chaine("4"), 4 },
					dls::paire{ dls::chaine("9"), 9 },
					dls::paire{ dls::chaine("16"), 16 });

		auto dico_portee = dls::cree_dico(
					dls::paire{ dls::chaine("corps"), portee_attr::CORPS },
					dls::paire{ dls::chaine("groupe"), portee_attr::GROUPE },
					dls::paire{ dls::chaine("points"), portee_attr::POINT },
					dls::paire{ dls::chaine("primitives"), portee_attr::PRIMITIVE },
					dls::paire{ dls::chaine("sommets"), portee_attr::VERTEX });

		auto plg_prec = dico_precisions.trouve(chaine_prec);

		if (plg_prec.est_finie()) {
			dls::flux_chaine ss;
			ss << "Précision d'attribut '" << chaine_prec << "' invalide !";
			ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		auto precision = plg_prec.front().second;

		auto plg_dim = dico_dimensions.trouve(chaine_dims);

		if (plg_dim.est_finie()) {
			dls::flux_chaine ss;
			ss << "Dimensions d'attribut '" << chaine_dims << "' invalide !";
			ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		auto dimensions = plg_dim.front().second;

		auto plg_portee = dico_portee.trouve(chaine_portee);

		if (plg_portee.est_finie()) {
			dls::flux_chaine ss;
			ss << "Portée d'attribut '" << chaine_portee << "' invalide !";
			ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		auto portee = plg_portee.front().second;

		type_attribut type;

		if (chaine_type == "naturel") {
			if (precision == 8) {
				type = type_attribut::N8;
			}
			else if (precision == 16) {
				type = type_attribut::N16;
			}
			else if (precision == 32) {
				type = type_attribut::N32;
			}
			else {
				type = type_attribut::N64;
			}
		}
		else if (chaine_type == "relatif") {
			if (precision == 8) {
				type = type_attribut::Z8;
			}
			else if (precision == 16) {
				type = type_attribut::Z16;
			}
			else if (precision == 32) {
				type = type_attribut::Z32;
			}
			else {
				type = type_attribut::Z64;
			}
		}
		else if (chaine_type == "réél") {
			if (precision == 8) {
				ajoute_avertissement("Un nombre réel ne peut avoir une précision de 8-bit !");
				return EXECUTION_ECHOUEE;
			}
			else if (precision == 16) {
				type = type_attribut::Z16;
			}
			else if (precision == 32) {
				type = type_attribut::Z32;
			}
			else {
				type = type_attribut::Z64;
			}
		}
		else if (chaine_type == "chaine") {
			type = type_attribut::CHAINE;
		}
		else {
			dls::flux_chaine ss;
			ss << "Type d'attribut '" << chaine_type << "' invalide !";
			ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		m_corps.ajoute_attribut(nom_attribut, type, dimensions, portee);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

class OperatriceSuppressionAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Suppression Attribut";
	static constexpr auto AIDE = "Supprime un attribut.";

	OperatriceSuppressionAttribut(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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
			ContexteEvaluation const &contexte,
			dls::chaine const &attache,
			dls::tableau<dls::chaine> &chaines) override
	{
		INUTILISE(contexte);
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

struct params_randomisation {
	dls::chaine distribution = "";
	int graine = 0;
	op_rand_attr operation{};
	float constante = 0.0f;
	float moyenne = 0.0f;
	float ecart_type = 0.0f;
	float val_min = 0.0f;
	float val_max = 0.0f;
};

template <typename T>
auto applique_randomisation(
		Attribut &attr,
		params_randomisation const &params)
{
	auto gna = GNA(params.graine);

	if (params.distribution == "constante") {
		transforme_attr<T>(attr, [&](T *ptr)
		{
			for (auto i = 0; i < attr.dimensions; ++i) {
				ptr[i] = applique_op(
							params.operation,
							ptr[i],
							static_cast<T>(params.constante));
			}
		});
	}
	else if (params.distribution == "uniforme") {
		transforme_attr<T>(attr, [&](T *ptr)
		{
			for (auto i = 0; i < attr.dimensions; ++i) {
				ptr[i] = applique_op(
							params.operation,
							ptr[i],
							static_cast<T>(gna.uniforme(params.val_min, params.val_max)));
			}
		});
	}
	else if (params.distribution == "gaussienne") {
		transforme_attr<T>(attr, [&](T *ptr)
		{
			for (auto i = 0; i < attr.dimensions; ++i) {
				ptr[i] = applique_op(
							params.operation,
							ptr[i],
							static_cast<T>(gna.normale(params.moyenne, params.ecart_type)));
			}
		});
	}
}

class OperatriceRandomisationAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Randomisation Attribut";
	static constexpr auto AIDE = "Randomise un attribut.";

	OperatriceRandomisationAttribut(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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
			dls::flux_chaine ss;
			ss << "Opération '" << enum_operation << "' inconnue !";
			ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		auto params = params_randomisation{};
		params.distribution = distribution;
		params.graine = graine;
		params.moyenne = moyenne;
		params.ecart_type = ecart_type;
		params.constante = constante;
		params.operation = operation;
		params.val_min = val_min;
		params.val_max = val_max;

		switch (attrib->type()) {
			case type_attribut::N8:
			{
				applique_randomisation<unsigned char>(*attrib, params);
				break;
			}
			case type_attribut::N16:
			{
				applique_randomisation<unsigned short>(*attrib, params);
				break;
			}
			case type_attribut::N32:
			{
				applique_randomisation<unsigned int>(*attrib, params);
				break;
			}
			case type_attribut::N64:
			{
				applique_randomisation<unsigned long>(*attrib, params);
				break;
			}
			case type_attribut::Z8:
			{
				applique_randomisation<char>(*attrib, params);
				break;
			}
			case type_attribut::Z16:
			{
				applique_randomisation<short>(*attrib, params);
				break;
			}
			case type_attribut::Z32:
			{
				applique_randomisation<int>(*attrib, params);
				break;
			}
			case type_attribut::Z64:
			{
				applique_randomisation<long>(*attrib, params);
				break;
			}
			case type_attribut::R16:
			{
				applique_randomisation<r16>(*attrib, params);
				break;
			}
			case type_attribut::R32:
			{
				applique_randomisation<float>(*attrib, params);
				break;
			}
			case type_attribut::R64:
			{
				applique_randomisation<double>(*attrib, params);
				break;
			}
			case type_attribut::CHAINE:
			{
				ajoute_avertissement("La randomisation d'attribut de type chaine n'est pas supportée !");
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
			ContexteEvaluation const &contexte,
			dls::chaine const &attache,
			dls::tableau<dls::chaine> &chaines) override
	{
		INUTILISE(contexte);
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

	OperatriceAjoutCouleur(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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
					dls::flux_chaine ss;
					ss << "Groupe '" << nom_groupe << "' inconnu !";
					ajoute_avertissement(ss.chn());
					return EXECUTION_ECHOUEE;
				}
			}
		}
		else if (chaine_portee == "primitives") {
			portee = portee_attr::PRIMITIVE;

			if (nom_groupe != "") {
				groupe_prims = m_corps.groupe_primitive(nom_groupe);

				if (groupe_prims == nullptr) {
					dls::flux_chaine ss;
					ss << "Groupe '" << nom_groupe << "' inconnu !";
					ajoute_avertissement(ss.chn());
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
					dls::flux_chaine ss;
					ss << "Groupe '" << nom_groupe << "' inconnu !";
					ajoute_avertissement(ss.chn());
					return EXECUTION_ECHOUEE;
				}

				portee = portee_attr::PRIMITIVE;
			}
			else {
				portee = portee_attr::POINT;
			}
		}
		else {
			dls::flux_chaine ss;
			ss << "Portée '" << chaine_portee << "' non-supportée !";
			ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		attrib = m_corps.ajoute_attribut("C", type_attribut::R32, 3, portee);

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
				assigne(attrib->r32(index), dls::math::vec3f(couleur_.r, couleur_.v, couleur_.b));
			}
		}
		else if (methode == "aléatoire") {
			auto gna = GNA(graine);

			for (auto index : iter) {
				assigne(attrib->r32(index), gna.uniforme_vec3(0.0f, 1.0f));
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

	OperatriceCreationNormaux(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

		if (!valide_corps_entree(*this, &m_corps, true, true)) {
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
			dls::flux_chaine ss;
			ss << "Méthode de location '" << chaine_location << "' inconnue";
			ajoute_avertissement(ss.chn());
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
			dls::flux_chaine ss;
			ss << "Méthode de pesée '" << chaine_location << "' inconnue";
			ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		calcul_normaux(m_corps, location, pesee, inverse_normaux);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

#include <mutex>
#include "biblinternes/moultfilage/boucle.hh"

class OpTransfereAttributs final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Transfère Attributs";
	static constexpr auto AIDE = "";

	OpTransfereAttributs(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

		if (!valide_corps_entree(*this, corps_orig, true, true, 1)) {
			return EXECUTION_ECHOUEE;
		}

		auto points = m_corps.points_pour_lecture();
		auto points_orig = corps_orig->points_pour_lecture();

		auto const nom_attribut = evalue_chaine("nom_attribut");
		auto const distance = evalue_decimal("distance", contexte.temps_courant);

		auto attr_orig = corps_orig->attribut(nom_attribut);

		if (attr_orig == nullptr) {
			dls::flux_chaine ss;
			ss << "Le corps d'origine ne possède pas l'attribut '"
			   << nom_attribut << "'";
			this->ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		if (attr_orig->portee != portee_attr::POINT) {
			dls::flux_chaine ss;
			ss << "L'attribut '"
			   << nom_attribut << "' n'est pas sur les points\n";
			this->ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("transfère attribut");

		/* À FAIRE : collision attribut. */
		auto attr_dest = m_corps.ajoute_attribut(
					nom_attribut,
					attr_orig->type(),
					attr_orig->dimensions,
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
			ContexteEvaluation const &contexte,
			dls::chaine const &attache,
			dls::tableau<dls::chaine> &chaines) override
	{
		INUTILISE(contexte);
		if (attache == "nom_attribut") {
			entree(1)->obtiens_liste_attributs(chaines);
		}
	}
};

/* ************************************************************************** */

#include "corps/iteration_corps.hh"
#include "biblinternes/outils/definitions.h"

struct donnees_promotion {
	long idx_dest{};
	dls::tableau<std::pair<long, float>> paires_idx_poids{};
};

static auto promeut_attribut(Corps &corps, Attribut &attr_orig, portee_attr portee_dest)
{
	auto portee_orig = attr_orig.portee;

	auto attr_dest = corps.ajoute_attribut(
				attr_orig.nom() + "_promotion",
				attr_orig.type(),
				attr_orig.dimensions,
				portee_dest);

	dls::tableau<donnees_promotion> donnees;
	donnees.reserve(attr_dest->taille());

	if (portee_orig == portee_attr::POINT) {
		if (portee_dest == portee_attr::PRIMITIVE) {
			/* moyenne des attributs des points autour de la primitive */
			pour_chaque_primitive(corps, [&](Corps const &corps_entree, Polygone *prim)
			{
				INUTILISE(corps_entree);

				auto donnee = donnees_promotion{};
				donnee.idx_dest = prim->index;

				auto nombre_points = prim->nombre_sommets();

				for (auto i = 0l; i < nombre_points; ++i) {
					donnee.paires_idx_poids.pousse({prim->index_point(i), 1.0f / static_cast<float>(nombre_points)});
				}

				donnees.pousse(donnee);
			});
		}
		else if (portee_dest == portee_attr::VERTEX) {
			/* attribut du point de ce vertex */
			pour_chaque_primitive(corps, [&](Corps const &corps_entree, Polygone *prim)
			{
				INUTILISE(corps_entree);

				auto nombre_points = prim->nombre_sommets();

				for (auto i = 0l; i < nombre_points; ++i) {
					auto donnee = donnees_promotion{};
					donnee.idx_dest = donnees.taille();
					donnee.paires_idx_poids.pousse({prim->index_point(i), 1.0f});
					donnees.pousse(donnee);
				}
			});
		}
		else if (portee_dest == portee_attr::CORPS) {
			/* moyenne de tous les attributs */
			auto donnee = donnees_promotion{};
			donnee.idx_dest = 0;

			for (auto i = 0l; i < attr_orig.taille(); ++i) {
				donnee.paires_idx_poids.pousse({i, static_cast<float>(i) / static_cast<float>(attr_orig.taille())});
			}

			donnees.pousse(donnee);
		}
	}
	else if (portee_orig == portee_attr::PRIMITIVE) {
		if (portee_dest == portee_attr::POINT) {
			// moyenne des attributs des primitives autour du point
		}
		else if (portee_dest == portee_attr::VERTEX) {
			// attribut de la primitive contenant le vertex
		}
		else if (portee_dest == portee_attr::CORPS) {
			/* moyenne de tous les attributs */
			auto donnee = donnees_promotion{};
			donnee.idx_dest = 0;

			for (auto i = 0l; i < attr_orig.taille(); ++i) {
				donnee.paires_idx_poids.pousse({i, static_cast<float>(i) / static_cast<float>(attr_orig.taille())});
			}

			donnees.pousse(donnee);
		}
	}
	else if (portee_orig == portee_attr::VERTEX) {
		if (portee_dest == portee_attr::POINT) {
			// moyenne des attributs des vertex autour du point
		}
		else if (portee_dest == portee_attr::PRIMITIVE) {
			// moyenne des attributs des vertex autour de la primitive
		}
		else if (portee_dest == portee_attr::CORPS) {
			/* moyenne de tous les attributs */
			auto donnee = donnees_promotion{};
			donnee.idx_dest = 0;

			for (auto i = 0l; i < attr_orig.taille(); ++i) {
				donnee.paires_idx_poids.pousse({i, static_cast<float>(i) / static_cast<float>(attr_orig.taille())});
			}

			donnees.pousse(donnee);
		}
	}
	else if (portee_orig == portee_attr::CORPS) {
		/* peut importe la portée de destination, l'attribut est le même */
		for (auto i = 0l; i < attr_dest->taille(); ++i) {
			auto donnee = donnees_promotion{};
			donnee.idx_dest = i;
			donnee.paires_idx_poids.pousse({0, 1.0f});

			donnees.pousse(donnee);
		}
	}
	else if (portee_orig == portee_attr::GROUPE) {
		/* À FAIRE */
	}

	for (auto const &donnee : donnees) {
		for (auto const &p : donnee.paires_idx_poids) {
			copie_attribut(&attr_orig, p.first, attr_dest, donnee.idx_dest);
		}
	}

	auto const &nom = attr_orig.nom();
	corps.supprime_attribut(nom);
	attr_dest->nom(nom);
}

class OpPromeutAttribut final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Promeut Attributs";
	static constexpr auto AIDE = "";

	OpPromeutAttribut(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(1);
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

		auto const nom_attribut = evalue_chaine("nom_attribut");

		auto attr_orig = m_corps.attribut(nom_attribut);

		if (attr_orig == nullptr) {
			dls::flux_chaine ss;
			ss << "Le corps d'origine ne possède pas l'attribut '"
			   << nom_attribut << "'";
			this->ajoute_avertissement(ss.chn());
			return EXECUTION_ECHOUEE;
		}

		return EXECUTION_REUSSIE;
	}

	void obtiens_liste(
			ContexteEvaluation const &contexte,
			dls::chaine const &attache,
			dls::tableau<dls::chaine> &chaines) override
	{
		INUTILISE(contexte);
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
	usine.enregistre_type(cree_desc<OpPromeutAttribut>());
}

#pragma clang diagnostic pop
