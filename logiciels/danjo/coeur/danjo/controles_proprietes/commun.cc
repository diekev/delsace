/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "commun.hh"

#include <QPushButton>

namespace danjo {

QPushButton *crée_bouton_animation_controle(QWidget *parent)
{
    auto résultat = new QPushButton("", parent);

    // À FAIRE : état animé vs. non-animé
    auto metriques = parent->fontMetrics();
    résultat->setFixedWidth(metriques.horizontalAdvance("C") * 2);
    résultat->setText("C");

    résultat->setToolTip("Active ou désactive l'animation de la propriété");
    return résultat;
}

void définit_état_bouton_animation(QPushButton *bouton, bool est_animé)
{
    if (est_animé) {
        bouton->setText("c");
    }
    else {
        bouton->setText("C");
    }
}

QPushButton *crée_bouton_échelle_valeur(QWidget *parent)
{
    auto résultat = new QPushButton("", parent);

    auto metriques = parent->fontMetrics();
    résultat->setFixedWidth(metriques.horizontalAdvance("H") * 2);
    résultat->setText("H");

    résultat->setToolTip("Affiche une échelle de valeur pour éditeur la valeur de la propriété");
    return résultat;
}

}  // namespace danjo
