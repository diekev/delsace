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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "kamikaze_main.h"

#include <systeme_fichier/utilitaires.h>

#include "sdk/mesh.h"
#include "sdk/primitive.h"
#include "sdk/prim_points.h"
#include "sdk/segmentprim.h"

#include <dlfcn.h>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFileDialog>
#include <QMessageBox>
#pragma GCC diagnostic pop

#include "commandes/commandes_graphes.h"
#include "commandes/commandes_objet.h"
#include "commandes/commandes_projet.h"
#include "commandes/commandes_scene.h"
#include "commandes/commandes_vue3d.h"

#include "operatrices/operatrices_physiques.h"
#include "operatrices/operatrices_standards.h"

#include "scene.h"

#include "entreface/fenetre_principale.h"

namespace fs = std::experimental::filesystem;
namespace sf = numero7::systeme_fichier;

static constexpr auto MAX_FICHIER_RECENT = 10;

Main::Main()
    : m_primitive_factory(new PrimitiveFactory)
	, m_usine_operatrice{}
    , m_scene(new Scene)
	, m_usine_commandes{}
	, contexte(m_usine_operatrice)
{}

Main::~Main()
{
}

void Main::fenetre_principale(FenetrePrincipale *fenetre)
{
	m_fenetre_principale = fenetre;
}

void Main::initialize()
{
	/* commandes */
	enregistre_commandes_graphes(this->usine_commandes());
	enregistre_commandes_objet(this->usine_commandes());
	enregistre_commandes_projet(this->usine_commandes());
	enregistre_commandes_scene(this->usine_commandes());
	enregistre_commandes_vue3d(this->usine_commandes());

	/* opérateurs */
	enregistre_operatrices_integres(this->usine_operatrice());
	enregistre_operatrices_physiques(this->usine_operatrice());

	/* primitive types */

	{
		auto factory = this->primitive_factory();

		Mesh::id = REGISTER_PRIMITIVE("Mesh", Mesh);
		PrimPoints::id = REGISTER_PRIMITIVE("PrimPoints", PrimPoints);
		SegmentPrim::id = REGISTER_PRIMITIVE("SegmentPrim", SegmentPrim);
	}
}

PrimitiveFactory *Main::primitive_factory() const
{
	return m_primitive_factory.get();
}

UsineOperatrice &Main::usine_operatrice()
{
	return m_usine_operatrice;
}

Scene *Main::scene() const
{
	return m_scene.get();
}

std::string Main::chemin_projet() const
{
	return m_chemin_projet;
}

void Main::chemin_projet(const std::string &chemin)
{
	m_chemin_projet = chemin;
	ajoute_fichier_recent(chemin);
}

void Main::ajoute_fichier_recent(const std::string &chemin)
{
	auto index = std::find(m_fichiers_recents.begin(), m_fichiers_recents.end(), chemin);

	if (index != m_fichiers_recents.end()) {
		std::rotate(m_fichiers_recents.begin(), index, index + 1);
	}
	else {
		m_fichiers_recents.insert(m_fichiers_recents.begin(), chemin);

		if (m_fichiers_recents.size() > MAX_FICHIER_RECENT) {
			m_fichiers_recents.resize(MAX_FICHIER_RECENT);
		}
	}
}

const std::vector<std::string> &Main::fichiers_recents()
{
	return m_fichiers_recents;
}

bool Main::projet_ouvert() const
{
	return m_projet_ouvert;
}

void Main::projet_ouvert(bool ouinon)
{
	m_projet_ouvert = ouinon;
}

std::string Main::requiers_dialogue(int type)
{
	/* À FAIRE : sort ça de la classe. */
	if (type == FICHIER_OUVERTURE) {
		const auto chemin = QFileDialog::getOpenFileName();
		return chemin.toStdString();
	}

	if (type == FICHIER_SAUVEGARDE) {
		const auto chemin = QFileDialog::getSaveFileName();
		return chemin.toStdString();
	}

	return "";
}

void Main::affiche_erreur(const std::string &message)
{
	/* À FAIRE : sort ça de la classe. */
	QMessageBox boite_message;
	boite_message.critical(nullptr, "Error", message.c_str());
	boite_message.setFixedSize(500, 200);
}


UsineCommande &Main::usine_commandes()
{
	return m_usine_commandes;
}
