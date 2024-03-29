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

#include <QApplication>

#include "danjo/controles/controle_courbe_couleur.h"
#include "danjo/controles/controle_echelle_valeur.h"
#include "danjo/controles/controle_masque.h"
#include "danjo/controles/controle_nombre_decimal.h"
#include "danjo/controles/controle_nombre_entier.h"
#include "danjo/controles/controle_rampe_couleur.h"
#include "danjo/controles/controle_roue_couleur.h"
#include "danjo/controles/controle_spectre_couleur.h"
#include "danjo/controles/controle_teinte_couleur.h"
#include "danjo/controles_proprietes/controle_propriete_courbe_couleur.h"
#include "danjo/controles_proprietes/controle_propriete_courbe_valeur.h"
#include "danjo/controles_proprietes/controle_propriete_liste.h"
#include "danjo/controles_proprietes/controle_propriete_rampe_couleur.h"

#include "danjo/conteneur_controles.h"

#include "danjo/controles_proprietes/donnees_controle.h"

#include "types/rampe_couleur.h"

class Conteneur : public danjo::ConteneurControles {
  public:
    void ajourne_manipulable() override
    {
    }

    void obtiens_liste(const dls::chaine & /*attache*/,
                       dls::tableau<dls::chaine> &chaines) override
    {
        chaines.ajoute("action1");
        chaines.ajoute("action2");
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //	Conteneur conteneur;

    //	danjo::ControleProprieteListe controle;
    //	controle.conteneur(&conteneur);
    //	controle.show();

    RampeCouleur rampe;
    ajoute_point_rampe(rampe, 0.0f, dls::phys::couleur32{0.0f, 0.0f, 0.0f, 1.0f});
    ajoute_point_rampe(rampe, 0.5f, dls::phys::couleur32{0.0f, 1.0f, 0.0f, 1.0f});
    ajoute_point_rampe(rampe, 1.0f, dls::phys::couleur32{1.0f, 1.0f, 1.0f, 1.0f});

    danjo::DonneesControle donnees;
    donnees.pointeur = &rampe;

    danjo::ControleProprieteRampeCouleur controle(nullptr, 0);
    controle.finalise(donnees);
    controle.show();

    return app.exec();
}
