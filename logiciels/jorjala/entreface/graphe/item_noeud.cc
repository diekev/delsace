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

#include "item_noeud.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QFont>
#include <QPen>
#pragma GCC diagnostic pop

#include "biblinternes/structures/tableau.hh"

#include "coeur/jorjala.hh"

static QBrush brosse_pour_couleur(JJL::CouleurTSL couleur)
{
    auto couleur_qt = QColor::fromHslF(static_cast<double>(couleur.t()),
                                       static_cast<double>(couleur.s()),
                                       static_cast<double>(couleur.l()));
    return QBrush(couleur_qt);
}

static QBrush brosse_pour_noeud(JJL::Noeud noeud)
{
    auto couleur_prise = JJL::couleur_pour_type_noeud(noeud);
    return brosse_pour_couleur(couleur_prise);
}

static void ajourne_rectangle(JJL::Prise *prise, float x, float y, float hauteur, float largeur)
{
    auto rect = prise->rectangle();
    rect.x(x);
    rect.y(y);
    rect.hauteur(hauteur);
    rect.largeur(largeur);
    prise->rectangle(rect);
}

ItemNoeud::ItemNoeud(JJL::Noeud &noeud,
                     bool selectionne,
                     bool est_noeud_detail,
                     QGraphicsItem *parent)
    : QGraphicsRectItem(parent)
{
    if (est_noeud_detail) {
        dessine_noeud_detail(noeud, selectionne);
    }
    else {
        auto brosse_couleur = brosse_pour_noeud(noeud);
        dessine_noeud_generique(noeud, brosse_couleur, selectionne);
    }
}

void ItemNoeud::dessine_noeud_detail(JJL::Noeud &noeud, bool selectionne)
{
    auto const pos_x = static_cast<double>(noeud.pos_x());
    auto const pos_y = static_cast<double>(noeud.pos_y());

    /* crée le texte en premier pour calculer sa taille */
    auto const decalage_texte = 8;
    auto texte = new QGraphicsTextItem(noeud.nom().vers_std_string().c_str(), this);
    auto police = QFont();
    police.setPointSize(16);
    texte->setFont(police);

    auto const largeur_texte = texte->boundingRect().width() + decalage_texte * 2;
    auto const hauteur_texte = texte->boundingRect().height();

    /* crée le texte pour chacune des entrées/sorties */

    auto textes_entrees = dls::tableau<QGraphicsTextItem *>();
    auto textes_sorties = dls::tableau<QGraphicsTextItem *>();

    auto const hauteur_prise = 32.0;
    auto const largeur_prise = 32.0;

    auto largeur_entrees = 0.0;
    auto hauteur_entrees = 0.0;

    auto prises_entrées = noeud.entrées();
    auto prises_sorties = noeud.sorties();

    for (auto prise : prises_entrées) {
        auto texte_prise = new QGraphicsTextItem(prise.nom().vers_std_string().c_str(), this);
        texte_prise->setFont(police);
        textes_entrees.ajoute(texte_prise);

        largeur_entrees = std::max(largeur_entrees, texte_prise->boundingRect().width());
        hauteur_entrees += texte_prise->boundingRect().height();
    }

    if (prises_entrées.taille() != 0) {
        largeur_entrees += largeur_prise;
    }

    auto largeur_sorties = 0.0;
    auto hauteur_sorties = 0.0;

    for (auto prise : prises_sorties) {
        auto texte_prise = new QGraphicsTextItem(prise.nom().vers_std_string().c_str(), this);
        texte_prise->setFont(police);
        textes_sorties.ajoute(texte_prise);

        largeur_sorties = std::max(largeur_sorties, texte_prise->boundingRect().width());
        hauteur_sorties += texte_prise->boundingRect().height();
    }

    if (prises_sorties.taille() != 0) {
        largeur_sorties += largeur_prise;
    }

    auto hauteur_noeud = hauteur_texte + decalage_texte +
                         std::max(hauteur_entrees, hauteur_sorties);
    auto largeur_noeud = std::max(largeur_texte,
                                  largeur_entrees + largeur_sorties + 2 * decalage_texte);

    /* positionne les textes */
    texte->setDefaultTextColor(Qt::white);
    texte->setPos(pos_x + decalage_texte + (largeur_noeud - largeur_texte) / 2, pos_y);

    auto ligne = new QGraphicsLineItem(this);
    ligne->setPen(QPen(Qt::white));
    ligne->setLine(pos_x, pos_y + hauteur_texte, pos_x + largeur_noeud, pos_y + hauteur_texte);

    auto pos_y_entree = hauteur_texte + decalage_texte;

    for (auto i = 0; i < textes_entrees.taille(); ++i) {
        textes_entrees[i]->setDefaultTextColor(Qt::white);
        textes_entrees[i]->setPos(pos_x + largeur_prise + decalage_texte, pos_y + pos_y_entree);

        auto prise = prises_entrées[static_cast<size_t>(i)];
        cree_geometrie_prise(&prise,
                             static_cast<float>(pos_x),
                             static_cast<float>(pos_y + pos_y_entree),
                             hauteur_prise,
                             largeur_prise);

        pos_y_entree += hauteur_texte;
    }

    auto pos_y_sortie = hauteur_texte + decalage_texte;

    for (auto i = 0; i < textes_sorties.taille(); ++i) {
        textes_sorties[i]->setDefaultTextColor(Qt::white);
        textes_sorties[i]->setPos(pos_x + largeur_noeud - decalage_texte -
                                      (textes_sorties[i]->boundingRect().width()) - largeur_prise,
                                  pos_y + pos_y_sortie);

        auto prise = prises_sorties[static_cast<size_t>(i)];
        cree_geometrie_prise(&prise,
                             static_cast<float>(pos_x + largeur_noeud - largeur_prise),
                             static_cast<float>(pos_y + pos_y_sortie),
                             hauteur_prise,
                             largeur_prise);

        pos_y_sortie += hauteur_texte;
    }

    if (selectionne) {
        /* pinceaux pour le contour du noeud */
        auto stylo = QPen(Qt::yellow);
        stylo.setWidthF(1.0);
        setPen(stylo);
    }
    else {
        /* pinceaux pour le contour du noeud */
        auto stylo = QPen(Qt::white);
        stylo.setWidthF(0.5);
        setPen(stylo);
    }

    finalise_dessin(noeud, selectionne, pos_x, pos_y, largeur_noeud, hauteur_noeud);
}

void ItemNoeud::dessine_noeud_generique(JJL::Noeud &noeud,
                                        QBrush const &brosse_couleur,
                                        bool selectionne)
{
    auto const pos_x = static_cast<double>(noeud.pos_x());
    auto const pos_y = static_cast<double>(noeud.pos_y());

    /* crée le texte en premier pour calculer sa taille */
    auto const decalage_texte = 8;
    auto texte = new QGraphicsTextItem(noeud.nom().vers_std_string().c_str(), this);
    auto police = QFont();
    police.setPointSize(16);
    texte->setFont(police);

    auto const largeur_texte = texte->boundingRect().width() + decalage_texte * 2;
    auto const hauteur_texte = texte->boundingRect().height();

    auto hauteur_noeud = 0.0;
    auto largeur_noeud = 0.0;

    auto const hauteur_icone = 64.0;
    auto const largeur_icone = 64.0;

    auto const hauteur_prise = 32.0;
    auto const largeur_prise = 32.0;

    auto const nombre_entrees = noeud.entrées().taille();
    auto const nombre_sorties = noeud.sorties().taille();

    auto decalage_icone_y = pos_y;
    auto decalage_texte_y = pos_y;
    auto decalage_sorties_y = pos_y;

    if (nombre_entrees > 0) {
        hauteur_noeud += hauteur_prise + hauteur_prise * 0.5;
        decalage_icone_y += hauteur_noeud;
        decalage_texte_y += hauteur_noeud;
        decalage_sorties_y += hauteur_noeud;
    }

    if (nombre_sorties > 0) {
        decalage_sorties_y += hauteur_icone + hauteur_prise * 0.5;
        hauteur_noeud += hauteur_prise + hauteur_prise * 0.5;
    }

    largeur_noeud += largeur_icone;
    largeur_noeud += largeur_texte;
    hauteur_noeud += hauteur_icone;

    /* entrées du noeud */
    if (nombre_entrees > 0) {
        auto const etendue_entree = (largeur_noeud / static_cast<double>(nombre_entrees));
        auto const pos_debut_entrees = etendue_entree * 0.5 - largeur_prise * 0.5;
        auto pos_entree = pos_x + pos_debut_entrees;

        for (auto prise : noeud.entrées()) {
            auto largeur_lien = largeur_prise;

            // À FAIRE : connexions multiples
            //			if (prise->multiple_connexions) {
            //				pos_entree -= largeur_prise * 0.5;
            //				largeur_lien *= 2.0;
            //			}

            cree_geometrie_prise(&prise,
                                 static_cast<float>(pos_entree),
                                 static_cast<float>(pos_y),
                                 hauteur_prise,
                                 static_cast<float>(largeur_lien));

            pos_entree += etendue_entree;
        }

        auto ligne = new QGraphicsLineItem(this);
        ligne->setPen(QPen(Qt::white));
        ligne->setLine(pos_x, decalage_icone_y, pos_x + largeur_noeud, decalage_icone_y);
    }

    /* icone */
    auto icone = new QGraphicsRectItem(this);
    icone->setRect(pos_x + 1, decalage_icone_y + 1, largeur_icone - 2, hauteur_icone - 2);
    icone->setBrush(brosse_couleur);
    icone->setPen(QPen(QColor(0, 0, 0, 0), 0.0));

    /* nom du noeud */
    texte->setDefaultTextColor(Qt::white);
    texte->setPos(pos_x + largeur_icone + decalage_texte,
                  decalage_texte_y + (hauteur_icone - hauteur_texte) / 2);

    /* sorties du noeud */
    if (nombre_sorties > 0) {
        auto ligne = new QGraphicsLineItem(this);
        ligne->setPen(QPen(Qt::white));
        ligne->setLine(pos_x,
                       decalage_icone_y + hauteur_icone,
                       pos_x + largeur_noeud,
                       decalage_icone_y + hauteur_icone);

        auto const etendue_sortie = (largeur_noeud / static_cast<double>(nombre_sorties));
        auto const pos_debut_sorties = etendue_sortie * 0.5 - largeur_prise * 0.5;
        auto pos_sortie = pos_x + pos_debut_sorties;

        for (auto prise : noeud.sorties()) {
            cree_geometrie_prise(&prise,
                                 static_cast<float>(pos_sortie),
                                 static_cast<float>(decalage_sorties_y),
                                 largeur_prise,
                                 hauteur_prise);
            pos_sortie += etendue_sortie;
        }
    }

    finalise_dessin(noeud, selectionne, pos_x, pos_y, largeur_noeud, hauteur_noeud);
}

void ItemNoeud::finalise_dessin(JJL::Noeud &noeud,
                                bool selectionne,
                                double pos_x,
                                double pos_y,
                                double largeur_noeud,
                                double hauteur_noeud)
{
    if (selectionne) {
        //			auto cadre = new QGraphicsRectItem(this);
        //			cadre->setRect(pos_x - 5, pos_y - 5, largeur_noeud + 5, hauteur_noeud + 5);
        //			cadre->setBrush(QBrush(QColor(128, 128, 128)));
        //			cadre->setPen(QPen(Qt::white, 0.0f));

        /* pinceaux pour le contour du noeud */
        auto stylo = QPen(Qt::yellow);
        stylo.setWidthF(1.0);
        setPen(stylo);
    }
    else {
        /* pinceaux pour le contour du noeud */
        auto stylo = QPen(Qt::white);
        stylo.setWidthF(0.5);
        setPen(stylo);
    }

    /* pinceaux pour le coeur du noeud */
    QBrush brosse;

    auto erreurs = noeud.donne_erreurs();

    if (erreurs.taille() == 0) {
        brosse = QBrush(QColor(45, 45, 45));
    }
    else {
        brosse = QBrush(QColor(255, 180, 10));
    }

    setBrush(brosse);

    setRect(pos_x, pos_y, largeur_noeud, hauteur_noeud);

    noeud.largeur(static_cast<float>(largeur_noeud));
    noeud.hauteur(static_cast<float>(hauteur_noeud));
}

void ItemNoeud::cree_geometrie_prise(
    JJL::Prise *prise, float x, float y, float hauteur, float largeur)
{
    auto item_prise = new QGraphicsRectItem(this);
    item_prise->setRect(static_cast<double>(x),
                        static_cast<double>(y),
                        static_cast<double>(largeur),
                        static_cast<double>(hauteur));
    item_prise->setBrush(brosse_pour_couleur(prise->donne_description().couleur()));
    item_prise->setPen(QPen(Qt::white, 0.5));

    ajourne_rectangle(prise, x, y, hauteur, largeur);
}
