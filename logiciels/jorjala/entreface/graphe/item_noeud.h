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

#pragma once

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QGraphicsRectItem>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

namespace JJL {
class Noeud;
class Prise;
}  // namespace JJL
class OperatriceImage;

class ItemNoeud : public QGraphicsRectItem {
  public:
    explicit ItemNoeud(JJL::Noeud &noeud,
                       bool selectionne,
                       bool est_noeud_detail,
                       QGraphicsItem *parent = nullptr);

    void dessine_noeud_detail(JJL::Noeud &noeud, bool selectionne);

    void dessine_noeud_generique(JJL::Noeud &noeud,
                                 QBrush const &brosse_couleur,
                                 bool selectionne);

  private:
    void finalise_dessin(JJL::Noeud &noeud,
                         bool selectionne,
                         double pos_x,
                         double pos_y,
                         double largeur_noeud,
                         double hauteur_noeud);

    void cree_geometrie_prise(JJL::Prise *prise, float x, float y, float hauteur, float largeur);
};
