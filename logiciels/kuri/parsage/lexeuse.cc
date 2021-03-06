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

#include "lexeuse.hh"

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/langage/outils.hh"
#include "biblinternes/langage/unicode.hh"

#include <cmath>

#include "empreinte_parfaite.hh"
#include "gerante_chaine.hh"
#include "identifiant.hh"
#include "modules.hh"

/**
 * Idées pour des optimisations :
 * - cas de caractères simples
 * -- https://github.com/dlang/dmd/pull/5208
 * - utilisation d'une table pour définir quand arrêter de scanner une chaine
 * -- https://v8.dev/blog/scanner
 *
 * - dans les profiles, identifiant_pour_chaine se montre, notamment pour la comparaison des
 * chaines nous pourrions revoir la structure vue_chaine_compacte, pour y ajouter une empreinte que
 * nous calculerions lors du lexage pousse_caractere pourrait ajourner l'empreinte
 */

/* ************************************************************************** */

enum {
    CARACTERE_PEUT_SUIVRE_ZERO = (1 << 0),
    CARACTERE_PEUT_SUIVRE_CHIFFRE = (1 << 1),
    CARACTERE_CHIFFRE_OCTAL = (1 << 2),
    CARACTERE_CHIFFRE_DECIMAL = (1 << 3),
};

static constexpr auto table_drapeaux_caracteres = [] {
    std::array<short, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = 0;

        if ('0' <= i && i <= '7') {
            t[i] |= CARACTERE_CHIFFRE_OCTAL;
        }

        if ('0' <= i && i <= '9') {
            t[i] |= (CARACTERE_CHIFFRE_DECIMAL);
        }

        switch (i) {
            case 'o':
            case 'O':
            case 'x':
            case 'X':
            case 'b':
            case 'B':
            case 'r':
            case 'R':
            {
                t[i] |= CARACTERE_PEUT_SUIVRE_ZERO;
                break;
            }
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '_':
            case '.':
            case 'e':
            case '-':
            {
                t[i] |= (CARACTERE_PEUT_SUIVRE_ZERO | CARACTERE_PEUT_SUIVRE_CHIFFRE);
                break;
            }
        }
    }

    return t;
}();

inline static bool peut_suivre_zero(char c)
{
    return (table_drapeaux_caracteres[static_cast<unsigned char>(c)] &
            CARACTERE_PEUT_SUIVRE_ZERO) != 0;
}

inline static bool peut_suivre_chiffre(char c)
{
    return (table_drapeaux_caracteres[static_cast<unsigned char>(c)] &
            CARACTERE_PEUT_SUIVRE_CHIFFRE) != 0;
}

inline static bool est_caractere_octal(char c)
{
    return (table_drapeaux_caracteres[static_cast<unsigned char>(c)] & CARACTERE_CHIFFRE_OCTAL) !=
           0;
}

inline static bool est_caractere_decimal(char c)
{
    return (table_drapeaux_caracteres[static_cast<unsigned char>(c)] &
            CARACTERE_CHIFFRE_DECIMAL) != 0;
}

/* ************************************************************************** */

/* Point-virgule implicite.
 *
 * Un point-virgule est ajouté quand nous rencontrons une nouvelle ligne si le
 * dernier identifiant correspond à l'un des cas suivants :
 *
 * - une chaine de caractère (nom de variable) ou un type
 * - une littérale (nombre, chaine, faux, vrai)
 * - une des instructions de controle de flux suivantes : retourne, arrête, continue
 * - une parenthèse ou en un crochet fermant
 */
static bool doit_ajouter_point_virgule(GenreLexeme dernier_id)
{
    switch (dernier_id) {
        default:
        {
            return false;
        }
        /* types */
        case GenreLexeme::N8:
        case GenreLexeme::N16:
        case GenreLexeme::N32:
        case GenreLexeme::N64:
        case GenreLexeme::R16:
        case GenreLexeme::R32:
        case GenreLexeme::R64:
        case GenreLexeme::Z8:
        case GenreLexeme::Z16:
        case GenreLexeme::Z32:
        case GenreLexeme::Z64:
        case GenreLexeme::BOOL:
        case GenreLexeme::RIEN:
        case GenreLexeme::EINI:
        case GenreLexeme::CHAINE:
        case GenreLexeme::OCTET:
        case GenreLexeme::CHAINE_CARACTERE:
        case GenreLexeme::TYPE_DE_DONNEES:
        /* littérales */
        case GenreLexeme::CHAINE_LITTERALE:
        case GenreLexeme::NOMBRE_REEL:
        case GenreLexeme::NOMBRE_ENTIER:
        case GenreLexeme::CARACTERE:
        case GenreLexeme::VRAI:
        case GenreLexeme::FAUX:
        case GenreLexeme::NUL:
        /* instructions */
        case GenreLexeme::ARRETE:
        case GenreLexeme::CONTINUE:
        case GenreLexeme::RETOURNE:
        /* fermeture */
        case GenreLexeme::PARENTHESE_FERMANTE:
        case GenreLexeme::CROCHET_FERMANT:
        /* pour les déclarations de structures externes sans définitions */
        case GenreLexeme::EXTERNE:
        case GenreLexeme::NON_INITIALISATION:
        {
            return true;
        }
    }
}

/* ************************************************************************** */

static int longueur_utf8_depuis_premier_caractere[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

Lexeuse::Lexeuse(ContexteLexage contexte, DonneesConstantesFichier *donnees, int drapeaux)
    : m_gerante_chaine(contexte.gerante_chaine), m_table_identifiants(contexte.table_identifiants),
      m_donnees(donnees), m_debut_mot(donnees->tampon.debut()), m_debut(donnees->tampon.debut()),
      m_fin(donnees->tampon.fin()), m_drapeaux(drapeaux), m_rappel_erreur(contexte.rappel_erreur)
{
}

void Lexeuse::performe_lexage()
{
#define POUSSE_CARACTERE(id)                                                                      \
    this->enregistre_pos_mot();                                                                   \
    this->pousse_caractere();                                                                     \
    this->pousse_mot(id);                                                                         \
    this->avance_fixe<1>();

#define POUSSE_MOT_SI_NECESSAIRE                                                                  \
    if (m_taille_mot_courant != 0) {                                                              \
        this->pousse_mot(lexeme_pour_chaine(this->mot_courant()));                                \
    }

#define CAS_CARACTERE(c, id)                                                                      \
    case c:                                                                                       \
    {                                                                                             \
        POUSSE_MOT_SI_NECESSAIRE                                                                  \
        POUSSE_CARACTERE(id);                                                                     \
        break;                                                                                    \
    }

#define CAS_CARACTERE_EGAL(c, id_sans_egal, id_avec_egal)                                         \
    case c:                                                                                       \
    {                                                                                             \
        POUSSE_MOT_SI_NECESSAIRE                                                                  \
        if (this->caractere_voisin(1) == '=') {                                                   \
            this->enregistre_pos_mot();                                                           \
            this->pousse_caractere();                                                             \
            this->pousse_caractere();                                                             \
            this->pousse_mot(id_avec_egal);                                                       \
            this->avance_fixe<2>();                                                               \
        }                                                                                         \
        else {                                                                                    \
            POUSSE_CARACTERE(id_sans_egal);                                                       \
        }                                                                                         \
        break;                                                                                    \
    }

#define APPARIE_SUIVANT(c, id)                                                                    \
    if (this->caractere_voisin(1) == c) {                                                         \
        this->enregistre_pos_mot();                                                               \
        this->pousse_caractere();                                                                 \
        this->pousse_caractere();                                                                 \
        this->pousse_mot(id);                                                                     \
        this->avance_fixe<2>();                                                                   \
        break;                                                                                    \
    }

#define APPARIE_2_SUIVANTS(c1, c2, id)                                                            \
    if (this->caractere_voisin(1) == c1 && this->caractere_voisin(2) == c2) {                     \
        this->enregistre_pos_mot();                                                               \
        this->pousse_caractere();                                                                 \
        this->pousse_caractere();                                                                 \
        this->pousse_caractere();                                                                 \
        this->pousse_mot(id);                                                                     \
        this->avance_fixe<3>();                                                                   \
        break;                                                                                    \
    }

    m_taille_mot_courant = 0;

    while (!this->fini()) {
        switch (this->caractere_courant()) {
            default:
            {
                if (m_taille_mot_courant == 0) {
                    this->enregistre_pos_mot();
                }

                auto nombre_octet =
                    longueur_utf8_depuis_premier_caractere[static_cast<unsigned char>(m_debut[0])];

                switch (nombre_octet) {
                    case 1:
                    {
                        this->pousse_caractere();
                        this->avance_fixe<1>();
                        break;
                    }
                    case 2:
                    case 3:
                    case 4:
                    {
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
                                    this->pousse_mot(lexeme_pour_chaine(this->mot_courant()));
                                }

                                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                                    this->enregistre_pos_mot();
                                    this->pousse_caractere(nombre_octet);
                                    this->pousse_mot(GenreLexeme::CARACTERE_BLANC);
                                }

                                this->avance(nombre_octet);

                                break;
                            }
                            case GUILLEMET_OUVRANT:
                            {
                                if (m_taille_mot_courant != 0) {
                                    this->pousse_mot(lexeme_pour_chaine(this->mot_courant()));
                                }

                                /* Saute le premier guillemet si nécessaire. */
                                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                                    this->enregistre_pos_mot();
                                    this->pousse_caractere(nombre_octet);
                                    this->avance(nombre_octet);
                                }
                                else {
                                    this->avance(nombre_octet);
                                    this->enregistre_pos_mot();
                                }

                                auto profondeur = 0;

                                while (!this->fini()) {
                                    nombre_octet = lng::nombre_octets(m_debut);
                                    c = lng::converti_utf32(m_debut, nombre_octet);

                                    if (c == GUILLEMET_OUVRANT) {
                                        ++profondeur;
                                    }

                                    if (c == GUILLEMET_FERMANT) {
                                        if (profondeur == 0) {
                                            break;
                                        }

                                        --profondeur;
                                    }

                                    this->avance(nombre_octet);
                                    this->pousse_caractere(nombre_octet);
                                }

                                /* Saute le dernier guillemet si nécessaire. */
                                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                                    this->pousse_caractere(nombre_octet);
                                }

                                this->avance(nombre_octet);

                                this->pousse_mot(GenreLexeme::CHAINE_LITTERALE);
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
                        rapporte_erreur("Le codec Unicode ne peut comprendre le caractère !");
                    }
                }

                break;
            }
            case '0':
            {
                if (m_taille_mot_courant == 0) {
                    if (!peut_suivre_zero(m_debut[1])) {
                        auto v = static_cast<unsigned>(this->caractere_courant() - '0');

                        this->enregistre_pos_mot();
                        this->pousse_caractere();
                        this->avance_fixe<1>();
                        this->pousse_lexeme_entier(v);
                        break;
                    }

                    this->lexe_nombre();
                }
                else {
                    this->pousse_caractere();
                    this->avance_fixe<1>();
                }

                break;
            }
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                if (m_taille_mot_courant == 0) {
                    if (!peut_suivre_chiffre(m_debut[1])) {
                        auto v = static_cast<unsigned>(this->caractere_courant() - '0');

                        this->enregistre_pos_mot();
                        this->pousse_caractere();
                        this->avance_fixe<1>();
                        this->pousse_lexeme_entier(v);
                        break;
                    }

                    this->lexe_nombre();
                }
                else {
                    this->pousse_caractere();
                    this->avance_fixe<1>();
                }

                break;
            }
            case '\t':
            case '\r':
            case '\v':
            case '\f':
            case '\n':
            case ' ':
            {
                POUSSE_MOT_SI_NECESSAIRE

                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                    this->enregistre_pos_mot();
                    this->pousse_caractere();
                    this->pousse_mot(GenreLexeme::CARACTERE_BLANC);
                }

                if (this->caractere_courant() == '\n') {
                    if (doit_ajouter_point_virgule(m_dernier_id)) {
                        this->enregistre_pos_mot();
                        this->pousse_caractere();
                        this->pousse_mot(GenreLexeme::POINT_VIRGULE);
                    }

                    ++m_debut;
                    ++m_compte_ligne;
                    m_position_ligne = 0;

                    // idée de micro-optimisation provenant de D, saute 4 espaces à la fois
                    // https://github.com/dlang/dmd/pull/11095
                    // 0x20 == ' '
                    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) == 0) {
                        while (m_debut <= m_fin - 4 &&
                               *reinterpret_cast<uint const *>(m_debut) == 0x20202020) {
                            m_debut += 4;
                            m_position_ligne += 4;
                        }
                    }

                    break;
                }

                this->avance_fixe<1>();
                break;
            }
            case '"':
            {
                POUSSE_MOT_SI_NECESSAIRE

                /* Saute le premier guillemet si nécessaire. */
                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                    this->enregistre_pos_mot();
                    this->pousse_caractere();
                    this->avance_fixe<1>();
                }
                else {
                    this->avance_fixe<1>();
                    this->enregistre_pos_mot();
                }

                while (!this->fini()) {
                    if (this->caractere_courant() == '"' && this->caractere_voisin(-1) != '\\') {
                        break;
                    }

                    this->avance();
                    this->pousse_caractere();
                }

                /* Saute le dernier guillemet si nécessaire. */
                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                    this->pousse_caractere();
                }

                this->avance_fixe<1>();

                this->pousse_mot(GenreLexeme::CHAINE_LITTERALE);
                break;
            }
            case '\'':
            {
                POUSSE_MOT_SI_NECESSAIRE

                /* Saute la première apostrophe si nécessaire. */
                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                    this->enregistre_pos_mot();
                    this->pousse_caractere();
                    this->avance_fixe<1>();
                }
                else {
                    this->avance_fixe<1>();
                    this->enregistre_pos_mot();
                }

                auto valeur = this->lexe_caractere_litteral(nullptr);

                if (this->caractere_courant() != '\'') {
                    rapporte_erreur("attendu une apostrophe");
                }

                /* Saute la dernière apostrophe si nécessaire. */
                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                    this->pousse_caractere();
                }

                this->avance_fixe<1>();
                this->pousse_mot(GenreLexeme::CARACTERE, valeur);
                break;
            }
                CAS_CARACTERE('~', GenreLexeme::TILDE)
                CAS_CARACTERE('[', GenreLexeme::CROCHET_OUVRANT)
                CAS_CARACTERE(']', GenreLexeme::CROCHET_FERMANT)
                CAS_CARACTERE('{', GenreLexeme::ACCOLADE_OUVRANTE)
                CAS_CARACTERE('}', GenreLexeme::ACCOLADE_FERMANTE)
                CAS_CARACTERE('@', GenreLexeme::AROBASE)
                CAS_CARACTERE(',', GenreLexeme::VIRGULE)
                CAS_CARACTERE(';', GenreLexeme::POINT_VIRGULE)
                CAS_CARACTERE('#', GenreLexeme::DIRECTIVE)
                CAS_CARACTERE('$', GenreLexeme::DOLLAR)
                CAS_CARACTERE('(', GenreLexeme::PARENTHESE_OUVRANTE)
                CAS_CARACTERE(')', GenreLexeme::PARENTHESE_FERMANTE)
                CAS_CARACTERE_EGAL('+', GenreLexeme::PLUS, GenreLexeme::PLUS_EGAL)
                CAS_CARACTERE_EGAL('!', GenreLexeme::EXCLAMATION, GenreLexeme::DIFFERENCE)
                CAS_CARACTERE_EGAL('=', GenreLexeme::EGAL, GenreLexeme::EGALITE)
                CAS_CARACTERE_EGAL('%', GenreLexeme::POURCENT, GenreLexeme::MODULO_EGAL)
                CAS_CARACTERE_EGAL('^', GenreLexeme::CHAPEAU, GenreLexeme::OUX_EGAL)
            case '*':
            {
                POUSSE_MOT_SI_NECESSAIRE

                if (this->caractere_voisin(1) == '/') {
                    rapporte_erreur("fin de commentaire bloc en dehors d'un commentaire");
                }

                APPARIE_SUIVANT('=', GenreLexeme::MULTIPLIE_EGAL)
                POUSSE_CARACTERE(GenreLexeme::FOIS)
                break;
            }
            case '/':
            {
                POUSSE_MOT_SI_NECESSAIRE

                if (this->caractere_voisin(1) == '*') {
                    lexe_commentaire_bloc();
                    break;
                }

                if (this->caractere_voisin(1) == '/') {
                    lexe_commentaire();
                    break;
                }

                APPARIE_SUIVANT('=', GenreLexeme::DIVISE_EGAL)
                POUSSE_CARACTERE(GenreLexeme::DIVISE)
                break;
            }
            case '-':
            {
                POUSSE_MOT_SI_NECESSAIRE

                // '-' ou -= ou ---
                APPARIE_2_SUIVANTS('-', '-', GenreLexeme::NON_INITIALISATION)
                APPARIE_SUIVANT('=', GenreLexeme::MOINS_EGAL)
                APPARIE_SUIVANT('>', GenreLexeme::RETOUR_TYPE)
                POUSSE_CARACTERE(GenreLexeme::MOINS)
                break;
            }
            case '.':
            {
                POUSSE_MOT_SI_NECESSAIRE

                // . ou ...
                APPARIE_2_SUIVANTS('.', '.', GenreLexeme::TROIS_POINTS)
                POUSSE_CARACTERE(GenreLexeme::POINT)
                break;
            }
            case '<':
            {
                POUSSE_MOT_SI_NECESSAIRE

                // <, <=, << ou <<=
                APPARIE_2_SUIVANTS('<', '=', GenreLexeme::DEC_GAUCHE_EGAL)
                APPARIE_SUIVANT('<', GenreLexeme::DECALAGE_GAUCHE)
                APPARIE_SUIVANT('=', GenreLexeme::INFERIEUR_EGAL)
                POUSSE_CARACTERE(GenreLexeme::INFERIEUR)
                break;
            }
            case '>':
            {
                POUSSE_MOT_SI_NECESSAIRE

                // >, >=, >> ou >>=
                APPARIE_2_SUIVANTS('>', '=', GenreLexeme::DEC_DROITE_EGAL)
                APPARIE_SUIVANT('>', GenreLexeme::DECALAGE_DROITE)
                APPARIE_SUIVANT('=', GenreLexeme::SUPERIEUR_EGAL)
                POUSSE_CARACTERE(GenreLexeme::SUPERIEUR)
                break;
            }
            case ':':
            {
                POUSSE_MOT_SI_NECESSAIRE

                // :, :=, ::
                APPARIE_SUIVANT(':', GenreLexeme::DECLARATION_CONSTANTE)
                APPARIE_SUIVANT('=', GenreLexeme::DECLARATION_VARIABLE)
                POUSSE_CARACTERE(GenreLexeme::DOUBLE_POINTS)
                break;
            }
            case '&':
            {
                POUSSE_MOT_SI_NECESSAIRE

                APPARIE_SUIVANT('&', GenreLexeme::ESP_ESP)
                APPARIE_SUIVANT('=', GenreLexeme::ET_EGAL)
                POUSSE_CARACTERE(GenreLexeme::ESPERLUETTE)
                break;
            }
            case '|':
            {
                POUSSE_MOT_SI_NECESSAIRE

                APPARIE_SUIVANT('|', GenreLexeme::BARRE_BARRE)
                APPARIE_SUIVANT('=', GenreLexeme::OU_EGAL)
                POUSSE_CARACTERE(GenreLexeme::BARRE)
                break;
            }
        }
    }

    if (m_taille_mot_courant != 0) {
        rapporte_erreur("Des caractères en trop se trouvent à la fin du texte !");
    }

#undef CAS_CARACTERE
#undef CAS_CARACTERE_EGAL
#undef APPARIE_SUIVANT
#undef APPARIE_2_SUIVANTS

    // crée les identifiants à la fin pour améliorer la cohérence de cache
    {
        auto table_identifiants = m_table_identifiants.verrou_ecriture();

        POUR (m_donnees->lexemes) {
            if (it.genre == GenreLexeme::EXTERNE) {
                it.ident = ID::externe;
            }
            else if (it.genre == GenreLexeme::SI) {
                it.ident = ID::si;
            }
            else if (it.genre == GenreLexeme::CHAINE_CARACTERE) {
                it.ident = table_identifiants->identifiant_pour_chaine(it.chaine);
            }
        }
    }

    // crée les chaines littérales à la fin pour améliorer la cohérence de cache
    {
        auto gerante_chaine = m_gerante_chaine.verrou_ecriture();

        /* en dehors de la boucle car nous l'utilisons comme tampon */
        kuri::chaine chaine;

        POUR (m_donnees->lexemes) {
            if (it.genre != GenreLexeme::CHAINE_LITTERALE) {
                continue;
            }

            this->m_debut = it.chaine.pointeur();
            auto fin_chaine = this->m_debut + it.chaine.taille();

            chaine.efface();
            chaine.reserve(it.chaine.taille());

            while (m_debut != fin_chaine) {
                this->lexe_caractere_litteral(&chaine);
            }

            it.index_chaine = gerante_chaine->ajoute_chaine(chaine);
        }
    }

    m_donnees->fut_lexe = true;
}

void Lexeuse::avance(int n)
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

dls::vue_chaine_compacte Lexeuse::mot_courant() const
{
    return dls::vue_chaine_compacte(m_debut_mot, m_taille_mot_courant);
}

void Lexeuse::rapporte_erreur(const kuri::chaine &quoi)
{
    m_possede_erreur = true;
    auto ligne_courante = m_donnees->tampon[m_compte_ligne];

    Enchaineuse enchaineuse;
    enchaineuse << "Erreur : ligne:" << m_compte_ligne + 1 << ":\n";
    enchaineuse << ligne_courante;

    /* La position ligne est en octet, il faut donc compter le nombre d'octets
     * de chaque point de code pour bien formater l'erreur. */
    for (auto i = 0l; i < m_position_ligne;) {
        if (ligne_courante[i] == '\t') {
            enchaineuse << '\t';
        }
        else {
            enchaineuse << ' ';
        }

        i += lng::decalage_pour_caractere(ligne_courante, i);
    }

    enchaineuse << "^~~~\n";
    enchaineuse << quoi;

    m_rappel_erreur(enchaineuse.chaine());
}

void Lexeuse::pousse_mot(GenreLexeme identifiant)
{
    Lexeme lexeme = {mot_courant(),
                     {0ul},
                     identifiant,
                     static_cast<int>(m_donnees->id),
                     m_compte_ligne,
                     m_pos_mot};

    m_donnees->lexemes.ajoute(lexeme);
    m_taille_mot_courant = 0;
    m_dernier_id = identifiant;
}

void Lexeuse::pousse_mot(GenreLexeme identifiant, unsigned valeur)
{
    m_donnees->lexemes.ajoute({mot_courant(),
                               {valeur},
                               identifiant,
                               static_cast<int>(m_donnees->id),
                               m_compte_ligne,
                               m_pos_mot});
    m_taille_mot_courant = 0;
    m_dernier_id = identifiant;
}

void Lexeuse::lexe_commentaire()
{
    if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
        this->enregistre_pos_mot();
    }

    /* ignore commentaire */
    while (this->caractere_courant() != '\n') {
        this->avance_fixe<1>();
        this->pousse_caractere();
    }

    if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
        this->pousse_mot(GenreLexeme::COMMENTAIRE);
    }

    /* Lorsqu'on inclus pas les commentaires, il faut ignorer les
     * caractères poussées. */
    m_taille_mot_courant = 0;
}

void Lexeuse::lexe_commentaire_bloc()
{
    if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
        this->enregistre_pos_mot();
    }

    this->avance_fixe<2>();
    this->pousse_caractere(2);

    // permet d'avoir des blocs de commentaires nichés
    auto compte_blocs = 0;

    while (!this->fini()) {
        if (this->caractere_courant() == '/' && this->caractere_voisin(1) == '*') {
            this->avance(2);
            this->pousse_caractere(2);
            compte_blocs += 1;
            continue;
        }

        if (this->caractere_courant() == '*' && this->caractere_voisin(1) == '/') {
            this->avance(2);
            this->pousse_caractere(2);

            if (compte_blocs == 0) {
                break;
            }

            compte_blocs -= 1;
        }
        else {
            this->avance(1);
            this->pousse_caractere();
        }
    }

    if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
        this->pousse_mot(GenreLexeme::COMMENTAIRE);
    }

    /* Lorsqu'on inclus pas les commentaires, il faut ignorer les
     * caractères poussées. */
    m_taille_mot_courant = 0;
}

void Lexeuse::lexe_nombre()
{
    this->enregistre_pos_mot();

    if (this->caractere_courant() == '0') {
        auto c = this->caractere_voisin();

        if (c == 'b' || c == 'B') {
            lexe_nombre_binaire();
            return;
        }

        if (c == 'o' || c == 'O') {
            lexe_nombre_octal();
            return;
        }

        if (c == 'x' || c == 'X') {
            lexe_nombre_hexadecimal();
            return;
        }

        if (c == 'r' || c == 'R') {
            lexe_nombre_reel_hexadecimal();
            return;
        }
    }

    this->lexe_nombre_decimal();
}

void Lexeuse::lexe_nombre_decimal()
{
    unsigned long long resultat_entier = 0;
    unsigned nombre_de_chiffres = 0;
    auto point_trouve = false;
    auto exposant_trouve = false;
    auto exposant_negatif = false;

    while (!fini()) {
        auto c = this->caractere_courant();

        if (!lng::est_nombre_decimal(c)) {
            if (c == '_') {
                this->avance_fixe<1>();
                this->pousse_caractere();
                continue;
            }

            if (lng::est_espace_blanc(c)) {
                break;
            }

            // gère triple points
            if (c == '.') {
                if (this->caractere_voisin() == '.' && this->caractere_voisin(2) == '.') {
                    break;
                }

                point_trouve = true;
                this->avance_fixe<1>();
                this->pousse_caractere();
                break;
            }

            if (c == 'e') {
                exposant_trouve = true;
                this->avance_fixe<1>();
                this->pousse_caractere();
                break;
            }

            break;
        }

        resultat_entier *= 10;
        resultat_entier += static_cast<unsigned long long>(c - '0');
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->pousse_caractere();
    }

    if (!point_trouve && !exposant_trouve) {
        if (nombre_de_chiffres > 20) {
            rapporte_erreur("constante entière trop grande");
        }

        this->pousse_lexeme_entier(resultat_entier);
        return;
    }

    auto part_entiere = static_cast<double>(resultat_entier);
    double part_fracionnelle[2] = {0.0, 0.0};
    int nombre_chiffres[2] = {0, 0};

    while (!fini()) {
        auto c = this->caractere_courant();

        if (!peut_suivre_chiffre(c)) {
            break;
        }

        if (c == '_') {
            this->avance_fixe<1>();
            this->pousse_caractere();
            continue;
        }

        // gère triple points
        if (c == '.') {
            if (this->caractere_voisin() == '.' && this->caractere_voisin(2) == '.') {
                break;
            }

            rapporte_erreur("point superflux dans l'expression du nombre");
        }

        if (c == 'e') {
            if (exposant_trouve) {
                rapporte_erreur("exposant superflux dans l'expression du nombre");
            }

            exposant_trouve = true;
            this->avance_fixe<1>();
            this->pousse_caractere();
            continue;
        }

        if (c == '-') {
            if (!exposant_trouve || nombre_chiffres[1] != 0) {
                break;
            }

            exposant_negatif = true;
            this->avance_fixe<1>();
            this->pousse_caractere();
            continue;
        }

        auto chiffre = static_cast<double>(c - '0');

        part_fracionnelle[exposant_trouve] *= 10.0;
        part_fracionnelle[exposant_trouve] += chiffre;

        nombre_chiffres[exposant_trouve] += 1;

        this->avance_fixe<1>();
        this->pousse_caractere();
    }

    if (nombre_chiffres[0] != 0) {
        static double puissances_de_10[16] = {
            10.0,
            100.0,
            1000.0,
            10000.0,
            100000.0,
            1000000.0,
            10000000.0,
            100000000.0,
            1000000000.0,
            10000000000.0,
            100000000000.0,
            1000000000000.0,
            10000000000000.0,
            100000000000000.0,
            1000000000000000.0,
            10000000000000000.0,
        };

        if (nombre_chiffres[0] <= 16) {
            part_entiere += part_fracionnelle[0] / puissances_de_10[nombre_chiffres[0] - 1];
        }
        else {
            part_entiere += part_fracionnelle[0] / std::pow(10.0, nombre_chiffres[0]);
        }
    }

    if (nombre_chiffres[1] != 0) {
        if (exposant_negatif) {
            part_fracionnelle[1] *= -1.0;
        }
        part_entiere *= std::pow(10.0, part_fracionnelle[1]);
    }

    this->pousse_lexeme_reel(part_entiere);
}

void Lexeuse::lexe_nombre_hexadecimal()
{
    this->avance_fixe<2>();
    this->pousse_caractere(2);

    unsigned long long resultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractere_courant();
        auto chiffre = 0u;

        if (est_caractere_decimal(c)) {
            chiffre = static_cast<unsigned>(c - '0');
        }
        else if ('a' <= c && c <= 'f') {
            chiffre = 10 + static_cast<unsigned>(c - 'a');
        }
        else if ('A' <= c && c <= 'F') {
            chiffre = 10 + static_cast<unsigned>(c - 'A');
        }
        else if (c == '_') {
            this->avance_fixe<1>();
            this->pousse_caractere();
            continue;
        }
        else {
            break;
        }

        resultat_entier *= 16;
        resultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->pousse_caractere();
    }

    if (nombre_de_chiffres > 16) {
        rapporte_erreur("constante entière trop grande");
    }

    this->pousse_lexeme_entier(resultat_entier);
}

void Lexeuse::lexe_nombre_reel_hexadecimal()
{
    this->avance_fixe<2>();
    this->pousse_caractere(2);

    unsigned long long resultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractere_courant();
        auto chiffre = 0u;

        if (est_caractere_decimal(c)) {
            chiffre = static_cast<unsigned>(c - '0');
        }
        else if ('a' <= c && c <= 'f') {
            chiffre = 10 + static_cast<unsigned>(c - 'a');
        }
        else if ('A' <= c && c <= 'F') {
            chiffre = 10 + static_cast<unsigned>(c - 'A');
        }
        else if (c == '_') {
            this->avance_fixe<1>();
            this->pousse_caractere();
            continue;
        }
        else {
            break;
        }

        resultat_entier *= 16;
        resultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->pousse_caractere();
    }

    if (nombre_de_chiffres % 8 != 0 || nombre_de_chiffres > 16) {
        rapporte_erreur("Une constante réelle hexadécimale doit avoir 8 ou 16 chiffres");
    }

    if (nombre_de_chiffres == 8) {
        unsigned int v = static_cast<unsigned>(resultat_entier);
        this->pousse_lexeme_reel(*reinterpret_cast<float *>(&v));
    }
    else {
        this->pousse_lexeme_reel(*reinterpret_cast<double *>(&resultat_entier));
    }
}

void Lexeuse::lexe_nombre_binaire()
{
    this->avance_fixe<2>();
    this->pousse_caractere(2);

    unsigned long long resultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractere_courant();
        auto chiffre = 0u;

        if (c == '0') {
            // chiffre est déjà 0
        }
        else if (c == '1') {
            chiffre = 1;
        }
        else if (c == '_') {
            this->avance_fixe<1>();
            this->pousse_caractere();
            continue;
        }
        else {
            break;
        }

        resultat_entier *= 2;
        resultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->pousse_caractere();
    }

    if (nombre_de_chiffres > 64) {
        rapporte_erreur("constante entière trop grande");
    }

    this->pousse_lexeme_entier(resultat_entier);
}

void Lexeuse::lexe_nombre_octal()
{
    this->avance_fixe<2>();
    this->pousse_caractere(2);

    unsigned long long resultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractere_courant();
        auto chiffre = 0u;

        if (est_caractere_octal(c)) {
            chiffre = static_cast<unsigned>(c - '0');
        }
        else if (c == '_') {
            this->avance_fixe<1>();
            this->pousse_caractere();
            continue;
        }
        else {
            break;
        }

        resultat_entier *= 8;
        resultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->pousse_caractere();
    }

    if (nombre_de_chiffres > 22) {
        rapporte_erreur("constante entière trop grande");
    }

    this->pousse_lexeme_entier(resultat_entier);
}

static int hex_depuis_char(char c)
{
    if (est_caractere_decimal(c)) {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }

    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }

    return 256;
}

/* Séquences d'échappement du langage :
 * \e : insère un caractère d'échappement (par exemple pour les couleurs dans les terminaux)
 * \f : insère un saut de page
 * \n : insère une nouvelle ligne
 * \r : insère un retour chariot
 * \t : insère une tabulation horizontale
 * \v : insère une tabulation verticale
 * \0 : insère un octet dont la valeur est de zéro (0)
 * \' : insère une apostrophe
 * \" : insère un guillemet
 * \\ : insère un slash arrière
 * \dnnn      : insère une valeur décimale, où n est nombre décimal
 * \unnnn     : insère un caractère Unicode dont seuls les 16-bits du bas sont spécifiés, où n est
 * un nombre hexadécimal \Unnnnnnnn : insère un caractère Unicode sur 32-bits, où n est un nombre
 * hexadécimal \xnn       : insère une valeur hexadécimale, où n est un nombre hexadécimal
 */
unsigned Lexeuse::lexe_caractere_litteral(kuri::chaine *chaine)
{
    auto c = this->caractere_courant();
    this->avance_fixe<1>();
    this->pousse_caractere();

    auto v = static_cast<unsigned>(c);

    if (c != '\\') {
        if (chaine) {
            chaine->ajoute(c);
        }

        return v;
    }

    c = this->caractere_courant();
    this->avance_fixe<1>();
    this->pousse_caractere();

    if (c == 'u') {
        for (auto j = 0; j < 4; ++j) {
            auto n = this->caractere_courant();

            auto c0 = hex_depuis_char(n);

            if (c0 == 256) {
                rapporte_erreur("\\u doit prendre 4 chiffres hexadécimaux");
            }

            v <<= 4;
            v |= static_cast<unsigned int>(c0);

            this->avance_fixe<1>();
            this->pousse_caractere();
        }

        unsigned char sequence[4];
        auto n = lng::point_de_code_vers_utf8(v, sequence);

        if (n == 0) {
            rapporte_erreur("Séquence Unicode invalide");
        }

        if (chaine) {
            for (auto j = 0; j < n; ++j) {
                chaine->ajoute(static_cast<char>(sequence[j]));
            }
        }

        return v;
    }

    if (c == 'U') {
        for (auto j = 0; j < 8; ++j) {
            auto n = this->caractere_courant();

            auto c0 = hex_depuis_char(n);

            if (c0 == 256) {
                rapporte_erreur("\\U doit prendre 8 chiffres hexadécimaux");
            }

            v <<= 4;
            v |= static_cast<unsigned int>(c0);

            this->avance_fixe<1>();
            this->pousse_caractere();
        }

        unsigned char sequence[4];
        auto n = lng::point_de_code_vers_utf8(v, sequence);

        if (n == 0) {
            rapporte_erreur("Séquence Unicode invalide");
        }

        if (chaine) {
            for (auto j = 0; j < n; ++j) {
                chaine->ajoute(static_cast<char>(sequence[j]));
            }
        }

        return v;
    }

    if (c == 'n') {
        v = '\n';
    }
    else if (c == 't') {
        v = '\t';
    }
    else if (c == 'e') {
        v = 0x1b;
    }
    else if (c == '\\') {
        // RÀF
    }
    else if (c == '0') {
        v = 0;
    }
    else if (c == '"') {
        v = '"';
    }
    else if (c == '\'') {
        v = '\'';
    }
    else if (c == 'r') {
        v = '\r';
    }
    else if (c == 'f') {
        v = '\f';
    }
    else if (c == 'v') {
        v = '\v';
    }
    else if (c == 'x') {
        for (auto j = 0; j < 2; ++j) {
            auto n = this->caractere_courant();

            auto c0 = hex_depuis_char(n);

            if (c0 == 256) {
                rapporte_erreur("\\x doit prendre 2 chiffres hexadécimaux");
            }

            v <<= 4;
            v |= static_cast<unsigned>(c0);

            this->avance_fixe<1>();
            this->pousse_caractere();
        }
    }
    else if (c == 'd') {
        for (auto j = 0; j < 3; ++j) {
            auto n = this->caractere_courant();

            if (n < '0' || n > '9') {
                rapporte_erreur("\\d doit prendre 3 chiffres décimaux");
            }

            v *= 10;
            v += static_cast<unsigned>(n - '0');

            this->avance_fixe<1>();
            this->pousse_caractere();
        }

        if (v > 255) {
            rapporte_erreur("Valeur décimale trop grande, le maximum est 255");
        }
    }
    else {
        rapporte_erreur("Séquence d'échappement invalide");
    }

    if (chaine) {
        chaine->ajoute(static_cast<char>(v));
    }

    return v;
}

void Lexeuse::pousse_lexeme_entier(unsigned long long valeur)
{
    auto lexeme = Lexeme{};
    lexeme.genre = GenreLexeme::NOMBRE_ENTIER;
    lexeme.valeur_entiere = valeur;
    lexeme.fichier = static_cast<int>(m_donnees->id);
    lexeme.colonne = m_pos_mot;
    lexeme.ligne = m_compte_ligne;
    lexeme.chaine = mot_courant();

    m_donnees->lexemes.ajoute(lexeme);

    m_taille_mot_courant = 0;
    m_dernier_id = GenreLexeme::NOMBRE_ENTIER;
}

void Lexeuse::pousse_lexeme_reel(double valeur)
{
    auto lexeme = Lexeme{};
    lexeme.genre = GenreLexeme::NOMBRE_REEL;
    lexeme.valeur_reelle = valeur;
    lexeme.fichier = static_cast<int>(m_donnees->id);
    lexeme.colonne = m_pos_mot;
    lexeme.ligne = m_compte_ligne;
    lexeme.chaine = mot_courant();

    m_donnees->lexemes.ajoute(lexeme);

    m_taille_mot_courant = 0;
    m_dernier_id = GenreLexeme::NOMBRE_REEL;
}
