/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "fenetre_principale.hh"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QCloseEvent>
#include <QCoreApplication>
#include <QMenuBar>
#include <QStack>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

namespace Impl {

class QT_Creatrice_Barre_Menu : public ::QT_Creatrice_Barre_Menu {
    FenetrePrincipale *m_fenêtre_principale{};
    QStack<QMenu *> m_menus{};

    static void commence_menu_impl(::QT_Creatrice_Barre_Menu *créatrice, QT_Chaine *nom)
    {
        auto impl = static_cast<QT_Creatrice_Barre_Menu *>(créatrice);
        auto barre_de_menu = impl->m_fenêtre_principale->menuBar();

        if (impl->m_menus.empty()) {
            auto menu = barre_de_menu->addMenu(nom->vers_std_string().c_str());
            impl->m_menus.push(menu);
        }
        else {
            auto menu = impl->m_menus.top();
            auto sous_menu = menu->addMenu(nom->vers_std_string().c_str());
            impl->m_menus.push(sous_menu);
        }
    }

    static void ajoute_action_impl(::QT_Creatrice_Barre_Menu *créatrice, QT_Action *données)
    {
        auto impl = static_cast<QT_Creatrice_Barre_Menu *>(créatrice);
        if (impl->m_menus.empty()) {
            return;
        }

        auto menu = impl->m_menus.top();
        auto action = reinterpret_cast<QAction *>(données);
        menu->addAction(action);

        auto fenêtre = impl->m_fenêtre_principale;
        fenêtre->connect(action, SIGNAL(triggered(bool)), fenêtre, SLOT(repond_clic_menu()));
    }

    static void ajoute_action_chaine_impl(::QT_Creatrice_Barre_Menu *créatrice,
                                          QT_Chaine *nom,
                                          QT_Chaine *données)
    {
        auto impl = static_cast<QT_Creatrice_Barre_Menu *>(créatrice);
        if (impl->m_menus.empty()) {
            return;
        }

        auto menu = impl->m_menus.top();
        auto action = menu->addAction(nom->vers_std_string().c_str());
        action->setData(données->vers_std_string().c_str());

        auto fenêtre = impl->m_fenêtre_principale;
        fenêtre->connect(action, SIGNAL(triggered(bool)), fenêtre, SLOT(repond_clic_menu()));
    }

    static void ajoute_separateur_impl(::QT_Creatrice_Barre_Menu *créatrice)
    {
        auto impl = static_cast<QT_Creatrice_Barre_Menu *>(créatrice);
        if (impl->m_menus.empty()) {
            return;
        }

        auto menu = impl->m_menus.top();
        menu->addSeparator();
    }

    static void ajoute_section_impl(::QT_Creatrice_Barre_Menu *créatrice, QT_Chaine *nom)
    {
        auto impl = static_cast<QT_Creatrice_Barre_Menu *>(créatrice);
        if (impl->m_menus.empty()) {
            return;
        }

        auto menu = impl->m_menus.top();
        menu->addSection(nom->vers_std_string().c_str());
    }

    static void termine_menu_impl(::QT_Creatrice_Barre_Menu *créatrice)
    {
        auto impl = static_cast<QT_Creatrice_Barre_Menu *>(créatrice);
        if (impl->m_menus.empty()) {
            return;
        }
        impl->m_menus.pop();
    }

  public:
    QT_Creatrice_Barre_Menu(FenetrePrincipale *fenêtre_principale)
        : m_fenêtre_principale(fenêtre_principale)
    {
        commence_menu = commence_menu_impl;
        ajoute_action = ajoute_action_impl;
        ajoute_action_chaine = ajoute_action_chaine_impl;
        ajoute_separateur = ajoute_separateur_impl;
        ajoute_section = ajoute_section_impl;
        termine_menu = termine_menu_impl;
    }

    EMPECHE_COPIE(QT_Creatrice_Barre_Menu);
};

}  // namespace Impl

FenetrePrincipale::FenetrePrincipale(QT_Rappels_Fenetre_Principale *rappels)
    : QMainWindow(), m_rappels(rappels)
{
    m_rappels->fenetre = reinterpret_cast<QT_Fenetre_Principale *>(this);

    construit_barre_de_menu();

    if (m_rappels->sur_filtre_evenement) {
        qApp->installEventFilter(this);
    }
}

bool FenetrePrincipale::eventFilter(QObject *object, QEvent *event)
{
    if (m_rappels->sur_filtre_evenement) {
        if (m_rappels->sur_filtre_evenement(m_rappels, reinterpret_cast<QT_Evenement *>(event))) {
            return true;
        }
    }

    return QWidget::eventFilter(object, event);
}

void FenetrePrincipale::closeEvent(QCloseEvent *event)
{
    if (!m_rappels->sur_fermeture || m_rappels->sur_fermeture(m_rappels)) {
        event->accept();
        return;
    }
    event->ignore();
}

void FenetrePrincipale::construit_barre_de_menu()
{
    if (!m_rappels->sur_creation_barre_menu) {
        return;
    }

    menuBar()->clear();

    Impl::QT_Creatrice_Barre_Menu créatrice_menu(this);
    m_rappels->sur_creation_barre_menu(m_rappels, &créatrice_menu);
}

void FenetrePrincipale::repond_clic_menu()
{
    auto action = qobject_cast<QAction *>(sender());
    if (!action) {
        return;
    }

    if (!m_rappels->sur_clic_action_menu) {
        return;
    }

    auto std_string = action->data().toString().toStdString();
    QT_Chaine données;
    données.caractères = std_string.data();
    données.taille = int64_t(std_string.size());

    m_rappels->sur_clic_action_menu(m_rappels, &données);
}
