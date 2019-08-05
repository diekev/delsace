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

#include "GL/glew.h"

#include "editrice_vue3d.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QToolButton>
#pragma GCC diagnostic pop

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/graphe/graphe.h"

#include "coeur/evenement.h"
#include "coeur/manipulatrice.h"
#include "coeur/mikisa.h"
#include "coeur/operatrice_image.h"

#include "opengl/visionneur_scene.h"

/* ************************************************************************** */

static void charge_manipulatrice(Mikisa &mikisa, int type_manipulation)
{
	mikisa.type_manipulation_3d = type_manipulation;

	if (mikisa.contexte != GRAPHE_COMPOSITE) {
		return;
	}

	auto graphe = mikisa.graphe;
	auto noeud = graphe->noeud_actif;

	if (noeud == nullptr) {
		return;
	}

	auto operatrice = extrait_opimage(noeud->donnees());

	if (!operatrice->possede_manipulatrice_3d(mikisa.type_manipulation_3d)) {
		mikisa.manipulatrice_3d = nullptr;
		return;
	}

	mikisa.manipulatrice_3d = operatrice->manipulatrice_3d(mikisa.type_manipulation_3d);
}

/* ************************************************************************** */

VueCanevas3D::VueCanevas3D(Mikisa &mikisa, EditriceVue3D *base, QWidget *parent)
	: QGLWidget(parent)
	, m_mikisa(mikisa)
	, m_visionneur_scene(new VisionneurScene(this, mikisa))
	, m_base(base)
{
	setMouseTracking(true);
}

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
	m_visionneur_scene->peint_opengl();
}

void VueCanevas3D::resizeGL(int w, int h)
{
	glViewport(0, 0, w, h);
	m_visionneur_scene->redimensionne(w, h);
}

void VueCanevas3D::mousePressEvent(QMouseEvent *e)
{
	m_base->rend_actif();

	auto donnees = DonneesCommande();
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());
	donnees.souris = static_cast<int>(e->buttons());
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

	m_mikisa.repondant_commande()->appele_commande("vue_3d", donnees);
}

void VueCanevas3D::mouseMoveEvent(QMouseEvent *e)
{
	auto donnees = DonneesCommande();
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());
	donnees.souris = static_cast<int>(e->buttons());

	if (e->buttons() == 0) {
		m_mikisa.repondant_commande()->appele_commande("vue_3d", donnees);
	}
	else {
		m_mikisa.repondant_commande()->ajourne_commande_modale(donnees);
	}
}

void VueCanevas3D::wheelEvent(QWheelEvent *e)
{
	m_base->rend_actif();

	/* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
	 * roulement de la molette de la souris, on prétend que le roulement est un
	 * double clique de la molette. */
	auto donnees = DonneesCommande();
	donnees.x = static_cast<float>(e->angleDelta().x());
	donnees.y = static_cast<float>(e->angleDelta().y());
	donnees.souris = Qt::MiddleButton;
	donnees.double_clique = true;

	m_mikisa.repondant_commande()->appele_commande("vue_3d", donnees);
}

void VueCanevas3D::mouseReleaseEvent(QMouseEvent *e)
{
	m_base->rend_actif();

	DonneesCommande donnees;
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());

	m_mikisa.repondant_commande()->acheve_commande_modale(donnees);
}

void VueCanevas3D::reconstruit_scene() const
{
	//m_visionneur_scene->reconstruit_scene();
}

void VueCanevas3D::change_moteur_rendu(dls::chaine const &id) const
{
	m_visionneur_scene->change_moteur_rendu(id);
}

/* ************************************************************************** */

EditriceVue3D::EditriceVue3D(Mikisa &mikisa, QWidget *parent)
	: BaseEditrice(mikisa, parent)
	, m_vue(new VueCanevas3D(mikisa, this, this))
{
	auto disp_widgets = new QVBoxLayout();
	auto disp_boutons = new QHBoxLayout();
	disp_boutons->addStretch();

	QIcon icone_manipulation;
	icone_manipulation.addPixmap(QPixmap("icones/icone_manipulation.png"), QIcon::Mode::Normal, QIcon::State::Off);
	icone_manipulation.addPixmap(QPixmap("icones/icone_manipulation_active.png"), QIcon::Mode::Normal, QIcon::State::On);

	auto bouton = new QToolButton();
	bouton->setIcon(icone_manipulation);
	bouton->setFixedHeight(32);
	bouton->setFixedWidth(32);
	bouton->setCheckable(true);
	bouton->setChecked(false);
	connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::bascule_manipulation);
	disp_boutons->addWidget(bouton);

	QIcon icone_position;
	icone_position.addPixmap(QPixmap("icones/icone_position.png"), QIcon::Mode::Normal, QIcon::State::Off);
	icone_position.addPixmap(QPixmap("icones/icone_position_active.png"), QIcon::Mode::Normal, QIcon::State::On);

	bouton = new QToolButton();
	bouton->setIcon(icone_position);
	bouton->setFixedHeight(32);
	bouton->setFixedWidth(32);
	bouton->setCheckable(true);
	bouton->setChecked(true);
	connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::manipule_position);
	disp_boutons->addWidget(bouton);
	m_bouton_position = bouton;
	m_bouton_actif = bouton;

	QIcon icone_rotation;
	icone_rotation.addPixmap(QPixmap("icones/icone_rotation.png"), QIcon::Mode::Normal, QIcon::State::Off);
	icone_rotation.addPixmap(QPixmap("icones/icone_rotation_active.png"), QIcon::Mode::Normal, QIcon::State::On);

	bouton = new QToolButton();
	bouton->setIcon(icone_rotation);
	bouton->setCheckable(true);
	bouton->setFixedHeight(32);
	bouton->setFixedWidth(32);
	connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::manipule_rotation);
	disp_boutons->addWidget(bouton);
	m_bouton_rotation = bouton;

	QIcon icone_echelle;
	icone_echelle.addPixmap(QPixmap("icones/icone_echelle.png"), QIcon::Mode::Normal, QIcon::State::Off);
	icone_echelle.addPixmap(QPixmap("icones/icone_echelle_active.png"), QIcon::Mode::Normal, QIcon::State::On);

	bouton = new QToolButton();
	bouton->setIcon(icone_echelle);
	bouton->setCheckable(true);
	bouton->setFixedHeight(32);
	bouton->setFixedWidth(32);
	connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::manipule_echelle);
	disp_boutons->addWidget(bouton);
	m_bouton_echelle = bouton;

	m_selecteur_rendu = new QComboBox(this);
	m_selecteur_rendu->addItem("OpenGL", QVariant("opengl"));
	m_selecteur_rendu->addItem("Koudou", QVariant("koudou"));

	connect(m_selecteur_rendu, SIGNAL(currentIndexChanged(int)),
			this, SLOT(change_moteur_rendu(int)));

	disp_boutons->addWidget(m_selecteur_rendu);

	disp_boutons->addStretch();

	disp_widgets->addWidget(m_vue);
	disp_widgets->addLayout(disp_boutons);

	m_main_layout->addLayout(disp_widgets);
}

void EditriceVue3D::ajourne_etat(int evenement)
{
	auto reconstruit_scene = evenement == (type_evenement::objet | type_evenement::ajoute);
	auto const camera_modifie = evenement == (type_evenement::camera_3d | type_evenement::modifie);

	auto ajourne = camera_modifie | reconstruit_scene;
	ajourne |= evenement == (type_evenement::noeud | type_evenement::selectionne);
	ajourne |= evenement == (type_evenement::noeud | type_evenement::enleve);
	ajourne |= evenement == (type_evenement::image | type_evenement::traite);
	ajourne |= evenement == (type_evenement::objet | type_evenement::manipule);
	ajourne |= evenement == (type_evenement::objet | type_evenement::traite);
	ajourne |= evenement == (type_evenement::scene | type_evenement::traite);
	ajourne |= evenement == (type_evenement::temps | type_evenement::modifie);
	ajourne |= evenement == (type_evenement::rafraichissement);

	if (!ajourne) {
		return;
	}

	if (reconstruit_scene) {
		m_vue->reconstruit_scene();
	}

	m_vue->update();
}

void EditriceVue3D::bascule_manipulation()
{
	m_mikisa.manipulation_3d_activee = !m_mikisa.manipulation_3d_activee;
	m_vue->update();
}

void EditriceVue3D::manipule_rotation()
{
	m_bouton_actif->setChecked(false);
	m_bouton_actif = m_bouton_rotation;

	charge_manipulatrice(m_mikisa, MANIPULATION_ROTATION);
	m_vue->update();
}

void EditriceVue3D::manipule_position()
{
	m_bouton_actif->setChecked(false);
	m_bouton_actif = m_bouton_position;

	charge_manipulatrice(m_mikisa, MANIPULATION_POSITION);
	m_vue->update();
}

void EditriceVue3D::manipule_echelle()
{
	m_bouton_actif->setChecked(false);
	m_bouton_actif = m_bouton_echelle;

	charge_manipulatrice(m_mikisa, MANIPULATION_ECHELLE);
	m_vue->update();
}

void EditriceVue3D::change_moteur_rendu(int idx)
{
	INUTILISE(idx);
	auto valeur = m_selecteur_rendu->currentData().toString().toStdString();

	m_vue->change_moteur_rendu(valeur);
	m_vue->update();
}
