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

#include "operatrices_alembic.hh"

#include <fstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#pragma GCC diagnostic ignored "-Wpedantic"
#include <Alembic/Abc/IArchive.h>
#include <Alembic/Abc/IObject.h>
#include <Alembic/AbcGeom/ICamera.h>
#include <Alembic/AbcGeom/ICurves.h>
#include <Alembic/AbcGeom/INuPatch.h>
#include <Alembic/AbcGeom/IPoints.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/ISubD.h>
#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcCoreOgawa/All.h>
#pragma GCC diagnostic pop

#include "biblinternes/outils/definitions.h"
#include "biblinternes/structures/flux_chaine.hh"

#include "../contexte_evaluation.hh"
#include "../chef_execution.hh"
#include "../donnees_aval.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#include "normaux.hh"

namespace ABC = Alembic::Abc;
namespace ABG = Alembic::AbcGeom;

/* ************************************************************************** */

static auto ouvre_archive(
		dls::chaine const &chemin,
		OperatriceImage &operatrice)
{
	try {
		Alembic::AbcCoreOgawa::ReadArchive archive_reader;

		return ABC::IArchive(archive_reader(chemin.c_str()), ABC::kWrapExisting, ABC::ErrorHandler::kThrowPolicy);
	}
	catch (...) {
		/* Inspect the file to see whether it's really a HDF5 file. */
		char header[4]; /* char(0x89) + "HDF" */
		std::ifstream le_fichier(chemin.c_str(), std::ios::in | std::ios::binary);

		dls::flux_chaine fc;

		if (!le_fichier) {
			fc << "Impossible d'ouvrir le fichier " << chemin;
		}
		else if (!le_fichier.read(header, sizeof(header))) {
			fc << "Impossible de lire le fichier " << chemin;
		}
		else if (strncmp(header + 1, "HDF", 3)) {
			fc << chemin << " a un format de fichier inconnu, impossible à lire.";
		}
		else {
			fc << chemin << " utilise le format obsolète HDF5, impossible à lire.";
		}

		if (le_fichier.is_open()) {
			le_fichier.close();
		}

		operatrice.ajoute_avertissement(fc.chn());
	}

	return ABC::IArchive{};
}

static auto brise(dls::chaine const &chn, char delim)
{
	std::stringstream ss(chn.c_str());
	std::string item;

	dls::tableau<dls::chaine> tokens;

	while (std::getline(ss, item, delim)) {
	  if (!item.empty()) {
		tokens.pousse(item);
	  }
	}

	return tokens;
}

static auto trouve_iobjet(const ABC::IObject &object, dls::chaine const &chemin)
{
	if (!object.valid()) {
		return ABC::IObject();
	}

	auto tokens = brise(chemin, '/');

	auto tmp = object;

	for (auto const &token : tokens) {
		tmp = tmp.getChild(token.c_str());
	}

	return tmp;
}

static auto rassemble_chemins_objets(
		ABC::IObject const &object,
		dls::tableau<dls::chaine> &chemins)
{
	if (!object.valid()) {
		return;
	}

	for (size_t i = 0; i < object.getNumChildren(); ++i) {
		chemins.pousse(object.getChild(i).getFullName().c_str());
		rassemble_chemins_objets(object.getChild(i), chemins);
	}
}

static auto converti_portee(ABG::GeometryScope portee)
{
	switch (portee) {
		case ABG::kUniformScope:
			return portee_attr::CORPS;
		case ABG::kConstantScope:
			return portee_attr::CORPS;
		case ABG::kFacevaryingScope:
			return portee_attr::VERTEX;
		case ABG::kVaryingScope:
			return portee_attr::POINT;
		case ABG::kVertexScope:
			return portee_attr::POINT;
		case ABG::kUnknownScope:
			return portee_attr::CORPS;
	}

	return portee_attr::CORPS;
}

/* ************************************************************************** */

static auto charge_points(
		Corps &corps,
		ABC::P3fArraySamplePtr positions)
{
	corps.points()->reserve(static_cast<long>(positions->size()));

	for (auto i = 0ul; i < positions->size(); ++i) {
		auto &p = (*positions)[i];

		corps.ajoute_point(p.x, p.y, p.z);
	}
}

static auto charge_attributs(
		Corps &corps,
		ABC::ICompoundProperty &prop,
		ABC::ISampleSelector const &selecteur)
{
	if (!prop.valid()) {
		return;
	}

	auto const num_props = prop.getNumProperties();

	for (size_t i = 0; i < num_props; ++i) {
		auto const &prop_header = prop.getPropertyHeader(i);
		if (ABG::IFloatGeomParam::matches(prop_header)) {
			auto param = ABG::IFloatGeomParam(prop, prop_header.getName());
			auto param_sample = ABG::IFloatGeomParam::Sample();
			param.getIndexed(param_sample, selecteur);

			auto valeurs = param_sample.getVals();
			auto indices = param_sample.getIndices();

			auto attr = corps.ajoute_attribut(
						prop_header.getName(),
						type_attribut::DECIMAL);

			attr->redimensionne(static_cast<long>(valeurs->size()));
			attr->portee = converti_portee(param_sample.getScope());

			for (auto j = 0ul; j < valeurs->size(); ++j) {
				auto &v = (*valeurs)[j];

				attr->decimal(static_cast<long>(j)) = v;
			}

			continue;
		}

		if (ABG::IV2fGeomParam::matches(prop_header)) {
			auto param = ABG::IV2fGeomParam(prop, prop_header.getName());
			auto param_sample = ABG::IV2fGeomParam::Sample();
			param.getIndexed(param_sample, selecteur);

			auto valeurs = param_sample.getVals();
			auto indices = param_sample.getIndices();

			auto attr = corps.ajoute_attribut(
						prop_header.getName(),
						type_attribut::VEC2);

			attr->redimensionne(static_cast<long>(valeurs->size()));
			attr->portee = converti_portee(param_sample.getScope());

			for (auto j = 0ul; j < valeurs->size(); ++j) {
				auto &v = (*valeurs)[j];

				attr->vec2(static_cast<long>(j)) = dls::math::vec2f(v.x, v.y);
			}

			continue;
		}

		if (ABG::IV3fGeomParam::matches(prop_header)) {
			auto param = ABG::IV3fGeomParam(prop, prop_header.getName());
			auto param_sample = ABG::IV3fGeomParam::Sample();
			param.getIndexed(param_sample, selecteur);

			auto valeurs = param_sample.getVals();
			auto indices = param_sample.getIndices();

			auto attr = corps.ajoute_attribut(
						prop_header.getName(),
						type_attribut::VEC3);

			attr->redimensionne(static_cast<long>(valeurs->size()));
			attr->portee = converti_portee(param_sample.getScope());

			for (auto j = 0ul; j < valeurs->size(); ++j) {
				auto &v = (*valeurs)[j];

				attr->vec3(static_cast<long>(j)) = dls::math::vec3f(v.x, v.y, v.z);
			}

			continue;
		}

		if (ABG::IC3fGeomParam::matches(prop_header)) {
			auto param = ABG::IC3fGeomParam(prop, prop_header.getName());
			auto param_sample = ABG::IC3fGeomParam::Sample();
			param.getIndexed(param_sample, selecteur);

			auto valeurs = param_sample.getVals();
			auto indices = param_sample.getIndices();

			auto attr = corps.ajoute_attribut(
						prop_header.getName(),
						type_attribut::VEC3);

			attr->redimensionne(static_cast<long>(valeurs->size()));
			attr->portee = converti_portee(param_sample.getScope());

			/* XXX */
			if (attr->nom() == "Col" || attr->nom() == "Cd") {
				attr->nom("C");
			}

			auto taille = (indices->size() > 0) ? indices->size() : valeurs->size();

			for (auto j = 0ul; j < taille; ++j) {

				if (indices->size() > 0) {
					auto idx = (*indices)[j];
					auto &v = (*valeurs)[idx];

					attr->vec3(static_cast<long>(idx)) = dls::math::vec3f(v.x, v.y, v.z);
				}
				else {
					auto &v = (*valeurs)[j];
					attr->vec3(static_cast<long>(j)) = dls::math::vec3f(v.x, v.y, v.z);
				}
			}

			continue;
		}

		if (ABG::IC4fGeomParam::matches(prop_header)) {
			auto param = ABG::IC4fGeomParam(prop, prop_header.getName());
			auto param_sample = ABG::IC4fGeomParam::Sample();
			param.getIndexed(param_sample, selecteur);

			auto valeurs = param_sample.getVals();
			auto indices = param_sample.getIndices();

			auto attr = corps.ajoute_attribut(
						prop_header.getName(),
						type_attribut::VEC4);

			/* XXX */
			if (attr->nom() == "Col" || attr->nom() == "Cd") {
				attr->nom("C");
			}

			attr->redimensionne(static_cast<long>(valeurs->size()));
			attr->portee = converti_portee(param_sample.getScope());

			for (auto j = 0ul; j < valeurs->size(); ++j) {
				auto &v = (*valeurs)[j];

				attr->vec4(static_cast<long>(j)) = dls::math::vec4f(v.r, v.g, v.b, v.a);
			}

			continue;
		}
	}
}

/* ************************************************************************** */

/* Fonctionalités à supporter pour une bonne pipeline :
 * - flux de données pour tous les objets depuis les fichiers (et non seulement
 *   si l'objet est animé), en incluant les instances (mais sans dupliquer la
 *   mémoire)
 * - avoir une option pour dépaqueter (à savoir convertir dans notre format) ou
 *   seulement charger pour être référencée pour les rendus ou simulations...
 * - filtrage « fuzzy » des fichiers dans l'entreface
 * - charger tous les attributs et ne pas les modifier (ex. clipping des
 *   couleurs), surtout les vecteurs de mouvements !
 * - permettre l'ajournement des caches, ou l'appent de caches préexistant
 *   (c-à-d mettre à jour toutes les opératrices utilisant le cache lors de
 *   l'ouverture d'une archive (requiers une commande pour ouvrir une archive
 *   via par exemple le menu « Fichier »))
 * - import/export des « creases »
 */

class OpImportAlembic : public OperatriceCorps {
	ABC::IArchive m_archive{};

public:
	static constexpr auto NOM = "Import Alembic";
	static constexpr auto AIDE = "Importe le contenu d'un fichier Alembic.";

	explicit OpImportAlembic(Graphe &graphe_parent, Noeud *noeud);

	OpImportAlembic(OpImportAlembic const &) = default;
	OpImportAlembic &operator=(OpImportAlembic const &) = default;

	const char *chemin_entreface() const override;

	const char *nom_classe() const override;

	const char *texte_aide() const override;

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override;

	void obtiens_liste(dls::chaine const &raison, dls::tableau<dls::chaine> &liste) override;

	bool depend_sur_temps() const override;
};

OpImportAlembic::OpImportAlembic(Graphe &graphe_parent, Noeud *noeud)
	: OperatriceCorps(graphe_parent, noeud)
{
	entrees(1);
	sorties(1);
}

const char *OpImportAlembic::chemin_entreface() const
{
	return "entreface/operatrice_import_alembic.jo";
}

const char *OpImportAlembic::nom_classe() const
{
	return NOM;
}

const char *OpImportAlembic::texte_aide() const
{
	return AIDE;
}

int OpImportAlembic::execute(
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval)
{
	INUTILISE(donnees_aval);
	m_corps.reinitialise();

	auto chef = contexte.chef;

	chef->demarre_evaluation("import alembic");

	auto chemin_archive = evalue_fichier_entree("chemin_archive");

	m_archive = ouvre_archive(chemin_archive, *this);

	if (!m_archive.valid()) {
		chef->indique_progression(1.0f);
		return EXECUTION_ECHOUEE;
	}

	auto obj_racine = m_archive.getTop();

	if (!obj_racine.valid()) {
		chef->indique_progression(1.0f);
		this->ajoute_avertissement("L'objet racine est invalide !");
		return EXECUTION_ECHOUEE;
	}

	auto chemin_objet = evalue_chaine("chemin_objet");

	if (chemin_objet.est_vide()) {
		this->ajoute_avertissement("Le chemin de l'objet est vide !");
		return EXECUTION_ECHOUEE;
	}

	auto obj = trouve_iobjet(obj_racine, chemin_objet);

	if (!obj.valid()) {
		chef->indique_progression(1.0f);
		this->ajoute_avertissement("Impossible de trouver l'objet !");
		return EXECUTION_ECHOUEE;
	}

	auto selecteur = ABC::ISampleSelector(static_cast<double>(contexte.temps_courant) / contexte.cadence);

	if (ABG::IPolyMesh::matches(obj.getHeader())) {
		auto poly_mesh = ABG::IPolyMesh(obj);

		auto schema = poly_mesh.getSchema();

		auto sample = schema.getValue(selecteur);

		/* positions */

		auto positions = sample.getPositions();
		charge_points(m_corps, positions);

		/* faces */

		auto compte_faces = sample.getFaceCounts();
		auto index_faces  = sample.getFaceIndices();
		auto poly_index = 0ul;

		for (auto i = 0ul; i < compte_faces->size(); ++i) {
			auto compte = (*compte_faces)[i];

			auto poly = Polygone::construit(&m_corps, type_polygone::FERME, compte);

			for (auto j = 0; j < compte; ++j) {
				poly->ajoute_sommet((*index_faces)[poly_index++]);
			}
		}

		/* normaux */

		auto normaux = schema.getNormalsParam();

		if (normaux.valid()) {
			if (normaux.getScope() == ABG::kFacevaryingScope) {
				/* normaux par poly */
			}
		}

		calcul_normaux(m_corps, location_normal::PRIMITIVE, pesee_normal::AIRE, true);

		/* attributs */

		auto prop = schema.getArbGeomParams();
		charge_attributs(m_corps, prop, selecteur);
	}
	else if (ABG::IPoints::matches(obj.getHeader())) {
		auto poly_mesh = ABG::IPoints(obj);

		auto schema = poly_mesh.getSchema();

		auto sample = schema.getValue(selecteur);

		auto positions = sample.getPositions();
		charge_points(m_corps, positions);

		auto prop = schema.getArbGeomParams();
		charge_attributs(m_corps, prop, selecteur);
	}
	else {
		ajoute_avertissement("Type d'objet non-supporté !");
	}

	chef->indique_progression(1.0f);

	return EXECUTION_REUSSIE;
}

void OpImportAlembic::obtiens_liste(
		dls::chaine const &raison,
		dls::tableau<dls::chaine> &liste)
{
	liste.clear();

	if (!m_archive.valid()) {
		return;
	}

	if (raison == "chemin_objet") {
		rassemble_chemins_objets(m_archive.getTop(), liste);
	}
}

bool OpImportAlembic::depend_sur_temps() const
{
	/* À FAIRE */
	return true;
}

/* ************************************************************************** */

void enregistre_operatrices_alembic(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OpImportAlembic>());
}
