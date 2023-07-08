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

#include "controle_echelle_valeur.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <iostream>

#include "controle_nombre_decimal.h"
#include "controle_nombre_entier.h"

/* ************************************************************************** */

static constexpr auto NOMBRE_PALIER = 8;

static const float PALIERS[NOMBRE_PALIER] = {
    1000.0f,
    100.0f,
    10.0f,
    1.0f,
    0.1f,
    0.01f,
    0.001f,
    0.0001f,
};

ControleEchelleDecimale::ControleEchelleDecimale(ControleNombreDecimal *controle_modifié,
                                                 QPushButton *bouton_affichage,
                                                 QWidget *parent)
    : BaseControle(parent)
{
    m_controle_modifié = controle_modifié;

    if (bouton_affichage) {
        connect(
            bouton_affichage, &QPushButton::pressed, this, &ControleEchelleDecimale::montre_toi);
    }

    auto metriques = this->fontMetrics();
    setFixedHeight(metriques.height() * 3 * NOMBRE_PALIER);
    setFixedWidth(
        static_cast<int>(static_cast<float>(metriques.horizontalAdvance("1000.0")) * 1.2f));
    setWindowFlags(Qt::WindowStaysOnTopHint);
}

void ControleEchelleDecimale::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    auto rectangle = this->rect();

    /* Couleur d'arrière plan. */
    QBrush pinceaux(QColor(40, 40, 40));
    painter.fillRect(rectangle, pinceaux);

    /* dessine les séparation entre paliers */
    auto hauteur_palier = rectangle.height() / NOMBRE_PALIER;
    auto largeur_palier = rectangle.width();

    QPen stylo = QPen(Qt::white);
    painter.setPen(stylo);

    for (int i = 1; i < NOMBRE_PALIER; ++i) {
        painter.drawLine(0, hauteur_palier * i, largeur_palier, hauteur_palier * i);
    }

    /* dessine les nombres */
    for (int i = 0; i < NOMBRE_PALIER; ++i) {
        if (m_souris_pressée && i == m_case) {
            painter.drawText(QRectF(0.1 * largeur_palier,
                                    (0.083 + i) * hauteur_palier,
                                    1.0 * largeur_palier,
                                    (0.083 + (i + 1)) * hauteur_palier),
                             Qt::AlignLeft,
                             QString::number(static_cast<double>(PALIERS[i])));

            painter.drawText(QRectF(0.1 * largeur_palier,
                                    (0.583 + i) * hauteur_palier,
                                    1.0 * largeur_palier,
                                    (0.583 + (i + 1)) * hauteur_palier),
                             Qt::AlignLeft,
                             QString::number(static_cast<double>(m_valeur)));
        }
        else {
            painter.drawText(QRectF(0.1 * largeur_palier,
                                    (1.0 / 3.0 + i) * hauteur_palier,
                                    1.0 * largeur_palier,
                                    (1.0 / 3.0 + (i + 1)) * hauteur_palier),
                             Qt::AlignLeft,
                             QString::number(static_cast<double>(PALIERS[i])));
        }
    }
}

RéponseÉvènement ControleEchelleDecimale::gère_clique_souris(QMouseEvent *event)
{
    const auto y = event->pos().y();

    auto rectangle = this->rect();
    auto hauteur_palier = rectangle.height() / NOMBRE_PALIER;
    m_case = y / hauteur_palier;

    QApplication::setOverrideCursor(Qt::SplitHCursor);

    return RéponseÉvènement::ENTRE_EN_ÉDITION;
}

void ControleEchelleDecimale::gère_mouvement_souris(QMouseEvent *event)
{
    const auto x = event->pos().x();
    m_valeur += static_cast<float>(x - m_vieil_x) * PALIERS[m_case];
    m_vieil_x = x;

    m_valeur = std::max(std::min(m_valeur, m_max), m_min);
    m_controle_modifié->ajourne_valeur(m_valeur);
}

void ControleEchelleDecimale::gère_fin_clique_souris(QMouseEvent *event)
{
    QApplication::restoreOverrideCursor();
}

void ControleEchelleDecimale::valeur(float v)
{
    m_valeur = v;
}

void ControleEchelleDecimale::plage(float min, float max)
{
    m_min = min;
    m_max = max;
}

void ControleEchelleDecimale::montre_toi()
{
    this->valeur(m_controle_modifié->valeur());
    this->plage(m_controle_modifié->min(), m_controle_modifié->max());
    this->show();
}

/* ************************************************************************** */

static constexpr auto NOMBRE_PALIER_ENTIER = 4;

static const int PALIERS_ENTIER[NOMBRE_PALIER_ENTIER] = {
    1000,
    100,
    10,
    1,
};

ControleEchelleEntiere::ControleEchelleEntiere(ControleNombreEntier *controle_modifié,
                                               QPushButton *bouton_affichage,
                                               QWidget *parent)
    : BaseControle(parent)
{
    m_controle_modifié = controle_modifié;

    if (bouton_affichage) {
        connect(
            bouton_affichage, &QPushButton::pressed, this, &ControleEchelleEntiere::montre_toi);
    }

    auto metriques = this->fontMetrics();
    setFixedHeight(metriques.height() * 3 * NOMBRE_PALIER_ENTIER);
    setFixedWidth(
        static_cast<int>(static_cast<float>(metriques.horizontalAdvance("1000.0")) * 1.2f));
    setWindowFlags(Qt::WindowStaysOnTopHint);
}

void ControleEchelleEntiere::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    auto rectangle = this->rect();

    /* Couleur d'arrière plan. */
    QBrush pinceaux(QColor(40, 40, 40));
    painter.fillRect(rectangle, pinceaux);

    /* dessine les séparation entre paliers */
    auto hauteur_palier = rectangle.height() / NOMBRE_PALIER_ENTIER;
    auto largeur_palier = rectangle.width();

    QPen stylo = QPen(Qt::white);
    painter.setPen(stylo);

    for (int i = 1; i < NOMBRE_PALIER_ENTIER; ++i) {
        painter.drawLine(0, hauteur_palier * i, largeur_palier, hauteur_palier * i);
    }

    /* dessine les nombres */
    for (int i = 0; i < NOMBRE_PALIER_ENTIER; ++i) {
        if (m_souris_pressée && i == m_case) {
            painter.drawText(QRectF(0.1 * largeur_palier,
                                    (0.083 + i) * hauteur_palier,
                                    1.0 * largeur_palier,
                                    (0.083 + (i + 1)) * hauteur_palier),
                             Qt::AlignLeft,
                             QString::number(PALIERS_ENTIER[i]));

            painter.drawText(QRectF(0.1 * largeur_palier,
                                    (0.583 + i) * hauteur_palier,
                                    1.0 * largeur_palier,
                                    (0.583 + (i + 1)) * hauteur_palier),
                             Qt::AlignLeft,
                             QString::number(m_valeur));
        }
        else {
            painter.drawText(QRectF(0.1 * largeur_palier,
                                    (1.0 / 3.0 + i) * hauteur_palier,
                                    1.0 * largeur_palier,
                                    (1.0 / 3.0 + (i + 1)) * hauteur_palier),
                             Qt::AlignLeft,
                             QString::number(PALIERS_ENTIER[i]));
        }
    }
}

RéponseÉvènement ControleEchelleEntiere::gère_clique_souris(QMouseEvent *event)
{
    const auto y = event->pos().y();

    auto rectangle = this->rect();
    auto hauteur_palier = rectangle.height() / NOMBRE_PALIER_ENTIER;
    m_case = y / hauteur_palier;

    QApplication::setOverrideCursor(Qt::SplitHCursor);

    return RéponseÉvènement::ENTRE_EN_ÉDITION;
}

void ControleEchelleEntiere::gère_mouvement_souris(QMouseEvent *event)
{
    const auto x = event->pos().x();
    m_valeur += (x - m_vieil_x) * PALIERS_ENTIER[m_case];
    m_vieil_x = x;

    m_valeur = std::max(std::min(m_valeur, m_max), m_min);
    m_controle_modifié->ajourne_valeur(m_valeur);
}

void ControleEchelleEntiere::gère_fin_clique_souris(QMouseEvent *event)
{
    QApplication::restoreOverrideCursor();
}

void ControleEchelleEntiere::valeur(int v)
{
    m_valeur = v;
}

void ControleEchelleEntiere::plage(int min, int max)
{
    m_min = min;
    m_max = max;
}

void ControleEchelleEntiere::montre_toi()
{
    this->valeur(m_controle_modifié->valeur());
    this->plage(m_controle_modifié->min(), m_controle_modifié->max());
    this->show();
}
