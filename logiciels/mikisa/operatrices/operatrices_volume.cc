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
#include "coeur/delegue_hbe.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/echantillonnage_volume.hh"
#include "corps/limites_corps.hh"
#include "corps/iter_volume.hh"
#include "corps/volume.hh"

#include "poseidon/bruit_vaguelette.hh"

#include "outils_visualisation.hh"

/* ************************************************************************** */

static int cree_volume(
		OperatriceCorps &op,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval)
{
	INUTILISE(contexte);
	INUTILISE(donnees_aval);

	op.corps()->reinitialise();

	auto min = op.evalue_vecteur("limite_min");
	auto max = op.evalue_vecteur("limite_max");

	auto desc = desc_grille_3d{};
	desc.etendue.min = min;
	desc.etendue.max = max;
	desc.fenetre_donnees = desc.etendue;
	desc.taille_voxel = static_cast<double>(op.evalue_decimal("taille_voxel"));

	auto graine = op.evalue_entier("graine");

	auto bruit = bruit_vaguelette::construit(graine);
	bruit.dx = static_cast<float>(desc.taille_voxel);
	bruit.taille_grille_inv = 1.0f / static_cast<float>(desc.taille_voxel);
	bruit.decalage_pos = op.evalue_vecteur("décalage_pos");
	bruit.echelle_pos = op.evalue_vecteur("échelle_pos");
	bruit.decalage_valeur = op.evalue_decimal("décalage_valeur");
	bruit.echelle_valeur = op.evalue_decimal("échelle_valeur");
	bruit.restreint = op.evalue_bool("restreint");
	bruit.restreint_neg = op.evalue_decimal("restreint_neg");
	bruit.restraint_pos = op.evalue_decimal("restreint_pos");
	bruit.temps_anim = op.evalue_decimal("temps_anim");

	auto volume = memoire::loge<Volume>("Volume");
	auto grille_scalaire = memoire::loge<grille_dense_3d<float>>("grille", desc);

	auto limites = limites3i{};
	limites.min = dls::math::vec3i(0);
	limites.max = grille_scalaire->desc().resolution;

	auto iter = IteratricePosition(limites);

	std::cerr << "Nombre de voxels : " << grille_scalaire->nombre_elements() << '\n';

	while (!iter.fini()) {
		auto pos = iter.suivante();
		auto idx = grille_scalaire->calcul_index(pos);
		auto pos_mnd = dls::math::discret_vers_continu<float>(pos);
		grille_scalaire->valeur(idx, bruit.evalue(pos_mnd));
	}

	volume->grille = grille_scalaire;
	op.corps()->prims()->pousse(volume);

	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

static int maillage_vers_volume(
		OperatriceCorps &op,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		Corps const &corps_entree)
{
	INUTILISE(donnees_aval);

	op.corps()->reinitialise();

	auto prims = corps_entree.prims();

	if (prims->taille() == 0) {
		op.ajoute_avertissement("Aucune primitive dans le corps !");
		return EXECUTION_ECHOUEE;
	}

	auto chef = contexte.chef;
	chef->demarre_evaluation("maillage vers volume");

	/* calcul boite englobante */
	auto limites = calcule_limites_mondiales_corps(corps_entree);

	auto const taille_voxel = op.evalue_decimal("taille_voxel");
	auto const densite = op.evalue_decimal("densité");

#if 1
	/* crée une grille éparse */
	auto limites_grille = limites3f{};
	limites_grille.min = limites.min * 2.0f;
	limites_grille.max = limites.max * 2.0f;

	limites_grille = limites;

	auto desc_volume = desc_grille_3d{};
	desc_volume.etendue = limites_grille;
	desc_volume.fenetre_donnees = limites_grille;
	desc_volume.taille_voxel = taille_voxel;
	desc_volume.type_donnees = type_grille::R32;

	auto grille = memoire::loge<grille_eparse<float>>("grille_eparse", desc_volume);
	grille->assure_tuiles(limites);

	auto volume =  memoire::loge<Volume>("Volume");
	volume->grille = grille;

	auto plg = grille->plage();

	auto delegue_prims = DeleguePrim(corps_entree);
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

					auto dist_max = grille->desc().taille_voxel;
					dist_max *= dist_max;
#if 1
					auto dpp = cherche_point_plus_proche(arbre_hbe, delegue_prims, rayon.origine, dist_max);

					if (dpp.distance_carree < -0.5) {
						continue;
					}

					if (dpp.distance_carree >= dist_max) {
						continue;
					}

					tuile->donnees[index_tuile] = densite;
#else

					auto axis = axe_dominant_abs(rayon.origine);

					rayon.direction = dls::math::vec3d(0.0);
					rayon.direction[axis] = 1.0;
					calcul_direction_inverse(rayon);

					auto accumulatrice = AccumulatriceTraverse(rayon.origine);
					traverse(arbre_hbe, delegue_prims, rayon, accumulatrice);

					if (accumulatrice.intersection().touche && accumulatrice.nombre_touche() % 2 == 1) {
						tuile->donnees[index_tuile] = densite;
					}
#endif
				}
			}
		}
	}

	grille->elague();

#else
	auto desc = desc_grille_3d{};
	desc.etendue = limites;
	desc.fenetre_donnees = limites;
	desc.taille_voxel = taille_voxel;

	auto volume =  memoire::loge<Volume>("Volume");
	auto grille_scalaire =  memoire::loge<Grille<float>>("grille", desc);
	auto res = grille_scalaire->resolution();

	auto delegue_prims = DeleguePrim(corps_entree);
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
				grille_scalaire->valeur(isp.x, isp.y, isp.z, densite);
			}
		}

		auto delta = static_cast<float>(plage.end() - plage.begin());
		auto total = static_cast<float>(res.x);

		chef->indique_progression_parallele(delta / total * 100.0f);
	});

	chef->indique_progression(100.0f);

	volume->grille = grille_scalaire;
#endif

	op.corps()->prims()->pousse(volume);

	return EXECUTION_REUSSIE;
}

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
		auto Eu = rast.grille().monde_vers_continu(wsP);
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
			auto Eu = rast.grille().monde_vers_continu(wsP);
			rast.ecris_trilineaire(Eu, densite);
		}
	}
}

static int ratisse_primitives(
		OperatriceCorps &op,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		Corps const &corps_entree)
{
	INUTILISE(donnees_aval);

	op.corps()->reinitialise();

	auto prims_entree = corps_entree.prims();

	if (prims_entree->taille() == 0) {
		op.ajoute_avertissement("Aucune primitive en entrée");
		return EXECUTION_ECHOUEE;
	}

	auto chef = contexte.chef;
	chef->demarre_evaluation("rastérisation prim");

	/* paramètres */
	auto const rayon = op.evalue_decimal("rayon");
	auto const taille_voxel = op.evalue_decimal("taille_voxel");
	auto const graine = op.evalue_entier("graine");
	auto const densite = op.evalue_decimal("densité");
	auto const nombre_echantillons = op.evalue_entier("nombre_échantillons");

	/* calcul les limites des primitives d'entrées */
	auto limites = calcule_limites_mondiales_corps(corps_entree);

	/* À FAIRE : prendre en compte le déplacement pour le bruit */
	limites.etends(dls::math::vec3f(rayon));

	auto gna = GNA(graine);

	auto desc = desc_grille_3d{};
	desc.etendue = limites;
	desc.fenetre_donnees = limites;
	desc.taille_voxel = static_cast<double>(taille_voxel);

	auto volume = memoire::loge<Volume>("Volume");
	auto grille_scalaire = memoire::loge<grille_dense_3d<float>>("grille", desc);

	auto fbm = FBM{};

	auto rast = Rasteriseur(*grille_scalaire);

	for (auto i = 0; i < prims_entree->taille(); ++i) {
		auto prim = prims_entree->prim(i);

		if (prim->type_prim() == type_primitive::POLYGONE) {
			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type == type_polygone::FERME) {
				rasterise_polygone(corps_entree, *poly, rast, rayon, densite, gna, nombre_echantillons, &fbm);
			}
			else {
				rasterise_ligne(corps_entree, *poly, rast, rayon, densite, gna, nombre_echantillons, &fbm);
			}
		}

		chef->indique_progression(static_cast<float>(i + 1) / static_cast<float>(prims_entree->taille()) * 100.0f);
	}

	/* À FAIRE : filtrage de l'échantillonage. */

	chef->indique_progression(100.0f);

	volume->grille = grille_scalaire;
	op.corps()->prims()->pousse(volume);

	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

static int reechantillonne_volume(
		OperatriceCorps &op,
		ContexteEvaluation const &contexte,
		DonneesAval *donnees_aval,
		Corps const &corps_entree)
{
	INUTILISE(donnees_aval);
	INUTILISE(contexte);

	op.corps()->reinitialise();

	auto prims = corps_entree.prims();

	if (prims->taille() == 0) {
		op.ajoute_avertissement("Aucune primitive en entrée !");
		return EXECUTION_ECHOUEE;
	}

	auto volume_entree = static_cast<Volume *>(nullptr);

	for (auto i = 0; i < prims->taille(); ++i) {
		auto prim = prims->prim(i);

		if (prim->type_prim() == type_primitive::VOLUME) {
			volume_entree = dynamic_cast<Volume *>(prim);
			break;
		}
	}

	if (volume_entree == nullptr) {
		op.ajoute_avertissement("Aucun volume en entrée !");
		return EXECUTION_ECHOUEE;
	}

	auto grille_entree = volume_entree->grille;

	if (grille_entree->desc().type_donnees != type_grille::R32) {
		op.ajoute_avertissement("La grille n'est pas scalaire !");
		return EXECUTION_ECHOUEE;
	}

	auto grille_scalaire = dynamic_cast<grille_dense_3d<float> *>(grille_entree);

	auto resultat = reechantillonne(*grille_scalaire, grille_scalaire->desc().taille_voxel * 2.0);

	auto volume = memoire::loge<Volume>("Volume");
	auto grille = memoire::loge<grille_dense_3d<float>>("grille");

	volume->grille = grille;

	grille->echange(resultat);

	op.corps()->ajoute_primitive(volume);

	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

/**
 * Implémentation de volume temporel basée sur
 * « Efficient Rendering of Volumetric Motion Blur using Temporally Unstructured Volumes »
 * par Magnus Wrenninge, Pixar Animation Studio.
 * http://magnuswrenninge.com/wp-content/uploads/2010/03/Wrenninge-EfficientRenderingOfVolumetricMotionBlur.pdf
 *
 * Ce type de volume est utile pour rendre le flou directionel ou encore
 * réanimer les simumlations de fluide.
 */

struct Courbe {
	float valeur{};
	float temps{};
};

using grille_auxilliaire = grille_eparse<dls::tableau<Courbe>>;

struct grille_temporelle : public base_grille_3d {
	using type_valeur = float;
	using type_topologie = dls::tableau<long>;

	struct tuile_temporelle {
		int decalage[TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE + 1];
		float *valeurs = nullptr;
		float *temps = nullptr;
		dls::math::vec3i min{};
		dls::math::vec3i max{};
	};

private:
	type_topologie m_index_tuiles{};
	dls::tableau<tuile_temporelle *> m_tuiles{};

	int m_tuiles_x = 0;
	int m_tuiles_y = 0;
	int m_tuiles_z = 0;

	type_valeur m_arriere_plan = type_valeur(0);

	bool hors_des_limites(int i, int j, int k) const
	{
		if (i < 0 || i >= m_tuiles_x) {
			return true;
		}

		if (j < 0 || j >= m_tuiles_y) {
			return true;
		}

		if (k < 0 || k >= m_tuiles_z) {
			return true;
		}

		return false;
	}

	inline long index_tuile(long i, long j, long k) const
	{
		return i + (j + k * m_tuiles_y) * m_tuiles_x;
	}

	inline int converti_nombre_tuile(int i) const
	{
		return i / TAILLE_TUILE + ((i % TAILLE_TUILE) != 0);
	}

public:
	using plage_tuile = dls::plage_continue<tuile_temporelle *>;
	using plage_tuile_const = dls::plage_continue<tuile_temporelle * const>;

	grille_temporelle(desc_grille_3d const &descr)
		: base_grille_3d(descr)
	{
		m_tuiles_x = converti_nombre_tuile(m_desc.resolution.x);
		m_tuiles_y = converti_nombre_tuile(m_desc.resolution.y);
		m_tuiles_z = converti_nombre_tuile(m_desc.resolution.z);

		auto nombre_tuiles = m_tuiles_x * m_tuiles_y * m_tuiles_z;

		m_desc.type_donnees = type_grille::R32;
		m_index_tuiles.redimensionne(nombre_tuiles, -1l);
	}

	~grille_temporelle() override
	{
		for (auto t : m_tuiles) {
			auto nombre_valeurs = t->decalage[TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE];
			memoire::deloge_tableau("tuile_temp::valeurs", t->valeurs, nombre_valeurs);
			memoire::deloge_tableau("tuile_temp::temps", t->temps, nombre_valeurs);
			memoire::deloge("tuile", t);
		}
	}

	void assure_tuiles(limites3f const &fenetre_donnees)
	{
		auto min = monde_vers_index(fenetre_donnees.min);
		auto max = monde_vers_index(fenetre_donnees.max);

		assure_tuiles(limites3i{min, max});
	}

	void assure_tuiles(limites3i const &fenetre_donnees)
	{
		auto min_tx = fenetre_donnees.min.x / TAILLE_TUILE;
		auto min_ty = fenetre_donnees.min.y / TAILLE_TUILE;
		auto min_tz = fenetre_donnees.min.z / TAILLE_TUILE;

		auto max_tx = converti_nombre_tuile(fenetre_donnees.max.x);
		auto max_ty = converti_nombre_tuile(fenetre_donnees.max.y);
		auto max_tz = converti_nombre_tuile(fenetre_donnees.max.z);

		for (auto z = min_tz; z < max_tz; ++z) {
			for (auto y = min_ty; y < max_ty; ++y) {
				for (auto x = min_tx; x < max_tx; ++x) {
					auto idx_tuile = index_tuile(x, y, z);

					if (m_index_tuiles[idx_tuile] != -1) {
						continue;
					}

					auto t = memoire::loge<tuile_temporelle>("tuile");
					t->min = dls::math::vec3i(x, y, z) * TAILLE_TUILE;
					t->max = t->min + dls::math::vec3i(TAILLE_TUILE);

					m_index_tuiles[idx_tuile] = m_tuiles.taille();
					m_tuiles.pousse(t);
				}
			}
		}
	}

	float valeur(int i, int j, int k) const
	{
		// trouve la tuile
		auto it = i / TAILLE_TUILE;
		auto jt = j / TAILLE_TUILE;
		auto kt = k / TAILLE_TUILE;

		if (hors_des_limites(it, jt, kt)) {
			return m_arriere_plan;
		}

		auto idx_tuile = index_tuile(it, jt, kt);

		if (m_index_tuiles[idx_tuile] == -1) {
			return m_arriere_plan;
		}

//		auto t = m_tuiles[idx_tuile];

//		// calcul l'index dans la tuile
//		auto xt = i - it * TAILLE_TUILE;
//		auto yt = j - jt * TAILLE_TUILE;
//		auto zt = k - kt * TAILLE_TUILE;

		return 0.0f;// t->donnees[static_cast<size_t>(xt + (yt + zt * TAILLE_TUILE) * TAILLE_TUILE)];
	}

	tuile_temporelle *tuile_par_index(long idx) const
	{
		auto idx_tuile = m_index_tuiles[idx];

		if (idx_tuile == -1) {
			return nullptr;
		}

		return m_tuiles[idx_tuile];
	}

	tuile_temporelle *cree_tuile(dls::math::vec3i const &co)
	{
		auto idx = dls::math::calcul_index(co / TAILLE_TUILE, res_tuile());
		auto t = memoire::loge<tuile_temporelle>("tuile");
		t->min = co;
		t->max = t->min + dls::math::vec3i(TAILLE_TUILE);
		m_index_tuiles[idx] = m_tuiles.taille();
		m_tuiles.pousse(t);
		return t;
	}

	long nombre_tuiles() const
	{
		return m_tuiles.taille();
	}

	type_topologie const &topologie() const
	{
		return m_index_tuiles;
	}

	dls::math::vec3i res_tuile() const
	{
		return dls::math::vec3i(m_tuiles_x, m_tuiles_y, m_tuiles_z);
	}

	plage_tuile plage()
	{
		if (m_tuiles.taille() == 0) {
			return plage_tuile(nullptr, nullptr);
		}

		auto d = &m_tuiles[0];
		auto f = d + m_tuiles.taille();
		return plage_tuile(d, f);
	}

	plage_tuile_const plage() const
	{
		if (m_tuiles.taille() == 0) {
			return plage_tuile_const(nullptr, nullptr);
		}

		auto d = &m_tuiles[0];
		auto f = d + m_tuiles.taille();
		return plage_tuile_const(d, f);
	}

	base_grille *copie() const override
	{
		auto grille = memoire::loge<grille_temporelle>("grille", desc());
		grille->m_arriere_plan = this->m_arriere_plan;
		grille->m_index_tuiles = this->m_index_tuiles;
		grille->m_tuiles = this->m_tuiles;

		return grille;
	}

	bool est_eparse() const override
	{
		return true;
	}

	void echange(grille_temporelle &autre)
	{
		std::swap(m_desc.etendue, autre.m_desc.etendue);
		std::swap(m_desc.resolution, autre.m_desc.resolution);
		std::swap(m_desc.fenetre_donnees, autre.m_desc.fenetre_donnees);
		std::swap(m_desc.taille_voxel, autre.m_desc.taille_voxel);

		m_index_tuiles.echange(autre.m_index_tuiles);
		m_tuiles.echange(autre.m_tuiles);

		std::swap(m_arriere_plan, autre.m_arriere_plan);
		std::swap(m_tuiles_x, autre.m_tuiles_x);
		std::swap(m_tuiles_y, autre.m_tuiles_y);
		std::swap(m_tuiles_z, autre.m_tuiles_z);
	}
};


static void ajoute_volume_temps(
		grille_auxilliaire &grille_aux,
		grille_eparse<float> const &grille,
		float temps,
		float dt,
		bool debut)
{
	auto plg = grille.plage();
	auto res = grille.res_tuile();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		auto co_tuile = tuile->min / TAILLE_TUILE;
		auto idx_tuile = dls::math::calcul_index(co_tuile, res);

		auto tuile_aux = grille_aux.tuile_par_index(idx_tuile);

		/* Étape 1 : entresecte les topologies et alloue les tuiles non-déjà
		 * présentes. Pour s'assurere que les tuiles possèdent des valeurs
		 * sensées, on les initialise avec une valeur zéro, pour
		 * temps = temps - dt, autrement un échantillonage entre le temps dans
		 * (temps - dt, temps) renverrai une valeur qui n'existait pas.
		 *
		 * Mais il faut le faire uniquement si ce n'est pas le temps de début de
		 * la génération de données (p.e. début de simulation).
		 */
		if (tuile_aux == nullptr) {
			tuile_aux = grille_aux.cree_tuile(tuile->min);

			if (!debut) {
				for (auto i = 0; i < TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE; ++i) {
					tuile_aux->donnees[i].pousse({ 0.0f, temps - dt });
				}
			}
		}

		/* Étape 2 : insère les valeurs pour ce temps.
		 */
		for (auto i = 0; i < TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE; ++i) {
			tuile_aux->donnees[i].pousse({ tuile->donnees[i], temps });
		}

		tuile_aux->visite = true;
	}

	/* Étape 3 : il reste le cas où une tuile existait avant et n'existe plus
	 * maintenant -> ajoute valeur (0, temps).
	 */
	auto plg_aux = grille_aux.plage();

	while (!plg_aux.est_finie()) {
		auto tuile = plg_aux.front();
		plg_aux.effronte();

		if (tuile->visite) {
			/* remet à zéro pour après */
			tuile->visite = false;
		}
		else {
			for (auto i = 0; i < TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE; ++i) {
				tuile->donnees[i].pousse({ 0.0f, temps });
			}
		}
	}
}

static void simplifie_courbes(
		grille_auxilliaire &grille)
{
	auto plg = grille.plage();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		for (auto i = 0; i < TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE; ++i) {
			auto &donnees = tuile->donnees[i];

			if (donnees.taille() == 0 || donnees.taille() == 1) {
				continue;
			}

			/* À FAIRE : enlève les valeurs répétées puisque nous présumons une
			 * entrepolation linéaire */

			/* À FAIRE : enlève les points les moins saillants */
			auto eps_u = 1.0f; // À FAIRE : paramètre utilisateur
			auto eps_r = eps_u * (donnees.back().valeur - donnees.front().valeur);

			INUTILISE(eps_r);
		}
	}
}

static auto compresse_grille_aux(
		grille_auxilliaire &grille)
{
	auto grille_temp = memoire::loge<grille_temporelle>("grille", grille.desc());

	auto plg_aux = grille.plage();

	while (!plg_aux.est_finie()) {
		auto tuile_aux = plg_aux.front();
		plg_aux.effronte();

		auto tuile_temp = grille_temp->cree_tuile(tuile_aux->min);
		auto nombre_valeurs = 0l;

		for (auto i = 0; i < TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE; ++i) {
			nombre_valeurs += tuile_aux->donnees[i].taille();
		}

		tuile_temp->valeurs = memoire::loge_tableau<float>("tuile_temp::valeurs", nombre_valeurs);
		tuile_temp->temps = memoire::loge_tableau<float>("tuile_temp::temps", nombre_valeurs);

		tuile_temp->decalage[0] = 0;
		nombre_valeurs = 0;

		for (auto i = 0; i < TAILLE_TUILE * TAILLE_TUILE * TAILLE_TUILE; ++i) {
			auto const &courbe = tuile_aux->donnees[i];

			for (auto j = 0; j < courbe.taille(); ++j) {
				tuile_temp->valeurs[nombre_valeurs + j] = courbe[j].valeur;
				tuile_temp->temps[nombre_valeurs + j] = courbe[j].temps;
			}

			nombre_valeurs += courbe.taille();
			tuile_temp->decalage[i + 1] = static_cast<int>(nombre_valeurs);
		}
	}

	return grille_temp;
}

static auto obtiens_grille(float temps)
{
	auto min = dls::math::vec3f(-1.0f);
	auto max = dls::math::vec3f( 1.0f);

	auto desc = desc_grille_3d{};
	desc.etendue.min = min;
	desc.etendue.max = max;
	desc.fenetre_donnees = desc.etendue;
	desc.taille_voxel = 0.1;

	auto graine = 1;

	auto bruit = bruit_vaguelette::construit(graine);
	bruit.dx = static_cast<float>(desc.taille_voxel);
	bruit.taille_grille_inv = 1.0f / static_cast<float>(desc.taille_voxel);
	bruit.decalage_pos = dls::math::vec3f(45.0f);
	bruit.echelle_pos = dls::math::vec3f( 1.0f);
	bruit.decalage_valeur = 0.0f;
	bruit.echelle_valeur = 1.0f;
	bruit.restreint = false;
	bruit.restreint_neg = 0.0f;
	bruit.restraint_pos = 1.0f;
	bruit.temps_anim = temps;

	auto grille = memoire::loge<grille_eparse<float>>("grille", desc);
	grille->assure_tuiles(desc.etendue);

	auto plg = grille->plage();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		auto index_tuile = 0;
		for (auto k = 0; k < TAILLE_TUILE; ++k) {
			for (auto j = 0; j < TAILLE_TUILE; ++j) {
				for (auto i = 0; i < TAILLE_TUILE; ++i, ++index_tuile) {
					auto pos_tuile = tuile->min;
					pos_tuile.x += i;
					pos_tuile.y += j;
					pos_tuile.z += k;

					auto mnd = grille->index_vers_monde(pos_tuile);

					tuile->donnees[index_tuile] = bruit.evalue(mnd);
				}
			}
		}
	}

	return grille;
}

static auto cree_volume_temporelle(float temps_courant)
{
	float const temps[3] = { -1.0f, 0.0f, 1.0f };

	auto grille_aux = static_cast<grille_auxilliaire *>(nullptr);

	for (auto i = 0; i < 3; ++i) {
		auto grille = obtiens_grille(temps_courant + temps[i]);

		if (i == 0) {
			grille_aux = memoire::loge<grille_auxilliaire>("grille_auxilliaire", grille->desc());
		}

		ajoute_volume_temps(*grille_aux, *grille, temps[i], 1.0f, i == 0);

		memoire::deloge("grille", grille);
	}

	simplifie_courbes(*grille_aux);

	auto grille_temp = compresse_grille_aux(*grille_aux);

	memoire::deloge("grille_auxilliaire", grille_aux);

	return grille_temp;
}

static auto echantillonne_grille_temp(
		grille_temporelle const &grille_temp,
		float temps)
{
	auto grille = memoire::loge<grille_eparse<float>>("grille", grille_temp.desc());
	grille->assure_tuiles(grille_temp.desc().etendue);

	auto plg = grille->plage();

	while (!plg.est_finie()) {
		auto tuile = plg.front();
		plg.effronte();

		auto min_tuile = tuile->min / TAILLE_TUILE;
		auto idx_tuile = dls::math::calcul_index(min_tuile, grille->res_tuile());
		auto tuile_temp = grille_temp.tuile_par_index(idx_tuile);

		auto index_tuile = 0;
		for (auto k = 0; k < TAILLE_TUILE; ++k) {
			for (auto j = 0; j < TAILLE_TUILE; ++j) {
				for (auto i = 0; i < TAILLE_TUILE; ++i, ++index_tuile) {
					auto pos_tuile = tuile->min;
					pos_tuile.x += i;
					pos_tuile.y += j;
					pos_tuile.z += k;

					//auto mnd = grille->index_vers_monde(pos_tuile);

					/* voisin le plus proche */
					auto deb = tuile_temp->decalage[index_tuile];
					auto dec = tuile_temp->decalage[index_tuile + 1] - deb;

					/* trouve le temps */
					for (auto x = deb; x < deb + dec; ++x) {
						if (tuile_temp->temps[x] == temps) {
							tuile->donnees[index_tuile] = tuile_temp->valeurs[x];
							break;
						}
					}
				}
			}
		}
	}

	return grille;
}

class OpCreationVolumeTemp : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Volume Temporel";
	static constexpr auto AIDE = "";

	OpCreationVolumeTemp(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
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
//		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

//		if (corps_entree == nullptr) {
//			this->ajoute_avertissement("Aucun corps n'est connecté !");
//			return EXECUTION_ECHOUEE;
//		}

//		if (!possede_volume(*corps_entree)) {
//			this->ajoute_avertissement("Aucun volume en entrée !");
//			return EXECUTION_ECHOUEE;
//		}

#if 0
		auto grille = obtiens_grille(static_cast<float>(contexte.temps_courant));
#else
		auto grille_temp = cree_volume_temporelle(static_cast<float>(contexte.temps_courant));
		auto grille = echantillonne_grille_temp(*grille_temp, 0.0f);

		memoire::deloge("grille", grille_temp);
#endif

		auto volume = memoire::loge<Volume>("Volume");
		volume->grille = grille;
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_volume(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc("Créer volume", "", "entreface/operatrice_creation_volume.jo", cree_volume, false));
	usine.enregistre_type(cree_desc("Maillage vers Volume", "", "entreface/operatrice_maillage_vers_volume.jo", maillage_vers_volume, false));
	usine.enregistre_type(cree_desc("Rastérisation Prim", "", "entreface/operatrice_rasterisation_prim.jo", ratisse_primitives, false));
	usine.enregistre_type(cree_desc("Rééchantillonne Volume", "", "", reechantillonne_volume, false));
	usine.enregistre_type(cree_desc<OpCreationVolumeTemp>());
}
