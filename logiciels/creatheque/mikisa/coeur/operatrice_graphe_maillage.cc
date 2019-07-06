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

#include "operatrice_graphe_maillage.h"

#include <delsace/math/bruit.hh>
#include <delsace/math/entrepolation.hh>

#include "corps/corps.h"

#include "contexte_evaluation.hh"

/* ************************************************************************** */

void execute_graphe(
		CompileuseGraphe::iterateur debut,
		CompileuseGraphe::iterateur fin,
		GestionnaireDonneesGraphe const &gestionnaire,
		dls::math::vec3f const &entree,
		dls::math::vec3f &sortie)
{
	auto courant = debut;

	while (courant != fin) {
		auto operation = pile_charge_entier(courant);

		if (operation == -1) {
			break;
		}

		switch (operation) {
			case NOEUD_POINT3D_ENTREE:
			{
				pile_stocke_vec3(courant, entree);
				break;
			}
			case NOEUD_POINT3D_MATH:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec_a = pile_charge_vec3(pointeur);

				decalage = pile_charge_entier(courant);
				pointeur = debut + decalage;
				auto vec_b = pile_charge_vec3(pointeur);

				auto operation_math = pile_charge_entier(courant);

				switch (operation_math) {
					case OPERATION_MATH_ADDITION:
						vec_a += vec_b;
						break;
					case OPERATION_MATH_SOUSTRACTION:
						vec_a -= vec_b;
						break;
					case OPERATION_MATH_MULTIPLICATION:
						vec_a *= vec_b;
						break;
					case OPERATION_MATH_DIVISION:
						vec_a /= vec_b;
						break;
					default:
						break;
				}

				pile_stocke_vec3(courant, vec_a);
				break;
			}
			case NOEUD_POINT3D_VALEUR:
			{
				/* la valeur doit déjà être chargée */
				courant += 1;
				break;
			}
			case NOEUD_POINT3D_VECTEUR:
			{
				/* le vecteur doit déjà être chargé */
				courant += 3;
				break;
			}
			case NOEUD_POINT3D_SEPARE_VECTEUR:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec = pile_charge_vec3(pointeur);
				/* les trois sorties sont l'une après l'autre donc on peut
				 * simplement stocker le vecteur directement */
				pile_stocke_vec3(courant, vec);
				break;
			}
			case NOEUD_POINT3D_COMBINE_VECTEUR:
			{
				dls::math::vec3f vec;

				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				vec.x = pile_charge_decimal(pointeur);

				decalage = pile_charge_entier(courant);
				pointeur = debut + decalage;
				vec.y = pile_charge_decimal(pointeur);

				decalage = pile_charge_entier(courant);
				pointeur = debut + decalage;
				vec.z = pile_charge_decimal(pointeur);

				pile_stocke_vec3(courant, vec);

				break;
			}
			case NOEUD_POINT3D_BRUIT_PROC:
			{
				/* charge position entrée */
				auto pointeur = debut + pile_charge_entier(courant);
				auto pos = pile_charge_vec3(pointeur);

				/* charge bruit */
				auto dimension = pile_charge_entier(courant);

				dls::math::BruitPerlin3D *bruit_x, *bruit_y, *bruit_z;

				if (dimension == 1) {
					auto index_bruit = static_cast<size_t>(pile_charge_entier(courant));
					bruit_x = gestionnaire.bruit(index_bruit);
				}
				else {
					auto index_bruit = static_cast<size_t>(pile_charge_entier(courant));
					bruit_x = gestionnaire.bruit(index_bruit);
					index_bruit = static_cast<size_t>(pile_charge_entier(courant));
					bruit_y = gestionnaire.bruit(index_bruit);
					index_bruit = static_cast<size_t>(pile_charge_entier(courant));
					bruit_z = gestionnaire.bruit(index_bruit);
				}

				auto dur = pile_charge_entier(courant);
				auto frequence = pile_charge_vec3(courant);
				auto decalage = pile_charge_vec3(courant);
				auto octaves = pile_charge_entier(courant);
				auto amplitude = pile_charge_decimal(courant);
				auto persistence = pile_charge_decimal(courant);
				auto lacunarite = pile_charge_decimal(courant);

				auto somme_x = 0.0f;
				auto somme_y = 0.0f;
				auto somme_z = 0.0f;

				if (dimension == 1) {
					auto freq = frequence.x;
					auto ampl = amplitude;

					for (int i = 0; i <= octaves; i++, ampl *= persistence) {
						auto t = 0.5f + 0.5f * (*bruit_x)(freq * pos.x + decalage.x,
														  freq * pos.y + decalage.y,
														  freq * pos.z + decalage.z);

						if (dur) {
							t = std::fabs(2.0f * t - 1.0f);
						}

						somme_x += t * ampl;
						somme_y += t * ampl;
						somme_z += t * ampl;
						freq *= lacunarite;
					}
				}
				else {
					auto freq_x = frequence.x;
					auto freq_y = frequence.y;
					auto freq_z = frequence.z;
					auto ampl = amplitude;

					for (int i = 0; i <= octaves; i++, ampl *= persistence) {
						auto tx = 0.5f + 0.5f * (*bruit_x)(freq_x * pos.x + decalage.x,
														   freq_y * pos.y + decalage.y,
														   freq_z * pos.z + decalage.z);
						auto ty = 0.5f + 0.5f * (*bruit_y)(freq_x * pos.x + decalage.x,
														   freq_y * pos.y + decalage.y,
														   freq_z * pos.z + decalage.z);
						auto tz = 0.5f + 0.5f * (*bruit_z)(freq_x * pos.x + decalage.x,
														   freq_y * pos.y + decalage.y,
														   freq_z * pos.z + decalage.z);

						if (dur) {
							tx = std::fabs(2.0f * tx - 1.0f);
							ty = std::fabs(2.0f * tx - 1.0f);
							tz = std::fabs(2.0f * tx - 1.0f);
						}

						somme_x += tx * ampl;
						somme_y += ty * ampl;
						somme_z += tz * ampl;
						freq_x *= lacunarite;
						freq_y *= lacunarite;
						freq_z *= lacunarite;
					}
				}

				auto rx = somme_x * (static_cast<float>(1 << octaves) / static_cast<float>((1 << (octaves + 1)) - 1));
				auto ry = somme_y * (static_cast<float>(1 << octaves) / static_cast<float>((1 << (octaves + 1)) - 1));
				auto rz = somme_z * (static_cast<float>(1 << octaves) / static_cast<float>((1 << (octaves + 1)) - 1));

				pile_stocke_vec3(courant, dls::math::vec3f(rx, ry, rz));

				break;
			}
			case NOEUD_POINT3D_TRAD_VEC:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec = pile_charge_vec3(pointeur);

				auto vieux_min = pile_charge_decimal(courant);
				auto vieux_max = pile_charge_decimal(courant);
				auto neuf_min = pile_charge_decimal(courant);
				auto neuf_max = pile_charge_decimal(courant);

				vec.x = dls::math::traduit(vec.x, vieux_min, vieux_max, neuf_min, neuf_max);
				vec.y = dls::math::traduit(vec.y, vieux_min, vieux_max, neuf_min, neuf_max);
				vec.z = dls::math::traduit(vec.z, vieux_min, vieux_max, neuf_min, neuf_max);

				pile_stocke_vec3(courant, vec);

				break;
			}
			case NOEUD_POINT3D_NORMALISE:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec = pile_charge_vec3(pointeur);

				vec = normalise(vec);

				pile_stocke_vec3(courant, vec);

				break;
			}
			case NOEUD_POINT3D_COMPLEMENT:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto val = 1.0f - pile_charge_decimal(pointeur);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_EP_FLUIDE_O1:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto val = pile_charge_decimal(pointeur);
				val = dls::math::entrepolation_fluide<1>(val);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_EP_FLUIDE_O2:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto val = pile_charge_decimal(pointeur);
				val = dls::math::entrepolation_fluide<2>(val);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_EP_FLUIDE_O3:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto val = pile_charge_decimal(pointeur);
				val = dls::math::entrepolation_fluide<3>(val);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_EP_FLUIDE_O4:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto val = pile_charge_decimal(pointeur);
				val = dls::math::entrepolation_fluide<4>(val);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_EP_FLUIDE_O5:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto val = pile_charge_decimal(pointeur);
				val = dls::math::entrepolation_fluide<5>(val);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_EP_FLUIDE_O6:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto val = pile_charge_decimal(pointeur);
				val = dls::math::entrepolation_fluide<6>(val);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_PRODUIT_SCALAIRE:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec1 = pile_charge_vec3(pointeur);

				decalage = pile_charge_entier(courant);
				pointeur = debut + decalage;
				auto vec2 = pile_charge_vec3(pointeur);

				auto val = dls::math::produit_scalaire(vec1, vec2);
				pile_stocke_decimal(courant, val);
				break;
			}
			case NOEUD_POINT3D_PRODUIT_CROIX:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec1 = pile_charge_vec3(pointeur);

				decalage = pile_charge_entier(courant);
				pointeur = debut + decalage;
				auto vec2 = pile_charge_vec3(pointeur);

				auto val = dls::math::produit_croix(vec1, vec2);
				pile_stocke_vec3(courant, val);
				break;
			}
			case NOEUD_POINT3D_SORTIE:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				sortie = pile_charge_vec3(pointeur);
				break;
			}
		}
	}
}

/* ************************************************************************** */

OperatriceGrapheMaillage::OperatriceGrapheMaillage(Graphe &graphe_parent, Noeud *noeud)
	: OperatriceCorps(graphe_parent, noeud)
	, m_graphe(cree_noeud_image, supprime_noeud_image)
{
	entrees(1);
	sorties(1);
}

const char *OperatriceGrapheMaillage::nom_classe() const
{
	return NOM;
}

const char *OperatriceGrapheMaillage::texte_aide() const
{
	return AIDE;
}

const char *OperatriceGrapheMaillage::chemin_entreface() const
{
	return "";
}

int OperatriceGrapheMaillage::type_entree(int) const
{
	return OPERATRICE_CORPS;
}

int OperatriceGrapheMaillage::type_sortie(int) const
{
	return OPERATRICE_CORPS;
}

Graphe *OperatriceGrapheMaillage::graphe()
{
	return &m_graphe;
}

int OperatriceGrapheMaillage::type() const
{
	return OPERATRICE_GRAPHE_MAILLAGE;
}

int OperatriceGrapheMaillage::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	if (!this->entree(0)->connectee()) {
		ajoute_avertissement("L'entrée n'est pas connectée !");
		return EXECUTION_ECHOUEE;
	}

	m_corps.reinitialise();
	entree(0)->requiers_copie_corps(&m_corps, contexte, donnees_aval);

	compile_graphe(contexte.temps_courant);

	/* fais une copie locale pour éviter les problèmes de concurrence critique */
	auto pile = m_compileuse.pile();

	for (auto i = 0; i < m_corps.points()->taille(); ++i) {
		auto pos = m_corps.points()->point(i);

		execute_graphe(pile.begin(), pile.end(), m_gestionnaire, pos, pos);

		m_corps.points()->point(i, pos);
	}

	return EXECUTION_REUSSIE;
}

void OperatriceGrapheMaillage::compile_graphe(int temps)
{
	m_compileuse = CompileuseGraphe();
	m_gestionnaire.reinitialise();

	if (m_graphe.besoin_ajournement) {
		tri_topologique(m_graphe);
		m_graphe.besoin_ajournement = false;
	}

	for (auto &noeud : m_graphe.noeuds()) {
		for (auto &sortie : noeud->sorties()) {
			sortie->decalage_pile = 0;
		}

		auto operatrice = std::any_cast<OperatriceImage *>(noeud->donnees());
		auto operatrice_p3d = dynamic_cast<OperatricePoint3D *>(operatrice);

		if (operatrice_p3d == nullptr) {
			ajoute_avertissement("Impossible de trouver une opératrice point 3D dans le graphe !");
			return;
		}

		operatrice_p3d->compile(m_compileuse, m_gestionnaire, temps);
	}
}

/* ************************************************************************** */

int OperatricePoint3D::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	return EXECUTION_REUSSIE;
}

/* ************************************************************************** */

void GestionnaireDonneesGraphe::reinitialise()
{
	m_bruits.clear();
}

size_t GestionnaireDonneesGraphe::ajoute_bruit(dls::math::BruitPerlin3D *bruit)
{
	m_bruits.pousse(bruit);
	return static_cast<size_t>(m_bruits.taille() - 1);
}

dls::math::BruitPerlin3D *GestionnaireDonneesGraphe::bruit(size_t index) const
{
	assert(static_cast<long>(index) < m_bruits.taille());
	return m_bruits[static_cast<long>(index)];
}
