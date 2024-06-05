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

#include "controle_propriete_courbe_couleur.h"

#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

#include "controles/controle_courbe_couleur.h"
#include "controles/controle_echelle_valeur.h"
#include "controles/controle_nombre_decimal.h"

#include "types/courbe_bezier.h"

#include "commun.hh"
#include "donnees_controle.h"

namespace danjo {

ControleProprieteCourbeCouleur::ControleProprieteCourbeCouleur(BasePropriete *p,
                                                               int temps,
                                                               QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement_principal(new QVBoxLayout()),
      m_agencement_nombre(crée_hbox_layout()), m_selection_mode(new QComboBox(this)),
      m_selection_type(new QComboBox(this)), m_utilise_table(new QCheckBox("Utilise table", this)),
      m_controle_courbe(new ControleCourbeCouleur(this)),
      m_bouton_echelle_x(crée_bouton_échelle_valeur(this)),
      m_pos_x(new ControleNombreDecimal(this)),
      m_echelle_x(new ControleEchelleDecimale(m_pos_x, m_bouton_echelle_x)),
      m_bouton_echelle_y(crée_bouton_échelle_valeur(this)),
      m_pos_y(new ControleNombreDecimal(this)),
      m_echelle_y(new ControleEchelleDecimale(m_pos_y, m_bouton_echelle_y))
{
    m_selection_mode->addItem("Maitresse");
    m_selection_mode->addItem("Rouge");
    m_selection_mode->addItem("Verte");
    m_selection_mode->addItem("Bleue");
    m_selection_mode->addItem("Alpha");

    m_selection_type->addItem("RGB");
    m_selection_type->addItem("Filmique");

    m_agencement_principal->addWidget(m_selection_mode);
    m_agencement_principal->addWidget(m_selection_type);
    m_agencement_principal->addWidget(m_utilise_table);
    m_agencement_principal->addWidget(m_controle_courbe);

    m_agencement_nombre->addWidget(m_bouton_echelle_x);
    m_agencement_nombre->addWidget(m_pos_x);
    m_agencement_nombre->addWidget(m_bouton_echelle_y);
    m_agencement_nombre->addWidget(m_pos_y);
    m_agencement_principal->addLayout(m_agencement_nombre);

    connect(
        m_selection_mode, SIGNAL(currentIndexChanged(int)), this, SLOT(change_mode_courbe(int)));

    connect(
        m_selection_type, SIGNAL(currentIndexChanged(int)), this, SLOT(change_type_courbe(int)));

    connect(m_utilise_table,
            &QCheckBox::clicked,
            this,
            &ControleProprieteCourbeCouleur::bascule_utilise_table);

    connect(m_controle_courbe,
            &ControleCourbeCouleur::position_changee,
            this,
            &ControleProprieteCourbeCouleur::ajourne_position);

    connect(m_controle_courbe,
            &ControleCourbeCouleur::point_change,
            this,
            &ControleProprieteCourbeCouleur::ajourne_point_actif);

    connect(m_pos_x,
            &ControleNombreDecimal::valeur_changee,
            this,
            &ControleProprieteCourbeCouleur::ajourne_position_x);

    connect(m_pos_y,
            &ControleNombreDecimal::valeur_changee,
            this,
            &ControleProprieteCourbeCouleur::ajourne_position_y);

    setLayout(m_agencement_principal);
}

ControleProprieteCourbeCouleur::~ControleProprieteCourbeCouleur()
{
    delete m_echelle_x;
    delete m_echelle_y;
}

void ControleProprieteCourbeCouleur::finalise(const DonneesControle &donnees)
{
    /* À FAIRE : min/max */
    m_courbe = static_cast<CourbeCouleur *>(donnees.pointeur);
    m_selection_mode->setCurrentIndex(m_courbe->mode);
    change_mode_courbe(m_courbe->mode);
    m_selection_type->setCurrentIndex(m_courbe->type);
    m_utilise_table->setChecked(m_courbe->courbes[COURBE_COULEUR_MAITRESSE].utilise_table);
    m_pos_x->ajourne_plage(m_courbe_active->valeur_min, m_courbe_active->valeur_max);
    m_pos_y->ajourne_plage(m_courbe_active->valeur_min, m_courbe_active->valeur_max);
}

void ControleProprieteCourbeCouleur::change_mode_courbe(int mode)
{
    m_courbe_active = &m_courbe->courbes[mode];
    m_courbe->mode = mode;
    m_controle_courbe->installe_courbe(m_courbe_active);
    m_controle_courbe->change_mode(mode);
    ajourne_point_actif();
}

void ControleProprieteCourbeCouleur::change_type_courbe(int type)
{
    émets_controle_changé_simple([this, type]() { m_courbe->type = type; });
}

void ControleProprieteCourbeCouleur::bascule_utilise_table(bool ouinon)
{
    émets_controle_changé_simple([this, ouinon]() {
        m_courbe->courbes[COURBE_COULEUR_MAITRESSE].utilise_table = ouinon;
        m_courbe->courbes[COURBE_COULEUR_ROUGE].utilise_table = ouinon;
        m_courbe->courbes[COURBE_COULEUR_VERTE].utilise_table = ouinon;
        m_courbe->courbes[COURBE_COULEUR_BLEUE].utilise_table = ouinon;
        m_courbe->courbes[COURBE_COULEUR_VALEUR].utilise_table = ouinon;
    });
}

void ControleProprieteCourbeCouleur::ajourne_position(float x, float y)
{
    m_pos_x->valeur(x);
    m_pos_y->valeur(y);
    Q_EMIT(controle_change());
}

void ControleProprieteCourbeCouleur::ajourne_position_x(float v)
{
    émets_controle_changé_simple([this, v]() { m_controle_courbe->ajourne_position_x(v); });
}

void ControleProprieteCourbeCouleur::ajourne_position_y(float v)
{
    émets_controle_changé_simple([this, v]() { m_controle_courbe->ajourne_position_y(v); });
}

void ControleProprieteCourbeCouleur::ajourne_point_actif()
{
    if (m_courbe_active->point_courant == nullptr) {
        return;
    }

    m_pos_x->valeur(m_courbe_active->point_courant->co[POINT_CENTRE].x);
    m_pos_y->valeur(m_courbe_active->point_courant->co[POINT_CENTRE].y);
}

} /* namespace danjo */
