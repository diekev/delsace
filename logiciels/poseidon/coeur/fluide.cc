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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "fluide.h"

#include "biblinternes/chrono/outils.hh"

#include <random>

#include "biblinternes/objets/creation.h"
#include "biblinternes/outils/parallelisme.h"

#include "adaptrice_creation_maillage.h"
#include "imcompressibilite.h"
#include "maillage.h"

#undef CHRONOMETRE_PORTEE
#define CHRONOMETRE_PORTEE(x, y)

#undef VELOCITE_SEPAREE

Fluide::Fluide()
{
	temps_debut = 0;
	temps_fin = 250;
	temps_courant = 0;
	temps_precedent = 0;
	source = new Maillage;
	domaine = new Maillage;
	res = dls::math::vec3<size_t>(32ul);

	AdaptriceCreationMaillage adaptrice;

	adaptrice.maillage = source;
	objets::cree_boite(&adaptrice, 2.0f, 2.0f, 2.0f, 0.0f, 8.0f, 0.0f);
	source->calcule_boite_englobante();

	adaptrice.maillage = domaine;
	objets::cree_boite(&adaptrice, 8.0f, 8.0f, 8.0f, 0.0f, 8.0f, 0.0f);
	domaine->calcule_boite_englobante();

	ajourne_pour_nouveau_temps();
}

Fluide::~Fluide()
{
	delete domaine;
	delete source;
}

void Fluide::ajourne_pour_nouveau_temps()
{
	/* Réinitialise si nous sommes à la première image. */
	if (temps_courant == temps_debut) {
		initialise();
		cree_particule(this, 8ul);
		return;
	}

	/* Ne simule pas si l'on a pas avancé d'une image. */
	if (temps_precedent + 1 != temps_courant) {
		return;
	}

	//std::cerr << "------------------------------------------------------\n";

	CHRONOMETRE_PORTEE(__func__, std::cerr);

	/* Réinitialise les drapeaux. */
	drapeaux.initialise(res.x, res.y, res.z);

	construit_grille_particule();

	/* Pour chaque voxel staggered, calcul la moyenne des vélocités des
	 * particules avoisinantes. */
	transfert_velocite();

	/* Pour FLIP : sauvegarde les vélocités. */
#ifdef VELOCITE_SEPAREE
	velocite_x_ancienne.copie(velocite_x);
	velocite_y_ancienne.copie(velocite_y);
	velocite_z_ancienne.copie(velocite_z);
#else
	ancienne_velocites.copie(velocite);
#endif

	/* Fais toutes les étapes de non-advection standardes sur la grille. */
	ajoute_acceleration();
	construit_champs_distance();
	etend_champs_velocite();
	conditions_bordure();
#ifdef VELOCITE_SEPAREE
	rend_imcompressible(velocite_x, velocite_y, velocite_z, drapeaux);
#else
	rend_imcompressible(velocite, drapeaux);
#endif
	etend_champs_velocite();

	/* Pour FLIP : soustraction des nouvelles vélocités des anciennes, et ajout
	 * des différences entrepolées aux particules. */
	soustrait_velocite();

	/* Pour PIC : entrepole les nouvelles vélocités aux particules. */
	sauvegarde_velocite_PIC();

	/* Pousse les particules dans le fluide, advection. */
	advecte_particules();
}

void Fluide::initialise()
{
	phi.initialise(res.x, res.y, res.z);
#ifdef VELOCITE_SEPAREE
	velocite_x.initialise(res.x + 1, res.y, res.z);
	velocite_y.initialise(res.x, res.y + 1, res.z);
	velocite_z.initialise(res.x, res.y, res.z + 1);
#else
	velocite.initialise(res.x, res.y, res.z);
	ancienne_velocites.initialise(res.x, res.y, res.z);
#endif
}

void Fluide::construit_grille_particule()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	auto const &min_domaine = domaine->min();
	auto const &taille_domaine = domaine->taille();

	grille_particules.initialise(res.x, res.y, res.z);

	for (auto &p : particules) {
		auto pos_domaine = p.pos - min_domaine;
		pos_domaine.x /= taille_domaine.x;
		pos_domaine.y /= taille_domaine.y;
		pos_domaine.z /= taille_domaine.z;

		pos_domaine.x *= static_cast<float>(res.x);
		pos_domaine.y *= static_cast<float>(res.y);
		pos_domaine.z *= static_cast<float>(res.z);

		grille_particules.ajoute_particule(
					static_cast<size_t>(pos_domaine.x),
					static_cast<size_t>(pos_domaine.y),
					static_cast<size_t>(pos_domaine.z),
					&p);
	}
}

void Fluide::transfert_velocite()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
#if 0
	auto const &min_domaine = domaine->min();
	auto const &taille_domaine = domaine->taille();

	for (auto &p : particules) {
		auto pos_domaine = p.pos - min_domaine;
		pos_domaine.x /= taille_domaine.x;
		pos_domaine.y /= taille_domaine.y;
		pos_domaine.z /= taille_domaine.z;

		pos_domaine.x *= res.x;
		pos_domaine.y *= res.y;
		pos_domaine.z *= res.z;

		velocite.valeur(pos_domaine.x, pos_domaine.y, pos_domaine.z, p.vel);
		drapeaux.valeur(pos_domaine.x, pos_domaine.y, pos_domaine.z, CELLULE_FLUIDE);
	}
#else

	boucle_parallele(tbb::blocked_range<size_t>(0, res.z),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		size_t index = plage.begin() * res.x * res.y;

		for (auto z = plage.begin(); z < plage.end(); ++z) {
			for (size_t y = 0; y < res.y; ++y) {
				for (size_t x = 0; x < res.x; ++x, ++index) {
					auto const &parts = grille_particules.particule(x, y, z);

					auto vel = dls::math::vec3f(0.0f);
					auto poids = 0.0f;

					for (auto &p : parts) {
						vel += p->vel * 0.95f + p->vel_pic * 0.05f;
						poids += 1.0f;
					}

					if (poids == 0.0f) {
						vel = dls::math::vec3f(0.0f);
					}
					else {
						drapeaux.valeur(index, CELLULE_FLUIDE);
						vel /= poids;
					}

#ifdef VELOCITE_SEPAREE
					velocite_x.valeur(index + 1, vel.x);
					velocite_y.valeur(index + res.x, vel.y);
					velocite_z.valeur(index + res.x * res.y, vel.z);
#else
					velocite.valeur(index, vel);
#endif
				}
			}
		}
	});
#endif
}

void Fluide::ajoute_acceleration()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	auto const &taille_domaine = domaine->taille();

	dls::math::vec3f dh_domaine(
		taille_domaine[0] / static_cast<float>(res.x),
		taille_domaine[1] / static_cast<float>(res.y),
		taille_domaine[2] / static_cast<float>(res.z)
	);

	auto dt = 1.0f / 24.0f;
	auto gravite = -9.81f * dh_domaine.x * dt;

	boucle_parallele(tbb::blocked_range<size_t>(0, res.z),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		size_t index = plage.begin() * res.x * res.y;

		for (size_t z = plage.begin(); z < plage.end(); ++z) {
			for (size_t y = 0; y < res.y; ++y) {
				for (size_t x = 0; x < res.x; ++x, ++index) {
					if (drapeaux.valeur(index) != CELLULE_FLUIDE) {
						continue;
					}

#ifdef VELOCITE_SEPAREE
					//auto vel_x = velocite_x.valeur(index + 1);
					auto vel_y = velocite_y.valeur(index + res.x);
					//auto vel_z = velocite_z.valeur(index + res.x * res.y);

					vel_y += gravite;

					//velocite_x.valeur(index + 1, vel_x);
					velocite_y.valeur(index + res.x, vel_y);
					//velocite_z.valeur(index + res.x * res.y, vel_z);
#else
					auto vel = velocite.valeur(index);
					vel.y += gravite;
					velocite.valeur(index, vel);
#endif
				}
			}
		}
	});
}

void Fluide::conditions_bordure()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	boucle_parallele(tbb::blocked_range<size_t>(0, res.y),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		for (size_t y = plage.begin(); y < plage.end(); ++y) {
			for (size_t x = 0; x < res.x; ++x) {
#ifdef VELOCITE_SEPAREE
				velocite_z.valeur(x, y, 0, 0.0f);
				velocite_z.valeur(x, y, res.z - 1, 0.0f);
#else
				velocite.valeur(x, y, 0, dls::math::vec3f(0.0f));
				velocite.valeur(x, y, res.z - 1, dls::math::vec3f(0.0f));
#endif
			}
		}
	});

	boucle_parallele(tbb::blocked_range<size_t>(0, res.z),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		for (size_t z = plage.begin(); z < plage.end(); ++z) {
			for (size_t x = 0; x < res.x; ++x) {
#ifdef VELOCITE_SEPAREE
				velocite_y.valeur(x, 0, z, 0.0f);
				velocite_y.valeur(x, res.y - 1, z, 0.0f);
#else
				velocite.valeur(x, 0, z, dls::math::vec3f(0.0f));
				velocite.valeur(x, res.y - 1, z, dls::math::vec3f(0.0f));
#endif
			}
		}
	});

	boucle_parallele(tbb::blocked_range<size_t>(0, res.z),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		for (size_t z = plage.begin(); z < plage.end(); ++z) {
			for (size_t y = 0; y < res.y; ++y) {
#ifdef VELOCITE_SEPAREE
				velocite_x.valeur(0, y, z, 0.0f);
				velocite_x.valeur(res.x - 1, y, z, 0.0f);
#else
				velocite.valeur(0, y, z, dls::math::vec3f(0.0f));
				velocite.valeur(res.x - 1, y, z, dls::math::vec3f(0.0f));
#endif
			}
		}
	});
}

void Fluide::soustrait_velocite()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	boucle_parallele(tbb::blocked_range<size_t>(0, res.z),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		size_t index = plage.begin() * res.x * res.y;

		for (size_t z = plage.begin(); z < plage.end(); ++z) {
			for (size_t y = 0; y < res.y; ++y) {
				for (size_t x = 0; x < res.x; ++x, ++index) {
#ifdef VELOCITE_SEPAREE
					auto vel_x = velocite_x.valeur(index + 1) - velocite_x_ancienne.valeur(index + 1);
					auto vel_y = velocite_y.valeur(index + res.x) - velocite_y_ancienne.valeur(index + res.x);
					auto vel_z = velocite_z.valeur(index + res.x * res.y) - velocite_z_ancienne.valeur(index + res.x * res.y);
#else
					//auto vel = velocite.valeur(index) - ancienne_velocites.valeur(index);
#endif
					/* Assigne la vélocité aux particules avoisinantes. */
				}
			}
		}
	});
}

/**
 * A FAST SWEEPING METHOD FOR EIKONAL EQUATIONS
 * https://www.ams.org/journals/mcom/2005-74-250/S0025-5718-04-01678-3/S0025-5718-04-01678-3.pdf
 */
static void calcule_distance(Grille<float> &phi, size_t x, size_t y, size_t z, float h)
{
	auto a = std::min(phi.valeur(x - 1, y, z), phi.valeur(x + 1, y, z));
	auto b = std::min(phi.valeur(x, y - 1, z), phi.valeur(x, y + 1, z));
	auto c = std::min(phi.valeur(x, y, z - 1), phi.valeur(x, y, z + 1));

	if (a > b) {
		std::swap(a, b);
	}
	if (b > c) {
		std::swap(b, c);
	}
	if (a > b) {
		std::swap(a, b);
	}

	float xi;

	if (b >= a + h) {
		xi = a + h;
	}
	else {
		xi = 0.5f * (a + b + std::sqrt(2.0f * h * h - (a - b) * (a - b)));

		/* À FAIRE : trouver la généralisation. */
//		if (xi > c) {
//			xi = (1.0f / 3.0f)
//				* (a + b + c
//				   + sqrt(3.0f * h * h * h + std::pow(a + b + c, 2.0) - 3.0f * h * h * h * (a * a + b * b + c * c)));
//		}
	}

	auto u_ijk = phi.valeur(x, y, z);
	phi.valeur(x, y, z, std::min(xi, u_ijk));
}

void Fluide::construit_champs_distance()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	/* Initialise phi selon les drapeaux. */
	auto const &taille_domaine = domaine->taille();
	auto distance_max = dls::math::longueur(taille_domaine);

	phi.arriere_plan(distance_max);

	boucle_parallele(tbb::blocked_range<size_t>(0, res.z),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		size_t index = plage.begin() * res.x * res.y;

		for (size_t z = plage.begin(); z < plage.end(); ++z) {
			for (size_t y = 0; y < res.y; ++y) {
				for (size_t x = 0; x < res.x; ++x, ++index) {
					auto valeur = 0.0f;

					if (drapeaux.valeur(index) != CELLULE_FLUIDE) {
						valeur = distance_max;
					}

					phi.valeur(index, valeur);
				}
			}
		}
	});

	/* Sweep : x : 0 -> X, y : 0 -> Y, z : 0 -> Z */
	/* Sweep : x : 0 -> X, y : 0 -> Y, z : Z -> 0 */
	/* Sweep : x : 0 -> X, y : Y -> 0, z : 0 -> Z */
	/* Sweep : x : 0 -> X, y : Y -> 0, z : Z -> 0 */
	/* Sweep : x : X -> 0, y : 0 -> Y, z : 0 -> Z */
	/* Sweep : x : X -> 0, y : 0 -> Y, z : Z -> 0 */
	/* Sweep : x : X -> 0, y : Y -> 0, z : 0 -> Z */
	/* Sweep : x : X -> 0, y : Y -> 0, z : Z -> 0 */

	/* À FAIRE : taille grille uniforme. */
	auto h = 4.0f / static_cast<float>(res.x);

	/* À FAIRE : différence sens unique près de bord du domaine. */

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

	for (size_t z = res.z - 1; z < -1ul; --z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = res.y - 1; y < -1ul; --y) {
			for (size_t x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

	for (size_t z = res.z - 1; z < -1ul; --z) {
		for (size_t y = res.y - 1; y < -1ul; --y) {
			for (size_t x = 0; x < res.x; ++x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = res.x - 1; x < -1ul; --x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

	for (size_t z = res.z - 1; z < -1ul; --z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = res.x - 1; x < -1ul; --x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = res.y - 1; y < -1ul; --y) {
			for (size_t x = res.x - 1; x < -1ul; --x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

	for (size_t z = res.z - 1; z < -1ul; --z) {
		for (size_t y = res.y - 1; y < -1ul; --y) {
			for (size_t x = res.x - 1; x < -1ul; --x) {
				if (drapeaux.valeur(x, y, z) != CELLULE_FLUIDE) {
					calcule_distance(phi, x, y, z, h);
				}
			}
		}
	}

#ifdef DEBOGAGE_CHAMPS_DISTANCE
	auto dist_max = -1.0f;

	for (size_t z = 0; z < res.z; ++z) {
		for (size_t y = 0; y < res.y; ++y) {
			for (size_t x = 0; x < res.x; ++x) {
				dist_max = std::max(dist_max, phi.valeur(x, y, z));
			}
		}
	}

	std::cerr << "Distance max (diagonale) " << distance_max << '\n';
	std::cerr << "Distance max (calculée)  " << dist_max << '\n';
#endif
}

void Fluide::etend_champs_velocite()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	/* Extrapole le champs de vélocité en utilisant le posize_t de surface le plus
	 * proche.
	 * Animation and Rendering of Complex Water Surfaces, Enright et al. 2002,
	 * http://physbam.stanford.edu/~fedkiw/papers/stanford2002-03.pdf.
	 */

	auto const &taille_domaine = domaine->taille();

	dls::math::vec3f dh(
		taille_domaine[0] / static_cast<float>(res.x),
		taille_domaine[1] / static_cast<float>(res.y),
		taille_domaine[2] / static_cast<float>(res.z)
	);

	boucle_parallele(tbb::blocked_range<size_t>(0, res.z),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		const size_t decalage_z = res.x * res.y;
		size_t index = plage.begin() * res.x * res.y;

		for (size_t z = plage.begin(); z < plage.end(); ++z) {
			for (size_t y = 0; y < res.y; ++y) {
				for (size_t x = 0; x < res.x; ++x, ++index) {
					if (drapeaux.valeur(index) == CELLULE_FLUIDE) {
						continue;
					}

					/* calcule le gradient de phi pour savoir la direction du voxel
					 * le plus proche. */

#if 1
					auto const x0 = phi.valeur(index - 1);
					auto const x1 = phi.valeur(index + 1);
					auto const y0 = phi.valeur(index - res.x);
					auto const y1 = phi.valeur(index + res.x);
					auto const z0 = phi.valeur(index - decalage_z);
					auto const z1 = phi.valeur(index + decalage_z);
#else
					auto const x0 = phi.valeur(x - 1, y, z);
					auto const x1 = phi.valeur(x + 1, y, z);
					auto const y0 = phi.valeur(x, y - 1, z);
					auto const y1 = phi.valeur(x, y + 1, z);
					auto const z0 = phi.valeur(x, y, z - 1);
					auto const z1 = phi.valeur(x, y, z + 1);
#endif

					auto const i = x1 - x0;
					auto const j = y1 - y0;
					auto const k = z1 - z0;

					auto const xi = i / dh.x;
					auto const xj = j / dh.y;
					auto const xk = k / dh.z;

#ifdef VELOCITE_SEPAREE
					auto const vel_x = velocite_x.valeur((x + 1 - xi) + (y - xj) * res.x + (z - xk) * decalage_z);
					auto const vel_y = velocite_y.valeur((x - xi) + (y + 1 - xj) * res.x + (z - xk) * decalage_z);
					auto const vel_z = velocite_z.valeur((x - xi) + (y - xj) * res.x + (z + 1 - xk) * decalage_z);
//					velocite_x.valeur(x + 1, y, z, vel_x);
//					velocite_y.valeur(x, y + 1, z, vel_y);
//					velocite_z.valeur(x, y, z + 1, vel_z);
					velocite_x.valeur(index + 1, vel_x);
					velocite_y.valeur(index + res.x, vel_y);
					velocite_z.valeur(index + decalage_z, vel_z);
#else
					velocite.valeur(index, velocite.valeur(
										x - static_cast<size_t>(xi),
										y - static_cast<size_t>(xj),
										z - static_cast<size_t>(xk)));
#endif
				}
			}
		}
	});
}

void Fluide::sauvegarde_velocite_PIC()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	auto const &min_domaine = domaine->min();
	auto const &taille_domaine = domaine->taille();

	boucle_parallele(tbb::blocked_range<size_t>(0, particules.size()),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		for (size_t i = plage.begin(); i < plage.end(); ++i) {
			auto &p = particules[i];

			auto pos_domaine = p.pos - min_domaine;
			pos_domaine.x /= taille_domaine.x;
			pos_domaine.y /= taille_domaine.y;
			pos_domaine.z /= taille_domaine.z;

			pos_domaine.x *= static_cast<float>(res.x);
			pos_domaine.y *= static_cast<float>(res.y);
			pos_domaine.z *= static_cast<float>(res.z);

#ifdef VELOCITE_SEPAREE
			auto index = pos_domaine.x + pos_domaine.y * res.x + pos_domaine.z * res.x * res.y;
			auto vel_x = velocite_x.valeur(index + 1);
			auto vel_y = velocite_y.valeur(index + res.x);
			auto vel_z = velocite_z.valeur(index + res.x * res.y);
			p.vel_pic = dls::math::vec3f(vel_x, vel_y, vel_z);
#else
			p.vel_pic = velocite.valeur(
							static_cast<size_t>(pos_domaine.x),
							static_cast<size_t>(pos_domaine.y),
							static_cast<size_t>(pos_domaine.z));
#endif
		}
	});
}

void Fluide::advecte_particules()
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
//	auto gravite = -0.01;

//	for (auto &particule : particules) {
//		particule.pos.y += gravite;
//	}

	auto const &min_domaine = domaine->min();
	auto const &taille_domaine = domaine->taille();


	boucle_parallele(tbb::blocked_range<size_t>(0, particules.size()),
					 [&](tbb::blocked_range<size_t> const &plage)
	{
		for (size_t i = plage.begin(); i < plage.end(); ++i) {
			auto &p = particules[i];
			auto pos_domaine = p.pos - min_domaine;
			pos_domaine.x /= taille_domaine.x;
			pos_domaine.y /= taille_domaine.y;
			pos_domaine.z /= taille_domaine.z;

			pos_domaine.x *= static_cast<float>(res.x);
			pos_domaine.y *= static_cast<float>(res.y);
			pos_domaine.z *= static_cast<float>(res.z);

#ifdef VELOCITE_SEPAREE
			auto index = pos_domaine.x + pos_domaine.y * res.x + pos_domaine.z * res.x * res.y;
			auto vel_x = velocite_x.valeur(index + 1);
			auto vel_y = velocite_y.valeur(index + res.x);
			auto vel_z = velocite_z.valeur(index + res.x * res.y);

			pos_domaine = (p.pos - min_domaine) - dls::math::vec3f(vel_x, vel_y, vel_z);
#else
			auto vel = velocite.valeur(
						   static_cast<size_t>(pos_domaine.x),
						   static_cast<size_t>(pos_domaine.y),
						   static_cast<size_t>(pos_domaine.z));

			pos_domaine = (p.pos - min_domaine) - vel;
#endif
			pos_domaine.x /= taille_domaine.x;
			pos_domaine.y /= taille_domaine.y;
			pos_domaine.z /= taille_domaine.z;

			pos_domaine.x *= static_cast<float>(res.x);
			pos_domaine.y *= static_cast<float>(res.y);
			pos_domaine.z *= static_cast<float>(res.z);

#ifdef VELOCITE_SEPAREE
			index = pos_domaine.x + pos_domaine.y * res.x + pos_domaine.z * res.x * res.y;
			vel_x = velocite_x.valeur(index + 1);
			vel_y = velocite_y.valeur(index + res.x);
			vel_z = velocite_z.valeur(index + res.x * res.y);

			p.pos += dls::math::vec3f(vel_x, vel_y, vel_z);
#else
			vel = velocite.valeur(
					  static_cast<size_t>(pos_domaine.x),
					  static_cast<size_t>(pos_domaine.y),
					  static_cast<size_t>(pos_domaine.z));
			p.pos += vel;
#endif
		}
	});
}

bool contiens(dls::math::vec3f const &min,
			  dls::math::vec3f const &max,
			  dls::math::vec3f const &pos)
{
	for (size_t i = 0; i < 3; ++i) {
		if (pos[i] < min[i] || pos[i] >= max[i]) {
			return false;
		}
	}

	return true;
}

void cree_particule(Fluide *fluide, size_t nombre)
{
	CHRONOMETRE_PORTEE(__func__, std::cerr);
	fluide->particules.clear();

	auto const &min_source = fluide->source->min();
	auto const &max_source = fluide->source->max();

	auto const &min_domaine = fluide->domaine->min();
	auto const &taille_domaine = fluide->domaine->taille();

	dls::math::vec3f dh_domaine(
		taille_domaine[0] / static_cast<float>(fluide->res.x),
		taille_domaine[1] / static_cast<float>(fluide->res.y),
		taille_domaine[2] / static_cast<float>(fluide->res.z)
	);

	auto dh_2 = dh_domaine * 0.50f;
	auto dh_4 = dh_domaine * 0.25f;

	Particule p;

	std::mt19937 rng(179541);
	std::uniform_real_distribution<float> dist_x(-dh_4.x, dh_4.x);
	std::uniform_real_distribution<float> dist_y(-dh_4.y, dh_4.y);
	std::uniform_real_distribution<float> dist_z(-dh_4.z, dh_4.z);

	dls::math::vec3f decalage[8] = {
		dls::math::vec3f(-dh_4.x, -dh_4.y, -dh_4.z),
		dls::math::vec3f(-dh_4.x,  dh_4.y, -dh_4.z),
		dls::math::vec3f(-dh_4.x, -dh_4.y,  dh_4.z),
		dls::math::vec3f(-dh_4.x,  dh_4.y,  dh_4.z),
		dls::math::vec3f( dh_4.x, -dh_4.y, -dh_4.z),
		dls::math::vec3f( dh_4.x,  dh_4.y, -dh_4.z),
		dls::math::vec3f( dh_4.x, -dh_4.y,  dh_4.z),
		dls::math::vec3f( dh_4.x,  dh_4.y,  dh_4.z),
	};

	/* trouve si les voxels entresectent la source, si oui, ajoute des
	 * particules */
	for (size_t z = 0; z < fluide->res.z; ++z) {
		for (size_t y = 0; y < fluide->res.y; ++y) {
			for (size_t x = 0; x < fluide->res.x; ++x) {
				auto pos = min_domaine;
				pos.x += static_cast<float>(x) * dh_domaine[0];
				pos.y += static_cast<float>(y) * dh_domaine[1];
				pos.z += static_cast<float>(z) * dh_domaine[2];

				if (!contiens(min_source, max_source, pos)) {
					continue;
				}

				auto centre_voxel = pos + dh_2;

				for (size_t i = 0; i < nombre; ++i) {
					auto pos_p = centre_voxel + decalage[i];

					p.pos.x = pos_p.x + dist_x(rng);
					p.pos.y = pos_p.y + dist_y(rng);
					p.pos.z = pos_p.z + dist_z(rng);
					p.vel = dls::math::vec3f(0.0f);

					fluide->particules.push_back(p);
				}
			}
		}
	}
}
