/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "biblinternes/langage/erreur.hh"
#include "biblinternes/outils/conditions.h"

#include <iostream>

#include "arbre_syntaxique/assembleuse.hh"

#include "compilation/graphe_dependance.hh"
#include "compilation/operateurs.hh"
#include "compilation/typage.hh"

#include "parsage/base_syntaxeuse.hh"
#include "parsage/gerante_chaine.hh"
#include "parsage/identifiant.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/impression.hh"

#include "structures/chemin_systeme.hh"
#include "structures/trie.hh"

#include "utilitaires/log.hh"

static void imprime_erreur(SiteSource site, kuri::chaine message)
{
    auto fichier = site.fichier;
    auto index_ligne = site.index_ligne;
    auto index_colonne = site.index_colonne;

    auto ligne_courante = fichier->tampon()[index_ligne];

    Enchaineuse enchaineuse;
    enchaineuse << "Erreur : " << fichier->chemin() << ":" << index_ligne + 1 << ":\n";
    enchaineuse << ligne_courante;

    /* La position ligne est en octet, il faut donc compter le nombre d'octets
     * de chaque point de code pour bien formater l'erreur. */
    for (auto i = 0l; i < index_colonne;) {
        if (ligne_courante[i] == '\t') {
            enchaineuse << '\t';
        }
        else {
            enchaineuse << ' ';
        }

        i += lng::decalage_pour_caractere(ligne_courante, i);
    }

    enchaineuse << "^~~~\n";
    enchaineuse << message;

    dbg() << enchaineuse.chaine();
}

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

    void gère_erreur_rapportée(const kuri::chaine &message_erreur) override;

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
    else if (apparie(ID::énum) || apparie(GenreLexème::ENUM)) {
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
void BaseSyntaxeuseRI<Impl>::gère_erreur_rapportée(const kuri::chaine &message_erreur)
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
    if (apparie(GenreLexème::TYPE_DE_DONNEES)) {
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

/* ------------------------------------------------------------------------- */
/** \name PrésyntaxeuseRI.
 * \{ */

#undef IMPRIME_RI

struct DescriptionAtome {
    Atome::Genre genre = {};
    Lexème const *lexème = nullptr;
    LexèmesType desc_type{};
};

#ifdef IMPRIME_RI
static kuri::chaine chaine_type(kuri::tableau_statique<Lexème *> lexèmes)
{
    Enchaineuse résultat;
    POUR (lexèmes) {
        résultat << it->chaine;
    }
    return résultat.chaine();
}

static std::ostream &operator<<(std::ostream &os, DescriptionAtome desc)
{
    if (!desc.desc_type.est_vide()) {
        os << chaine_type(desc.desc_type) << ' ';
    }

    switch (desc.genre) {
        case Atome::Genre::INSTRUCTION:
        {
            if (desc.lexème->genre == GenreLexème::CHAINE_CARACTERE) {
                os << "%" << desc.lexème->ident->nom;
            }
            else {
                os << "%" << desc.lexème->valeur_entiere;
            }
            break;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            os << desc.lexème->valeur_entiere;
            break;
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            os << desc.lexème->valeur_reelle;
            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            os << "nul";
            break;
        }
        case Atome::Genre::GLOBALE:
        {
            os << "@" << desc.lexème->chaine;
            break;
        }
        case Atome::Genre::FONCTION:
        {
            os << desc.lexème->chaine;
            break;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            os << "{}";
            break;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            os << "[]";
            break;
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            os << "taille_de(" << chaine_type(desc.desc_type) << ")";
            break;
        }
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        {
            os << "index_de(" << chaine_type(desc.desc_type) << ")";
            break;
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            os << "index constant";
            break;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            os << "données_constantes";
            break;
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            os << "constante caractère";
            break;
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            os << "constante booléenne";
            break;
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            os << "constante type";
            break;
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            os << "init_tableau";
            break;
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            os << "transtype constant";
            break;
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            os << "---";
            break;
        }
    }

    return os;
}
#endif

class PrésyntaxeuseRI;

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

    DonnéesPréparsage const *donne_données_préparsage() const
    {
        return &m_données_préparsage;
    }

    /* Types. */
    LexèmesType crée_type_pointeur(Lexème const *lexème, LexèmesType const &type_pointé)
    {
        return {};
    }

    LexèmesType crée_type_référence(Lexème const *lexème, LexèmesType const &type_pointé)
    {
        return {};
    }

    LexèmesType crée_type_tableau_dynamique(Lexème const *crochet_ouvrant,
                                            Lexème const *crochet_fermant,
                                            LexèmesType const &type_élément)
    {
        return {};
    }

    LexèmesType crée_type_tableau_fixe(Lexème const *crochet_ouvrant,
                                       Lexème const *crochet_fermant,
                                       LexèmesType const &type_élément,
                                       int32_t taille)
    {
        return {};
    }

    LexèmesType crée_type_tranche(Lexème const *crochet_ouvrant,
                                  Lexème const *crochet_fermant,
                                  LexèmesType const &type_élément)
    {
        return {};
    }

    LexèmesType crée_type_variadique(Lexème const *lexème, LexèmesType const &type_élément)
    {
        return {};
    }

    LexèmesType crée_type_type_de_données(Lexème const *lexème, LexèmesType const &type)
    {
        return {};
    }

    LexèmesType crée_type_nomimal(kuri::tableau_statique<Lexème *> lexèmes)
    {
        return {};
    }

    LexèmesType crée_type_basique(Lexème const *lexème)
    {
        return {};
    }

    LexèmesType crée_type_fonction(Lexème const *lexème,
                                   kuri::tableau_statique<LexèmesType> types_entrée,
                                   kuri::tableau_statique<LexèmesType> types_sortie)
    {
        return {};
    }

    LexèmesType crée_type_tuple(Lexème const *lexème, kuri::tableau_statique<LexèmesType> types)
    {
        return {};
    }

    /* Atomes. */

    DescriptionAtome crée_atome_nul() const
    {
        return {Atome::Genre::CONSTANTE_NULLE, nullptr, {}};
    }

    DescriptionAtome parse_données_constantes(LexèmesType const &type)
    {
        while (!fini()) {
            if (apparie(GenreLexème::CROCHET_FERMANT)) {
                break;
            }

            consomme();
        }

        return {Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES, nullptr, type};
    }

    void crée_globale(Lexème const *lexème,
                      LexèmesType const &type,
                      DescriptionAtome const &initialisateur)
    {
        m_données_préparsage.globales.ajoute({lexème, type});

#ifdef IMPRIME_RI
        std::cerr << "globale @" << lexème->chaine << " = ";
        if (initialisateur.lexème == nullptr) {
            std::cerr << chaine_type(type);
        }
        else {
            std::cerr << initialisateur;
        }
        std::cerr << "\n";
#endif
    }

    DescriptionAtome crée_référence_instruction(LexèmesType const &type, Lexème const *lexème)
    {
        return {Atome::Genre::INSTRUCTION, lexème, type};
    }

    DescriptionAtome crée_construction_structure(
        LexèmesType const &type, kuri::tableau_statique<InfoInitMembreStructure> membres)
    {
        return {Atome::Genre::CONSTANTE_STRUCTURE, nullptr, type};
    }

    DescriptionAtome crée_constante_entière(LexèmesType const &type, Lexème const *lexème)
    {
        return {Atome::Genre::CONSTANTE_ENTIÈRE, lexème};
    }

    DescriptionAtome crée_constante_réelle(LexèmesType const &type, Lexème const *lexème)
    {
        return {Atome::Genre::CONSTANTE_RÉELLE, lexème};
    }

    DescriptionAtome crée_constante_nulle(LexèmesType const &type)
    {
        return {Atome::Genre::CONSTANTE_NULLE, nullptr};
    }

    DescriptionAtome crée_référence_globale(LexèmesType const &type, Lexème const *lexème)
    {
        return {Atome::Genre::GLOBALE, lexème};
    }

    DescriptionAtome crée_indexage_constant(LexèmesType const &type,
                                            Lexème const *lexème_nombre,
                                            DescriptionAtome const &globale)
    {
        /* À FAIRE. */
        return {Atome::Genre::ACCÈS_INDEX_CONSTANT, lexème_nombre, type};
    }

    DescriptionAtome crée_taille_de(Lexème const *lexème, LexèmesType const &type)
    {
        return {Atome::Genre::CONSTANTE_TAILLE_DE, lexème, type};
    }

    DescriptionAtome crée_index_de(Lexème const *lexème, LexèmesType const &type)
    {
        return {Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE, lexème, type};
    }

    DescriptionAtome crée_transtypage_constant(Lexème const *lexème,
                                               DescriptionAtome const &atome_transtypé,
                                               LexèmesType const &type_destination)
    {
        // À FAIRE : retourne l'instruction
        return atome_transtypé;
    }

    DescriptionAtome parse_init_tableau(LexèmesType const &type, Lexème const *lexème)
    {
        // À FAIRE : retourne l'instruction
        return analyse_atome_typé();
    }

    DescriptionAtome parse_référence_fonction(LexèmesType const &type, Lexème const *lexème)
    {
        return {Atome::Genre::FONCTION, lexème, type};
    }

    DescriptionAtome crée_construction_tableau(LexèmesType const &type,
                                               kuri::tableau_statique<DescriptionAtome> valeurs)
    {
        return {Atome::Genre::CONSTANTE_TABLEAU_FIXE, nullptr, type};
    }

    void crée_déclaration_type_structure(DonnéesTypeComposé const &données)
    {
        m_données_préparsage.structures.ajoute(données);

#ifdef IMPRIME_RI
        std::cerr << "structure " << chaine_type(données.données_types_nominal) << " = ";

        auto virgule = "{ ";

        POUR (données.membres) {
            std::cerr << virgule << it.nom->ident->nom << " " << chaine_type(it.type);
            virgule = ", ";
        }

        if (données.membres.est_vide()) {
            std::cerr << "{";
        }

        std::cerr << " }\n";
#endif
    }

    void crée_déclaration_type_union(DonnéesTypeComposé const &données, bool est_nonsûre)
    {
        m_données_préparsage.unions.ajoute(données);

#ifdef IMPRIME_RI
        std::cerr << "union " << chaine_type(données.données_types_nominal) << " = ";

        auto virgule = "{ ";

        POUR (données.membres) {
            std::cerr << virgule << it.nom->ident->nom << " " << chaine_type(it.type);
            virgule = ", ";
        }

        if (données.membres.est_vide()) {
            std::cerr << "{";
        }

        std::cerr << " }\n";
#endif
    }

    void crée_déclaration_type_énum(DonnéesTypeComposé const &données)
    {
        m_données_préparsage.énums.ajoute(données);
    }

    void crée_déclaration_type_opaque(DonnéesTypeComposé const &données)
    {
        m_données_préparsage.opaques.ajoute(données);
    }

    void débute_fonction(Fonction const &fonction)
    {
        m_données_préparsage.fonctions.ajoute(fonction);

#ifdef IMPRIME_RI
        numéro_instruction_courante = 0;

        std::cerr << "fonction " << fonction.nom;
        auto virgule = "(";

        POUR (fonction.paramètres) {
            std::cerr << virgule << it.nom << " " << chaine_type(it.type);
            virgule = ", ";
            numéro_instruction_courante++;
        }

        if (fonction.paramètres.est_vide()) {
            std::cerr << virgule;
        }

        std::cerr << ") -> " << chaine_type(fonction.type_retour) << "\n";
        /* À FAIRE : n'incrémente que si le type n'est pas « rien ». */
        numéro_instruction_courante++;
#endif
    }

    void termine_fonction()
    {
#ifdef IMPRIME_RI
        std::cerr << "\n";
#endif
    }

    void crée_allocation(LexèmesType const &type, IdentifiantCode *ident)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true, ident);
        std::cerr << "alloue " << chaine_type(type) << '\n';
#endif
    }

    void crée_appel(DescriptionAtome const &atome_fonction,
                    kuri::tableau_statique<DescriptionAtome> arguments)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true);
        std::cerr << "appel ";

        auto virgule = "(";

        std::cerr << atome_fonction;
        POUR (arguments) {
            std::cerr << virgule << it;
            virgule = ", ";
        }

        if (arguments.taille() == 0) {
            std::cerr << virgule;
        }
        std::cerr << ")" << '\n';
#endif
    }

    void crée_branche(uint64_t cible)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(false);
        std::cerr << "branche %" << cible << '\n';
#endif
    }

    void crée_charge(DescriptionAtome const &valeur)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true);
        std::cerr << "charge " << valeur << '\n';
#endif
    }

    void crée_index(DescriptionAtome const &indexé, DescriptionAtome const &valeur)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true);
        std::cerr << "index " << indexé << ", " << valeur << '\n';
#endif
    }

    void crée_label(uint64_t index)
    {
#ifdef IMPRIME_RI
        numéro_instruction_courante++;
        std::cerr << "label " << index << '\n';
#endif
    }

    void crée_membre(DescriptionAtome const &valeur, uint64_t index)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true);
        std::cerr << "membre " << valeur << ", " << index << '\n';
#endif
    }

    void crée_retourne(DescriptionAtome const &valeur)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(false);
        std::cerr << "retourne";
        if (valeur.lexème != nullptr) {
            std::cerr << " " << valeur;
        }
        std::cerr << '\n';
#endif
    }

    void crée_si(DescriptionAtome const &prédicat, uint64_t si_vrai, uint64_t si_faux)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(false);
        std::cerr << "si " << prédicat << " alors %" << si_vrai << " sinon %" << si_faux << '\n';
#endif
    }

    void crée_stocke(DescriptionAtome const &cible, DescriptionAtome const &valeur)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(false);
        std::cerr << "stocke " << cible << ", " << valeur << '\n';
#endif
    }

    void crée_transtype(IdentifiantCode *ident,
                        DescriptionAtome const &valeur,
                        LexèmesType const &type)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true);
        std::cerr << ident->nom << " " << valeur << " vers " << chaine_type(type) << '\n';
#endif
    }

    void crée_opérateur_unaire(OpérateurUnaire::Genre genre, DescriptionAtome const &opérande)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true);
        std::cerr << chaine_pour_genre_op(genre) << ' ' << opérande << '\n';
#endif
    }

    void crée_opérateur_binaire(OpérateurBinaire::Genre genre,
                                DescriptionAtome const &gauche,
                                DescriptionAtome const &droite)
    {
#ifdef IMPRIME_RI
        imprime_numéro_instruction(true);
        std::cerr << chaine_pour_genre_op(genre) << ' ' << gauche << ", " << droite << '\n';
#endif
    }

  private:
#ifdef IMPRIME_RI
    int32_t numéro_instruction_courante = 0;

    void imprime_numéro_instruction(bool numérote, IdentifiantCode const *ident = nullptr)
    {
        std::cerr << "  ";
        if (numérote) {
            if (ident) {
                numéro_instruction_courante++;
                std::cerr << "%" << ident->nom << " = ";
            }
            else {
                std::cerr << "%" << numéro_instruction_courante++ << " = ";
            }
        }
        else {
            numéro_instruction_courante++;
        }
    }
#endif
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name SyntaxeuseRI.
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

    using TypeTrie = kuri::trie<IdentifiantCode *, NoeudDeclarationType *>;
    using TypeNoeudTrie = TypeTrie::Noeud;

    TypeTrie m_trie_types{};
    TypeTrieBloc m_trie_blocs{};

    AllocatriceNoeud m_allocatrice{};
    AssembleuseArbre m_assembleuse{m_allocatrice};
    Typeuse &m_typeuse;

    ConstructriceRI m_constructrice;

    kuri::table_hachage<IdentifiantCode *, AtomeGlobale *> m_table_globales{"table_globales"};
    kuri::table_hachage<IdentifiantCode *, AtomeFonction *> m_table_fonctions{"table_fonctions"};

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
                 PrésyntaxeuseRI &pré_syntaxeuse)
        : BaseSyntaxeuseRI(fichier), m_typeuse(typeuse), m_constructrice(typeuse, registre)
    {
        auto données_préparsage = pré_syntaxeuse.donne_données_préparsage();
        POUR (données_préparsage->structures) {
            crée_type_préparsé(it.données_types_nominal, GenreNoeud::DECLARATION_STRUCTURE);
        }

        POUR (données_préparsage->unions) {
            crée_type_préparsé(it.données_types_nominal, GenreNoeud::DECLARATION_UNION);
        }

        POUR (données_préparsage->énums) {
            crée_type_préparsé(it.données_types_nominal, GenreNoeud::DECLARATION_ENUM);
        }

        POUR (données_préparsage->opaques) {
            crée_type_préparsé(it.données_types_nominal, GenreNoeud::DECLARATION_OPAQUE);
        }

        POUR (données_préparsage->globales) {
            auto globale = m_constructrice.crée_globale(
                *it.lexème_nom->ident, nullptr, nullptr, false, false);
            m_table_globales.insère(it.lexème_nom->ident, globale);
        }

        POUR (données_préparsage->fonctions) {
            auto fonction = m_constructrice.crée_fonction(it.nom->ident->nom);
            m_table_fonctions.insère(it.nom->ident, fonction);
        }
    }

    EMPECHE_COPIE(SyntaxeuseRI);

    /* Types. */
    Type *crée_type_pointeur(Lexème const *lexème, Type *type_pointé)
    {
        return m_typeuse.type_pointeur_pour(type_pointé, false, false);
    }

    Type *crée_type_référence(Lexème const *lexème, Type *type_pointé)
    {
        return m_typeuse.type_reference_pour(type_pointé);
    }

    Type *crée_type_tableau_dynamique(Lexème const *crochet_ouvrant,
                                      Lexème const *crochet_fermant,
                                      Type *type_élément)
    {
        return m_typeuse.type_tableau_dynamique(type_élément);
    }

    Type *crée_type_tableau_fixe(Lexème const *crochet_ouvrant,
                                 Lexème const *crochet_fermant,
                                 Type *type_élément,
                                 int32_t taille)
    {
        return m_typeuse.type_tableau_fixe(type_élément, taille);
    }

    Type *crée_type_tranche(Lexème const *crochet_ouvrant,
                            Lexème const *crochet_fermant,
                            Type *type_élément)
    {
        return m_typeuse.crée_type_tranche(type_élément);
    }

    Type *crée_type_variadique(Lexème const *lexème, Type *type_élément)
    {
        return m_typeuse.type_variadique(type_élément);
    }

    Type *crée_type_type_de_données(Lexème const *lexème, Type *type)
    {
        return m_typeuse.type_type_de_donnees(type);
    }

    Type *crée_type_nomimal(kuri::tableau_statique<Lexème *> lexèmes)
    {
        auto déclaration = donne_déclaration_pour_type_nominale(lexèmes);
        if (!déclaration) {
            rapporte_erreur("Type inconnu");
            return nullptr;
        }

        return déclaration;
    }

    Type *crée_type_basique(Lexème const *lexème)
    {
        return m_typeuse.type_pour_lexeme(lexème->genre);
    }

    Type *crée_type_fonction(Lexème const *lexème,
                             kuri::tableau_statique<Type *> types_entrée,
                             kuri::tableau_statique<Type *> types_sortie)
    {
        kuri::tablet<Type *, 6> entrées;
        POUR (types_entrée) {
            entrées.ajoute(it);
        }

        auto sortie = types_sortie[0];
        if (types_sortie.taille() > 1) {
            sortie = crée_type_tuple(lexème, types_sortie);
        }

        return m_typeuse.type_fonction(entrées, sortie, false);
    }

    Type *crée_type_tuple(Lexème const *lexème, kuri::tableau_statique<Type *> types)
    {
        kuri::tablet<MembreTypeComposé, 6> membres;
        membres.réserve(types.taille());

        POUR (types) {
            membres.ajoute({nullptr, it});
        }

        return m_typeuse.crée_tuple(membres);
    }

    /* Atomes. */

    Atome *crée_atome_nul() const
    {
        return nullptr;
    }

    Atome *parse_données_constantes(Type *type)
    {
        kuri::tableau<char> données;

        while (!fini()) {
            if (apparie(GenreLexème::CROCHET_FERMANT)) {
                break;
            }

            CONSOMME_NOMBRE_ENTIER(octet, "Attendu un nombre entier.", nullptr);

            auto valeur_entière = lexème_octet->valeur_entiere;
            if (valeur_entière > 255) {
                rapporte_erreur("Valeur trop grande pour l'octet des données constantes.");
                return nullptr;
            }

            données.ajoute(char(lexème_octet->valeur_entiere));

            if (!apparie(GenreLexème::VIRGULE)) {
                break;
            }

            consomme();
        }

        /* À FAIRE : validation. */
        return m_constructrice.crée_constante_tableau_données_constantes(type, std::move(données));
    }

    void crée_globale(Lexème const *lexème, Type *type, Atome *initialisateur)
    {
        auto globale = m_table_globales.valeur_ou(lexème->ident, nullptr);
        if (!globale) {
            rapporte_erreur("Globale inconnue");
            return;
        }

        if (initialisateur && !initialisateur->est_constante() &&
            !initialisateur->est_initialisation_tableau()) {
            rapporte_erreur("Initialisateur non constant pour la globale.");
            return;
        }

        globale->type = m_typeuse.type_pointeur_pour(type);
        globale->initialisateur = static_cast<AtomeConstante *>(initialisateur);
    }

    Atome *crée_construction_structure(Type *type,
                                       kuri::tableau_statique<InfoInitMembreStructure> membres)
    {
        kuri::tableau<AtomeConstante *> valeurs;
        valeurs.réserve(membres.taille());

        POUR (membres) {
            /* À FAIRE : validation. */
            valeurs.ajoute(static_cast<AtomeConstante *>(it.atome));
        }

        return m_constructrice.crée_constante_structure(type, std::move(valeurs));
    }

    Atome *crée_constante_entière(Type *type, Lexème const *lexème)
    {
        return m_constructrice.crée_constante_nombre_entier(type, lexème->valeur_entiere);
    }

    Atome *crée_constante_réelle(Type *type, Lexème const *lexème)
    {
        return m_constructrice.crée_constante_nombre_réel(type, lexème->valeur_reelle);
    }

    Atome *crée_constante_nulle(Type *type)
    {
        return m_constructrice.crée_constante_nulle(type);
    }

    Atome *crée_référence_globale(Type *type, Lexème const *lexème)
    {
        auto globale = m_table_globales.valeur_ou(lexème->ident, nullptr);
        if (!globale) {
            rapporte_erreur("Globale inconnue");
            return nullptr;
        }
        /* À FAIRE : source du type. */
        globale->type = type;
        return globale;
    }

    Atome *crée_indexage_constant(Type *type, Lexème const *lexème_nombre, Atome *globale)
    {
        if (!globale->est_constante() && !globale->est_globale()) {
            rapporte_erreur("Valeur non constante pour l'indexage constant.");
            return nullptr;
        }
        /* À FAIRE : passe le type. */
        globale->type = type;
        return m_constructrice.crée_accès_index_constant(static_cast<AtomeConstante *>(globale),
                                                         int64_t(lexème_nombre->valeur_entiere));
    }

    Atome *crée_taille_de(Lexème const *lexème, Type *type)
    {
        return m_constructrice.crée_constante_taille_de(type);
    }

    Atome *crée_index_de(Lexème const *lexème, Type *type)
    {
        return m_constructrice.crée_index_table_type(type);
    }

    Atome *crée_transtypage_constant(Lexème const *lexème,
                                     Atome *atome_transtypé,
                                     Type *type_destination)
    {
        if (!atome_transtypé->est_constante() && !atome_transtypé->est_globale() &&
            !atome_transtypé->est_fonction() && !atome_transtypé->est_accès_index_constant()) {
            rapporte_erreur("Valeur non constante pour le transtypage constant.");
            return nullptr;
        }

        return m_constructrice.crée_transtype_constant(
            type_destination, static_cast<AtomeConstante *>(atome_transtypé));
    }

    Atome *parse_init_tableau(Type *type, Lexème const *lexème)
    {
        auto atome = analyse_atome_typé();
        if (!atome->est_constante() && !atome->est_constante_structure() &&
            !atome->est_constante_tableau() && !atome->est_initialisation_tableau()) {
            dbg() << atome->genre_atome;
            rapporte_erreur("Atome non-constant pour l'initialisation de tableau.");
            return nullptr;
        }
        return m_constructrice.crée_initialisation_tableau(type,
                                                           static_cast<AtomeConstante *>(atome));
    }

    Atome *parse_référence_fonction(Type *type, Lexème const *lexème)
    {
        auto fonction = m_table_fonctions.valeur_ou(lexème->ident, nullptr);
        if (!fonction) {
            rapporte_erreur("Fonction inconnue.");
            return nullptr;
        }
        /* À FAIRE : compare le type avec celui de la déclaration. */
        fonction->type = type;
        return fonction;
    }

    Atome *crée_construction_tableau(Type *type, kuri::tableau_statique<Atome *> atomes)
    {
        kuri::tableau<AtomeConstante *> valeurs;
        valeurs.réserve(atomes.taille());

        POUR (atomes) {
            /* À FAIRE : validation. */
            valeurs.ajoute(static_cast<AtomeConstante *>(it));
        }
        return m_constructrice.crée_constante_tableau_fixe(type, std::move(valeurs));
    }

    void crée_déclaration_type_structure(DonnéesTypeComposé const &données)
    {
        /* À FAIRE : il y a des redéfinitions. */
        auto déclaration = donne_déclaration_pour_type_nominale(données.données_types_nominal);

        if (!déclaration) {
            rapporte_erreur("Structure inconnue");
            return;
        }

        auto structure = déclaration->comme_type_structure();

        POUR (données.membres) {
            auto membre_type = MembreTypeCompose{};
            membre_type.nom = it.nom->ident;
            membre_type.type = it.type;
            structure->membres.ajoute(membre_type);
        }

        structure->nombre_de_membres_réels = int32_t(données.membres.taille());

        if (structure->ident == ID::InfoType) {
            m_typeuse.type_info_type_ = structure;
            TypeBase::EINI->comme_type_compose()->membres[1].type = m_typeuse.type_pointeur_pour(
                structure);
        }
    }

    void crée_déclaration_type_union(DonnéesTypeComposé const &données, bool est_nonsûre)
    {
        /* À FAIRE : il y a des redéfinitions. */
        auto déclaration = donne_déclaration_pour_type_nominale(données.données_types_nominal);

        if (!déclaration) {
            rapporte_erreur("Union inconnue");
            return;
        }

        auto structure = déclaration->comme_type_union();

        POUR (données.membres) {
            auto membre_type = MembreTypeCompose{};
            membre_type.nom = it.nom->ident;
            membre_type.type = it.type;
            structure->membres.ajoute(membre_type);
        }

        structure->nombre_de_membres_réels = int32_t(données.membres.taille());
        structure->est_nonsure = est_nonsûre;

        /* À FAIRE : taille des types. */
        // calcule_taille_type_compose(structure, false, 0);

        if (!est_nonsûre) {
            crée_type_structure(m_typeuse, structure, 0);
        }
    }

    void crée_déclaration_type_énum(DonnéesTypeComposé const &données)
    {
        /* À FAIRE : il y a des redéfinitions. */
        auto déclaration = donne_déclaration_pour_type_nominale(données.données_types_nominal);

        if (!déclaration) {
            rapporte_erreur("Type Énumération inconnu.");
            return;
        }

        auto énum = déclaration->comme_type_enum();
        énum->type_sous_jacent = données.type_sous_jacent;
    }

    void crée_déclaration_type_opaque(DonnéesTypeComposé const &données)
    {
        /* À FAIRE : il y a des redéfinitions. */
        auto déclaration = donne_déclaration_pour_type_nominale(données.données_types_nominal);

        if (!déclaration) {
            rapporte_erreur("Type opaque inconnu.");
            return;
        }

        auto opaque = déclaration->comme_type_opaque();
        opaque->type_opacifie = données.type_sous_jacent;
    }

    void débute_fonction(Fonction const &données_fonction)
    {
        auto fonction = m_table_fonctions.valeur_ou(données_fonction.nom->ident, nullptr);
        if (!fonction) {
            rapporte_erreur("Fonction inconnue");
            return;
        }

        POUR (données_fonction.paramètres) {
            auto alloc = m_constructrice.crée_allocation(nullptr, it.type, it.nom->ident, true);
            fonction->params_entrée.ajoute(alloc);
        }

        fonction->param_sortie = m_constructrice.crée_allocation(
            nullptr, données_fonction.type_retour, données_fonction.nom_retour->ident, true);

        /* À FAIRE : type fonction. */

        m_décalage_instructions = fonction->numérote_instructions();
        m_fonction_courante = fonction;
        m_constructrice.définis_fonction_courante(fonction);

        dbg() << fonction->nom;
    }

    void termine_fonction()
    {
        m_constructrice.définis_fonction_courante(nullptr);
        m_fonction_courante = nullptr;
        m_labels_réservés.efface();
    }

    /* Instruction. */

    Atome *crée_référence_instruction(Type *type, Lexème const *lexème)
    {
        if (lexème->genre == GenreLexème::NOMBRE_ENTIER) {
            auto index_instruction = int32_t(lexème->valeur_entiere) - m_décalage_instructions;
            if (index_instruction >= m_fonction_courante->instructions.taille()) {
                rapporte_erreur(
                    "Instruction référencée hors des limites des instructions connues.");
                return nullptr;
            }

            return m_fonction_courante->instructions[index_instruction];
        }

        assert(lexème->genre == GenreLexème::CHAINE_CARACTERE);

        POUR (m_fonction_courante->params_entrée) {
            if (it->ident == lexème->ident) {
                return it;
            }
        }

        if (m_fonction_courante->param_sortie->ident == lexème->ident) {
            return m_fonction_courante->param_sortie;
        }

        POUR (m_fonction_courante->instructions) {
            if (!it->est_alloc()) {
                continue;
            }

            if (it->comme_alloc()->ident == lexème->ident) {
                return it;
            }
        }

        rapporte_erreur("Instruction inconnue.");
        return nullptr;
    }

    void crée_allocation(Type *type, IdentifiantCode *ident)
    {
        m_constructrice.crée_allocation(nullptr, type, ident);
    }

    void crée_appel(Atome *atome, kuri::tableau_statique<Atome *> arguments)
    {
        kuri::tableau<Atome *, int> tableau_arguments;
        POUR (arguments) {
            tableau_arguments.ajoute(it);
        }
        m_constructrice.crée_appel(nullptr, atome, std::move(tableau_arguments));
    }

    void crée_branche(uint64_t cible)
    {
        auto label = donne_label_pour_cible(int32_t(cible));
        if (!label) {
            return;
        }
        m_constructrice.crée_branche(nullptr, label);
    }

    void crée_charge(Atome *valeur)
    {
        m_constructrice.crée_charge_mem(nullptr, valeur);
    }

    void crée_index(Atome *indexé, Atome *valeur)
    {
        m_constructrice.crée_accès_index(nullptr, indexé, valeur);
    }

    void crée_label(uint64_t index)
    {
        /* À FAIRE : précrée les labels. */
        POUR (m_labels_réservés) {
            if (it.cible == m_fonction_courante->instructions.taille()) {
                /* À FAIRE : vérifie que les labels des branches sont dans les instructions. */
                m_constructrice.insère_label(it.label);
                return;
            }
        }

        auto label = m_constructrice.crée_label(nullptr);
        label->id = int32_t(index);
    }

    void crée_membre(Atome *valeur, uint64_t index)
    {
        dbg() << chaine_type(valeur->type) << " " << index;
        m_constructrice.crée_référence_membre(nullptr, valeur, int32_t(index));
    }

    void crée_retourne(Atome *valeur)
    {
        m_constructrice.crée_retour(nullptr, valeur);
    }

    void crée_si(Atome *prédicat, uint64_t si_vrai, uint64_t si_faux)
    {
        auto label_si_vrai = donne_label_pour_cible(int32_t(si_vrai));
        if (!label_si_vrai) {
            return;
        }
        auto label_si_faux = donne_label_pour_cible(int32_t(si_faux));
        if (!label_si_faux) {
            return;
        }
        m_constructrice.crée_branche(nullptr, label_si_vrai, label_si_faux);
    }

    void crée_stocke(Atome *cible, Atome *valeur)
    {
        m_constructrice.crée_stocke_mem(nullptr, cible, valeur);
    }

    void crée_transtype(IdentifiantCode *ident, Atome *valeur, Type *type)
    {
        auto type_transtypage = type_transtypage_depuis_ident(ident);
        m_constructrice.crée_transtype(nullptr, type, valeur, type_transtypage);
    }

    void crée_opérateur_unaire(OpérateurUnaire::Genre genre, Atome *opérande)
    {
        m_constructrice.crée_op_unaire(nullptr, opérande->type, genre, opérande);
    }

    void crée_opérateur_binaire(OpérateurBinaire::Genre genre, Atome *gauche, Atome *droite)
    {
        auto type_résultat = gauche->type;
        if (est_opérateur_comparaison(genre)) {
            type_résultat = TypeBase::BOOL;
        }

        m_constructrice.crée_op_binaire(nullptr, type_résultat, genre, gauche, droite);
    }

  private:
    NoeudBloc *donne_bloc_parent_pour_type(kuri::tableau_statique<Lexème *> lexèmes)
    {
        /* À FAIRE : déduplique les blocs. */
        NoeudBloc *bloc_courant = nullptr;
        POUR (lexèmes) {
            bloc_courant = m_assembleuse.crée_bloc_seul(it, bloc_courant);
            bloc_courant->ident = it->ident;
        }

        if (bloc_courant) {
            return bloc_courant->bloc_parent;
        }

        return nullptr;
    }

    NoeudDeclarationType *donne_déclaration_pour_type_nominale(
        kuri::tableau_statique<Lexème *> lexèmes)
    {
        auto ident_modules = kuri::tablet<IdentifiantCode *, 6>();

        for (auto i = 0; i < lexèmes.taille(); i++) {
            ident_modules.ajoute(lexèmes[i]->ident);
        }

        auto résultat = m_trie_types.trouve_valeur_ou_noeud_insertion(ident_modules);
        if (!std::holds_alternative<NoeudDeclarationType *>(résultat)) {
            return nullptr;
        }

        return std::get<NoeudDeclarationType *>(résultat);
    }

    NoeudDeclarationType *crée_type_préparsé(kuri::tableau_statique<Lexème *> données_type_nominal,
                                             GenreNoeud genre)
    {
        auto nom_type = *(données_type_nominal.end() - 1);
        auto ident_modules = kuri::tablet<IdentifiantCode *, 6>();
        auto blocs_modules = kuri::tablet<Lexème *, 6>();

        for (auto i = 0; i < données_type_nominal.taille() - 1; i++) {
            ident_modules.ajoute(données_type_nominal[i]->ident);
            blocs_modules.ajoute(données_type_nominal[i]);
        }

        ident_modules.ajoute(nom_type->ident);

        auto résultat = m_trie_types.trouve_valeur_ou_noeud_insertion(ident_modules);
        if (std::holds_alternative<NoeudDeclarationType *>(résultat)) {
            return nullptr;
        }

        auto type = crée_type_nominal_pour_genre(nom_type, genre);
        type->type = type;
        type->bloc_parent = donne_bloc_parent_pour_type(blocs_modules);

        auto point_insertion = std::get<TypeNoeudTrie *>(résultat);
        point_insertion->données = type;
        return type;
    }

    NoeudDeclarationType *crée_type_nominal_pour_genre(Lexème const *lexème, GenreNoeud genre)
    {
        switch (genre) {
            case GenreNoeud::DECLARATION_STRUCTURE:
            {
                return m_assembleuse.crée_type_structure(lexème);
            }
            case GenreNoeud::DECLARATION_UNION:
            {
                return m_assembleuse.crée_type_union(lexème);
            }
            case GenreNoeud::DECLARATION_ENUM:
            {
                return m_assembleuse.crée_type_enum(lexème);
            }
            case GenreNoeud::DECLARATION_OPAQUE:
            {
                return m_assembleuse.crée_type_opaque(lexème);
            }
            default:
            {
                assert_rappel(false, [&]() { dbg() << "Genre de type non-géré " << genre; });
                return nullptr;
            }
        }
    }

    InstructionLabel *donne_label_pour_cible(int32_t cible)
    {
        auto cible_décalée = cible - m_décalage_instructions;
        if (cible_décalée < m_fonction_courante->instructions.taille()) {
            auto inst = m_fonction_courante->instructions[cible_décalée];
            if (!inst->est_label()) {
                rapporte_erreur("La cible de la branche n'est pas un label");
                return nullptr;
            }
            return inst->comme_label();
        }

        POUR (m_labels_réservés) {
            if (it.cible == cible) {
                return it.label;
            }
        }

        auto résultat = m_constructrice.réserve_label(nullptr);
        auto label_réservé = LabelRéservé{résultat, cible};
        m_labels_réservés.ajoute(label_réservé);
        return résultat;
    }
};

/** \} */

int main(int argc, char **argv)
{
    if (argc < 2) {
        dbg() << "Utilisation " << argv[0] << " " << "FICHIER_RI";
        return 1;
    }

    auto chemin_fichier_ri = kuri::chemin_systeme::chemin_temporaire(argv[1]);
    if (!kuri::chemin_systeme::existe(chemin_fichier_ri)) {
        std::cerr << "Fichier '" << argv[1] << "' inconnu.";
        return 1;
    }

    auto texte = charge_contenu_fichier(
        {chemin_fichier_ri.pointeur(), chemin_fichier_ri.taille()});

    Fichier fichier;
    fichier.tampon_ = lng::tampon_source(texte.c_str());
    fichier.chemin_ = "";

    auto gerante_chaine = dls::outils::Synchrone<GeranteChaine>();
    auto table_identifiants = dls::outils::Synchrone<TableIdentifiant>();
    auto contexte_lexage = ContexteLexage{gerante_chaine, table_identifiants, imprime_erreur};

    Lexeuse lexeuse(contexte_lexage, &fichier);
    lexeuse.performe_lexage();

    if (lexeuse.possède_erreur()) {
        return 1;
    }

    auto registre_opérateurs = dls::outils::Synchrone<RegistreDesOpérateurs>();
    Typeuse typeuse(registre_opérateurs);

    PrésyntaxeuseRI pré_syntaxeuse(&fichier);
    pré_syntaxeuse.analyse();

    if (pré_syntaxeuse.possède_erreur()) {
        return 1;
    }

    auto registre_symbolique = RegistreSymboliqueRI(typeuse);
    SyntaxeuseRI syntaxeuse(&fichier, typeuse, registre_symbolique, pré_syntaxeuse);
    syntaxeuse.analyse();

    return 0;
}
