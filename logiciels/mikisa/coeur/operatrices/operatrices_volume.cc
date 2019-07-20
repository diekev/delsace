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

#include "operatrices_volume.hh"

#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/moultfilage/boucle.hh"

#include "biblinternes/memoire/logeuse_memoire.hh"

#include "corps/collision.hh"
#include "corps/volume.hh"

#include "../chef_execution.hh"
#include "../contexte_evaluation.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceAdvectionVolume;
class OperatriceConditionLimite;
class OperatriceAjoutFlottabilite;
class OperatriceResolutionPression;

class IteratricePosition {
	limites3i m_lim;
	dls::math::vec3i m_etat;

public:
	IteratricePosition(limites3i const &lim)
		: m_lim(lim)
		, m_etat(lim.min)
	{}

	dls::math::vec3i suivante()
	{
		auto etat = m_etat;

		m_etat.x += 1;

		if (m_etat.x >= m_lim.max.x) {
			m_etat.x = m_lim.min.x;

			m_etat.y += 1;

			if (m_etat.y >= m_lim.max.y) {
				m_etat.y = m_lim.min.y;

				m_etat.z += 1;
			}
		}

		return etat;
	}

	bool fini() const
	{
		return m_etat.z >= m_lim.max.z;
	}
};

/* ************************************************************************** */

class OperatriceCreationVolume : public OperatriceCorps {
public:
	static constexpr auto NOM = "Créer volume";
	static constexpr auto AIDE = "";

	explicit OperatriceCreationVolume(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(0);
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
		INUTILISE(contexte);
		INUTILISE(donnees_aval);

		m_corps.reinitialise();

		auto gna = GNA();

		auto etendu = limites3f{};
		etendu.min = dls::math::vec3f(-1.0f);
		etendu.max = dls::math::vec3f(1.0f);
		auto fenetre_donnees = etendu;
		auto taille_voxel = 2.0f / 32.0f;

		auto volume = memoire::loge<Volume>("Volume");
		auto grille_scalaire = memoire::loge<Grille<float>>("grille", etendu, fenetre_donnees, taille_voxel);

		auto limites = limites3i{};
		limites.min = dls::math::vec3i(0);
		limites.max = grille_scalaire->resolution();

		auto iter = IteratricePosition(limites);

		while (!iter.fini()) {
			auto pos = iter.suivante();
			grille_scalaire->valeur(pos, gna.uniforme(0.0f, 1.0f));
		}

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

static auto initialise_limites3f()
{
	auto limites = limites3f{};
	limites.min = dls::math::vec3f( std::numeric_limits<float>::max());
	limites.max = dls::math::vec3f(-std::numeric_limits<float>::max());
	return limites;
}

/* Retourne les limites en espace globale des points du corps. */
static auto calcule_limites_points(Corps const &corps)
{
	auto limites = initialise_limites3f();

	auto const &points = corps.points();

	for (auto i = 0; i < points->taille(); ++i) {
		auto point = corps.point_transforme(i);
		dls::math::extrait_min_max(point, limites.min, limites.max);
	}

	return limites;
}

class OperatriceMaillageVersVolume : public OperatriceCorps {
public:
	static constexpr auto NOM = "Maillage vers Volume";
	static constexpr auto AIDE = "";

	explicit OperatriceMaillageVersVolume(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_maillage_vers_volume.jo";
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
			ajoute_avertissement("Aucun corps connecté !");
			return EXECUTION_ECHOUEE;
		}

		auto prims = corps_entree->prims();

		if (prims->taille() == 0) {
			ajoute_avertissement("Aucune primitive dans le corps !");
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("maillage vers volume");

		/* calcul boite englobante */
		auto liste_points = corps_entree->points();
		auto limites = calcule_limites_points(*corps_entree);

		auto const taille_voxel = evalue_decimal("taille_voxel");
		auto const densite = evalue_decimal("densité");

		auto volume =  memoire::loge<Volume>("Volume");
		auto grille_scalaire =  memoire::loge<Grille<float>>("grille", limites, limites, taille_voxel);
		auto dim = limites.taille();
		auto res = grille_scalaire->resolution();

		boucle_parallele(tbb::blocked_range<int>(0, res.z),
						 [&](tbb::blocked_range<int> const &plage)
		{
			auto rayon = Rayon{};

			auto lims = limites3i{};
			lims.min = dls::math::vec3i(0, 0, plage.begin());
			lims.max = dls::math::vec3i(res.x, res.y, plage.end());

			auto iter = IteratricePosition(lims);

			while (!iter.fini()) {
				if (chef->interrompu()) {
					return;
				}

				auto isp = iter.suivante();
				rayon.origine = grille_scalaire->index_vers_monde(isp);

				auto axis = axe_dominant_abs(rayon.origine);

				rayon.direction = dls::math::vec3f(0.0f, 0.0f, 0.0f);
				rayon.direction[axis] = 1.0f;

				auto distance = dim.x * 0.5f;

				auto index_prim = cherche_collision(corps_entree, rayon, distance);

				if (index_prim < 0) {
					continue;
				}

				auto prim_coll = corps_entree->prims()->prim(index_prim);

				/* calcul normal au niveau de la prim */
				auto poly = dynamic_cast<Polygone *>(prim_coll);
				auto const &v0 = liste_points->point(poly->index_point(0));
				auto const &v1 = liste_points->point(poly->index_point(1));
				auto const &v2 = liste_points->point(poly->index_point(2));

				auto const e1 = v1 - v0;
				auto const e2 = v2 - v0;
				auto nor_poly = normalise(produit_croix(e1, e2));

				//grille_scalaire->valeur(x, y, z, dist(rng));
				if (produit_scalaire(nor_poly, rayon.direction) < 0.0f) {
					continue;
				}

				rayon.direction = -rayon.direction;
				distance = dim.x * 0.5f;

				if (cherche_collision(corps_entree, rayon, distance) != -1) {
					grille_scalaire->valeur(
								static_cast<size_t>(isp.x),
								static_cast<size_t>(isp.y),
								static_cast<size_t>(isp.z),
								densite);
				}
			}

			auto delta = static_cast<float>(plage.end() - plage.begin());
			auto total = static_cast<float>(res.x);

			chef->indique_progression_parallele(delta / total * 100.0f);
		});

		chef->indique_progression(100.0f);

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/* À FAIRE : bibliothèque de bruit */
#include "biblinternes/math/bruit.hh"

struct FBM {
	float echelle = 1.0f;
	float inv_echelle = 1.0f / echelle;
	float octaves = 8.0f * 1.618f;
	float gain = 1.0f;
	float lacunarite = 1.0f;

	dls::math::BruitPerlin3D bruit{};

	float eval(dls::math::vec3f const &P) const
	{
		/* mise à l'échelle du point d'échantillonage */
		auto p = P * this->inv_echelle;

		/* initialisation des variables */
		auto resultat = 0.0f;
		auto contribuation_octave = 1.0f;
		auto octave = this->octaves;

		for (; octave > 1.0f; octave -= 1.0f) {
			resultat += bruit(p) * contribuation_octave;
			contribuation_octave *= this->gain;
			p *= this->lacunarite;
		}

		if (octave > 0.0f) {
			resultat += bruit(p) * contribuation_octave * octave;
		}

		return resultat;
	}
};

static void rasterise_polygone(
		Corps const &corps,
		Polygone &poly,
		Rasteriseur<float> &rast,
		float rayon,
		float densite,
		GNA &gna,
		int nombre_echantillons,
		FBM *fbm)
{
	// le système de coordonnées pour un polygone est (dP/du, dP/dv, N)

	// À FAIRE : n-gons
	if (poly.nombre_sommets() != 4) {
		return;
	}

	auto p0 = corps.point_transforme(poly.index_point(0));
	auto p1 = corps.point_transforme(poly.index_point(1));
	auto p3 = corps.point_transforme(poly.index_point(3));

	auto e1 = p1 - p0;
	auto e2 = p3 - p0;

	auto N = normalise(produit_croix(e1, e2));

	auto st = dls::math::vec2f();

	for (auto i = 0; i < nombre_echantillons; ++i) {
		/* coordonnées aléatoires sur (dP/du, dP/dv) */
		st.x = gna.uniforme(0.0f, 1.0f);
		st.y = gna.uniforme(0.0f, 1.0f);

		/* déplacement aléatoire le long du normal */
		auto u = gna.uniforme(0.0f, 1.0f);

		/* trouve les vecteurs de base */
		// À FAIRE : interpole le normal si lisse
		auto wsP = p0 + e1 * st.x + e2 * st.y;
		wsP += N * u * rayon;

		if (fbm != nullptr) {
			auto nsP = dls::math::vec3f(st[0], st[1], u);
			auto disp = rayon * fbm->eval(nsP)/* * amplitude*/;
			wsP += N * disp;
		}

		// transforme la position en espace unitaire
		auto Eu = rast.grille().monde_vers_continue(wsP);
		rast.ecris_trilineaire(Eu, densite);
	}
}

static auto echantillone_disque(GNA &gna)
{
	auto x = 0.0f;
	auto y = 0.0f;
	auto l = 0.0f;

	do {
		x = gna.uniforme(-1.0f, 1.0f);
		y = gna.uniforme(-1.0f, 1.0f);
		l = std::sqrt(x * x + y * y);
	} while (l >= 1.0f || l == 0.0f);

	return dls::math::vec2f(x, y);
}

static void rasterise_ligne(
		Corps const &corps,
		Polygone &poly,
		Rasteriseur<float> &rast,
		float rayon,
		float densite,
		GNA &gna,
		int nombre_echantillons,
		FBM *fbm)
{
	// le système de coordonnées pour une ligne est (NxT, N, T)

	for (auto i = 0; i < poly.nombre_segments(); ++i) {
		auto p0 = corps.point_transforme(poly.index_point(i));
		auto p1 = corps.point_transforme(poly.index_point(i + 1));

		for (auto j = 0; j < nombre_echantillons; ++j) {
			// trouve une position aléatoire le long du segment
			auto u = gna.uniforme(0.0f, 1.0f);

			// trouve la tengeante, pour une courbe il faudra interpoler, mais
			// pour une ligne, c'est simplement la ligne
			auto T = (p1 - p0);

			// trouve la normale, simplement un vecteur orthogonal à T
			auto N = dls::math::vec3f(-T.y, T.x, 0.0f);

			// trouve le troisième axe
			auto NxT = produit_croix(N, T);

			auto st = echantillone_disque(gna);

			// tire l'échantillon
			auto wsP = (1.0f - u) * p0 + u * p1;
			wsP += st.x * NxT * rayon;
			wsP += st.y * N * rayon;

			if (fbm != nullptr) {
				auto nsP = dls::math::vec3f(st[0], st[1], u);
				auto wsRadial = normalise(st.x * NxT + st.y * N);
				auto disp = rayon * fbm->eval(nsP)/* * amplitude*/;
				wsP += wsRadial * disp;
			}

			// transforme la position en espace unitaire
			auto Eu = rast.grille().monde_vers_continue(wsP);
			rast.ecris_trilineaire(Eu, densite);
		}
	}
}

class OpRasterisationPrimitive : public OperatriceCorps {
public:
	static constexpr auto NOM = "Rastérisation Prim";
	static constexpr auto AIDE = "";

	explicit OpRasterisationPrimitive(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_rasterisation_prim.jo";
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
			this->ajoute_avertissement("Aucun corps en entrée");
			return EXECUTION_ECHOUEE;
		}

		auto prims_entree = corps_entree->prims();

		if (prims_entree->taille() == 0) {
			this->ajoute_avertissement("Aucune primitive en entrée");
			return EXECUTION_ECHOUEE;
		}

		auto chef = contexte.chef;
		chef->demarre_evaluation("rastérisation prim");

		/* paramètres */
		auto const rayon = evalue_decimal("rayon");
		auto const taille_voxel = evalue_decimal("taille_voxel");
		auto const graine = evalue_entier("graine");
		auto const densite = evalue_decimal("densité");
		auto const nombre_echantillons = evalue_entier("nombre_échantillons");

		/* calcul les limites des primitives d'entrées */
		auto limites = calcule_limites_points(*corps_entree);

		/* À FAIRE : prendre en compte le déplacement pour le bruit */
		limites.etends(dls::math::vec3f(rayon));

		auto gna = GNA(graine);

		auto volume = memoire::loge<Volume>("Volume");
		auto grille_scalaire = memoire::loge<Grille<float>>("grille", limites, limites, taille_voxel);

		auto fbm = FBM{};

		auto rast = Rasteriseur(*grille_scalaire);

		for (auto i = 0; i < prims_entree->taille(); ++i) {
			auto prim = prims_entree->prim(i);

			if (prim->type_prim() == type_primitive::POLYGONE) {
				auto poly = dynamic_cast<Polygone *>(prim);

				if (poly->type == type_polygone::FERME) {
					rasterise_polygone(*corps_entree, *poly, rast, rayon, densite, gna, nombre_echantillons, &fbm);
				}
				else {
					rasterise_ligne(*corps_entree, *poly, rast, rayon, densite, gna, nombre_echantillons, &fbm);
				}
			}

			chef->indique_progression(static_cast<float>(i + 1) / static_cast<float>(prims_entree->taille()) * 100.0f);
		}

		/* À FAIRE : filtrage de l'échantillonage. */

		chef->indique_progression(100.0f);

		volume->grille = grille_scalaire;
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_volume(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceCreationVolume>());
	usine.enregistre_type(cree_desc<OperatriceMaillageVersVolume>());
	usine.enregistre_type(cree_desc<OpRasterisationPrimitive>());
}

#pragma clang diagnostic pop
