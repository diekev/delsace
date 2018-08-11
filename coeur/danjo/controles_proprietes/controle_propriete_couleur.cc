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

#include <QMouseEvent>
#include <QPainter>

#include "compilation/morceaux.h"

#include "dialogues/dialogue_couleur.h"

#include "types/outils.h"

#include "donnees_controle.h"

namespace danjo {

ControleProprieteCouleur::ControleProprieteCouleur(QWidget *parent)
	: ControlePropriete(parent)
	, m_couleur(nullptr)
	, m_dialogue(new DialogueCouleur(this))
{
	connect(m_dialogue, &DialogueCouleur::couleur_changee, this, &ControleProprieteCouleur::ajourne_couleur);

	auto metriques = this->fontMetrics();
	setFixedHeight(metriques.height() * 1.5f);
	setFixedWidth(metriques.width("#000000"));
}

void ControleProprieteCouleur::finalise(const DonneesControle &donnees)
{
	m_min = 0.0f;
	if (donnees.valeur_min != "") {
		m_min = std::atof(donnees.valeur_min.c_str());
	}

	m_max = 1.0f;
	if (donnees.valeur_max != "") {
		m_max = std::atof(donnees.valeur_max.c_str());
	}

	m_dialogue->ajourne_plage(m_min, m_max);

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

void ControleProprieteCouleur::ajourne_couleur()
{
	const auto &couleur = m_dialogue->couleur_nouvelle();

	for (int i = 0; i < 4; ++i) {
		m_couleur[i] = couleur[i];
	}

	update();
	Q_EMIT(controle_change());
}

void ControleProprieteCouleur::mouseReleaseEvent(QMouseEvent *e)
{
	if (QRect(QPoint(0, 0), this->size()).contains(e->pos())) {
		m_dialogue->couleur_originale(m_couleur);
		m_dialogue->setModal(true);
		m_dialogue->show();

		auto ok = m_dialogue->exec();

		auto couleur = (ok == QDialog::Accepted) ? m_dialogue->couleur_nouvelle()
												 : m_dialogue->couleur_originale();

		for (int i = 0; i < 4; ++i) {
			m_couleur[i] = couleur[i];
		}

		update();
		Q_EMIT(controle_change());
	}
}

void ControleProprieteCouleur::paintEvent(QPaintEvent *)
{
	QPainter peintre(this);
	peintre.fillRect(this->rect(), converti_couleur(m_couleur));
	peintre.setPen(QPen(Qt::black));
	peintre.drawRect(0, 0, this->rect().width() - 1, this->rect().height() - 1);
}

}  /* namespace danjo */
