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

#include "controle_propriete_vecteur.h"

#include <QHBoxLayout>
#include <QPushButton>

#include "biblinternes/outils/chaine.hh"

#include "compilation/morceaux.h"

#include "controles/controle_echelle_valeur.h"
#include "controles/controle_nombre_decimal.h"
#include "controles/controle_nombre_entier.h"

#include "commun.hh"
#include "controle_propriete_decimal.h"
#include "donnees_controle.h"

#include <sstream>

namespace danjo {

/* ************************************************************************* */

BaseControleProprieteVecteur::BaseControleProprieteVecteur(BasePropriete *p,
                                                           int temps,
                                                           QWidget *parent)
    : ControlePropriete(p, temps, parent), m_agencement(new QHBoxLayout(this)),
      m_bouton_animation(crée_bouton_animation_controle(this))
{
    for (int i = 0; i < DIMENSIONS_MAX; i++) {
        m_bouton_echelle_dim[i] = nullptr;
    }

    const int dimensions = p->donne_dimensions_vecteur();
    m_dimensions = dimensions;

    for (int i = 0; i < m_dimensions; i++) {
        m_bouton_echelle_dim[i] = crée_bouton_échelle_valeur(this);
    }
}

void BaseControleProprieteVecteur::montre_echelle_0()
{
    montre_echelle(0);
}

void BaseControleProprieteVecteur::montre_echelle_1()
{
    montre_echelle(1);
}

void BaseControleProprieteVecteur::montre_echelle_2()
{
    montre_echelle(2);
}

void BaseControleProprieteVecteur::montre_echelle_3()
{
    montre_echelle(3);
}

/* ************************************************************************* */

ControleProprieteVecteurDecimal::ControleProprieteVecteurDecimal(BasePropriete *p,
                                                                 int temps,
                                                                 QWidget *parent)
    : BaseControleProprieteVecteur(p, temps, parent)

{
    for (int i = 0; i < DIMENSIONS_MAX; i++) {
        m_dim[i] = nullptr;
        m_echelle[i] = nullptr;
    }

    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i] = new ControleNombreDecimal(this);

        m_echelle[i] = new ControleEchelleDecimale();
        m_echelle[i]->setWindowFlags(Qt::WindowStaysOnTopHint);
    }

    m_agencement->addWidget(m_bouton_animation);

    for (int i = 0; i < m_dimensions; i++) {
        m_agencement->addWidget(m_bouton_echelle_dim[i]);
        m_agencement->addWidget(m_dim[i]);
    }
    setLayout(m_agencement);

#define CONNECT_VALEUR_CHANGEE(dim, func)                                                         \
    if (m_dim[dim]) {                                                                             \
        connect(m_dim[dim],                                                                       \
                &ControleNombreDecimal::valeur_changee,                                           \
                this,                                                                             \
                &ControleProprieteVecteurDecimal::func);                                          \
    }

    CONNECT_VALEUR_CHANGEE(0, ajourne_valeur_0);
    CONNECT_VALEUR_CHANGEE(1, ajourne_valeur_1);
    CONNECT_VALEUR_CHANGEE(2, ajourne_valeur_2);
    CONNECT_VALEUR_CHANGEE(3, ajourne_valeur_3);

#undef CONNECT_VALEUR_CHANGEE

    for (int i = 0; i < m_dimensions; i++) {
        connect(m_dim[i],
                &ControleNombreDecimal::prevaleur_changee,
                this,
                &ControleProprieteVecteurDecimal::emet_precontrole_change);

        connect(m_echelle[i],
                &ControleEchelleDecimale::valeur_changee,
                m_dim[i],
                &ControleNombreDecimal::ajourne_valeur);

        connect(m_echelle[i],
                &ControleEchelleDecimale::prevaleur_changee,
                this,
                &ControleProprieteVecteurDecimal::emet_precontrole_change);
    }

#define CONNECT_MONTRE_ECHELLE(dim, func)                                                         \
    if (m_bouton_echelle_dim[dim]) {                                                              \
        connect(m_bouton_echelle_dim[dim],                                                        \
                &QPushButton::pressed,                                                            \
                this,                                                                             \
                &ControleProprieteVecteurDecimal::func);                                          \
    }

    CONNECT_MONTRE_ECHELLE(0, montre_echelle_0);
    CONNECT_MONTRE_ECHELLE(1, montre_echelle_1);
    CONNECT_MONTRE_ECHELLE(2, montre_echelle_2);
    CONNECT_MONTRE_ECHELLE(3, montre_echelle_3);

#undef CONNECT_MONTRE_ECHELLE

    connect(m_bouton_animation,
            &QPushButton::pressed,
            this,
            &ControleProprieteVecteurDecimal::bascule_animation);
}

ControleProprieteVecteurDecimal::~ControleProprieteVecteurDecimal()
{
    for (int i = 0; i < m_dimensions; i++) {
        delete m_echelle[i];
    }
}

void ControleProprieteVecteurDecimal::finalise(const DonneesControle &donnees)
{
    auto plage = m_propriete->plage_valeur_vecteur_décimal();

    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i]->ajourne_plage(plage.min, plage.max);
    }

    if (!m_propriete->est_animable()) {
        m_bouton_animation->hide();
    }

    m_animation = m_propriete->est_animee();
    définit_état_bouton_animation(m_bouton_animation, m_animation);

    if (m_animation) {
        auto temps_exacte = m_propriete->possede_cle(m_temps);

        for (int i = 0; i < m_dimensions; i++) {
            m_dim[i]->marque_anime(m_animation, temps_exacte);
        }
    }

    ajourne_valeurs_controles();
}

void ControleProprieteVecteurDecimal::bascule_animation()
{
    m_animation = !m_animation;

    if (m_animation == false) {
        m_propriete->supprime_animation();
        ajourne_valeurs_controles();
    }
    else {
        // À FAIRE : restaure ceci
        // m_propriete->ajoute_cle(m_propriete->evalue_vecteur(m_temps), m_temps);
    }

    définit_état_bouton_animation(m_bouton_animation, m_animation);
    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i]->marque_anime(m_animation, m_animation);
    }
}

void ControleProprieteVecteurDecimal::ajourne_valeur_0(float valeur)
{
    ajourne_valeur(0, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurDecimal::ajourne_valeur_1(float valeur)
{
    ajourne_valeur(1, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurDecimal::ajourne_valeur_2(float valeur)
{
    ajourne_valeur(2, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurDecimal::ajourne_valeur_3(float valeur)
{
    ajourne_valeur(3, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurDecimal::montre_echelle(int index)
{
    m_echelle[index]->valeur(m_dim[index]->valeur());
    m_echelle[index]->plage(m_dim[index]->min(), m_dim[index]->max());
    m_echelle[index]->show();
}

void ControleProprieteVecteurDecimal::ajourne_valeurs_controles()
{
    float valeurs[4];
    m_propriete->evalue_vecteur_décimal(m_temps, valeurs);
    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i]->valeur(valeurs[size_t(i)]);
    }
}

void ControleProprieteVecteurDecimal::ajourne_valeur(int index, float valeur)
{
    auto vec = dls::math::vec3f(0.0f);
    for (int i = 0; i < m_dimensions; i++) {
        vec[size_t(i)] = m_dim[size_t(i)]->valeur();
    }
    vec[size_t(index)] = valeur;

    if (m_animation) {
        m_propriete->ajoute_cle(vec, m_temps);
    }
    else {
        m_propriete->définit_valeur_vec3(vec);
    }
}

/* ************************************************************************* */

ControleProprieteVecteurEntier::ControleProprieteVecteurEntier(BasePropriete *p,
                                                               int temps,
                                                               QWidget *parent)
    : BaseControleProprieteVecteur(p, temps, parent)

{
    for (int i = 0; i < DIMENSIONS_MAX; i++) {
        m_dim[i] = nullptr;
        m_echelle[i] = nullptr;
    }

    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i] = new ControleNombreEntier(this);

        m_echelle[i] = new ControleEchelleEntiere();
        m_echelle[i]->setWindowFlags(Qt::WindowStaysOnTopHint);
    }

    m_agencement->addWidget(m_bouton_animation);

    for (int i = 0; i < m_dimensions; i++) {
        m_agencement->addWidget(m_bouton_echelle_dim[i]);
        m_agencement->addWidget(m_dim[i]);
    }
    setLayout(m_agencement);

#define CONNECT_VALEUR_CHANGEE(dim, func)                                                         \
    if (m_dim[dim]) {                                                                             \
        connect(m_dim[dim],                                                                       \
                &ControleNombreEntier::valeur_changee,                                            \
                this,                                                                             \
                &ControleProprieteVecteurEntier::func);                                           \
    }

    CONNECT_VALEUR_CHANGEE(0, ajourne_valeur_0);
    CONNECT_VALEUR_CHANGEE(1, ajourne_valeur_1);
    CONNECT_VALEUR_CHANGEE(2, ajourne_valeur_2);
    CONNECT_VALEUR_CHANGEE(3, ajourne_valeur_3);

#undef CONNECT_VALEUR_CHANGEE

    for (int i = 0; i < m_dimensions; i++) {
        connect(m_dim[i],
                &ControleNombreEntier::prevaleur_changee,
                this,
                &ControleProprieteVecteurEntier::emet_precontrole_change);

        connect(m_echelle[i],
                &ControleEchelleEntiere::valeur_changee,
                m_dim[i],
                &ControleNombreEntier::ajourne_valeur);

        connect(m_echelle[i],
                &ControleEchelleEntiere::prevaleur_changee,
                this,
                &ControleProprieteVecteurEntier::emet_precontrole_change);
    }

#define CONNECT_MONTRE_ECHELLE(dim, func)                                                         \
    if (m_bouton_echelle_dim[dim]) {                                                              \
        connect(m_bouton_echelle_dim[dim],                                                        \
                &QPushButton::pressed,                                                            \
                this,                                                                             \
                &ControleProprieteVecteurEntier::func);                                           \
    }

    CONNECT_MONTRE_ECHELLE(0, montre_echelle_0);
    CONNECT_MONTRE_ECHELLE(1, montre_echelle_1);
    CONNECT_MONTRE_ECHELLE(2, montre_echelle_2);
    CONNECT_MONTRE_ECHELLE(3, montre_echelle_3);

#undef CONNECT_MONTRE_ECHELLE

    connect(m_bouton_animation,
            &QPushButton::pressed,
            this,
            &ControleProprieteVecteurEntier::bascule_animation);
}

ControleProprieteVecteurEntier::~ControleProprieteVecteurEntier()
{
    for (int i = 0; i < m_dimensions; i++) {
        delete m_echelle[i];
    }
}

void ControleProprieteVecteurEntier::finalise(const DonneesControle &donnees)
{
    auto plage = m_propriete->plage_valeur_vecteur_entier();

    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i]->ajourne_plage(plage.min, plage.max);
    }

    if (!m_propriete->est_animable()) {
        m_bouton_animation->hide();
    }

    m_animation = m_propriete->est_animee();
    définit_état_bouton_animation(m_bouton_animation, m_animation);

    if (m_animation) {
        auto temps_exacte = m_propriete->possede_cle(m_temps);

        for (int i = 0; i < m_dimensions; i++) {
            m_dim[i]->marque_anime(m_animation, temps_exacte);
        }
    }

    ajourne_valeurs_controles();
}

void ControleProprieteVecteurEntier::bascule_animation()
{
    m_animation = !m_animation;

    if (m_animation == false) {
        m_propriete->supprime_animation();
        ajourne_valeurs_controles();
    }
    else {
        // À FAIRE : restaure ceci
        // m_propriete->ajoute_cle(m_propriete->evalue_vecteur(m_temps), m_temps);
    }

    définit_état_bouton_animation(m_bouton_animation, m_animation);
    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i]->marque_anime(m_animation, m_animation);
    }
}

void ControleProprieteVecteurEntier::ajourne_valeur_0(int valeur)
{
    ajourne_valeur(0, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurEntier::ajourne_valeur_1(int valeur)
{
    ajourne_valeur(1, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurEntier::ajourne_valeur_2(int valeur)
{
    ajourne_valeur(2, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurEntier::ajourne_valeur_3(int valeur)
{
    ajourne_valeur(3, valeur);
    Q_EMIT(controle_change());
}

void ControleProprieteVecteurEntier::montre_echelle(int index)
{
    m_echelle[index]->valeur(m_dim[index]->valeur());
    m_echelle[index]->plage(m_dim[index]->min(), m_dim[index]->max());
    m_echelle[index]->show();
}

void ControleProprieteVecteurEntier::ajourne_valeurs_controles()
{
    int valeurs[4];
    m_propriete->evalue_vecteur_entier(m_temps, valeurs);
    for (int i = 0; i < m_dimensions; i++) {
        m_dim[i]->valeur(valeurs[size_t(i)]);
    }
}

void ControleProprieteVecteurEntier::ajourne_valeur(int index, int valeur)
{
    auto vec = dls::math::vec3i(0);
    for (int i = 0; i < m_dimensions; i++) {
        vec[size_t(i)] = m_dim[size_t(i)]->valeur();
    }
    vec[size_t(index)] = valeur;

    if (m_animation) {
        m_propriete->ajoute_cle(vec, m_temps);
    }
    else {
        m_propriete->définit_valeur_vec3(vec);
    }
}

} /* namespace danjo */
