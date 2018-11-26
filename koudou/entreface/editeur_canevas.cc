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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
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

#include <image/pixel.h>
#include <image/operations/operations.h>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"

#include "coeur/evenement.h"
#include "coeur/koudou.h"
#include "coeur/moteur_rendu.h"

/* ************************************************************************** */

VueCanevas::VueCanevas(Koudou *koudou, QWidget *parent)
	: QGLWidget(parent)
	, m_koudou(koudou)
	, m_visionneur_image(new VisionneurImage(this))
{}

VueCanevas::~VueCanevas()
{
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
}

void VueCanevas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_visionneur_image->peint_opengl();
}

void VueCanevas::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
}

void VueCanevas::mousePressEvent(QMouseEvent *e)
{
	auto donnees = DonneesCommande();
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();
	donnees.souris = e->buttons();
	donnees.modificateur = QApplication::keyboardModifiers();

	m_koudou->repondant_commande->appele_commande("vue_2d", donnees);
}

void VueCanevas::mouseMoveEvent(QMouseEvent *e)
{
	if (e->buttons() == 0) {
		return;
	}

	auto donnees = DonneesCommande();
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();
	donnees.souris = e->buttons();

	m_koudou->repondant_commande->ajourne_commande_modale(donnees);
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

	m_koudou->repondant_commande->appele_commande("vue_2d", donnees);
}

void VueCanevas::mouseReleaseEvent(QMouseEvent *e)
{
	DonneesCommande donnees;
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();

	m_koudou->repondant_commande->acheve_commande_modale(donnees);
}

void VueCanevas::charge_image(const numero7::math::matrice<dls::math::vec3d> &image)
{
	m_visionneur_image->charge_image(image);
}

/* ************************************************************************** */

EditeurCanevas::EditeurCanevas(Koudou &koudou, QWidget *parent)
	: BaseEditrice(koudou, parent)
	, m_vue(new VueCanevas(&koudou, this))
	, m_zone_defilement(new QScrollArea(this))
{
	m_zone_defilement->setWidget(m_vue);
	m_zone_defilement->setAlignment(Qt::AlignCenter);
	m_zone_defilement->setWidgetResizable(false);

	m_agencement_principal->addWidget(m_zone_defilement);
}

void EditeurCanevas::ajourne_etat(int event)
{
	if (event == (type_evenement::rendu | type_evenement::fini)) {
		m_vue->charge_image(m_koudou->moteur_rendu->pellicule());
	}

	m_vue->update();
}

EditeurCanevas::~EditeurCanevas()
{
	delete m_zone_defilement;
}

/* ************************************************************************** */

VueCanevas3D::VueCanevas3D(Koudou *koudou, QWidget *parent)
	: QGLWidget(parent)
	, m_koudou(koudou)
	, m_visionneur_scene(new VisionneurScene(this, koudou))
{}

VueCanevas3D::~VueCanevas3D()
{
	delete m_visionneur_scene;
}

void VueCanevas3D::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum err = glewInit();

	if (err != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(err) << "\n";
	}

	m_visionneur_scene->initialise();
}

void VueCanevas3D::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_visionneur_scene->peint_opengl();
}

void VueCanevas3D::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	m_visionneur_scene->redimensionne(w, h);
}

void VueCanevas3D::mousePressEvent(QMouseEvent *e)
{
	auto donnees = DonneesCommande();
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();
	donnees.souris = e->buttons();
	donnees.modificateur = QApplication::keyboardModifiers();

	m_koudou->repondant_commande->appele_commande("vue_3d", donnees);
}

void VueCanevas3D::mouseMoveEvent(QMouseEvent *e)
{
	if (e->buttons() == 0) {
		return;
	}

	auto donnees = DonneesCommande();
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();
	donnees.souris = e->buttons();

	m_koudou->repondant_commande->ajourne_commande_modale(donnees);
}

void VueCanevas3D::wheelEvent(QWheelEvent *e)
{
	/* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
	 * roulement de la molette de la souris, on prétend que le roulement est un
	 * double clique de la molette. */
	auto donnees = DonneesCommande();
	donnees.x = e->delta();
	donnees.souris = Qt::MidButton;
	donnees.double_clique = true;

	m_koudou->repondant_commande->appele_commande("vue_3d", donnees);
}

void VueCanevas3D::mouseReleaseEvent(QMouseEvent *e)
{
	DonneesCommande donnees;
	donnees.x = e->pos().x();
	donnees.y = e->pos().y();

	m_koudou->repondant_commande->acheve_commande_modale(donnees);
}

void VueCanevas3D::reconstruit_scene() const
{
	m_visionneur_scene->reconstruit_scene();
}

/* ************************************************************************** */

EditriceVue3D::EditriceVue3D(Koudou &koudou, QWidget *parent)
	: BaseEditrice(koudou, parent)
	, m_vue(new VueCanevas3D(&koudou, this))
{
	m_agencement_principal->addWidget(m_vue);
}

void EditriceVue3D::ajourne_etat(int evenement)
{
	if (evenement == (type_evenement::objet | type_evenement::ajoute)) {
		m_vue->reconstruit_scene();
	}

	m_vue->update();
}

void EditriceVue3D::resizeEvent(QResizeEvent *event)
{
	m_vue->resize(this->size());
}
