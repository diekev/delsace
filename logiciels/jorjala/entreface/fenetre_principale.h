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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QMainWindow>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

class BarreDeProgres;
class VueRegion;

namespace JJL {
class Jorjala;
}

class QLabel;

class FenetrePrincipale : public QMainWindow {
    Q_OBJECT

    JJL::Jorjala &m_jorjala;

    BarreDeProgres *m_barre_progres = nullptr;
    QLabel *m_texte_état = nullptr;
    QToolBar *m_barre_outil = nullptr;

    QVector<VueRegion *> m_régions{};

  public:
    explicit FenetrePrincipale(JJL::Jorjala &jorjala, QWidget *parent = nullptr);

    FenetrePrincipale(FenetrePrincipale const &) = delete;
    FenetrePrincipale &operator=(FenetrePrincipale const &) = delete;

    void définit_texte_état(const QString &texte);

  public Q_SLOTS:
    void image_traitee();
    void mis_a_jour_menu_fichier_recent();

    void signale_proces(int quoi);

    /* barre de progrès */
    void tache_demarree();
    void ajourne_progres(float progres);
    void tache_terminee();
    void evaluation_debutee(const QString &message, int execution, int total);

    /** Si des changements existe dans la session courante, affiche une boîte de dialogue pour
     * demander à l'utilisateur si les changments doivent être sauvegardés ou non.
     * Retourne faux si l'utilisateur demande d'annuler. Si l'on retourne vrai, nous pouvons
     * continuer de faire ce que nous voulions. */
    bool demande_permission_avant_de_fermer();

  private:
    void genere_barre_menu();
    void genere_menu_prereglages();
    void charge_reglages();
    void ecrit_reglages() const;
    void closeEvent(QCloseEvent *) override;

    void construit_interface_depuis_jorjala();

    bool eventFilter(QObject *, QEvent *) override;
};
