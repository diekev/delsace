#include "ui_pref_window.h"
#include "user_preferences.h"

UserPreferences::UserPreferences(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::UserPreferences)
{
	ui->setupUi(this);
}

auto UserPreferences::getRandomMode() const -> bool
{
	return ui->m_random_mode->isChecked();
}

auto UserPreferences::setRandomMode(const bool b) -> void
{
	ui->m_random_mode->setChecked(b);
}

auto UserPreferences::getDiaporamatime() const -> int
{
	return ui->m_diap_dur->value();
}

auto UserPreferences::setDiaporamatime(const int time) -> void
{
	ui->m_diap_dur->setValue(time);
}
