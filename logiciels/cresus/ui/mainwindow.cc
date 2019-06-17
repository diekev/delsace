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

#include "mainwindow.h"

#include <QDate>
#include <QDebug>
#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>
#include <QSqlError>
#include <QSqlQuery>
#include <QTextStream>

#include "dialogs.h"
#include "interfacewidgets.h"
#include "ui_mainwindow.h"
#include "utilisateur.h"

#include "util/util_bool.h"

MainWindow::MainWindow()
    : ui(new Ui::MainWindow)
    , m_account_dialog(new AccountDialog(this))
    , m_transaction_dialog(new TransactionDialog(this))
    , m_monnaie_dialog(new MonnaieDialog(this))
    , m_retrait_dialog(new RetraitDialog(this))
    , m_user_prefs(new UserPreferences(this))
    , m_user(nullptr)
{
	ui->setupUi(this);

	ui->m_cash_amount->setName(tr("Argent Liquide"));
	connect(ui->m_cash_amount, SIGNAL(valueChanged(QString, float)),
	        this, SLOT(editAccount(QString, float)));
	ui->m_user_info->setSize(14);
	ui->m_user_info->setBold(true, true);
	ui->m_user_info->showPushButton(false);

	ui->m_capital_negatif->hide();
	ui->m_capital_positif->hide();
	ui->m_debt_group_box->hide();
	ui->m_debt_short->hide();
	ui->m_debt_long->hide();

	readSettings();
	createDefaultTableWidgets();

	/* TODO: de-duplicate this, make it prettier */
	auto locale = QLocale::system().name();
	locale.truncate(locale.lastIndexOf('_'));
	m_current_language = locale;
	locale = m_user_prefs->currentLanguageLocale();
	loadLanguage(locale);
}

MainWindow::~MainWindow()
{
	delete ui;

	if (m_user != nullptr) {
		delete m_user;
	}
}

/* ********************************* Events ********************************* */

auto MainWindow::closeEvent(QCloseEvent*) -> void
{
	writeSettings();
}

auto MainWindow::keyPressEvent(QKeyEvent *e) -> void
{
	switch (e->key()) {
		case Qt::Key_F11:
			setWindowState(this->windowState() ^ Qt::WindowFullScreen);
			ui->menubar->setHidden(!ui->menubar->isHidden());
			break;
	}
}

auto MainWindow::changeEvent(QEvent *event) -> void
{
	if (event == nullptr) {
		return;
	}

	switch (event->type()) {
		case QEvent::LanguageChange:
			ui->retranslateUi(this);
			break;
		case QEvent::LocaleChange:
		{
			auto locale = QLocale::system().name();
			locale.truncate(locale.lastIndexOf('_'));
			loadLanguage(locale);
			break;
		}
		default:
			break;
	}
}

/* ********************************* Setters ******************************** */

auto MainWindow::setDatabase(const QSqlDatabase &db) -> void
{
	m_database = db;
}

auto MainWindow::setUser(const QString &name) -> void
{
	for (const auto &user : m_users) {
		if (user->name() == name) {
			m_user = user;
			break;
		}
	}

	openFile();
	updateUserTables();
}

/* ********************************* Factory ******************************** */

auto MainWindow::createUser(const QString &name) -> void
{
	m_user = new Utilisateur(name);

	if (!(std::find(m_users.begin(), m_users.end(), m_user) != m_users.end())) {
		m_users.push_back(m_user);
	}
}

auto MainWindow::createDefaultTableWidgets() -> void
{
	ui->m_net_salary->setCategory(tr("Salaire Net"));
	m_active_revenues.push_back(ui->m_net_salary);

	ui->m_interests->setCategory(tr("Intérêts"));
	m_passive_revenues.push_back(ui->m_interests);

	ui->m_groceries->setCategory(tr("Alimentaire"));
	m_personal_expenses.push_back(ui->m_groceries);
	ui->m_clothing->setCategory(tr("Habillement"));
	m_personal_expenses.push_back(ui->m_clothing);
	ui->m_hygiene->setCategory(tr("Hygiène"));
	m_personal_expenses.push_back(ui->m_hygiene);
	ui->m_health->setCategory(tr("Santé"));
	m_personal_expenses.push_back(ui->m_health);
	ui->m_phone->setCategory(tr("Téléphone"));
	m_personal_expenses.push_back(ui->m_phone);

	/* TODO: make it a separate function */
	for (const auto &expense : m_personal_expenses) {
		m_transaction_dialog->addTypeItem(expense->category(), QVariant(DEPENSE_PERS), false);
	}

	for (const auto &expense : m_active_revenues) {
		m_transaction_dialog->addTypeItem(expense->category(), QVariant(REVENUE_ACTIF), true);
	}

	for (const auto &expense : m_passive_revenues) {
		m_transaction_dialog->addTypeItem(expense->category(), QVariant(REVENUE_PASSIF), true);
	}
}

auto MainWindow::createWidgetForAccount(const Compte &compte) -> void
{
	auto name = compte.name();
	m_retrait_dialog->addCompteItem(name);
	m_transaction_dialog->addCompteItem(name);

	auto widget = new AccountWidget(name, compte.value());
	connect(widget, SIGNAL(valueChanged(QString, float)),
	        this, SLOT(editAccount(QString, float)));
	m_account_widgets.push_back(widget);

	if (compte.type() == COMPTE_COURANT) {
		ui->m_capital_liquid->layout()->addWidget(widget);
	}
	else {
		ui->m_capital_positif->layout()->addWidget(widget);

		if (ui->m_capital_positif->isHidden()) {
			ui->m_capital_positif->show();
		}
	}
}

void MainWindow::editAccount(const QString &name, const float value)
{
	if (name == "Argent Liquide") {
		m_user->setArgentLiquide(value);
	}
	else {
		for (auto &account : m_user->accounts()) {
			if (account.name() == name) {
				account.setValue(value);
				break;
			}
		}
	}

	updateBilanTab();
	saveFile();
}

/* ********************************* Dialogs ******************************** */

auto MainWindow::about() -> void
{
	QMessageBox::about(this, tr("À propos de Crésus"),
	                   tr("<b>Crésus</b> est un logiciel \
	                      de gestion financière personel."));
}

auto MainWindow::ajouterRetrait() -> void
{
	m_retrait_dialog->show();

	if (m_retrait_dialog->exec() == QDialog::Accepted) {
		Compte *compte = &m_user->account(m_retrait_dialog->accountIndex());
		compte->setWithdrawal(m_retrait_dialog->montant());

		m_user->setArgentLiquide(m_user->getArgentLiquide() + m_retrait_dialog->montant());

		m_retrait_dialog->resetToDefaults();

		updateBilanWidget(compte->name(), compte->value());

		updateBilanTab();
		saveFile();
	}
}

auto MainWindow::ajouterGain() -> void
{
	addTransaction(true);
}

auto MainWindow::ajouterDepense() -> void
{
	addTransaction(false);
}

auto MainWindow::addTransaction(const bool income) -> void
{
	m_transaction_dialog->setTransactionUI(income);
	m_transaction_dialog->show();

	if (m_transaction_dialog->exec() == QDialog::Accepted) {
		auto category = m_transaction_dialog->category(income);
		auto montant = m_transaction_dialog->montant();
		auto date = m_transaction_dialog->date();
		auto mois = date.month();
		auto annee = date.year();
		auto type = m_transaction_dialog->typeItem(income);
		auto medium = m_transaction_dialog->mediumItem();

		if (medium == TRANSACTION_LIQUID) {
			m_user->setArgentLiquide(m_user->getArgentLiquide() + ((income) ? montant : -montant));
		}
		else if (is_elem(medium, TRANSACTION_CHECK, TRANSACTION_WIRE)) {
			Compte *compte = &m_user->account(m_transaction_dialog->accountIndex());
			compte->addTransaction(montant, income);
			updateBilanWidget(compte->name(), compte->value());
		}

		updateTableWidget(category, montant, mois, type);

		if (!income) {
			updateUserTables(date, mois, annee, montant, category);
		}

		m_transaction_dialog->resetToDefaults();

		updateBilanTab();
		saveFile();
	}
}

auto MainWindow::ajouterCompte() -> void
{
	m_account_dialog->show();

	if (m_account_dialog->exec() == QDialog::Accepted) {
		Compte compte;

		compte.setName(m_account_dialog->name());
		compte.setType(m_account_dialog->type());
		compte.setValue(m_account_dialog->value());
		compte.setBlocked(m_account_dialog->blockedStatus());

		m_user->addAccount(compte);

		m_account_dialog->resetToDefaults();

		createWidgetForAccount(compte);
		updateBilanTab();
		saveFile();
	}
}

auto MainWindow::editWallet() -> void
{
	m_monnaie_dialog->show();

	if (m_monnaie_dialog->exec() == QDialog::Accepted) {
		m_user->setArgentLiquide(m_monnaie_dialog->totalValue());

		updateBilanTab();
		saveFile();
	}
}

auto MainWindow::editPreferences() -> void
{
	m_user_prefs->show();

	if (m_user_prefs->exec() == QDialog::Accepted) {
		auto locale = m_user_prefs->currentLanguageLocale();
		loadLanguage(locale);
	}
}

auto MainWindow::switchTranslator(QTranslator &translator, const QString &filename) -> void
{
	qApp->removeTranslator(&translator);

	if (translator.load(filename)) {
		qApp->installTranslator(&translator);
	}
}

auto MainWindow::loadLanguage(const QString &language) -> void
{
	if (m_current_language != language) {
		m_current_language = language;

		auto locale = QLocale(m_current_language);
		QLocale::setDefault(locale);

		auto filename = QApplication::applicationDirPath().append("/languages/cresus_%1.qm").arg(language);

		switchTranslator(m_translator, filename);
	}
}

/* ************************** Read and write files ************************** */

auto MainWindow::openFile() -> void
{
	const auto &filename = m_user_prefs->filePath();

	if (filename.isEmpty()) {
		return;
	}

	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly)) {
		QMessageBox::information(this, tr("Impossible d'ouvrir le fichier"),
								 file.errorString());
		return;
	}

	QDataStream in(&file);
	in.setVersion(QDataStream::Qt_5_3);
	Utilisateur user;
	in >> user;

	file.close();

	if (m_user != nullptr) {
		delete m_user;
	}

	m_user = new Utilisateur(user);
	ui->m_user_info->setName(m_user->name());

	for (const auto &compte : m_user->accounts()) {
		createWidgetForAccount(compte);
	}

	updateBilanTab();
}

auto MainWindow::saveFile() -> void
{
	const auto &filename = m_user_prefs->filePath();

	if (filename.isEmpty()) {
		return;
	}

	QFile file(filename);

	if (!file.open(QIODevice::WriteOnly)) {
		QMessageBox::information(this, tr("Impossible d'ouvrir le fichier"),
								 file.errorString());
		return;
	}

	QDataStream out(&file);
	out.setVersion(QDataStream::Qt_5_3);
	out << *m_user;
}

auto MainWindow::readSettings() -> void
{
	QSettings settings;

	auto filename = settings.value("filepath").toString();
	m_user_prefs->setFilePath(filename);
	auto language_index = settings.value("language").toInt();
	m_user_prefs->setCurrentLanguageIndex(language_index);
}

auto MainWindow::writeSettings() -> void
{
	QSettings settings;

	settings.setValue("filepath", m_user_prefs->filePath());
	settings.setValue("language", m_user_prefs->currentLanguageIndex());
}

/* ******************************** Update UI ******************************* */

auto MainWindow::updateBilanTab() -> void
{
	auto cash = m_user->getArgentLiquide();
	ui->m_cash_amount->setValue(cash);
	m_transaction_dialog->setCash(cash);
	ui->m_user_info->setValue(m_user->netValue());
}

auto MainWindow::updateBilanWidget(const QString &name, const float value) -> void
{
	for (auto widget : m_account_widgets) {
		if (widget->name() == name) {
			widget->setValue(value);
			break;
		}
	}
}

auto MainWindow::updateUserTables(QDate date, int mois, int annee, float montant, QString category) -> void
{
	if (!m_database.isOpen()) {
		qDebug() << "La base de données n'est pas ouverte !";
		return;
	}

	QSqlQuery query;

	auto str = QString("insert into kevin_expense_table values('%1', %2, %3, '%4', %5)");
	auto ret = query.exec(str.arg(date.toString()).arg(mois).arg(annee).arg(category).arg(montant));

	str = "replace into kevin_monthly_expense_table (date, mois, année, catégorie, valeur)"
          "values('%1', %2, %3, '%4', (select sum(valeur) from kevin_expense_table where mois=%2 and année=%3 and catégorie='%4'))";
	ret &= query.exec(str.arg(date.toString("MM.yyyy")).arg(mois).arg(annee).arg(category));

	str = "replace into kevin_yearly_expense_table (année, catégorie, valeur)"
          "values (%1, '%2', (select sum(valeur) from kevin_monthly_expense_table where année=%1 and catégorie='%2'))";
	ret &= query.exec(str.arg(annee).arg(category));

	if (!ret) {
		qDebug() << query.lastError();
	}
}

auto MainWindow::updateUserTables() -> void
{
	if (!m_database.isOpen()) {
		qDebug() << "La base de données n'est pas ouverte !";
		return;
	}

	auto annee = 2015;

	/* update revenue table */
	updateUserTables("kevin_revenue_table", REVENUE_ACTIF, REVENUE_PASSIF, annee);
	/* update expense table */
	updateUserTables("kevin_monthly_expense_table", DEPENSE_PERS, DEPENSE_TRSPRT, annee);
}

void MainWindow::updateUserTables(QString table_name, int begin, int end, int annee)
{
	QSqlQuery query;

	for (auto type = begin; type <= end; ++type) {
		for (auto mois(1); mois <= 12; ++mois) {
			auto str = QString("select valeur, catégorie from %1 where mois=%2 and année=%3");
			auto ret = query.exec(str.arg(table_name).arg(mois).arg(annee));

			if (!ret) {
				qDebug() << query.lastError();
			}

			while (query.next()) {
				auto value = query.value(0).toFloat();
				auto category = query.value(1).toString();

				updateTableWidget(category, value, mois, type);
			}
		}
	}
}

auto MainWindow::updateTableWidget(const QString &category, const float value, const int mois, int type) -> void
{
	auto container = m_personal_expenses;

	switch (type) {
		case REVENUE_ACTIF:
			container = m_active_revenues;
			break;
		case REVENUE_PASSIF:
			container = m_passive_revenues;
			break;
		case DEPENSE_PERS:
			break;
		case DEPENSE_PROF:
			/* TODO */
			break;
		case DEPENSE_LGMT:
			container = m_housing_expenses;
			break;
		case DEPENSE_TRSPRT:
			container = m_transport_expenses;
			break;
		default:
			break;
	}

	for (auto widget : container) {
		if (widget->category() == category) {
			widget->setValue(value, mois);
			break;
		}
	}
}
