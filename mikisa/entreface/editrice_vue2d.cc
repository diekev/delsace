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

#include <GL/glew.h>

#include "editrice_vue2d.h"

#include <cassert>
#include <ego/outils.h>
#include <iostream>

#include <QApplication>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <image/operations/operations.h>
#include <image/pixel.h>

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"
#include "bibliotheques/outils/constantes.h"

#include "coeur/composite.h"
#include "coeur/evenement.h"
#include "coeur/mikisa.h"

#include "opengl/rendu_image.h"
#include "opengl/rendu_manipulatrice_2d.h"

/* ************************************************************************** */

Visionneuse2D::Visionneuse2D(Mikisa *mikisa, EditriceVue2D *base, QWidget *parent)
	: QGLWidget(parent)
	, m_mikisa(mikisa)
	, m_base(base)
{}

Visionneuse2D::~Visionneuse2D()
{
	delete m_rendu_image;
	delete m_rendu_manipulatrice;
}

void Visionneuse2D::initializeGL()
{
	glewExperimental = GL_TRUE;
	GLenum erreur = glewInit();

	if (erreur != GLEW_OK) {
		std::cerr << "Error: " << glewGetErrorString(erreur) << "\n";
		return;
	}

	m_rendu_image = new RenduImage();
	m_rendu_manipulatrice = new RenduManipulatrice2D();
}

void Visionneuse2D::paintGL()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);

	m_contexte.MVP(m_mikisa->camera_2d->matrice);
	m_contexte.matrice_objet(m_matrice_image);

	m_rendu_image->dessine(m_contexte);

	m_contexte.matrice_objet(glm::mat4(1.0));

	/* À FAIRE */
	m_rendu_manipulatrice->dessine(m_contexte);

	glDisable(GL_BLEND);
}

void Visionneuse2D::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);

	m_mikisa->camera_2d->hauteur = h;
	m_mikisa->camera_2d->largeur = w;

	m_mikisa->camera_2d->ajourne_matrice();

	m_matrice_image = glm::mat4(1.0);
	m_matrice_image[0][0] = 1.0;
	m_matrice_image[1][1] = static_cast<float>(720) / 1280;
}

void Visionneuse2D::charge_image(const numero7::math::matrice<numero7::image::Pixel<float>> &image)
{
	if ((image.nombre_colonnes() == 0) || (image.nombre_lignes() == 0)) {
		m_matrice_image = glm::mat4(1.0);
		m_matrice_image[0][0] = 1.0;
		m_matrice_image[1][1] = static_cast<float>(720) / 1280;
		return;
	}

	GLint size[2] = {
		static_cast<GLint>(image.nombre_colonnes()),
		static_cast<GLint>(image.nombre_lignes())
	};

	m_matrice_image = glm::mat4(1.0);
	m_matrice_image[0][0] = 1.0;
	m_matrice_image[1][1] = static_cast<float>(size[1]) / size[0];

	/* À FAIRE : il y a des crashs lors du démarrage, il faudrait réviser la
	 * manière d'initialiser les éditeurs quand ils sont ajoutés */
	if (m_rendu_image != nullptr) {
		m_rendu_image->charge_image(image);
	}
}

void Visionneuse2D::mousePressEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	auto donnees = DonneesCommande();
	donnees.x = (this->size().width() - event->pos().x());
	donnees.y = event->pos().y();
	donnees.souris = event->buttons();
	donnees.modificateur = QApplication::keyboardModifiers();

	m_mikisa->repondant_commande()->appele_commande("vue_2d", donnees);
}

void Visionneuse2D::mouseMoveEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	auto donnees = DonneesCommande();
	donnees.x = (this->size().width() - event->pos().x());
	donnees.y = event->pos().y();
	donnees.souris = event->buttons();
	donnees.modificateur = QApplication::keyboardModifiers();

	if (event->buttons() == 0) {
		m_mikisa->repondant_commande()->appele_commande("vue_2d", donnees);
	}
	else {
		m_mikisa->repondant_commande()->ajourne_commande_modale(donnees);
	}
}

void Visionneuse2D::mouseReleaseEvent(QMouseEvent *event)
{
	m_base->rend_actif();

	auto donnees = DonneesCommande();
	donnees.x = (this->size().width() - event->pos().x());
	donnees.y = event->pos().y();
	donnees.souris = event->buttons();
	donnees.modificateur = QApplication::keyboardModifiers();

	m_mikisa->repondant_commande()->acheve_commande_modale(donnees);
}

void Visionneuse2D::wheelEvent(QWheelEvent *event)
{
	m_base->rend_actif();

	/* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
	 * roulement de la molette de la souris, on prétend que le roulement est un
	 * double clique de la molette. */
	auto donnees = DonneesCommande();
	donnees.x = event->angleDelta().x();
	donnees.y = event->angleDelta().y();
	donnees.souris = Qt::MiddleButton;
	donnees.double_clique = true;
	donnees.modificateur = QApplication::keyboardModifiers();

	m_mikisa->repondant_commande()->appele_commande("vue_2d", donnees);
}

/* ************************************************************************** */

EditriceVue2D::EditriceVue2D(Mikisa *mikisa, QWidget *parent)
	: BaseEditrice(mikisa, parent)
	, m_vue(new Visionneuse2D(mikisa, this))
{
	m_main_layout->addWidget(m_vue);
}

void EditriceVue2D::ajourne_etat(int evenement)
{
	auto chargement = evenement == (type_evenement::image | type_evenement::traite);
	chargement |= (evenement == (type_evenement::rafraichissement));

	if (chargement) {
		const auto &image = m_mikisa->composite->image();
		/* À FAIRE : meilleur façon de sélectionner le calque à visionner. */
		auto tampon = image.calque(image.nom_calque_actif());

		if (tampon == nullptr) {
			/* Charge une image vide, les dimensions sont à peu près celle d'une
			 * image de 1280x720. */
			auto image_vide = type_image(numero7::math::Hauteur(10),
										 numero7::math::Largeur(17));

			auto pixel = numero7::image::Pixel<float>(0.0f);
			pixel.a = 1.0f;

			image_vide.remplie(pixel);

			m_vue->charge_image(image_vide);
		}
		else {
			m_vue->charge_image(tampon->tampon);
		}
	}

	m_vue->update();
}
