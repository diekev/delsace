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

#include "vue_editrice_graphe.h"

#include "../editrice_noeud.h"

/* ************************************************************************** */

VueEditeurNoeud::VueEditeurNoeud(EditriceGraphe *base, QWidget *parent)
    : QGraphicsView(parent), m_base(base)
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

VueEditeurNoeud::~VueEditeurNoeud()
{
}

void VueEditeurNoeud::keyPressEvent(QKeyEvent *event)
{
    m_base->keyPressEvent(event);
}

void VueEditeurNoeud::wheelEvent(QWheelEvent *event)
{
    m_base->wheelEvent(event);
}

void VueEditeurNoeud::mouseMoveEvent(QMouseEvent *event)
{
    m_base->mouseMoveEvent(event);
}

void VueEditeurNoeud::mousePressEvent(QMouseEvent *event)
{
    m_base->mousePressEvent(event);
}

void VueEditeurNoeud::mouseDoubleClickEvent(QMouseEvent *event)
{
    m_base->mouseDoubleClickEvent(event);
}

void VueEditeurNoeud::mouseReleaseEvent(QMouseEvent *event)
{
    m_base->mouseReleaseEvent(event);
}

bool VueEditeurNoeud::focusNextPrevChild(bool /*next*/)
{
    /* Pour pouvoir utiliser la touche tab, il faut désactiver la focalisation
     * sur les éléments enfants du conteneur de contrôles. */
    return false;
}
