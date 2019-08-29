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
	params_bruit.decalage_pos = op.evalue_vecteur("décalage_pos");
	params_bruit.echelle_pos = op.evalue_vecteur("échelle_pos");
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
			auto N = dls::math::vec3f(-T.y, T.x, 0.0f);

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
	params_bruit.decalage_pos = dls::math::vec3f(45.0f);
	params_bruit.echelle_pos = dls::math::vec3f(1.0f);
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

	OpCreationVolumeTemp(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
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

	OpFiltrageVolume(Graphe &graphe_parent, Noeud *noeud)
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

	OpAffinageVolume(Graphe &graphe_parent, Noeud *noeud)
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

//! A point cloud class that uses a k-d tree for storing points.
//!
//! The GetPoints and GetClosest methods return the neighboring points to a given location.

template <typename PointType, typename FType, uint32_t DIMENSIONS, typename SIZE_TYPE=uint32_t>
class PointCloud {
public:
	/////////////////////////////////////////////////////////////////////////////////
	//!@name Constructors and Destructor

	PointCloud() = default;
	PointCloud(SIZE_TYPE numPts, const PointType *pts, const SIZE_TYPE *customIndices=nullptr) : points(nullptr), pointCount(0) { Build(numPts,pts,customIndices); }
	~PointCloud() { delete [] points; }

	PointCloud(PointCloud const &) = default;
	PointCloud &operator=(PointCloud const &) = default;

	/////////////////////////////////////////////////////////////////////////////////
	//!@ Access to internal data

	SIZE_TYPE GetPointCount() const { return pointCount-1; }					//!< Returns the point count
	const PointType& GetPoint(SIZE_TYPE i) const { return points[i+1].Pos(); }	//!< Returns the point at position i
	SIZE_TYPE GetPointIndex(SIZE_TYPE i) const { return points[i+1].Index(); }	//!< Returns the index of the point at position i

	/////////////////////////////////////////////////////////////////////////////////
	//!@ Initialization

	//! Builds a k-d tree for the given points.
	//! The positions are stored internally.
	//! The build is parallelized using Intel's Thread Building Library (TBB) or Microsoft's Parallel Patterns Library (PPL),
	//! if ttb.h or ppl.h is included prior to including cyPointCloud.h.
	void Build(SIZE_TYPE numPts, const PointType *pts) { BuildWithFunc(numPts, [&pts](SIZE_TYPE i){ return pts[i]; }); }

	//! Builds a k-d tree for the given points.
	//! The positions are stored internally, along with the indices to the given array.
	//! The build is parallelized using Intel's Thread Building Library (TBB) or Microsoft's Parallel Patterns Library (PPL),
	//! if ttb.h or ppl.h is included prior to including cyPointCloud.h.
	void Build(SIZE_TYPE numPts, const PointType *pts, const SIZE_TYPE *customIndices) { BuildWithFunc(numPts, [&pts](SIZE_TYPE i){ return pts[i]; }, [&customIndices](SIZE_TYPE i){ return customIndices[i]; }); }

	//! Builds a k-d tree for the given points.
	//! The positions are stored internally, retrieved from the given function.
	//! The build is parallelized using Intel's Thread Building Library (TBB) or Microsoft's Parallel Patterns Library (PPL),
	//! if ttb.h or ppl.h is included prior to including cyPointCloud.h.
	template <typename PointPosFunc>
	void BuildWithFunc(SIZE_TYPE numPts, PointPosFunc ptPosFunc) { BuildWithFunc(numPts, ptPosFunc, [](SIZE_TYPE i){ return i; }); }

	//! Builds a k-d tree for the given points.
	//! The positions are stored internally, along with the indices to the given array.
	//! The positions and custom indices are retrieved from the given functions.
	//! The build is parallelized using Intel's Thread Building Library (TBB) or Microsoft's Parallel Patterns Library (PPL),
	//! if ttb.h or ppl.h is included prior to including cyPointCloud.h.
	template <typename PointPosFunc, typename CustomIndexFunc>
	void BuildWithFunc(SIZE_TYPE numPts, PointPosFunc ptPosFunc, CustomIndexFunc custIndexFunc)
	{
		if (points) delete [] points;
		pointCount = numPts;
		if (pointCount == 0) { points = nullptr; return; }
		points = new PointData[static_cast<size_t>((pointCount|1)+1)];
		PointData *orig = new PointData[static_cast<size_t>(pointCount)];
		PointType boundMin((std::numeric_limits<FType>::max)()), boundMax((std::numeric_limits<FType>::min)());
		for (SIZE_TYPE i=0; i<pointCount; i++) {
			PointType p = ptPosFunc(i);
			orig[i].Set(p, custIndexFunc(i));
			for (auto j=0ul; j<DIMENSIONS; j++) {
				if (boundMin[j] > p[j]) boundMin[j] = p[j];
				if (boundMax[j] < p[j]) boundMax[j] = p[j];
			}
		}
		BuildKDTree(orig, boundMin, boundMax, 1, 0, pointCount);
		delete [] orig;
		if ((pointCount & 1) == 0) {
			// if the point count is even, we should add a bogus point
			points[pointCount+1].Set(PointType(std::numeric_limits<FType>::infinity()), 0, 0);
		}
		numInternal = pointCount / 2;
	}

	//! Returns true if the Build or BuildWithFunc methods would perform the build in parallel using multi-threading.
	//! The build is parallelized using Intel's Thread Building Library (TBB) or Microsoft's Parallel Patterns Library (PPL),
	//! if ttb.h or ppl.h are included prior to including cyPointCloud.h.
	static bool IsBuildParallel()
	{
#ifdef _CY_PARALLEL_LIB
		return true;
#else
		return false;
#endif
	}

	/////////////////////////////////////////////////////////////////////////////////
	//!@ General search methods

	//! Returns all points to the given position within the given radius.
	//! Calls the given pointFound function for each point found.
	//!
	//! The given pointFound function can reduce the radiusSquared value.
	//! However, increasing the radiusSquared value can have unpredictable results.
	//! The callback function must be in the following form:
	//!
	//! void _CALLBACK(SIZE_TYPE index, const PointType &p, FType distanceSquared, FType &radiusSquared)
	template <typename _CALLBACK>
	void GetPoints(const PointType &position, FType radius, _CALLBACK pointFound) const
	{
		FType r2 = radius*radius;
		GetPoints(position, r2, pointFound, 1);
	}

	//! Used by one of the PointCloud::GetPoints() methods.
	//!
	//! Keeps the point index, position, and distance squared to a given search position.
	//! Used by one of the GetPoints methods.
	struct PointInfo {
		SIZE_TYPE index;			//!< The index of the point
		PointType pos;				//!< The position of the point
		FType     distanceSquared;	//!< Squared distance from the search position
		bool operator < (const PointInfo &b) const { return distanceSquared < b.distanceSquared; }	//!< Comparison operator
	};

	//! Returns the closest points to the given position within the given radius.
	//! It returns the number of points found.
	int GetPoints(const PointType &position, FType radius, SIZE_TYPE maxCount, PointInfo *closestPoints) const
	{
		int pointsFound = 0;
		GetPoints(position, radius, [&](SIZE_TYPE i, const PointType &p, FType d2, FType &r2) {
			if (pointsFound == maxCount) {
				std::pop_heap(closestPoints, closestPoints+maxCount);
				closestPoints[maxCount-1].index = i;
				closestPoints[maxCount-1].pos = p;
				closestPoints[maxCount-1].distanceSquared = d2;
				std::push_heap(closestPoints, closestPoints+maxCount);
				r2 = closestPoints[0].distanceSquared;
			} else {
				closestPoints[pointsFound].index = i;
				closestPoints[pointsFound].pos = p;
				closestPoints[pointsFound].distanceSquared = d2;
				pointsFound++;
				if (pointsFound == maxCount) {
					std::make_heap(closestPoints, closestPoints+maxCount);
					r2 = closestPoints[0].distanceSquared;
				}
			}
		});
		return pointsFound;
	}

	//! Returns the closest points to the given position.
	//! It returns the number of points found.
	int GetPoints(const PointType &position, SIZE_TYPE maxCount, PointInfo *closestPoints) const
	{
		return GetPoints(position, (std::numeric_limits<FType>::max)(), maxCount, closestPoints);
	}

	/////////////////////////////////////////////////////////////////////////////////
	//!@name Closest point methods

	//! Returns the closest point to the given position within the given radius.
	//! It returns true, if a point is found.
	bool GetClosest(const PointType &position, FType radius, SIZE_TYPE &closestIndex, PointType &closestPosition, FType &closestDistanceSquared) const
	{
		bool found = false;
		FType dist2 = radius * radius;
		GetPoints(position, dist2, [&](SIZE_TYPE i, const PointType &p, FType d2, FType &r2){ found=true; closestIndex=i; closestPosition=p; closestDistanceSquared=d2; r2=d2; }, 1);
		return found;
	}

	//! Returns the closest point to the given position.
	//! It returns true, if a point is found.
	bool GetClosest(const PointType &position, SIZE_TYPE &closestIndex, PointType &closestPosition, FType &closestDistanceSquared) const
	{
		return GetClosest(position, (std::numeric_limits<FType>::max)(), closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point index and position to the given position within the given index.
	//! It returns true, if a point is found.
	bool GetClosest(const PointType &position, FType radius, SIZE_TYPE &closestIndex, PointType &closestPosition) const
	{
		FType closestDistanceSquared;
		return GetClosest(position, radius, closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point index and position to the given position.
	//! It returns true, if a point is found.
	bool GetClosest(const PointType &position, SIZE_TYPE &closestIndex, PointType &closestPosition) const
	{
		FType closestDistanceSquared;
		return GetClosest(position, closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point index to the given position within the given radius.
	//! It returns true, if a point is found.
	bool GetClosestIndex(const PointType &position, FType radius, SIZE_TYPE &closestIndex) const
	{
		FType closestDistanceSquared;
		PointType closestPosition;
		return GetClosest(position, radius, closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point index to the given position.
	//! It returns true, if a point is found.
	bool GetClosestIndex(const PointType &position, SIZE_TYPE &closestIndex) const
	{
		FType closestDistanceSquared;
		PointType closestPosition;
		return GetClosest(position, closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point position to the given position within the given radius.
	//! It returns true, if a point is found.
	bool GetClosestPosition(const PointType &position, FType radius, PointType &closestPosition) const
	{
		SIZE_TYPE closestIndex;
		FType closestDistanceSquared;
		return GetClosest(position, radius, closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point position to the given position.
	//! It returns true, if a point is found.
	bool GetClosestPosition(const PointType &position, PointType &closestPosition) const
	{
		SIZE_TYPE closestIndex;
		FType closestDistanceSquared;
		return GetClosest(position, closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point distance squared to the given position within the given radius.
	//! It returns true, if a point is found.
	bool GetClosestDistanceSquared(const PointType &position, FType radius, FType &closestDistanceSquared) const
	{
		SIZE_TYPE closestIndex;
		PointType closestPosition;
		return GetClosest(position, radius, closestIndex, closestPosition, closestDistanceSquared);
	}

	//! Returns the closest point distance squared to the given position.
	//! It returns true, if a point is found.
	bool GetClosestDistanceSquared(const PointType &position, FType &closestDistanceSquared) const
	{
		SIZE_TYPE closestIndex;
		PointType closestPosition;
		return GetClosest(position, closestIndex, closestPosition, closestDistanceSquared);
	}

	/////////////////////////////////////////////////////////////////////////////////

private:

	/////////////////////////////////////////////////////////////////////////////////
	//!@name Internal Structures and Methods

	class PointData {
	private:
		SIZE_TYPE indexAndSplitPlane{};	// first NBits bits indicates the splitting plane, the rest of the bits store the point index.
		PointType p{};					// point position

	public:
		void Set(const PointType &pt, SIZE_TYPE index, uint32_t plane=0)
		{
			p = pt;
			indexAndSplitPlane = static_cast<int>((static_cast<uint32_t>(index) << NBits()) | (plane & ((1u << NBits()) - 1u)));
		}

		void SetPlane(uint32_t plane)
		{
			indexAndSplitPlane = static_cast<int>((static_cast<uint32_t>(indexAndSplitPlane) & (~((1u << NBits()) - 1u))) | plane);
		}

		int       Plane() const { return indexAndSplitPlane & ((1<<NBits())-1); }
		SIZE_TYPE Index() const { return indexAndSplitPlane >> NBits(); }
		const PointType& Pos() const { return p; }
	private:
#if defined(__cpp_constexpr) || (defined(_MSC_VER) && _MSC_VER >= 1900)
		constexpr uint32_t NBits(uint32_t v = DIMENSIONS) const
		{
			return v < 2 ? v : 1 + NBits(v >> 1);
		}
#else
		int NBits() const { int v = DIMENSIONS-1, r, s; r=(v>0xF)<<2; v>>=r; s=(v>0x3)<<1; v>>=s; r|=s|(v>>1); return r+1; }	// Supports up to 256 dimensions
#endif
	};

	PointData *points = nullptr;		// Keeps the points as a k-d tree.
	SIZE_TYPE  pointCount = 0;	// Keeps the point count.
	SIZE_TYPE  numInternal = 0;	// Keeps the number of internal k-d tree nodes.

	// The main method for recursively building the k-d tree.
	void BuildKDTree(PointData *orig, PointType boundMin, PointType boundMax, SIZE_TYPE kdIndex, SIZE_TYPE ixStart, SIZE_TYPE ixEnd)
	{
		SIZE_TYPE n = ixEnd - ixStart;
		if (n > 1) {
			auto axis = static_cast<unsigned>(SplitAxis(boundMin, boundMax));
			SIZE_TYPE leftSize = LeftSize(n);
			SIZE_TYPE ixMid = ixStart+leftSize;
			std::nth_element(orig+ixStart, orig+ixMid, orig+ixEnd, [axis](const PointData &a, const PointData &b){ return a.Pos()[axis] < b.Pos()[axis]; });
			points[kdIndex] = orig[ixMid];
			points[kdIndex].SetPlane(axis);
			PointType bMax = boundMax;
			bMax[axis] = orig[ixMid].Pos()[axis];
			PointType bMin = boundMin;
			bMin[axis] = orig[ixMid].Pos()[axis];
#ifdef _CY_PARALLEL_LIB
			const SIZE_TYPE parallel_invoke_threshold = 256;
			if (ixMid-ixStart > parallel_invoke_threshold && ixEnd - ixMid+1 > parallel_invoke_threshold) {
				_CY_PARALLEL_LIB::parallel_invoke(
					[&]{ BuildKDTree(orig, boundMin, bMax, kdIndex*2,   ixStart, ixMid); },
					[&]{ BuildKDTree(orig, bMin, boundMax, kdIndex*2+1, ixMid+1, ixEnd); }
				);
			} else
#endif
			{
				BuildKDTree(orig, boundMin, bMax, kdIndex*2,   ixStart, ixMid);
				BuildKDTree(orig, bMin, boundMax, kdIndex*2+1, ixMid+1, ixEnd);
			}
		} else if (n > 0) {
			points[kdIndex] = orig[ixStart];
		}
	}

	// Returns the total number of nodes on the left sub-tree of a complete k-d tree of size n.
	static SIZE_TYPE LeftSize(SIZE_TYPE n)
	{
		SIZE_TYPE f = n; // Size of the full tree
		for (SIZE_TYPE s=1; s<static_cast<int>(8*sizeof(SIZE_TYPE)); s*=2) f |= f >> s;
		SIZE_TYPE l = f >> 1; // Size of the full left child
		SIZE_TYPE r = l >> 1; // Size of the full right child without leaf nodes
		return (l+r+1 <= n) ? l : n-r-1;
	}

	// Returns axis with the largest span, used as the splitting axis for building the k-d tree
	static int SplitAxis(const PointType &boundMin, const PointType &boundMax)
	{
		PointType d = boundMax - boundMin;
		int axis = 0;
		FType dmax = d[0];
		for (auto j=1ul; j<DIMENSIONS; j++) {
			if (dmax < d[j]) {
				axis = static_cast<int>(j);
				dmax = d[j];
			}
		}
		return axis;
	}

	template <typename _CALLBACK>
	void GetPoints(const PointType &position, FType &dist2, _CALLBACK pointFound, SIZE_TYPE nodeID) const
	{
		SIZE_TYPE stack[sizeof(SIZE_TYPE)*8];
		SIZE_TYPE stackPos = 0;

		TraverseCloser(position, dist2, pointFound, nodeID, stack, stackPos);

		// empty the stack
		while (stackPos > 0) {
			SIZE_TYPE nodeID_loc = stack[--stackPos];
			// check the internal node point
			const PointData &p = points[nodeID_loc];
			const PointType pos = p.Pos();
			int axis = p.Plane();
			float dist1 = position[axis] - pos[axis];
			if (dist1*dist1 < dist2) {
				// check its point
				FType d2 = (position - pos).LengthSquared();
				if (d2 < dist2) pointFound(p.Index(), pos, d2, dist2);
				// traverse down the other child node
				SIZE_TYPE child = 2*nodeID_loc;
				nodeID_loc = dist1 < 0 ? child+1 : child;
				TraverseCloser(position, dist2, pointFound, nodeID_loc, stack, stackPos);
			}
		}
	}

	template <typename _CALLBACK>
	void TraverseCloser(const PointType &position, FType &dist2, _CALLBACK pointFound, SIZE_TYPE nodeID, SIZE_TYPE *stack, SIZE_TYPE &stackPos) const
	{
		// Traverse down to a leaf node along the closer branch
		while (nodeID <= numInternal) {
			stack[stackPos++] = nodeID;
			const PointData &p = points[nodeID];
			const PointType pos = p.Pos();
			int axis = p.Plane();
			float dist1 = position[axis] - pos[axis];
			uint32_t child = 2*nodeID;
			nodeID = dist1 < 0 ? child : child + 1;
		}
		// Now we are at a leaf node, do the test
		const PointData &p = points[nodeID];
		const PointType pos = p.Pos();
		FType d2 = (position - pos).LengthSquared();
		if (d2 < dist2) pointFound(p.Index(), pos, d2, dist2);
	}

	/////////////////////////////////////////////////////////////////////////////////
};

namespace dls::math {

inline auto operator<<=(dls::math::vec3i &v, int s)
{
	v.x <<= s;
	v.y <<= s;
	v.z <<= s;

	return v;
}

}

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
		PointCloud<dls::math::vec3f, float, 3, int> pc{};
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

		return m_niveaux[niveau].pc.GetPointCount();
	}

	int   GetNumLevels() const { return m_nombre_niveaux; }	//!< Returns the number of levels in the hierarchy.
	float GetCellSize () const { return m_taille_cellule; }		//!< Returns the size of a cell in the lowest (finest) level of the hierarchy.
	const dls::math::vec3f& GetLightPos   (int level, int i) const { return m_niveaux[level].pc.GetPoint(i); }							//!< Returns the i^th light position at the given level. Note that this is not the position of the light with index i.
	int      GetLightIndex (int level, int i) const { return m_niveaux[level].pc.GetPointIndex(i); }						//!< Returns the i^th light index at the given level.
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

			m_niveaux[0].pc.GetPoints(pos, r*2, [&](int i, const dls::math::vec3f &p, float dist2, float &radius2)
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

				m_niveaux[level].pc.GetPoints(pos, r*2, [&](int i, const dls::math::vec3f &p, float dist2, float &radius2)
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
			auto n = m_niveaux[m_nombre_niveaux - 1].pc.GetPointCount();

			for (auto i = 0; i < n; i++) {
				auto const &p = m_niveaux[m_nombre_niveaux-1].pc.GetPoint(i);
				auto dist2 = longueur_carree(pos - p);

				if (dist2 <= rr_min) {
					continue;
				}

				auto id = m_niveaux[m_nombre_niveaux-1].pc.GetPointIndex(i);
				auto c = m_niveaux[m_nombre_niveaux-1].colors[id];

				if (dist2 < rr) {
					c *= (std::sqrt(dist2) - r_min) / r_min;
				}

				callLightingFunc(m_nombre_niveaux-1, i, p, c);
			}
		}
		else {
			// Single-level (a.k.a. brute-force)
			auto n = m_niveaux[0].pc.GetPointCount();
			for (auto i=0; i<n; i++) {
				auto const &p = m_niveaux[0].pc.GetPoint(i);
				auto id = m_niveaux[0].pc.GetPointIndex(i);
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

			thisLevel.pc.Build(lightCount, pos.donnees());
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

		m_niveaux[0].pc.Build(numLights, pos.donnees());
		this->m_taille_cellule = nodeCellSize;

		return true;
	}
};


class OpGrilleEclairage final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Création Grille Éclairage";
	static constexpr auto AIDE = "";

	OpGrilleEclairage(Graphe &graphe_parent, Noeud *noeud)
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
}

#pragma clang diagnostic pop
