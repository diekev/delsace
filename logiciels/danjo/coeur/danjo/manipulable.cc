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

#include "manipulable.h"

#include <assert.h>

#include "types/courbe_bezier.h"
#include "types/liste_manip.hh"
#include "types/rampe_couleur.h"

namespace danjo {

Manipulable::iterateur Manipulable::debut()
{
    return m_proprietes.debut();
}

Manipulable::iterateur_const Manipulable::debut() const
{
    return m_proprietes.debut();
}

Manipulable::iterateur Manipulable::fin()
{
    return m_proprietes.fin();
}

Manipulable::iterateur_const Manipulable::fin() const
{
    return m_proprietes.fin();
}

void Manipulable::ajoute_propriete(const dls::chaine &nom,
                                   TypePropriete type,
                                   const std::any &valeur)
{
    auto propriete = Propriete::cree(type, valeur);
    m_proprietes.insere({nom, propriete});
}

void Manipulable::ajoute_propriete(const dls::chaine &nom, TypePropriete type)
{
    auto propriete = Propriete::cree(type);
    m_proprietes.insere({nom, propriete});
}

void Manipulable::ajoute_propriete_extra(const dls::chaine &nom, BasePropriete &propriete)
{
    //	Propriete prop = propriete;
    //	prop.est_extra = true;
    //	m_proprietes[nom] = prop;
}

int Manipulable::evalue_entier(const dls::chaine &nom, int temps) const
{
    auto prop = propriete(nom);
    return prop->evalue_entier(temps);
}

float Manipulable::evalue_decimal(const dls::chaine &nom, int temps) const
{
    auto prop = propriete(nom);
    return prop->evalue_decimal(temps);
}

dls::math::vec3f Manipulable::evalue_vecteur(const dls::chaine &nom, int temps) const
{
    auto prop = propriete(nom);
    dls::math::vec3f résultat;
    prop->evalue_vecteur_décimal(temps, &résultat[0]);
    return résultat;
}

dls::phys::couleur32 Manipulable::evalue_couleur(const dls::chaine &nom, int temps) const
{
    auto prop = propriete(nom);
    return prop->evalue_couleur(temps);
}

dls::chaine Manipulable::evalue_fichier_entree(const dls::chaine &nom) const
{
    auto prop = propriete(nom);
    return prop->evalue_chaine(0);
}

dls::chaine Manipulable::evalue_fichier_sortie(const dls::chaine &nom) const
{
    auto prop = propriete(nom);
    return prop->evalue_chaine(0);
}

dls::chaine Manipulable::evalue_chaine(const dls::chaine &nom) const
{
    auto prop = propriete(nom);
    return prop->evalue_chaine(0);
}

bool Manipulable::evalue_bool(const dls::chaine &nom) const
{
    auto prop = propriete(nom);
    return prop->evalue_bool(0);
}

dls::chaine Manipulable::evalue_enum(const dls::chaine &nom) const
{
    auto prop = propriete(nom);
    return prop->evalue_chaine(0);
}

dls::chaine Manipulable::evalue_liste(const dls::chaine &nom) const
{
    auto prop = propriete(nom);
    return prop->evalue_chaine(0);
}

CourbeCouleur const *Manipulable::evalue_courbe_couleur(const dls::chaine &nom) const
{
    return {};  // std::any_cast<CourbeCouleur>(&propriete(nom)->valeur);
}

CourbeBezier const *Manipulable::evalue_courbe_valeur(const dls::chaine &nom) const
{
    return {};  // std::any_cast<CourbeBezier>(&propriete(nom)->valeur);
}

RampeCouleur const *Manipulable::evalue_rampe_couleur(const dls::chaine &nom) const
{
    return {};  // std::any_cast<RampeCouleur>(&propriete(nom)->valeur);
}

ListeManipulable const *Manipulable::evalue_liste_manip(const dls::chaine &nom) const
{
    return nullptr;  // std::any_cast<ListeManipulable>(&propriete(nom)->valeur);
}

void Manipulable::rend_propriete_visible(const dls::chaine &nom, bool ouinon)
{
    m_proprietes[nom]->definit_visibilité(ouinon);
}

bool Manipulable::ajourne_proprietes()
{
    return true;
}

void Manipulable::valeur_bool(const dls::chaine &nom, bool valeur)
{
    auto prop = propriete(nom);
    prop->définis_valeur_bool(valeur);
}

void Manipulable::valeur_entier(const dls::chaine &nom, int valeur)
{
    auto prop = propriete(nom);
    prop->définis_valeur_entier(valeur);
}

void Manipulable::valeur_decimal(const dls::chaine &nom, float valeur)
{
    auto prop = propriete(nom);
    prop->définis_valeur_décimal(valeur);
}

void Manipulable::valeur_vecteur(const dls::chaine &nom, const dls::math::vec3f &valeur)
{
    auto prop = propriete(nom);
    prop->définis_valeur_vec3(valeur);
}

void Manipulable::valeur_couleur(const dls::chaine &nom, const dls::phys::couleur32 &valeur)
{
    auto prop = propriete(nom);
    prop->définis_valeur_couleur(valeur);
}

void Manipulable::valeur_chaine(const dls::chaine &nom, const dls::chaine &valeur)
{
    auto prop = propriete(nom);
    prop->définis_valeur_chaine(valeur.c_str());
}

void *Manipulable::operator[](const dls::chaine &nom)
{
    auto propriete = static_cast<Propriete *>(m_proprietes[nom]);
    void *pointeur = nullptr;

    switch (propriete->type()) {
        case TypePropriete::ENTIER:
            pointeur = std::any_cast<int>(&propriete->valeur);
            break;
        case TypePropriete::DECIMAL:
            pointeur = std::any_cast<float>(&propriete->valeur);
            break;
        case TypePropriete::VECTEUR_DECIMAL:
            pointeur = std::any_cast<dls::math::vec3f>(&propriete->valeur);
            break;
        case TypePropriete::COULEUR:
            pointeur = std::any_cast<dls::phys::couleur32>(&propriete->valeur);
            break;
        case TypePropriete::ENUM:
        case TypePropriete::FICHIER_ENTREE:
        case TypePropriete::FICHIER_SORTIE:
        case TypePropriete::DOSSIER:
        case TypePropriete::CHAINE_CARACTERE:
        case TypePropriete::TEXTE:
            pointeur = std::any_cast<dls::chaine>(&propriete->valeur);
            break;
        case TypePropriete::BOOL:
            pointeur = std::any_cast<bool>(&propriete->valeur);
            break;
        case TypePropriete::COURBE_COULEUR:
            pointeur = std::any_cast<CourbeCouleur>(&propriete->valeur);
            break;
        case TypePropriete::COURBE_VALEUR:
            pointeur = std::any_cast<CourbeBezier>(&propriete->valeur);
            break;
        case TypePropriete::RAMPE_COULEUR:
            pointeur = std::any_cast<RampeCouleur>(&propriete->valeur);
            break;
        case TypePropriete::LISTE_MANIP:
            pointeur = std::any_cast<ListeManipulable>(&propriete->valeur);
            break;
        case TypePropriete::VECTEUR_ENTIER:
        case TypePropriete::LISTE:
            break;
    }

    return pointeur;
}

TypePropriete Manipulable::type_propriete(const dls::chaine &nom) const
{
    auto prop = propriete(nom);
    return prop->type();
}

BasePropriete *Manipulable::propriete(const dls::chaine &nom)
{
    auto iter = m_proprietes.trouve(nom);

    if (iter == m_proprietes.fin()) {
        return nullptr;
    }

    return iter->second;
}

BasePropriete const *Manipulable::propriete(const dls::chaine &nom) const
{
    auto iter = m_proprietes.trouve(nom);

    if (iter == m_proprietes.fin()) {
        return nullptr;
    }

    return iter->second;
}

bool Manipulable::possede_animation() const
{
    for (auto iter = this->debut(); iter != this->fin(); ++iter) {
        auto &prop = iter->second;

        if (prop->est_animee()) {
            return true;
        }
    }

    return false;
}

void Manipulable::performe_versionnage()
{
}

} /* namespace danjo */
