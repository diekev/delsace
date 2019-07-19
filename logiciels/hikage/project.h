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

#pragma once

#include <QIcon>
#include <QMetaType>
#include <QString>
#include <QVector>

struct Project {
	QString name = "";
	QString filename = "";
	QIcon thumbnail{};

	Project() = default;
	Project(const Project &other) = default;
	~Project() = default;
};

Q_DECLARE_METATYPE(Project)
Q_DECLARE_METATYPE(Project *)

class ProjectManager {
	QString m_project_folder = "";
	QVector<Project> m_projects = {};
	Project *m_current_project = nullptr;
	bool m_need_save = false;

public:
	ProjectManager() = default;
	~ProjectManager() = default;

	ProjectManager(ProjectManager const &) = default;
	ProjectManager &operator=(ProjectManager const &) = default;

	void currentProject(const QString &name);
	void createProject(const QString &name);

	QString projectFolder() const;
	void projectFolder(const QString &name);

	void buildProjectList();

	const QVector<Project> &projects() const;

	QString openProject() const;
	void saveProject(const QString &text, QImage *thumbnail);

	void tagUpdate();
};
