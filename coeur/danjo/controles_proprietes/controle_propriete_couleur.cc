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

#include "controle_propriete_couleur.h"

#include <QColorDialog>
#include <QMouseEvent>
#include <QPainter>

#include "interne/morceaux.h"

#include "donnees_controle.h"

namespace danjo {

SelecteurCouleur::SelecteurCouleur(QWidget *parent)
	: ControlePropriete(parent)
	, m_couleur(nullptr)
{}

void SelecteurCouleur::finalise(const DonneesControle &donnees)
{
	m_min = std::atof(donnees.valeur_min.c_str());
	m_max = std::atof(donnees.valeur_max.c_str());
	m_couleur = static_cast<float *>(donnees.pointeur);

	auto valeurs = decoupe(donnees.valeur_defaut, ',');
	auto index = 0;

	m_valeur_defaut[0] = 1.0f;
	m_valeur_defaut[1] = 1.0f;
	m_valeur_defaut[2] = 1.0f;
	m_valeur_defaut[3] = 1.0f;

	for (auto v : valeurs) {
		m_valeur_defaut[index++] = std::atof(v.c_str());
	}

	if (donnees.initialisation) {
		for (int i = 0; i < 4; ++i) {
			m_couleur[i] = m_valeur_defaut[i];
		}
	}

	setToolTip(donnees.infobulle.c_str());
}

void SelecteurCouleur::mouseReleaseEvent(QMouseEvent *e)
{
	if (QRect(QPoint(0, 0), this->size()).contains(e->pos())) {
		Q_EMIT(clicked());

		const auto &color = QColorDialog::getColor(QColor(m_couleur[0] * 255, m_couleur[1] * 255, m_couleur[2] * 255, m_couleur[3] * 255));

		if (color.isValid()) {
			m_couleur[0] = color.redF();
			m_couleur[1] = color.greenF();
			m_couleur[2] = color.blueF();
			m_couleur[3] = color.alphaF();

			for (int i = 0; i < 4; ++i) {
				Q_EMIT(valeur_changee(m_couleur[i], i));
			}
		}
	}
}

void SelecteurCouleur::paintEvent(QPaintEvent *)
{
	QPainter painter(this);
	const auto &rect = this->geometry();

	QColor color(m_couleur[0] * 255, m_couleur[1] * 255, m_couleur[2] * 255, m_couleur[3] * 255);

	const auto w = rect.width();
	const auto h = rect.height();

	painter.fillRect(0, 0, w, h, color);
}

ControleProprieteCouleur::ControleProprieteCouleur(QWidget *parent)
	: SelecteurCouleur(parent)
{
	connect(this, &SelecteurCouleur::valeur_changee, this, &ControleProprieteCouleur::ajourne_valeur_pointee);
	auto metriques = this->fontMetrics();
	setFixedHeight(metriques.height() * 1.5f);
	setFixedWidth(metriques.width("#000000"));
}

void ControleProprieteCouleur::ajourne_valeur_pointee(double valeur, int axis)
{
	m_couleur[axis] = static_cast<float>(valeur);
	Q_EMIT(controle_change());
}

}  /* namespace danjo */
