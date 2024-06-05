/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "commun.hh"

#include <QHBoxLayout>
#include <QPushButton>

#include "../fournisseuse_icones.hh"

namespace danjo {

QPushButton *crée_bouton_animation_controle(QWidget *parent)
{
    auto résultat = new QPushButton("", parent);

    auto &fournisseuse = donne_fournisseuse_icone();
    auto icone = fournisseuse.icone_pour_bouton_animation(ÉtatIcône::ACTIF);

    // À FAIRE : état animé vs. non-animé
    if (icone.has_value()) {
        résultat->setIcon(icone.value());
    }
    else {
        auto metriques = parent->fontMetrics();
        résultat->setFixedWidth(metriques.horizontalAdvance("C") * 2);
        résultat->setText("C");
    }

    résultat->setToolTip("Active ou désactive l'animation de la propriété");
    return résultat;
}

void définis_état_bouton_animation(QPushButton *bouton, bool est_animé)
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

    auto &fournisseuse = donne_fournisseuse_icone();
    auto icone = fournisseuse.icone_pour_echelle_valeur(ÉtatIcône::ACTIF);

    // À FAIRE : état animé vs. non-animé
    if (icone.has_value()) {
        résultat->setIcon(icone.value());
    }
    else {
        auto metriques = parent->fontMetrics();
        résultat->setFixedWidth(metriques.horizontalAdvance("H") * 2);
        résultat->setText("H");
    }

    résultat->setToolTip("Affiche une échelle de valeur pour éditer la valeur de la propriété");
    return résultat;
}

static void définis_marges(QLayout *layout)
{
    layout->setContentsMargins(0, 0, 0, 0);
}

QHBoxLayout *crée_hbox_layout(QWidget *parent)
{
    auto résultat = new QHBoxLayout(parent);
    définis_marges(résultat);
    return résultat;
}

QVBoxLayout *crée_vbox_layout(QWidget *parent)
{
    auto résultat = new QVBoxLayout(parent);
    définis_marges(résultat);
    return résultat;
}

}  // namespace danjo
