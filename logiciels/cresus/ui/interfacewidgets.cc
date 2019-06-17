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

#include "interfacewidgets.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QVBoxLayout>

/* ************************** Balance Sheet Widget ************************** */

AccountWidget::AccountWidget(QWidget *parent)
	: AccountWidget("", 0.0f, parent)
{}

AccountWidget::AccountWidget(const QString &text, const float value, QWidget *parent)
	: QWidget(parent)
    , m_text(new QLabel(text, this))
    , m_value(new QLabel(this))
    , m_layout(new QHBoxLayout(this))
    , m_action(new QPushButton(tr("Modifier"), this))
    , m_dialog(new QDialog(this))
    , m_layout_dialog(new QVBoxLayout(m_dialog))
    , m_value_edit(new QLineEdit(m_dialog))
    , m_button_box(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, m_dialog))
{
	m_value->setText(QLocale().toCurrencyString(value));
	m_value->setAlignment(Qt::AlignRight);

	m_action->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	m_layout->addWidget(m_text);
	m_layout->addWidget(m_value);
	m_layout->addWidget(m_action);

	setLayout(m_layout);

	/* Setup dialog. */
	m_layout_dialog->addWidget(m_value_edit);
	m_layout_dialog->addWidget(m_button_box);
	m_dialog->setLayout(m_layout_dialog);

	connect(m_action, SIGNAL(clicked()), this, SLOT(editValue()));
	connect(m_button_box, SIGNAL(accepted()), m_dialog, SLOT(accept()));
	connect(m_button_box, SIGNAL(rejected()), m_dialog, SLOT(reject()));
}

auto AccountWidget::name() const -> QString
{
	return m_text->text();
}

auto AccountWidget::setName(const QString &name) const -> void
{
	m_text->setText(name);
}

auto AccountWidget::setValue(const float value) const -> void
{
	m_value->setText(QLocale().toCurrencyString(value));

	auto stylesheet = QString{"QLabel { color : "};
	stylesheet += ((value < 0.0f) ? "red; }" : "black; }");

	m_value->setStyleSheet(stylesheet);
}

auto AccountWidget::setBold(const bool do_name_bold, const bool do_value_bold) const -> void
{
	auto name_font = m_text->font();
	auto value_font = m_value->font();

	name_font.setBold(do_name_bold);
	value_font.setBold(do_value_bold);

	m_text->setFont(name_font);
	m_value->setFont(value_font);
}

auto AccountWidget::setSize(const int size) const -> void
{
	auto name_font = m_text->font();
	auto value_font = m_value->font();

	name_font.setPointSize(size);
	value_font.setPointSize(size);

	m_text->setFont(name_font);
	m_value->setFont(value_font);
}

auto AccountWidget::showPushButton(const bool b) const -> void
{
	m_action->setHidden(!b);
}

void AccountWidget::editValue()
{
	m_value_edit->clear();
	m_value_edit->setPlaceholderText(m_value->text());
	m_dialog->show();

	if (m_dialog->exec() == QDialog::Accepted) {
		const float value = m_value_edit->text().toFloat();
		m_value->setText(QLocale().toCurrencyString(value));
		Q_EMIT valueChanged(m_text->text(), value);
	}
}

/* ************************** Expense Sheet Widget ************************** */

MonthlyTableWidget::MonthlyTableWidget(QWidget *parent)
	: MonthlyTableWidget("", parent)
{}

MonthlyTableWidget::MonthlyTableWidget(const QString &category, QWidget *parent)
	: QWidget(parent)
    , m_category(new QLabel(category, this))
    , m_layout(new QHBoxLayout(this))
{
	m_category->setFixedWidth(100);
	m_layout->addWidget(m_category);

	for (auto i(0); i < 12; ++i) {
		m_values[i] = new QLabel("-", this);
		m_values[i]->setAlignment(Qt::AlignCenter);
		m_layout->addWidget(m_values[i]);
	}

	setLayout(m_layout);
}

auto MonthlyTableWidget::category() const -> QString
{
	return m_category->text();
}

auto MonthlyTableWidget::setCategory(const QString &category) const -> void
{
	m_category->setText(category);
}

auto MonthlyTableWidget::setValue(const float value, const int mois) const -> void
{
	m_values[mois - 1]->setText(QLocale().toCurrencyString(value));
}


/* ************************ Expense Sheet Group Widget *********************** */

TableWidgetGroup::TableWidgetGroup(QWidget *parent)
    : TableWidgetGroup("", parent)
{}

TableWidgetGroup::TableWidgetGroup(const QString &category, QWidget *parent)
    : QWidget(parent)
    , m_group_box(new QGroupBox(category, this))
    , m_layout(new QVBoxLayout(this))
    , m_box_layout(new QVBoxLayout(m_group_box))
    , m_action(new QPushButton(tr("Ajouter Catégorie"), this))
    , m_dialog(new QDialog(this))
    , m_layout_dialog(new QVBoxLayout(m_dialog))
    , m_value_edit(new QLineEdit(m_dialog))
    , m_button_box(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, m_dialog))
{
	m_action->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_layout->addWidget(m_group_box);
	m_layout->addWidget(m_action);
	m_layout->setAlignment(m_action, Qt::AlignRight);

	setLayout(m_layout);

	/* Setup dialog. */
	m_value_edit->setPlaceholderText(tr("Catégorie"));

	m_layout_dialog->addWidget(m_value_edit);
	m_layout_dialog->addWidget(m_button_box);
	m_dialog->setLayout(m_layout_dialog);

	connect(m_action, SIGNAL(clicked()), this, SLOT(addCategory()));
	connect(m_button_box, SIGNAL(accepted()), m_dialog, SLOT(accept()));
	connect(m_button_box, SIGNAL(rejected()), m_dialog, SLOT(reject()));
}

void TableWidgetGroup::addCategory()
{
	m_value_edit->clear();

	if (m_dialog->exec() == QDialog::Accepted) {
		const auto &category = m_value_edit->text();
		MonthlyTableWidget *widget = new MonthlyTableWidget(category, this);
		m_box_layout->addWidget(widget);
		Q_EMIT monthlyTableAdded(category);
	}
}

void TableWidgetGroup::setTitle(const QString &title) const
{
	m_group_box->setTitle(title);
}
