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

#include "operatrice_scene.h"

#include "bibliotheques/vision/camera.h"

#include "rendu/moteur_rendu.hh"

#include "corps/corps.h"

#include "contexte_evaluation.hh"
#include "objet.h"

template <typename T>
static auto converti_matrice_glm(dls::math::mat4x4<T> const &matrice)
{
	dls::math::mat4x4f resultat;

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			resultat[i][j] = static_cast<float>(matrice[i][j]);
		}
	}

	return resultat;
}

/* ************************************************************************** */

OperatriceScene::OperatriceScene(Graphe &graphe_parent, Noeud *node)
	: OperatriceImage(graphe_parent, node)
	, m_graphe(cree_noeud_image, supprime_noeud_image)
{
	entrees(1);
	sorties(1);
}

int OperatriceScene::type() const
{
	return OPERATRICE_SCENE;
}

int OperatriceScene::type_entree(int n) const
{
	return OPERATRICE_CAMERA;
}

int OperatriceScene::type_sortie(int) const
{
	return OPERATRICE_IMAGE;
}

const char *OperatriceScene::chemin_entreface() const
{
	return "";
}

const char *OperatriceScene::nom_classe() const
{
	return NOM;
}

const char *OperatriceScene::texte_aide() const
{
	return AIDE;
}

Scene *OperatriceScene::scene()
{
	return &m_scene;
}

Graphe *OperatriceScene::graphe()
{
	return &m_graphe;
}

int OperatriceScene::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	auto camera = entree(0)->requiers_camera(contexte, donnees_aval);

	if (camera == nullptr) {
		ajoute_avertissement("Aucune caméra trouvée !");
		return EXECUTION_ECHOUEE;
	}

	m_scene.camera(camera);

	auto const &rectangle = contexte.resolution_rendu;

	m_image.reinitialise();
	auto tampon = m_image.ajoute_calque("image", rectangle);

	if (m_moteur_rendu == nullptr) {
		m_moteur_rendu = memoire::loge<MoteurRendu>();
	}

	m_moteur_rendu->camera(camera);
	m_moteur_rendu->scene(&m_scene);

	m_moteur_rendu->calcule_rendu(
				&tampon->tampon[0][0][0],
				static_cast<int>(rectangle.hauteur),
				static_cast<int>(rectangle.largeur),
				true);

	return EXECUTION_REUSSIE;
}
