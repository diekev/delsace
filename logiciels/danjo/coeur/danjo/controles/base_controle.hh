/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include <QWidget>

// À FAIRE : ControleCouleur dialogue

enum class RéponseÉvènement {
    IGNORE_ÉVÈNEMENT,
    ENTRE_EN_ÉDITION,
    ENTRE_EN_ÉDITION_TEXTE,
    CONTINUE_ÉDITION_TEXTE,
    TERMINE_ÉDITION,
};

/**
 * Classe de base pour tous les controles.
 *
 * Cette classe capture les QEvents et dispatche aux classes dérivées des fonctions de gestion des
 * évènements à réimplémenter au besoin afin d'abstraire l'état d'édition des controles.
 *
 * Les cliques de souris ne sont gérer que s'ils proviennent d'un clique gauche.
 *
 * Les évènements de claviers ne sont gérer que si le controle réponds à un évènement de clique
 * avec #RéponseÉvènement::ENTRE_EN_ÉDITION_TEXTE.
 */
class BaseControle : public QWidget {
    Q_OBJECT

  protected:
    /* Données sur la souris. */
    bool m_souris_pressée = false;
    int m_vieil_x = 0;
    int m_vieil_y = 0;

    /* Vrai si le changement en mode édition est le premier. */
    bool m_premier_changement = false;
    bool m_en_édition_du_à_clique_souris = false;
    bool m_en_édition_texte = false;

  public:
    BaseControle(QWidget *parent = nullptr) : QWidget(parent)
    {
    }

    void keyPressEvent(QKeyEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseDoubleClickEvent(QMouseEvent *event) override;

    // À FAIRE : wheelEvent
    // void wheelEvent(QWheelEvent *event) override;

    /* Les classes dérivées doivent réimplémenter ces fonctions pour gérer les évènements. */

    virtual RéponseÉvènement gère_entrée_clavier(QKeyEvent *)
    {
        return RéponseÉvènement::IGNORE_ÉVÈNEMENT;
    }

    virtual RéponseÉvènement gère_clique_souris(QMouseEvent *)
    {
        return RéponseÉvènement::IGNORE_ÉVÈNEMENT;
    }

    virtual RéponseÉvènement gère_double_clique_souris(QMouseEvent *)
    {
        return RéponseÉvènement::IGNORE_ÉVÈNEMENT;
    }

    /* Uniquement appelé si un clique simple est accepté. */
    virtual void gère_mouvement_souris(QMouseEvent *)
    {
    }

    virtual void gère_fin_clique_souris(QMouseEvent *)
    {
    }

  Q_SIGNALS:
    /** Signal envoyé quand l'édition d'un controle commence. */
    void debute_changement_controle();

    /** Signal envoyé lors de l'ajournement interractif d'un controle. */
    void valeur_changee();

    /** Signal envoyé quand l'édition d'un controle est terminée. */
    void termine_changement_controle();
};
