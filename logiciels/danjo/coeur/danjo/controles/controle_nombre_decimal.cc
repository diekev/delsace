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

#include "controle_nombre_decimal.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

static constexpr auto DECALAGE_PIXEL = 4;

ControleNombreDecimal::ControleNombreDecimal(QWidget *parent) : QWidget(parent)
{
    auto metriques = this->fontMetrics();
    setFixedHeight(static_cast<int>(static_cast<float>(metriques.height()) * 1.5f));
}

void ControleNombreDecimal::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    /* Couleur d'arrière plan. */
    if (m_temps_exacte) {  // image courante
        QBrush pinceaux(QColor(31, 46, 158));
        painter.fillRect(this->rect(), pinceaux);
    }
    else if (m_anime) {
        QBrush pinceaux(QColor(18, 91, 18));
        painter.fillRect(this->rect(), pinceaux);
    }
    else {
        QBrush pinceaux(QColor(40, 40, 40));
        painter.fillRect(this->rect(), pinceaux);
    }

    painter.setPen(QColor(255, 255, 255));

    const auto &texte = ((m_edition && m_tampon != "") ?
                             m_tampon :
                             QString::number(static_cast<double>(m_valeur), 'g', 6)) +
                        m_suffixe;

    auto metriques = this->fontMetrics();
    auto rectangle = this->rect();

    painter.drawText(DECALAGE_PIXEL,
                     static_cast<int>(static_cast<float>(metriques.height()) * 0.25f),
                     rectangle.width() - 1,
                     metriques.height(),
                     0,
                     texte);

    metriques = painter.fontMetrics();
    int largeur = metriques.horizontalAdvance(texte);
    int hauteur = metriques.height();

    if (m_edition) {
        painter.drawLine(DECALAGE_PIXEL + largeur,
                         static_cast<int>(static_cast<float>(hauteur) * 0.25f),
                         DECALAGE_PIXEL + largeur,
                         hauteur);
    }

    painter.setPen(QPen(QColor(255, 255, 255), 1, Qt::DotLine));

    painter.drawLine(DECALAGE_PIXEL,
                     static_cast<int>(static_cast<float>(hauteur) * 1.25f),
                     DECALAGE_PIXEL + largeur,
                     static_cast<int>(static_cast<float>(hauteur) * 1.25f));
}

void ControleNombreDecimal::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MouseButton::LeftButton) {
        QApplication::setOverrideCursor(Qt::SplitHCursor);
        m_vieil_x = event->pos().x();
        m_souris_pressee = true;
        m_premier_changement = true;
        event->accept();
        update();
    }
}

void ControleNombreDecimal::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_edition = true;
    m_tampon = "";
    update();
    setFocus();
    event->accept();
}

void ControleNombreDecimal::mouseMoveEvent(QMouseEvent *event)
{
    if (m_souris_pressee) {
        const auto x = event->pos().x();

        auto delta = static_cast<float>(x - m_vieil_x) * m_inv_precision;

        /* protection contre -0.0f */
        if (delta == 0.0f) {
            delta = 0.0f;
        }

        m_valeur += delta;

        /* protection contre -0.0f */
        if (m_valeur == 0.0f) {
            m_valeur = 0.0f;
        }

        m_valeur = std::max(m_min, std::min(m_valeur, m_max));

        m_vieil_x = x;

        if (m_anime) {
            m_temps_exacte = true;
        }

        if (m_premier_changement) {
            Q_EMIT(prevaleur_changee());
            m_premier_changement = false;
        }

        Q_EMIT(valeur_changee(m_valeur));

        update();
        event->accept();
    }
}

void ControleNombreDecimal::mouseReleaseEvent(QMouseEvent *event)
{
    QApplication::restoreOverrideCursor();

    m_souris_pressee = false;
    event->accept();
}

void ControleNombreDecimal::keyPressEvent(QKeyEvent *event)
{
    if (m_edition == false) {
        return;
    }

    event->accept();

    switch (event->key()) {
        case Qt::Key_Minus:
            if (m_tampon.isEmpty()) {
                m_tampon.append('-');
            }
            break;
        case Qt::Key_0:
            m_tampon.append('0');
            break;
        case Qt::Key_1:
            m_tampon.append('1');
            break;
        case Qt::Key_2:
            m_tampon.append('2');
            break;
        case Qt::Key_3:
            m_tampon.append('3');
            break;
        case Qt::Key_4:
            m_tampon.append('4');
            break;
        case Qt::Key_5:
            m_tampon.append('5');
            break;
        case Qt::Key_6:
            m_tampon.append('6');
            break;
        case Qt::Key_7:
            m_tampon.append('7');
            break;
        case Qt::Key_8:
            m_tampon.append('8');
            break;
        case Qt::Key_9:
            m_tampon.append('9');
            break;
        case Qt::Key_Period:
            m_tampon.append('.');
            break;
        case Qt::Key_Backspace:
            m_tampon.resize(std::max(0, m_tampon.size() - 1));
            break;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            if (m_tampon != "") {
                m_valeur = m_tampon.toFloat();
                m_valeur = std::max(m_min, std::min(m_valeur, m_max));

                if (m_anime) {
                    m_temps_exacte = true;
                }
                Q_EMIT(prevaleur_changee());
                Q_EMIT(valeur_changee(m_valeur));
            }

            m_edition = false;
            break;
    }

    update();
}

void ControleNombreDecimal::marque_anime(bool ouinon, bool temps_exacte)
{
    m_anime = ouinon;
    m_temps_exacte = temps_exacte;
    update();
}

void ControleNombreDecimal::ajourne_plage(float min, float max)
{
    m_min = min;
    m_max = max;
}

float ControleNombreDecimal::valeur() const
{
    return m_valeur;
}

void ControleNombreDecimal::valeur(const float v)
{
    m_valeur = v;
    update();
}

void ControleNombreDecimal::suffixe(const QString &s)
{
    m_suffixe = s;
}

float ControleNombreDecimal::min() const
{
    return m_min;
}

float ControleNombreDecimal::max() const
{
    return m_max;
}

void ControleNombreDecimal::ajourne_valeur(float valeur)
{
    m_valeur = std::max(m_min, std::min(valeur, m_max));

    if (m_anime) {
        m_temps_exacte = true;
    }
    update();
    Q_EMIT(valeur_changee(m_valeur));
}
