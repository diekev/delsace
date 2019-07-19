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

#pragma once

#include <QMainWindow>
#include <QSqlDatabase>
#include <QTranslator>

#include "biblinternes/structures/tableau.hh"

namespace Ui {
class MainWindow;
}

class AccountDialog;
class AccountWidget;
class Compte;
class MonnaieDialog;
class MonthlyTableWidget;
class RetraitDialog;
class TransactionDialog;
class UserPreferences;
class Utilisateur;

enum {
	REVENUE_ACTIF	 = 0,
	REVENUE_PASSIF	 = 1,
	DEPENSE_PERS	 = 2,
	DEPENSE_PROF	 = 3,
	DEPENSE_LGMT	 = 4,
	DEPENSE_TRSPRT   = 5,
};

class MainWindow : public QMainWindow {
	Q_OBJECT

	Ui::MainWindow *ui = nullptr;

	AccountDialog *m_account_dialog = nullptr;
	TransactionDialog *m_transaction_dialog = nullptr;
	MonnaieDialog *m_monnaie_dialog = nullptr;
	RetraitDialog *m_retrait_dialog = nullptr;
	/* TODO: per-user preferences */
	UserPreferences *m_user_prefs = nullptr;

	Utilisateur *m_user = nullptr;
	dls::tableau<Utilisateur *> m_users = {};

	dls::tableau<AccountWidget *> m_account_widgets = {};
	dls::tableau<MonthlyTableWidget *> m_active_revenues = {};
	dls::tableau<MonthlyTableWidget *> m_passive_revenues = {};
	dls::tableau<MonthlyTableWidget *> m_personal_expenses = {};
	dls::tableau<MonthlyTableWidget *> m_housing_expenses = {};
	dls::tableau<MonthlyTableWidget *> m_transport_expenses = {};

	QSqlDatabase m_database = {};
	QTranslator m_translator{};
	QString m_current_language = "";

	auto keyPressEvent(QKeyEvent *e) -> void;
	auto closeEvent(QCloseEvent *) -> void;
	auto changeEvent(QEvent *event) -> void;

	void switchTranslator(QTranslator &translator, const QString &filname);
	void loadLanguage(const QString &language);
	auto addTransaction(const bool income) -> void;
	void updateUserTables(QString table_name, int begin, int end, int annee);

private Q_SLOTS:
	void about();
	void ajouterDepense();
	void ajouterRetrait();
	void ajouterGain();
	void ajouterCompte();
	void editWallet();
	void saveFile();
	void openFile();
	void editPreferences();
	void editAccount(const QString &name, const double value);

public:
	MainWindow();
	~MainWindow();

	MainWindow(MainWindow const &) = default;
	MainWindow &operator=(MainWindow const &) = default;

	auto setUser(const QString &name) -> void;
	auto createUser(const QString &name) -> void;
	auto updateBilanTab() -> void;
	auto updateBilanWidget(const QString &name, const double value) -> void;

	auto createDefaultTableWidgets() -> void;
	auto updateTableWidget(const QString &category, const double value, const int mois, int type) -> void;

	auto setDatabase(const QSqlDatabase &db) -> void;

	auto readSettings() -> void;
	auto writeSettings() -> void;
	auto createWidgetForAccount(const Compte &compte) -> void;
	auto updateUserTables(QDate date, int mois, int annee, double montant, QString category) -> void;
	auto updateUserTables() -> void;
};
