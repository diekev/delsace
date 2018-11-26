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
#include "opengl/visionneur_image.h"
#include "opengl/visionneur_scene.h"

#include "editeur_canevas.h"

#include <cassert>
#include <ego/outils.h>
#include <iostream>

#include <QApplication>
#include <QComboBox>
#include <QMouseEvent>
#include <QScrollArea>
#include <QVBoxLayout>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/kanba.h"

/* ************************************************************************** */

VueCanevas::VueCanevas(Kanba *kanba, QWidget *parent)
	: QGLWidget(parent)
	, m_visionneur_image(new VisionneurImage(this, kanba))
	, m_visionneur_scene(new VisionneurScene(this, kanba))
	, m_kanba(kanba)
{
	setMouseTracking(true);
}

VueCanevas::~VueCanevas()
{
	delete m_visionneur_scene;
	delete m_visionneur_image;
}

void VueCanevas::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(err) << "\n";
	}

	m_visionneur_image->initialise();
	m_visionneur_scene->initialise();
}

void VueCanevas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (m_mode_visionnage == VISIONNAGE_IMAGE) {
		m_visionneur_image->peint_opengl();
	}
	else {
		m_visionneur_scene->peint_opengl();
	}
}

void VueCanevas::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);

	if (m_mode_visionnage == VISIONNAGE_SCENE) {
		m_visionneur_scene->redimensionne(w, h);
	}
	else {
		m_visionneur_image->redimensionne(w, h);
	}
}

void VueCanevas::mousePressEvent(QMouseEvent *e)
{
	auto donnees = DonneesCommande();
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();
	donnees.souris = e->button();
	donnees.modificateur = QApplication::keyboardModifiers();

	if (m_mode_visionnage == VISIONNAGE_SCENE) {
		m_kanba->repondant_commande->appele_commande("vue_3d", donnees);
	}
	else {
		m_kanba->repondant_commande->appele_commande("vue_2d", donnees);
	}
}

void VueCanevas::mouseMoveEvent(QMouseEvent *e)
{
	this->m_visionneur_scene->position_souris(e->pos().x(), e->pos().y());
	this->update();

	if (e->buttons() == 0) {
		return;
	}

	auto donnees = DonneesCommande();
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();
	donnees.souris = e->button();

	m_kanba->repondant_commande->ajourne_commande_modale(donnees);
}

void VueCanevas::wheelEvent(QWheelEvent *e)
{
	/* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
	 * roulement de la molette de la souris, on prétend que le roulement est un
	 * double clique de la molette. */
	auto donnees = DonneesCommande();
	donnees.x = e->delta();
	donnees.souris = Qt::MidButton;
	donnees.double_clique = true;

	m_kanba->repondant_commande->appele_commande("vue_3d", donnees);
}

void VueCanevas::mouseReleaseEvent(QMouseEvent *e)
{
	DonneesCommande donnees;
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();

	m_kanba->repondant_commande->acheve_commande_modale(donnees);
}

void VueCanevas::charge_image(const numero7::math::matrice<dls::math::vec4f> &image)
{
	m_visionneur_image->charge_image(image);
}

void VueCanevas::mode_visionnage(int mode)
{
	m_mode_visionnage = mode;
}

int VueCanevas::mode_visionnage() const
{
	return m_mode_visionnage;
}

/* ************************************************************************** */

EditeurCanevas::EditeurCanevas(Kanba &kanba, QWidget *parent)
	: BaseEditrice(kanba, parent)
	, m_vue(new VueCanevas(&kanba, this))
{
	auto choix_visionneur = new QComboBox();
	choix_visionneur->addItem("Vue2D", QVariant(VISIONNAGE_IMAGE));
	choix_visionneur->addItem("Vue3D", QVariant(VISIONNAGE_SCENE));
	connect(choix_visionneur, SIGNAL(currentIndexChanged(int)), this, SLOT(change_mode_visionnage(int)));

	auto agencement_vertical = new QVBoxLayout();
	agencement_vertical->addWidget(m_vue);
	agencement_vertical->addWidget(choix_visionneur);

	m_agencement_principal->addLayout(agencement_vertical);
}

void EditeurCanevas::ajourne_etat(int evenement)
{
	if (evenement == (type_evenement::dessin | type_evenement::fini)) {
		if (m_vue->mode_visionnage() == VISIONNAGE_IMAGE) {
			m_vue->charge_image(m_kanba->tampon);
		}
	}

	m_vue->update();
}

void EditeurCanevas::change_mode_visionnage(int mode)
{
	m_vue->mode_visionnage(mode);

	if (mode == VISIONNAGE_SCENE) {
		m_vue->resize(this->size());
	}
	else {
		m_vue->charge_image(m_kanba->tampon);
	}
}

void EditeurCanevas::resizeEvent(QResizeEvent */*event*/)
{
	if (m_vue->mode_visionnage() == VISIONNAGE_SCENE) {
		m_vue->resize(this->size());
	}
}
