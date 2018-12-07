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

#pragma once

#include <GL/glew.h>  /* needs to be included before QGLWidget (includes gl.h) */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QGLWidget>
#pragma GCC diagnostic pop

#include <stack>

#include "../bibliotheques/opengl/contexte_rendu.h"
#include "../bibliotheques/opengl/pile_matrice.h"

#include "coeur/sdk/context.h"
#include "coeur/util/util_input.h"

#include "base_editeur.h"

class RenduGrille;
class RenduTexte;
class RepondantCommande;
class Scene;

class Canevas : public QGLWidget {
	Q_OBJECT

	bool m_dessine_grille = true;
	dls::math::vec4f m_bg = dls::math::vec4f(0.5f, 0.5f, 0.5f, 1.0f);

	RenduGrille *m_rendu_grille = nullptr;
	RenduTexte *m_rendu_texte = nullptr;
	ContexteRendu m_contexte_rendu;

	RepondantCommande *m_repondant_commande;

	PileMatrice m_pile = {};

	Context *m_contexte = nullptr;
	BaseEditrice *m_base = nullptr;
	double m_debut;

public Q_SLOTS:
	void changeBackground();
	void drawGrid(bool b);

public:
	explicit Canevas(RepondantCommande *repondant, QWidget *parent = nullptr);
	~Canevas();

	Canevas(Canevas const &) = default;
	Canevas &operator=(Canevas const &) = default;

	void initializeGL();
	void paintGL();
	void resizeGL(int largeur, int hauteur);

	void keyPressEvent(QKeyEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void wheelEvent(QWheelEvent *e);

	void set_context(Context *context);

	void set_base(BaseEditrice *base);
};

/* ************************************************************************** */

class EditeurCanvas : public BaseEditrice {
	Q_OBJECT

	Canevas *m_viewer;

public:
	explicit EditeurCanvas(RepondantCommande *repondant, QWidget *parent = nullptr);

	EditeurCanvas(EditeurCanvas const &) = default;
	EditeurCanvas &operator=(EditeurCanvas const &) = default;

	void update_state(type_evenement event) override;
};
