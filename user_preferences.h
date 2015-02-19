#ifndef USER_PREFERENCES_H
#define USER_PREFERENCES_H

#include <QDialog>

class MainWindow;

namespace Ui {
class UserPreferences;
}

class UserPreferences : public QDialog {
	Q_OBJECT

	class MainWindow &m_main_win;
	Ui::UserPreferences *ui;

public:
	explicit UserPreferences(class MainWindow &main_win);
};

#endif // USER_PREFERENCES_H
