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
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QFile>
#pragma GCC diagnostic pop

#include <optional>

#include "entreface/fenetre_principale.h"

struct OptionThème {
    QString clé{};
    QString valeur{};
};

static const OptionThème options_theme[] = {
    {"$COULEUR_ARRIERE_PLAN_WIDGET$", "rgb(127, 127, 127)"},
    {"$COULEUR_ARRIERE_PLAN_CONTROLE$", "rgb(40, 40, 40)"},
    {"$COULEUR_SPLITTER$", "rgb(105, 105, 105)"},
    {"$COULEUR_TEXTE$", "rgb(240, 240, 240)"},
    {"$COULEUR_BORDURE$", "rgb(164, 164, 164)"},
    {"$COULEUR_BORDURE_EDITRICE_ACTIVE$", "#52b1ee"},
    {"$MARGE_CONTROLE$", "5px"},
    /* Bouton (QPushButton). */
    {"$COULEUR_BOUTON$", "rgb(150, 150, 150)"},
    {"$COULEUR_BOUTON_PRESSE$", "rgb(90, 90, 90)"},
    {"$COULEUR_BOUTON_SURVOL$", "rgb(160, 160, 160)"},
    {"$RAYON_BORDURE_BOUTON$", "5px"},
    {"$TAILLE_SPLITTER$", "3px"},
};

static std::optional<QString> donne_feuille_de_style()
{
    QFile file("styles/main.qss");
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }

    QString feuille_de_style = file.readAll();
    file.close();

    for (auto const &option_theme : options_theme) {
        feuille_de_style.replace(option_theme.clé, option_theme.valeur);
    }

    return feuille_de_style;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("giraffeenfeu");
    QCoreApplication::setApplicationName("kanba");

    const auto opt_feuille_de_style = donne_feuille_de_style();
    if (opt_feuille_de_style.has_value()) {
        qApp->setStyleSheet(opt_feuille_de_style.value());
    }

    FenetrePrincipale w;
    w.setWindowTitle(QCoreApplication::applicationName());
    w.showMaximized();

    return a.exec();
}
