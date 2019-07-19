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

#include <QWidget>

class QDialogButtonBox;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QVBoxLayout;
class QPushButton;

/* Widget used to  display a given's account name and balance. It is also used to
 * display the user total balance and cash amount. */
class AccountWidget final : public QWidget {
	Q_OBJECT

	QLabel *m_text = nullptr;
	QLabel *m_value = nullptr;
	QHBoxLayout *m_layout = nullptr;
	QPushButton *m_action = nullptr;

	/* dialog */
	QDialog *m_dialog = nullptr;
	QVBoxLayout *m_layout_dialog = nullptr;
	QLineEdit *m_value_edit = nullptr;
	QDialogButtonBox *m_button_box = nullptr;

private Q_SLOTS:
	void editValue();

Q_SIGNALS:
	void valueChanged(const QString &text, const double value);

public:
	explicit AccountWidget(QWidget *parent = nullptr);
	AccountWidget(const QString &text, const double value, QWidget *parent = nullptr);
	~AccountWidget() = default;

	AccountWidget(AccountWidget const &) = default;
	AccountWidget &operator=(AccountWidget const &) = default;

	auto name() const -> QString;
	auto setName(const QString &name) const -> void;
	auto setValue(const double value) const -> void;
	auto setSize(const int size) const -> void;
	auto setBold(const bool do_name_bold, const bool do_value_bold) const -> void;
	auto showPushButton(const bool b) const -> void;
};

/* A MonthlyTableWidget holds the monthly values of a given category. */
class MonthlyTableWidget final : public QWidget {
	Q_OBJECT

	QLabel *m_category = nullptr;
	QLabel *m_values[12] = {};
	QHBoxLayout *m_layout = nullptr;

public:
	explicit MonthlyTableWidget(QWidget *parent = nullptr);
	MonthlyTableWidget(const QString &category, QWidget *parent = nullptr);
	~MonthlyTableWidget() = default;

	MonthlyTableWidget(MonthlyTableWidget const &) = default;
	MonthlyTableWidget &operator=(MonthlyTableWidget const &) = default;

	auto category() const -> QString;
	auto setValue(const double value, const int mois) const -> void;
	void setCategory(const QString &category) const;
};

/* A TableWidgetGroup holds several MonthlyTableWidget. */
class TableWidgetGroup final : public QWidget {
	Q_OBJECT

	QGroupBox *m_group_box = nullptr;
	QVBoxLayout *m_layout = nullptr;
	QVBoxLayout *m_box_layout = nullptr;
	QPushButton *m_action = nullptr;

	/* dialog */
	QDialog *m_dialog = nullptr;
	QVBoxLayout *m_layout_dialog = nullptr;
	QLineEdit *m_value_edit = nullptr;
	QDialogButtonBox *m_button_box = nullptr;

private Q_SLOTS:
	void addCategory();

Q_SIGNALS:
	void monthlyTableAdded(const QString &category);

public:
	explicit TableWidgetGroup(QWidget *parent = nullptr);
	TableWidgetGroup(const QString &category, QWidget *parent = nullptr);
	~TableWidgetGroup() = default;

	TableWidgetGroup(TableWidgetGroup const &) = default;
	TableWidgetGroup &operator=(TableWidgetGroup const &) = default;

	void setTitle(const QString &title) const;
};
