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

#include "operatrice_objet.h"

#include <tbb/tick_count.h>

#include "bibliotheques/objets/creation.h"
#include "bibliotheques/outils/constantes.h"
#include "bibliotheques/texture/texture.h"

#include "contexte_evaluation.hh"

OperatriceObjet::OperatriceObjet(Graphe &graphe_parent, Noeud *noeud)
	: OperatriceImage(graphe_parent, noeud)
{
	entrees(0);
	sorties(0);
}

int OperatriceObjet::type() const
{
	return OPERATRICE_OBJET;
}

const char *OperatriceObjet::chemin_entreface() const
{
	return "entreface/operatrice_objet.jo";
}

const char *OperatriceObjet::nom_classe() const
{
	return NOM;
}

const char *OperatriceObjet::texte_aide() const
{
	return AIDE;
}

Objet *OperatriceObjet::objet()
{
	return &m_objet;
}

vision::Camera3D *OperatriceObjet::camera()
{
//	if (m_maillage.texture() && m_maillage.texture()->camera()) {
//		return m_maillage.texture()->camera();
//	}

	return nullptr;
}

Graphe *OperatriceObjet::graphe()
{
	return &m_graphe;
}

bool OperatriceObjet::possede_manipulatrice_3d(int type) const
{
	return type == MANIPULATION_POSITION
			|| type == MANIPULATION_ECHELLE
			|| type == MANIPULATION_ROTATION;
}

Manipulatrice3D *OperatriceObjet::manipulatrice_3d(int type)
{
	if (type == MANIPULATION_POSITION) {
		return &m_manipulatrice_position;
	}

	if (type == MANIPULATION_ECHELLE) {
		return &m_manipulatrice_echelle;
	}

	if (type == MANIPULATION_ROTATION) {
		return &m_manipulatrice_rotation;
	}

	return nullptr;
}

void OperatriceObjet::ajourne_selon_manipulatrice_3d(int type, const int temps)
{
	dls::math::point3f position, rotation, taille;

	if (type == MANIPULATION_POSITION) {
		position = m_manipulatrice_position.pos();
		rotation = dls::math::point3f(evalue_vecteur("rotation", temps)) * constantes<float>::POIDS_DEG_RAD;
		taille = dls::math::point3f(evalue_vecteur("taille", temps));

		valeur_vecteur("position", dls::math::vec3f(position));
	}
	else if (type == MANIPULATION_ECHELLE) {
		position = dls::math::point3f(evalue_vecteur("position", temps));
		rotation = dls::math::point3f(evalue_vecteur("rotation", temps)) * constantes<float>::POIDS_DEG_RAD;
		taille = m_manipulatrice_echelle.taille();

		valeur_vecteur("taille", dls::math::vec3f(taille));
	}
	else if (type == MANIPULATION_ROTATION) {
		position = dls::math::point3f(evalue_vecteur("position", temps));
		rotation = m_manipulatrice_rotation.rotation();
		taille = dls::math::point3f(evalue_vecteur("taille", temps));

		valeur_vecteur("rotation", dls::math::vec3f(rotation) * constantes<float>::POIDS_RAD_DEG);
	}
	else {
		return;
	}

	auto transformation = math::transformation();
	transformation *= math::translation(position.x, position.y, position.z);
	transformation *= math::rotation_x(rotation.x);
	transformation *= math::rotation_y(rotation.y);
	transformation *= math::rotation_z(rotation.z);
	transformation *= math::echelle(taille.x, taille.y, taille.z);

	m_objet.transformation = transformation;

	m_manipulatrice_position.pos(position);
	m_manipulatrice_rotation.pos(position);
	m_manipulatrice_echelle.pos(position);
}

int OperatriceObjet::execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval)
{
	/* transformation */

	auto position = dls::math::point3f(evalue_vecteur("position", contexte.temps_courant));
	auto rotation = evalue_vecteur("rotation", contexte.temps_courant);
	auto taille = evalue_vecteur("taille", contexte.temps_courant);

	auto transformation = math::transformation();
	transformation *= math::translation(position.x, position.y, position.z);
	transformation *= math::rotation_x(rotation.x * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::rotation_y(rotation.y * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::rotation_z(rotation.z * constantes<float>::POIDS_DEG_RAD);
	transformation *= math::echelle(taille.x, taille.y, taille.z);

	m_objet.transformation = transformation;

	m_manipulatrice_position.pos(position);
	m_manipulatrice_rotation.pos(position);
	m_manipulatrice_echelle.pos(position);

	/* évaluation graphe */

	Noeud *noeud_sortie = m_graphe.dernier_noeud_sortie;

	if (noeud_sortie == nullptr) {
		this->ajoute_avertissement("Aucun noeud de sortie trouvé dans le graphe !");
		return EXECUTION_ECHOUEE;
	}

	auto operatrice = std::any_cast<OperatriceImage *>(noeud_sortie->donnees());

	auto const t0 = tbb::tick_count::now();

	operatrice->reinitialise_avertisements();
	operatrice->execute(contexte, donnees_aval);

	auto const t1 = tbb::tick_count::now();
	auto const delta = (t1 - t0).seconds();
	noeud_sortie->temps_execution(static_cast<float>(delta));

	/* À FAIRE? :- on garde une copie pour l'évaluation dans des threads
	 * séparés, copie nécessaire pour pouvoir rendre l'objet dans la vue quand
	 * le rendu prend plus de temps que l'évaluation asynchrone. */
	m_objet.mutex_corps.lock();
	m_objet.corps.reinitialise();
	operatrice->corps()->copie_vers(&m_objet.corps);
	m_objet.mutex_corps.unlock();

	return EXECUTION_REUSSIE;
}
