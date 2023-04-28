/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestion_entreface.hh"

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QMouseEvent>
#pragma GCC diagnostic pop

#include "base_editrice.h"

#include "coeur/jorjala.hh"

void active_editrice(JJL::Jorjala &jorjala, BaseEditrice *editrice)
{
    auto données = accède_données_programme(jorjala);

    if (données->editrice_active) {
        données->editrice_active->actif(false);
    }

    données->editrice_active = editrice;
}

/* ------------------------------------------------------------------------- */

static DonneesCommande donnees_commande_depuis_event(QMouseEvent *e, const char *id)
{
    // À FAIRE : donnees.x = static_cast<float>(this->size().width() - event->pos().x());
    // this => éditrice
    // auto est_vue2d = std::string("vue_2d") == id;

    auto donnees = DonneesCommande();
    donnees.x = static_cast<float>(e->pos().x());
    donnees.y = static_cast<float>(e->pos().y());
    donnees.souris = static_cast<int>(e->buttons());
    donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());
    return donnees;
}

void gere_pression_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id)
{
    auto donnees = donnees_commande_depuis_event(e, id);
    repondant_commande(jorjala)->appele_commande(id, donnees);
}

void gere_double_clique_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id)
{
    auto donnees = donnees_commande_depuis_event(e, id);
    donnees.double_clique = true;
    repondant_commande(jorjala)->appele_commande(id, donnees);
}

void gere_mouvement_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id)
{
    auto donnees = donnees_commande_depuis_event(e, id);

    if (e->buttons() == 0) {
        repondant_commande(jorjala)->appele_commande(id, donnees);
    }
    else {
        repondant_commande(jorjala)->ajourne_commande_modale(donnees);
    }
}

void gere_relachement_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id)
{
    auto donnees = donnees_commande_depuis_event(e, id);
    repondant_commande(jorjala)->acheve_commande_modale(donnees);
}

void gere_molette_souris(JJL::Jorjala &jorjala, QWheelEvent *e, const char *id)
{
    /* Puisque Qt ne semble pas avoir de bouton pour différencier un clique d'un
     * roulement de la molette de la souris, on prétend que le roulement est un
     * double clique de la molette. */
    auto donnees = DonneesCommande();
    donnees.x = static_cast<float>(e->angleDelta().x());
    donnees.y = static_cast<float>(e->angleDelta().y());
    donnees.souris = Qt::MiddleButton;
    donnees.double_clique = true;
    donnees.modificateur = static_cast<int>(QApplication::keyboardModifiers());

    repondant_commande(jorjala)->appele_commande(id, donnees);
}
