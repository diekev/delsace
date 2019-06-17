

#include <QLabel>
#include <QPushButton>
#include <QTabBar>
#include <QTabWidget>

class GestionnaireOnglet : public QTabWidget {
	QPushButton *m_bouton_ajout;

public:
	explicit GestionnaireOnglet(QWidget *parent = nullptr)
	    : QTabWidget(parent)
	{
		this->setTabsClosable(true);
		this->setMovable(true);

		m_bouton_ajout = new QPushButton("+", this);
		m_bouton_ajout->setFixedSize(20, 20);

		connect(this, &GestionnaireOnglet::tabCloseRequested, this, &GestionnaireOnglet::enleve_onglet);
		connect(m_bouton_ajout, &QPushButton::clicked, this, &GestionnaireOnglet::ajoute_onglet_ex);
	}

	void ajoute_onglet(QWidget *widget, const QString &texte)
	{
		int index = this->addTab(widget, texte);
		this->ajourne_position_bouton();
		this->setCurrentIndex(index);
	}

	void ajourne_position_bouton()
	{
		const auto taille_barre = this->tabBar()->sizeHint();
		const auto hauteur = taille_barre.height();
		const auto largeur = taille_barre.width() + 6;
		const auto h = (hauteur - 20) / 2;
		const auto w = this->width();

		if (largeur > w) {
			m_bouton_ajout->move(w - 54, h);
		}
		else {
			m_bouton_ajout->move(largeur, h);
		}
	}

	void resizeEvent(QResizeEvent *e) override
	{
		QTabWidget::resizeEvent(e);
		this->ajourne_position_bouton();
	}

private Q_SLOTS:
	void enleve_onglet(int index)
	{
		this->removeTab(index);
		this->ajourne_position_bouton();

		if (this->tabBar()->count() == 0) {
			QApplication::exit(0);
		}
	}

	void ajoute_onglet_ex()
	{
		this->ajoute_onglet(new QWidget(), "Nouvel onglet");
		this->ajourne_position_bouton();
	}
};
