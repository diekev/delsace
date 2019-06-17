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

#include <QDialog>

namespace Ui {
class AccountDialog;
class MonnaieDialog;
class RetraitDialog;
class TransactionDialog;
class UserPreferences;
}

enum {
	TRANSACTION_LIQUID = 0,
	TRANSACTION_CHECK  = 1,
	TRANSACTION_WIRE   = 2,
};

class RetraitDialog : public QDialog {
	Q_OBJECT

	Ui::RetraitDialog *ui;

public:
	explicit RetraitDialog(QWidget *parent = nullptr);
	~RetraitDialog();

	auto accountIndex() const -> int;
	auto montant() const -> float;
	auto addCompteItem(const QString &name) -> void;
	auto date() const -> QDate;
	auto resetToDefaults() -> void;
};

class TransactionDialog : public QDialog {
	Q_OBJECT

	Ui::TransactionDialog *ui;

	/* This is to be in sync with Utilisateur::m_argent_liquide!
	 * It is used to check if we do not end up with negative cash values */
	float m_cash;
	bool m_is_income;

public Q_SLOTS:
	void onAcceptButtonClicked();
	void onSpinBoxEdit();
	void enableCompteBox(const int index);

public:
	explicit TransactionDialog(QWidget *parent = nullptr);
	~TransactionDialog();

	auto setCash(const float cash) -> void;

	auto accountIndex() const -> int;
	auto addCompteItem(const QString &name) -> void;
	auto typeItem(const bool income) const -> int;
	auto addTypeItem(const QString &name, const QVariant data, const bool income) -> void;
	auto category(const bool income) const -> QString;
	auto mediumItem() const -> int;
	auto montant() const -> float;
	auto resetToDefaults() -> void;
	auto setTransactionUI(const bool income) -> void;
	auto date() const -> QDate;
};

/* TODO: consider removing all this, maybe let the user input a single value? */
class MonnaieDialog : public QDialog {
	Q_OBJECT

	Ui::MonnaieDialog *ui;
	double m_total_value, m_1c_value, m_2c_value, m_5c_value, m_10c_value,
	m_20c_value, m_50c_value, m_1e_value, m_2e_value, m_5e_value, m_10e_value,
	m_20e_value, m_50e_value, m_100e_value, m_200e_value, m_500e_value;

public Q_SLOTS:
	void set1cValue(const int v);
	void set2cValue(const int v);
	void set5cValue(const int v);
	void set10cValue(const int v);
	void set20cValue(const int v);
	void set50cValue(const int v);
	void set1eValue(const int v);
	void set2eValue(const int v);
	void set5eValue(const int v);
	void set10eValue(const int v);
	void set20eValue(const int v);
	void set50eValue(const int v);
	void set100eValue(const int v);
	void set200eValue(const int v);
	void set500eValue(const int v);
	void setTotalValue();

public:
	explicit MonnaieDialog(QWidget *parent = nullptr);
	~MonnaieDialog();

	auto totalValue() const -> float;
};

class AccountDialog : public QDialog {
	Q_OBJECT

	Ui::AccountDialog *ui;

private Q_SLOTS:
	void onAcceptButtonClicked();
	void onTextEditChange();

public:
	explicit AccountDialog(QWidget *parent = nullptr);
	~AccountDialog();

	auto name() const -> QString;
	auto type() const -> int;
	auto value() const -> float;
	auto resetToDefaults() -> void;
	auto blockedStatus() const -> bool;
};

/* TODO:
 * - tabs
 * - database path
 * - translation
 */
class UserPreferences : public QDialog {
	Q_OBJECT

	Ui::UserPreferences *ui;
	QString m_language_path;

private Q_SLOTS:
	void setFilePath();

public:
	explicit UserPreferences(QWidget *parent = nullptr);
	~UserPreferences();

	auto filePath() const -> QString;
	auto setFilePath(const QString &path) -> void;
	auto createLanguageMenu() -> void;
	auto currentLanguageLocale() const -> QString;
	auto currentLanguageIndex() const -> int;
	auto setCurrentLanguageIndex(const int index) -> void;
};
