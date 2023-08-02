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

#include "decoupeuse.h"

#include <algorithm>
#include <iostream>

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/langage/outils.hh"
#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/langage/unicode.hh"

#include "erreur.h"
#include "morceaux.h"

using denombreuse = lng::decoupeuse_nombre<danjo::id_morceau>;

namespace danjo {

/* ************************************************************************** */

Decoupeuse::Decoupeuse(lng::tampon_source const &tampon)
    : m_debut(tampon.debut()), m_debut_mot(tampon.debut()), m_fin(tampon.fin()), m_tampon(tampon)
{
    construit_tables_caractere_speciaux();
}

void Decoupeuse::impression_debogage(const dls::chaine &quoi)
{
    std::cout << "Trouvé symbole " << quoi << ", ligne : " << m_ligne
              << ", position : " << m_position_ligne << ".\n";
}

Decoupeuse::iterateur Decoupeuse::begin()
{
    return m_identifiants.debut();
}

Decoupeuse::iterateur Decoupeuse::end()
{
    return m_identifiants.fin();
}

Decoupeuse::iterateur_const Decoupeuse::cbegin() const
{
    return m_identifiants.debut();
}

Decoupeuse::iterateur_const Decoupeuse::cend() const
{
    return m_identifiants.fin();
}

void Decoupeuse::decoupe()
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

                        this->avance(static_cast<size_t>(nombre_octet));
                        break;
                    }
                    case GUILLEMET_OUVRANT:
                    {
                        if (m_taille_mot_courant != 0) {
                            this->pousse_mot(id_chaine(this->mot_courant()));
                        }

                        /* Saute le premier guillemet. */
                        this->avance(static_cast<size_t>(nombre_octet));
                        this->enregistre_pos_mot();

                        while (!this->fini()) {
                            nombre_octet = lng::nombre_octets(m_debut);
                            c = lng::converti_utf32(m_debut, nombre_octet);

                            if (c == GUILLEMET_FERMANT) {
                                break;
                            }

                            m_taille_mot_courant += nombre_octet;
                            this->avance(static_cast<size_t>(nombre_octet));
                        }

                        /* Saute le dernier guillemet. */
                        this->avance(static_cast<size_t>(nombre_octet));

                        this->pousse_mot(id_morceau::CHAINE_LITTERALE);
                        break;
                    }
                    default:
                    {
                        m_taille_mot_courant += nombre_octet;
                        this->avance(static_cast<size_t>(nombre_octet));
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

// si caractere blanc:
//    ajoute mot
// sinon si caractere speciale:
//    ajoute mot
//    si caractere suivant constitue caractere double
//        ajoute mot caractere double
//    sinon
//        si caractere est '.':
//            decoupe nombre ou trois point
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
void Decoupeuse::analyse_caractere_simple()
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
                    lance_erreur("Point inattendu !\n");
                }

                if (this->caractere_voisin(2) != '.') {
                    lance_erreur("Un point est manquant ou un point est en trop !\n");
                }

                this->pousse_caractere();
                this->pousse_caractere();
                this->pousse_caractere();

                this->pousse_mot(id_morceau::TROIS_POINT);
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
        const auto compte = denombreuse::extrait_nombre(m_debut, m_fin, id_nombre);
        m_taille_mot_courant = static_cast<long>(compte);

        this->pousse_mot(id_nombre);
        this->avance(compte);
    }
    else {
        if (m_taille_mot_courant == 0) {
            this->enregistre_pos_mot();
        }

        this->pousse_caractere();
        this->avance();
    }
}

void Decoupeuse::lance_erreur(const dls::chaine &quoi) const
{
    throw ErreurFrappe(m_tampon[m_ligne], m_ligne + 1, m_position_ligne, quoi);
}

void Decoupeuse::ajoute_identifiant(id_morceau identifiant, const dls::vue_chaine &contenu)
{
    m_identifiants.ajoute(
        {contenu, static_cast<size_t>((m_ligne << 32) | m_position_ligne), identifiant});
}

dls::tableau<DonneesMorceaux> &Decoupeuse::morceaux()
{
    return m_identifiants;
}

void Decoupeuse::pousse_caractere()
{
    m_taille_mot_courant += 1;
}

void Decoupeuse::pousse_mot(id_morceau identifiant)
{
    m_identifiants.ajoute(
        {mot_courant(), static_cast<size_t>((m_ligne << 32) | m_position_ligne), identifiant});
    m_taille_mot_courant = 0;
}

void Decoupeuse::enregistre_pos_mot()
{
    m_pos_mot = m_position_ligne;
    m_debut_mot = m_debut;
}

bool Decoupeuse::fini() const
{
    return m_debut >= m_fin;
}

void Decoupeuse::avance(size_t compte)
{
    for (size_t i = 0; i < compte; ++i) {
        if (this->caractere_courant() == '\n') {
            ++m_ligne;
            m_position_ligne = 0;
        }
        else {
            ++m_position_ligne;
        }

        ++m_debut;
    }
}

char Decoupeuse::caractere_courant() const
{
    return *m_debut;
}

char Decoupeuse::caractere_voisin(int n) const
{
    auto ptr = std::max(m_debut, std::min(m_fin, m_debut + n));
    return *ptr;
}

dls::vue_chaine Decoupeuse::mot_courant() const
{
    return dls::vue_chaine(m_debut_mot, m_taille_mot_courant);
}

} /* namespace danjo */
