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

#pragma once

#include "biblinternes/math/transformation.hh"
#include "biblinternes/structures/dico.hh"
#include "biblinternes/structures/tableau.hh"

#include "calques.h"

namespace KNB {

struct Kanba;

enum class ChoseÀRecalculer : uint32_t {
    /* Le canal doit être fusionner, par exemple suite à une peinture. */
    CANAL_FUSIONNÉ = (1 << 0),

    TOUT = (~0u),
};

DEFINIS_OPERATEURS_DRAPEAU(ChoseÀRecalculer)

/**
 * Représentation d'un sommet dans l'espace tridimensionel.
 */
struct Sommet {
    dls::math::vec3f pos{};
    long index{};
};

struct Polygone;

/**
 * Représentation d'une arrête d'un polygone.
 */
struct Arrete {
    /* Les sommets connectés à cette arrête. */
    Sommet *s[2] = {nullptr, nullptr};

    /* Le polygone propriétaire de cette arrête. */
    Polygone *p = nullptr;

    /* L'arrête du polygone adjacent allant dans la direction opposée de
     * celle-ci. */
    Arrete *opposee = nullptr;

    /* L'index de l'arrête dans la boucle d'arrêtes du polygone. */
    long index = 0;

    Arrete() = default;
};

/**
 * Représentation d'un quadrilatère et de son vecteur normal dans l'espace
 * tridimensionel.
 */
struct Polygone {
    /* Les sommets formant ce polygone. */
    Sommet *s[4] = {nullptr, nullptr, nullptr, nullptr};

    /* Les arrêtes formant ce polygone. */
    Arrete *a[4] = {nullptr, nullptr, nullptr, nullptr};

    /* Le vecteur normal de ce polygone. */
    dls::math::vec3f nor{};

    /* L'index de ce polygone. */
    long index = 0;

    /* La résolution UV de ce polygone. */
    unsigned int res_u = 0;
    unsigned int res_v = 0;

    unsigned int x = 0;
    unsigned int y = 0;

    Polygone() = default;
};

/**
 * La classe Maillage contient les polygones, les sommets, et les arrêtes
 * formant un objet dans l'espace tridimensionel.
 */
class Maillage {
    dls::tableau<Polygone *> m_polys{};
    dls::tableau<Sommet *> m_sommets{};
    dls::tableau<Arrete *> m_arretes{};

    dls::dico<std::pair<int, int>, Arrete *> m_tableau_arretes{};

    math::transformation m_transformation{};

    CanauxTexture m_canaux{};

    Calque *m_calque_actif = nullptr;

    dls::chaine m_nom{};

    Maillage(Maillage const &autre) = default;
    Maillage &operator=(Maillage const &autre) = default;

    ChoseÀRecalculer m_chose_à_recalculer = ChoseÀRecalculer::TOUT;

  public:
    Maillage();

    ~Maillage();

    /**
     * Ajoute un sommet à ce maillage.
     */
    void ajoute_sommet(dls::math::vec3f const &coord);

    /**
     * Ajoute une suite de sommets à ce maillage.
     */
    void ajoute_sommets(const dls::math::vec3f *sommets, long nombre);

    /**
     * Retourne un pointeur vers le sommet dont l'index correspond à l'index
     * passé en paramètre.
     */
    const Sommet *sommet(long i) const;

    /**
     * Retourne le nombre de sommets de ce maillage.
     */
    long nombre_sommets() const;

    /**
     * Ajoute un quadrilatère à ce maillage. Les paramètres sont les index des
     * sommets déjà ajoutés à ce maillage.
     */
    void ajoute_quad(const long s0, const long s1, const long s2, const long s3);

    /**
     * Retourne le nombre de polygones de ce maillage.
     */
    long nombre_polygones() const;

    /**
     * Retourne un pointeur vers le polygone dont l'index correspond à l'index
     * passé en paramètre.
     */
    Polygone *polygone(long i);

    /**
     * Retourne un pointeur vers le polygone dont l'index correspond à l'index
     * passé en paramètre.
     */
    const Polygone *polygone(long i) const;

    dls::tableau<Polygone *> &donne_polygones()
    {
        return m_polys;
    }

    /**
     * Retourne le nombre d'arrêtes de ce maillage.
     */
    long nombre_arretes() const;

    /**
     * Retourne un pointeur vers l'arrête dont l'index correspond à l'index
     * passé en paramètre.
     */
    Arrete *arrete(long i);

    /**
     * Retourne un pointeur vers le polygone dont l'index correspond à l'index
     * passé en paramètre.
     */
    const Arrete *arrete(long i) const;

    /**
     * Renseigne la transformation de ce maillage.
     */
    void transformation(math::transformation const &transforme);

    /**
     * Retourne la transformation de ce maillage.
     */
    math::transformation const &transformation() const;

    /**
     * Crée un tampon PTex par défaut. À FAIRE : supprimer.
     */
    void cree_tampon(Kanba *kanba);

    /**
     * Renseigne le calque actif de ce maillage.
     */
    void calque_actif(Calque *calque);

    /**
     * Retourne un pointeur vers le calque actif de ce maillage.
     */
    Calque *calque_actif();

    /**
     * Retourne une référence constante vers les données de canaux de ce maillage.
     */
    CanauxTexture const &canaux_texture() const;

    /**
     * Retourne une référence vers les données de canaux de ce maillage.
     */
    CanauxTexture &canaux_texture();

    /**
     * Retourne le nom de ce maillage.
     */
    dls::chaine const &nom() const;

    /**
     * Renomme ce maillage.
     */
    void nom(dls::chaine const &nom);

    void marque_chose_à_recalculer(ChoseÀRecalculer quoi)
    {
        m_chose_à_recalculer |= quoi;
    }

    CanalFusionné donne_canal_fusionné();
    bool doit_recalculer_canal_fusionné() const;
};

}  // namespace KNB
