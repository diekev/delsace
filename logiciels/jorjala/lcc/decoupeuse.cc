﻿/*
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

#include "decoupeuse.h"

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/langage/outils.hh"
#include "biblinternes/langage/unicode.hh"
#include "biblinternes/structures/flux_chaine.hh"

#include "erreur.h"
#include "modules.hh"

using denombreuse = lng::decoupeuse_nombre<id_morceau>;

/* ************************************************************************** */

decoupeuse_texte::decoupeuse_texte(DonneesModule *module)
    : m_module(module), m_debut_mot(module->tampon.debut()), m_debut(module->tampon.debut()),
      m_fin(module->tampon.fin())
{
    construit_tables_caractere_speciaux();
}

void decoupeuse_texte::genere_morceaux()
{
    m_taille_mot_courant = 0;

    while (!this->fini()) {
        auto nombre_octet = lng::nombre_octets(m_debut);

        switch (nombre_octet) {
            case 1:
            {
                analyse_caractere_simple();
                break;
            }
            case 2:
            case 3:
            case 4:
            {
                if (m_taille_mot_courant == 0) {
                    this->enregistre_pos_mot();
                }

                auto c = lng::converti_utf32(m_debut, nombre_octet);

                switch (c) {
                    case ESPACE_INSECABLE:
                    case ESPACE_D_OGAM:
                    case SEPARATEUR_VOYELLES_MONGOL:
                    case DEMI_CADRATIN:
                    case CADRATIN:
                    case ESPACE_DEMI_CADRATIN:
                    case ESPACE_CADRATIN:
                    case TIERS_DE_CADRATIN:
                    case QUART_DE_CADRATIN:
                    case SIXIEME_DE_CADRATIN:
                    case ESPACE_TABULAIRE:
                    case ESPACE_PONCTUATION:
                    case ESPACE_FINE:
                    case ESPACE_ULTRAFINE:
                    case ESPACE_SANS_CHASSE:
                    case ESPACE_INSECABLE_ETROITE:
                    case ESPACE_MOYENNE_MATHEMATIQUE:
                    case ESPACE_IDEOGRAPHIQUE:
                    case ESPACE_INSECABLE_SANS_CHASSE:
                    {
                        if (m_taille_mot_courant != 0) {
                            this->pousse_mot(id_chaine(this->mot_courant()));
                        }

                        this->avance(nombre_octet);
                        break;
                    }
                    case GUILLEMET_OUVRANT:
                    {
                        if (m_taille_mot_courant != 0) {
                            this->pousse_mot(id_chaine(this->mot_courant()));
                        }

                        /* Saute le premier guillemet. */
                        this->avance(nombre_octet);
                        this->enregistre_pos_mot();

                        while (!this->fini()) {
                            nombre_octet = lng::nombre_octets(m_debut);
                            c = lng::converti_utf32(m_debut, nombre_octet);

                            if (c == GUILLEMET_FERMANT) {
                                break;
                            }

                            m_taille_mot_courant += nombre_octet;
                            this->avance(nombre_octet);
                        }

                        /* Saute le dernier guillemet. */
                        this->avance(nombre_octet);

                        this->pousse_mot(id_morceau::CHAINE_LITTERALE);
                        break;
                    }
                    default:
                    {
                        m_taille_mot_courant += nombre_octet;
                        this->avance(nombre_octet);
                        break;
                    }
                }

                break;
            }
            default:
            {
                /* Le caractère (octet) courant est invalide dans le codec unicode. */
                lance_erreur("Le codec Unicode ne peut comprendre le caractère !");
            }
        }
    }

    if (m_taille_mot_courant != 0) {
        lance_erreur("Des caractères en trop se trouvent à la fin du texte !");
    }
}

size_t decoupeuse_texte::memoire_morceaux() const
{
    return static_cast<size_t>(m_module->morceaux.taille()) * sizeof(DonneesMorceaux);
}

void decoupeuse_texte::imprime_morceaux(std::ostream &os)
{
    for (auto const &morceau : m_module->morceaux) {
        os << chaine_identifiant(morceau.genre) << '\n';
    }
}

bool decoupeuse_texte::fini() const
{
    return m_debut >= m_fin;
}

void decoupeuse_texte::avance(int n)
{
    for (int i = 0; i < n; ++i) {
        if (this->caractere_courant() == '\n') {
            ++m_compte_ligne;
            m_position_ligne = 0;
        }
        else {
            ++m_position_ligne;
        }

        ++m_debut;
    }
}

char decoupeuse_texte::caractere_courant() const
{
    return *m_debut;
}

char decoupeuse_texte::caractere_voisin(int n) const
{
    return *(m_debut + n);
}

dls::vue_chaine decoupeuse_texte::mot_courant() const
{
    return dls::vue_chaine(m_debut_mot, m_taille_mot_courant);
}

void decoupeuse_texte::lance_erreur(const dls::chaine &quoi) const
{
    std::cerr << __func__ << '\n';

    auto ligne_courante = m_module->tampon[m_compte_ligne];

    dls::flux_chaine ss;
    ss << "Erreur : ligne:" << m_compte_ligne + 1 << ":\n";
    ss << ligne_courante;

    /* La position ligne est en octet, il faut donc compter le nombre d'octets
     * de chaque point de code pour bien formater l'erreur. */
    for (auto i = 0l; i < m_position_ligne;) {
        if (ligne_courante[i] == '\t') {
            ss << '\t';
        }
        else {
            ss << ' ';
        }

        i += lng::decalage_pour_caractere(ligne_courante, i);
    }

    ss << "^~~~\n";
    ss << quoi;

    throw erreur::frappe(ss.chn().c_str(), erreur::type_erreur::DECOUPAGE);
}

// si caractere blanc:
//    ajoute mot
// sinon si caractere speciale:
//    ajoute mot
//    si caractere suivant constitue caractere double
//        ajoute mot caractere double
//    sinon
//        si caractere est '.':
//            decoupe trois point
//        sinon si caractere est '"':
//            decoupe chaine caractere littérale
//        sinon si caractere est '#':
//            decoupe commentaire
//        sinon si caractere est '\'':
//            decoupe caractere
//        sinon:
//        	ajoute mot caractere simple
// sinon si nombre et mot est vide:
//    decoupe nombre
// sinon:
//    ajoute caractere mot courant
void decoupeuse_texte::analyse_caractere_simple()
{
    auto idc = id_morceau::INCONNU;

    if (lng::est_espace_blanc(this->caractere_courant())) {
        if (m_taille_mot_courant != 0) {
            this->pousse_mot(id_chaine(this->mot_courant()));
        }

        this->avance();
    }
    else if (est_caractere_special(this->caractere_courant(), idc)) {
        if (m_taille_mot_courant != 0) {
            this->pousse_mot(id_chaine(this->mot_courant()));
        }

        this->enregistre_pos_mot();

        auto id = id_caractere_double(dls::vue_chaine(m_debut, 2));

        if (id != id_morceau::INCONNU) {
            this->pousse_caractere();
            this->pousse_caractere();
            this->pousse_mot(id);
            this->avance(2);
            return;
        }

        switch (this->caractere_courant()) {
            case '.':
            {
                if (this->caractere_voisin() != '.') {
                    this->pousse_caractere();
                    this->pousse_mot(id_morceau::POINT);
                    this->avance();
                    break;
                }

                if (this->caractere_voisin(2) != '.') {
                    lance_erreur("Un point est manquant ou un point est en trop !\n");
                }

                this->pousse_caractere();
                this->pousse_caractere();
                this->pousse_caractere();

                this->pousse_mot(id_morceau::TROIS_POINTS);
                this->avance(3);
                break;
            }
            case '"':
            {
                /* Saute le premier guillemet. */
                this->avance();
                this->enregistre_pos_mot();

                while (!this->fini()) {
                    if (this->caractere_courant() == '"' && this->caractere_voisin(-1) != '\\') {
                        break;
                    }

                    this->pousse_caractere();
                    this->avance();
                }

                /* Saute le dernier guillemet. */
                this->avance();

                this->pousse_mot(id_morceau::CHAINE_LITTERALE);
                break;
            }
            case '\'':
            {
                /* Saute la première apostrophe. */
                this->avance();

                this->enregistre_pos_mot();

                if (this->caractere_courant() == '\\') {
                    this->pousse_caractere();
                    this->avance();
                }

                this->pousse_caractere();
                this->pousse_mot(id_morceau::CARACTERE);

                this->avance();

                /* Saute la dernière apostrophe. */
                if (this->caractere_courant() != '\'') {
                    lance_erreur("Plusieurs caractères détectés dans un caractère simple !\n");
                }

                this->avance();
                break;
            }
            case '#':
            {
                /* ignore commentaire */
                while (this->caractere_courant() != '\n') {
                    this->avance();
                }
                break;
            }
            default:
            {
                this->pousse_caractere();
                this->pousse_mot(idc);
                this->avance();
                break;
            }
        }
    }
    else if (m_taille_mot_courant == 0 && lng::est_nombre_decimal(this->caractere_courant())) {
        this->enregistre_pos_mot();

        id_morceau id_nombre;

        /* NOTE : on utilise une variable temporaire pour stocker le compte au
         * lieu d'utiliser m_taille_mot_courant directement, car
         * m_taille_mot_courant est remis à 0 dans pousse_mot() donc on ne peut
         * l'utiliser en paramètre de avance() (ce qui causerait une boucle
         * infinie. */
        auto const compte = denombreuse::extrait_nombre(m_debut, m_fin, id_nombre);
        m_taille_mot_courant = static_cast<long>(compte);

        this->pousse_mot(id_nombre);
        this->avance(static_cast<int>(compte));
    }
    else {
        if (m_taille_mot_courant == 0) {
            this->enregistre_pos_mot();
        }

        this->pousse_caractere();
        this->avance();
    }
}

void decoupeuse_texte::pousse_caractere()
{
    m_taille_mot_courant += 1;
}

void decoupeuse_texte::pousse_mot(id_morceau identifiant)
{
    m_module->morceaux.ajoute({mot_courant(),
                               static_cast<size_t>((m_compte_ligne << 32) | m_pos_mot),
                               identifiant,
                               static_cast<int>(m_module->id)});
    m_taille_mot_courant = 0;
}

void decoupeuse_texte::enregistre_pos_mot()
{
    m_pos_mot = m_position_ligne;
    m_debut_mot = m_debut;
}
