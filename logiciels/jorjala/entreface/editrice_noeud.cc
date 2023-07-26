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
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "editrice_noeud.h"

#include "danjo/danjo.h"

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <QComboBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolTip>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include "biblinternes/outils/definitions.h"
#include "biblinternes/patrons_conception/repondant_commande.h"
#include "biblinternes/structures/flux_chaine.hh"

#include "graphe/item_noeud.h"
#include "graphe/vue_editrice_graphe.h"

#include "coeur/jorjala.hh"

#include "gestion_entreface.hh"

EditriceGraphe::EditriceGraphe(JJL::Jorjala &jorjala, QWidget *parent)
    : BaseEditrice("graphe", jorjala, parent), m_scene(new QGraphicsScene(this)),
      m_vue(new VueEditeurNoeud(this, this)), m_barre_chemin(new QLineEdit()),
      m_selecteur_graphe(new QComboBox(this))
{
    m_vue->setScene(m_scene);

    auto disposition_vert = new QVBoxLayout();
    auto disposition_barre = new QHBoxLayout();

    ajourne_sélecteur_graphe();

    connect(
        m_selecteur_graphe, SIGNAL(currentIndexChanged(int)), this, SLOT(change_contexte(int)));

    disposition_barre->addWidget(m_selecteur_graphe);
    disposition_barre->addWidget(m_barre_chemin);

    auto bouton_retour = new QPushButton("^");
    disposition_barre->addWidget(bouton_retour);

    connect(bouton_retour, &QPushButton::clicked, this, &EditriceGraphe::sors_noeud);

    disposition_vert->addLayout(disposition_barre);
    disposition_vert->addWidget(m_vue);

    m_main_layout->addLayout(disposition_vert);
}

EditriceGraphe::~EditriceGraphe()
{
    delete m_scene;
}

void EditriceGraphe::ajourne_état(JJL::TypeÉvènement évènement)
{
    auto creation = ((évènement & JJL::TypeÉvènement::NOEUD) == JJL::TypeÉvènement::NOEUD);
    creation |= (évènement == (JJL::TypeÉvènement::IMAGE | JJL::TypeÉvènement::TRAITÉ));
    creation |= (évènement == (JJL::TypeÉvènement::OBJET | JJL::TypeÉvènement::AJOUTÉ));
    creation |= (évènement == (JJL::TypeÉvènement::OBJET | JJL::TypeÉvènement::ENLEVÉ));
    creation |= (évènement == (JJL::TypeÉvènement::RAFRAICHISSEMENT));

    if (!creation) {
        return;
    }

    ajourne_sélecteur_graphe();

    m_scene->clear();
    assert(m_scene->items().size() == 0);

    auto const graphe = m_jorjala.donne_graphe();

    if (graphe == nullptr) {
        return;
    }

    m_vue->resetTransform();

    /* m_vue->centerOn ne semble pas fonctionner donc on modifier le rectangle
     * de la scène */
    auto rect_scene = m_scene->sceneRect();
    auto const largeur = rect_scene.width();
    auto const hauteur = rect_scene.height();

    rect_scene = QRectF(graphe.donne_centre_x() - static_cast<float>(largeur) * 0.5f,
                        graphe.donne_centre_y() - static_cast<float>(hauteur) * 0.5f,
                        largeur,
                        hauteur);

    m_scene->setSceneRect(rect_scene);

    m_vue->scale(graphe.donne_zoom(), graphe.donne_zoom());

    for (auto node_ptr : graphe.donne_noeuds()) {
        auto item = new ItemNoeud(
            node_ptr,
            node_ptr == graphe.donne_noeud_actif(),
            /*graphe->type == type_graphe::DETAIL || graphe->type == type_graphe::CYCLES*/ false);
        m_scene->addItem(item);

        for (auto prise : node_ptr.donne_entrées()) {
            auto connexion = prise.donne_connexion();
            if (connexion == nullptr) {
                continue;
            }

            auto rectangle_prise = prise.donne_rectangle();
            auto rectangle_lien = connexion.donne_prise_sortie().donne_rectangle();

            auto const p1 = rectangle_prise.position_centrale();
            auto const p2 = rectangle_lien.position_centrale();

            auto ligne = new QGraphicsLineItem();

            if (graphe.donne_connexion_interactive() &&
                prise.donne_connexion() ==
                    graphe.donne_connexion_interactive().donne_connexion_originelle()) {
                ligne->setPen(QPen(Qt::gray, 2.0));
            }
            else {
                ligne->setPen(QPen(Qt::white, 2.0));
            }

            ligne->setLine(p1.donne_x(), p1.donne_y(), p2.donne_x(), p2.donne_y());

            m_scene->addItem(ligne);
        }
    }

    if (graphe.donne_connexion_interactive()) {
        auto connexion = graphe.donne_connexion_interactive();
        JJL::Vec2 p1({});
        if (connexion.donne_prise_entrée() != nullptr) {
            auto prise_entree = connexion.donne_prise_entrée();
            p1 = prise_entree.donne_rectangle().position_centrale();
        }
        else {
            auto prise_sortie = connexion.donne_prise_sortie();
            p1 = prise_sortie.donne_rectangle().position_centrale();
        }

        auto const x2 = connexion.donne_x();
        auto const y2 = connexion.donne_y();

        auto ligne = new QGraphicsLineItem();
        ligne->setPen(QPen(Qt::white, 2.0));
        ligne->setLine(p1.donne_x(), p1.donne_y(), x2, y2);

        m_scene->addItem(ligne);
    }

    if (graphe.donne_noeud_pour_information()) {
        affiche_informations_noeud(graphe.donne_noeud_pour_information());
    }
}

void EditriceGraphe::sors_noeud()
{
    donne_repondant_commande(m_jorjala)->repond_clique("sors_noeud", "");
}

void EditriceGraphe::change_contexte(int index)
{
    INUTILISE(index);
    auto valeur = m_selecteur_graphe->currentData().toString().toStdString();
    donne_repondant_commande(m_jorjala)->repond_clique("change_contexte", valeur);
}

void EditriceGraphe::keyPressEvent(QKeyEvent *event)
{
    rend_actif();

    if (event->key() == Qt::Key_Tab) {
        auto menu = menu_pour_graphe();

        if (!menu) {
            return;
        }

        menu->popup(QCursor::pos());
        return;
    }

    BaseEditrice::keyPressEvent(event);
}

static std::string texte_danjo_pour_menu_catégorisation(JJL::CatégorisationNoeuds catégorisation,
                                                        const std::string &identifiant)
{
    std::stringstream ss;

    ss << "menu \"" << identifiant << "\" {\n";

    for (auto catégorie : catégorisation.donne_catégories()) {
        ss << "  menu \"" << catégorie.donne_nom().vers_std_string() << "\" {\n";

        for (auto noeud : catégorie.donne_noeuds()) {
            ss << "    action(valeur=\"" << noeud.vers_std_string()
               << "\"; attache=ajouter_noeud; métadonnée=\"" << noeud.vers_std_string() << "\")\n";
        }

        ss << "  }\n";
    }

    ss << "}\n";

    return ss.str();
}

QMenu *EditriceGraphe::menu_pour_graphe()
{
    auto graphe = m_jorjala.donne_graphe();
    auto catégorisation = graphe.catégorisation_noeuds();
    auto identifiant_graphe = graphe.identifiant_graphe().vers_std_string();
    if (catégorisation == nullptr) {
        return nullptr;
    }

    auto menu_existant = m_menus.find(identifiant_graphe);
    if (menu_existant != m_menus.end()) {
        return menu_existant->second;
    }

    auto texte = texte_danjo_pour_menu_catégorisation(catégorisation, identifiant_graphe);
    auto donnees = cree_donnees_interface_danjo(m_jorjala, nullptr, nullptr);
    auto gestionnaire = donne_gestionnaire_danjo(m_jorjala);

    auto menu = gestionnaire->compile_menu_texte(donnees, texte);
    if (!menu) {
        return nullptr;
    }

    m_menus[identifiant_graphe] = menu;

    return menu;
}

QPointF EditriceGraphe::transforme_position_evenement(QPoint pos)
{
    auto position = m_vue->mapToScene(pos);
    return QPointF(position.x(), position.y());
}

void EditriceGraphe::ajourne_sélecteur_graphe()
{
    auto chemin_courant = m_jorjala.donne_chemin_courant().vers_std_string();
    m_barre_chemin->setText(chemin_courant.c_str());

    auto racine_chemin_courant = chemin_courant.substr(1, 3);
    auto racines = JJL::liste_informations_graphes_racines();

    /* ajourne le sélecteur, car il sera désynchronisé lors des ouvertures de
     * fichiers */
    ajourne_combo_box(
        m_selecteur_graphe,
        racine_chemin_courant,
        racines,
        [](JJL::InformationGrapheRacine info) -> DonnéesItemComboxBox {
            return {info.donne_nom().vers_std_string(), info.donne_dossier().vers_std_string()};
        });
}

static void imprime_données_section(JJL::InformationsNoeud_Section section, dls::flux_chaine &ss)
{
    if (section.donne_infos().taille() == 0) {
        return;
    }

    ss << "<hr/>";

    auto titre = section.donne_titre().vers_std_string();
    if (!titre.empty()) {
        ss << "<h3>" << titre << "</h3>";
    }

    for (auto info : section.donne_infos()) {
        ss << "<p>" << info.donne_nom().vers_std_string();

        auto texte = info.donne_texte().vers_std_string();
        if (!texte.empty()) {
            ss << " : " << texte;
        }

        ss << "</p>";
    }
}

void EditriceGraphe::affiche_informations_noeud(JJL::Noeud noeud)
{
    dls::flux_chaine ss;
    ss << "<p>Noeud : " << noeud.donne_nom().vers_std_string() << "</p>";

    auto infos = noeud.donne_informations();
    imprime_données_section(infos.donne_entête(), ss);

    for (auto section : infos.donne_sections()) {
        imprime_données_section(section, ss);
    }

    auto point = m_vue->mapFromScene(noeud.donne_pos_x(), noeud.donne_pos_y());
    point = m_vue->mapToGlobal(point);
    QToolTip::showText(point, ss.chn().c_str());
}
