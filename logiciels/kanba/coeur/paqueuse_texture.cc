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

#include "paqueuse_texture.h"

#include "biblinternes/structures/dico.hh"

#include "kanba.h"
#include "maillage.h"

/**
 * Considération des deux papiers suivant pour continuer l'implémentation :
 *
 * « A Space-efficient and Hardware-friendly Implementation of Ptex »
 * http://wwwx.cs.unc.edu/~sujeong/Ptex/PTEX-sa011-small.pdf
 *
 * « Per-Face Texture Mapping for Realtime Rendering »
 * http://developer.download.nvidia.com/assets/gamedev/docs/RealtimePtex-siggraph2011.pdf
 */

#undef QUEUE_PRIORITE

uint64_t empreinte_xy(unsigned x, unsigned y)
{
    return uint64_t(x) << 32ul | uint64_t(y);
}

static dls::dico<uint64_t, Polygone *> tableau_coord_polygones;

PaqueuseTexture::Noeud::~Noeud()
{
    if (droite) {
        delete droite;
    }

    if (gauche) {
        delete gauche;
    }
}

PaqueuseTexture::PaqueuseTexture(Kanba *kanba) : m_racine(new Noeud()), m_kanba(kanba)
{
}

PaqueuseTexture::~PaqueuseTexture()
{
    delete m_racine;
}

void PaqueuseTexture::empaquete(const dls::tableau<Polygone *> &polygones)
{
    auto const largeur_max = polygones[0]->res_u * polygones.taille();
    auto const hauteur_max = polygones[0]->res_v * polygones.taille();
    m_racine->largeur = 16384;
    m_racine->hauteur = 16384;

    m_kanba->ajoute_log(EntréeLog::Type::EMPAQUETAGE,
                        "Taille noeud racine : ",
                        m_racine->largeur,
                        'x',
                        m_racine->hauteur);

#ifdef QUEUE_PRIORITE
    m_queue_priorite.enfile(m_racine);
#endif

    Noeud *noeud;

    for (auto const &polygone : polygones) {
#ifdef QUEUE_PRIORITE
        while (!m_queue_priorite.est_vide()) {
            noeud = m_queue_priorite.haut();

            if (polygone->res_u <= noeud->largeur && polygone->res_v <= noeud->hauteur) {
                noeud = brise_noeud(noeud, polygone->res_u, polygone->res_v);
                polygone->x = noeud->x;
                polygone->y = noeud->y;

                LOG << "Assignation des coordonnées " << polygone->x << ',' << polygone->y
                    << " au polygone " << polygone->index << " (" << polygone->res_u << 'x'
                    << polygone->res_v << ')' << '\n';

                break;
            }
        }
#else
        noeud = trouve_noeud(m_racine, polygone->res_u, polygone->res_v);

        if (noeud != nullptr) {
            noeud = brise_noeud(noeud, polygone->res_u, polygone->res_v);
        }
        else {
            noeud = elargi_noeud(static_cast<unsigned>(largeur_max),
                                 static_cast<unsigned>(hauteur_max));

            if (!noeud) {
#    if 0
                // À FAIRE : erreur
                LOG << "N'arrive pas à élargir texture " << m_racine->largeur << 'x'
                    << m_racine->hauteur << " pour un polygone de taille " << polygone->res_u
                    << 'x' << polygone->res_v << '\n';
#    endif
            }
        }

        polygone->x = noeud->x;
        polygone->y = noeud->y;

        auto const empreinte = empreinte_xy(polygone->x, polygone->y);

        if (tableau_coord_polygones.trouve(empreinte) != tableau_coord_polygones.fin()) {
            // std::cerr << "Plusieurs polygones ont les mêmes coordonnées !\n";
        }
        else {
            tableau_coord_polygones.insere({empreinte, polygone});
        }

#    if 0
        LOG << "Assignation des coordonnées " << polygone->x << ',' << polygone->y
            << " au polygone " << polygone->index << " (" << polygone->res_u << 'x'
            << polygone->res_v << ')' << '\n';
#    endif

        max_x = std::max(max_x, polygone->x + polygone->res_u);
        max_y = std::max(max_y, polygone->y + polygone->res_v);
#endif
    }

    m_kanba->ajoute_log(
        EntréeLog::Type::EMPAQUETAGE, "Taille texture finale ", largeur(), 'x', hauteur());
}

unsigned int PaqueuseTexture::largeur() const
{
    return max_x;  // m_racine->largeur;
}

unsigned int PaqueuseTexture::hauteur() const
{
    return max_y;  // m_racine->hauteur;
}

PaqueuseTexture::Noeud *PaqueuseTexture::trouve_noeud(Noeud *racine,
                                                      unsigned largeur,
                                                      unsigned hauteur)
{
    // LOG << "Recherche d'un noeud de taille " << largeur << 'x' << hauteur << "...\n";

    if (racine->utilise) {
        Noeud *noeud = nullptr;

        if (racine->droite) {
            noeud = trouve_noeud(racine->droite, largeur, hauteur);
        }

        if (noeud == nullptr && racine->gauche) {
            noeud = trouve_noeud(racine->gauche, largeur, hauteur);
        }

        return noeud;
    }

    if (largeur <= racine->largeur && hauteur <= racine->hauteur) {
        return racine;
    }

    return nullptr;
}

PaqueuseTexture::Noeud *PaqueuseTexture::brise_noeud(Noeud *noeud,
                                                     unsigned largeur,
                                                     unsigned hauteur)
{
    noeud->utilise = true;

    if (noeud->hauteur > hauteur) {
        noeud->gauche = new Noeud();
        noeud->gauche->x = noeud->x;
        noeud->gauche->y = noeud->y + hauteur;
        noeud->gauche->largeur = noeud->largeur;
        noeud->gauche->hauteur = noeud->hauteur - hauteur;

#ifdef QUEUE_PRIORITE
        m_queue_priorite.enfile(noeud->gauche);
#endif
    }

    if (noeud->largeur > largeur) {
        noeud->droite = new Noeud();
        noeud->droite->x = noeud->x + largeur;
        noeud->droite->y = noeud->y;
        noeud->droite->largeur = noeud->largeur - largeur;
        noeud->droite->hauteur = noeud->hauteur;

#ifdef QUEUE_PRIORITE
        m_queue_priorite.enfile(noeud->droite);
#endif
    }

    return noeud;
}

PaqueuseTexture::Noeud *PaqueuseTexture::elargi_noeud(unsigned largeur, unsigned hauteur)
{
#if 1
    if (m_racine->largeur < m_racine->hauteur) {
        return elargi_largeur(largeur, hauteur);
    }

    return elargi_hauteur(largeur, hauteur);
#else
    auto const peut_elargir_hauteur = largeur <= m_racine->largeur;
    auto const peut_elargir_largeur = hauteur <= m_racine->hauteur;

    /* Essaie de rester carrée en élargissant la largeur quand la hauteur
     * est bien plus grande que la largeur. */
    auto const doit_elargir_largeur = peut_elargir_largeur &&
                                      (m_racine->hauteur >= (m_racine->largeur + largeur));

    /* Essaie de rester carrée en élargissant la hauteur quand la largeur
     * est bien plus grande que la hauteur. */
    auto const doit_elargir_hauteur = peut_elargir_hauteur &&
                                      (m_racine->largeur >= (m_racine->hauteur + hauteur));

    if (doit_elargir_largeur) {
        return elargi_largeur(largeur, hauteur);
    }

    if (doit_elargir_hauteur) {
        return elargi_hauteur(largeur, hauteur);
    }

    if (peut_elargir_largeur) {
        return elargi_largeur(largeur, hauteur);
    }

    if (peut_elargir_hauteur) {
        return elargi_hauteur(largeur, hauteur);
    }

    /* Il faut choisir une bonne taille de base pour éviter de retourner ici. */
    return nullptr;
#endif
}

PaqueuseTexture::Noeud *PaqueuseTexture::elargi_largeur(unsigned largeur, unsigned hauteur)
{
    auto racine = new Noeud();
    racine->utilise = true;
    racine->x = 0;
    racine->y = 0;
    racine->hauteur = m_racine->hauteur;
    racine->largeur = m_racine->largeur + largeur;
    racine->gauche = m_racine;
    racine->droite = new Noeud();
    racine->droite->x = m_racine->largeur;
    racine->droite->y = 0;
    racine->droite->largeur = largeur;
    racine->droite->hauteur = m_racine->largeur;

    m_racine = racine;

    auto noeud = trouve_noeud(m_racine, largeur, hauteur);

    if (noeud) {
        return brise_noeud(noeud, largeur, hauteur);
    }

    return nullptr;
}

PaqueuseTexture::Noeud *PaqueuseTexture::elargi_hauteur(unsigned largeur, unsigned hauteur)
{
    auto racine = new Noeud();
    racine->utilise = true;
    racine->x = 0;
    racine->y = 0;
    racine->hauteur = m_racine->hauteur + hauteur;
    racine->largeur = m_racine->largeur;
    racine->droite = m_racine;
    racine->gauche = new Noeud();
    racine->gauche->x = 0;
    racine->gauche->y = m_racine->hauteur;
    racine->gauche->largeur = m_racine->largeur;
    racine->gauche->hauteur = largeur;

    m_racine = racine;

    auto noeud = trouve_noeud(m_racine, largeur, hauteur);

    if (noeud) {
        return brise_noeud(noeud, largeur, hauteur);
    }

    return nullptr;
}
