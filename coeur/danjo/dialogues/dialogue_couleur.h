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

#pragma once

#include <QDialog>

#include "types/couleur.h"

class ControleNombreDecimal;
class QGridLayout;
class QHBoxLayout;
class QLabel;
class QVBoxLayout;

namespace danjo {

class AffichageCouleur;
class ControleSatVal;
class SelecteurTeinte;
class ControleValeurCouleur;

class DialogueCouleur final : public QDialog {
	Q_OBJECT

	QVBoxLayout *m_disposition{};
	QVBoxLayout *m_disposition_sel_ct{};
	QHBoxLayout *m_disposition_sel_cv{};
	QHBoxLayout *m_disposition_horiz{};
	QGridLayout *m_disposition_rvba{};
	QHBoxLayout *m_disposition_boutons{};

	ControleSatVal *m_selecteur_sat_val{};
	SelecteurTeinte *m_selecteur_teinte{};
	ControleValeurCouleur *m_selecteur_valeur{};

	ControleNombreDecimal *m_r{};
	ControleNombreDecimal *m_v{};
	ControleNombreDecimal *m_b{};
	ControleNombreDecimal *m_h{};
	ControleNombreDecimal *m_s{};
	ControleNombreDecimal *m_v0{};
	ControleNombreDecimal *m_a{};
	AffichageCouleur *m_affichage_couleur_nouvelle{};
	AffichageCouleur *m_affichage_couleur_originale{};

	couleur32 m_couleur_origine{};
	couleur32 m_couleur_nouvelle{};

	QLabel *m_contraste{};

	void ajourne_label_contraste();

public:
	explicit DialogueCouleur(QWidget *parent = nullptr);

	DialogueCouleur(DialogueCouleur const &) = default;
	DialogueCouleur &operator=(DialogueCouleur const &) = default;

	void couleur_originale(const couleur32 &c);
	couleur32 couleur_originale();

	couleur32 couleur_nouvelle();

	couleur32 couleur();

	void ajourne_plage(float min, float max);

	static couleur32 requiers_couleur(const couleur32 &couleur_origine);

public Q_SLOTS:
	void ajourne_couleur();
	void ajourne_couleur_hsv();
	void ajourne_couleur_sat_val();
	void ajourne_couleur_teinte();
	void ajourne_couleur_valeur();

Q_SIGNALS:
	void couleur_changee();
};

}  /* namespace danjo */
