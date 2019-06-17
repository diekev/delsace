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

	QLabel *m_text, *m_value;
	QHBoxLayout *m_layout;
	QPushButton *m_action;

	/* dialog */
	QDialog *m_dialog;
	QVBoxLayout *m_layout_dialog;
	QLineEdit *m_value_edit;
	QDialogButtonBox *m_button_box;

private Q_SLOTS:
	void editValue();

Q_SIGNALS:
	void valueChanged(const QString &text, const float value);

public:
	explicit AccountWidget(QWidget *parent = nullptr);
	AccountWidget(const QString &text, const float value, QWidget *parent = nullptr);
	~AccountWidget() = default;

	auto name() const -> QString;
	auto setName(const QString &name) const -> void;
	auto setValue(const float value) const -> void;
	auto setSize(const int size) const -> void;
	auto setBold(const bool do_name_bold, const bool do_value_bold) const -> void;
	auto showPushButton(const bool b) const -> void;
};

/* A MonthlyTableWidget holds the monthly values of a given category. */
class MonthlyTableWidget final : public QWidget {
	Q_OBJECT

	QLabel *m_category;
	QLabel *m_values[12];
	QHBoxLayout *m_layout;

public:
	explicit MonthlyTableWidget(QWidget *parent = nullptr);
	MonthlyTableWidget(const QString &category, QWidget *parent = nullptr);
	~MonthlyTableWidget() = default;

	auto category() const -> QString;
	auto setValue(const float value, const int mois) const -> void;
	void setCategory(const QString &category) const;
};

/* A TableWidgetGroup holds several MonthlyTableWidget. */
class TableWidgetGroup final : public QWidget {
	Q_OBJECT

	QGroupBox *m_group_box;
	QVBoxLayout *m_layout, *m_box_layout;
	QPushButton *m_action;

	/* dialog */
	QDialog *m_dialog;
	QVBoxLayout *m_layout_dialog;
	QLineEdit *m_value_edit;
	QDialogButtonBox *m_button_box;

private Q_SLOTS:
	void addCategory();

Q_SIGNALS:
	void monthlyTableAdded(const QString &category);

public:
	explicit TableWidgetGroup(QWidget *parent = nullptr);
	TableWidgetGroup(const QString &category, QWidget *parent = nullptr);
	~TableWidgetGroup() = default;

	void setTitle(const QString &title) const;
};
