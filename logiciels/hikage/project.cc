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

#include "project.h"

#include <iostream>
#include <QDir>
#include <QImage>
#include <QTextStream>

void ProjectManager::currentProject(const QString &name)
{
	m_current_project = nullptr;

	for (auto &project : m_projects) {
		if (project.name == name) {
			m_current_project = &project;
		}
	}

	if (m_current_project == nullptr) {
		createProject(name);
	}
}

void ProjectManager::createProject(const QString &name)
{
	Project p;
	p.name = name;

	m_projects.push_back(p);
	m_current_project = &m_projects.back();

	QString path(projectFolder() + "/" + m_current_project->name);

	if (!QDir().mkpath(path)) {
		throw "Unable to create working directory!\n";
	}

	if (!QDir::setCurrent(path)) {
		throw "Unable to set the current working directory!\n";
	}

	m_need_save = true;
}

QString ProjectManager::projectFolder() const
{
	return m_project_folder;
}

void ProjectManager::projectFolder(const QString &name)
{
	m_project_folder = name;

	if (!QDir(m_project_folder).exists()) {
		QDir().mkpath(m_project_folder);
	}
}

void ProjectManager::buildProjectList()
{
	QDir dir(m_project_folder + "/");

	const auto &dirnames = dir.entryList(QStringList{}, QDir::Dirs, QDir::Name);

	for (const auto &dirname : dirnames) {
		if (dirname == "." || dirname == "..") {
			continue;
		}

		Project p;
		p.name = dirname;
		p.thumbnail = QIcon(dir.absoluteFilePath(dirname + "/thumbnail"));

		m_projects.push_back(p);
	}
}

const QVector<Project> &ProjectManager::projects() const
{
	return m_projects;
}

QString ProjectManager::openProject() const
{
	if (m_current_project == nullptr) {
		return "";
	}

	QString path(projectFolder() + "/" + m_current_project->name);

	if (!QDir::setCurrent(path)) {
		throw "Unable to set the current working directory!\n";
	}

	QFile file("shader.frag");

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream stream(&file);
		return stream.readAll();

		file.close();
	}

	return "";
}

void ProjectManager::saveProject(const QString &text, QImage *thumbnail)
{
	if (!m_need_save) {
		return;
	}

	QFile file("shader.frag");

	if (file.open(QIODevice::WriteOnly)) {
		QTextStream stream(&file);
		stream << text;

		file.close();
	}

	if (thumbnail) {
		if (!thumbnail->save("thumbnail.png")) {
			std::cerr << "Unable to save image\n";
		}
	}

	m_need_save = false;
}

void ProjectManager::tagUpdate()
{
	m_need_save = true;
}
