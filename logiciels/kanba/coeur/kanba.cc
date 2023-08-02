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

#include "kanba.h"

#include "biblinternes/patrons_conception/commande.h"
#include "biblinternes/patrons_conception/repondant_commande.h"

#include "commandes/commandes_calques.h"
#include "commandes/commandes_fichier.h"
#include "commandes/commandes_vue2d.h"
#include "commandes/commandes_vue3d.h"

#include "ipa/kanba.cc"
#include "table_types.c"

namespace KNB {

static UsineCommande usine_commande;
static RepondantCommande *repondant_commande = nullptr;

std::optional<KNB::Kanba> initialise_kanba()
{
    if (!KNB_initialise("ipa/libkanba.so")) {
        return {};
    }

    KNB::Kanba kanba = KNB::crée_une_instance_de_kanba();
    if (kanba == nullptr) {
        return {};
    }

    return kanba;
}

void issitialise_kanba(KNB::Kanba &kanba)
{
    détruit_kanba(kanba);
    KNB_issitialise();
}

void enregistre_commandes(KNB::Kanba &kanba)
{
    repondant_commande = new RepondantCommande(usine_commande, kanba);

    enregistre_commandes_calques(usine_commande);
    enregistre_commandes_fichier(usine_commande);
    enregistre_commandes_vue2d(usine_commande);
    enregistre_commandes_vue3d(usine_commande);
}

RepondantCommande *donne_repondant_commande()
{
    return repondant_commande;
}

}  // namespace KNB
