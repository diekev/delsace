#pragma once

#include <QDialog>

namespace Ui {
class UserPreferences;
}

class UserPreferences : public QDialog {
	Q_OBJECT

	Ui::UserPreferences *ui;

public:
	explicit UserPreferences(QWidget *parent = 0);

	auto getRandomMode() const -> bool;
	auto setRandomMode(const bool b) -> void;
	auto getDiaporamatime() const -> int;
	auto setDiaporamatime(const int time) -> void;
};
