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

#include "base_editrice.h"

#include <QApplication>
#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QStyle>
#include <QVariant>

#include "coeur/composite.h"
#include "coeur/mikisa.h"

BaseEditrice::BaseEditrice(Mikisa *mikisa, QWidget *parent)
	: danjo::ConteneurControles(parent)
	, m_mikisa(mikisa)
    , m_frame(new QFrame(this))
    , m_layout(new QVBoxLayout())
{
	this->ecoute(m_mikisa);

	QSizePolicy size_policy(QSizePolicy::Preferred, QSizePolicy::Preferred);
	size_policy.setHorizontalStretch(0);
	size_policy.setVerticalStretch(0);
	size_policy.setHeightForWidth(m_frame->sizePolicy().hasHeightForWidth());

	/* Intern frame, where individual entreface regions put their buttons. */

	m_frame->setSizePolicy(size_policy);
	m_frame->setFrameShape(QFrame::StyledPanel);
	m_frame->setFrameShadow(QFrame::Raised);

	m_layout->addWidget(m_frame);

	m_layout->setMargin(0);
	this->setLayout(m_layout);

	m_main_layout = new QHBoxLayout(m_frame);
	m_main_layout->setMargin(0);

	this->actif(false);
}

void BaseEditrice::actif(bool ouinon)
{
	m_frame->setProperty("state", (ouinon) ? "on" : "off");
	m_frame->setStyle(QApplication::style());
}

void BaseEditrice::rend_actif()
{
	if (m_mikisa->editrice_active) {
		m_mikisa->editrice_active->actif(false);
	}

	m_mikisa->editrice_active = this;
	this->actif(true);
}

void BaseEditrice::mousePressEvent(QMouseEvent *e)
{
	this->rend_actif();
	QWidget::mousePressEvent(e);
}
