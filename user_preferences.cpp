#include "mainwindow.h"
#include "ui_pref_window.h"
#include "user_preferences.h"

UserPreferences::UserPreferences(MainWindow &main_win)
	: QDialog(&main_win)
	, m_main_win(main_win)
	, ui(new Ui::UserPreferences)
{
	ui->setupUi(this);

//	connect(ui->m_random_mode, SIGNAL(toggled(bool)), &m_main_win, SLOT(setRandomize(bool)));
//	connect(ui->m_diap_dur, SIGNAL(valueChanged(int)), &m_main_win, SLOT(setDiapTime(int)));
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
