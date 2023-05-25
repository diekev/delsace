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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <memory>

#include "biblinternes/image/pixel.h"
#include "biblinternes/math/matrice/matrice.hh"
#include "biblinternes/math/rectangle.hh"
#include "biblinternes/outils/iterateurs.h"
#include "biblinternes/structures/liste.hh"

#include "wolika/grille_dense.hh"

/* ************************************************************************** */

using grille_couleur = wlk::grille_dense_2d<dls::phys::couleur32>;

struct calque_image {
  private:
    wlk::base_grille_2d *m_tampon = nullptr;

  public:
    dls::chaine nom{};

    wlk::base_grille_2d *&tampon();

    wlk::base_grille_2d const *tampon() const;

    /* échantillons pour les calques profonds */
    dls::tableau<float> echantillons{};

    calque_image() = default;

    calque_image(calque_image const &autre);

    calque_image(calque_image &&autre);

    calque_image &operator=(calque_image const &autre);

    calque_image &operator=(calque_image &&autre);

    ~calque_image();

    static calque_image construit_calque(wlk::base_grille_2d::type_desc const &desc,
                                         wlk::type_grille type_donnees);
};

template <typename T>
void copie_donnees_calque(wlk::grille_dense_2d<T> const &tampon_de,
                          wlk::grille_dense_2d<T> &tampon_vers)
{
    auto nombre_elements = tampon_de.nombre_elements();

    for (auto i = 0; i < nombre_elements; ++i) {
        tampon_vers.valeur(i) = tampon_de.valeur(i);
    }
}

wlk::desc_grille_2d desc_depuis_rectangle(Rectangle const &rectangle);

inline auto extrait_grille_couleur(calque_image const *calque)
{
    return dynamic_cast<grille_couleur const *>(calque->tampon());
}

inline auto extrait_grille_couleur(calque_image *calque)
{
    return dynamic_cast<grille_couleur *>(calque->tampon());
}

/* ************************************************************************** */

struct Image {
  private:
    using ptr_calque = std::shared_ptr<calque_image>;
    using ptr_calque_profond = std::shared_ptr<calque_image>;

    dls::liste<ptr_calque> m_calques{};
    dls::chaine m_nom_calque{};

  public:
    using plage_calques = dls::outils::plage_iterable<dls::liste<ptr_calque>::iteratrice>;
    using plage_calques_const =
        dls::outils::plage_iterable<dls::liste<ptr_calque>::const_iteratrice>;

    dls::liste<ptr_calque_profond> m_calques_profond{};
    bool est_profonde = false;

    ~Image();

    /**
     * Ajoute un calque à cette image avec le nom spécifié. La taille du calque
     * est définie par le rectangle passé en paramètre. Retourne un pointeur
     * vers le calque ajouté.
     */
    calque_image *ajoute_calque(dls::chaine const &nom,
                                wlk::desc_grille_2d const &desc,
                                wlk::type_grille type);

    calque_image *ajoute_calque_profond(dls::chaine const &nom,
                                        wlk::desc_grille_2d const &desc,
                                        wlk::type_grille type);

    /**
     * Retourne un pointeur vers le calque portant le nom passé en paramètre. Si
     * aucun calque ne portant ce nom est trouvé, retourne nullptr.
     */
    calque_image const *calque_pour_lecture(dls::chaine const &nom) const;

    calque_image *calque_pour_ecriture(dls::chaine const &nom);

    calque_image const *calque_profond_pour_lecture(const dls::chaine &nom) const;

    calque_image *calque_profond_pour_ecriture(const dls::chaine &nom);

    /**
     * Retourne une plage itérable sur la liste de calques de cette Image.
     */
    plage_calques calques();

    /**
     * Retourne une plage itérable constante sur la liste de calques de cette Image.
     */
    plage_calques_const calques() const;

    /**
     * Vide la liste de calques de cette image. Si garde_memoires est faux,
     * les calques seront supprimés. Cette méthode est à utiliser pour
     * transférer la propriété des calques d'une image à une autre.
     */
    void reinitialise();

    /**
     * Renseigne le nom du calque actif.
     */
    void nom_calque_actif(dls::chaine const &nom);

    /**
     * Retourne le nom du calque actif.
     */
    dls::chaine const &nom_calque_actif() const;
};
