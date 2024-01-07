/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "lexeuse.hh"

#include "biblinternes/langage/nombres.hh"
#include "biblinternes/langage/outils.hh"
#include "biblinternes/langage/unicode.hh"

#include <array>
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
 * nous calculerions lors du lexage ajoute_caractère pourrait ajourner l'empreinte
 */

/* ************************************************************************** */

enum {
    CARACTERE_PEUT_SUIVRE_ZERO = (1 << 0),
    CARACTERE_PEUT_SUIVRE_CHIFFRE = (1 << 1),
    CARACTERE_CHIFFRE_OCTAL = (1 << 2),
    CARACTERE_CHIFFRE_DECIMAL = (1 << 3),
    PEUT_COMMENCER_NOMBRE = (1 << 4),
    PEUT_COMMENCER_IDENTIFIANT = (1 << 5),
    PEUT_SUIVRE_IDENTIFIANT = (1 << 6),
    EST_ESPACE_BLANCHE = (1 << 7),
};

static constexpr auto table_drapeaux_caractères = [] {
    std::array<short, 256> t{};

    for (auto i = 0u; i < 256; ++i) {
        t[i] = 0;

        if ('0' <= i && i <= '7') {
            t[i] |= CARACTERE_CHIFFRE_OCTAL;
        }

        if ('0' <= i && i <= '9') {
            t[i] |= (CARACTERE_CHIFFRE_DECIMAL | PEUT_COMMENCER_NOMBRE | PEUT_SUIVRE_IDENTIFIANT);
        }

        if ('a' <= i && i <= 'z') {
            t[i] |= (PEUT_COMMENCER_IDENTIFIANT | PEUT_SUIVRE_IDENTIFIANT);
        }

        if ('A' <= i && i <= 'Z') {
            t[i] |= (PEUT_COMMENCER_IDENTIFIANT | PEUT_SUIVRE_IDENTIFIANT);
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
            case '.':
            case 'e':
            case '-':
            {
                t[i] |= (CARACTERE_PEUT_SUIVRE_ZERO | CARACTERE_PEUT_SUIVRE_CHIFFRE);
                break;
            }
            case ' ':
            case '\t':
            case '\r':
            case '\n':
            case '\v':
            case '\f':
            {
                t[i] |= EST_ESPACE_BLANCHE;
                break;
            }
            case '_':
            {
                t[i] |= (CARACTERE_PEUT_SUIVRE_ZERO | CARACTERE_PEUT_SUIVRE_CHIFFRE |
                         PEUT_SUIVRE_IDENTIFIANT | PEUT_COMMENCER_IDENTIFIANT);
                break;
            }
        }
    }

    return t;
}();

inline static bool peut_suivre_zero(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] &
            CARACTERE_PEUT_SUIVRE_ZERO) != 0;
}

inline static bool peut_suivre_chiffre(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] &
            CARACTERE_PEUT_SUIVRE_CHIFFRE) != 0;
}

inline static bool est_caractère_octal(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] & CARACTERE_CHIFFRE_OCTAL) !=
           0;
}

inline static bool est_caractère_décimal(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] &
            CARACTERE_CHIFFRE_DECIMAL) != 0;
}

inline static bool est_espace_blanche(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] & EST_ESPACE_BLANCHE) != 0;
}

inline static bool peut_commencer_nombre(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] & PEUT_COMMENCER_NOMBRE) != 0;
}

inline static bool peut_commencer_identifiant(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] &
            PEUT_COMMENCER_IDENTIFIANT) != 0;
}

inline static bool peut_suivre_identifiant(char c)
{
    return (table_drapeaux_caractères[static_cast<unsigned char>(c)] & PEUT_SUIVRE_IDENTIFIANT) !=
           0;
}

static bool est_rune_espace_blanche(uint32_t rune)
{
    switch (rune) {
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
            return true;
        }
        default:
        {
            return false;
        }
    }

    return false;
}

static bool est_rune_guillemet(uint32_t rune)
{
    switch (rune) {
        case GUILLEMET_OUVRANT:
        case GUILLEMET_FERMANT:
        {
            return true;
        }
        default:
        {
            return false;
        }
    }

    return false;
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
static bool doit_ajouter_point_virgule(GenreLexème dernier_id)
{
    switch (dernier_id) {
        default:
        {
            return false;
        }
        /* types */
        case GenreLexème::N8:
        case GenreLexème::N16:
        case GenreLexème::N32:
        case GenreLexème::N64:
        case GenreLexème::R16:
        case GenreLexème::R32:
        case GenreLexème::R64:
        case GenreLexème::Z8:
        case GenreLexème::Z16:
        case GenreLexème::Z32:
        case GenreLexème::Z64:
        case GenreLexème::BOOL:
        case GenreLexème::RIEN:
        case GenreLexème::EINI:
        case GenreLexème::CHAINE:
        case GenreLexème::OCTET:
        case GenreLexème::CHAINE_CARACTERE:
        case GenreLexème::TYPE_DE_DONNEES:
        /* littérales */
        case GenreLexème::CHAINE_LITTERALE:
        case GenreLexème::NOMBRE_REEL:
        case GenreLexème::NOMBRE_ENTIER:
        case GenreLexème::CARACTERE:
        case GenreLexème::VRAI:
        case GenreLexème::FAUX:
        case GenreLexème::NUL:
        /* instructions */
        case GenreLexème::ARRETE:
        case GenreLexème::CONTINUE:
        case GenreLexème::REPRENDS:
        case GenreLexème::RETOURNE:
        /* fermeture */
        case GenreLexème::PARENTHESE_FERMANTE:
        case GenreLexème::CROCHET_FERMANT:
        case GenreLexème::NON_INITIALISATION:
        {
            return true;
        }
    }
}

/* ************************************************************************** */

static int longueur_utf8_depuis_premier_caractère[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4};

Lexeuse::Lexeuse(ContexteLexage contexte, Fichier *données, int drapeaux)
    : m_gérante_chaine(contexte.gerante_chaine), m_table_identifiants(contexte.table_identifiants),
      m_données(données), m_début_mot(données->tampon().debut()),
      m_début(données->tampon().debut()), m_fin(données->tampon().fin()), m_drapeaux(drapeaux),
      m_rappel_erreur(contexte.rappel_erreur)
{
}

Lexème Lexeuse::crée_lexème_opérateur(int nombre_de_caractère, GenreLexème genre_lexème)
{
    enregistre_pos_mot();
    ajoute_caractère(nombre_de_caractère);
    avance_sans_nouvelle_ligne(nombre_de_caractère);
    Lexème résultat = {mot_courant(),
                       {0ul},
                       genre_lexème,
                       static_cast<int>(m_données->id()),
                       m_compte_ligne,
                       m_pos_mot};
    return résultat;
}

Lexème Lexeuse::crée_lexème_littérale_entier(uint64_t valeur)
{
    Lexème résultat = {mot_courant(),
                       {valeur},
                       GenreLexème::NOMBRE_ENTIER,
                       static_cast<int>(m_données->id()),
                       m_compte_ligne,
                       m_pos_mot};
    return résultat;
}

Lexème Lexeuse::crée_lexème_littérale_réelle(double valeur)
{
    Lexème résultat = {mot_courant(),
                       {0ul},
                       GenreLexème::NOMBRE_REEL,
                       static_cast<int>(m_données->id()),
                       m_compte_ligne,
                       m_pos_mot};
    résultat.valeur_reelle = valeur;
    return résultat;
}

void Lexeuse::performe_lexage()
{
    while (!this->fini()) {
        consomme_espaces_blanches();

        if (fini()) {
            /* Fichier vide ou se terminant par des espaces blanches. */
            break;
        }

        auto lexème = donne_lexème_suivant();

        if ((m_drapeaux & INCLUS_COMMENTAIRES) == 0 && lexème.genre == GenreLexème::COMMENTAIRE) {
            continue;
        }

        ajoute_lexème(lexème);
    }

    // crée les identifiants à la fin pour améliorer la cohérence de cache
    {
        auto table_identifiants = m_table_identifiants.verrou_ecriture();

        POUR (m_données->lexèmes) {
            if (it.genre == GenreLexème::SI) {
                it.ident = ID::si;
            }
            else if (it.genre == GenreLexème::SAUFSI) {
                it.ident = ID::saufsi;
            }
            else if (it.genre == GenreLexème::CHAINE_CARACTERE) {
                it.ident = table_identifiants->identifiant_pour_chaine(it.chaine);
            }
        }
    }

    // crée les chaines littérales à la fin pour améliorer la cohérence de cache
    {
        auto gérante_chaine = m_gérante_chaine.verrou_ecriture();

        /* en dehors de la boucle car nous l'utilisons comme tampon */
        kuri::chaine chaine;

        POUR (m_données->lexèmes) {
            if (it.genre != GenreLexème::CHAINE_LITTERALE) {
                continue;
            }

            /* Metton-nous sur la bonne ligne en cas d'erreur. */
            this->m_compte_ligne = it.ligne;
            this->m_position_ligne = it.colonne;

            this->m_début = it.chaine.pointeur();
            auto fin_chaine = this->m_début + it.chaine.taille();

            chaine.efface();
            chaine.reserve(it.chaine.taille());

            while (m_début != fin_chaine) {
                if (*m_début == '\n') {
                    this->m_compte_ligne += 1;
                    this->m_position_ligne = 0;
                }
                this->lèxe_caractère_littéral(&chaine);
            }

            it.index_chaine = gérante_chaine->ajoute_chaine(chaine);
        }
    }

    m_données->fut_lexé = true;
}

void Lexeuse::consomme_espaces_blanches()
{
    while (!this->fini()) {
        auto c = this->caractère_courant();
        auto nombre_octet = longueur_utf8_depuis_premier_caractère[static_cast<unsigned char>(c)];

        switch (nombre_octet) {
            case 1:
            {
                if (!est_espace_blanche(c)) {
                    return;
                }
                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                    this->enregistre_pos_mot();
                    this->ajoute_caractère();
                    this->ajoute_lexème(GenreLexème::CARACTERE_BLANC);
                }

                if (c == '\n') {
                    if (doit_ajouter_point_virgule(m_dernier_id)) {
                        this->enregistre_pos_mot();
                        this->ajoute_caractère();
                        ajoute_lexème(GenreLexème::POINT_VIRGULE);
                    }
                }

                this->avance_fixe<1>();

                if (c == '\n') {
                    m_position_ligne = 0;
                    m_compte_ligne += 1;

                    // idée de micro-optimisation provenant de D, saute 4 espaces à la fois
                    // https://github.com/dlang/dmd/pull/11095
                    // 0x20 == ' '
                    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) == 0) {
                        while (m_début <= m_fin - 4 &&
                               *reinterpret_cast<uint32_t const *>(m_début) == 0x20202020) {
                            m_début += 4;
                            m_position_ligne += 4;
                        }
                    }
                }
                break;
            }
            default:
            {
                auto rune = lng::converti_utf32(m_début, nombre_octet);

                if (rune == 0) {
                    rapporte_erreur_caractère_unicode();
                    return;
                }

                if (!est_rune_espace_blanche(uint32_t(rune))) {
                    return;
                }

                if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
                    this->enregistre_pos_mot();
                    this->ajoute_caractère(nombre_octet);
                    this->ajoute_lexème(GenreLexème::CARACTERE_BLANC);
                }

                this->avance_sans_nouvelle_ligne(nombre_octet);
                break;
            }
        }
    }
}

Lexème Lexeuse::donne_lexème_suivant()
{
#define APPARIE_CARACTERE_SIMPLE(caractere, genre_lexeme)                                         \
    if (c == caractere) {                                                                         \
        return crée_lexème_opérateur(1, genre_lexeme);                                            \
    }

#define APPARIE_CARACTERE_DOUBLE_EGAL(caractere, genre_lexeme, genre_lexeme_avec_egal)            \
    if (c == caractere) {                                                                         \
        if (caractère_voisin(1) == '=') {                                                         \
            return crée_lexème_opérateur(2, genre_lexeme_avec_egal);                              \
        }                                                                                         \
        return crée_lexème_opérateur(1, genre_lexeme);                                            \
    }

#define APPARIE_CARACTERE_SUIVANT(caractere, genre_lexeme)                                        \
    if (caractère_voisin(1) == caractere) {                                                       \
        return crée_lexème_opérateur(2, genre_lexeme);                                            \
    }

#define APPARIE_2_CARACTERES_SUIVANTS(caractere1, caractere2, genre_lexeme)                       \
    if (caractère_voisin(1) == caractere1 && caractère_voisin(2) == caractere2) {                 \
        return crée_lexème_opérateur(3, genre_lexeme);                                            \
    }

    auto c = this->caractère_courant();
    auto nombre_octet = longueur_utf8_depuis_premier_caractère[static_cast<unsigned char>(c)];

    switch (nombre_octet) {
        case 1:
        {
            if (c == '\"') {
                return lèxe_chaine_littérale();
            }

            if (c == '\'') {
                return lèxe_caractère_littérale();
            }

            if (c == '*') {
                if (this->caractère_voisin(1) == '/') {
                    rapporte_erreur("fin de commentaire bloc en dehors d'un commentaire");
                }

                if (this->caractère_voisin(1) == '=') {
                    return crée_lexème_opérateur(2, GenreLexème::MULTIPLIE_EGAL);
                }

                return crée_lexème_opérateur(1, GenreLexème::FOIS);
            }

            if (c == '/') {
                if (this->caractère_voisin(1) == '*') {
                    return lèxe_commentaire_bloc();
                }

                if (this->caractère_voisin(1) == '/') {
                    return lèxe_commentaire();
                }

                if (this->caractère_voisin(1) == '=') {
                    return crée_lexème_opérateur(2, GenreLexème::DIVISE_EGAL);
                }

                return crée_lexème_opérateur(1, GenreLexème::DIVISE);
            }

            if (c == '-') {
                // '-' ou -= ou ---
                APPARIE_2_CARACTERES_SUIVANTS('-', '-', GenreLexème::NON_INITIALISATION)
                APPARIE_CARACTERE_SUIVANT('=', GenreLexème::MOINS_EGAL)
                APPARIE_CARACTERE_SUIVANT('>', GenreLexème::RETOUR_TYPE)
                return crée_lexème_opérateur(1, GenreLexème::MOINS);
            }

            if (c == '.') {
                // . ou ...
                APPARIE_2_CARACTERES_SUIVANTS('.', '.', GenreLexème::TROIS_POINTS)
                APPARIE_CARACTERE_SUIVANT('.', GenreLexème::DEUX_POINTS)
                return crée_lexème_opérateur(1, GenreLexème::POINT);
            }

            if (c == '<') {
                // <, <=, << ou <<=
                APPARIE_2_CARACTERES_SUIVANTS('<', '=', GenreLexème::DEC_GAUCHE_EGAL)
                APPARIE_CARACTERE_SUIVANT('<', GenreLexème::DECALAGE_GAUCHE)
                APPARIE_CARACTERE_SUIVANT('=', GenreLexème::INFERIEUR_EGAL)
                return crée_lexème_opérateur(1, GenreLexème::INFERIEUR);
            }

            if (c == '>') {
                // >, >=, >> ou >>=
                APPARIE_2_CARACTERES_SUIVANTS('>', '=', GenreLexème::DEC_DROITE_EGAL)
                APPARIE_CARACTERE_SUIVANT('>', GenreLexème::DECALAGE_DROITE)
                APPARIE_CARACTERE_SUIVANT('=', GenreLexème::SUPERIEUR_EGAL)
                return crée_lexème_opérateur(1, GenreLexème::SUPERIEUR);
            }

            if (c == ':') {
                // :, :=, ::
                APPARIE_CARACTERE_SUIVANT(':', GenreLexème::DECLARATION_CONSTANTE)
                APPARIE_CARACTERE_SUIVANT('=', GenreLexème::DECLARATION_VARIABLE)
                return crée_lexème_opérateur(1, GenreLexème::DOUBLE_POINTS);
            }

            if (c == '&') {
                APPARIE_CARACTERE_SUIVANT('&', GenreLexème::ESP_ESP)
                APPARIE_CARACTERE_SUIVANT('=', GenreLexème::ET_EGAL)
                return crée_lexème_opérateur(1, GenreLexème::ESPERLUETTE);
            }

            if (c == '|') {
                APPARIE_CARACTERE_SUIVANT('|', GenreLexème::BARRE_BARRE)
                APPARIE_CARACTERE_SUIVANT('=', GenreLexème::OU_EGAL)
                return crée_lexème_opérateur(1, GenreLexème::BARRE);
            }

            APPARIE_CARACTERE_SIMPLE('`', GenreLexème::ACCENT_GRAVE)
            APPARIE_CARACTERE_SIMPLE('~', GenreLexème::TILDE)
            APPARIE_CARACTERE_SIMPLE('[', GenreLexème::CROCHET_OUVRANT)
            APPARIE_CARACTERE_SIMPLE(']', GenreLexème::CROCHET_FERMANT)
            APPARIE_CARACTERE_SIMPLE('{', GenreLexème::ACCOLADE_OUVRANTE)
            APPARIE_CARACTERE_SIMPLE('}', GenreLexème::ACCOLADE_FERMANTE)
            APPARIE_CARACTERE_SIMPLE('@', GenreLexème::AROBASE)
            APPARIE_CARACTERE_SIMPLE(',', GenreLexème::VIRGULE)
            APPARIE_CARACTERE_SIMPLE(';', GenreLexème::POINT_VIRGULE)
            APPARIE_CARACTERE_SIMPLE('#', GenreLexème::DIRECTIVE)
            APPARIE_CARACTERE_SIMPLE('$', GenreLexème::DOLLAR)
            APPARIE_CARACTERE_SIMPLE('(', GenreLexème::PARENTHESE_OUVRANTE)
            APPARIE_CARACTERE_SIMPLE(')', GenreLexème::PARENTHESE_FERMANTE)

            APPARIE_CARACTERE_DOUBLE_EGAL('+', GenreLexème::PLUS, GenreLexème::PLUS_EGAL)
            APPARIE_CARACTERE_DOUBLE_EGAL('!', GenreLexème::EXCLAMATION, GenreLexème::DIFFERENCE)
            APPARIE_CARACTERE_DOUBLE_EGAL('=', GenreLexème::EGAL, GenreLexème::EGALITE)
            APPARIE_CARACTERE_DOUBLE_EGAL('%', GenreLexème::POURCENT, GenreLexème::MODULO_EGAL)
            APPARIE_CARACTERE_DOUBLE_EGAL('^', GenreLexème::CHAPEAU, GenreLexème::OUX_EGAL)

            if (peut_commencer_identifiant(c)) {
                return lèxe_identifiant();
            }

            if (peut_commencer_nombre(c)) {
                return lèxe_littérale_nombre();
            }

            rapporte_erreur("Caractère inconnu");
            break;
        }
        default:
        {
            auto rune = lng::converti_utf32(m_début, nombre_octet);

            if (rune == 0) {
                rapporte_erreur_caractère_unicode();
                break;
            }

            if (est_rune_guillemet(uint32_t(rune))) {
                if (rune == GUILLEMET_FERMANT) {
                    rapporte_erreur("Guillemet fermant sans guillement ouvrant");
                }

                return lèxe_chaine_littérale_guillemet();
            }

            return lèxe_identifiant();
        }
    }

#undef APPARIE_CARACTERE_SIMPLE
#undef APPARIE_CARACTERE_DOUBLE_EGAL
#undef APPARIE_CARACTERE_SUIVANT
#undef APPARIE_2_CARACTERES_SUIVANTS

    return {};
}

Lexème Lexeuse::lèxe_chaine_littérale()
{
    assert(caractère_courant() == '"');

    /* Saute le premier guillemet si nécessaire. */
    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
        this->enregistre_pos_mot();
        this->ajoute_caractère();
        this->avance_fixe<1>();
    }
    else {
        this->avance_fixe<1>();
        this->enregistre_pos_mot();
    }

    /* Sauvegarde la ligne au cas où nous aurions des nouvelles lignes dans le texte. Ce sera
     * utilisé comme ligne pour le lexème. */
    auto const ligne_début = m_compte_ligne;

    while (!this->fini()) {
        if (this->caractère_courant() == '"' && this->caractère_voisin(-1) != '\\') {
            break;
        }

        this->avance();
        this->ajoute_caractère();
    }

    /* Saute le dernier guillemet si nécessaire. */
    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
        this->ajoute_caractère();
    }

    this->avance_fixe<1>();

    Lexème résultat = {mot_courant(),
                       {0ul},
                       GenreLexème::CHAINE_LITTERALE,
                       static_cast<int>(m_données->id()),
                       ligne_début,
                       m_pos_mot};

    return résultat;
}

Lexème Lexeuse::lèxe_chaine_littérale_guillemet()
{
    auto nombre_octet = lng::nombre_octets(m_début);
    /* Saute le premier guillemet si nécessaire. */
    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
        this->enregistre_pos_mot();
        this->ajoute_caractère(nombre_octet);
        this->avance_sans_nouvelle_ligne(nombre_octet);
    }
    else {
        this->avance_sans_nouvelle_ligne(nombre_octet);
        this->enregistre_pos_mot();
    }

    auto profondeur = 0;

    /* Sauvegarde la ligne au cas où nous aurions des nouvelles lignes dans le texte. Ce sera
     * utilisé comme ligne pour le lexème. */
    auto const ligne_début = m_compte_ligne;

    while (!this->fini()) {
        nombre_octet = lng::nombre_octets(m_début);
        auto c = lng::converti_utf32(m_début, nombre_octet);

        if (c == GUILLEMET_OUVRANT) {
            ++profondeur;
        }

        if (c == GUILLEMET_FERMANT) {
            if (profondeur == 0) {
                break;
            }

            --profondeur;
        }

        this->avance_sans_nouvelle_ligne(nombre_octet);
        this->ajoute_caractère(nombre_octet);

        if (c == '\n') {
            m_compte_ligne += 1;
            m_position_ligne = 0;
        }
    }

    /* Saute le dernier guillemet si nécessaire. */
    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
        this->ajoute_caractère(nombre_octet);
    }

    this->avance_sans_nouvelle_ligne(nombre_octet);

    Lexème résultat = {mot_courant(),
                       {0ul},
                       GenreLexème::CHAINE_LITTERALE,
                       static_cast<int>(m_données->id()),
                       ligne_début,
                       m_pos_mot};

    return résultat;
}

Lexème Lexeuse::lèxe_caractère_littérale()
{
    assert(caractère_courant() == '\'');

    /* Saute la première apostrophe si nécessaire. */
    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
        this->enregistre_pos_mot();
        this->ajoute_caractère();
        this->avance_fixe<1>();
    }
    else {
        this->avance_fixe<1>();
        this->enregistre_pos_mot();
    }

    auto valeur = this->lèxe_caractère_littéral(nullptr);

    if (this->caractère_courant() != '\'') {
        rapporte_erreur("attendu une apostrophe");
    }

    /* Saute la dernière apostrophe si nécessaire. */
    if ((m_drapeaux & INCLUS_CARACTERES_BLANC) != 0) {
        this->ajoute_caractère();
    }

    this->avance_fixe<1>();
    Lexème résultat = {mot_courant(),
                       {valeur},
                       GenreLexème::CARACTERE,
                       static_cast<int>(m_données->id()),
                       m_compte_ligne,
                       m_pos_mot};

    return résultat;
}

Lexème Lexeuse::lèxe_identifiant()
{
    this->enregistre_pos_mot();

    while (!fini()) {
        auto c = this->caractère_courant();
        auto nombre_octet = longueur_utf8_depuis_premier_caractère[static_cast<unsigned char>(c)];

        if (nombre_octet == 1) {
            if (!peut_suivre_identifiant(c)) {
                break;
            }

            ajoute_caractère(1);
            avance_sans_nouvelle_ligne(1);
            continue;
        }

        auto rune = lng::converti_utf32(m_début, nombre_octet);

        if (rune == 0) {
            rapporte_erreur_caractère_unicode();
            break;
        }

        if (est_rune_espace_blanche(uint32_t(rune)) || est_rune_guillemet(uint32_t(rune))) {
            break;
        }

        ajoute_caractère(nombre_octet);
        avance_sans_nouvelle_ligne(nombre_octet);
    }

    auto chaine_du_lexème = mot_courant();
    auto genre_du_lexème = lexème_pour_chaine(chaine_du_lexème);

    Lexème résultat = {chaine_du_lexème,
                       {0ul},
                       genre_du_lexème,
                       static_cast<int>(m_données->id()),
                       m_compte_ligne,
                       m_pos_mot};

    return résultat;
}

void Lexeuse::avance(int n)
{
    for (int i = 0; i < n; ++i) {
        if (this->caractère_courant() == '\n') {
            ++m_compte_ligne;
            m_position_ligne = 0;
        }
        else {
            ++m_position_ligne;
        }

        ++m_début;
    }
}

dls::vue_chaine_compacte Lexeuse::mot_courant() const
{
    return dls::vue_chaine_compacte(m_début_mot, m_taille_mot_courant);
}

void Lexeuse::rapporte_erreur(kuri::chaine const &quoi)
{
    rapporte_erreur(quoi, m_position_ligne, m_position_ligne, m_position_ligne + 1);
}

void Lexeuse::rapporte_erreur_caractère_unicode()
{
    rapporte_erreur("Le codec unicode ne peut décoder le caractère");
}

void Lexeuse::rapporte_erreur(const kuri::chaine &quoi, int centre, int min, int max)
{
    if (m_possède_erreur) {
        return;
    }

    m_possède_erreur = true;

    SiteSource site;
    site.fichier = m_données;
    site.index_ligne = m_compte_ligne;
    site.index_colonne = centre;
    site.index_colonne_min = min;
    site.index_colonne_max = max;

    m_rappel_erreur(site, quoi);
}

void Lexeuse::ajoute_lexème(GenreLexème genre)
{
    Lexème résultat = {
        mot_courant(), {0ul}, genre, static_cast<int>(m_données->id()), m_compte_ligne, m_pos_mot};
    ajoute_lexème(résultat);
}

void Lexeuse::ajoute_lexème(Lexème lexème)
{
    m_données->lexèmes.ajoute(lexème);
    m_taille_mot_courant = 0;
    m_dernier_id = lexème.genre;
}

Lexème Lexeuse::lèxe_commentaire()
{
    if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
        return lèxe_commentaire_impl<true>();
    }
    return lèxe_commentaire_impl<false>();
}

template <bool INCLUS_COMMENTAIRE>
Lexème Lexeuse::lèxe_commentaire_impl()
{
    if (INCLUS_COMMENTAIRE) {
        this->enregistre_pos_mot();
    }

    while (!this->fini() && this->caractère_courant() != '\n') {
        this->avance_fixe<1>();
        if (INCLUS_COMMENTAIRE) {
            this->ajoute_caractère();
        }
    }

    Lexème résultat;
    résultat.genre = GenreLexème::COMMENTAIRE;
    if (INCLUS_COMMENTAIRE) {
        résultat = {mot_courant(),
                    {0ul},
                    GenreLexème::COMMENTAIRE,
                    static_cast<int>(m_données->id()),
                    m_compte_ligne,
                    m_pos_mot};
    }
    return résultat;
}

Lexème Lexeuse::lèxe_commentaire_bloc()
{
    if ((m_drapeaux & INCLUS_COMMENTAIRES) != 0) {
        return lèxe_commentaire_bloc_impl<true>();
    }
    return lèxe_commentaire_bloc_impl<false>();
}

template <bool INCLUS_COMMENTAIRE>
Lexème Lexeuse::lèxe_commentaire_bloc_impl()
{
    if (INCLUS_COMMENTAIRE) {
        this->enregistre_pos_mot();
    }

    this->avance_fixe<2>();

    if (INCLUS_COMMENTAIRE) {
        this->ajoute_caractère(2);
    }

    // permet d'avoir des blocs de commentaires nichés
    auto compte_blocs = 0;

    while (!this->fini()) {
        if (this->caractère_courant() == '/' && this->caractère_voisin(1) == '*') {
            this->avance_fixe<2>();
            if (INCLUS_COMMENTAIRE) {
                this->ajoute_caractère(2);
            }
            compte_blocs += 1;
            continue;
        }

        if (this->caractère_courant() == '*' && this->caractère_voisin(1) == '/') {
            this->avance_fixe<2>();
            if (INCLUS_COMMENTAIRE) {
                this->ajoute_caractère(2);
            }

            if (compte_blocs == 0) {
                break;
            }

            compte_blocs -= 1;
        }
        else {
            this->avance(1);
            if (INCLUS_COMMENTAIRE) {
                this->ajoute_caractère();
            }
        }
    }

    Lexème résultat;
    résultat.genre = GenreLexème::COMMENTAIRE;
    if (INCLUS_COMMENTAIRE) {
        résultat = {mot_courant(),
                    {0ul},
                    GenreLexème::COMMENTAIRE,
                    static_cast<int>(m_données->id()),
                    m_compte_ligne,
                    m_pos_mot};
    }
    return résultat;
}

Lexème Lexeuse::lèxe_littérale_nombre()
{
    this->enregistre_pos_mot();

    if (this->caractère_courant() == '0') {
        auto c = this->caractère_voisin();

        if (c == 'b' || c == 'B') {
            return lèxe_nombre_binaire();
        }

        if (c == 'o' || c == 'O') {
            return lèxe_nombre_octal();
        }

        if (c == 'x' || c == 'X') {
            return lèxe_nombre_hexadécimal();
        }

        if (c == 'r' || c == 'R') {
            return lèxe_nombre_reel_hexadécimal();
        }
    }

    return lèxe_nombre_décimal();
}

Lexème Lexeuse::lèxe_nombre_décimal()
{
    uint64_t résultat_entier = 0;
    unsigned nombre_de_chiffres = 0;
    auto début_nombre = m_position_ligne;
    auto taille_texte = 0;
    auto point_trouvé = false;
    auto exposant_trouvé = false;
    auto exposant_négatif = false;

    while (!fini()) {
        auto c = this->caractère_courant();
        taille_texte += 1;

        if (!lng::est_nombre_decimal(c)) {
            if (c == '_') {
                this->avance_fixe<1>();
                this->ajoute_caractère();
                continue;
            }

            if (lng::est_espace_blanc(c)) {
                break;
            }

            // gère triple points
            if (c == '.') {
                if (this->caractère_voisin() == '.' && this->caractère_voisin(2) == '.') {
                    break;
                }

                point_trouvé = true;
                this->avance_fixe<1>();
                this->ajoute_caractère();
                break;
            }

            if (c == 'e') {
                exposant_trouvé = true;
                this->avance_fixe<1>();
                this->ajoute_caractère();
                break;
            }

            break;
        }

        résultat_entier *= 10;
        résultat_entier += static_cast<uint64_t>(c - '0');
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->ajoute_caractère();
    }

    if (!point_trouvé && !exposant_trouvé) {
        if (nombre_de_chiffres > 20) {
            rapporte_erreur(
                "constante entière trop grande", début_nombre, début_nombre, taille_texte);
        }

        return crée_lexème_littérale_entier(résultat_entier);
    }

    auto part_entière = static_cast<double>(résultat_entier);
    double part_fracionnelle[2] = {0.0, 0.0};
    int nombre_chiffres[2] = {0, 0};

    while (!fini()) {
        auto c = this->caractère_courant();

        if (!peut_suivre_chiffre(c)) {
            break;
        }

        if (c == '_') {
            this->avance_fixe<1>();
            this->ajoute_caractère();
            continue;
        }

        // gère triple points
        if (c == '.') {
            if (this->caractère_voisin() == '.' && this->caractère_voisin(2) == '.') {
                break;
            }

            rapporte_erreur("point superflux dans l'expression du nombre",
                            m_position_ligne,
                            m_position_ligne,
                            m_position_ligne);
        }

        if (c == 'e') {
            if (exposant_trouvé) {
                rapporte_erreur("exposant superflux dans l'expression du nombre",
                                m_position_ligne,
                                m_position_ligne,
                                m_position_ligne);
            }

            exposant_trouvé = true;
            this->avance_fixe<1>();
            this->ajoute_caractère();
            continue;
        }

        if (c == '-') {
            if (!exposant_trouvé || nombre_chiffres[1] != 0) {
                break;
            }

            exposant_négatif = true;
            this->avance_fixe<1>();
            this->ajoute_caractère();
            continue;
        }

        auto chiffre = static_cast<double>(c - '0');

        part_fracionnelle[exposant_trouvé] *= 10.0;
        part_fracionnelle[exposant_trouvé] += chiffre;

        nombre_chiffres[exposant_trouvé] += 1;

        this->avance_fixe<1>();
        this->ajoute_caractère();
    }

    if (nombre_chiffres[0] != 0) {
        if (nombre_chiffres[0] <= 16) {
            static const double puissances_de_10[16] = {
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
            part_entière += part_fracionnelle[0] / puissances_de_10[nombre_chiffres[0] - 1];
        }
        else {
            part_entière += part_fracionnelle[0] / std::pow(10.0, nombre_chiffres[0]);
        }
    }

    if (nombre_chiffres[1] != 0) {
        if (exposant_négatif) {
            part_fracionnelle[1] *= -1.0;
        }
        part_entière *= std::pow(10.0, part_fracionnelle[1]);
    }

    return crée_lexème_littérale_réelle(part_entière);
}

Lexème Lexeuse::lèxe_nombre_hexadécimal()
{
    this->avance_fixe<2>();
    this->ajoute_caractère(2);
    auto début_texte = m_position_ligne;
    auto fin_texte = m_position_ligne;

    uint64_t résultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractère_courant();
        auto chiffre = 0u;
        fin_texte += 1;

        if (est_caractère_décimal(c)) {
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
            this->ajoute_caractère();
            continue;
        }
        else {
            break;
        }

        résultat_entier *= 16;
        résultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->ajoute_caractère();
    }

    if (nombre_de_chiffres > 16) {
        rapporte_erreur(
            "constante entière trop grande", début_texte, début_texte - 2, fin_texte - 1);
    }

    return crée_lexème_littérale_entier(résultat_entier);
}

Lexème Lexeuse::lèxe_nombre_reel_hexadécimal()
{
    this->avance_fixe<2>();
    this->ajoute_caractère(2);
    auto début_texte = m_position_ligne;
    auto fin_texte = m_position_ligne;

    uint64_t résultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractère_courant();
        auto chiffre = 0u;
        fin_texte += 1;

        if (est_caractère_décimal(c)) {
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
            this->ajoute_caractère();
            continue;
        }
        else {
            break;
        }

        résultat_entier *= 16;
        résultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->ajoute_caractère();
    }

    if (nombre_de_chiffres % 8 != 0 || nombre_de_chiffres > 16) {
        rapporte_erreur("Une constante réelle hexadécimale doit avoir 8 ou 16 chiffres",
                        début_texte,
                        début_texte - 2,
                        fin_texte - 1);
    }

    if (nombre_de_chiffres == 8) {
        uint32_t v = static_cast<unsigned>(résultat_entier);
        return crée_lexème_littérale_réelle(*reinterpret_cast<float *>(&v));
    }

    return crée_lexème_littérale_réelle(*reinterpret_cast<double *>(&résultat_entier));
}

Lexème Lexeuse::lèxe_nombre_binaire()
{
    this->avance_fixe<2>();
    this->ajoute_caractère(2);
    auto début_texte = m_position_ligne;
    auto fin_texte = m_position_ligne;

    uint64_t résultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractère_courant();
        auto chiffre = 0u;
        fin_texte += 1;

        if (c == '0') {
            // chiffre est déjà 0
        }
        else if (c == '1') {
            chiffre = 1;
        }
        else if (c == '_') {
            this->avance_fixe<1>();
            this->ajoute_caractère();
            continue;
        }
        else {
            break;
        }

        résultat_entier *= 2;
        résultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->ajoute_caractère();
    }

    if (nombre_de_chiffres > 64) {
        rapporte_erreur(
            "constante entière trop grande", début_texte, début_texte - 2, fin_texte - 1);
    }

    return crée_lexème_littérale_entier(résultat_entier);
}

Lexème Lexeuse::lèxe_nombre_octal()
{
    this->avance_fixe<2>();
    this->ajoute_caractère(2);
    auto début_texte = m_position_ligne;
    auto fin_texte = m_position_ligne;

    uint64_t résultat_entier = 0;
    unsigned nombre_de_chiffres = 0;

    while (!fini()) {
        auto c = this->caractère_courant();
        auto chiffre = 0u;
        fin_texte += 1;

        if (est_caractère_octal(c)) {
            chiffre = static_cast<unsigned>(c - '0');
        }
        else if (c == '_') {
            this->avance_fixe<1>();
            this->ajoute_caractère();
            continue;
        }
        else {
            break;
        }

        résultat_entier *= 8;
        résultat_entier += chiffre;
        nombre_de_chiffres += 1;
        this->avance_fixe<1>();
        this->ajoute_caractère();
    }

    if (nombre_de_chiffres > 22) {
        rapporte_erreur(
            "constante entière trop grande", début_texte, début_texte - 2, fin_texte - 1);
    }

    return crée_lexème_littérale_entier(résultat_entier);
}

static int hex_depuis_char(char c)
{
    if (est_caractère_décimal(c)) {
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
unsigned Lexeuse::lèxe_caractère_littéral(kuri::chaine *chaine)
{
    auto c = this->caractère_courant();
    this->avance_fixe<1>();
    this->ajoute_caractère();

    auto v = static_cast<unsigned>(c);

    if (c != '\\') {
        if (chaine) {
            chaine->ajoute(c);
        }

        return v;
    }

    c = this->caractère_courant();
    this->avance_fixe<1>();
    this->ajoute_caractère();

    if (c == 'u') {
        auto début_texte = m_position_ligne;
        for (auto j = 0; j < 4; ++j) {
            auto n = this->caractère_courant();

            auto c0 = hex_depuis_char(n);

            if (c0 == 256) {
                rapporte_erreur("\\u doit prendre 4 chiffres hexadécimaux",
                                début_texte,
                                début_texte - 2,
                                m_position_ligne);
            }

            v <<= 4;
            v |= static_cast<uint32_t>(c0);

            this->avance_fixe<1>();
            this->ajoute_caractère();
        }

        unsigned char sequence[4];
        auto n = lng::point_de_code_vers_utf8(v, sequence);

        if (n == 0) {
            rapporte_erreur(
                "Séquence Unicode invalide", début_texte, début_texte - 2, m_position_ligne);
        }

        if (chaine) {
            for (auto j = 0; j < n; ++j) {
                chaine->ajoute(static_cast<char>(sequence[j]));
            }
        }

        return v;
    }

    if (c == 'U') {
        auto début_texte = m_position_ligne;
        for (auto j = 0; j < 8; ++j) {
            auto n = this->caractère_courant();

            auto c0 = hex_depuis_char(n);

            if (c0 == 256) {
                rapporte_erreur("\\U doit prendre 8 chiffres hexadécimaux",
                                début_texte,
                                début_texte - 2,
                                m_position_ligne);
            }

            v <<= 4;
            v |= static_cast<uint32_t>(c0);

            this->avance_fixe<1>();
            this->ajoute_caractère();
        }

        unsigned char sequence[4];
        auto n = lng::point_de_code_vers_utf8(v, sequence);

        if (n == 0) {
            rapporte_erreur(
                "Séquence Unicode invalide", début_texte, début_texte - 2, m_position_ligne);
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
        auto début_texte = m_position_ligne;
        for (auto j = 0; j < 2; ++j) {
            auto n = this->caractère_courant();

            auto c0 = hex_depuis_char(n);

            if (c0 == 256) {
                rapporte_erreur("\\x doit prendre 2 chiffres hexadécimaux",
                                début_texte,
                                début_texte - 2,
                                m_position_ligne);
            }

            v <<= 4;
            v |= static_cast<unsigned>(c0);

            this->avance_fixe<1>();
            this->ajoute_caractère();
        }
    }
    else if (c == 'd') {
        auto début_texte = m_position_ligne;
        for (auto j = 0; j < 3; ++j) {
            auto n = this->caractère_courant();

            if (n < '0' || n > '9') {
                rapporte_erreur("\\d doit prendre 3 chiffres décimaux",
                                début_texte,
                                début_texte - 2,
                                m_position_ligne);
            }

            v *= 10;
            v += static_cast<unsigned>(n - '0');

            this->avance_fixe<1>();
            this->ajoute_caractère();
        }

        if (v > 255) {
            rapporte_erreur("Valeur décimale trop grande, le maximum est 255",
                            début_texte,
                            début_texte - 2,
                            m_position_ligne);
        }
    }
    else {
        rapporte_erreur("Séquence d'échappement invalide",
                        m_position_ligne - 2,
                        m_position_ligne - 2,
                        m_position_ligne);
    }

    if (chaine) {
        chaine->ajoute(static_cast<char>(v));
    }

    return v;
}
