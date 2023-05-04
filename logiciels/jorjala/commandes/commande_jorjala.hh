/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/patrons_conception/commande.h"

namespace JJL {
class Jorjala;
enum class TypeCurseur : int;
}

class CommandeJorjala : public Commande {
public:
    /* Fonctions requises pas Commande. */
    int execute(std::any const &pointeur, DonneesCommande const &donnees) override;
    void ajourne_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override;
    void termine_execution_modale(std::any const &pointeur, DonneesCommande const &donnees) override;
    bool evalue_predicat(std::any const &pointeur, dls::chaine const &metadonnee) override;

    /* Fonctions virtuelles correspondantes à celles requises, remplaçant std::any par JJL::Jorjala. */
    virtual int execute_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees);
    virtual void ajourne_execution_modale_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees);
    virtual void termine_execution_modale_jorjala(JJL::Jorjala &jorjala, DonneesCommande const &donnees);
    virtual bool evalue_predicat_jorjala(JJL::Jorjala &jorjala, dls::chaine const &metadonnee);

    virtual JJL::TypeCurseur type_curseur_modal();
};
