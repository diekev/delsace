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

#include "dialogs.h"

#include <QFileDialog>

#include "ui_account_dialog.h"
#include "ui_transaction_dialog.h"
#include "ui_monnaie_dialog.h"
#include "ui_retrait_dialog.h"
#include "ui_user_preference.h"

/* **************************** Withdrawal Dialog *************************** */

RetraitDialog::RetraitDialog(QWidget *parent)
    : QDialog(parent)
	, ui(new Ui::RetraitDialog)
{
	ui->setupUi(this);
	ui->m_date_edit->setDate(QDate::currentDate());
}

RetraitDialog::~RetraitDialog()
{
	delete ui;
}

auto RetraitDialog::accountIndex() const -> int
{
	return ui->m_comptes_box->currentIndex();
}

auto RetraitDialog::montant() const -> float
{
	return ui->m_montant->value();
}

auto RetraitDialog::addCompteItem(const QString &name) -> void
{
	ui->m_comptes_box->addItem(name);
}

auto RetraitDialog::date() const -> QDate
{
	return ui->m_date_edit->date();
}

auto RetraitDialog::resetToDefaults() -> void
{
	ui->m_montant->setValue(0.0);
	ui->m_comptes_box->setCurrentIndex(0);
}

/* ****************************** Income Dialog ***************************** */

TransactionDialog::TransactionDialog(QWidget *parent)
    : QDialog(parent)
	, ui(new Ui::TransactionDialog)
{
	ui->setupUi(this);
	ui->m_date_edit->setDate(QDate::currentDate());
	ui->m_error_label->setStyleSheet("QLabel { color : red; }");

	setCash(0.0f);

	/* those are added in such order so that they are aligned with enum defined
	 * in dialogs.h */
	ui->m_pay_box->addItem(tr("Liquide"));
	ui->m_pay_box->addItem(tr("Chèque"));
	ui->m_pay_box->addItem(tr("Virement"));

	m_is_income = false;
}

TransactionDialog::~TransactionDialog()
{
	delete ui;
}

auto TransactionDialog::setCash(const float cash) -> void
{
	m_cash = cash;
}

auto TransactionDialog::accountIndex() const -> int
{
	return ui->m_comptes_box->currentIndex();
}

auto TransactionDialog::addCompteItem(const QString &name) -> void
{
	ui->m_comptes_box->addItem(name);
}

auto TransactionDialog::typeItem(const bool income) const -> int
{
	if (income) {
		return ui->m_income_box->itemData(ui->m_income_box->currentIndex()).toInt();
	}

	return ui->m_outcome_box->itemData(ui->m_outcome_box->currentIndex()).toInt();
}

auto TransactionDialog::category(const bool income) const -> QString
{
	if (income) {
		return ui->m_income_box->itemText(ui->m_income_box->currentIndex());
	}

	return ui->m_outcome_box->itemText(ui->m_outcome_box->currentIndex());
}

auto TransactionDialog::addTypeItem(const QString &name, const QVariant data, const bool income) -> void
{
	if (income) {
		ui->m_income_box->addItem(name, data);
	}
	else {
		ui->m_outcome_box->addItem(name, data);
	}
}

auto TransactionDialog::mediumItem() const -> int
{
	return ui->m_pay_box->currentIndex();
}

auto TransactionDialog::montant() const -> float
{
	return ui->m_montant->value();
}

auto TransactionDialog::setTransactionUI(const bool income) -> void
{
	m_is_income = income;

	if (income) {
		setWindowTitle(tr("Gain"));
		ui->m_income_box->show();
		ui->m_income_label->show();
		ui->m_outcome_box->hide();
		ui->m_outcome_label->hide();
	}
	else {
		setWindowTitle(tr("Dépense"));
		ui->m_income_box->hide();
		ui->m_income_label->hide();
		ui->m_outcome_box->show();
		ui->m_outcome_label->show();
	}
}

auto TransactionDialog::resetToDefaults() -> void
{
	ui->m_pay_box->setCurrentIndex(0);
	ui->m_income_box->setCurrentIndex(0);
	ui->m_outcome_box->setCurrentIndex(0);
	ui->m_comptes_box->setCurrentIndex(0);
	ui->m_comptes_box->setEnabled(false);
	ui->m_montant->setValue(0.0);
}

void TransactionDialog::enableCompteBox(const int index)
{
	auto enable = (index == TRANSACTION_LIQUID) ? false : true;
	ui->label_4->setEnabled(enable);
	ui->m_comptes_box->setEnabled(enable);
}

auto TransactionDialog::date() const -> QDate
{
	return ui->m_date_edit->date();
}

auto TransactionDialog::onAcceptButtonClicked() -> void
{
	if (!m_is_income && (mediumItem() == TRANSACTION_LIQUID) && (m_cash - ui->m_montant->value()) < 0.0f) {
		ui->m_error_label->setText(tr("Montant incorrecte : vous n'avez pas assez de liquides !"));
		ui->m_montant->setFocus();
	}
	else {
		Q_EMIT accept();
	}
}

auto TransactionDialog::onSpinBoxEdit() -> void
{
	ui->m_error_label->clear();
}

/* ****************************** Cash Dialog ******************************* */

MonnaieDialog::MonnaieDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::MonnaieDialog)
{
	ui->setupUi(this);
	m_total_value = 0.0;
}

MonnaieDialog::~MonnaieDialog()
{
	delete ui;
}

#define SET_MONEY_VALUE(money, value)										 \
	void MonnaieDialog::set ## money ## Value(const int v)					 \
	{																		 \
		m_ ## money ## _value = value * v;									 \
		ui->m_ ## money ## _label->setText(QLocale().toCurrencyString(m_ ## money ## _value)); \
		setTotalValue();													 \
	}

SET_MONEY_VALUE(1c, 0.01)
SET_MONEY_VALUE(2c, 0.02)
SET_MONEY_VALUE(5c, 0.05)
SET_MONEY_VALUE(10c, 0.1)
SET_MONEY_VALUE(20c, 0.2)
SET_MONEY_VALUE(50c, 0.5)
SET_MONEY_VALUE(1e, 1.0)
SET_MONEY_VALUE(2e, 2.0)
SET_MONEY_VALUE(5e, 5.0)
SET_MONEY_VALUE(10e, 10.0)
SET_MONEY_VALUE(20e, 20.0)
SET_MONEY_VALUE(50e, 50.0)
SET_MONEY_VALUE(100e, 100.0)
SET_MONEY_VALUE(200e, 200.0)
SET_MONEY_VALUE(500e, 500.0)

#undef SET_MONEY_VALUE

void MonnaieDialog::setTotalValue()
{
	m_total_value = m_1c_value + m_2c_value + m_5c_value + m_10c_value +
			m_20c_value + m_50c_value + m_1e_value + m_2e_value + m_5e_value + m_10e_value +
			m_20e_value + m_50e_value + m_100e_value + m_200e_value + m_500e_value;
	ui->m_total_value->setText(QLocale().toCurrencyString(m_total_value));
}

auto MonnaieDialog::totalValue() const -> float
{
	return m_total_value;
}

/* ***************************** Account Dialog ***************************** */

AccountDialog::AccountDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::AccountDialog)
{
	ui->setupUi(this);

	ui->m_combo_box->addItem(tr("Compte Courant"));
	ui->m_combo_box->addItem(tr("Compte Épargne"));
	ui->m_error_label->setStyleSheet("QLabel { color : red; }");
}

AccountDialog::~AccountDialog()
{
	delete ui;
}

auto AccountDialog::name() const -> QString
{
	return ui->m_line_edit->text();
}

auto AccountDialog::type() const -> int
{
	return ui->m_combo_box->currentIndex();
}

auto AccountDialog::value() const -> float
{
	return ui->m_value_box->value();
}

auto AccountDialog::blockedStatus() const -> bool
{
	return ui->m_is_blocked->isChecked();
}

auto AccountDialog::resetToDefaults() -> void
{
	ui->m_line_edit->clear();
	ui->m_value_box->setValue(0.0);
	ui->m_combo_box->setCurrentIndex(0);
	ui->m_is_blocked->setChecked(false);
}

auto AccountDialog::onAcceptButtonClicked() -> void
{
	if (ui->m_line_edit->text().isEmpty()) {
		ui->m_error_label->setText(tr("Le compte n'a pas de nom !"));
		ui->m_line_edit->setFocus();
	}
	else {
		Q_EMIT accept();
	}
}

auto AccountDialog::onTextEditChange() -> void
{
	ui->m_error_label->clear();
}

/* **************************** User Preferences **************************** */

UserPreferences::UserPreferences(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::UserPreferences)
{
	ui->setupUi(this);
	createLanguageMenu();
}

UserPreferences::~UserPreferences()
{
	delete ui;
}

void UserPreferences::setFilePath()
{
	const auto &filename = QFileDialog::getOpenFileName(this, tr("Sauvegarder"), "",
														tr("Fichier Crésus (*.crs) ;;Tous les fichiers (*)"));

	ui->m_file_path->setText(filename);
}

auto UserPreferences::filePath() const -> QString
{
	return ui->m_file_path->text();
}

auto UserPreferences::setFilePath(const QString &path) -> void
{
	ui->m_file_path->setText(path);
}

auto UserPreferences::currentLanguageLocale() const -> QString
{
	return ui->m_language_box->itemData(ui->m_language_box->currentIndex()).toString();
}

auto UserPreferences::currentLanguageIndex() const -> int
{
	return ui->m_language_box->currentIndex();
}

auto UserPreferences::setCurrentLanguageIndex(const int index) -> void
{
	ui->m_language_box->setCurrentIndex(index);
}

auto UserPreferences::createLanguageMenu() -> void
{
	/* format system language: e.g. de_DE -> de */
	auto default_locale = QLocale::system().name();
	default_locale.truncate(default_locale.lastIndexOf('_'));

	m_language_path = QApplication::applicationDirPath();
	m_language_path.append("/languages");

	QDir language_dir(m_language_path);
	auto language_list = language_dir.entryList({"cresus_*.qm"});

	auto index(0);

	for (auto language : language_list) {
		language.truncate(language.lastIndexOf('.')); // "cresus_ja"
		language.remove(0, language.indexOf('_') + 1); // "ja"

		auto name = QLocale::languageToString(QLocale(language).language());

		ui->m_language_box->addItem(name, QVariant(language));

		if (default_locale == language) {
			ui->m_language_box->setCurrentIndex(index);
		}

		index++;
	}
}
