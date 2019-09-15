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

#include "biblinternes/bruit/evaluation.hh"
#include "biblinternes/bruit/turbulent.hh"
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/arbre_kd.hh"
#include "biblinternes/structures/plage.hh"

#include "coeur/chef_execution.hh"
#include "coeur/contexte_evaluation.hh"
#include "coeur/delegue_hbe.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "corps/limites_corps.hh"
#include "corps/volume.hh"

#include "wolika/echantillonnage.hh"
#include "wolika/filtre_3d.hh"
#include "wolika/grille_dense.hh"
#include "wolika/grille_eparse.hh"
#include "wolika/grille_temporelle.hh"
#include "wolika/iteration.hh"

#include "outils_visualisation.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

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

	auto desc = wlk::desc_grille_3d{};
	desc.etendue.min = min;
	desc.etendue.max = max;
	desc.fenetre_donnees = desc.etendue;
	desc.taille_voxel = static_cast<double>(op.evalue_decimal("taille_voxel"));

	auto graine = op.evalue_entier("graine");

	auto params_bruit = bruit::parametres();
	params_bruit.origine_bruit = op.evalue_vecteur("décalage_pos");
	params_bruit.taille_bruit = op.evalue_vecteur("échelle_pos");
	params_bruit.decalage_valeur = op.evalue_decimal("décalage_valeur");
	params_bruit.echelle_valeur = op.evalue_decimal("échelle_valeur");
	params_bruit.restreint = op.evalue_bool("restreint");
	params_bruit.restreint_neg = op.evalue_decimal("restreint_neg");
	params_bruit.restraint_pos = op.evalue_decimal("restreint_pos");
	params_bruit.temps_anim = op.evalue_decimal("temps_anim");

	bruit::ondelette::construit(params_bruit, graine);

	auto grille_scalaire = memoire::loge<wlk::grille_eparse<float>>("grille", desc);
	grille_scalaire->assure_tuiles(desc.etendue);

	wlk::pour_chaque_tuile_parallele(*grille_scalaire, [&](wlk::tuile_scalaire<float> *tuile)
	{
		auto index_tuile = 0;
		for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
			for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
				for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
					auto pos_tuile = tuile->min;
					pos_tuile.x += i;
					pos_tuile.y += j;
					pos_tuile.z += k;

					auto pos_monde = grille_scalaire->index_vers_monde(pos_tuile);

					tuile->donnees[index_tuile] = bruit::ondelette::evalue(params_bruit, pos_monde);
				}
			}
		}
	});

	auto volume = memoire::loge<Volume>("Volume", grille_scalaire);
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

	if (!valide_corps_entree(op, &corps_entree, true, true)) {
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

	auto desc_volume = wlk::desc_grille_3d{};
	desc_volume.etendue = limites_grille;
	desc_volume.fenetre_donnees = limites_grille;
	desc_volume.taille_voxel = static_cast<double>(taille_voxel);
	desc_volume.type_donnees = wlk::type_grille::R32;

	auto grille = memoire::loge<wlk::grille_eparse<float>>("wlk::grille_eparse", desc_volume);
	grille->assure_tuiles(limites);

	auto volume =  memoire::loge<Volume>("Volume", grille);

	auto delegue_prims = DeleguePrim(corps_entree);
	auto arbre_hbe = construit_arbre_hbe(delegue_prims, 24);

	wlk::pour_chaque_tuile_parallele(*grille, [&](wlk::tuile_scalaire<float> *tuile)
	{
		auto index_tuile = 0;

		auto rayon = dls::phys::rayond{};

		for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
			for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
				for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
					auto pos_tuile = tuile->min;
					pos_tuile.x += i;
					pos_tuile.y += j;
					pos_tuile.z += k;

					auto mnd = grille->index_vers_monde(pos_tuile);

					rayon.origine = dls::math::converti_type_point<double>(mnd);

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

					auto esect = traverse(arbre_hbe, delegue_prims, rayon);

					if (!esect.touche) {
						continue;
					}

					rayon.direction = -rayon.direction;
					calcul_direction_inverse(rayon);

					esect = traverse(arbre_hbe, delegue_prims, rayon);

					if (!esect.touche) {
						continue;
					}

					tuile->donnees[index_tuile] = densite;
#endif
				}
			}
		}
	});

	grille->elague();

#else
	auto desc = desc_grille_3d{};
	desc.etendue = limites;
	desc.fenetre_donnees = limites;
	desc.taille_voxel = taille_voxel;

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

	auto volume =  memoire::loge<Volume>("Volume", grille_scalaire);
#endif

	op.corps()->prims()->pousse(volume);

	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

static void rasterise_polygone(
		Corps const &corps,
		Polygone &poly,
		wlk::Rasteriseur<float> &rast,
		float rayon,
		float densite,
		GNA &gna,
		int nombre_echantillons,
		bruit::parametres *params_bruit,
		bruit::param_turbulence *params_turb)
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

		if (params_bruit != nullptr) {
			auto nsP = dls::math::vec3f(st[0], st[1], u);
			auto disp = rayon * bruit::evalue_turb(*params_bruit, *params_turb, nsP)/* * amplitude*/;
			wsP += N * disp;
		}

		// transforme la position en espace unitaire
		auto Eu = rast.grille().monde_vers_continu(wsP);
		rast.ecris_trilineaire(Eu, densite);
	}
}

static void rasterise_ligne(
		Corps const &corps,
		Polygone &poly,
		wlk::Rasteriseur<float> &rast,
		float rayon,
		float densite,
		GNA &gna,
		int nombre_echantillons,
		bruit::parametres *params_bruit,
		bruit::param_turbulence *params_turb)
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
			auto N = vec_ortho(T);

			// trouve le troisième axe
			auto NxT = produit_croix(N, T);

			auto st = echantillone_disque_uniforme(gna);

			// tire l'échantillon
			auto wsP = (1.0f - u) * p0 + u * p1;
			wsP += st.x * NxT * rayon;
			wsP += st.y * N * rayon;

			if (params_bruit != nullptr) {
				auto nsP = dls::math::vec3f(st[0], st[1], u);
				auto wsRadial = normalise(st.x * NxT + st.y * N);
				auto disp = rayon * bruit::evalue_turb(*params_bruit, *params_turb, nsP)/* * amplitude*/;
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
	auto const marge = op.evalue_vecteur("marge", contexte.temps_courant);

	/* calcul les limites des primitives d'entrées */
	auto limites = calcule_limites_mondiales_corps(corps_entree);

	limites.etends(marge * rayon);

	auto gna = GNA(graine);

	auto desc = wlk::desc_grille_3d{};
	desc.etendue = limites;
	desc.fenetre_donnees = limites;
	desc.taille_voxel = static_cast<double>(taille_voxel);

	auto grille_scalaire = memoire::loge<wlk::grille_dense_3d<float>>("grille", desc);

	auto rast = wlk::Rasteriseur(*grille_scalaire);

	for (auto i = 0; i < prims_entree->taille(); ++i) {
		auto prim = prims_entree->prim(i);

		if (prim->type_prim() == type_primitive::POLYGONE) {
			auto poly = dynamic_cast<Polygone *>(prim);

			if (poly->type == type_polygone::FERME) {
				rasterise_polygone(corps_entree, *poly, rast, rayon, densite, gna, nombre_echantillons, nullptr, nullptr);
			}
			else {
				rasterise_ligne(corps_entree, *poly, rast, rayon, densite, gna, nombre_echantillons, nullptr, nullptr);
			}
		}

		chef->indique_progression(static_cast<float>(i + 1) / static_cast<float>(prims_entree->taille()) * 100.0f);
	}

	/* À FAIRE : filtrage de l'échantillonage. */

	chef->indique_progression(100.0f);

	auto volume = memoire::loge<Volume>("Volume", grille_scalaire);
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

	if (!valide_corps_entree(op, &corps_entree, true, true)) {
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

	if (grille_entree->desc().type_donnees != wlk::type_grille::R32) {
		op.ajoute_avertissement("La grille n'est pas scalaire !");
		return EXECUTION_ECHOUEE;
	}

	auto grille_scalaire = dynamic_cast<wlk::grille_dense_3d<float> *>(grille_entree);

	auto resultat = wlk::reechantillonne(*grille_scalaire, grille_scalaire->desc().taille_voxel * 2.0);

	auto grille = memoire::loge<wlk::grille_dense_3d<float>>("grille");
	grille->echange(resultat);

	auto volume = memoire::loge<Volume>("Volume", grille);
	op.corps()->ajoute_primitive(volume);

	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

static void ajoute_volume_temps(
		wlk::grille_auxilliaire &grille_aux,
		wlk::grille_eparse<float> const &grille,
		float temps,
		float dt,
		bool debut)
{
	wlk::pour_chaque_tuile(grille, [&](wlk::tuile_scalaire<float> const *tuile)
	{
		auto res = grille.res_tuile();
		auto co_tuile = tuile->min / wlk::TAILLE_TUILE;
		auto idx_tuile = dls::math::calcul_index(co_tuile, res);

		auto tuile_aux = grille_aux.tuile_par_index(idx_tuile);

		/* Étape 1 : entresecte les topologies et alloue les tuiles non-déjà
		 * présentes. Pour s'assurer que les tuiles possèdent des valeurs
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
				for (auto i = 0; i < wlk::VOXELS_TUILE; ++i) {
					tuile_aux->donnees[i].pousse({ 0.0f, temps - dt });
				}
			}
		}

		/* Étape 2 : insère les valeurs pour ce temps.
		 */
		for (auto i = 0; i < wlk::VOXELS_TUILE; ++i) {
			tuile_aux->donnees[i].pousse({ tuile->donnees[i], temps });
		}

		tuile_aux->visite = true;
	});

	/* Étape 3 : il reste le cas où une tuile existait avant et n'existe plus
	 * maintenant -> ajoute valeur (0, temps).
	 */
	wlk::pour_chaque_tuile_parallele(grille_aux,
									 [&](wlk::grille_auxilliaire::type_tuile *tuile)
	{
		if (tuile->visite) {
			/* remet à zéro pour après */
			tuile->visite = false;
		}
		else {
			for (auto i = 0; i < wlk::VOXELS_TUILE; ++i) {
				tuile->donnees[i].pousse({ 0.0f, temps });
			}
		}
	});
}

#undef LOG_COMPRESSION

static void simplifie_courbes(
		wlk::grille_auxilliaire &grille,
		float seuil_saillance)
{
#ifdef LOG_COMPRESSION
	auto ancien_nombre_points = 0l;
	auto nouveau_nombre_points = 0l;
#endif

	wlk::pour_chaque_tuile_parallele(grille, [&](wlk::grille_auxilliaire::type_tuile *tuile)
	{
		for (auto i = 0; i < wlk::VOXELS_TUILE; ++i) {
			auto &donnees = tuile->donnees[i];
#ifdef LOG_COMPRESSION
			ancien_nombre_points += donnees.taille();
#endif

			if (donnees.taille() == 0 || donnees.taille() == 1) {
#ifdef LOG_COMPRESSION
				nouveau_nombre_points += donnees.taille();
#endif
				continue;
			}

			/* Étape 1 : enlève les valeurs répétées puisque nous présumons une
			 * entrepolation linéaire. */
			auto nv_courbe = wlk::type_courbe();
			nv_courbe.pousse(donnees[0]);

			for (auto j = 1; j < donnees.taille() - 1; ++j) {
				auto const &v0 = donnees[j - 1].valeur;
				auto const &v1 = donnees[j   ].valeur;
				auto const &v2 = donnees[j + 1].valeur;

				if (dls::math::sont_environ_egaux(v0, v1) && dls::math::sont_environ_egaux(v1, v2)) {
					continue;
				}

				nv_courbe.pousse(donnees[j]);
			}

			nv_courbe.pousse(donnees.back());

			donnees = nv_courbe;

			/* Étape 2 : enlève les points les moins saillants. */
			nv_courbe = wlk::type_courbe();
			nv_courbe.pousse(donnees[0]);

			auto const valeur_totale = (donnees.back().valeur - donnees.front().valeur);
			auto const temps_total = (donnees.back().temps - donnees.front().temps);
			auto const eps_u = seuil_saillance;
			auto const eps_r = std::abs(eps_u * valeur_totale);

			for (auto j = 1; j < donnees.taille() - 1; ++j) {
				auto const &p = donnees[j];

				/* les points avec une valeur à zéro sont préservés pour ne pas
				 * risquer d'introduire de matière quand il ne faut pas */
				if (p.valeur == 0.0f) {
					nv_courbe.pousse(donnees[j]);
					continue;
				}

				auto const fac = (p.temps - donnees.front().temps) / temps_total;

				/* trouve où la valeur serait si la courbe était une droite */
				auto const v = donnees.front().temps + fac * valeur_totale;
				auto const eps = std::abs(p.valeur - v);

				if (eps <= eps_r) {
					continue;
				}

				nv_courbe.pousse(donnees[j]);
			}

			nv_courbe.pousse(donnees.back());

			donnees = nv_courbe;
#ifdef LOG_COMPRESSION
			nouveau_nombre_points += donnees.taille();
#endif
		}
	});

#ifdef LOG_COMPRESSION
	std::cerr << "Ancien  nombre de points : " << ancien_nombre_points << '\n';
	std::cerr << "Nouveau nombre de points : " << nouveau_nombre_points << '\n';
	std::cerr << "Compression              : " << (1.0 - (static_cast<double>(nouveau_nombre_points) / static_cast<double>(ancien_nombre_points))) << '\n';
#endif
}

static auto compresse_grille_aux(
		wlk::grille_auxilliaire const &grille)
{
	auto grille_temp = memoire::loge<wlk::grille_temporelle>("grille", grille.desc());

	wlk::pour_chaque_tuile(grille,
						   [&](wlk::grille_auxilliaire::type_tuile const *tuile_aux)
	{
		auto tuile_temp = grille_temp->cree_tuile(tuile_aux->min);
		auto nombre_valeurs = 0l;

		for (auto i = 0; i < wlk::VOXELS_TUILE; ++i) {
			nombre_valeurs += tuile_aux->donnees[i].taille();
		}

		tuile_temp->valeurs = memoire::loge_tableau<float>("tuile_temp::valeurs", nombre_valeurs);
		tuile_temp->temps = memoire::loge_tableau<float>("tuile_temp::temps", nombre_valeurs);

		tuile_temp->decalage[0] = 0;
		nombre_valeurs = 0;

		for (auto i = 0; i < wlk::VOXELS_TUILE; ++i) {
			auto const &courbe = tuile_aux->donnees[i];

			for (auto j = 0; j < courbe.taille(); ++j) {
				tuile_temp->valeurs[nombre_valeurs + j] = courbe[j].valeur;
				tuile_temp->temps[nombre_valeurs + j] = courbe[j].temps;
			}

			nombre_valeurs += courbe.taille();
			tuile_temp->decalage[i + 1] = static_cast<int>(nombre_valeurs);
		}
	});

	return grille_temp;
}

static auto obtiens_grille(float temps)
{
	auto min = dls::math::vec3f(-1.0f);
	auto max = dls::math::vec3f(1.0f);

	auto desc = wlk::desc_grille_3d{};
	desc.etendue.min = min;
	desc.etendue.max = max;
	desc.fenetre_donnees = desc.etendue;
	desc.taille_voxel = 0.1;

	auto graine = 1;

	auto params_bruit = bruit::parametres();
	params_bruit.origine_bruit = dls::math::vec3f(45.0f);
	params_bruit.taille_bruit = dls::math::vec3f(1.0f);
	params_bruit.decalage_valeur = 0.0f;
	params_bruit.echelle_valeur = 1.0f;
	params_bruit.restreint = false;
	params_bruit.restreint_neg = 0.0f;
	params_bruit.restraint_pos = 1.0f;
	params_bruit.temps_anim = temps;

	bruit::ondelette::construit(params_bruit, graine);

	auto grille = memoire::loge<wlk::grille_eparse<float>>("grille", desc);
	grille->assure_tuiles(desc.etendue);

	wlk::pour_chaque_tuile_parallele(*grille, [&](wlk::tuile_scalaire<float> *tuile)
	{
		auto index_tuile = 0;
		for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
			for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
				for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
					auto pos_tuile = tuile->min;
					pos_tuile.x += i;
					pos_tuile.y += j;
					pos_tuile.z += k;

					auto mnd = grille->index_vers_monde(pos_tuile);

					tuile->donnees[index_tuile] = bruit::ondelette::evalue(params_bruit, mnd);
				}
			}
		}
	});

	return grille;
}

static auto cree_volume_temporelle(
		float temps_courant,
		float seuil_saillance)
{
	float const temps[3] = { -1.0f, 0.0f, 1.0f };

	auto grille_aux = static_cast<wlk::grille_auxilliaire *>(nullptr);

	for (auto i = 0; i < 3; ++i) {
		auto grille = obtiens_grille(temps_courant + temps[i]);

		if (i == 0) {
			grille_aux = memoire::loge<wlk::grille_auxilliaire>("wlk::grille_auxilliaire", grille->desc());
		}

		ajoute_volume_temps(*grille_aux, *grille, temps[i], 1.0f, i == 0);

		memoire::deloge("grille", grille);
	}

	simplifie_courbes(*grille_aux, seuil_saillance);

	auto grille_temp = compresse_grille_aux(*grille_aux);

	memoire::deloge("wlk::grille_auxilliaire", grille_aux);

	return grille_temp;
}

static auto echantillonne_grille_temp(
		wlk::grille_temporelle const &grille_temp,
		float temps)
{
	auto desc = grille_temp.desc();
	auto grille = memoire::loge<wlk::grille_eparse<float>>("grille", desc);
	grille->assure_tuiles(grille_temp.desc().etendue);

	wlk::pour_chaque_tuile_parallele(*grille, [&](wlk::tuile_scalaire<float> *tuile)
	{
		auto min_tuile = tuile->min / wlk::TAILLE_TUILE;
		auto idx_tuile = dls::math::calcul_index(min_tuile, grille->res_tuile());
		auto tuile_temp = grille_temp.tuile_par_index(idx_tuile);

		auto index_tuile = 0;
		for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
			for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
				for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
					tuile->donnees[index_tuile] = wlk::tuile_temporelle::echantillonne(tuile_temp, index_tuile, temps);
				}
			}
		}
	});

	return grille;
}

class OpCreationVolumeTemp final : public OperatriceCorps {
	wlk::grille_temporelle *m_grille_temps = nullptr;
	int m_dernier_temps = 0;
	float m_dernier_seuil = 1.0f;

public:
	static constexpr auto NOM = "Création Volume Temporel";
	static constexpr auto AIDE = "";

	OpCreationVolumeTemp(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
	{
		entrees(0);
		sorties(1);
	}

	OpCreationVolumeTemp(OpCreationVolumeTemp const &) = default;
	OpCreationVolumeTemp &operator=(OpCreationVolumeTemp const &) = default;

	~OpCreationVolumeTemp() override
	{
		memoire::deloge("grille", m_grille_temps);
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_volume_temporel.jo";
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		if (m_dernier_temps != contexte.temps_courant) {
			memoire::deloge("grille", m_grille_temps);
		}

		m_dernier_temps = contexte.temps_courant;

		auto const temps = evalue_decimal("temps");
		auto const seuil_saillance = evalue_decimal("seuil_saillance");

		/* reconstruit le volume si le seuil a changé */
		if (!dls::math::sont_environ_egaux(m_dernier_seuil, seuil_saillance)) {
			memoire::deloge("grille", m_grille_temps);
		}

		m_dernier_seuil = seuil_saillance;

		if (m_grille_temps == nullptr) {
			m_grille_temps = cree_volume_temporelle(static_cast<float>(contexte.temps_courant), seuil_saillance);
		}

		auto grille = echantillonne_grille_temp(*m_grille_temps, temps);

		auto volume = memoire::loge<Volume>("Volume", grille);
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

class OpFiltrageVolume final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Filtrage Volume";
	static constexpr auto AIDE = "";

	OpFiltrageVolume(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_volume_temporel.jo";
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_entree, false, true)) {
			return EXECUTION_ECHOUEE;
		}

		auto grille_entree = static_cast<wlk::grille_eparse<float> *>(nullptr);

		for (auto i = 0; i < corps_entree->prims()->taille(); ++i) {
			auto prim = corps_entree->prims()->prim(i);

			if (prim->type_prim() != type_primitive::VOLUME) {
				continue;
			}

			auto volume = dynamic_cast<Volume *>(prim);

			auto grille = volume->grille;

			if (grille->est_eparse() && grille->desc().type_donnees == wlk::type_grille::R32) {
				grille_entree = dynamic_cast<wlk::grille_eparse<float> *>(grille);
				break;
			}
		}

		if (grille_entree == nullptr) {
			this->ajoute_avertissement("Aucun volume (grille éparse R32) en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto taille_fenetre = 2;
		auto grille = wlk::floute_volume(*grille_entree, taille_fenetre);

		auto volume = memoire::loge<Volume>("Volume", grille);
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

class OpAffinageVolume final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Affinage Volume";
	static constexpr auto AIDE = "";

	OpAffinageVolume(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_volume_temporel.jo";
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_entree, false, true)) {
			return EXECUTION_ECHOUEE;
		}

		auto grille_entree = static_cast<wlk::grille_eparse<float> *>(nullptr);

		for (auto i = 0; i < corps_entree->prims()->taille(); ++i) {
			auto prim = corps_entree->prims()->prim(i);

			if (prim->type_prim() != type_primitive::VOLUME) {
				continue;
			}

			auto volume = dynamic_cast<Volume *>(prim);

			auto grille = volume->grille;

			if (grille->est_eparse() && grille->desc().type_donnees == wlk::type_grille::R32) {
				grille_entree = dynamic_cast<wlk::grille_eparse<float> *>(grille);
				break;
			}
		}

		if (grille_entree == nullptr) {
			this->ajoute_avertissement("Aucun volume (grille éparse R32) en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto taille_fenetre = 2;
		auto grille = wlk::affine_volume(*grille_entree, taille_fenetre, 0.5f);

		auto volume = memoire::loge<Volume>("Volume", grille);
		m_corps.prims()->pousse(volume);

		return EXECUTION_REUSSIE;
	}

	bool depend_sur_temps() const override
	{
		return true;
	}
};

/* ************************************************************************** */

//! An implementation of the Lighting Grid Hierarchy method.
//!
//! Can Yuksel and Cem Yuksel. 2017. Lighting Grid Hierarchy for
//! Self-illuminating Explosions. ACM Transactions on Graphics
//! (Proceedings of SIGGRAPH 2017) 36, 4, Article 110 (July 2017).
//! http://www.cemyuksel.com/research/lgh/
//!
//! This class builds the Lighting Grid Hierarchy and uses it for lighting.

class LightingGridHierarchy {
	struct Level {
		arbre_3df pc{};
		dls::tableau<dls::math::vec3f> colors{};
		 // position deviation for random shadow sampling
		dls::tableau<dls::math::vec3f> pDev{};

		Level() = default;
		~Level() = default;
	};

	dls::tableau<Level> m_niveaux{};
	int    m_nombre_niveaux = 0;
	float  m_taille_cellule = 0.0f;
	dls::math::vec3f m_dev_pos_n0 = dls::math::vec3f(0.0f);

public:
	LightingGridHierarchy() = default;

	~LightingGridHierarchy()
	{
		Clear();
	}

	int nombre_points(int niveau) const
	{
		if (m_niveaux.est_vide()) {
			return 0;
		}

		return m_niveaux[niveau].pc.compte_points();
	}

	int   GetNumLevels() const { return m_nombre_niveaux; }	//!< Returns the number of levels in the hierarchy.
	float GetCellSize () const { return m_taille_cellule; }		//!< Returns the size of a cell in the lowest (finest) level of the hierarchy.
	const dls::math::vec3f& GetLightPos   (int level, int i) const { return m_niveaux[level].pc.pos_point(i); }							//!< Returns the i^th light position at the given level. Note that this is not the position of the light with index i.
	int      GetLightIndex (int level, int i) const { return m_niveaux[level].pc.index_point(i); }						//!< Returns the i^th light index at the given level.
	const dls::math::vec3f&   GetLightIntens(int level, int ix) const { return m_niveaux[level].colors[ix]; }								//!< Returns the intensity of the light with index ix at the given level.
	const dls::math::vec3f& GetLightPosDev(int level, int ix) const { return level > 0 ? m_niveaux[level].pDev[ix] : m_dev_pos_n0; }	//!< Returns the position variation of the light with index ix at the given level.

	void Clear() { m_niveaux.efface(); m_nombre_niveaux = 0; }	//!< Deletes all data.

	//! Builds the Lighting Grid Hierarchy for the given point light positions and intensities using the given parameters.
	//! This method builds the hierarchy using the given cellSize as the size of the lowest (finest) level grid cells.
	bool Build(const dls::math::vec3f *lightPos,			//!< Light positions.
				const dls::math::vec3f   *lightIntensities, 	//!< Light intensities.
				int            numLights, 			//!< Number of lights.
				int            minLevelLights,		//!< The minimum number of lights permitted for the highest (coarsest) level of the hierarchy. The build stops if a higher (coarser) level would have fewer lights.
				float          cellSize,			//!< The size of a grid cell in the lowest (finest) level of the hierarchy.
				int            highestLevel			//!< The highest level permitted, where level 0 contains the original lights.
			)
	{
		return DoBuild(lightPos, lightIntensities, numLights, 0, minLevelLights, cellSize, highestLevel);
	}

	//! Builds the Lighting Grid Hierarchy for the given point light positions and intensities using the given parameters.
	//! This method automatically determines the grid cell size based on the bounding box of the light positions.
	bool Build(const dls::math::vec3f *lightPos,			//!< Light positions.
				const dls::math::vec3f   *lightIntensities, 	//!< Light intensities.
				int            numLights, 			//!< Number of lights.
				int            minLevelLights, 		//!< The minimum number of lights permitted for the highest (coarsest) level of the hierarchy. The build stops if a higher (coarser) level would have fewer lights.
				float          autoFitScale = 1.01f	//!< Extends the bounding box of the light positions using the given scale. This value must be 1 or greater. A value slightly greater than 1 often provides a good fit for the grid.
			)
	{
		return DoBuild(lightPos, lightIntensities, numLights, autoFitScale, minLevelLights);
	}

	//! Computes the illumination at the given position using the given accuracy parameter alpha.
	template <typename LightingFunction>
	void Light(const dls::math::vec3f    &pos,						//!< The position where the lighting will be evaluated.
				float            alpha,						//!< The accuracy parameter. It should be 1 or greater. Larger values produce more accurate results with substantially more computation.
				LightingFunction lightingFunction			//!< This function is called for each light used for lighting computation. It should be in the form void LightingFunction(int level, int light_id, const dls::math::vec3f &light_position, const dls::math::vec3f &light_intensity).
			)
	{
		Light(pos, alpha, 0, lightingFunction);
	}

	//! Computes the illumination at the given position using the given accuracy parameter alpha.
	//! This method provides stochastic sampling by randomly changing the given light position when calling the lighting function.
	template <typename LightingFunction>
	void Light(const dls::math::vec3f    &pos,						//!< The position where the lighting will be evaluated.
				float            alpha,						//!< The accuracy parameter. It should be 1 or greater. Larger values produce more accurate results with substantially more computation.
				int              stochasticShadowSamples,   //!< When this parameter is zero, the given lightingFunction is called once per light, using the position of the light. Otherwise, it is called as many times as this parameter specifies, using random positions around each light position.
				LightingFunction lightingFunction			//!< This function is called for each light used for lighting computation. It should be in the form void LightingFunction(int level, int light_id, const dls::math::vec3f &light_position, const dls::math::vec3f &light_intensity).
			)
	{
		if (m_nombre_niveaux > 1) {

			// First level
			auto r = alpha * m_taille_cellule;
			auto rr = r * r;

			m_niveaux[0].pc.cherche_points(pos, r*2, [&](int i, const dls::math::vec3f &p, float dist2, float &radius2)
			{
				INUTILISE(radius2);
				auto c = m_niveaux[0].colors[i];

				if (dist2 > rr) {
					c *= 1.0f - (std::sqrt(dist2) - r) / r;
				}

				lightingFunction(0, i, p, c);
			});

			auto callLightingFunc = [&](int level, int i, const dls::math::vec3f &p, const dls::math::vec3f &c)
			{
				if (stochasticShadowSamples > 0) {
					auto cc = c / static_cast<float>(stochasticShadowSamples);

					for (auto j = 0; j<stochasticShadowSamples; j++) {
						auto pj = p + RandomPos() * m_niveaux[level].pDev[i];
						lightingFunction(level, i, pj, cc);
					}
				}
				else {
					lightingFunction(level, i, p, c);
				}
			};

			// Middle levels
			for (int level=1; level<m_nombre_niveaux-1; level++) {
				float r_min = r;
				float rr_min = r * r;
				r *= 2;

				m_niveaux[level].pc.cherche_points(pos, r*2, [&](int i, const dls::math::vec3f &p, float dist2, float &radius2)
				{
					INUTILISE(radius2);

					if (dist2 <= rr_min) {
						return;
					}

					auto c = m_niveaux[level].colors[i];
					auto d = std::sqrt(dist2);

					if (d > r) {
						c *= 1.0f - (d - r) / r;
					}
					else {
						c *= (d - r_min) / r_min;
					}

					callLightingFunc(level, i, p, c);
				});
			}

			// Last level
			auto r_min = r;
			auto rr_min = r * r;
			r *= 2;
			rr = r * r;
			auto n = m_niveaux[m_nombre_niveaux - 1].pc.compte_points();

			for (auto i = 0; i < n; i++) {
				auto const &p = m_niveaux[m_nombre_niveaux-1].pc.pos_point(i);
				auto dist2 = longueur_carree(pos - p);

				if (dist2 <= rr_min) {
					continue;
				}

				auto id = m_niveaux[m_nombre_niveaux-1].pc.index_point(i);
				auto c = m_niveaux[m_nombre_niveaux-1].colors[id];

				if (dist2 < rr) {
					c *= (std::sqrt(dist2) - r_min) / r_min;
				}

				callLightingFunc(m_nombre_niveaux-1, i, p, c);
			}
		}
		else {
			// Single-level (a.k.a. brute-force)
			auto n = m_niveaux[0].pc.compte_points();
			for (auto i=0; i<n; i++) {
				auto const &p = m_niveaux[0].pc.pos_point(i);
				auto id = m_niveaux[0].pc.index_point(i);
				auto const &c = m_niveaux[0].colors[id];
				lightingFunction(0, i, p, c);
			}
		}
	}

private:
	float RandomX()
	{
		static thread_local std::mt19937 generator;
		std::uniform_real_distribution<float> distribution;
		float x = distribution(generator);
		float y = distribution(generator);
		if (y > (cosf(x*constantes<float>::PI)+1)*0.5f) x -= 1;
		return x;
	}

	dls::math::vec3f RandomPos()
	{
		dls::math::vec3f p;
		p.x = RandomX();
		p.y = RandomX();
		p.z = RandomX();
		return p;
	}

	bool DoBuild(const dls::math::vec3f *lightPos, const dls::math::vec3f *lightColor, int numLights, float autoFitScale, int minLevelLights, float cellSize=0, int highestLevel=10)
	{
		Clear();

		if (numLights <= 0 || highestLevel <= 0) {
			return false;
		}

		// Compute the bounding box for the lighPoss
		auto boundMin = dls::math::vec3f(lightPos[0]);
		auto boundMax = dls::math::vec3f(lightPos[0]);

		for (auto i = 1; i < numLights; i++) {
			dls::math::extrait_min_max(lightPos[i], boundMin, boundMax);
		}

		auto boundDif = boundMax - boundMin;
		auto boundDifMin = min(boundDif);

		// Determine the actual highest level
		float highestCellSize;
		dls::math::vec3i highestGridRes;
		if (autoFitScale > 0) {
			highestCellSize = max(boundDif) * autoFitScale;
			int s = int(1.0f/autoFitScale) + 2;
			if (s < 2) s = 2;
			highestGridRes = dls::math::vec3i(s);
		}
		else {
			int highestLevelMult = 1 << (highestLevel-1);
			highestCellSize = cellSize * static_cast<float>(highestLevelMult);
			while (highestLevel>1 && highestCellSize > boundDifMin*2) {
				highestLevel--;
				highestLevelMult = 1 << (highestLevel-1);
				highestCellSize = cellSize * static_cast<float>(highestLevelMult);
			}
			highestGridRes = dls::math::converti_type<int>(boundDif / highestCellSize) + dls::math::vec3i(2);
		}

		struct Node {
			Node() = default;

			dls::math::vec3f position = dls::math::vec3f(0.0f);
			dls::math::vec3f color = dls::math::vec3f(0.0f);
#ifdef CY_LIGHTING_GRID_ORIG_POS
			dls::math::vec3f origPos = dls::math::vec3f(0.0f);
#endif // CY_LIGHTING_GRID_ORIG_POS
			dls::math::vec3f stdev = dls::math::vec3f(0.0f);
			float weight = 0.0f;
			int firstChild = -1;

			void AddLight(float w, const dls::math::vec3f &p, const dls::math::vec3f &c)
			{
				weight   += w;
				position += w * p;
				color    += w * c;
				stdev    += w * (p*p);
			}
			void Normalize()
			{
				if (weight > 0) {
					position /= weight;
					stdev = stdev/weight - position*position;
				}
			}
		};

		// Allocate the temporary nodes
		m_nombre_niveaux = highestLevel+1;
		auto nodes = dls::tableau< dls::tableau<Node> >(m_nombre_niveaux);

		auto gridIndex = [](dls::math::vec3i &index, const dls::math::vec3f &pos, float dx_loc)
		{
			auto normP = pos / dx_loc;
			index = dls::math::converti_type<int>(normP);
			return normP - dls::math::converti_type<float>(index);
		};

		auto addLightToNodes = [](dls::tableau<Node> &nds, const int nodeIDs[8], const dls::math::vec3f &interp, const dls::math::vec3f &light_pos, const dls::math::vec3f &light_color)
		{
			for (int j=0; j<8; j++) {
				float w = ((j&1) ? interp.x : (1-interp.x)) * ((j&2) ? interp.y : (1-interp.y)) * ((j&4) ? interp.z : (1-interp.z));
				nds[nodeIDs[j]].AddLight(w, light_pos, light_color);
			}
		};

		// Generate the grid for the highest level
		auto highestGridSize = dls::math::converti_type<float>(highestGridRes - dls::math::vec3i(1)) * highestCellSize;
		auto center = (boundMax + boundMin) / dls::math::vec3f(2.0f);
		auto corner = center - highestGridSize * 0.5f;
		nodes[highestLevel].redimensionne(highestGridRes.x * highestGridRes.y * highestGridRes.z);
#ifdef CY_LIGHTING_GRID_ORIG_POS
		for (int z=0, j=0; z<highestGridRes.z; z++) {
			for (int y=0; y<highestGridRes.y; y++) {
				for (int x=0; x<highestGridRes.x; x++, j++) {
					nodes[highestLevel][j].origPos = corner + dls::math::vec3f(x,y,z)*highestCellSize;
				}
			}
		}
#endif // CY_LIGHTING_GRID_ORIG_POS
		for (int i=0; i<numLights; i++) {
			dls::math::vec3i index;
			auto interp = gridIndex(index, dls::math::vec3f(lightPos[i])-corner, highestCellSize);
			int is = index.z*highestGridRes.y*highestGridRes.x + index.y*highestGridRes.x + index.x;

			int nodeIDs[8] = {
				is,
				is + 1,
				is + highestGridRes.x,
				is + highestGridRes.x + 1,
				is + highestGridRes.x*highestGridRes.y,
				is + highestGridRes.x*highestGridRes.y + 1,
				is + highestGridRes.x*highestGridRes.y + highestGridRes.x,
				is + highestGridRes.x*highestGridRes.y + highestGridRes.x + 1,
			};

			for (int j=0; j<8; j++) {
				assert(nodeIDs[j] >= 0 && nodeIDs[j] < nodes[highestLevel].taille());
			}

			addLightToNodes(nodes[highestLevel], nodeIDs, interp, lightPos[i], lightColor[i]);
		}

		for (auto i = 0; i < nodes[highestLevel].taille(); i++) {
			nodes[highestLevel][i].Normalize();
		}

		// Generate the lower levels
		auto nodeCellSize = highestCellSize;
		auto gridRes = highestGridRes;
		auto levelSkip = 0;

		for (auto level = highestLevel - 1; level > 0; level--) {
			// Find the number of nodes for this level
			auto nodeCount = 0;

			for (auto i = 0; i < nodes[level + 1].taille(); i++) {
				if (nodes[level+1][i].weight > 0) {
					nodes[level+1][i].firstChild = nodeCount;
					nodeCount += 8;
				}
			}

			if (nodeCount > numLights/4) {
				levelSkip = level;
				break;
			}

			nodes[level].redimensionne(nodeCount);
			// Add the lights to the nodes
			nodeCellSize /= 2;
			gridRes *= 2;
#ifdef CY_LIGHTING_GRID_ORIG_POS
			for (int i=0; i<(int)nodes[level+1].size(); i++) {
				int fc = nodes[level+1][i].firstChild;
				if (fc < 0) continue;
				for (int z=0, j=0; z<2; z++) {
					for (int y=0; y<2; y++) {
						for (int x=0; x<2; x++, j++) {
							nodes[level][fc+j].origPos = nodes[level+1][i].origPos + dls::math::vec3f(x,y,z)*nodeCellSize;
						}
					}
				}
			}
#endif // CY_LIGHTING_GRID_ORIG_POS

			for (int i=0; i<numLights; i++) {
				dls::math::vec3i index;
				auto interp = gridIndex(index, dls::math::vec3f(lightPos[i])-corner, nodeCellSize);
				// find the node IDs
				int nodeIDs[8];
				index <<= level+2;

				for (auto z=0, j=0; z<2; z++) {
					auto iz = index.z + z;

					for (auto y=0; y<2; y++) {
						auto iy = index.y + y;

						for (auto x=0; x<2; x++, j++) {
							auto ix = index.x + x;
							auto hix = ix >> (highestLevel+2);
							auto hiy = iy >> (highestLevel+2);
							auto hiz = iz >> (highestLevel+2);
							auto nid = hiz*highestGridRes.y*highestGridRes.x + hiy*highestGridRes.x + hix;

							for (auto l=highestLevel-1; l>=level; l--) {
								auto ii = ((index.z >> l)&4) | ((index.y >> (l+1))&2) |  ((index.x >> (l+2))&1);
								assert(nodes[l+1][nid].firstChild >= 0);
								nid = nodes[l+1][nid].firstChild + ii;
								assert(nid >= 0 && nid < nodes[l].taille());
							}

							nodeIDs[j] = nid;
						}
					}
				}

				addLightToNodes(nodes[level], nodeIDs, interp, lightPos[i], lightColor[i]);
			}

			for (auto i = 0; i < nodes[level].taille(); i++) {
				nodes[level][i].Normalize();
			}
		}

		// Copy light data
		m_nombre_niveaux = highestLevel + 1 - levelSkip;
		//auto levelBaseSkip = 0;
		// Skip levels that have two few lights (based on minLevelLights).
		for (int level=1; level<m_nombre_niveaux; level++) {
			auto &levelNodes = nodes[level+levelSkip];
			int count = 0;

			for (auto i = 0; i < levelNodes.taille(); i++) {
				if (levelNodes[i].weight > 0) {
					count++;
				}
			}

			if (count < minLevelLights) {
				m_nombre_niveaux = level;
				break;
			}
		}

		m_niveaux.redimensionne(m_nombre_niveaux);

		for (auto level = 1; level < m_nombre_niveaux; level++) {
			auto &levelNodes = nodes[level+levelSkip];
			auto &thisLevel = m_niveaux[level];
			auto pos = dls::tableau<dls::math::vec3f>(levelNodes.taille());
			auto lightCount = 0;

			for (auto i = 0; i < levelNodes.taille(); i++) {
				if (levelNodes[i].weight > 0) {
					pos[lightCount++] = levelNodes[i].position;
				}
			}

			thisLevel.pc.construit(lightCount, pos.donnees());
			thisLevel.colors = dls::tableau<dls::math::vec3f>(lightCount);
			thisLevel.pDev = dls::tableau<dls::math::vec3f>(lightCount);

			for (auto i = 0, j = 0; i < levelNodes.taille(); i++) {
				if (levelNodes[i].weight > 0) {
					assert(j < lightCount);
					thisLevel.colors[j] = levelNodes[i].color;
					thisLevel.pDev[j].x = sqrtf(levelNodes[i].stdev.x) * constantes<float>::PI;
					thisLevel.pDev[j].y = sqrtf(levelNodes[i].stdev.y) * constantes<float>::PI;
					thisLevel.pDev[j].z = sqrtf(levelNodes[i].stdev.z) * constantes<float>::PI;
					j++;
				}
			}

			levelNodes.redimensionne(0);
			levelNodes.adapte_taille();
		}

		auto pos = dls::tableau<dls::math::vec3f>(numLights);
		m_niveaux[0].colors = dls::tableau<dls::math::vec3f>(numLights);

		for (auto i=0; i<numLights; i++) {
			pos[i] = lightPos[i];
			m_niveaux[0].colors[i] = lightColor[i];
		}

		m_niveaux[0].pc.construit(numLights, pos.donnees());
		this->m_taille_cellule = nodeCellSize;

		return true;
	}
};

class OpGrilleEclairage final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Grille Éclairage";
	static constexpr auto AIDE = "";

	OpGrilleEclairage(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_creation_grille_eclairage.jo";
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		auto corps_entree = entree(0)->requiers_corps(contexte, donnees_aval);

		if (!valide_corps_entree(*this, corps_entree, false, true)) {
			return EXECUTION_ECHOUEE;
		}

		auto grille_entree = static_cast<wlk::grille_eparse<float> *>(nullptr);

		for (auto i = 0; i < corps_entree->prims()->taille(); ++i) {
			auto prim = corps_entree->prims()->prim(i);

			if (prim->type_prim() != type_primitive::VOLUME) {
				continue;
			}

			auto volume = dynamic_cast<Volume *>(prim);

			auto grille = volume->grille;

			if (grille->est_eparse() && grille->desc().type_donnees == wlk::type_grille::R32) {
				grille_entree = dynamic_cast<wlk::grille_eparse<float> *>(grille);
				break;
			}
		}

		if (grille_entree == nullptr) {
			this->ajoute_avertissement("Aucun volume (grille éparse R32) en entrée !");
			return EXECUTION_ECHOUEE;
		}

		auto intensite_min = evalue_decimal("intensité_min");

		auto positions = dls::tableau<dls::math::vec3f>{};
		auto intensites = dls::tableau<dls::math::vec3f>{};

		wlk::pour_chaque_tuile(*grille_entree, [&](wlk::tuile_scalaire<float> const *tuile)
		{
			auto index_tuile = 0;
			for (auto k = 0; k < wlk::TAILLE_TUILE; ++k) {
				for (auto j = 0; j < wlk::TAILLE_TUILE; ++j) {
					for (auto i = 0; i < wlk::TAILLE_TUILE; ++i, ++index_tuile) {
						auto pos_tuile = tuile->min;
						pos_tuile.x += i;
						pos_tuile.y += j;
						pos_tuile.z += k;

						auto intensite = tuile->donnees[index_tuile];

						if (intensite < intensite_min) {
							continue;
						}

						auto pos_monde = grille_entree->index_vers_monde(pos_tuile);
						positions.pousse(pos_monde);
						intensites.pousse(dls::math::vec3f(intensite));
					}
				}
			}
		});

		auto lumieres_min = evalue_entier("lumières_min");
		auto niveaux_max = evalue_entier("niveaux_max");

		auto hierarchie = LightingGridHierarchy();
		hierarchie.Build(
					positions.donnees(),
					intensites.donnees(),
					static_cast<int>(positions.taille()),
					lumieres_min,
					static_cast<float>(grille_entree->desc().taille_voxel),
					niveaux_max);

		auto dernier_niveau = std::max(0, hierarchie.GetNumLevels() - 1);

		auto vis_niveau = evalue_entier("vis_niveau");
		vis_niveau = dls::math::restreint(vis_niveau, 0, dernier_niveau);

		auto nombre_points = hierarchie.nombre_points(vis_niveau);

		for (auto i = 0; i < nombre_points; ++i) {
			auto pos = hierarchie.GetLightPos(vis_niveau, i);
			m_corps.ajoute_point(pos);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

/**
 * Implémentation des descripteurs de
 * « Deep Scattering: Rendering Atmospheric Clouds with Radiance-Predicting
 * Neural Networks »
 *
 * http://simon-kallweit.me/deepscattering/
 * http://simon-kallweit.me/deepscattering/deepscattering.pdf
 * https://tom94.net/data/publications/kallweit17deep/interactive-viewer/
 */
class OpDeepScattering final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Deep Scattering";
	static constexpr auto AIDE = "";

	OpDeepScattering(Graphe &graphe_parent, Noeud &noeud_)
		: OperatriceCorps(graphe_parent, noeud_)
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

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_deep_scattering.jo";
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(contexte);
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		/* il y a 10 descripteurs de 5x5x9 */
		auto desc = wlk::desc_grille_3d();
		desc.resolution.x = 5;
		desc.resolution.y = 5;
		desc.resolution.z = 9;
		desc.taille_voxel = 1.0; // par grille
		/* les descripteurs vont de (-1, -1, -1) à (1, 1, 3) */
		desc.etendue.min = dls::math::vec3f(-1.0f);
		desc.etendue.max = dls::math::vec3f(1.0f, 1.0f, 3.0f);
		desc.fenetre_donnees = desc.etendue;

		wlk::desc_grille_3d descripteurs[10];

		/* les supports des descripteurs sont agrandis avec un facteur de 2^(k-1) */
		for (auto k = 1; k <= 10; ++k) {
			descripteurs[k - 1] = desc;
			descripteurs[k - 1].taille_voxel = desc.taille_voxel * std::pow(2.0, static_cast<double>(k) - 1.0);
		}

		auto omega = dls::math::vec3f(0.0f, 0.0f, -1.0f); // la direction du rayon entrant

		/* l'axe Z des descripteurs pointent vers la lumière */
		auto position = dls::math::vec3f(0.0f, 0.0f, 0.0f);
		auto position_lumiere = evalue_vecteur("position_lumière");
		auto axe_z = normalise(position_lumiere - position);

		/* l'axe X doit être perpendiculaire au plan défini par l'axe Z et omega */
		auto axe_x = normalise(produit_croix(axe_z, omega));

		/* l'axe Y est orthogonal aux deux autres */
		auto axe_y = normalise(produit_croix(axe_x, axe_z));

		auto mat = dls::math::mat3x3f(
					axe_x.x, axe_x.y, axe_x.z,
					axe_y.x, axe_y.y, axe_y.z,
					axe_z.x, axe_z.y, axe_z.z);

		/* angle ajouté aux couches du réseau */
		//auto gamma = std::acos(produit_scalaire(omega, direction));

		/* dessine les descripteurs */
		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::R32, 3);
		auto couleur = dls::math::vec3f(0.2f, 0.1f, 0.8f);
		for (auto k = 0; k < 10; ++k) {
			auto min = descripteurs[k].etendue.min * static_cast<float>(descripteurs[k].taille_voxel);
			auto max = descripteurs[k].etendue.max * static_cast<float>(descripteurs[k].taille_voxel);

			/* nous devons appliquer la matrice à tous les points et non
			 * seulement aux points min et max -> il faudra sans doute garder
			 * trace de chaque matrice pour échantillonner proprement le volume
			 * afin de remplir les descripteurs */
			dls::math::vec3f sommets[8] = {
				dls::math::vec3f(min.x, min.y, min.z),
				dls::math::vec3f(min.x, min.y, max.z),
				dls::math::vec3f(max.x, min.y, max.z),
				dls::math::vec3f(max.x, min.y, min.z),
				dls::math::vec3f(min.x, max.y, min.z),
				dls::math::vec3f(min.x, max.y, max.z),
				dls::math::vec3f(max.x, max.y, max.z),
				dls::math::vec3f(max.x, max.y, min.z),
			};

			for (auto i = 0; i < 8; ++i) {
				sommets[i] = mat * sommets[i];
			}

			dessine_boite(m_corps, attr_C, sommets, couleur);
		}

		dessine_boite(m_corps,
					  attr_C,
					  position_lumiere - dls::math::vec3f(0.1f),
					  position_lumiere + dls::math::vec3f(0.1f),
					  dls::math::vec3f(0.2f, 0.9f, 0.3f));

		/* dessine les axes */
		auto axe_x_ = dls::math::vec3f(1.0f, 0.0f, 0.0f);
		auto axe_y_ = dls::math::vec3f(0.0f, 1.0f, 0.0f);
		auto axe_z_ = dls::math::vec3f(0.0f, 0.0f, 1.0f);

		auto i0 = m_corps.ajoute_point(0.0f, 0.0f, 0.0f);
		auto i1 = m_corps.ajoute_point(mat * axe_x_);

		assigne(attr_C->r32(i1), axe_x_);

		auto poly = m_corps.ajoute_polygone(type_polygone::OUVERT, 2);
		m_corps.ajoute_sommet(poly, i0);
		m_corps.ajoute_sommet(poly, i1);

		i1 = m_corps.ajoute_point(mat * axe_y_);

		assigne(attr_C->r32(i1), axe_y_);

		poly = m_corps.ajoute_polygone(type_polygone::OUVERT, 2);
		m_corps.ajoute_sommet(poly, i0);
		m_corps.ajoute_sommet(poly, i1);

		i1 = m_corps.ajoute_point(mat * axe_z_);

		assigne(attr_C->r32(i1), axe_z_);

		poly = m_corps.ajoute_polygone(type_polygone::OUVERT, 2);
		m_corps.ajoute_sommet(poly, i0);
		m_corps.ajoute_sommet(poly, i1);

		return EXECUTION_REUSSIE;
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
	usine.enregistre_type(cree_desc<OpFiltrageVolume>());
	usine.enregistre_type(cree_desc<OpAffinageVolume>());
	usine.enregistre_type(cree_desc<OpGrilleEclairage>());
	usine.enregistre_type(cree_desc<OpDeepScattering>());
}

#pragma clang diagnostic pop
