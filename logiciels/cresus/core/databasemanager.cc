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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 */

#include "databasemanager.h"

#include <QDebug>
#include <QDir>

DatabaseManager::DatabaseManager(QObject *parent)
    : QObject(parent)
{}

DatabaseManager::~DatabaseManager()
{
	m_database.close();
}

auto DatabaseManager::openDatabase(const QString &path) -> bool
{
	m_database = QSqlDatabase::addDatabase("QSQLITE");
	m_database.setDatabaseName(path);
	return m_database.open();
}

auto DatabaseManager::createTable() -> bool
{
	auto ret = false;

	if (m_database.isOpen()) {
		QSqlQuery query;

		ret = query.exec("create table if not exists users "
						 "(id integer primary key,"
						 "username varchar(20),"
						 "password varchar(30),"
						 "fullname varchar(30))");
	}

	return ret;
}

auto DatabaseManager::createUserTables(const QString &username) -> bool
{
	auto ret = false;

	if (m_database.isOpen()) {
		QSqlQuery query;
		auto table_name = username + "_expense_table";

		ret = query.exec(
				  "create table if not exists " + table_name +
				  "(date datetime,"
				  "mois integer,"
				  "année integer,"
				  "catégorie varchar(30),"
				  "valeur float)");

		table_name = username + "_monthly_expense_table";

		ret &= query.exec(
				   "create table if not exists " + table_name +
				   "(date datetime,"
				   "mois integer,"
				   "année integer,"
				   "catégorie varchar(30),"
				   "valeur float,"
				   "primary key (date, catégorie))");

		table_name = username + "_yearly_expense_table";

		ret &= query.exec(
				   "create table if not exists " + table_name +
				   "(année integer,"
				   "catégorie varchar(30),"
				   "valeur float,"
				   "primary key (année, catégorie))");

		table_name = username + "_revenue_table";

		ret &= query.exec(
				   "create table if not exists " + table_name +
				   "(date datetime,"
				   "mois integer,"
				   "année integer,"
				   "catégorie varchar(30),"
				   "valeur float)");

		if (!ret) {
			qDebug() << query.lastError();
		}
	}

	return ret;
}

auto DatabaseManager::getDatabase() const -> QSqlDatabase
{
	return m_database;
}
