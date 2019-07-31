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

#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/plage.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/limites_corps.hh"
#include "corps/iter_volume.hh"
#include "corps/volume.hh"

#include "arbre_hbe.hh"
#include "delegue_hbe.hh"
#include "outils_visualisation.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

class OperatriceAdvectionVolume;
class OperatriceConditionLimite;
class OperatriceAjoutFlottabilite;
class OperatriceResolutionPression;

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

template <typename T>
static auto visualise_topologie(Corps &corps, grille_eparse<T> const &grille)
{
	auto limites = grille.desc().etendues;
	auto attr_C = corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
	auto plg = grille.plage();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		auto min_tuile = grille.index_vers_monde(tuile->min);
		auto max_tuile = grille.index_vers_monde(tuile->max);

		dessine_boite(corps, attr_C, min_tuile, max_tuile, dls::math::vec3f(0.1f, 0.1f, 0.8f));
	}

	dessine_boite(corps, attr_C, limites.min, limites.max, dls::math::vec3f(0.1f, 0.8f, 0.1f));
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
		auto limites = calcule_limites_mondiales_corps(*corps_entree);

		auto const taille_voxel = evalue_decimal("taille_voxel");
		auto const densite = evalue_decimal("densité");

#if 1
		/* crée une grille éparse */
		auto limites_grille = limites3f{};
		limites_grille.min = limites.min * 2.0f;
		limites_grille.max = limites.max * 2.0f;

		limites_grille = limites;

		auto desc_volume = description_volume{};
		desc_volume.etendues = limites_grille;
		desc_volume.fenetre_donnees = limites_grille;
		desc_volume.taille_voxel = taille_voxel;

		auto grille = memoire::loge<grille_eparse<float>>("grille_eparse", desc_volume);
		grille->assure_tuiles(limites);

		auto volume =  memoire::loge<Volume>("Volume");
		volume->grille = grille;

		auto plg = grille->plage();

		auto delegue_prims = DeleguePrim(*corps_entree);
		auto arbre_hbe = construit_arbre_hbe(delegue_prims, 24);

		while (!plg.est_finie()) {
			auto tuile = plg.front();
			plg.effronte();

			auto index_tuile = 0;

			auto rayon = dls::phys::rayond{};

			for (auto k = 0; k < TAILLE_TUILE; ++k) {
				for (auto j = 0; j < TAILLE_TUILE; ++j) {
					for (auto i = 0; i < TAILLE_TUILE; ++i, ++index_tuile) {
						auto pos_tuile = tuile->min;
						pos_tuile.x += i;
						pos_tuile.y += j;
						pos_tuile.z += k;

						auto mnd = grille->index_vers_monde(pos_tuile);

						rayon.origine.x = static_cast<double>(mnd.x);
						rayon.origine.y = static_cast<double>(mnd.y);
						rayon.origine.z = static_cast<double>(mnd.z);

						auto axis = axe_dominant_abs(rayon.origine);

						rayon.direction = dls::math::vec3d(0.0);
						rayon.direction[axis] = 1.0;
						calcul_direction_inverse(rayon);

						auto accumulatrice = AccumulatriceTraverse(rayon.origine);
						traverse(arbre_hbe, delegue_prims, rayon, accumulatrice);

						if (accumulatrice.intersection().touche && accumulatrice.nombre_touche() % 2 == 1) {
							tuile->donnees[index_tuile] = densite;
						}
					}
				}
			}
		}

		grille->elague();

		visualise_topologie(m_corps, *grille);

#else
		auto volume =  memoire::loge<Volume>("Volume");
		auto grille_scalaire =  memoire::loge<Grille<float>>("grille", limites, limites, taille_voxel);
		auto res = grille_scalaire->resolution();

		auto delegue_prims = DeleguePrim(*corps_entree);
		auto arbre_hbe = construit_arbre_hbe(delegue_prims, 24);

		boucle_parallele(tbb::blocked_range<int>(0, res.z),
						 [&](tbb::blocked_range<int> const &plage)
		{
			auto rayon = dls::phys::rayond{};

			auto lims = limites3i{};
			lims.min = dls::math::vec3i(0, 0, plage.begin());
			lims.max = dls::math::vec3i(res.x, res.y, plage.end());

			auto iter = IteratricePosition(lims);

			while (!iter.fini()) {
				if (chef->interrompu()) {
					return;
				}

				auto isp = iter.suivante();
				auto origine = grille_scalaire->index_vers_monde(isp);

				rayon.origine.x = static_cast<double>(origine.x);
				rayon.origine.y = static_cast<double>(origine.y);
				rayon.origine.z = static_cast<double>(origine.z);

				auto axis = axe_dominant_abs(rayon.origine);

				rayon.direction = dls::math::vec3d(0.0);
				rayon.direction[axis] = 1.0;
				calcul_direction_inverse(rayon);

				auto accumulatrice = AccumulatriceTraverse(rayon.origine);
				traverse(arbre_hbe, delegue_prims, rayon, accumulatrice);

				if (accumulatrice.intersection().touche && accumulatrice.nombre_touche() % 2 == 1) {
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
#endif
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
		auto limites = calcule_limites_mondiales_corps(*corps_entree);

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
