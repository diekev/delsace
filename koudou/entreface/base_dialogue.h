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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include <QDialog>

#include "controles/assembleur_controles.h"

class Koudou;
class QVBoxLayout;

/* ************************************************************************** */

class BaseDialogue : public QDialog {
	Q_OBJECT

	QVBoxLayout *m_agencement;
	QGridLayout *m_agencement_grille;
	AssembleurControles m_assembleur_controles;
	Koudou *m_koudou;

public:
	explicit BaseDialogue(Koudou &koudou, QWidget *parent = nullptr);

	void montre();
	void ajourne();
};

/* ************************************************************************** */

class ProjectSettingsDialog : public QDialog {
	Q_OBJECT

	QVBoxLayout *m_agencement;
	QGridLayout *m_agencement_grille;
	AssembleurControles m_assembleur_controles;
	Koudou *m_koudou;

public:
	explicit ProjectSettingsDialog(Koudou &koudou, QWidget *parent = nullptr);

	void montre();
	void ajourne();
};
