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

#include "ondelette.hh"

#include <mutex>

#include "biblinternes/outils/gna.hh"
#include "biblinternes/math/outils.hh"
#include "biblinternes/memoire/logeuse_memoire.hh"
#include "biblinternes/moultfilage/boucle.hh"

namespace bruit {

/**
 * Implémentation de la génération de tuiles pour les bruits d'ondelettes.
 * L'implémentation dérive principalement de la publication « Wavelet Noise » de
 * Pixar : http://graphics.pixar.com/library/WaveletNoise/paper.pdf.
 *
 * Des choses manquent par rapport au papier :
 * - tile meshing
 * - decorrelating bands
 * - fading out the last band, pour l'évaluation multi-band
 *
 * Il faudra également voir d'autres implémentation de cet algorithme pour des
 * améliorations possibles.
 */

static inline auto mod_lent(int x, int n)
{
	int m = x % n;
	return (m < 0) ? m + n : m;
}

static inline auto mod_rapide_128(int n)
{
	return n & 127;
}

static constexpr auto ARAD = 16;

static float coeffs_a[2 * ARAD] = {
	0.000334f,-0.001528f, 0.000410f, 0.003545f,-0.000938f,-0.008233f, 0.002172f, 0.019120f,
	-0.005040f,-0.044412f, 0.011655f, 0.103311f,-0.025936f,-0.243780f, 0.033979f, 0.655340f,
	0.655340f, 0.033979f,-0.243780f,-0.025936f, 0.103311f, 0.011655f,-0.044412f,-0.005040f,
	0.019120f, 0.002172f,-0.008233f,-0.000938f, 0.003546f, 0.000410f,-0.001528f, 0.000334f
};

void sous_echantillonne(float *from, float *to, int n, int stride)
{
	float *a = &coeffs_a[ARAD];

	for (int i = 0; i < n / 2; i++) {
		to[i * stride] = 0;

		for (int k = 2 * i - ARAD; k < 2 * i + ARAD; k++) {
			auto f = from[mod_rapide_128(k) * stride];
			auto a_ = a[k - 2 * i];
			to[i * stride] += a_ * f;
		}
	}
}

static float coeffs_p[4] = { 0.25, 0.75, 0.75, 0.25 };

void sur_echantillonne(float *from, float *to, int n, int stride)
{
	float *p = &coeffs_p[2];

	for (int i = 0; i < n; i++) {
		to[i * stride] = 0;

		for (int k= i / 2; k <= i / 2 + 1; k++) {
			to[i * stride] += p[i - 2 * k] * from[mod_lent(k, n / 2) * stride];
		}
	}
}

/* Cette fonction prenait un paramètre d'overlapping. */
static void genere_tuile_bruit(float *&bruit, int n)
{
	/* La taille doit être paire. */
	if (n % 2 != 0) {
		n++;
	}

	auto const sz = n * n * n * static_cast<long>(sizeof(float));

	auto temp1 = memoire::loge_tableau<float>("temp1", sz);
	auto temp2 = memoire::loge_tableau<float>("temp2", sz);

	/* On ne peut pas utiliser memoire::loge_tableau car la déallocation se
	 * fera après la fin du début quand C++ appelera les destructeurs des
	 * globales, et on ne peut savoir si constructrice_bruit sera détruite avant
	 * ou après logeuse_memoire, donc nous allouons via new. */
	bruit = new float[static_cast<size_t>(sz)];

	/* NOTE : toutes les implémentation du bruit ondelette semblent être les
	 * mêmes, venant d'une même source, mais personne ne calcul le bruit en
	 * parallèle, ce qui n'est pas impossible. La raison semblerait que le bruit
	 * est mis en cache dans un fichier temporaire. La génération en série peut
	 * prendre plusieurs secondes au lancement des programmes quand le fichier
	 * temporaire à été supprimé ou est non encore généré.
	 *
	 * Il existe une différence entre le résultat calculé en série et celui en
	 * parallèle due à la génération parallèle de nombre aléatoire : utiliser
	 * l'index de début de plage comme décalage de graine ne semble pas suffir
	 * pour générer la même séquence.
	 *
	 * Enfin, après un simple profilage, les emboutteillages de l'algorithme
	 * sont la génération de nombres aléatoires avec une distribuion gaussienne,
	 * et le sur/sous-échantillonnage sur l'axe Z. Cette dernière opération
	 * prenant jusque 40% du temps de génération de la tuile.
	 */

	/* Étape 1. Remplis la tuile avec des nombres aléatoires entre -1 et 1. */
	boucle_parallele(tbb::blocked_range<long>(0, sz),
					 [&](tbb::blocked_range<long> const &plage)
	{
		auto gna = GNA{static_cast<int>(plage.begin())};

		for (auto i = plage.begin(); i < plage.end(); i++) {
			bruit[i] = gna.normale(0.0f, 1.0f);
		}
	});

	/* Étape 2 and 3. Sur et souséchantillonne la tuile. */

	/* chaque ligne X */
	boucle_parallele(tbb::blocked_range<long>(0, n),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto iz = plage.begin(); iz < plage.end(); iz++) {
			for (auto iy = 0; iy < n; iy++) {
				auto i = iy * n + iz * n * n;
				sous_echantillonne(&bruit[i], &temp1[i], n, 1);
				sur_echantillonne(&temp1[i], &temp2[i], n, 1);
			}
		}
	});

	/* chaque ligne Y */
	boucle_parallele(tbb::blocked_range<long>(0, n),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto iz = plage.begin(); iz < plage.end(); iz++) {
			for (auto ix = 0; ix < n; ix++) {
				auto i = ix + iz * n * n;
				sous_echantillonne(&temp2[i], &temp1[i], n, n);
				sur_echantillonne(&temp1[i], &temp2[i], n, n);
			}
		}
	});

	/* chaque ligne Z */
	boucle_parallele(tbb::blocked_range<long>(0, n),
					 [&](tbb::blocked_range<long> const &plage)
	{
		for (auto iy = plage.begin(); iy < plage.end(); iy++) {
			for (auto ix = 0; ix < n; ix++) {
				auto i = ix + iy * n;
				sous_echantillonne(&temp2[i], &temp1[i], n, n*n);
				sur_echantillonne(&temp1[i], &temp2[i], n, n*n);
			}
		}
	});

	/* Étape 4. Soustrait la contribution de la version sous-échantillonnée. */
	for (auto i = 0; i < sz; i += 4) {
		bruit[i + 0] -= temp2[i + 0];
		bruit[i + 1] -= temp2[i + 1];
		bruit[i + 2] -= temp2[i + 2];
		bruit[i + 3] -= temp2[i + 3];
	}

	/* Ajout d'une différence de variance paire/impaire par l'ajout d'une
	 * version du bruit décalée par un nombre impair à lui-même. */
	auto offset = n / 2;

	if (offset % 2 == 0){
		offset++;
	}

	auto idx = 0;
	for (auto iz = 0; iz < n; iz++) {
		auto co_z = mod_lent(iz + offset, n) * n * n;

		for (auto iy = 0; iy < n; iy++) {
			auto co_y = mod_lent(iy + offset, n) * n;

			for (auto ix = 0; ix < n; ix++) {
				auto co_x = mod_lent(ix + offset, n);

				temp1[idx++] = bruit[co_x + co_y + co_z];
			}
		}
	}

	for (auto i = 0; i < sz; i += 4) {
		bruit[i + 0] += temp1[i + 0];
		bruit[i + 1] += temp1[i + 1];
		bruit[i + 2] += temp1[i + 2];
		bruit[i + 3] += temp1[i + 3];
	}

	memoire::deloge_tableau("temp1", temp1, sz);
	memoire::deloge_tableau("temp2", temp2, sz);
}

/* Bruit 3D non-projetté. */
static float evalue_bruit(float *tuile, int n, float p[3])
{
	int f[3], c[3], mid[3]; /* f, c = filter, noise coeff indices */
	float w[3][3];

	/* Evaluate quadratic B-spline basis functions */
	for (int i = 0; i < 3; i++) {
		mid[i] = static_cast<int>(std::ceil(p[i] - 0.5f));
		auto const t = static_cast<float>(mid[i]) - (p[i] - 0.5f);

		w[i][0] = t * t / 2.0f;
		w[i][2] = (1 - t) * (1 - t) / 2.0f;
		w[i][1]= 1.0f - w[i][0] - w[i][2];
	}

	/* Evaluate noise by weighting noise coefficients by basis function values */
	auto result = 0.0f;

	for (f[2] = -1; f[2] <= 1; f[2]++) {
		for (f[1] = -1; f[1] <= 1; f[1]++) {
			for (f[0] = -1; f[0] <= 1; f[0]++) {
				float weight = 1.0f;

				for (int i=0; i<3; i++) {
					c[i] = mod_lent(mid[i] + f[i], n);
					weight *= w[i][f[i] + 1];
				}

				result += weight * tuile[c[2]*n*n+c[1]*n+c[0]];
			}
		}
	}

	return result;
}

/* Bruit 3D projetté en 2D. */
static float evalue_projette(float *tuile, int n, float p[3], float normal[3])
{
	int c[3], min[3], max[3]; /* c = noise coeff location */
	auto resultat = 0.0f;

	/* Borne le support des fonctions de bases pour la direction de cette projection. */
	for (auto i = 0; i < 3; i++) {
		auto support = 3.0f * std::abs(normal[i]) + 3.0f * std::sqrt((1.0f - normal[i] * normal[i]) / 2.0f);
		min[i] = static_cast<int>(std::ceil(p[i] - (support)));
		max[i] = static_cast<int>(std::floor(p[i] + (support)));
	}

	/* Boucle sur les coefficients à l'intérieur des bornes. */
	for (c[2] = min[2]; c[2] <= max[2]; c[2]++) {
		for (c[1] = min[1]; c[1] <= max[1]; c[1]++) {
			for (c[0] = min[0]; c[0] <= max[0]; c[0]++) {
				auto dot = 0.0f;
				auto poids = 1.0f;

				/* Produit scalaire du normal avec le vecteur allant de c à p. */
				for (auto i = 0; i < 3; i++) {
					dot += normal[i] * (p[i] - static_cast<float>(c[i]));
				}

				/* Évalue la fonction de base à c déplacé à mi-chemin vers p
				 * sur le normal. */
				for (auto i = 0; i < 3; i++) {
					auto t = (static_cast<float>(c[i]) + normal[i] * dot / 2.0f) - (p[i] - 1.5f);
					auto t1 = t - 1.0f;
					auto t2 = 2.0f - t;
					auto t3 = 3.0f - t;

					poids *= (t <= 0.0f || t >= 3.0f)
							? 0
							: (t < 1.0f)
							  ? t * t / 2.0f
							  : (t < 2.0f )
								? 1.0f - (t1 * t1 + t2 * t2) / 2.0f
								: t3 * t3 / 2.0f;
				}

				/* Évalue le bruit en pondérant les coefficients de bruit par
				 * les valeurs de la fonction de base. */
				resultat += poids * tuile[mod_lent(c[2], n) * n * n + mod_lent(c[1], n) * n + mod_lent(c[0],n)];
			}
		}
	}

	return resultat;
}

static float evalue_multi_bande(
		float *tuile,
		int n,
		float p[3],
		float s,
		float *normal,
		int firstBand,
		int nbands,
		float *w)
{
	float q[3], resultat = 0.0f, variance = 0.0f;

	for (auto b = 0; b < nbands && s + static_cast<float>(firstBand + b) < 0.0f; b++) {
		for (auto i = 0; i <= 2; i++) {
			q[i] = 2.0f * p[i] * std::pow(2.0f, static_cast<float>(firstBand + b));
		}

		resultat += (normal) ? w[b] * evalue_projette(tuile, n, q, normal)
							 : w[b] * evalue_bruit(tuile, n, q);
	}

	for (auto b = 0; b < nbands; b++) {
		variance += w[b] * w[b];
	}

	/* Adjustement du bruit pour que sa variance soit de 1.0f. */
	if (variance != 0.0f) {
		resultat /= std::sqrt(variance * ((normal) ? 0.296f : 0.210f));
	}

	return resultat;
}

/* ************************************************************************** */

/* Le bruit d'ondelette est coûteux à produire donc assurons qu'une seule
 * version existe via cette variable globale.
 *
 * À FAIRE : stockage dans un fichier.
 */

struct constructrice_bruit {
private:
	static constructrice_bruit m_instance;
	std::atomic<float *> m_donnees = nullptr;
	std::mutex m_mutex{};

public:
	static constructrice_bruit &instance()
	{
		return m_instance;
	}

	~constructrice_bruit()
	{
		delete [] m_donnees;
	}

	float *genere_donnees()
	{
		/* Met une barrière sur l'opération de chargement pour que toute
		 * écriture survenant auparavant sur les données nous soit visible.
		 * Ainsi nous évitons toute concurrence critique, verrou mort, ou autre
		 * famine. */
		if (m_donnees.load(std::memory_order_acquire) == nullptr) {
			std::unique_lock<std::mutex> lock(m_mutex);

			/* performe un chargement non-atomic */
			if (m_donnees.load() == nullptr) {
				float *ptr;
				genere_tuile_bruit(ptr, 128);

				/* Relâche la barrière pour garantir que toutes les opérations
				 * de mémoire planifiée avant elle dans l'ordre du programme
				 * deviennent visible avant la prochaine barrière. */
				m_donnees.store(ptr, std::memory_order_release);
			}
		}

		return m_donnees;
	}
};

constructrice_bruit constructrice_bruit::m_instance = constructrice_bruit{};

/* ************************************************************************** */

void ondelette::construit(parametres &params, int graine)
{
	auto &inst_const = constructrice_bruit::instance();
	params.tuile = inst_const.genere_donnees();

	auto gna = GNA{graine};
	auto rand_x = gna.uniforme(0.0f, 1.0f);
	auto rand_y = gna.uniforme(0.0f, 1.0f);
	auto rand_z = gna.uniforme(0.0f, 1.0f);

	params.decalage_graine = normalise(dls::math::vec3f(rand_x, rand_y, rand_z));
}

float ondelette::evalue(parametres const &params, dls::math::vec3f pos)
{
	return evalue_bruit(params.tuile, 128, &pos[0]);
}

}  /* namespace bruit */
