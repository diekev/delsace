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

#include "controle_couleur.h"

#include <QMouseEvent>
#include <QPainter>

#include "dialogues/dialogue_couleur.h"

#include "types/outils.h"

namespace danjo {

ControleCouleur::ControleCouleur(QWidget *parent)
	: QWidget(parent)
	, m_dialogue(new DialogueCouleur(this))
{
	connect(m_dialogue, &DialogueCouleur::couleur_changee,
			this, &ControleCouleur::ajourne_couleur);

	const auto &metriques = this->fontMetrics();
	setFixedHeight(static_cast<int>(static_cast<float>(metriques.height()) * 1.5f));
	setFixedWidth(metriques.horizontalAdvance("#000000"));
}

dls::phys::couleur32 ControleCouleur::couleur()
{
	return m_couleur;
}

void ControleCouleur::couleur(const dls::phys::couleur32 &c)
{
	m_couleur = c;
	update();
}

void ControleCouleur::ajourne_plage(const float min, const float max)
{
	m_dialogue->ajourne_plage(min, max);
}

void ControleCouleur::mouseReleaseEvent(QMouseEvent *e)
{
	if (!QRect(QPoint(0, 0), this->size()).contains(e->pos())) {
		return;
	}

	m_dialogue->couleur_originale(m_couleur);
	m_dialogue->setModal(true);
	m_dialogue->show();

	auto ok = m_dialogue->exec();

	m_couleur = (ok == QDialog::Accepted) ? m_dialogue->couleur_nouvelle()
										  : m_dialogue->couleur_originale();

	update();
	Q_EMIT(couleur_changee());
}

void ControleCouleur::paintEvent(QPaintEvent *)
{
	QPainter peintre(this);
	peintre.fillRect(this->rect(), converti_couleur(m_couleur));
	peintre.setPen(QPen(Qt::black));
	peintre.drawRect(0, 0, this->rect().width() - 1, this->rect().height() - 1);
}

void ControleCouleur::ajourne_couleur()
{
	m_couleur = m_dialogue->couleur_nouvelle();
	update();
	Q_EMIT(couleur_changee());
}

}  /* namespace danjo */
