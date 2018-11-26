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

#include "corps/corps.h"
#include "corps/maillage.h"

/* ************************************************************************** */

void execute_graphe(
		CompileuseGraphe::iterateur debut,
		CompileuseGraphe::iterateur fin,
		const GestionnaireDonneesGraphe &gestionnaire,
		const dls::math::vec3f &entree,
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
				pile_stocke_vec3f(courant, entree);
				break;
			}
			case NOEUD_POINT3D_MATH:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec_a = pile_charge_vec3f(pointeur);

				decalage = pile_charge_entier(courant);
				pointeur = debut + decalage;
				auto vec_b = pile_charge_vec3f(pointeur);

				auto operation = pile_charge_entier(courant);

				switch (operation) {
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

				pile_stocke_vec3f(courant, vec_a);
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
				auto vec = pile_charge_vec3f(pointeur);
				/* les trois sorties sont l'une après l'autre donc on peut
				 * simplement stocker le vecteur directement */
				pile_stocke_vec3f(courant, vec);
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

				pile_stocke_vec3f(courant, vec);

				break;
			}
			case NOEUD_POINT3D_BRUIT_PROC:
			{
				/* charge position entrée */
				auto pointeur = debut + pile_charge_entier(courant);
				auto pos = pile_charge_vec3f(pointeur);

				/* charge bruit */
				auto dimension = pile_charge_entier(courant);

				dls::math::BruitPerlin3D *bruit_x, *bruit_y, *bruit_z;

				if (dimension == 1) {
					auto index_bruit = pile_charge_entier(courant);
					bruit_x = gestionnaire.bruit(index_bruit);
				}
				else {
					auto index_bruit = pile_charge_entier(courant);
					bruit_x = gestionnaire.bruit(index_bruit);
					index_bruit = pile_charge_entier(courant);
					bruit_y = gestionnaire.bruit(index_bruit);
					index_bruit = pile_charge_entier(courant);
					bruit_z = gestionnaire.bruit(index_bruit);
				}

				auto dur = pile_charge_entier(courant);
				auto frequence = pile_charge_vec3f(courant);
				auto decalage = pile_charge_vec3f(courant);
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

				auto rx = somme_x * ((float)(1 << octaves) / (float)((1 << (octaves + 1)) - 1));
				auto ry = somme_y * ((float)(1 << octaves) / (float)((1 << (octaves + 1)) - 1));
				auto rz = somme_z * ((float)(1 << octaves) / (float)((1 << (octaves + 1)) - 1));

				pile_stocke_vec3f(courant, dls::math::vec3f(rx, ry, rz));

				break;
			}
			case NOEUD_POINT3D_TRAD_VEC:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				auto vec = pile_charge_vec3f(pointeur);

				auto vieux_min = pile_charge_decimal(courant);
				auto vieux_max = pile_charge_decimal(courant);
				auto neuf_min = pile_charge_decimal(courant);
				auto neuf_max = pile_charge_decimal(courant);

				vec.x = dls::math::traduit(vec.x, vieux_min, vieux_max, neuf_min, neuf_max);
				vec.y = dls::math::traduit(vec.y, vieux_min, vieux_max, neuf_min, neuf_max);
				vec.z = dls::math::traduit(vec.z, vieux_min, vieux_max, neuf_min, neuf_max);

				pile_stocke_vec3f(courant, vec);

				break;
			}
			case NOEUD_POINT3D_SORTIE:
			{
				auto decalage = pile_charge_entier(courant);
				auto pointeur = debut + decalage;
				sortie = pile_charge_vec3f(pointeur);
				break;
			}
		}
	}
}

/* ************************************************************************** */

OperatriceGrapheMaillage::OperatriceGrapheMaillage(Noeud *noeud)
	: OperatriceCorps(noeud)
{
	inputs(1);
	outputs(1);
}

const char *OperatriceGrapheMaillage::class_name() const
{
	return NOM_GRAPHE_MAILLAGE;
}

const char *OperatriceGrapheMaillage::help_text() const
{
	return AIDE_GRAPHE_MAILLAGE;
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

int OperatriceGrapheMaillage::execute(const Rectangle &rectangle, const int temps)
{
	if (!this->input(0)->connectee()) {
		ajoute_avertissement("L'entrée n'est pas connectée !");
		return EXECUTION_ECHOUEE;
	}

	m_corps.reinitialise();
	input(0)->requiers_copie_corps(&m_corps, rectangle, temps);

	compile_graphe(temps);

	/* fais une copie locale pour éviter les problèmes de concurrence critique */
	auto pile = m_compileuse.pile();

	for (Point3D *point : m_corps.points()->points()) {
		dls::math::vec3f pos = dls::math::vec3f(point->x, point->y, point->z);

		execute_graphe(pile.begin(), pile.end(), m_gestionnaire, pos, pos);

		point->x = pos.x;
		point->y = pos.y;
		point->z = pos.z;
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
		auto pointeur = noeud.get();

		for (auto &sortie : pointeur->sorties()) {
			sortie->decalage_pile = 0;
		}

		auto operatrice = static_cast<OperatriceImage *>(pointeur->donnees());
		auto operatrice_p3d = dynamic_cast<OperatricePoint3D *>(operatrice);

		if (operatrice_p3d == nullptr) {
			ajoute_avertissement("Impossible de trouver une opératrice point 3D dans le graphe !");
			return;
		}

		operatrice_p3d->compile(m_compileuse, m_gestionnaire, temps);
	}
}

/* ************************************************************************** */

int OperatricePoint3D::execute(const Rectangle &rectangle, const int temps)
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
	m_bruits.push_back(bruit);
	return m_bruits.size() - 1;
}

dls::math::BruitPerlin3D *GestionnaireDonneesGraphe::bruit(size_t index) const
{
	assert(index < m_bruits.size());
	return m_bruits[index];
}
