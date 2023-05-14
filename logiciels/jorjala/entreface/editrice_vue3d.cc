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

#include "coeur/evenement.h"
//#include "coeur/manipulatrice.h"
//#include "coeur/operatrice_image.h"
//#include "coeur/rendu.hh"

#include "coeur/jorjala.hh"

#include "opengl/visionneur_scene.h"

/* ************************************************************************** */

static void charge_manipulatrice(JJL::Jorjala &jorjala, int type_manipulation)
{
#if 0
	jorjala.type_manipulation_3d = type_manipulation;

	auto graphe = jorjala.graphe;
	auto noeud = graphe->noeud_actif;

	if (noeud == nullptr) {
		return;
	}

	if (noeud->type != type_noeud::OPERATRICE) {
		return;
	}

	auto operatrice = extrait_opimage(noeud->donnees);

	if (!operatrice->possede_manipulatrice_3d(jorjala.type_manipulation_3d)) {
		jorjala.manipulatrice_3d = nullptr;
		return;
	}

	jorjala.manipulatrice_3d = operatrice->manipulatrice_3d(jorjala.type_manipulation_3d);
#endif
}

/* ************************************************************************** */

VueCanevas3D::VueCanevas3D(JJL::Jorjala &jorjala, EditriceVue3D *base, QWidget *parent)
	: QGLWidget(parent)
	, m_jorjala(jorjala)
	, m_visionneur_scene(new VisionneurScene(this, jorjala))
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
    m_base->mousePressEvent(e);
}

void VueCanevas3D::mouseMoveEvent(QMouseEvent *e)
{
    m_base->mouseMoveEvent(e);
}

void VueCanevas3D::wheelEvent(QWheelEvent *e)
{
    m_base->wheelEvent(e);
}

void VueCanevas3D::mouseReleaseEvent(QMouseEvent *e)
{
    m_base->mouseReleaseEvent(e);
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

enum {
   BOUTON_MANIPULATION = 0,
   BOUTON_POSITION = 1,
   BOUTON_ROTATION = 2,
   BOUTON_ECHELLE = 3,

   NOMBRE_DE_BOUTONS = 4,
};

struct DonneesBoutonsManipulation {
    int type = BOUTON_MANIPULATION;
    const char *chemin_icone_inactif;
    const char *chemin_icone_actif;
};

static DonneesBoutonsManipulation donnees_boutons_manipulation[NOMBRE_DE_BOUTONS] = {
    {BOUTON_MANIPULATION, "icones/icone_manipulation.png", "icones/icone_manipulation_active.png"},
    {BOUTON_POSITION, "icones/icone_position.png", "icones/icone_position_active.png"},
    {BOUTON_ROTATION, "icones/icone_rotation.png", "icones/icone_rotation_active.png"},
    {BOUTON_ECHELLE, "icones/icone_echelle.png", "icones/icone_echelle_active.png"},
};

EditriceVue3D::EditriceVue3D(JJL::Jorjala &jorjala, QWidget *parent)
    : BaseEditrice("vue_3d", jorjala, parent)
	, m_vue(new VueCanevas3D(jorjala, this, this))
{
	auto disp_widgets = new QVBoxLayout();
	auto disp_boutons = new QHBoxLayout();
	disp_boutons->addStretch();

    QToolButton *boutons_manipulation[4];
    for (auto données_bouton : donnees_boutons_manipulation) {
        QIcon icone_manipulation;
        icone_manipulation.addPixmap(QPixmap(données_bouton.chemin_icone_inactif), QIcon::Mode::Normal, QIcon::State::Off);
        icone_manipulation.addPixmap(QPixmap(données_bouton.chemin_icone_actif), QIcon::Mode::Normal, QIcon::State::On);

        auto bouton = new QToolButton();
        bouton->setIcon(icone_manipulation);
        bouton->setFixedHeight(32);
        bouton->setFixedWidth(32);
        bouton->setCheckable(true);
        disp_boutons->addWidget(bouton);

        boutons_manipulation[données_bouton.type] = bouton;
        disp_boutons->addWidget(bouton);
    }

    auto bouton = boutons_manipulation[BOUTON_MANIPULATION];
	bouton->setChecked(false);
    connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::bascule_manipulation);

    bouton = boutons_manipulation[BOUTON_POSITION];
	bouton->setChecked(true);
	connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::manipule_position);
	m_bouton_position = bouton;
	m_bouton_actif = bouton;

    bouton = boutons_manipulation[BOUTON_ROTATION];
	connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::manipule_rotation);
	m_bouton_rotation = bouton;

    bouton = boutons_manipulation[BOUTON_ECHELLE];
	connect(bouton, &QToolButton::clicked, this, &EditriceVue3D::manipule_echelle);
	m_bouton_echelle = bouton;

	m_selecteur_rendu = new QComboBox(this);

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
	ajourne |= evenement == (type_evenement::temps | type_evenement::modifie);
	ajourne |= evenement == (type_evenement::rafraichissement);

	auto signaux_blockes = m_selecteur_rendu->blockSignals(true);
	m_selecteur_rendu->clear();

//	for (auto rendu : m_jorjala.bdd.rendus()) {
//		m_selecteur_rendu->addItem(rendu->noeud.nom.c_str(), QVariant(rendu->noeud.nom.c_str()));
//	}

	m_selecteur_rendu->blockSignals(signaux_blockes);

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
    // m_jorjala.manipulation_3d_activee = !m_jorjala.manipulation_3d_activee;
	m_vue->update();
}

void EditriceVue3D::manipule_rotation()
{
	m_bouton_actif->setChecked(false);
	m_bouton_actif = m_bouton_rotation;

    // charge_manipulatrice(m_jorjala, MANIPULATION_ROTATION);
	m_vue->update();
}

void EditriceVue3D::manipule_position()
{
	m_bouton_actif->setChecked(false);
	m_bouton_actif = m_bouton_position;

    // charge_manipulatrice(m_jorjala, MANIPULATION_POSITION);
	m_vue->update();
}

void EditriceVue3D::manipule_echelle()
{
	m_bouton_actif->setChecked(false);
	m_bouton_actif = m_bouton_echelle;

    // charge_manipulatrice(m_jorjala, MANIPULATION_ECHELLE);
	m_vue->update();
}

void EditriceVue3D::change_moteur_rendu(int idx)
{
	INUTILISE(idx);
	auto valeur = m_selecteur_rendu->currentData().toString().toStdString();

	m_vue->change_moteur_rendu(valeur);
	m_vue->update();
}
