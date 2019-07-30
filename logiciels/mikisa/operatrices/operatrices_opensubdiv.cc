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

#include "operatrices_opensubdiv.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#include <opensubdiv/far/primvarRefiner.h>
#include <opensubdiv/far/topologyDescriptor.h>
#pragma GCC diagnostic pop

#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/structures/dico_fixe.hh"
#include "biblinternes/structures/tableau.hh"

#include "corps/iteration_corps.hh"

#include "normaux.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

struct SommetOSD {
	SommetOSD() = default;

	SommetOSD(SommetOSD const & src)
	{
		_position[0] = src._position[0];
		_position[1] = src._position[1];
		_position[2] = src._position[2];
	}

	void Clear( void * = nullptr )
	{
		_position[0] = _position[1] = _position[2] = 0.0f;
	}

	void AddWithWeight(SommetOSD const & src, float weight)
	{
		_position[0] += weight * src._position[0];
		_position[1] += weight * src._position[1];
		_position[2] += weight * src._position[2];
	}

	// Public entreface ------------------------------------
	void SetPosition(float x, float y, float z)
	{
		_position[0] = x;
		_position[1] = y;
		_position[2] = z;
	}

	const float *GetPosition() const
	{
		return _position;
	}

private:
	float _position[3];
};

class OperatriceOpenSubDiv final : public OperatriceCorps {
public:
	static constexpr auto NOM = "OpenSubDiv";
	static constexpr auto AIDE = "Sousdivise les maillages d'entrée en utilisant la bibliothèque OpenSubDiv.";

	explicit OperatriceOpenSubDiv(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_opensubdiv.jo";
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
		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (corps_entree == nullptr) {
			this->ajoute_avertissement("Aucun corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		/* peuple un descripteur avec nos données crues */
		using Descripteur   = OpenSubdiv::Far::TopologyDescriptor;
		using Refineur      = OpenSubdiv::Far::TopologyRefiner;
		using UsineRafineur = OpenSubdiv::Far::TopologyRefinerFactory<Descripteur>;

		auto const niveau_max = evalue_entier("niveau_max");

		auto const schema = evalue_enum("schéma");

		static auto dico_schema = dls::cree_dico(
					dls::paire{ dls::chaine("catmark"), OpenSubdiv::Sdc::SCHEME_CATMARK },
					dls::paire{ dls::chaine("bilineaire"), OpenSubdiv::Sdc::SCHEME_BILINEAR },
					/* À FAIRE : CRASH avec OpenSubdiv::Sdc::SCHEME_LOOP */
					dls::paire{ dls::chaine("boucle"), OpenSubdiv::Sdc::SCHEME_CATMARK });

		auto plg_subdiv = dico_schema.trouve(schema);

		if (plg_subdiv.est_finie()) {
			ajoute_avertissement("Type de schéma invalide !");
			return EXECUTION_ECHOUEE;
		}

		auto const type_subdiv = plg_subdiv.front().second;

		OpenSubdiv::Sdc::Options options;

		auto const entrep_bord = evalue_enum("entrep_bord");

		static auto dico_entrep = dls::cree_dico(
					dls::paire{ dls::chaine("aucune"), OpenSubdiv::Sdc::Options::VTX_BOUNDARY_NONE },
					dls::paire{ dls::chaine("segment"), OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_ONLY },
					dls::paire{ dls::chaine("segment_coin"), OpenSubdiv::Sdc::Options::VTX_BOUNDARY_EDGE_AND_CORNER });

		auto plg_entrep_bord = dico_entrep.trouve(entrep_bord);

		if (plg_entrep_bord.est_finie()) {
			ajoute_avertissement("Type d'entrepolation bordure sommet invalide !");
			return EXECUTION_ECHOUEE;
		}

		options.SetVtxBoundaryInterpolation(plg_entrep_bord.front().second);

		static auto dico_entrep_fvar = dls::cree_dico(
					dls::paire{ dls::chaine("aucune"), OpenSubdiv::Sdc::Options::FVAR_LINEAR_NONE },
					dls::paire{ dls::chaine("coins_seuls"), OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_ONLY },
					dls::paire{ dls::chaine("coins_p1"), OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_PLUS1 },
					dls::paire{ dls::chaine("coins_p2"), OpenSubdiv::Sdc::Options::FVAR_LINEAR_CORNERS_PLUS2 },
					dls::paire{ dls::chaine("bordures"), OpenSubdiv::Sdc::Options::FVAR_LINEAR_BOUNDARIES },
					dls::paire{ dls::chaine("tout"), OpenSubdiv::Sdc::Options::FVAR_LINEAR_ALL });

		auto const entrep_fvar = evalue_enum("entrep_fvar");

		auto plg_entrep_fvar = dico_entrep_fvar.trouve(entrep_fvar);

		if (plg_entrep_fvar.est_finie()) {
			ajoute_avertissement("Type d'entrepolation bordure sommet invalide !");
			return EXECUTION_ECHOUEE;
		}

		options.SetFVarLinearInterpolation(plg_entrep_fvar.front().second);

		static auto dico_pliure = dls::cree_dico(
					dls::paire{ dls::chaine("uniforme"), OpenSubdiv::Sdc::Options::CREASE_UNIFORM },
					dls::paire{ dls::chaine("chaikin"), OpenSubdiv::Sdc::Options::CREASE_CHAIKIN });

		auto const pliure = evalue_enum("pliage");

		auto plg_pliure = dico_pliure.trouve(pliure);

		if (plg_pliure.est_finie()) {
			ajoute_avertissement("Type de pliage invalide !");
			return EXECUTION_ECHOUEE;
		}

		options.SetCreasingMethod(plg_pliure.front().second);

		static auto dico_sd_tri = dls::cree_dico(
					dls::paire{ dls::chaine("catmark"), OpenSubdiv::Sdc::Options::TRI_SUB_CATMARK },
					dls::paire{ dls::chaine("lisse"), OpenSubdiv::Sdc::Options::TRI_SUB_SMOOTH });

		auto const sousdivision_triangle = evalue_enum("sousdivision_triangle");

		auto plg_sd_tri = dico_sd_tri.trouve(sousdivision_triangle);

		if (plg_sd_tri.est_finie()) {
			ajoute_avertissement("Type de sousdivision triangulaire invalide !");
			return EXECUTION_ECHOUEE;
		}

		options.SetTriangleSubdivision(plg_sd_tri.front().second);

		auto nombre_sommets = corps_entree->points()->taille();
		auto nombre_polygones = corps_entree->prims()->taille();

		Descripteur desc;
		desc.numVertices = static_cast<int>(nombre_sommets);
		desc.numFaces    = static_cast<int>(nombre_polygones);

		dls::tableau<int> nombre_sommets_par_poly;
		nombre_sommets_par_poly.reserve(nombre_polygones);

		dls::tableau<int> index_sommets_polys;
		index_sommets_polys.reserve(nombre_sommets * nombre_polygones);

		pour_chaque_polygone_ferme(*corps_entree,
								   [&](Corps const &, Polygone *poly)
		{
			nombre_sommets_par_poly.pousse(static_cast<int>(poly->nombre_sommets()));

			for (long i = 0; i < poly->nombre_sommets(); ++i) {
				index_sommets_polys.pousse(static_cast<int>(poly->index_point(i)));
			}
		});

		desc.numVertsPerFace = &nombre_sommets_par_poly[0];
		desc.vertIndicesPerFace = &index_sommets_polys[0];

		/* Crée un rafineur depuis le descripteur. */
		auto rafineur = UsineRafineur::Create(
							desc, UsineRafineur::Options(type_subdiv, options));

		/* Rafine uniformément la topologie jusque 'niveau_max'. */
		rafineur->RefineUniform(Refineur::UniformOptions(niveau_max));

		/* Alloue un tampon pouvant contenir le nombre total de sommets à
			 * 'niveau_max' de rafinement. */
		dls::tableau<SommetOSD> sommets_osd(rafineur->GetNumVerticesTotal());
		SommetOSD *sommets = &sommets_osd[0];

		/* Initialise les positions du maillage grossier. */
		auto index_point = 0;
		for (auto i = 0; i < corps_entree->points()->taille(); ++i) {
			auto point = corps_entree->points()->point(i);
			auto const v0 = corps_entree->transformation(dls::math::point3d(point));

			sommets[index_point++].SetPosition(static_cast<float>(v0.x), static_cast<float>(v0.y), static_cast<float>(v0.z));
		}

		/* Entrepole les sommets */
		OpenSubdiv::Far::PrimvarRefiner rafineur_primvar(*rafineur);

		SommetOSD *src_sommets = sommets;
		for (int niveau = 1; niveau <= niveau_max; ++niveau) {
			auto dst_sommets = src_sommets + rafineur->GetLevel(niveau - 1).GetNumVertices();
			rafineur_primvar.Interpolate(niveau, src_sommets, dst_sommets);
			src_sommets = dst_sommets;
		}

		{
			auto const ref_der_niv = rafineur->GetLevel(niveau_max);
			nombre_sommets = ref_der_niv.GetNumVertices();
			nombre_polygones = ref_der_niv.GetNumFaces();

			auto premier_sommet = rafineur->GetNumVerticesTotal() - nombre_sommets;

			auto attr_N = corps_entree->attribut("N");

			m_corps.points()->reserve(nombre_sommets);
			m_corps.prims()->reserve(nombre_polygones);

			auto liste_points = m_corps.points();

			for (long vert = 0; vert < nombre_sommets; ++vert) {
				float const * pos = sommets[premier_sommet + vert].GetPosition();
				auto point = dls::math::vec3f(pos[0], pos[1], pos[2]);
				liste_points->pousse(point);
			}

			for (long face = 0; face < nombre_polygones; ++face) {
				auto fverts = ref_der_niv.GetFaceVertices(static_cast<int>(face));

				auto poly = Polygone::construit(&m_corps, type_polygone::FERME, fverts.size());

				for (int i = 0; i < fverts.size(); ++i) {
					poly->ajoute_sommet(fverts[i]);
				}
			}

			if (attr_N != nullptr) {
				/* À FAIRE : savoir si les normaux ont été inversé. */
				calcul_normaux(m_corps, attr_N->portee == portee_attr::PRIMITIVE, false);
			}
		}

		delete rafineur;

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_opensubdiv(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceOpenSubDiv>());
}

#pragma clang diagnostic pop
