/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "gestion_entreface.hh"

#include "biblinternes/patrons_conception/commande.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <QApplication>
#include <QMouseEvent>
#pragma GCC diagnostic pop

#include "base_editrice.h"

#include "jorjala.hh"

namespace detail {

static void notifie_observatrices(void *donnees, JJL::TypeEvenement evenement)
{
    auto gestionnaire = static_cast<GestionnaireEntreface *>(donnees);
    gestionnaire->sujette.notifie_observatrices(static_cast<int>(evenement));
}

static void ajoute_observatrice(void *donnees, void *ptr_observatrice)
{
    auto gestionnaire = static_cast<GestionnaireEntreface *>(donnees);
    Observatrice *observatrice = static_cast<Observatrice *>(ptr_observatrice);
    gestionnaire->sujette.ajoute_observatrice(observatrice);
}

}

static GestionnaireEntreface *extrait_gestionnaire(JJL::Jorjala &jorjala)
{
    auto gestionnaire_jjl = jorjala.gestionnaire_fenêtre();
    return static_cast<GestionnaireEntreface *>(gestionnaire_jjl.données());
}

void initialise_gestion_entreface(GestionnaireEntreface *gestionnaire, JJL::Jorjala &jorjala)
{
    auto gestionnaire_jjl = jorjala.crée_gestionnaire_fenêtre(gestionnaire);
    gestionnaire_jjl.mute_rappel_ajout_observatrice(reinterpret_cast<void *>(detail::ajoute_observatrice));
    gestionnaire_jjl.mute_rappel_notification(reinterpret_cast<void *>(detail::notifie_observatrices));
}

void ajoute_observatrice(JJL::Jorjala &jorjala, Observatrice *observatrice)
{
    auto gestionnaire = extrait_gestionnaire(jorjala);
    observatrice->observe(&gestionnaire->sujette);
}

void active_editrice(JJL::Jorjala &jorjala, BaseEditrice *editrice)
{
    auto gestionnaire = extrait_gestionnaire(jorjala);

    if (gestionnaire->editrice_active) {
        gestionnaire->editrice_active->actif(false);
    }

    gestionnaire->editrice_active = editrice;
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
    // m_jorjala.repondant_commande()->appele_commande("vue_3d", donnees);
}

void gere_mouvement_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id)
{
    auto donnees = donnees_commande_depuis_event(e, id);

    if (e->buttons() == 0) {
        // m_jorjala.repondant_commande()->appele_commande(id, donnees);
    }
    else {
        // m_jorjala.repondant_commande()->ajourne_commande_modale(donnees);
    }
}

void gere_relachement_souris(JJL::Jorjala &jorjala, QMouseEvent *e, const char *id)
{
    auto donnees = donnees_commande_depuis_event(e, id);
    // m_jorjala.repondant_commande()->acheve_commande_modale(donnees);
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

    // m_jorjala.repondant_commande()->appele_commande(id, donnees);
}
