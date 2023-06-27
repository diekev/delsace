/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "base_editrice.h"

class QComboBox;
class QLabel;
class QTableWidget;

class EditriceAttributs : public BaseEditrice {
    Q_OBJECT

    QTableWidget *m_table = nullptr;
    QLabel *m_label_pour_noeud_manquant = nullptr;
    QComboBox *m_sélecteur_domaine = nullptr;
    int m_domaine = 0;

  public:
    explicit EditriceAttributs(JJL::Jorjala &jorjala, QWidget *parent = nullptr);

    EditriceAttributs(EditriceAttributs const &) = delete;
    EditriceAttributs &operator=(EditriceAttributs const &) = delete;

    void ajourne_état(JJL::TypeEvenement évènement) override;

    void ajourne_manipulable() override{};

  private:
    /** Si \a est_visible est vrai, la table est montré, sinon la table est cachée et un message
     * indiquant qu'aucun noeud n'est actif est affiché. */
    void définit_visibilité_table(bool est_visible);

  private Q_SLOTS:
    void ajourne_pour_changement_domaine(int);
};
