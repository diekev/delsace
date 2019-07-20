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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QDialog>
#pragma GCC diagnostic pop

class Koudou;
class QVBoxLayout;
class QGridLayout;

/* ************************************************************************** */

class BaseDialogue : public QDialog {
	Q_OBJECT

	QVBoxLayout *m_agencement;
	QGridLayout *m_agencement_grille;
	Koudou *m_koudou;

public:
	explicit BaseDialogue(Koudou &koudou, QWidget *parent = nullptr);

	BaseDialogue(BaseDialogue const &) = default;
	BaseDialogue &operator=(BaseDialogue const &) = default;

	void montre();
	void ajourne();
};

/* ************************************************************************** */

class ProjectSettingsDialog : public QDialog {
	Q_OBJECT

	QVBoxLayout *m_agencement;
	QGridLayout *m_agencement_grille;
	Koudou *m_koudou;

public:
	explicit ProjectSettingsDialog(Koudou &koudou, QWidget *parent = nullptr);

	ProjectSettingsDialog(ProjectSettingsDialog const &) = default;
	ProjectSettingsDialog &operator=(ProjectSettingsDialog const &) = default;

	void montre();
	void ajourne();
};
