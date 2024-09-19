/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include "syntaxage.hh"

#include <iostream>

#include "impression.hh"

#undef IMPRIME_RI

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

/* ------------------------------------------------------------------------- */
/** \name PrésyntaxeuseRI.
 * \{ */

const PrésyntaxeuseRI::DonnéesPréparsage *PrésyntaxeuseRI::donne_données_préparsage() const
{
    return &m_données_préparsage;
}

LexèmesType PrésyntaxeuseRI::crée_type_pointeur(const Lexème *lexème,
                                                const LexèmesType &type_pointé)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_référence(const Lexème *lexème,
                                                 const LexèmesType &type_pointé)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_tableau_dynamique(const Lexème *crochet_ouvrant,
                                                         const Lexème *crochet_fermant,
                                                         const LexèmesType &type_élément)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_tableau_fixe(const Lexème *crochet_ouvrant,
                                                    const Lexème *crochet_fermant,
                                                    const LexèmesType &type_élément,
                                                    int32_t taille)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_tranche(const Lexème *crochet_ouvrant,
                                               const Lexème *crochet_fermant,
                                               const LexèmesType &type_élément)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_variadique(const Lexème *lexème,
                                                  const LexèmesType &type_élément)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_type_de_données(const Lexème *lexème,
                                                       const LexèmesType &type)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_nomimal(kuri::tableau_statique<Lexème *> lexèmes)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_basique(const Lexème *lexème)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_fonction(const Lexème *lexème,
                                                kuri::tableau_statique<LexèmesType> types_entrée,
                                                kuri::tableau_statique<LexèmesType> types_sortie)
{
    return {};
}

LexèmesType PrésyntaxeuseRI::crée_type_tuple(const Lexème *lexème,
                                             kuri::tableau_statique<LexèmesType> types)
{
    return {};
}

DescriptionAtome PrésyntaxeuseRI::crée_atome_nul() const
{
    return {Atome::Genre::CONSTANTE_NULLE, nullptr, {}};
}

DescriptionAtome PrésyntaxeuseRI::parse_données_constantes(const LexèmesType &type)
{
    while (!fini()) {
        if (apparie(GenreLexème::CROCHET_FERMANT)) {
            break;
        }

        consomme();
    }

    return {Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES, nullptr, type};
}

void PrésyntaxeuseRI::crée_globale(const Lexème *lexème,
                                   const LexèmesType &type,
                                   const DescriptionAtome &initialisateur)
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

DescriptionAtome PrésyntaxeuseRI::crée_référence_instruction(const LexèmesType &type,
                                                             const Lexème *lexème)
{
    return {Atome::Genre::INSTRUCTION, lexème, type};
}

DescriptionAtome PrésyntaxeuseRI::crée_construction_structure(
    const LexèmesType &type, kuri::tableau_statique<InfoInitMembreStructure> membres)
{
    return {Atome::Genre::CONSTANTE_STRUCTURE, nullptr, type};
}

DescriptionAtome PrésyntaxeuseRI::crée_constante_entière(const LexèmesType &type,
                                                         const Lexème *lexème)
{
    return {Atome::Genre::CONSTANTE_ENTIÈRE, lexème};
}

DescriptionAtome PrésyntaxeuseRI::crée_constante_réelle(const LexèmesType &type,
                                                        const Lexème *lexème)
{
    return {Atome::Genre::CONSTANTE_RÉELLE, lexème};
}

DescriptionAtome PrésyntaxeuseRI::crée_constante_nulle(const LexèmesType &type)
{
    return {Atome::Genre::CONSTANTE_NULLE, nullptr};
}

DescriptionAtome PrésyntaxeuseRI::crée_référence_globale(const LexèmesType &type,
                                                         const Lexème *lexème)
{
    return {Atome::Genre::GLOBALE, lexème};
}

DescriptionAtome PrésyntaxeuseRI::crée_indexage_constant(const LexèmesType &type,
                                                         const Lexème *lexème_nombre,
                                                         const DescriptionAtome &globale)
{
    /* À FAIRE. */
    return {Atome::Genre::ACCÈS_INDEX_CONSTANT, lexème_nombre, type};
}

DescriptionAtome PrésyntaxeuseRI::crée_taille_de(const Lexème *lexème, const LexèmesType &type)
{
    return {Atome::Genre::CONSTANTE_TAILLE_DE, lexème, type};
}

DescriptionAtome PrésyntaxeuseRI::crée_index_de(const Lexème *lexème, const LexèmesType &type)
{
    return {Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE, lexème, type};
}

DescriptionAtome PrésyntaxeuseRI::crée_transtypage_constant(
    const Lexème *lexème,
    const DescriptionAtome &atome_transtypé,
    const LexèmesType &type_destination)
{
    // À FAIRE : retourne l'instruction
    return atome_transtypé;
}

DescriptionAtome PrésyntaxeuseRI::parse_init_tableau(const LexèmesType &type, const Lexème *lexème)
{
    // À FAIRE : retourne l'instruction
    return analyse_atome_typé();
}

DescriptionAtome PrésyntaxeuseRI::parse_référence_fonction(const LexèmesType &type,
                                                           const Lexème *lexème)
{
    return {Atome::Genre::FONCTION, lexème, type};
}

DescriptionAtome PrésyntaxeuseRI::crée_construction_tableau(
    const LexèmesType &type, kuri::tableau_statique<DescriptionAtome> valeurs)
{
    return {Atome::Genre::CONSTANTE_TABLEAU_FIXE, nullptr, type};
}

void PrésyntaxeuseRI::crée_déclaration_type_structure(const DonnéesTypeComposé &données)
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

void PrésyntaxeuseRI::crée_déclaration_type_union(const DonnéesTypeComposé &données,
                                                  bool est_nonsûre)
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

void PrésyntaxeuseRI::crée_déclaration_type_énum(const DonnéesTypeComposé &données)
{
    m_données_préparsage.énums.ajoute(données);
}

void PrésyntaxeuseRI::crée_déclaration_type_opaque(const DonnéesTypeComposé &données)
{
    m_données_préparsage.opaques.ajoute(données);
}

void PrésyntaxeuseRI::débute_fonction(const Fonction &fonction)
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

void PrésyntaxeuseRI::termine_fonction()
{
#ifdef IMPRIME_RI
    std::cerr << "\n";
#endif
}

void PrésyntaxeuseRI::crée_allocation(const LexèmesType &type, IdentifiantCode *ident)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(true, ident);
    std::cerr << "alloue " << chaine_type(type) << '\n';
#endif
}

void PrésyntaxeuseRI::crée_appel(const DescriptionAtome &atome_fonction,
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

void PrésyntaxeuseRI::crée_branche(uint64_t cible)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(false);
    std::cerr << "branche %" << cible << '\n';
#endif
}

void PrésyntaxeuseRI::crée_charge(const DescriptionAtome &valeur)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(true);
    std::cerr << "charge " << valeur << '\n';
#endif
}

void PrésyntaxeuseRI::crée_index(const DescriptionAtome &indexé, const DescriptionAtome &valeur)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(true);
    std::cerr << "index " << indexé << ", " << valeur << '\n';
#endif
}

void PrésyntaxeuseRI::crée_label(uint64_t index)
{
#ifdef IMPRIME_RI
    numéro_instruction_courante++;
    std::cerr << "label " << index << '\n';
#endif
}

void PrésyntaxeuseRI::crée_membre(const DescriptionAtome &valeur, uint64_t index)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(true);
    std::cerr << "membre " << valeur << ", " << index << '\n';
#endif
}

void PrésyntaxeuseRI::crée_retourne(const DescriptionAtome &valeur)
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

void PrésyntaxeuseRI::crée_si(const DescriptionAtome &prédicat, uint64_t si_vrai, uint64_t si_faux)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(false);
    std::cerr << "si " << prédicat << " alors %" << si_vrai << " sinon %" << si_faux << '\n';
#endif
}

void PrésyntaxeuseRI::crée_stocke(const DescriptionAtome &cible, const DescriptionAtome &valeur)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(false);
    std::cerr << "stocke " << cible << ", " << valeur << '\n';
#endif
}

void PrésyntaxeuseRI::crée_transtype(IdentifiantCode *ident,
                                     const DescriptionAtome &valeur,
                                     const LexèmesType &type)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(true);
    std::cerr << ident->nom << " " << valeur << " vers " << chaine_type(type) << '\n';
#endif
}

void PrésyntaxeuseRI::crée_opérateur_unaire(OpérateurUnaire::Genre genre,
                                            const DescriptionAtome &opérande)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(true);
    std::cerr << chaine_pour_genre_op(genre) << ' ' << opérande << '\n';
#endif
}

void PrésyntaxeuseRI::crée_opérateur_binaire(OpérateurBinaire::Genre genre,
                                             const DescriptionAtome &gauche,
                                             const DescriptionAtome &droite)
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(true);
    std::cerr << chaine_pour_genre_op(genre) << ' ' << gauche << ", " << droite << '\n';
#endif
}

void PrésyntaxeuseRI::crée_inatteignable()
{
#ifdef IMPRIME_RI
    imprime_numéro_instruction(false);
    std::cerr << "inatteignable\n";
#endif
}

#ifdef IMPRIME_RI
void PrésyntaxeuseRI::imprime_numéro_instruction(bool numérote, IdentifiantCode const *ident)
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

/** \} */

/* ------------------------------------------------------------------------- */
/** \name SyntaxeuseRI.
 * \{ */

SyntaxeuseRI::SyntaxeuseRI(Fichier *fichier,
                           Typeuse &typeuse,
                           RegistreSymboliqueRI &registre,
                           PrésyntaxeuseRI &pré_syntaxeuse)
    : BaseSyntaxeuseRI(fichier), m_typeuse(typeuse), m_constructrice(typeuse, registre)
{
    auto données_préparsage = pré_syntaxeuse.donne_données_préparsage();
    POUR (données_préparsage->structures) {
        crée_type_préparsé(it.données_types_nominal, GenreNoeud::DÉCLARATION_STRUCTURE);
    }

    POUR (données_préparsage->unions) {
        crée_type_préparsé(it.données_types_nominal, GenreNoeud::DÉCLARATION_UNION);
    }

    POUR (données_préparsage->énums) {
        crée_type_préparsé(it.données_types_nominal, GenreNoeud::DÉCLARATION_ÉNUM);
    }

    POUR (données_préparsage->opaques) {
        crée_type_préparsé(it.données_types_nominal, GenreNoeud::DÉCLARATION_OPAQUE);
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

kuri::tableau_statique<AtomeFonction *> SyntaxeuseRI::donne_fonctions() const
{
    return m_fonctions;
}

ConstructriceRI &SyntaxeuseRI::donne_constructrice()
{
    return m_constructrice;
}

Type *SyntaxeuseRI::crée_type_pointeur(const Lexème *lexème, Type *type_pointé)
{
    return m_typeuse.type_pointeur_pour(type_pointé, false, false);
}

Type *SyntaxeuseRI::crée_type_référence(const Lexème *lexème, Type *type_pointé)
{
    return m_typeuse.type_reference_pour(type_pointé);
}

Type *SyntaxeuseRI::crée_type_tableau_dynamique(const Lexème *crochet_ouvrant,
                                                const Lexème *crochet_fermant,
                                                Type *type_élément)
{
    return m_typeuse.type_tableau_dynamique(type_élément);
}

Type *SyntaxeuseRI::crée_type_tableau_fixe(const Lexème *crochet_ouvrant,
                                           const Lexème *crochet_fermant,
                                           Type *type_élément,
                                           int32_t taille)
{
    return m_typeuse.type_tableau_fixe(type_élément, taille);
}

Type *SyntaxeuseRI::crée_type_tranche(const Lexème *crochet_ouvrant,
                                      const Lexème *crochet_fermant,
                                      Type *type_élément)
{
    return m_typeuse.crée_type_tranche(type_élément);
}

Type *SyntaxeuseRI::crée_type_variadique(const Lexème *lexème, Type *type_élément)
{
    return m_typeuse.type_variadique(type_élément);
}

Type *SyntaxeuseRI::crée_type_type_de_données(const Lexème *lexème, Type *type)
{
    return m_typeuse.type_type_de_donnees(type);
}

Type *SyntaxeuseRI::crée_type_nomimal(kuri::tableau_statique<Lexème *> lexèmes)
{
    auto déclaration = donne_déclaration_pour_type_nominale(lexèmes);
    if (!déclaration) {
        rapporte_erreur("Type inconnu");
        return nullptr;
    }

    return déclaration;
}

Type *SyntaxeuseRI::crée_type_basique(const Lexème *lexème)
{
    return m_typeuse.type_pour_lexeme(lexème->genre);
}

Type *SyntaxeuseRI::crée_type_fonction(const Lexème *lexème,
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

Type *SyntaxeuseRI::crée_type_tuple(const Lexème *lexème, kuri::tableau_statique<Type *> types)
{
    kuri::tablet<MembreTypeComposé, 6> membres;
    membres.réserve(types.taille());

    POUR (types) {
        membres.ajoute({nullptr, it});
    }

    return m_typeuse.crée_tuple(membres);
}

Atome *SyntaxeuseRI::crée_atome_nul() const
{
    return nullptr;
}

Atome *SyntaxeuseRI::parse_données_constantes(Type *type)
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

void SyntaxeuseRI::crée_globale(const Lexème *lexème, Type *type, Atome *initialisateur)
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

Atome *SyntaxeuseRI::crée_construction_structure(
    Type *type, kuri::tableau_statique<InfoInitMembreStructure> membres)
{
    kuri::tableau<AtomeConstante *> valeurs;
    valeurs.réserve(membres.taille());

    POUR (membres) {
        /* À FAIRE : validation. */
        valeurs.ajoute(static_cast<AtomeConstante *>(it.atome));
    }

    return m_constructrice.crée_constante_structure(type, std::move(valeurs));
}

Atome *SyntaxeuseRI::crée_constante_entière(Type *type, const Lexème *lexème)
{
    return m_constructrice.crée_constante_nombre_entier(type, lexème->valeur_entiere);
}

Atome *SyntaxeuseRI::crée_constante_réelle(Type *type, const Lexème *lexème)
{
    return m_constructrice.crée_constante_nombre_réel(type, lexème->valeur_reelle);
}

Atome *SyntaxeuseRI::crée_constante_nulle(Type *type)
{
    return m_constructrice.crée_constante_nulle(type);
}

Atome *SyntaxeuseRI::crée_référence_globale(Type *type, const Lexème *lexème)
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

Atome *SyntaxeuseRI::crée_indexage_constant(Type *type,
                                            const Lexème *lexème_nombre,
                                            Atome *globale)
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

Atome *SyntaxeuseRI::crée_taille_de(const Lexème *lexème, Type *type)
{
    return m_constructrice.crée_constante_taille_de(type);
}

Atome *SyntaxeuseRI::crée_index_de(const Lexème *lexème, Type *type)
{
    return m_constructrice.crée_index_table_type(type);
}

Atome *SyntaxeuseRI::crée_transtypage_constant(const Lexème *lexème,
                                               Atome *atome_transtypé,
                                               Type *type_destination)
{
    if (!atome_transtypé->est_constante() && !atome_transtypé->est_globale() &&
        !atome_transtypé->est_fonction() && !atome_transtypé->est_accès_index_constant()) {
        rapporte_erreur("Valeur non constante pour le transtypage constant.");
        return nullptr;
    }

    return m_constructrice.crée_transtype_constant(type_destination,
                                                   static_cast<AtomeConstante *>(atome_transtypé));
}

Atome *SyntaxeuseRI::parse_init_tableau(Type *type, const Lexème *lexème)
{
    auto atome = analyse_atome_typé();
    if (!atome->est_constante() && !atome->est_constante_structure() &&
        !atome->est_constante_tableau() && !atome->est_initialisation_tableau()) {
        dbg() << atome->genre_atome;
        rapporte_erreur("Atome non-constant pour l'initialisation de tableau.");
        return nullptr;
    }
    return m_constructrice.crée_initialisation_tableau(type, static_cast<AtomeConstante *>(atome));
}

Atome *SyntaxeuseRI::parse_référence_fonction(Type *type, const Lexème *lexème)
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

Atome *SyntaxeuseRI::crée_construction_tableau(Type *type, kuri::tableau_statique<Atome *> atomes)
{
    kuri::tableau<AtomeConstante *> valeurs;
    valeurs.réserve(atomes.taille());

    POUR (atomes) {
        /* À FAIRE : validation. */
        valeurs.ajoute(static_cast<AtomeConstante *>(it));
    }
    return m_constructrice.crée_constante_tableau_fixe(type, std::move(valeurs));
}

void SyntaxeuseRI::crée_déclaration_type_structure(const DonnéesTypeComposé &données)
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
        TypeBase::EINI->comme_type_composé()->membres[1].type = m_typeuse.type_pointeur_pour(
            structure);
    }
}

void SyntaxeuseRI::crée_déclaration_type_union(const DonnéesTypeComposé &données, bool est_nonsûre)
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

void SyntaxeuseRI::crée_déclaration_type_énum(const DonnéesTypeComposé &données)
{
    /* À FAIRE : il y a des redéfinitions. */
    auto déclaration = donne_déclaration_pour_type_nominale(données.données_types_nominal);

    if (!déclaration) {
        rapporte_erreur("Type Énumération inconnu.");
        return;
    }

    auto énum = déclaration->comme_type_énum();
    énum->type_sous_jacent = données.type_sous_jacent;
}

void SyntaxeuseRI::crée_déclaration_type_opaque(const DonnéesTypeComposé &données)
{
    /* À FAIRE : il y a des redéfinitions. */
    auto déclaration = donne_déclaration_pour_type_nominale(données.données_types_nominal);

    if (!déclaration) {
        rapporte_erreur("Type opaque inconnu.");
        return;
    }

    auto opaque = déclaration->comme_type_opaque();
    opaque->type_opacifié = données.type_sous_jacent;
}

void SyntaxeuseRI::débute_fonction(const Fonction &données_fonction)
{
    auto fonction = m_table_fonctions.valeur_ou(données_fonction.nom->ident, nullptr);
    if (!fonction) {
        rapporte_erreur("Fonction inconnue");
        return;
    }

    auto decl = m_assembleuse.crée_entête_fonction(données_fonction.nom);
    fonction->decl = decl;

    m_fonctions.ajoute(fonction);

    auto types_entrées = kuri::tablet<Type *, 6>();

    POUR (données_fonction.paramètres) {
        auto alloc = m_constructrice.crée_allocation(nullptr, it.type, it.nom->ident, true);
        fonction->params_entrée.ajoute(alloc);
        types_entrées.ajoute(it.type);
    }

    fonction->param_sortie = m_constructrice.crée_allocation(
        nullptr, données_fonction.type_retour, données_fonction.nom_retour->ident, true);

    fonction->type = m_typeuse.type_fonction(types_entrées, données_fonction.type_retour, false);

    m_décalage_instructions = fonction->numérote_instructions();
    m_fonction_courante = fonction;
    m_constructrice.définis_fonction_courante(fonction);

    // dbg() << fonction->nom;
}

void SyntaxeuseRI::termine_fonction()
{
    m_constructrice.définis_fonction_courante(nullptr);
    m_fonction_courante = nullptr;
    m_labels_réservés.efface();
}

Atome *SyntaxeuseRI::crée_référence_instruction(Type *type, const Lexème *lexème)
{
    if (lexème->genre == GenreLexème::NOMBRE_ENTIER) {
        auto index_instruction = int32_t(lexème->valeur_entiere) - m_décalage_instructions;
        if (index_instruction >= m_fonction_courante->instructions.taille()) {
            rapporte_erreur("Instruction référencée hors des limites des instructions connues.");
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

void SyntaxeuseRI::crée_allocation(Type *type, IdentifiantCode *ident)
{
    m_constructrice.crée_allocation(nullptr, type, ident);
}

void SyntaxeuseRI::crée_appel(Atome *atome, kuri::tableau_statique<Atome *> arguments)
{
    kuri::tableau<Atome *, int> tableau_arguments;
    POUR (arguments) {
        tableau_arguments.ajoute(it);
    }
    m_constructrice.crée_appel(nullptr, atome, std::move(tableau_arguments));
}

void SyntaxeuseRI::crée_branche(uint64_t cible)
{
    auto label = donne_label_pour_cible(int32_t(cible));
    if (!label) {
        return;
    }
    m_constructrice.crée_branche(nullptr, label);
}

void SyntaxeuseRI::crée_charge(Atome *valeur)
{
    m_constructrice.crée_charge_mem(nullptr, valeur);
}

void SyntaxeuseRI::crée_index(Atome *indexé, Atome *valeur)
{
    m_constructrice.crée_accès_index(nullptr, indexé, valeur);
}

void SyntaxeuseRI::crée_label(uint64_t index)
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

void SyntaxeuseRI::crée_membre(Atome *valeur, uint64_t index)
{
    dbg() << chaine_type(valeur->type) << " " << index;
    m_constructrice.crée_référence_membre(nullptr, valeur, int32_t(index));
}

void SyntaxeuseRI::crée_retourne(Atome *valeur)
{
    m_constructrice.crée_retour(nullptr, valeur);
}

void SyntaxeuseRI::crée_si(Atome *prédicat, uint64_t si_vrai, uint64_t si_faux)
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

void SyntaxeuseRI::crée_stocke(Atome *cible, Atome *valeur)
{
    m_constructrice.crée_stocke_mem(nullptr, cible, valeur);
}

void SyntaxeuseRI::crée_transtype(IdentifiantCode *ident, Atome *valeur, Type *type)
{
    auto type_transtypage = type_transtypage_depuis_ident(ident);
    m_constructrice.crée_transtype(nullptr, type, valeur, type_transtypage);
}

void SyntaxeuseRI::crée_opérateur_unaire(OpérateurUnaire::Genre genre, Atome *opérande)
{
    m_constructrice.crée_op_unaire(nullptr, opérande->type, genre, opérande);
}

void SyntaxeuseRI::crée_opérateur_binaire(OpérateurBinaire::Genre genre,
                                          Atome *gauche,
                                          Atome *droite)
{
    auto type_résultat = gauche->type;
    if (est_opérateur_comparaison(genre)) {
        type_résultat = TypeBase::BOOL;
    }

    m_constructrice.crée_op_binaire(nullptr, type_résultat, genre, gauche, droite);
}

void SyntaxeuseRI::crée_inatteignable()
{
    m_constructrice.crée_inatteignable(nullptr);
}

NoeudBloc *SyntaxeuseRI::donne_bloc_parent_pour_type(kuri::tableau_statique<Lexème *> lexèmes)
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

NoeudDéclarationType *SyntaxeuseRI::donne_déclaration_pour_type_nominale(
    kuri::tableau_statique<Lexème *> lexèmes)
{
    auto ident_modules = kuri::tablet<IdentifiantCode *, 6>();

    for (auto i = 0; i < lexèmes.taille(); i++) {
        ident_modules.ajoute(lexèmes[i]->ident);
    }

    auto résultat = m_trie_types.trouve_valeur_ou_noeud_insertion(ident_modules);
    if (!std::holds_alternative<NoeudDéclarationType *>(résultat)) {
        return nullptr;
    }

    return std::get<NoeudDéclarationType *>(résultat);
}

NoeudDéclarationType *SyntaxeuseRI::crée_type_préparsé(
    kuri::tableau_statique<Lexème *> données_type_nominal, GenreNoeud genre)
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
    if (std::holds_alternative<NoeudDéclarationType *>(résultat)) {
        return nullptr;
    }

    auto type = crée_type_nominal_pour_genre(nom_type, genre);
    type->type = type;
    type->bloc_parent = donne_bloc_parent_pour_type(blocs_modules);

    auto point_insertion = std::get<TypeNoeudTrie *>(résultat);
    point_insertion->données = type;
    return type;
}

NoeudDéclarationType *SyntaxeuseRI::crée_type_nominal_pour_genre(const Lexème *lexème,
                                                                 GenreNoeud genre)
{
    switch (genre) {
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            return m_assembleuse.crée_type_structure(lexème);
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            return m_assembleuse.crée_type_union(lexème);
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        {
            return m_assembleuse.crée_type_énum(lexème);
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
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

InstructionLabel *SyntaxeuseRI::donne_label_pour_cible(int32_t cible)
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

/** \} */
