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

/* Nous devons inclure ces fichiers avant editeur_canevas.h à cause de l'ordre
 * d'inclusion des fichiers OpenGL (gl.h doit être inclu après glew.h, etc.). */
#include "opengl/visionneur_scene.h"

#include <GL/glew.h>

#include "editeur_canevas.h"

#include <cassert>
#include "biblinternes/ego/outils.h"
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QMouseEvent>
#include <QScrollArea>
#include <QVBoxLayout>
#pragma GCC diagnostic pop

#include "biblinternes/commandes/commande.h"
#include "biblinternes/commandes/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/poseidon.h"

/* ************************************************************************** */

VueCanevas::VueCanevas(Poseidon *poseidon, QWidget *parent)
	: QGLWidget(parent)
	, m_visionneur_scene(new VisionneurScene(this, poseidon))
	, m_poseidon(poseidon)
{
	setMouseTracking(true);
}

VueCanevas::~VueCanevas()
{
	delete m_visionneur_scene;
}

void VueCanevas::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum erreur = glewInit();

	if (erreur != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(erreur) << "\n";
	}

	m_visionneur_scene->initialise();
}

void VueCanevas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_visionneur_scene->peint_opengl();
}

void VueCanevas::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	m_visionneur_scene->redimensionne(w, h);
}

void VueCanevas::mousePressEvent(QMouseEvent *e)
{
	auto donnees = DonneesCommande();
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());
	donnees.souris = e->button();
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_poseidon->repondant_commande->appele_commande("vue_3d", donnees);
}

void VueCanevas::mouseMoveEvent(QMouseEvent *e)
{
	this->m_visionneur_scene->position_souris(e->pos().x(), e->pos().y());
	this->update();

	if (e->buttons() == 0) {
		return;
	}

	auto donnees = DonneesCommande();
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());
	donnees.souris = e->button();

	m_poseidon->repondant_commande->ajourne_commande_modale(donnees);
}

void VueCanevas::wheelEvent(QWheelEvent *e)
{
	/* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
	 * roulement de la molette de la souris, on prétend que le roulement est un
	 * double clique de la molette. */
	auto donnees = DonneesCommande();
	donnees.x = static_cast<float>(e->delta());
	donnees.souris = Qt::MidButton;
	donnees.double_clique = true;

	m_poseidon->repondant_commande->appele_commande("vue_3d", donnees);
}

void VueCanevas::mouseReleaseEvent(QMouseEvent *e)
{
	DonneesCommande donnees;
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());

	m_poseidon->repondant_commande->acheve_commande_modale(donnees);
}

/* ************************************************************************** */

EditeurCanevas::EditeurCanevas(Poseidon &poseidon, QWidget *parent)
	: BaseEditrice(poseidon, parent)
	, m_vue(new VueCanevas(&poseidon, this))
{
	m_agencement_principal->addWidget(m_vue);
}

void EditeurCanevas::ajourne_etat(int evenement)
{
	m_vue->update();
}

void EditeurCanevas::resizeEvent(QResizeEvent */*event*/)
{
	m_vue->resize(this->size());
}
