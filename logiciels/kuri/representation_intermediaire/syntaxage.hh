/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#pragma once

#include <optional>
#include <variant>

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/assembleuse.hh"

#include "compilation/operateurs.hh"

#include "parsage/base_syntaxeuse.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexemes.hh"

#include "structures/tablet.hh"

#include "utilitaires/log.hh"

#include "constructrice_ri.hh"

using LexèmesType = kuri::tablet<Lexème *, 6>;

/* Pour discriminer le début de l'expression d'un type.
 * Le lexème débute un type...
 */
enum class LexèmeType : uint8_t {
    /* ...pointeur. */
    POINTEUR,
    /* ...référence. */
    RÉFÉRENCE,
    /* ...tableau fixe ou dynamique. */
    TABLEAU,
    /* ...varidique. */
    VARIADIQUE,
    /* ...type de données. */
    TYPE_DE_DONNEES,
    /* ...nominal. */
    NOMINAL,
    /* ...de fonction. */
    FONCTION,
    /* ...de tuple. */
    TUPLE,
    /* ...de base (r32, z16, etc.). */
    BASIQUE,
};

using NomInstruction = std::variant<int, IdentifiantCode *>;

/* ------------------------------------------------------------------------- */
/** \name BaseSyntaxeuseRI
 * Classe de base pour la PréSyntaxeuseRI et la SyntaxeuseRI, elle contient la
 * logique principale de validation de la sémantique des instructions.
 * \{ */

template <typename Impl>
struct TypesSyntaxageRI;

template <typename Impl>
struct BaseSyntaxeuseRI : public BaseSyntaxeuse {
  protected:
    using TypeAtome = typename TypesSyntaxageRI<Impl>::TypeAtome;
    using TypeType = typename TypesSyntaxageRI<Impl>::TypeType;
    using TableauType = kuri::tablet<TypeType, 6>;

    struct ParamètreFonction {
        Lexème const *nom{};
        TypeType type{};
    };

    struct Fonction {
        Lexème *nom = nullptr;
        kuri::tablet<ParamètreFonction, 6> paramètres{};
        Lexème const *nom_retour = nullptr;
        TypeType type_retour{};
    };

    struct DonnéesMembreTypeComposé {
        Lexème *nom = nullptr;
        TypeType type{};
    };

    struct DonnéesTypeComposé {
        LexèmesType données_types_nominal{};
        kuri::tablet<DonnéesMembreTypeComposé, 6> membres{};
        TypeType type_sous_jacent{};
    };

    struct InfoInitMembreStructure {
        Lexème const *nom = nullptr;
        TypeAtome atome{};
    };

  public:
    BaseSyntaxeuseRI(Fichier *fichier);

    ~BaseSyntaxeuseRI() override;

  protected:
    void analyse_une_chose() override;

    void gère_erreur_rapportée(kuri::chaine_statique message_erreur) override;

    void analyse_fonction();
    void analyse_globale();
    void analyse_structure();
    void analyse_énum();
    void analyse_opaque();
    void analyse_union(bool est_nonsûre);
    void analyse_instructions();
    void analyse_instruction();

    [[nodiscard]] TypeAtome analyse_atome(TypeType const &type);
    [[nodiscard]] TypeAtome analyse_atome_typé();

    [[nodiscard]] std::optional<LexèmeType> apparie_lexème_type() const;
    [[nodiscard]] LexèmesType parse_type_nominal();
    [[nodiscard]] TypeType analyse_type();
    [[nodiscard]] TypeType analyse_type_référence();
    [[nodiscard]] TypeType analyse_type_pointeur();
    [[nodiscard]] TypeType analyse_type_tableau();
    [[nodiscard]] TypeType analyse_type_variadique();
    [[nodiscard]] TableauType analyse_paramètres_type();

    void analyse_alloue(NomInstruction nom);
    void analyse_appel(std::optional<NomInstruction> nom);
    void analyse_branche();
    void analyse_charge(NomInstruction nom);
    void analyse_index(NomInstruction nom);
    void analyse_label();
    void analyse_membre(NomInstruction nom);
    void analyse_retourne();
    void analyse_si();
    void analyse_stocke();
    void analyse_transtype(IdentifiantCode *ident, NomInstruction nom);
    void analyse_inatteignable();

    void analyse_opérateur_unaire(OpérateurUnaire::Genre genre, NomInstruction nom);
    void analyse_opérateur_binaire(OpérateurBinaire::Genre genre, NomInstruction nom);

    [[nodiscard]] Impl *impl()
    {
        return static_cast<Impl *>(this);
    }
};

template <typename Impl>
BaseSyntaxeuseRI<Impl>::BaseSyntaxeuseRI(Fichier *fichier) : BaseSyntaxeuse(fichier)
{
}

template <typename Impl>
BaseSyntaxeuseRI<Impl>::~BaseSyntaxeuseRI()
{
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_une_chose()
{
    if (apparie(ID::fonction)) {
        analyse_fonction();
    }
    else if (apparie(ID::structure)) {
        analyse_structure();
    }
    else if (apparie(ID::globale)) {
        analyse_globale();
    }
    else if (apparie(ID::opaque)) {
        analyse_opaque();
    }
    else if (apparie(ID::énum) || apparie(GenreLexème::ÉNUM)) {
        analyse_énum();
    }
    else if (apparie(GenreLexème::UNION)) {
        analyse_union(false);
    }
    else if (apparie(ID::union_nonsûre)) {
        analyse_union(true);
    }
    else {
        rapporte_erreur("Lexème inattendu\n");
    }
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::gère_erreur_rapportée(kuri::chaine_statique message_erreur)
{
    dbg() << message_erreur;
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_fonction()
{
    auto résultat = Fonction{};

    /* Saute « fonction ». */
    consomme();

    REQUIERS_LEXEME(CHAINE_CARACTERE, "attendu une chaine de caractère après « fonction »");
    auto nom_fonction = lexème_courant();
    consomme();

    résultat.nom = nom_fonction;

    CONSOMME_LEXEME(PARENTHESE_OUVRANTE,
                    "attendu une parenthèse ouvrante pour annoncer les paramètres de la fonction");

    /* Parsage des paramètres. */
    while (!fini() && !possède_erreur() && !apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        CONSOMME_IDENTIFIANT(param, "Attendu un identifiant de paramètre");

        auto paramètre = ParamètreFonction{};
        paramètre.nom = lexème_param;
        paramètre.type = analyse_type();

        résultat.paramètres.ajoute(paramètre);

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }
        consomme();
    }

    CONSOMME_LEXEME(PARENTHESE_FERMANTE,
                    "attendu une parenthèse fermante pour terminer les paramètres de la fonction");

    CONSOMME_LEXEME(RETOUR_TYPE, "attendu « -> » après  la parenthèse fermante");

    CONSOMME_IDENTIFIANT(nom_valeur_retour,
                         "Attendu une chaine de caractère pour le nom du retour.");

    résultat.nom_retour = lexème_nom_valeur_retour;
    résultat.type_retour = analyse_type();

    impl()->débute_fonction(résultat);

    CONSOMME_POINT_VIRGULE;

    analyse_instructions();
    impl()->termine_fonction();
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_globale()
{
    /* Saute « globale ». */
    consomme();

    CONSOMME_LEXEME(AROBASE, "Attendu « @ » avant le nom de la globale.");

    if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
        rapporte_erreur("Attendu une chaine de caractère pour le nom de la globale.");
        return;
    }

    auto nom_globale = m_lexème_courant;
    consomme();

    CONSOMME_LEXEME(EGAL, "Attendu « = » pour la valeur de la globale.");

    auto valeur = impl()->crée_atome_nul();
    TypeType type;

    if (apparie(ID::données_constantes)) {
        consomme();

        type = analyse_type();

        CONSOMME_LEXEME(CROCHET_OUVRANT,
                        "Attendu un crochet ouvrant pour commencer les données constantes.");

        valeur = impl()->parse_données_constantes(type);

        CONSOMME_LEXEME(CROCHET_FERMANT,
                        "Attendu un crochet fermant pour terminer les données constantes.");
    }
    else {
        type = analyse_type();

        if (!apparie(GenreLexème::POINT_VIRGULE)) {
            valeur = analyse_atome(type);
        }
    }

    impl()->crée_globale(nom_globale, type, valeur);

    CONSOMME_POINT_VIRGULE;
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_structure()
{
    /* Saute « structure ». */
    consomme();

    /* Nom de la structure. */
    auto données = parse_type_nominal();

    CONSOMME_LEXEME(EGAL, "Attendu « = » après le nom de la structure.");
    CONSOMME_LEXEME(ACCOLADE_OUVRANTE,
                    "Attendu « { » pour commencer la liste des membres de la structure.");

    kuri::tablet<DonnéesMembreTypeComposé, 6> membres;
    while (!fini()) {
        if (apparie(GenreLexème::ACCOLADE_FERMANTE)) {
            break;
        }

        if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine de caractère pour le nom du membre.");
            return;
        }

        auto nom_membre = lexème_courant();
        consomme();

        auto type_membre = analyse_type();

        membres.ajoute(DonnéesMembreTypeComposé{nom_membre, type_membre});
        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    CONSOMME_LEXEME(ACCOLADE_FERMANTE,
                    "Attendu « } » pour commencer la liste des membres de la structure.");

    DonnéesTypeComposé données_type;
    données_type.données_types_nominal = données;
    données_type.membres = membres;
    impl()->crée_déclaration_type_structure(données_type);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_union(bool est_nonsûre)
{
    /* Saute « union ». */
    consomme();

    /* Nom de l'union. */
    auto données = parse_type_nominal();
    auto union_anonyme = false;

    while (apparie(GenreLexème::BARRE)) {
        union_anonyme = true;
        consomme();

        données = parse_type_nominal();
    }

    if (union_anonyme) {
        if (apparie(GenreLexème::POINT_VIRGULE)) {
            consomme();
        }
        return;
    }

    CONSOMME_LEXEME(EGAL, "Attendu « = » après le nom de l'union.");
    CONSOMME_LEXEME(ACCOLADE_OUVRANTE,
                    "Attendu « { » pour commencer la liste des membres de la structure.");

    kuri::tablet<DonnéesMembreTypeComposé, 6> membres;
    while (!fini()) {
        if (apparie(GenreLexème::ACCOLADE_FERMANTE)) {
            break;
        }

        if (!apparie(GenreLexème::CHAINE_CARACTERE) && !apparie(GenreLexème::NOMBRE_ENTIER)) {
            rapporte_erreur("Attendu une chaine de caractère pour le nom du membre.");
            return;
        }

        auto nom_membre = lexème_courant();
        consomme();

        auto type_membre = analyse_type();

        membres.ajoute(DonnéesMembreTypeComposé{nom_membre, type_membre});
        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    CONSOMME_LEXEME(ACCOLADE_FERMANTE,
                    "Attendu « } » pour commencer la liste des membres de l'union.");

    DonnéesTypeComposé données_type;
    données_type.données_types_nominal = données;
    données_type.membres = membres;
    impl()->crée_déclaration_type_union(données_type, est_nonsûre);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_énum()
{
    /* Saute « énum ». */
    consomme();

    /* Nom de l'énum. */
    auto données = parse_type_nominal();

    CONSOMME_LEXEME(EGAL, "Attendu « = » après le nom de l'énum.");

    auto type_sous_jacent = analyse_type();

    DonnéesTypeComposé données_type;
    données_type.données_types_nominal = données;
    données_type.type_sous_jacent = type_sous_jacent;
    impl()->crée_déclaration_type_énum(données_type);

    CONSOMME_POINT_VIRGULE;
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_opaque()
{
    /* Saute « opaque ». */
    consomme();

    /* Nom de l'opaque. */
    auto données = parse_type_nominal();

    CONSOMME_LEXEME(EGAL, "Attendu « = » après le nom de l'opaque.");

    auto type_sous_jacent = analyse_type();

    DonnéesTypeComposé données_type;
    données_type.données_types_nominal = données;
    données_type.type_sous_jacent = type_sous_jacent;
    impl()->crée_déclaration_type_opaque(données_type);

    CONSOMME_POINT_VIRGULE;
}

static bool est_instruction(Lexème const *lexème)
{
    if (dls::outils::est_element(
            lexème->ident, ID::si, ID::appel, ID::retourne, ID::branche, ID::label, ID::stocke)) {
        return true;
    }

    if (lexème->genre == GenreLexème::POURCENT || lexème->genre == GenreLexème::RETOURNE) {
        return true;
    }

    return false;
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_instructions()
{
    while (!fini() && !possède_erreur()) {
        if (!est_instruction(lexème_courant())) {
            break;
        }

        analyse_instruction();
    }
}

static bool est_identifiant_instruction(GenreLexème const genre)
{
    switch (genre) {
        case GenreLexème::CHAINE_CARACTERE:
        case GenreLexème::CHARGE:
        case GenreLexème::RETOURNE:
        case GenreLexème::SI:
        {
            return true;
        }
        default:
        {
            return false;
        }
    }
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_instruction()
{
#define ANALYSE_INSTRUCTION(id)                                                                   \
    if (ident == ID::id || lexème_courant()->chaine == #id) {                                     \
        analyse_##id();                                                                           \
        return;                                                                                   \
    }

#define ANALYSE_INSTRUCTION_NOMMEE(id)                                                            \
    if (ident == ID::id || lexème_courant()->chaine == #id) {                                     \
        analyse_##id(nom);                                                                        \
        return;                                                                                   \
    }

#define ANALYSE_INSTRUCTION_TRANSTYPE(id)                                                         \
    if (ident == ID::id) {                                                                        \
        analyse_transtype(ident, nom);                                                            \
        return;                                                                                   \
    }

    auto ident = lexème_courant()->ident;

    ANALYSE_INSTRUCTION(label);
    ANALYSE_INSTRUCTION(stocke);
    ANALYSE_INSTRUCTION(si);
    ANALYSE_INSTRUCTION(branche);
    ANALYSE_INSTRUCTION(retourne);
    ANALYSE_INSTRUCTION(inatteignable);

    if (ident == ID::appel) {
        /* Ne peut utiliser le macro car nous n'avons pas de nom. */
        analyse_appel({});
        return;
    }

    if (!apparie(GenreLexème::POURCENT)) {
        rapporte_erreur("Attendu '%' pour introduire une instruction valorifère.");
        return;
    }
    consomme();

    auto nom = NomInstruction{};

    /* À FAIRE : numéro courant doit être le nombre de paramètres entrée/sortie de la fonction. */
    if (apparie(GenreLexème::NOMBRE_ENTIER)) {
        nom = int32_t(lexème_courant()->valeur_entiere);
        consomme();
    }
    else if (apparie(GenreLexème::CHAINE_CARACTERE)) {
        nom = lexème_courant()->ident;
        consomme();
    }
    else {
        rapporte_erreur("Attendu un nombre ou une chaine de caractère pour le nom de "
                        "l'instruction valorifère.");
        return;
    }

    if (!apparie(GenreLexème::EGAL)) {
        rapporte_erreur("Attendu '=' pour définir la valeur de l'instruction.");
        return;
    }
    consomme();

    REQUIERS_CONDITION(est_identifiant_instruction(lexème_courant()->genre),
                       "attendu l'identifiant d'une instruction");

    ident = lexème_courant()->ident;

    ANALYSE_INSTRUCTION_NOMMEE(alloue);
    ANALYSE_INSTRUCTION_NOMMEE(appel);
    ANALYSE_INSTRUCTION_NOMMEE(charge);
    ANALYSE_INSTRUCTION_NOMMEE(membre);
    ANALYSE_INSTRUCTION_NOMMEE(index);
    ANALYSE_INSTRUCTION_TRANSTYPE(augmente_naturel);
    ANALYSE_INSTRUCTION_TRANSTYPE(augmente_naturel_vers_relatif);
    ANALYSE_INSTRUCTION_TRANSTYPE(augmente_relatif);
    ANALYSE_INSTRUCTION_TRANSTYPE(augmente_relatif_vers_naturel);
    ANALYSE_INSTRUCTION_TRANSTYPE(augmente_réel);
    ANALYSE_INSTRUCTION_TRANSTYPE(diminue_naturel);
    ANALYSE_INSTRUCTION_TRANSTYPE(diminue_naturel_vers_relatif);
    ANALYSE_INSTRUCTION_TRANSTYPE(diminue_relatif);
    ANALYSE_INSTRUCTION_TRANSTYPE(diminue_relatif_vers_naturel);
    ANALYSE_INSTRUCTION_TRANSTYPE(diminue_réel);
    ANALYSE_INSTRUCTION_TRANSTYPE(entier_vers_pointeur);
    ANALYSE_INSTRUCTION_TRANSTYPE(naturel_vers_réel);
    ANALYSE_INSTRUCTION_TRANSTYPE(pointeur_vers_entier);
    ANALYSE_INSTRUCTION_TRANSTYPE(relatif_vers_réel);
    ANALYSE_INSTRUCTION_TRANSTYPE(réel_vers_naturel);
    ANALYSE_INSTRUCTION_TRANSTYPE(réel_vers_relatif);
    ANALYSE_INSTRUCTION_TRANSTYPE(transtype_bits);

#undef ANALYSE_INSTRUCTION

#define ENUMERE_GENRE_OPBINAIRE_EX(genre, id, op_code)                                            \
    if (lexème_courant()->chaine == #id) {                                                        \
        analyse_opérateur_binaire(OpérateurBinaire::Genre::genre, nom);                           \
        return;                                                                                   \
    }
    ENUMERE_OPERATEURS_BINAIRE
#undef ENUMERE_GENRE_OPBINAIRE_EX

#define ENUMERE_GENRE_OPUNAIRE_EX(genre, id)                                                      \
    if (lexème_courant()->chaine == #id) {                                                        \
        analyse_opérateur_unaire(OpérateurUnaire::Genre::genre, nom);                             \
        return;                                                                                   \
    }
    ENUMERE_OPERATEURS_UNAIRE
#undef ENUMERE_GENRE_OPUNAIRE_EX

    rapporte_erreur("instruction inconnue");
}

template <typename Impl>
std::optional<LexèmeType> BaseSyntaxeuseRI<Impl>::apparie_lexème_type() const
{
    if (apparie(GenreLexème::FOIS)) {
        return LexèmeType::POINTEUR;
    }
    if (apparie(GenreLexème::ESPERLUETTE)) {
        return LexèmeType::RÉFÉRENCE;
    }
    if (apparie(GenreLexème::CROCHET_OUVRANT)) {
        return LexèmeType::TABLEAU;
    }
    if (apparie(GenreLexème::TROIS_POINTS)) {
        return LexèmeType::VARIADIQUE;
    }
    if (apparie(GenreLexème::TYPE_DE_DONNÉES)) {
        return LexèmeType::TYPE_DE_DONNEES;
    }
    if (apparie(GenreLexème::CHAINE_CARACTERE)) {
        return LexèmeType::NOMINAL;
    }
    if (est_identifiant_type(lexème_courant()->genre)) {
        return LexèmeType::BASIQUE;
    }
    if (apparie(GenreLexème::NUL)) {
        return LexèmeType::BASIQUE;
    }
    if (apparie(GenreLexème::FONC)) {
        return LexèmeType::FONCTION;
    }
    if (apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
        return LexèmeType::TUPLE;
    }
    return {};
}

template <typename Impl>
LexèmesType BaseSyntaxeuseRI<Impl>::parse_type_nominal()
{
    LexèmesType résultat;
    résultat.ajoute(m_lexème_courant);
    consomme();

    while (apparie(GenreLexème::POINT)) {
        résultat.ajoute(m_lexème_courant);
        consomme();

        if (!apparie(GenreLexème::CHAINE_CARACTERE)) {
            rapporte_erreur("Attendu une chaine de caractère après '.'");
            break;
        }

        résultat.ajoute(m_lexème_courant);
        consomme();
    }

    return résultat;
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TypeType BaseSyntaxeuseRI<Impl>::analyse_type()
{
    auto const début_type = apparie_lexème_type();
    if (!début_type.has_value()) {
        rapporte_erreur("Lexème inconnu pour le type");
        return {};
    }

    switch (début_type.value()) {
        case LexèmeType::POINTEUR:
        {
            return analyse_type_pointeur();
        }
        case LexèmeType::RÉFÉRENCE:
        {
            return analyse_type_référence();
        }
        case LexèmeType::TABLEAU:
        {
            return analyse_type_tableau();
        }
        case LexèmeType::VARIADIQUE:
        {
            return analyse_type_variadique();
        }
        case LexèmeType::TYPE_DE_DONNEES:
        {
            auto lexème = m_lexème_courant;
            consomme();

            TypeType type_élément = {};
            if (apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
                consomme();
                type_élément = analyse_type();
                CONSOMME_LEXEME(PARENTHESE_FERMANTE, "Attendu une parenthèse fermante.", {});
            }

            return impl()->crée_type_type_de_données(lexème, type_élément);
        }
        case LexèmeType::NOMINAL:
        {
            auto donnée_type_nominal = parse_type_nominal();
            return impl()->crée_type_nomimal(donnée_type_nominal);
        }
        case LexèmeType::FONCTION:
        {
            auto lexème = m_lexème_courant;
            consomme();

            if (!apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
                rapporte_erreur("Attendu une parenthèse ouvrante pour les paramètres d'entrée du "
                                "type fonction");
                return {};
            }

            auto types_entrée = analyse_paramètres_type();

            if (!apparie(GenreLexème::PARENTHESE_OUVRANTE)) {
                rapporte_erreur("Attendu une parenthèse ouvrante pour les paramètres de sortie du "
                                "type fonction");
                return {};
            }

            auto types_sortie = analyse_paramètres_type();
            return impl()->crée_type_fonction(lexème, types_entrée, types_sortie);
        }
        case LexèmeType::TUPLE:
        {
            auto lexème = m_lexème_courant;
            auto types = analyse_paramètres_type();
            return impl()->crée_type_tuple(lexème, types);
        }
        case LexèmeType::BASIQUE:
        {
            auto lexème = m_lexème_courant;
            consomme();
            return impl()->crée_type_basique(lexème);
        }
    }

    return {};
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TypeType BaseSyntaxeuseRI<Impl>::analyse_type_référence()
{
    auto lexème = m_lexème_courant;
    consomme();
    auto type_pointé = analyse_type();
    return impl()->crée_type_référence(lexème, type_pointé);
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TypeType BaseSyntaxeuseRI<Impl>::analyse_type_pointeur()
{
    auto lexème = m_lexème_courant;
    consomme();
    auto type_pointé = analyse_type();
    return impl()->crée_type_pointeur(lexème, type_pointé);
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TypeType BaseSyntaxeuseRI<Impl>::analyse_type_tableau()
{
    auto lexème_crochet_ouvrant = m_lexème_courant;
    consomme();

    if (apparie(GenreLexème::CROCHET_FERMANT)) {
        auto lexème_crochet_fermant = m_lexème_courant;
        consomme();
        auto type_élément = analyse_type();
        return impl()->crée_type_tranche(
            lexème_crochet_ouvrant, lexème_crochet_fermant, type_élément);
    }

    auto taille = 0;
    if (apparie(GenreLexème::NOMBRE_ENTIER)) {
        taille = int32_t(m_lexème_courant->valeur_entiere);
        consomme();
    }
    else if (apparie(GenreLexème::DEUX_POINTS)) {
        consomme();
    }
    else {
        rapporte_erreur("Attendu un nombre entier ou '..' pour le spécifiant de type tableau.");
        return {};
    }

    if (!apparie(GenreLexème::CROCHET_FERMANT)) {
        rapporte_erreur(
            "Attendu un crochet fermant pour terminer la déclaration du type tableau.");
        return {};
    }
    auto lexème_crochet_fermant = m_lexème_courant;
    consomme();

    auto type_élément = analyse_type();

    if (taille != 0) {
        return impl()->crée_type_tableau_fixe(
            lexème_crochet_ouvrant, lexème_crochet_fermant, type_élément, taille);
    }

    return impl()->crée_type_tableau_dynamique(
        lexème_crochet_ouvrant, lexème_crochet_fermant, type_élément);
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TypeType BaseSyntaxeuseRI<Impl>::analyse_type_variadique()
{
    auto lexème = m_lexème_courant;
    consomme();
    auto const début_type = apparie_lexème_type();
    TypeType type_élément = {};
    if (début_type.has_value()) {
        type_élément = analyse_type();
    }
    return impl()->crée_type_variadique(lexème, type_élément);
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TableauType BaseSyntaxeuseRI<Impl>::analyse_paramètres_type()
{
    assert(apparie(GenreLexème::PARENTHESE_OUVRANTE));

    TableauType résultat;
    consomme();

    while (!fini()) {
        if (apparie(GenreLexème::PARENTHESE_FERMANTE)) {
            break;
        }

        résultat.ajoute(analyse_type());
        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }

        consomme();
    }

    if (!apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        rapporte_erreur("Attendu une parenthèse fermante pour finir les paramètres du type");
        return {};
    }

    consomme();
    return résultat;
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TypeAtome BaseSyntaxeuseRI<Impl>::analyse_atome_typé()
{
    auto const type = analyse_type();
    return analyse_atome(type);
}

template <typename Impl>
typename BaseSyntaxeuseRI<Impl>::TypeAtome BaseSyntaxeuseRI<Impl>::analyse_atome(
    TypeType const &type)
{
    if (apparie(GenreLexème::POURCENT)) {
        consomme();

        if (apparie(GenreLexème::NOMBRE_ENTIER) || apparie(GenreLexème::CHAINE_CARACTERE)) {
            auto const lexème_id = lexème_courant();
            consomme();
            return impl()->crée_référence_instruction(type, lexème_id);
        }

        rapporte_erreur(
            "attendu un nombre entier ou une chaine de caractère pour nommer l'instruction");
        return {};
    }

    if (apparie(GenreLexème::ACCOLADE_OUVRANTE)) {
        consomme();

        kuri::tablet<InfoInitMembreStructure, 6> membres;

        while (!fini()) {
            if (apparie(GenreLexème::ACCOLADE_FERMANTE)) {
                break;
            }

            CONSOMME_IDENTIFIANT(
                nom_membre, "Attendu une chaine de caractère pour le nom du membre", {});

            CONSOMME_LEXEME(EGAL, "Attendu « = » pour la valeur du membre.", {});

            auto atome_membre = analyse_atome_typé();

            membres.ajoute({lexème_nom_membre, atome_membre});

            if (!apparie(GenreLexème::VIRGULE)) {
                break;
            }

            consomme();
        }

        CONSOMME_LEXEME(ACCOLADE_FERMANTE,
                        "Attendu une accolade fermante pour terminer la structure constante.",
                        {});

        return impl()->crée_construction_structure(type, membres);
    }

    if (apparie(GenreLexème::NOMBRE_ENTIER)) {
        auto const lexème_id = m_lexème_courant;
        consomme();
        return impl()->crée_constante_entière(type, lexème_id);
    }

    if (apparie(GenreLexème::NOMBRE_REEL)) {
        auto const lexème_id = m_lexème_courant;
        consomme();
        return impl()->crée_constante_réelle(type, lexème_id);
    }

    if (apparie(GenreLexème::MOINS) || apparie(GenreLexème::MOINS_UNAIRE)) {
        consomme();

        if (apparie(GenreLexème::NOMBRE_ENTIER)) {
            auto const lexème_id = m_lexème_courant;
            m_lexème_courant->valeur_entiere = -m_lexème_courant->valeur_entiere;
            consomme();
            return impl()->crée_constante_entière(type, lexème_id);
        }

        if (apparie(GenreLexème::NOMBRE_REEL)) {
            auto const lexème_id = m_lexème_courant;
            m_lexème_courant->valeur_reelle = -m_lexème_courant->valeur_reelle;
            consomme();
            return impl()->crée_constante_réelle(type, lexème_id);
        }

        rapporte_erreur("Attendu un nombre entier ou réel après « - »");
        return {};
    }

    if (apparie(GenreLexème::NUL)) {
        consomme();
        return impl()->crée_constante_nulle(type);
    }

    if (apparie(GenreLexème::AROBASE)) {
        consomme();
        CONSOMME_IDENTIFIANT(globale, "Attendu l'identifiant de la globale après « @ »", {});

        auto globale = impl()->crée_référence_globale(type, lexème_globale);

        if (apparie(GenreLexème::CROCHET_OUVRANT)) {
            consomme();
            CONSOMME_NOMBRE_ENTIER(
                index_accès,
                "Attendu un nombre entier après le crochet ouvrant de l'accès d'index",
                {});

            CONSOMME_LEXEME(
                CROCHET_FERMANT, "Attendu un crochet pour terminer l'accès d'index constant.", {});

            return impl()->crée_indexage_constant(type, lexème_index_accès, globale);
        }

        return globale;
    }

    if (apparie(GenreLexème::TAILLE_DE)) {
        auto const lexème_taille_de = m_lexème_courant;
        consomme();

        CONSOMME_LEXEME(
            PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après « taille_de »", {});

        auto type_de_données = analyse_type();

        CONSOMME_LEXEME(PARENTHESE_FERMANTE,
                        "Attendu une parenthèse fermante après le type de « taille_de »",
                        {});
        return impl()->crée_taille_de(lexème_taille_de, type_de_données);
    }

    if (apparie(ID::index_de)) {
        auto const lexème_index_de = m_lexème_courant;
        consomme();

        CONSOMME_LEXEME(
            PARENTHESE_OUVRANTE, "Attendu une parenthèse ouvrante après « index_de »", {});

        auto type_de_données = analyse_type();

        CONSOMME_LEXEME(PARENTHESE_FERMANTE,
                        "Attendu une parenthèse fermante après le type de « index_de »",
                        {});

        return impl()->crée_index_de(lexème_index_de, type_de_données);
    }

    if (apparie(ID::transtype)) {
        auto const lexème_transtype = m_lexème_courant;
        consomme();
        auto atome_transtypé = analyse_atome_typé();
        if (!apparie(ID::vers)) {
            rapporte_erreur("Attendu « vers » pour le type destiné du transtypage");
            return {};
        }
        consomme();
        auto type_destiné = analyse_type();
        return impl()->crée_transtypage_constant(lexème_transtype, atome_transtypé, type_destiné);
    }

    if (apparie(ID::init_tableau)) {
        auto const lexème_init = m_lexème_courant;
        consomme();
        return impl()->parse_init_tableau(type, lexème_init);
    }

    /* Test les chaines en dernier pour ne pas confondre les mot-clés sans lexème spécifique avec
     * des fonctions. */
    if (apparie(GenreLexème::CHAINE_CARACTERE)) {
        auto const lexème_fonction = m_lexème_courant;
        consomme();
        return impl()->parse_référence_fonction(type, lexème_fonction);
    }

    if (apparie(GenreLexème::CROCHET_OUVRANT)) {
        consomme();

        kuri::tablet<TypeAtome, 6> valeurs;

        while (!fini()) {
            if (apparie(GenreLexème::CROCHET_FERMANT)) {
                break;
            }

            auto atome = analyse_atome_typé();
            valeurs.ajoute(atome);

            if (!apparie(GenreLexème::VIRGULE)) {
                break;
            }
            consomme();
        }

        CONSOMME_LEXEME(CROCHET_FERMANT, "Attendu un crochet fermant pour terminer l'atome.", {});
        return impl()->crée_construction_tableau(type, valeurs);
    }

    rapporte_erreur("lexème inattendu pour l'atome");
    return {};
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_alloue(NomInstruction nom)
{
    consomme();
    auto const type = analyse_type();
    CONSOMME_POINT_VIRGULE;

    IdentifiantCode *ident = nullptr;
    if (std::holds_alternative<IdentifiantCode *>(nom)) {
        ident = std::get<IdentifiantCode *>(nom);
    }

    impl()->crée_allocation(type, ident);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_stocke()
{
    consomme();
    auto atome_cible = analyse_atome_typé();
    CONSOMME_LEXEME(VIRGULE, "attendu une virgule après la destination de « stocke »");
    auto const atome_valeur = analyse_atome_typé();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_stocke(atome_cible, atome_valeur);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_charge(NomInstruction nom)
{
    consomme();
    auto atome_chargé = analyse_atome_typé();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_charge(atome_chargé);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_label()
{
    consomme();
    REQUIERS_NOMBRE_ENTIER("attendu un nombre entier après « label »");
    auto const id_label = lexème_courant()->valeur_entiere;
    consomme();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_label(id_label);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_transtype(IdentifiantCode *ident, NomInstruction nom)
{
    consomme();
    auto const atome_valeur = analyse_atome_typé();
    CONSOMME_IDENTIFIANT_CODE(vers, "attendu « vers » après l'atome du transtypage");
    auto type = analyse_type();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_transtype(ident, atome_valeur, type);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_index(NomInstruction nom)
{
    consomme();
    consomme();
    auto const atome_indexé = analyse_atome_typé();
    CONSOMME_LEXEME(VIRGULE, "attendu une virgule après l'atome de « index »");
    auto const atome_index = analyse_atome_typé();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_index(atome_indexé, atome_index);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_membre(NomInstruction nom)
{
    consomme();
    auto const atome_membre = analyse_atome_typé();
    CONSOMME_LEXEME(VIRGULE, "attendu une virgule après l'atome de « membre »");
    REQUIERS_NOMBRE_ENTIER("attendu un nombre entier pour l'index du membre");
    auto const index_membre = lexème_courant()->valeur_entiere;
    consomme();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_membre(atome_membre, index_membre);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_si()
{
    consomme();

    auto const atome_prédicat = analyse_atome_typé();

    CONSOMME_IDENTIFIANT_CODE(alors, "attendu « alors » après l'atome de « si »");
    CONSOMME_LEXEME(POURCENT, "attendu '%' après « alors »");
    CONSOMME_NOMBRE_ENTIER(si_vrai, "attendu un nombre entier après '%'");

    CONSOMME_LEXEME(SINON, "attendu « sinon » après l'atome de « alors »");
    CONSOMME_LEXEME(POURCENT, "attendu '%' après « sinon »");
    CONSOMME_NOMBRE_ENTIER(si_faux, "attendu un nombre entier après '%'");

    CONSOMME_POINT_VIRGULE;

    impl()->crée_si(
        atome_prédicat, lexème_si_vrai->valeur_entiere, lexème_si_faux->valeur_entiere);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_retourne()
{
    consomme();

    if (apparie(GenreLexème::POINT_VIRGULE)) {
        consomme();
        /* Aucune valeur de retour. */
        impl()->crée_retourne({});
        return;
    }

    auto const valeur = analyse_atome_typé();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_retourne(valeur);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_appel(std::optional<NomInstruction> nom)
{
    consomme();
    auto const atome_fonction = analyse_atome_typé();
    CONSOMME_LEXEME(PARENTHESE_OUVRANTE,
                    "attendu une parenthèse ouvrante pour les arguments de la fonction");
    kuri::tablet<typename BaseSyntaxeuseRI<Impl>::TypeAtome, 16> arguments;
    while (!fini() && !possède_erreur() && !apparie(GenreLexème::PARENTHESE_FERMANTE)) {
        auto atome = analyse_atome_typé();
        arguments.ajoute(atome);

        if (!apparie(GenreLexème::VIRGULE)) {
            break;
        }
        consomme();
    }
    CONSOMME_LEXEME(PARENTHESE_FERMANTE,
                    "attendu une parenthèse fermante après les arguments de la fonction");
    CONSOMME_POINT_VIRGULE;

    impl()->crée_appel(atome_fonction, arguments);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_branche()
{
    consomme();

    CONSOMME_LEXEME(POURCENT, "attendu '%' après « branche »");
    CONSOMME_NOMBRE_ENTIER(id_cible, "attendu un nombre entier après '%'");
    CONSOMME_POINT_VIRGULE;

    impl()->crée_branche(lexème_id_cible->valeur_entiere);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_opérateur_unaire(OpérateurUnaire::Genre genre,
                                                      NomInstruction nom)
{
    consomme();

    auto const atome = analyse_atome_typé();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_opérateur_unaire(genre, atome);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_opérateur_binaire(OpérateurBinaire::Genre genre,
                                                       NomInstruction nom)
{
    consomme();

    auto const atome_gauche = analyse_atome_typé();
    CONSOMME_LEXEME(VIRGULE, "attendu une virgule après l'atome gauche de l'opérateur");
    auto const atome_droite = analyse_atome_typé();
    CONSOMME_POINT_VIRGULE;

    impl()->crée_opérateur_binaire(genre, atome_gauche, atome_droite);
}

template <typename Impl>
void BaseSyntaxeuseRI<Impl>::analyse_inatteignable()
{
    consomme();
    impl()->crée_inatteignable();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name PrésyntaxeuseRI.
 * Performe le présyntaxage d'un script RI. Ceci rassemble des données globales
 * et autres informations afin de syntaxer le script dans le désordre (p.e. les
 * fonctions récursives peuvent dépendre d'une fonction qui ne fut pas encore
 * définie, donc le présyntaxage nous permet de savoir que la fonction
 * indéfinie le sera plus tard dans le programme).
 * \{ */

class PrésyntaxeuseRI;

struct DescriptionAtome {
    Atome::Genre genre = {};
    Lexème const *lexème = nullptr;
    LexèmesType desc_type{};
};

template <>
struct TypesSyntaxageRI<PrésyntaxeuseRI> {
    using TypeAtome = DescriptionAtome;
    using TypeType = LexèmesType;
};

class PrésyntaxeuseRI : public BaseSyntaxeuseRI<PrésyntaxeuseRI> {
  public:
    struct DéclarationGlobale {
        Lexème const *lexème_nom = nullptr;
        LexèmesType type{};
    };

    struct DonnéesPréparsage {
        kuri::tableau<DonnéesTypeComposé> structures{};
        kuri::tableau<DonnéesTypeComposé> unions{};
        kuri::tableau<DonnéesTypeComposé> énums{};
        kuri::tableau<DonnéesTypeComposé> opaques{};
        kuri::tableau<DéclarationGlobale> globales{};
        kuri::tableau<Fonction> fonctions{};
    };

  private:
    DonnéesPréparsage m_données_préparsage{};

  public:
    using BaseSyntaxeuseRI::BaseSyntaxeuseRI;

    DonnéesPréparsage const *donne_données_préparsage() const;

    /* Types. */
    LexèmesType crée_type_pointeur(Lexème const *lexème, LexèmesType const &type_pointé);

    LexèmesType crée_type_référence(Lexème const *lexème, LexèmesType const &type_pointé);

    LexèmesType crée_type_tableau_dynamique(Lexème const *crochet_ouvrant,
                                            Lexème const *crochet_fermant,
                                            LexèmesType const &type_élément);

    LexèmesType crée_type_tableau_fixe(Lexème const *crochet_ouvrant,
                                       Lexème const *crochet_fermant,
                                       LexèmesType const &type_élément,
                                       int32_t taille);

    LexèmesType crée_type_tranche(Lexème const *crochet_ouvrant,
                                  Lexème const *crochet_fermant,
                                  LexèmesType const &type_élément);

    LexèmesType crée_type_variadique(Lexème const *lexème, LexèmesType const &type_élément);

    LexèmesType crée_type_type_de_données(Lexème const *lexème, LexèmesType const &type);

    LexèmesType crée_type_nomimal(kuri::tableau_statique<Lexème *> lexèmes);

    LexèmesType crée_type_basique(Lexème const *lexème);

    LexèmesType crée_type_fonction(Lexème const *lexème,
                                   kuri::tableau_statique<LexèmesType> types_entrée,
                                   kuri::tableau_statique<LexèmesType> types_sortie);

    LexèmesType crée_type_tuple(Lexème const *lexème, kuri::tableau_statique<LexèmesType> types);

    /* Atomes. */

    DescriptionAtome crée_atome_nul() const;

    DescriptionAtome parse_données_constantes(LexèmesType const &type);

    void crée_globale(Lexème const *lexème,
                      LexèmesType const &type,
                      DescriptionAtome const &initialisateur);

    DescriptionAtome crée_référence_instruction(LexèmesType const &type, Lexème const *lexème);

    DescriptionAtome crée_construction_structure(
        LexèmesType const &type, kuri::tableau_statique<InfoInitMembreStructure> membres);

    DescriptionAtome crée_constante_entière(LexèmesType const &type, Lexème const *lexème);

    DescriptionAtome crée_constante_réelle(LexèmesType const &type, Lexème const *lexème);

    DescriptionAtome crée_constante_nulle(LexèmesType const &type);

    DescriptionAtome crée_référence_globale(LexèmesType const &type, Lexème const *lexème);

    DescriptionAtome crée_indexage_constant(LexèmesType const &type,
                                            Lexème const *lexème_nombre,
                                            DescriptionAtome const &globale);

    DescriptionAtome crée_taille_de(Lexème const *lexème, LexèmesType const &type);

    DescriptionAtome crée_index_de(Lexème const *lexème, LexèmesType const &type);

    DescriptionAtome crée_transtypage_constant(Lexème const *lexème,
                                               DescriptionAtome const &atome_transtypé,
                                               LexèmesType const &type_destination);

    DescriptionAtome parse_init_tableau(LexèmesType const &type, Lexème const *lexème);

    DescriptionAtome parse_référence_fonction(LexèmesType const &type, Lexème const *lexème);

    DescriptionAtome crée_construction_tableau(LexèmesType const &type,
                                               kuri::tableau_statique<DescriptionAtome> valeurs);

    void crée_déclaration_type_structure(DonnéesTypeComposé const &données);

    void crée_déclaration_type_union(DonnéesTypeComposé const &données, bool est_nonsûre);

    void crée_déclaration_type_énum(DonnéesTypeComposé const &données);

    void crée_déclaration_type_opaque(DonnéesTypeComposé const &données);

    void débute_fonction(Fonction const &fonction);

    void termine_fonction();

    void crée_allocation(LexèmesType const &type, IdentifiantCode *ident);

    void crée_appel(DescriptionAtome const &atome_fonction,
                    kuri::tableau_statique<DescriptionAtome> arguments);

    void crée_branche(uint64_t cible);

    void crée_charge(DescriptionAtome const &valeur);

    void crée_index(DescriptionAtome const &indexé, DescriptionAtome const &valeur);

    void crée_label(uint64_t index);

    void crée_membre(DescriptionAtome const &valeur, uint64_t index);

    void crée_retourne(DescriptionAtome const &valeur);

    void crée_si(DescriptionAtome const &prédicat, uint64_t si_vrai, uint64_t si_faux);

    void crée_stocke(DescriptionAtome const &cible, DescriptionAtome const &valeur);

    void crée_transtype(IdentifiantCode *ident,
                        DescriptionAtome const &valeur,
                        LexèmesType const &type);

    void crée_opérateur_unaire(OpérateurUnaire::Genre genre, DescriptionAtome const &opérande);

    void crée_opérateur_binaire(OpérateurBinaire::Genre genre,
                                DescriptionAtome const &gauche,
                                DescriptionAtome const &droite);

    void crée_inatteignable();

  private:
    int32_t numéro_instruction_courante = 0;

    void imprime_numéro_instruction(bool numérote, IdentifiantCode const *ident = nullptr);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name SyntaxeuseRI.
 * Cette classe crée les fonctions et globales depuis un script RI.
 * \{ */

class SyntaxeuseRI;

template <>
struct TypesSyntaxageRI<SyntaxeuseRI> {
    using TypeAtome = Atome *;
    using TypeType = Type *;
};

class SyntaxeuseRI : public BaseSyntaxeuseRI<SyntaxeuseRI> {
    using TypeTrieBloc = kuri::trie<IdentifiantCode *, NoeudBloc *>;
    using TypeNoeudTrieBloc = TypeTrieBloc::Noeud;

    using TypeTrie = kuri::trie<IdentifiantCode *, NoeudDéclarationType *>;
    using TypeNoeudTrie = TypeTrie::Noeud;

    TypeTrie m_trie_types{};
    TypeTrieBloc m_trie_blocs{};

    AllocatriceNoeud m_allocatrice{};
    AssembleuseArbre m_assembleuse{m_allocatrice};
    Typeuse &m_typeuse;

    ConstructriceRI m_constructrice;

    kuri::table_hachage<IdentifiantCode *, AtomeGlobale *> m_table_globales{"table_globales"};
    kuri::table_hachage<IdentifiantCode *, AtomeFonction *> m_table_fonctions{"table_fonctions"};

    kuri::tableau<AtomeFonction *> m_fonctions{};

    AtomeFonction *m_fonction_courante = nullptr;
    int32_t m_décalage_instructions = 0;

    struct LabelRéservé {
        InstructionLabel *label = nullptr;
        int32_t cible = 0;
    };

    kuri::tableau<LabelRéservé, int> m_labels_réservés{};

  public:
    SyntaxeuseRI(Fichier *fichier,
                 Typeuse &typeuse,
                 RegistreSymboliqueRI &registre,
                 PrésyntaxeuseRI &pré_syntaxeuse);

    EMPECHE_COPIE(SyntaxeuseRI);

    kuri::tableau_statique<AtomeFonction *> donne_fonctions() const;

    ConstructriceRI &donne_constructrice();

    /* Types. */
    Type *crée_type_pointeur(Lexème const *lexème, Type *type_pointé);

    Type *crée_type_référence(Lexème const *lexème, Type *type_pointé);

    Type *crée_type_tableau_dynamique(Lexème const *crochet_ouvrant,
                                      Lexème const *crochet_fermant,
                                      Type *type_élément);

    Type *crée_type_tableau_fixe(Lexème const *crochet_ouvrant,
                                 Lexème const *crochet_fermant,
                                 Type *type_élément,
                                 int32_t taille);

    Type *crée_type_tranche(Lexème const *crochet_ouvrant,
                            Lexème const *crochet_fermant,
                            Type *type_élément);

    Type *crée_type_variadique(Lexème const *lexème, Type *type_élément);

    Type *crée_type_type_de_données(Lexème const *lexème, Type *type);

    Type *crée_type_nomimal(kuri::tableau_statique<Lexème *> lexèmes);

    Type *crée_type_basique(Lexème const *lexème);

    Type *crée_type_fonction(Lexème const *lexème,
                             kuri::tableau_statique<Type *> types_entrée,
                             kuri::tableau_statique<Type *> types_sortie);

    Type *crée_type_tuple(Lexème const *lexème, kuri::tableau_statique<Type *> types);

    /* Atomes. */

    Atome *crée_atome_nul() const;

    Atome *parse_données_constantes(Type *type);

    void crée_globale(Lexème const *lexème, Type *type, Atome *initialisateur);

    Atome *crée_construction_structure(Type *type,
                                       kuri::tableau_statique<InfoInitMembreStructure> membres);

    Atome *crée_constante_entière(Type *type, Lexème const *lexème);

    Atome *crée_constante_réelle(Type *type, Lexème const *lexème);

    Atome *crée_constante_nulle(Type *type);

    Atome *crée_référence_globale(Type *type, Lexème const *lexème);

    Atome *crée_indexage_constant(Type *type, Lexème const *lexème_nombre, Atome *globale);

    Atome *crée_taille_de(Lexème const *lexème, Type *type);

    Atome *crée_index_de(Lexème const *lexème, Type *type);

    Atome *crée_transtypage_constant(Lexème const *lexème,
                                     Atome *atome_transtypé,
                                     Type *type_destination);

    Atome *parse_init_tableau(Type *type, Lexème const *lexème);

    Atome *parse_référence_fonction(Type *type, Lexème const *lexème);

    Atome *crée_construction_tableau(Type *type, kuri::tableau_statique<Atome *> atomes);

    void crée_déclaration_type_structure(DonnéesTypeComposé const &données);

    void crée_déclaration_type_union(DonnéesTypeComposé const &données, bool est_nonsûre);

    void crée_déclaration_type_énum(DonnéesTypeComposé const &données);

    void crée_déclaration_type_opaque(DonnéesTypeComposé const &données);

    void débute_fonction(Fonction const &données_fonction);

    void termine_fonction();

    /* Instruction. */

    Atome *crée_référence_instruction(Type *type, Lexème const *lexème);

    void crée_allocation(Type *type, IdentifiantCode *ident);

    void crée_appel(Atome *atome, kuri::tableau_statique<Atome *> arguments);

    void crée_branche(uint64_t cible);

    void crée_charge(Atome *valeur);

    void crée_index(Atome *indexé, Atome *valeur);

    void crée_label(uint64_t index);

    void crée_membre(Atome *valeur, uint64_t index);

    void crée_retourne(Atome *valeur);

    void crée_si(Atome *prédicat, uint64_t si_vrai, uint64_t si_faux);

    void crée_stocke(Atome *cible, Atome *valeur);

    void crée_transtype(IdentifiantCode *ident, Atome *valeur, Type *type);

    void crée_opérateur_unaire(OpérateurUnaire::Genre genre, Atome *opérande);

    void crée_opérateur_binaire(OpérateurBinaire::Genre genre, Atome *gauche, Atome *droite);

    void crée_inatteignable();

  private:
    NoeudBloc *donne_bloc_parent_pour_type(kuri::tableau_statique<Lexème *> lexèmes);

    NoeudDéclarationType *donne_déclaration_pour_type_nominale(
        kuri::tableau_statique<Lexème *> lexèmes);

    NoeudDéclarationType *crée_type_préparsé(kuri::tableau_statique<Lexème *> données_type_nominal,
                                             GenreNoeud genre);

    NoeudDéclarationType *crée_type_nominal_pour_genre(Lexème const *lexème, GenreNoeud genre);

    InstructionLabel *donne_label_pour_cible(int32_t cible);
};

/** \} */
