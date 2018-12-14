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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "editeur_canevas.h"

#include <chronometrage/utilitaires.h>
#include <iostream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QTimer>
#pragma GCC diagnostic pop

#include "bibliotheques/commandes/commande.h"
#include "bibliotheques/commandes/repondant_commande.h"
#include "bibliotheques/opengl/rendu_grille.h"
#include "bibliotheques/opengl/rendu_texte.h"
#include "bibliotheques/vision/camera.h"

#include "opengl/rendu_courbes.h"
#include "opengl/rendu_maillage.h"
#include "opengl/rendu_nuage_points.h"

#include "coeur/sdk/mesh.h"
#include "coeur/sdk/prim_points.h"
#include "coeur/sdk/segmentprim.h"

#include "coeur/object.h"
#include "coeur/scene.h"

template <typename T>
[[nodiscard]] auto converti_matrice_opengl(dls::math::mat4x4<T> const &mat)
{
	dls::math::mat4x4<float> resultat;

	for (size_t i = 0; i < 4; ++i) {
		for (size_t j = 0; j < 4; ++j) {
			resultat[i][j] = static_cast<float>(mat[i][j]);
		}
	}

	return resultat;
}

Canevas::Canevas(RepondantCommande *repondant, QWidget *parent)
    : QGLWidget(parent)
	, m_contexte_rendu()
	, m_repondant_commande(repondant)
	, m_debut(0)
{
	setFocusPolicy(Qt::FocusPolicy::StrongFocus);
}

Canevas::~Canevas()
{
	delete m_rendu_texte;
	delete m_rendu_grille;
}

void Canevas::initializeGL()
{
	glewExperimental = GL_TRUE;
	auto const &erreur = glewInit();

	if (erreur != GLEW_OK) {
		std::cerr << "Erreur lors de l'initialisation du canevas OpenGL : "
				  << glewGetErrorString(erreur)
				  << '\n';
	}

	glClearColor(m_bg.r, m_bg.g, m_bg.b, m_bg.a);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

//	m_rendu_grille = new RenduGrille(20, 20);
	m_rendu_texte = new RenduTexte();
	m_contexte->camera->ajourne();

	m_debut = numero7::chronometrage::maintenant();
}

void Canevas::resizeGL(int largeur, int hauteur)
{
	glViewport(0, 0, largeur, hauteur);
	m_contexte->camera->redimensionne(largeur, hauteur);
	m_rendu_texte->etablie_dimension_fenetre(largeur, hauteur);
}

void Canevas::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* setup stencil mask for outlining active object */
	glStencilFunc(GL_ALWAYS, 1, 0xff);
	glStencilMask(0xff);

	m_contexte->camera->ajourne();

	auto const &MV = m_contexte->camera->MV();
	auto const &P = m_contexte->camera->P();
	auto const &MVP = P * MV;

	m_contexte_rendu.vue(m_contexte->camera->dir());
	m_contexte_rendu.modele_vue(MV);
	m_contexte_rendu.projection(P);
	m_contexte_rendu.MVP(MVP);
	m_contexte_rendu.normal(dls::math::inverse_transpose(dls::math::mat3_depuis_mat4(MV)));
	m_contexte_rendu.matrice_objet(converti_matrice_opengl(m_pile.sommet()));
	m_contexte_rendu.pour_surlignage(false);

	/* À FAIRE */
//	if (m_dessine_grille) {
//		m_rendu_grille->dessine(m_contexte_rendu);
//	}

	/* XXX - only happens on initialization, but not nice. Better to construct
     * context listeners with valid context. */
	if (m_contexte == nullptr) {
		return;
	}

	std::unique_ptr<RenduPrimitive> rendu_primitive;

	if (m_contexte->scene != nullptr) {
		for (auto &node : m_contexte->scene->nodes()) {
			auto object = static_cast<Object *>(node.get());

			if (!object->collection()) {
				continue;
			}

			const bool active_object = (object == m_contexte->scene->active_node());

			auto const collection = object->collection();

			if (object->parent()) {
				m_pile.pousse(object->parent()->matrix());
			}

			m_pile.pousse(object->matrix());

			for (auto &prim : collection->primitives()) {
				/* update prim before drawing */
				prim->update();

				if (prim->typeID() == Mesh::id) {
					auto maillage = new RenduMaillage(static_cast<Mesh *>(prim));
					rendu_primitive = std::unique_ptr<RenduPrimitive>(maillage);
				}
				else if (prim->typeID() == PrimPoints::id) {
					auto points = new RenduNuagePoints(static_cast<PrimPoints *>(prim));
					rendu_primitive = std::unique_ptr<RenduPrimitive>(points);
				}
				else if (prim->typeID() == SegmentPrim::id) {
					auto courbes = new RenduCourbes(static_cast<SegmentPrim *>(prim));
					rendu_primitive = std::unique_ptr<RenduPrimitive>(courbes);
				}
				else {
					continue;
				}

				/* À FAIRE : dessine la boîte englobante. */

				rendu_primitive->initialise();

				m_pile.pousse(prim->matrix());

				m_contexte_rendu.matrice_objet(converti_matrice_opengl(m_pile.sommet()));

				rendu_primitive->dessine(m_contexte_rendu);

				if (active_object) {
					m_contexte_rendu.pour_surlignage(true);

					glStencilFunc(GL_NOTEQUAL, 1, 0xff);
					glStencilMask(0x00);
					glDisable(GL_DEPTH_TEST);

					glLineWidth(5);
					glPolygonMode(GL_FRONT, GL_LINE);

					rendu_primitive->dessine(m_contexte_rendu);

					/* Restore state. */
					glPolygonMode(GL_FRONT, GL_FILL);
					glLineWidth(1);

					glStencilFunc(GL_ALWAYS, 1, 0xff);
					glStencilMask(0xff);
					glEnable(GL_DEPTH_TEST);

					m_contexte_rendu.pour_surlignage(false);
				}

				m_pile.enleve_sommet();
			}

			m_pile.enleve_sommet();

			if (object->parent()) {
				m_pile.enleve_sommet();
			}
		}
	}

	auto const fin = numero7::chronometrage::maintenant();

	auto const temps = fin - m_debut;
	auto const fps = static_cast<int>(1.0 / temps);

	std::stringstream ss;
	ss << fps << " IPS";

	glEnable(GL_BLEND);

	m_rendu_texte->reinitialise();
	m_rendu_texte->dessine(m_contexte_rendu, ss.str());

	glDisable(GL_BLEND);

	m_debut = numero7::chronometrage::maintenant();
}

void Canevas::mousePressEvent(QMouseEvent *e)
{
	DonneesCommande donnees;
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());
	donnees.double_clique = false;
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());
	donnees.souris = e->button();

	m_base->set_active();
	m_repondant_commande->appele_commande("vue_3d", donnees);
}

void Canevas::mouseMoveEvent(QMouseEvent *e)
{
	DonneesCommande donnees;
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());
	donnees.double_clique = false;
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());
	donnees.souris = e->button();

	m_repondant_commande->ajourne_commande_modale(donnees);
}

void Canevas::mouseReleaseEvent(QMouseEvent *e)
{
	DonneesCommande donnees;
	donnees.x = static_cast<float>(e->pos().x());
	donnees.y = static_cast<float>(e->pos().y());
	donnees.double_clique = false;
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());
	donnees.souris = e->button();

	m_repondant_commande->acheve_commande_modale(donnees);
}

void Canevas::keyPressEvent(QKeyEvent *e)
{
	DonneesCommande donnees;
	donnees.cle = e->key();

	m_base->set_active();
	m_repondant_commande->appele_commande("vue_3d", donnees);
}

void Canevas::wheelEvent(QWheelEvent *e)
{
	DonneesCommande donnees;
	donnees.x = static_cast<float>(e->delta());
	donnees.double_clique = true;
	donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());
	donnees.souris = Qt::MidButton;

	m_base->set_active();
	m_repondant_commande->appele_commande("vue_3d", donnees);
}

void Canevas::set_context(Context *context)
{
	m_contexte = context;
}

void Canevas::set_base(BaseEditrice *base)
{
	m_base = base;
}

void Canevas::changeBackground()
{
	auto const &color = QColorDialog::getColor();

	if (color.isValid()) {
		m_bg = dls::math::vec4f(
				   static_cast<float>(color.redF()),
				   static_cast<float>(color.greenF()),
				   static_cast<float>(color.blueF()),
				   1.0f);

		glClearColor(m_bg.r, m_bg.g, m_bg.b, m_bg.a);
		update();
	}
}

void Canevas::drawGrid(bool b)
{
	m_dessine_grille = b;
	update();
}

/* ************************************************************************** */

EditeurCanvas::EditeurCanvas(RepondantCommande *repondant, QWidget *parent)
	: BaseEditrice(parent)
	, m_viewer(new Canevas(repondant, this))
{
	m_viewer->set_base(this);

	auto vert_layout = new QVBoxLayout();
	vert_layout->addWidget(m_viewer);

	auto horiz_layout = new QHBoxLayout();

	auto push_button = new QPushButton("Change Color");
	push_button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
	connect(push_button, SIGNAL(clicked()), m_viewer, SLOT(changeBackground()));
	horiz_layout->addWidget(push_button);

	auto check = new QCheckBox("Draw Box");
	check->setChecked(true);
	connect(check, SIGNAL(toggled(bool)), m_viewer, SLOT(drawGrid(bool)));
	horiz_layout->addWidget(check);

	vert_layout->addLayout(horiz_layout);

	m_main_layout->addLayout(vert_layout);
}

void EditeurCanvas::update_state(type_evenement event)
{
	if (categorie_evenement(event) == type_evenement::noeud) {
		/* Ignore all events from edit mode stuff. */
		if (m_context->eval_ctx->edit_mode) {
			return;
		}
	}

	m_viewer->set_context(m_context);
	m_viewer->update();
}
