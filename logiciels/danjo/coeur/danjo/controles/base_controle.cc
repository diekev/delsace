/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "base_controle.hh"

#include <QMouseEvent>

void BaseControle::keyPressEvent(QKeyEvent *event)
{
    if (!m_en_édition_texte) {
        return;
    }

    auto état = gère_entrée_clavier(event);

    switch (état) {
        case RéponseÉvènement::IGNORE_ÉVÈNEMENT:
        case RéponseÉvènement::ENTRE_EN_ÉDITION:
        case RéponseÉvènement::ENTRE_EN_ÉDITION_TEXTE:
        {
            break;
        }
        case RéponseÉvènement::CONTINUE_ÉDITION_TEXTE:
        {
            if (m_premier_changement) {
                Q_EMIT(debute_changement_controle());
                m_premier_changement = false;
            }
            update();
            event->accept();
            break;
        }
        case RéponseÉvènement::TERMINE_ÉDITION:
        {
            Q_EMIT(termine_changement_controle());
            m_en_édition_texte = false;
            update();
            event->accept();
            break;
        }
    }
}

void BaseControle::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    auto état = gère_clique_souris(event);

    switch (état) {
        case RéponseÉvènement::IGNORE_ÉVÈNEMENT:
        {
            m_souris_pressée = false;
            break;
        }
        case RéponseÉvènement::ENTRE_EN_ÉDITION:
        {
            m_souris_pressée = true;
            m_premier_changement = true;
            m_en_édition_du_à_clique_souris = true;
            m_vieil_x = event->pos().x();
            event->accept();
            update();
            break;
        }
        case RéponseÉvènement::ENTRE_EN_ÉDITION_TEXTE:
        case RéponseÉvènement::CONTINUE_ÉDITION_TEXTE:
        case RéponseÉvènement::TERMINE_ÉDITION:
        {
            m_souris_pressée = false;
            break;
        }
    }
}

void BaseControle::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_souris_pressée) {
        return;
    }

    if (m_en_édition_du_à_clique_souris) {
        if (m_premier_changement) {
            Q_EMIT(debute_changement_controle());
            m_premier_changement = false;
        }
    }

    gère_mouvement_souris(event);

    if (m_en_édition_du_à_clique_souris) {
        Q_EMIT(valeur_changee());
        update();
    }

    event->accept();
}

void BaseControle::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_souris_pressée) {
        /* Le clique ne vient pas de nous, ou nous l'avons ignoré. */
        return;
    }

    gère_fin_clique_souris(event);
    update();

    /* Si m_premier_changement est vrai, nous avons cliquer sans bouger la souris, n'envoyons pas
     * de signal. */
    if (m_en_édition_du_à_clique_souris && !m_premier_changement) {
        Q_EMIT(termine_changement_controle());
    }

    m_souris_pressée = false;
    m_en_édition_du_à_clique_souris = false;
    m_premier_changement = false;

    event->accept();
}

void BaseControle::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }

    auto état = gère_double_clique_souris(event);
    switch (état) {
        case RéponseÉvènement::IGNORE_ÉVÈNEMENT:
        case RéponseÉvènement::TERMINE_ÉDITION:
        case RéponseÉvènement::ENTRE_EN_ÉDITION:
        case RéponseÉvènement::CONTINUE_ÉDITION_TEXTE:
        {
            m_souris_pressée = false;
            break;
        }
        case RéponseÉvènement::ENTRE_EN_ÉDITION_TEXTE:
        {
            m_en_édition_texte = true;
            m_premier_changement = true;
            event->accept();
            setFocus();
            update();
            break;
        }
    }
}
