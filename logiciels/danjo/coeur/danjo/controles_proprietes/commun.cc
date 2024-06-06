/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "commun.hh"

#include <QHBoxLayout>
#include <QPushButton>
#include <QStyle>

#include "../fournisseuse_icones.hh"

namespace danjo {

static const char *donne_texte_pour_icône(const IcônePourBouton icône)
{
    switch (icône) {
        case IcônePourBouton::AJOUTE:
        {
            return "Ajoute";
        }
        case IcônePourBouton::AJOUTE_IMAGE_CLÉ:
        {
            return "C";
        }
        case IcônePourBouton::CHOISIR_FICHIER:
        {
            return "Choisir Fichier";
        }
        case IcônePourBouton::DÉPLACE_EN_HAUT:
        {
            return "Monte";
        }
        case IcônePourBouton::DÉPLACE_EN_BAS:
        {
            return "Déscend";
        }
        case IcônePourBouton::ÉCHELLE_VALEUR:
        {
            return "H";
        }
        case IcônePourBouton::LISTE_CHAINE:
        {
            return "Liste";
        }
        case IcônePourBouton::RAFRAICHIS_TEXTE:
        {
            return "Rafraichis";
        }
        case IcônePourBouton::SUPPRIME:
        {
            return "Supprime";
        }
    }
}

static const char *donne_infobulle_pour_icône(const IcônePourBouton icône)
{
    switch (icône) {
        case IcônePourBouton::AJOUTE:
        {
            return "Ajoute un élément";
        }
        case IcônePourBouton::AJOUTE_IMAGE_CLÉ:
        {
            return "Active ou désactive l'animation de la propriété";
        }
        case IcônePourBouton::CHOISIR_FICHIER:
        {
            return "Ouvre un dialogue pour choisir le chemin";
        }
        case IcônePourBouton::DÉPLACE_EN_HAUT:
        {
            return "Déplace l'élément vers le haut";
        }
        case IcônePourBouton::DÉPLACE_EN_BAS:
        {
            return "Déplace l'élément vers le bas";
        }
        case IcônePourBouton::ÉCHELLE_VALEUR:
        {
            return "Affiche une échelle de valeur pour éditer la valeur de la propriété";
        }
        case IcônePourBouton::LISTE_CHAINE:
        {
            return "Montre la liste de chaines";
        }
        case IcônePourBouton::RAFRAICHIS_TEXTE:
        {
            return "Rafraichis";
        }
        case IcônePourBouton::SUPPRIME:
        {
            return "Supprime l'élément";
        }
    }
}

static std::optional<QIcon> donne_icone_pour_bouton(const IcônePourBouton icône, QStyle *style)
{
    auto &fournisseuse = donne_fournisseuse_icone();
    auto résultat = fournisseuse.icone_pour_bouton(icône, ÉtatIcône::ACTIF);
    if (résultat.has_value()) {
        return résultat.value();
    }

    switch (icône) {
        case IcônePourBouton::AJOUTE:
        {
            return {};
        }
        case IcônePourBouton::AJOUTE_IMAGE_CLÉ:
        {
            return {};
        }
        case IcônePourBouton::CHOISIR_FICHIER:
        {
            return style->standardIcon(QStyle::SP_DirOpenIcon);
        }
        case IcônePourBouton::DÉPLACE_EN_HAUT:
        {
            return style->standardIcon(QStyle::SP_ArrowUp);
        }
        case IcônePourBouton::DÉPLACE_EN_BAS:
        {
            return style->standardIcon(QStyle::SP_ArrowDown);
        }
        case IcônePourBouton::ÉCHELLE_VALEUR:
        {
            return {};
        }
        case IcônePourBouton::LISTE_CHAINE:
        {
            return style->standardIcon(QStyle::SP_TitleBarUnshadeButton);
        }
        case IcônePourBouton::RAFRAICHIS_TEXTE:
        {
            return style->standardIcon(QStyle::SP_BrowserReload);
        }
        case IcônePourBouton::SUPPRIME:
        {
            return style->standardIcon(QStyle::SP_TrashIcon);
        }
    }

    return {};
}

QPushButton *crée_bouton_animation_controle(QWidget *parent)
{
    auto résultat = new QPushButton("", parent);

    auto icone = donne_icone_pour_bouton(IcônePourBouton::AJOUTE_IMAGE_CLÉ, parent->style());

    // À FAIRE : état animé vs. non-animé
    if (icone.has_value()) {
        résultat->setIcon(icone.value());
    }
    else {
        auto metriques = parent->fontMetrics();
        résultat->setFixedWidth(metriques.horizontalAdvance("C") * 2);
        résultat->setText("C");
    }

    résultat->setToolTip(donne_infobulle_pour_icône(IcônePourBouton::AJOUTE_IMAGE_CLÉ));
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

    auto icone = donne_icone_pour_bouton(IcônePourBouton::ÉCHELLE_VALEUR, parent->style());

    // À FAIRE : état animé vs. non-animé
    if (icone.has_value()) {
        résultat->setIcon(icone.value());
    }
    else {
        auto metriques = parent->fontMetrics();
        résultat->setFixedWidth(metriques.horizontalAdvance("H") * 2);
        résultat->setText("H");
    }

    résultat->setToolTip(donne_infobulle_pour_icône(IcônePourBouton::ÉCHELLE_VALEUR));
    return résultat;
}

QPushButton *crée_bouton(const IcônePourBouton icône, QWidget *parent)
{
    auto résultat = new QPushButton("", parent);

    auto icone = donne_icone_pour_bouton(icône, parent->style());

    if (icone.has_value()) {
        résultat->setIcon(icone.value());
    }
    else {
        résultat->setText(donne_texte_pour_icône(icône));
    }

    résultat->setToolTip(donne_infobulle_pour_icône(icône));
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
