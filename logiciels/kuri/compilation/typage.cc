/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "typage.hh"

#include "biblinternes/outils/assert.hh"

#include <algorithm>
#include <iostream>

#include "parsage/identifiant.hh"
#include "parsage/outils_lexemes.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "compilatrice.hh"
#include "graphe_dependance.hh"
#include "operateurs.hh"

#include "statistiques/statistiques.hh"

#include "structures/pile.hh"

/* ************************************************************************** */

namespace TypeBase {
#define DECLARE_EXTERNE_TYPE(nom) Type *nom;
ENUMERE_TYPE_FONDAMENTAL(DECLARE_EXTERNE_TYPE)
#undef DECLARE_EXTERNE_TYPE
}  // namespace TypeBase

struct DonneesTypeCommun {
    Type **ptr_type;
    GenreLexeme dt[2];
};

static DonneesTypeCommun donnees_types_communs[] = {
    {&TypeBase::PTR_N8, {GenreLexeme::POINTEUR, GenreLexeme::N8}},
    {&TypeBase::PTR_N16, {GenreLexeme::POINTEUR, GenreLexeme::N16}},
    {&TypeBase::PTR_N32, {GenreLexeme::POINTEUR, GenreLexeme::N32}},
    {&TypeBase::PTR_N64, {GenreLexeme::POINTEUR, GenreLexeme::N64}},
    {&TypeBase::PTR_Z8, {GenreLexeme::POINTEUR, GenreLexeme::Z8}},
    {&TypeBase::PTR_Z16, {GenreLexeme::POINTEUR, GenreLexeme::Z16}},
    {&TypeBase::PTR_Z32, {GenreLexeme::POINTEUR, GenreLexeme::Z32}},
    {&TypeBase::PTR_Z64, {GenreLexeme::POINTEUR, GenreLexeme::Z64}},
    {&TypeBase::PTR_R16, {GenreLexeme::POINTEUR, GenreLexeme::R16}},
    {&TypeBase::PTR_R32, {GenreLexeme::POINTEUR, GenreLexeme::R32}},
    {&TypeBase::PTR_R64, {GenreLexeme::POINTEUR, GenreLexeme::R64}},
    {&TypeBase::PTR_EINI, {GenreLexeme::POINTEUR, GenreLexeme::EINI}},
    {&TypeBase::PTR_CHAINE, {GenreLexeme::POINTEUR, GenreLexeme::CHAINE}},
    {&TypeBase::PTR_RIEN, {GenreLexeme::POINTEUR, GenreLexeme::RIEN}},
    {&TypeBase::PTR_NUL, {GenreLexeme::POINTEUR, GenreLexeme::NUL}},
    {&TypeBase::PTR_BOOL, {GenreLexeme::POINTEUR, GenreLexeme::BOOL}},
    {&TypeBase::PTR_OCTET, {GenreLexeme::POINTEUR, GenreLexeme::OCTET}},

    {&TypeBase::REF_N8, {GenreLexeme::REFERENCE, GenreLexeme::N8}},
    {&TypeBase::REF_N16, {GenreLexeme::REFERENCE, GenreLexeme::N16}},
    {&TypeBase::REF_N32, {GenreLexeme::REFERENCE, GenreLexeme::N32}},
    {&TypeBase::REF_N64, {GenreLexeme::REFERENCE, GenreLexeme::N64}},
    {&TypeBase::REF_Z8, {GenreLexeme::REFERENCE, GenreLexeme::Z8}},
    {&TypeBase::REF_Z16, {GenreLexeme::REFERENCE, GenreLexeme::Z16}},
    {&TypeBase::REF_Z32, {GenreLexeme::REFERENCE, GenreLexeme::Z32}},
    {&TypeBase::REF_Z64, {GenreLexeme::REFERENCE, GenreLexeme::Z64}},
    {&TypeBase::REF_R16, {GenreLexeme::REFERENCE, GenreLexeme::R16}},
    {&TypeBase::REF_R32, {GenreLexeme::REFERENCE, GenreLexeme::R32}},
    {&TypeBase::REF_R64, {GenreLexeme::REFERENCE, GenreLexeme::R64}},
    {&TypeBase::REF_EINI, {GenreLexeme::REFERENCE, GenreLexeme::EINI}},
    {&TypeBase::REF_CHAINE, {GenreLexeme::REFERENCE, GenreLexeme::CHAINE}},
    {&TypeBase::REF_RIEN, {GenreLexeme::REFERENCE, GenreLexeme::RIEN}},
    {&TypeBase::REF_BOOL, {GenreLexeme::REFERENCE, GenreLexeme::BOOL}},

    {&TypeBase::TABL_N8, {GenreLexeme::TABLEAU, GenreLexeme::N8}},
    {&TypeBase::TABL_N16, {GenreLexeme::TABLEAU, GenreLexeme::N16}},
    {&TypeBase::TABL_N32, {GenreLexeme::TABLEAU, GenreLexeme::N32}},
    {&TypeBase::TABL_N64, {GenreLexeme::TABLEAU, GenreLexeme::N64}},
    {&TypeBase::TABL_Z8, {GenreLexeme::TABLEAU, GenreLexeme::Z8}},
    {&TypeBase::TABL_Z16, {GenreLexeme::TABLEAU, GenreLexeme::Z16}},
    {&TypeBase::TABL_Z32, {GenreLexeme::TABLEAU, GenreLexeme::Z32}},
    {&TypeBase::TABL_Z64, {GenreLexeme::TABLEAU, GenreLexeme::Z64}},
    {&TypeBase::TABL_R16, {GenreLexeme::TABLEAU, GenreLexeme::R16}},
    {&TypeBase::TABL_R32, {GenreLexeme::TABLEAU, GenreLexeme::R32}},
    {&TypeBase::TABL_R64, {GenreLexeme::TABLEAU, GenreLexeme::R64}},
    {&TypeBase::TABL_EINI, {GenreLexeme::TABLEAU, GenreLexeme::EINI}},
    {&TypeBase::TABL_CHAINE, {GenreLexeme::TABLEAU, GenreLexeme::CHAINE}},
    {&TypeBase::TABL_BOOL, {GenreLexeme::TABLEAU, GenreLexeme::BOOL}},
    {&TypeBase::TABL_OCTET, {GenreLexeme::TABLEAU, GenreLexeme::OCTET}},
};

/* ************************************************************************** */

const char *chaine_genre_type(GenreType genre)
{
#define ENUMERE_GENRE_TYPE_EX(nom, Genre, TypeRafine)                                             \
    case GenreType::Genre:                                                                        \
    {                                                                                             \
        return #Genre;                                                                            \
    }
    switch (genre) {
        ENUMERE_TYPE(ENUMERE_GENRE_TYPE_EX)
    }
#undef ENUMERE_GENRE_TYPE_EX

    return "erreur, ceci ne devrait pas s'afficher";
}

std::ostream &operator<<(std::ostream &os, GenreType genre)
{
    os << chaine_genre_type(genre);
    return os;
}

/* ------------------------------------------------------------------------- */
/** \name Création de types de bases.
 * \{ */

static TypeCompose *crée_type_eini()
{
    auto type = memoire::loge<TypeCompose>("TypeCompose");
    type->genre = GenreType::EINI;
    type->taille_octet = 16;
    type->alignement = 8;
    return type;
}

static TypeCompose *crée_type_chaine()
{
    auto type = memoire::loge<TypeCompose>("TypeCompose");
    type->genre = GenreType::CHAINE;
    type->taille_octet = 16;
    type->alignement = 8;
    return type;
}

static Type *crée_type_entier(unsigned taille_octet, bool est_naturel)
{
    auto type = memoire::loge<Type>("Type");
    type->genre = est_naturel ? GenreType::ENTIER_NATUREL : GenreType::ENTIER_RELATIF;
    type->taille_octet = taille_octet;
    type->alignement = taille_octet;
    type->drapeaux |= (TYPE_FUT_VALIDE);
    return type;
}

static Type *crée_type_entier_constant()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::ENTIER_CONSTANT;
    type->drapeaux |= (TYPE_FUT_VALIDE);
    return type;
}

static Type *crée_type_reel(unsigned taille_octet)
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::REEL;
    type->taille_octet = taille_octet;
    type->alignement = taille_octet;
    type->drapeaux |= (TYPE_FUT_VALIDE);
    return type;
}

static Type *crée_type_rien()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::RIEN;
    type->taille_octet = 0;
    type->drapeaux |= (TYPE_FUT_VALIDE);
    return type;
}

static Type *crée_type_bool()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::BOOL;
    type->taille_octet = 1;
    type->alignement = 1;
    type->drapeaux |= (TYPE_FUT_VALIDE);
    return type;
}

static Type *crée_type_octet()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::OCTET;
    type->taille_octet = 1;
    type->alignement = 1;
    type->drapeaux |= (TYPE_FUT_VALIDE);
    return type;
}

/** \} */

TypePointeur::TypePointeur(Type *type_pointe_) : TypePointeur()
{
    this->type_pointe = type_pointe_;
    this->taille_octet = 8;
    this->alignement = 8;
    this->drapeaux |= (TYPE_FUT_VALIDE);

    if (type_pointe_) {
        if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
        }

        type_pointe_->drapeaux |= POSSEDE_TYPE_POINTEUR;
    }
}

TypeReference::TypeReference(Type *type_pointe_) : TypeReference()
{
    assert(type_pointe_);

    this->type_pointe = type_pointe_;
    this->taille_octet = 8;
    this->alignement = 8;
    this->drapeaux |= (TYPE_FUT_VALIDE);

    if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }

    type_pointe_->drapeaux |= POSSEDE_TYPE_REFERENCE;
}

TypeFonction::TypeFonction(kuri::tablet<Type *, 6> const &entrees, Type *sortie) : TypeFonction()
{
    this->types_entrees.reserve(static_cast<int>(entrees.taille()));
    POUR (entrees) {
        this->types_entrees.ajoute(it);
    }

    this->type_sortie = sortie;
    this->taille_octet = 8;
    this->alignement = 8;
    marque_polymorphique(this);
    this->drapeaux |= (TYPE_FUT_VALIDE);
}

TypeTableauFixe::TypeTableauFixe(Type *type_pointe_,
                                 int taille_,
                                 kuri::tableau<MembreTypeComposé, int> &&membres_)
    : TypeTableauFixe()
{
    assert(type_pointe_);
    assert(taille_ > 0);

    this->membres = std::move(membres_);
    this->nombre_de_membres_réels = 0;
    this->type_pointe = type_pointe_;
    this->taille = taille_;
    this->alignement = type_pointe_->alignement;
    this->taille_octet = type_pointe_->taille_octet * static_cast<unsigned>(taille_);
    this->drapeaux |= (TYPE_FUT_VALIDE);

    if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }

    type_pointe_->drapeaux |= POSSEDE_TYPE_TABLEAU_FIXE;
}

TypeTableauDynamique::TypeTableauDynamique(Type *type_pointe_,
                                           kuri::tableau<MembreTypeComposé, int> &&membres_)
    : TypeTableauDynamique()
{
    assert(type_pointe_);

    this->membres = std::move(membres_);
    this->nombre_de_membres_réels = this->membres.taille();
    this->type_pointe = type_pointe_;
    this->taille_octet = 24;
    this->alignement = 8;
    this->drapeaux |= (TYPE_FUT_VALIDE);

    if (type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }

    type_pointe_->drapeaux |= POSSEDE_TYPE_TABLEAU_DYNAMIQUE;
}

TypeVariadique::TypeVariadique(Type *type_pointe_,
                               kuri::tableau<MembreTypeComposé, int> &&membres_)
    : TypeVariadique()
{
    this->type_pointe = type_pointe_;

    if (type_pointe_ && type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }

    this->membres = std::move(membres_);
    this->nombre_de_membres_réels = this->membres.taille();
    this->taille_octet = 24;
    this->alignement = 8;
    this->drapeaux |= (TYPE_FUT_VALIDE);
}

TypeTypeDeDonnees::TypeTypeDeDonnees(Type *type_connu_) : TypeTypeDeDonnees()
{
    this->genre = GenreType::TYPE_DE_DONNEES;
    // un type 'type' est un genre de pointeur déguisé, donc donnons lui les mêmes caractéristiques
    this->taille_octet = 8;
    this->alignement = 8;
    this->type_connu = type_connu_;
    this->drapeaux |= (TYPE_FUT_VALIDE);

    if (type_connu_) {
        type_connu_->drapeaux |= POSSEDE_TYPE_TYPE_DE_DONNEES;
    }
}

TypePolymorphique::TypePolymorphique(IdentifiantCode *ident_) : TypePolymorphique()
{
    this->ident = ident_;
    this->drapeaux |= (TYPE_FUT_VALIDE);
}

TypeOpaque::TypeOpaque(NoeudDeclarationTypeOpaque *decl_, Type *opacifie) : TypeOpaque()
{
    this->decl = decl_;
    this->ident = decl_->ident;
    this->type_opacifie = opacifie;
    this->drapeaux |= TYPE_FUT_VALIDE;
    this->taille_octet = opacifie->taille_octet;
    this->alignement = opacifie->alignement;

    if (opacifie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }
}

/* ************************************************************************** */

static Type *crée_type_pour_lexeme(GenreLexeme lexeme)
{
    switch (lexeme) {
        case GenreLexeme::BOOL:
        {
            return crée_type_bool();
        }
        case GenreLexeme::OCTET:
        {
            return crée_type_octet();
        }
        case GenreLexeme::N8:
        {
            return crée_type_entier(1, true);
        }
        case GenreLexeme::Z8:
        {
            return crée_type_entier(1, false);
        }
        case GenreLexeme::N16:
        {
            return crée_type_entier(2, true);
        }
        case GenreLexeme::Z16:
        {
            return crée_type_entier(2, false);
        }
        case GenreLexeme::N32:
        {
            return crée_type_entier(4, true);
        }
        case GenreLexeme::Z32:
        {
            return crée_type_entier(4, false);
        }
        case GenreLexeme::N64:
        {
            return crée_type_entier(8, true);
        }
        case GenreLexeme::Z64:
        {
            return crée_type_entier(8, false);
        }
        case GenreLexeme::R16:
        {
            return crée_type_reel(2);
        }
        case GenreLexeme::R32:
        {
            return crée_type_reel(4);
        }
        case GenreLexeme::R64:
        {
            return crée_type_reel(8);
        }
        case GenreLexeme::RIEN:
        {
            return crée_type_rien();
        }
        default:
        {
            return nullptr;
        }
    }
}

Typeuse::Typeuse(dls::outils::Synchrone<GrapheDependance> &g,
                 dls::outils::Synchrone<RegistreDesOpérateurs> &o)
    : graphe_(g), operateurs_(o)
{
    /* initialise les types communs */
#define CREE_TYPE_SIMPLE(IDENT)                                                                   \
    TypeBase::IDENT = crée_type_pour_lexeme(GenreLexeme::IDENT);                                  \
    types_simples->ajoute(TypeBase::IDENT)

    CREE_TYPE_SIMPLE(N8);
    CREE_TYPE_SIMPLE(N16);
    CREE_TYPE_SIMPLE(N32);
    CREE_TYPE_SIMPLE(N64);
    CREE_TYPE_SIMPLE(Z8);
    CREE_TYPE_SIMPLE(Z16);
    CREE_TYPE_SIMPLE(Z32);
    CREE_TYPE_SIMPLE(Z64);
    CREE_TYPE_SIMPLE(R16);
    CREE_TYPE_SIMPLE(R32);
    CREE_TYPE_SIMPLE(R64);
    CREE_TYPE_SIMPLE(RIEN);
    CREE_TYPE_SIMPLE(BOOL);
    CREE_TYPE_SIMPLE(OCTET);

#undef CREE_TYPE_SIMPLE

    TypeBase::RIEN->drapeaux |= (TYPE_NE_REQUIERS_PAS_D_INITIALISATION |
                                 INITIALISATION_TYPE_FUT_CREEE);

    TypeBase::ENTIER_CONSTANT = crée_type_entier_constant();
    types_simples->ajoute(TypeBase::ENTIER_CONSTANT);

    TypeBase::EINI = crée_type_eini();
    TypeBase::CHAINE = crée_type_chaine();

    type_type_de_donnees_ = types_type_de_donnees->ajoute_element(nullptr);

    // nous devons créer le pointeur nul avant les autres types, car nous en avons besoin pour
    // définir les opérateurs pour les pointeurs
    auto ptr_nul = types_pointeurs->ajoute_element(nullptr);

    TypeBase::PTR_NUL = ptr_nul;

    for (auto &donnees : donnees_types_communs) {
        auto type = this->type_pour_lexeme(donnees.dt[1]);

        if (donnees.dt[0] == GenreLexeme::TABLEAU) {
            type = this->type_tableau_dynamique(type);
        }
        else if (donnees.dt[0] == GenreLexeme::POINTEUR) {
            type = this->type_pointeur_pour(type);
        }
        else if (donnees.dt[0] == GenreLexeme::REFERENCE) {
            type = this->type_reference_pour(type);
        }
        else {
            assert_rappel(false, [&]() {
                std::cerr << "Genre de type non-géré : " << chaine_du_lexème(donnees.dt[0])
                          << '\n';
            });
        }

        *donnees.ptr_type = type;
    }

    type_contexte = reserve_type_structure(nullptr);
    type_info_type_ = reserve_type_structure(nullptr);

    auto membres_eini = kuri::tableau<MembreTypeComposé, int>();
    membres_eini.ajoute({nullptr, TypeBase::PTR_RIEN, ID::pointeur, 0});
    membres_eini.ajoute({nullptr, type_pointeur_pour(type_info_type_), ID::info, 8});
    auto type_eini = TypeBase::EINI->comme_type_compose();
    type_eini->membres = std::move(membres_eini);
    type_eini->nombre_de_membres_réels = type_eini->membres.taille();
    type_eini->drapeaux |= (TYPE_FUT_VALIDE);

    auto type_chaine = TypeBase::CHAINE->comme_type_compose();
    auto membres_chaine = kuri::tableau<MembreTypeComposé, int>();
    membres_chaine.ajoute({nullptr, TypeBase::PTR_Z8, ID::pointeur, 0});
    membres_chaine.ajoute({nullptr, TypeBase::Z64, ID::taille, 8});
    type_chaine->membres = std::move(membres_chaine);
    type_chaine->nombre_de_membres_réels = type_chaine->membres.taille();
    type_chaine->drapeaux |= (TYPE_FUT_VALIDE);
}

Typeuse::~Typeuse()
{
    for (auto ptr : *types_simples.verrou_ecriture()) {
        memoire::deloge("Type", ptr);
    }

    auto type_chaine = TypeBase::CHAINE->comme_type_compose();
    memoire::deloge("TypeCompose", type_chaine);
    auto type_eini = TypeBase::EINI->comme_type_compose();
    memoire::deloge("TypeCompose", type_eini);
}

void Typeuse::crée_tâches_précompilation(Compilatrice &compilatrice)
{
    auto gestionnaire = compilatrice.gestionnaire_code.verrou_ecriture();
    auto espace = compilatrice.espace_de_travail_defaut;

    /* Crée les fonctions d'initialisations de type qui seront partagées avec d'autres types.
     * Les fonctions pour les entiers sont partagées avec les énums, celle de *rien, avec les
     * autres pointeurs et les fonctions. */
    gestionnaire->requiers_initialisation_type(espace, TypeBase::N8);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::N16);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::N32);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::N64);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::Z8);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::Z16);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::Z32);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::Z64);
    gestionnaire->requiers_initialisation_type(espace, TypeBase::PTR_RIEN);
}

Type *Typeuse::type_pour_lexeme(GenreLexeme lexeme)
{
    switch (lexeme) {
        case GenreLexeme::BOOL:
        {
            return TypeBase::BOOL;
        }
        case GenreLexeme::OCTET:
        {
            return TypeBase::OCTET;
        }
        case GenreLexeme::N8:
        {
            return TypeBase::N8;
        }
        case GenreLexeme::Z8:
        {
            return TypeBase::Z8;
        }
        case GenreLexeme::N16:
        {
            return TypeBase::N16;
        }
        case GenreLexeme::Z16:
        {
            return TypeBase::Z16;
        }
        case GenreLexeme::N32:
        {
            return TypeBase::N32;
        }
        case GenreLexeme::Z32:
        {
            return TypeBase::Z32;
        }
        case GenreLexeme::N64:
        {
            return TypeBase::N64;
        }
        case GenreLexeme::Z64:
        {
            return TypeBase::Z64;
        }
        case GenreLexeme::R16:
        {
            return TypeBase::R16;
        }
        case GenreLexeme::R32:
        {
            return TypeBase::R32;
        }
        case GenreLexeme::R64:
        {
            return TypeBase::R64;
        }
        case GenreLexeme::CHAINE:
        {
            return TypeBase::CHAINE;
        }
        case GenreLexeme::EINI:
        {
            return TypeBase::EINI;
        }
        case GenreLexeme::RIEN:
        {
            return TypeBase::RIEN;
        }
        case GenreLexeme::TYPE_DE_DONNEES:
        {
            return type_type_de_donnees_;
        }
        default:
        {
            return nullptr;
        }
    }
}

TypePointeur *Typeuse::type_pointeur_pour(Type *type,
                                          bool ajoute_operateurs,
                                          bool insere_dans_graphe)
{
    if (!type) {
        return TypeBase::PTR_NUL->comme_type_pointeur();
    }

    auto types_pointeurs_ = types_pointeurs.verrou_ecriture();

    if (type->type_pointeur) {
        auto resultat = type->type_pointeur;
        /* À FAIRE : meilleure structure pour stocker les opérateurs de bases.
         * L'optimisation de l'ajout d'opérateur peut nous faire échouer la compilation si le type
         * fut d'abord créé dans la RI, mais que nous avons besoin des opérateurs pour la
         * validation sémantique plus tard. */
        if ((resultat->drapeaux & TYPE_POSSEDE_OPERATEURS_DE_BASE) == 0) {
            if (ajoute_operateurs) {
                operateurs_->ajoute_opérateurs_basiques_pointeur(resultat);
            }
            resultat->drapeaux |= TYPE_POSSEDE_OPERATEURS_DE_BASE;
        }

        return type->type_pointeur;
    }

    auto resultat = types_pointeurs_->ajoute_element(type);

    if (insere_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(resultat, type);
    }

    if (ajoute_operateurs) {
        operateurs_->ajoute_opérateurs_basiques_pointeur(resultat);
        resultat->drapeaux |= TYPE_POSSEDE_OPERATEURS_DE_BASE;
    }

    type->type_pointeur = resultat;

    /* Tous les pointeurs sont des adresses, il est donc inutile de créer des fonctions spécifiques
     * pour chacun d'entre eux.
     * Lors de la création de la typeuse, les fonction sauvegardées sont nulles. */
    if (init_type_pointeur) {
        assigne_fonction_init(resultat, init_type_pointeur);
    }

    return resultat;
}

TypeReference *Typeuse::type_reference_pour(Type *type)
{
    auto types_references_ = types_references.verrou_ecriture();

    if ((type->drapeaux & POSSEDE_TYPE_REFERENCE) != 0) {
        POUR_TABLEAU_PAGE ((*types_references_)) {
            if (it.type_pointe == type) {
                return &it;
            }
        }
    }

    auto resultat = types_references_->ajoute_element(type);

    auto graphe = graphe_.verrou_ecriture();
    graphe->connecte_type_type(resultat, type);

    return resultat;
}

TypeTableauFixe *Typeuse::type_tableau_fixe(Type *type_pointe, int taille, bool insere_dans_graphe)
{
    assert(taille);

    auto types_tableaux_fixes_ = types_tableaux_fixes.verrou_ecriture();

    if ((type_pointe->drapeaux & POSSEDE_TYPE_TABLEAU_FIXE) != 0) {
        POUR_TABLEAU_PAGE ((*types_tableaux_fixes_)) {
            if (it.type_pointe == type_pointe && it.taille == taille) {
                return &it;
            }
        }
    }

    // les décalages sont à zéros car ceci n'est pas vraiment une structure
    auto membres = kuri::tableau<MembreTypeComposé, int>();
    membres.ajoute(
        {nullptr, type_pointeur_pour(type_pointe, false, insere_dans_graphe), ID::pointeur, 0});
    membres.ajoute({nullptr, TypeBase::Z64, ID::taille, 0});

    auto type = types_tableaux_fixes_->ajoute_element(type_pointe, taille, std::move(membres));

    /* À FAIRE: nous pouvons être en train de traverser le graphe lors de la création du type,
     * alors n'essayons pas de créer une dépendance car nous aurions un verrou mort. */
    if (insere_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_pointe);
    }

    return type;
}

TypeTableauDynamique *Typeuse::type_tableau_dynamique(Type *type_pointe, bool insere_dans_graphe)
{
    auto types_tableaux_dynamiques_ = types_tableaux_dynamiques.verrou_ecriture();

    if ((type_pointe->drapeaux & POSSEDE_TYPE_TABLEAU_DYNAMIQUE) != 0) {
        POUR_TABLEAU_PAGE ((*types_tableaux_dynamiques_)) {
            if (it.type_pointe == type_pointe) {
                return &it;
            }
        }
    }

    auto membres = kuri::tableau<MembreTypeComposé, int>();
    membres.ajoute({nullptr, type_pointeur_pour(type_pointe), ID::pointeur, 0});
    membres.ajoute({nullptr, TypeBase::Z64, ID::taille, 8});
    membres.ajoute({nullptr, TypeBase::Z64, ID::capacite, 16});

    auto type = types_tableaux_dynamiques_->ajoute_element(type_pointe, std::move(membres));

    /* À FAIRE: nous pouvons être en train de traverser le graphe lors de la création du type,
     * alors n'essayons pas de créer une dépendance car nous aurions un verrou mort. */
    if (insere_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_pointe);
    }

    return type;
}

TypeVariadique *Typeuse::type_variadique(Type *type_pointe)
{
    auto types_variadiques_ = types_variadiques.verrou_ecriture();

    POUR_TABLEAU_PAGE ((*types_variadiques_)) {
        if (it.type_pointe == type_pointe) {
            return &it;
        }
    }

    auto membres = kuri::tableau<MembreTypeComposé, int>();
    membres.ajoute({nullptr, type_pointeur_pour(type_pointe), ID::pointeur, 0});
    membres.ajoute({nullptr, TypeBase::Z64, ID::taille, 8});
    membres.ajoute({nullptr, TypeBase::Z64, ID::capacite, 16});

    auto type = types_variadiques_->ajoute_element(type_pointe, std::move(membres));

    if (type_pointe != nullptr) {
        /* crée un tableau dynamique correspond pour que la génération */
        auto tableau_dyn = type_tableau_dynamique(type_pointe);

        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_pointe);
        graphe->connecte_type_type(type, tableau_dyn);

        type->type_tableau_dynamique = tableau_dyn;
    }
    else {
        /* Pour les types variadiques externes, nous ne pouvons générer de fonction
         * d'initialisations.
         * INITIALISATION_TYPE_FUT_CREEE est à cause de attente_sur_type_si_drapeau_manquant.  */
        type->drapeaux |= (TYPE_NE_REQUIERS_PAS_D_INITIALISATION | INITIALISATION_TYPE_FUT_CREEE);
    }

    return type;
}

TypeFonction *Typeuse::discr_type_fonction(TypeFonction *it,
                                           kuri::tablet<Type *, 6> const &entrees)
{
    if (it->types_entrees.taille() != entrees.taille()) {
        return nullptr;
    }

    for (int i = 0; i < it->types_entrees.taille(); ++i) {
        if (it->types_entrees[i] != entrees[i]) {
            return nullptr;
        }
    }

    return it;
}

TypeFonction *Typeuse::type_fonction(kuri::tablet<Type *, 6> const &entrees,
                                     Type *type_sortie,
                                     bool ajoute_operateurs)
{
    auto types_fonctions_ = types_fonctions.verrou_ecriture();

    auto candidat = trie.trouve_type_ou_noeud_insertion(entrees, type_sortie);

    if (std::holds_alternative<TypeFonction *>(candidat)) {
        /* Le type est dans le Trie, retournons-le. */
        return std::get<TypeFonction *>(candidat);
    }

    /* Créons un nouveau type. */
    auto type = types_fonctions_->ajoute_element(entrees, type_sortie);

    /* Insère le type dans le Trie. */
    auto noeud = std::get<Trie::Noeud *>(candidat);
    noeud->type = type;

    if (ajoute_operateurs) {
        operateurs_->ajoute_opérateurs_basiques_fonction(type);
    }

    auto graphe = graphe_.verrou_ecriture();

    POUR (type->types_entrees) {
        graphe->connecte_type_type(type, it);
    }

    // À FAIRE(architecture) : voir commentaire dans TypeFonction::marque_polymorphique()
    if (type_sortie) {
        graphe->connecte_type_type(type, type_sortie);
    }

    /* Les rappels de fonctions sont des pointeurs, donc nous utilisons la même fonction
     * d'initialisation que pour les pointeurs. */
    assigne_fonction_init(type, init_type_pointeur);

    return type;
}

TypeTypeDeDonnees *Typeuse::type_type_de_donnees(Type *type_connu)
{
    if (type_connu == nullptr) {
        return type_type_de_donnees_;
    }

    auto types_type_de_donnees_ = types_type_de_donnees.verrou_ecriture();

    if ((type_connu->drapeaux & POSSEDE_TYPE_TYPE_DE_DONNEES) != 0) {
        return table_types_de_donnees.valeur_ou(type_connu, nullptr);
    }

    auto resultat = types_type_de_donnees_->ajoute_element(type_connu);
    table_types_de_donnees.insère(type_connu, resultat);
    return resultat;
}

TypeStructure *Typeuse::reserve_type_structure(NoeudStruct *decl)
{
    auto type = types_structures->ajoute_element();
    type->decl = decl;

    // decl peut être nulle pour la réservation du type pour InfoType
    if (type->decl) {
        type->nom = decl->lexeme->ident;
    }

    return type;
}

TypeEnum *Typeuse::reserve_type_enum(NoeudEnum *decl)
{
    auto type = types_enums->ajoute_element();
    type->nom = decl->lexeme->ident;
    type->decl = decl;

    return type;
}

TypeUnion *Typeuse::reserve_type_union(NoeudStruct *decl)
{
    auto type = types_unions->ajoute_element();
    type->nom = decl->lexeme->ident;
    type->decl = decl;
    return type;
}

TypeUnion *Typeuse::union_anonyme(const kuri::tablet<MembreTypeComposé, 6> &membres)
{
    auto types_unions_ = types_unions.verrou_ecriture();

    POUR_TABLEAU_PAGE ((*types_unions_)) {
        if (!it.est_anonyme) {
            continue;
        }

        if (it.membres.taille() != membres.taille()) {
            continue;
        }

        auto type_apparie = true;

        for (auto i = 0; i < it.membres.taille(); ++i) {
            if (it.membres[i].type != membres[i].type) {
                type_apparie = false;
                break;
            }
        }

        if (type_apparie) {
            return &it;
        }
    }

    auto type = types_unions_->ajoute_element();
    type->nom = ID::anonyme;

    type->membres.reserve(static_cast<int>(membres.taille()));
    POUR (membres) {
        type->membres.ajoute(it);
    }
    type->nombre_de_membres_réels = type->membres.taille();

    type->est_anonyme = true;
    type->drapeaux |= (TYPE_FUT_VALIDE);

    marque_polymorphique(type);

    if ((type->drapeaux & TYPE_EST_POLYMORPHIQUE) == 0) {
        calcule_taille_type_compose(type, false, 0);
        crée_type_structure(*this, type, type->decalage_index);
    }

    return type;
}

TypeEnum *Typeuse::reserve_type_erreur(NoeudEnum *decl)
{
    auto type = reserve_type_enum(decl);
    type->genre = GenreType::ERREUR;

    return type;
}

TypePolymorphique *Typeuse::crée_polymorphique(IdentifiantCode *ident)
{
    auto types_polymorphiques_ = types_polymorphiques.verrou_ecriture();

    // pour le moment un ident nul est utilisé pour les types polymorphiques des
    // structures dans les types des fonction (foo :: fonc (p: Polymorphe(T = $T)),
    // donc ne déduplique pas ces types pour éviter les problèmes quand nous validons
    // ces expresssions, car les données associées doivent être spécifiques à chaque
    // déclaration
    if (ident) {
        POUR_TABLEAU_PAGE ((*types_polymorphiques_)) {
            if (it.ident == ident) {
                return &it;
            }
        }
    }

    return types_polymorphiques_->ajoute_element(ident);
}

TypeOpaque *Typeuse::crée_opaque(NoeudDeclarationTypeOpaque *decl, Type *type_opacifie)
{
    auto type = types_opaques->ajoute_element(decl, type_opacifie);
    if (type_opacifie) {
        graphe_->connecte_type_type(type, type_opacifie);
    }
    return type;
}

TypeOpaque *Typeuse::monomorphe_opaque(NoeudDeclarationTypeOpaque *decl, Type *type_monomorphique)
{
    auto types_opaques_ = types_opaques.verrou_ecriture();

    POUR_TABLEAU_PAGE ((*types_opaques_)) {
        if (it.decl != decl) {
            continue;
        }

        if (it.type_opacifie == type_monomorphique) {
            return &it;
        }
    }

    auto type = types_opaques_->ajoute_element(decl, type_monomorphique);
    type->drapeaux |= TYPE_FUT_VALIDE;
    graphe_->connecte_type_type(type, type_monomorphique);
    return type;
}

TypeTuple *Typeuse::crée_tuple(const kuri::tablet<MembreTypeComposé, 6> &membres)
{
    auto types_tuples_ = types_tuples.verrou_ecriture();

    POUR_TABLEAU_PAGE ((*types_tuples_)) {
        if (it.membres.taille() != membres.taille()) {
            continue;
        }

        auto trouve = true;

        for (auto i = 0; i < membres.taille(); ++i) {
            if (it.membres[i].type != membres[i].type) {
                trouve = false;
                break;
            }
        }

        if (trouve) {
            return &it;
        }
    }

    auto type = types_tuples_->ajoute_element();
    type->membres.reserve(static_cast<int>(membres.taille()));

    POUR (membres) {
        type->membres.ajoute(it);
        graphe_->connecte_type_type(type, it.type);
    }
    type->nombre_de_membres_réels = type->membres.taille();

    marque_polymorphique(type);

    type->drapeaux |= (TYPE_FUT_VALIDE | TYPE_NE_REQUIERS_PAS_D_INITIALISATION |
                       INITIALISATION_TYPE_FUT_CREEE);

    return type;
}

void Typeuse::rassemble_statistiques(Statistiques &stats) const
{
#define DONNEES_ENTREE(Type, Tableau)                                                             \
#    Type, Tableau->taille(), Tableau->taille() * (taille_de(Type *) + taille_de(Type))

    auto &stats_types = stats.stats_types;

    stats_types.fusionne_entrée({DONNEES_ENTREE(Type, types_simples)});
    stats_types.fusionne_entrée({DONNEES_ENTREE(TypePointeur, types_pointeurs)});
    stats_types.fusionne_entrée({DONNEES_ENTREE(TypeReference, types_references)});
    stats_types.fusionne_entrée({DONNEES_ENTREE(TypeTypeDeDonnees, types_type_de_donnees)});
    stats_types.fusionne_entrée({DONNEES_ENTREE(TypePolymorphique, types_polymorphiques)});

    auto memoire_membres_structures = 0l;
    POUR_TABLEAU_PAGE ((*types_structures.verrou_lecture())) {
        memoire_membres_structures += it.membres.taille_memoire();
    }
    stats_types.fusionne_entrée(
        {DONNEES_ENTREE(TypeStructure, types_structures) + memoire_membres_structures});

    auto memoire_membres_enums = 0l;
    POUR_TABLEAU_PAGE ((*types_enums.verrou_lecture())) {
        memoire_membres_enums += it.membres.taille_memoire();
    }
    stats_types.fusionne_entrée({DONNEES_ENTREE(TypeEnum, types_enums) + memoire_membres_enums});

    auto memoire_membres_unions = 0l;
    POUR_TABLEAU_PAGE ((*types_unions.verrou_lecture())) {
        memoire_membres_unions += it.membres.taille_memoire();
    }
    stats_types.fusionne_entrée(
        {DONNEES_ENTREE(TypeUnion, types_unions) + memoire_membres_unions});

    auto memoire_membres_tuples = 0l;
    POUR_TABLEAU_PAGE ((*types_tuples.verrou_lecture())) {
        memoire_membres_tuples += it.membres.taille_memoire();
    }
    stats_types.fusionne_entrée(
        {DONNEES_ENTREE(TypeTuple, types_tuples) + memoire_membres_tuples});

    auto memoire_membres_tfixes = 0l;
    POUR_TABLEAU_PAGE ((*types_tableaux_fixes.verrou_lecture())) {
        memoire_membres_tfixes += it.membres.taille_memoire();
    }
    stats_types.fusionne_entrée(
        {DONNEES_ENTREE(TypeTableauFixe, types_tableaux_fixes) + memoire_membres_tfixes});

    auto memoire_membres_tdyns = 0l;
    POUR_TABLEAU_PAGE ((*types_tableaux_dynamiques.verrou_lecture())) {
        memoire_membres_tdyns += it.membres.taille_memoire();
    }
    stats_types.fusionne_entrée(
        {DONNEES_ENTREE(TypeTableauDynamique, types_tableaux_dynamiques) + memoire_membres_tdyns});

    auto memoire_membres_tvars = 0l;
    POUR_TABLEAU_PAGE ((*types_variadiques.verrou_lecture())) {
        memoire_membres_tvars += it.membres.taille_memoire();
    }
    stats_types.fusionne_entrée(
        {DONNEES_ENTREE(TypeVariadique, types_variadiques) + memoire_membres_tvars});

    auto memoire_params_fonctions = 0l;
    POUR_TABLEAU_PAGE ((*types_fonctions.verrou_lecture())) {
        memoire_params_fonctions += it.types_entrees.taille_memoire();
    }
    stats_types.fusionne_entrée(
        {DONNEES_ENTREE(TypeFonction, types_fonctions) + memoire_params_fonctions});

    auto type_eini = TypeBase::EINI->comme_type_compose();
    auto type_chaine = TypeBase::CHAINE->comme_type_compose();
    stats_types.fusionne_entrée(
        {"eini",
         1,
         taille_de(TypeCompose) + taille_de(TypeCompose *) + type_eini->membres.taille_memoire()});
    stats_types.fusionne_entrée({"chaine",
                                 1,
                                 taille_de(TypeCompose) + taille_de(TypeCompose *) +
                                     type_chaine->membres.taille_memoire()});

#undef DONNES_ENTREE
}

NoeudDeclaration *Typeuse::decl_pour_info_type(InfoType const *info_type)
{
    POUR_TABLEAU_PAGE ((*types_structures.verrou_lecture())) {
        if (it.info_type == info_type) {
            return decl_pour_type(&it);
        }
    }
    POUR_TABLEAU_PAGE ((*types_unions.verrou_lecture())) {
        if (it.info_type == info_type) {
            return decl_pour_type(&it);
        }
    }
    POUR_TABLEAU_PAGE ((*types_enums.verrou_lecture())) {
        if (it.info_type == info_type) {
            return decl_pour_type(&it);
        }
    }
    return nullptr;
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions diverses pour les types.
 * \{ */

void assigne_fonction_init(Type *type, NoeudDeclarationEnteteFonction *fonction)
{
    type->fonction_init = fonction;
    type->drapeaux |= INITIALISATION_TYPE_FUT_CREEE;
}

/* Retourne vrai si le type à besoin d'une fonction d'initialisation que celle-ci soit partagée
 * ou non.
 */
bool requiers_fonction_initialisation(Type const *type)
{
    return (type->drapeaux & TYPE_NE_REQUIERS_PAS_D_INITIALISATION) == 0;
}

/* Retourne vrai si une fonction d'initialisation doit être créée pour ce type, s'il en besoin
 * et qu'elle n'a pas encore été créée.
 */
bool requiers_création_fonction_initialisation(Type const *type)
{
    if (!requiers_fonction_initialisation(type)) {
        return false;
    }

    /* #fonction_init peut être non-nulle si seulement l'entête est créée. Le drapeaux n'est
     * mis en place que lorsque la fonction et son corps furent créés. */
    if ((type->drapeaux & INITIALISATION_TYPE_FUT_CREEE) != 0) {
        return false;
    }

    if (est_type_polymorphique(type)) {
        return false;
    }

    return true;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux membres des types composés.
 * \{ */

std::optional<InformationMembreTypeCompose> donne_membre_pour_type(TypeCompose const *type_composé,
                                                                   Type const *type)
{
    POUR_INDEX (type_composé->membres) {
        if (it.type == type) {
            return InformationMembreTypeCompose{it, index_it};
        }
    }

    return {};
}

std::optional<InformationMembreTypeCompose> donne_membre_pour_nom(
    TypeCompose const *type_composé, IdentifiantCode const *nom_membre)
{
    POUR_INDEX (type_composé->membres) {
        if (it.nom == nom_membre) {
            return InformationMembreTypeCompose{it, index_it};
        }
    }

    return {};
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux noms hiérarchiques des types.
 * \{ */

static kuri::chaine nom_hiérarchique(NoeudBloc *bloc, kuri::chaine_statique ident)
{
    auto const noms = donne_les_noms_de_la_hiérarchie(bloc);

    Enchaineuse enchaineuse;
    /* -2 pour éviter le nom du module. */
    for (auto i = noms.taille() - 2; i >= 0; --i) {
        enchaineuse.ajoute(noms[i]->nom);
        enchaineuse.ajoute(".");
    }
    enchaineuse.ajoute(ident);

    return enchaineuse.chaine();
}

kuri::chaine_statique donne_nom_hiérarchique(TypeUnion *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = nom_hiérarchique(type->decl ? type->decl->bloc_parent : nullptr,
                                               chaine_type(type, false));
    return type->nom_hiérarchique_;
}

kuri::chaine_statique donne_nom_hiérarchique(TypeEnum *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = nom_hiérarchique(type->decl ? type->decl->bloc_parent : nullptr,
                                               chaine_type(type, false));
    return type->nom_hiérarchique_;
}

kuri::chaine_statique donne_nom_hiérarchique(TypeOpaque *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = nom_hiérarchique(type->decl->bloc_parent, chaine_type(type, false));
    return type->nom_hiérarchique_;
}

kuri::chaine_statique donne_nom_hiérarchique(TypeStructure *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = nom_hiérarchique(type->decl ? type->decl->bloc_parent : nullptr,
                                               chaine_type(type, false));
    return type->nom_hiérarchique_;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux noms portables des types.
 * \{ */

static kuri::chaine nom_portable(NoeudBloc *bloc, kuri::chaine_statique nom)
{
    auto const noms = donne_les_noms_de_la_hiérarchie(bloc);

    Enchaineuse enchaineuse;
    for (auto i = noms.taille() - 1; i >= 0; --i) {
        enchaineuse.ajoute(noms[i]->nom);
    }
    enchaineuse.ajoute(nom);

    return enchaineuse.chaine();
}

kuri::chaine const &donne_nom_portable(TypeUnion *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->decl ? type->decl->bloc_parent : nullptr,
                                       type->nom->nom);
    return type->nom_portable_;
}

kuri::chaine const &donne_nom_portable(TypeEnum *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->decl ? type->decl->bloc_parent : nullptr,
                                       type->nom->nom);
    return type->nom_portable_;
}

kuri::chaine const &donne_nom_portable(TypeOpaque *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->decl->bloc_parent, type->ident->nom);
    return type->nom_portable_;
}

kuri::chaine const &donne_nom_portable(TypeStructure *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->decl ? type->decl->bloc_parent : nullptr,
                                       type->nom->nom);
    return type->nom_portable_;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Marquage des types comme étant polymorphiques.
 * \{ */

void marque_polymorphique(TypeFonction *type)
{
    POUR (type->types_entrees) {
        if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
            return;
        }
    }

    // À FAIRE(architecture) : il est possible que le type_sortie soit nul car ce peut être une
    // union non encore validée, donc son type_le_plus_grand ou type_structure n'est pas encore
    // généré. Il y a plusieurs problèmes à résoudre :
    // - une unité de compilation ne doit aller en RI tant qu'une de ses dépendances n'est pas
    // encore validée (requiers de se débarrasser du graphe et utiliser les unités comme « noeud »)
    // - la gestion des types polymorphiques est à revoir, notamment la manière ils sont stockés
    // - nous ne devrions pas marquée comme polymorphique lors de la génération de RI
    if (type->type_sortie && type->type_sortie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }
}

void marque_polymorphique(TypeCompose *type)
{
    POUR (type->membres) {
        if (it.type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            type->drapeaux |= TYPE_EST_POLYMORPHIQUE;
            return;
        }
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Fonctions pour les unions.
 * \{ */

void crée_type_structure(Typeuse &typeuse, TypeUnion *type, unsigned alignement_membre_actif)
{
    assert(!type->est_nonsure);

    type->type_structure = typeuse.reserve_type_structure(nullptr);

    if (type->type_le_plus_grand) {
        auto membres_ = kuri::tableau<MembreTypeComposé, int>(2);
        membres_[0] = {nullptr, type->type_le_plus_grand, ID::valeur, 0};
        membres_[1] = {nullptr, TypeBase::Z32, ID::membre_actif, alignement_membre_actif};
        type->type_structure->membres = std::move(membres_);
        type->type_structure->nombre_de_membres_réels = type->type_structure->membres.taille();
    }
    else {
        auto membres_ = kuri::tableau<MembreTypeComposé, int>(1);
        membres_[0] = {nullptr, TypeBase::Z32, ID::membre_actif, alignement_membre_actif};
        type->type_structure->membres = std::move(membres_);
        type->type_structure->nombre_de_membres_réels = type->type_structure->membres.taille();
    }

    type->type_structure->taille_octet = type->taille_octet;
    type->type_structure->alignement = type->alignement;
    type->type_structure->nom = type->nom;
    type->type_structure->est_anonyme = type->est_anonyme;
    // Il nous faut la déclaration originelle afin de pouvoir utiliser un typedef différent
    // dans la coulisse pour chaque monomorphisation.
    type->type_structure->decl = type->decl;
    type->type_structure->union_originelle = type;
    /* L'initialisation est créée avec le type de l'union et non celui de la structure. */
    type->type_structure->drapeaux |= (TYPE_FUT_VALIDE | INITIALISATION_TYPE_FUT_CREEE |
                                       UNITE_POUR_INITIALISATION_FUT_CREE);

    typeuse.graphe_->connecte_type_type(type, type->type_structure);
}

/** \} */

/* ************************************************************************** */

static void chaine_type_structure(Enchaineuse &enchaineuse,
                                  const TypeCompose *type_structure,
                                  const NoeudStruct *decl,
                                  bool ajoute_nom_paramètres_polymorphiques)
{
    enchaineuse << type_structure->nom->nom;
    const char *virgule = "(";
    if (decl->est_monomorphisation) {
        POUR ((*decl->bloc_constantes->membres.verrou_lecture())) {
            enchaineuse << virgule;

            if (ajoute_nom_paramètres_polymorphiques) {
                enchaineuse << it->ident->nom << ": ";
            }

            if (it->type->est_type_type_de_donnees()) {
                enchaineuse << chaine_type(it->type->comme_type_type_de_donnees()->type_connu);
            }
            else {
                enchaineuse << it->comme_declaration_constante()->valeur_expression;
            }

            virgule = ", ";
        }
        enchaineuse << ')';
    }
    else if (decl->est_polymorphe) {
        POUR ((*decl->bloc_constantes->membres.verrou_lecture())) {
            enchaineuse << virgule;
            enchaineuse << '$' << it->ident->nom;
            virgule = ", ";
        }
        enchaineuse << ')';
    }
}

static void chaine_type(Enchaineuse &enchaineuse,
                        const Type *type,
                        bool ajoute_nom_paramètres_polymorphiques)
{
    if (type == nullptr) {
        enchaineuse.ajoute("nul");
        return;
    }

    switch (type->genre) {
        case GenreType::EINI:
        {
            enchaineuse.ajoute("eini");
            return;
        }
        case GenreType::CHAINE:
        {
            enchaineuse.ajoute("chaine");
            return;
        }
        case GenreType::RIEN:
        {
            enchaineuse.ajoute("rien");
            return;
        }
        case GenreType::BOOL:
        {
            enchaineuse.ajoute("bool");
            return;
        }
        case GenreType::OCTET:
        {
            enchaineuse.ajoute("octet");
            return;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            enchaineuse.ajoute("entier_constant");
            return;
        }
        case GenreType::ENTIER_NATUREL:
        {
            if (type->taille_octet == 1) {
                enchaineuse.ajoute("n8");
                return;
            }

            if (type->taille_octet == 2) {
                enchaineuse.ajoute("n16");
                return;
            }

            if (type->taille_octet == 4) {
                enchaineuse.ajoute("n32");
                return;
            }

            if (type->taille_octet == 8) {
                enchaineuse.ajoute("n64");
                return;
            }

            enchaineuse.ajoute("invalide");
            return;
        }
        case GenreType::ENTIER_RELATIF:
        {
            if (type->taille_octet == 1) {
                enchaineuse.ajoute("z8");
                return;
            }

            if (type->taille_octet == 2) {
                enchaineuse.ajoute("z16");
                return;
            }

            if (type->taille_octet == 4) {
                enchaineuse.ajoute("z32");
                return;
            }

            if (type->taille_octet == 8) {
                enchaineuse.ajoute("z64");
                return;
            }

            enchaineuse.ajoute("invalide");
            return;
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 2) {
                enchaineuse.ajoute("r16");
                return;
            }

            if (type->taille_octet == 4) {
                enchaineuse.ajoute("r32");
                return;
            }

            if (type->taille_octet == 8) {
                enchaineuse.ajoute("r64");
                return;
            }

            enchaineuse.ajoute("invalide");
            return;
        }
        case GenreType::REFERENCE:
        {
            enchaineuse.ajoute("&");
            chaine_type(enchaineuse,
                        static_cast<TypeReference const *>(type)->type_pointe,
                        ajoute_nom_paramètres_polymorphiques);
            return;
        }
        case GenreType::POINTEUR:
        {
            auto const type_pointe = type->comme_type_pointeur()->type_pointe;
            if (type_pointe == nullptr) {
                enchaineuse.ajoute("type_de(nul)");
            }
            else {
                enchaineuse.ajoute("*");
                chaine_type(enchaineuse, type_pointe, ajoute_nom_paramètres_polymorphiques);
            }
            return;
        }
        case GenreType::UNION:
        {
            auto type_union = static_cast<TypeUnion const *>(type);

            if (!type_union->nom || !type_union->decl) {
                enchaineuse.ajoute("union.anonyme");
                return;
            }

            chaine_type_structure(
                enchaineuse, type_union, type_union->decl, ajoute_nom_paramètres_polymorphiques);
            return;
        }
        case GenreType::STRUCTURE:
        {
            auto type_structure = static_cast<TypeStructure const *>(type);

            if (!type_structure->nom || !type_structure->decl) {
                enchaineuse.ajoute("struct.anonyme");
                return;
            }

            chaine_type_structure(enchaineuse,
                                  type_structure,
                                  type_structure->decl,
                                  ajoute_nom_paramètres_polymorphiques);
            return;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            enchaineuse.ajoute("[]");
            chaine_type(enchaineuse,
                        static_cast<TypeTableauDynamique const *>(type)->type_pointe,
                        ajoute_nom_paramètres_polymorphiques);
            return;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tabl = static_cast<TypeTableauFixe const *>(type);

            enchaineuse << "[" << type_tabl->taille << "]";
            chaine_type(enchaineuse, type_tabl->type_pointe, ajoute_nom_paramètres_polymorphiques);
            return;
        }
        case GenreType::VARIADIQUE:
        {
            auto type_variadique = static_cast<TypeVariadique const *>(type);
            enchaineuse << "...";
            /* N'imprime rien pour les types variadiques externes. */
            if (type_variadique->type_pointe) {
                chaine_type(enchaineuse,
                            type_variadique->type_pointe,
                            ajoute_nom_paramètres_polymorphiques);
            }
            return;
        }
        case GenreType::FONCTION:
        {
            auto type_fonc = static_cast<TypeFonction const *>(type);

            enchaineuse << "fonc";

            auto virgule = '(';

            POUR (type_fonc->types_entrees) {
                enchaineuse << virgule;
                chaine_type(enchaineuse, it, ajoute_nom_paramètres_polymorphiques);
                virgule = ',';
            }

            if (type_fonc->types_entrees.est_vide()) {
                enchaineuse << virgule;
            }
            enchaineuse << ')';

            enchaineuse << '(';
            chaine_type(enchaineuse, type_fonc->type_sortie, ajoute_nom_paramètres_polymorphiques);
            enchaineuse << ')';
            return;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            enchaineuse << static_cast<TypeEnum const *>(type)->nom->nom;
            return;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            auto type_de_donnees = type->comme_type_type_de_donnees();
            enchaineuse << "type_de_données";
            if (type_de_donnees->type_connu) {
                enchaineuse << '(' << chaine_type(type_de_donnees->type_connu) << ')';
            }
            return;
        }
        case GenreType::POLYMORPHIQUE:
        {
            auto type_polymorphique = static_cast<TypePolymorphique const *>(type);
            enchaineuse << "$";

            if (type_polymorphique->est_structure_poly) {
                enchaineuse << type_polymorphique->structure->ident->nom;
                return;
            }

            enchaineuse << type_polymorphique->ident->nom;
            return;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = static_cast<TypeOpaque const *>(type);
            enchaineuse << static_cast<TypeOpaque const *>(type)->ident->nom;

            if (ajoute_nom_paramètres_polymorphiques) {
                enchaineuse << "(";
                chaine_type(
                    enchaineuse, type_opaque->type_opacifie, ajoute_nom_paramètres_polymorphiques);
                enchaineuse << ")";
            }
            return;
        }
        case GenreType::TUPLE:
        {
            auto type_tuple = static_cast<TypeTuple const *>(type);
            enchaineuse << "tuple ";

            auto virgule = '(';
            POUR (type_tuple->membres) {
                enchaineuse << virgule;
                chaine_type(enchaineuse, it.type, ajoute_nom_paramètres_polymorphiques);
                virgule = ',';
            }

            enchaineuse << ')';
            return;
        }
    }
}

kuri::chaine chaine_type(const Type *type, bool ajoute_nom_paramètres_polymorphiques)
{
    Enchaineuse enchaineuse;
    chaine_type(enchaineuse, type, ajoute_nom_paramètres_polymorphiques);
    return enchaineuse.chaine();
}

Type *type_dereference_pour(Type const *type)
{
    if (type->est_type_pointeur()) {
        return type->comme_type_pointeur()->type_pointe;
    }

    if (type->est_type_reference()) {
        return type->comme_type_reference()->type_pointe;
    }

    if (type->est_type_tableau_fixe()) {
        return type->comme_type_tableau_fixe()->type_pointe;
    }

    if (type->est_type_tableau_dynamique()) {
        return type->comme_type_tableau_dynamique()->type_pointe;
    }

    if (type->est_type_variadique()) {
        return type->comme_type_variadique()->type_pointe;
    }

    if (type->est_type_opaque()) {
        return type_dereference_pour(type->comme_type_opaque()->type_opacifie);
    }

    return nullptr;
}

bool est_type_booleen_implicite(Type *type)
{
    return dls::outils::est_element(type->genre,
                                    GenreType::BOOL,
                                    GenreType::CHAINE,
                                    GenreType::EINI,
                                    GenreType::ENTIER_CONSTANT,
                                    GenreType::ENTIER_NATUREL,
                                    GenreType::ENTIER_RELATIF,
                                    GenreType::FONCTION,
                                    GenreType::POINTEUR,
                                    GenreType::TABLEAU_DYNAMIQUE);
}

static inline uint32_t marge_pour_alignement(const uint32_t alignement,
                                             const uint32_t taille_octet)
{
    return (alignement - (taille_octet % alignement)) % alignement;
}

template <bool COMPACTE>
void calcule_taille_structure(TypeCompose *type, uint32_t alignement_desire)
{
    auto decalage = 0u;
    auto alignement_max = 0u;

    POUR (type->donne_membres_pour_code_machine()) {
        assert_rappel(it.type->taille_octet != 0, [&] {
            std::cerr << "Taille octet de 0 pour le type « " << chaine_type(it.type) << " »\n";
        });

        if (!COMPACTE) {
            /* Ajout de rembourrage entre les membres si le type n'est pas compacte. */
            auto alignement_type = it.type->alignement;

            assert_rappel(it.type->alignement != 0, [&] {
                std::cerr << "Alignement de 0 pour le type « " << chaine_type(it.type) << " »\n";
            });

            alignement_max = std::max(alignement_type, alignement_max);
            auto rembourrage = marge_pour_alignement(alignement_type, decalage);
            const_cast<MembreTypeComposé &>(it).rembourrage = rembourrage;
            decalage += rembourrage;
        }

        const_cast<MembreTypeComposé &>(it).decalage = decalage;
        decalage += it.type->taille_octet;
    }

    if (COMPACTE) {
        /* Une structure compacte a un alignement de 1 ce qui permet de lire
         * et écrire et des valeurs à des adresses non-alignées. */
        type->alignement = 1;
    }
    else {
        /* Ajout d'un rembourrage si nécessaire. */
        decalage += marge_pour_alignement(alignement_max, decalage);
        type->alignement = alignement_max;
    }

    if (alignement_desire != 0) {
        decalage += marge_pour_alignement(alignement_desire, decalage);
        type->alignement = alignement_desire;
    }

    type->taille_octet = decalage;
}

void calcule_taille_type_compose(TypeCompose *type, bool compacte, uint32_t alignement_desire)
{
    if (type->est_type_union()) {
        auto type_union = type->comme_type_union();

        auto max_alignement = 0u;
        auto taille_union = 0u;
        auto type_le_plus_grand = Type::nul();

        POUR (type->donne_membres_pour_code_machine()) {
            auto type_membre = it.type;
            auto taille = type_membre->taille_octet;

            /* Ignore les membres qui n'ont pas de type. */
            if (type_membre->est_type_rien()) {
                continue;
            }

            max_alignement = std::max(type_membre->alignement, max_alignement);

            assert_rappel(it.type->alignement != 0, [&] {
                std::cerr << "Dans le calcul de la taille du type : " << chaine_type(type) << '\n';
                std::cerr << "Alignement de 0 pour le type « " << chaine_type(it.type) << " »\n";
            });

            assert_rappel(it.type->taille_octet != 0, [&] {
                std::cerr << "Taille octet de 0 pour le type « " << chaine_type(it.type) << " »\n";
            });

            if (taille > taille_union) {
                type_le_plus_grand = type_membre;
                taille_union = taille;
            }
        }

        /* Pour les unions sûres, il nous faut prendre en compte le
         * membre supplémentaire. */
        if (!type_union->est_nonsure) {
            /* Il est possible que tous les membres soit de type « rien » ou que l'union soit
             * déclarée sans membre. */
            if (taille_union != 0) {
                /* ajoute une marge d'alignement */
                taille_union += marge_pour_alignement(max_alignement, taille_union);
            }

            type_union->decalage_index = taille_union;

            /* ajoute la taille du membre actif */
            taille_union += static_cast<unsigned>(taille_de(int));
            max_alignement = std::max(static_cast<unsigned>(taille_de(int)), max_alignement);

            /* ajoute une marge d'alignement finale */
            taille_union += marge_pour_alignement(max_alignement, taille_union);
        }

        type_union->type_le_plus_grand = type_le_plus_grand;
        type_union->taille_octet = taille_union;
        type_union->alignement = max_alignement;
    }
    else if (type->est_type_structure() || type->est_type_tuple()) {
        if (compacte) {
            calcule_taille_structure<true>(type, alignement_desire);
        }
        else {
            calcule_taille_structure<false>(type, alignement_desire);
        }
    }
}

NoeudDeclaration *decl_pour_type(const Type *type)
{
    if (type->est_type_structure()) {
        return type->comme_type_structure()->decl;
    }

    if (type->est_type_enum()) {
        return type->comme_type_enum()->decl;
    }

    if (type->est_type_erreur()) {
        return type->comme_type_erreur()->decl;
    }

    if (type->est_type_union()) {
        return type->comme_type_union()->decl;
    }

    return nullptr;
}

bool est_type_polymorphique(Type const *type)
{
    if (type->est_type_polymorphique()) {
        return true;
    }

    if (type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        return true;
    }

    auto decl = decl_pour_type(type);
    if (decl && decl->est_type_structure() && decl->comme_type_structure()->est_polymorphe) {
        return true;
    }

    return false;
}

bool est_type_tableau_fixe(Type const *type)
{
    return type->est_type_tableau_fixe() ||
           (type->est_type_opaque() &&
            type->comme_type_opaque()->type_opacifie->est_type_tableau_fixe());
}

bool est_pointeur_vers_tableau_fixe(Type const *type)
{
    if (!type->est_type_pointeur()) {
        return false;
    }

    auto const type_pointeur = type->comme_type_pointeur();

    if (!type_pointeur->type_pointe) {
        return false;
    }

    return est_type_tableau_fixe(type_pointeur->type_pointe);
}

/* Retourne vrai si le type possède un info type qui est seulement une instance de InfoType et non
 * un type dérivé. */
bool est_structure_info_type_défaut(GenreType genre)
{
    switch (genre) {
        case GenreType::EINI:
        case GenreType::RIEN:
        case GenreType::CHAINE:
        case GenreType::TYPE_DE_DONNEES:
        case GenreType::REEL:
        case GenreType::OCTET:
        case GenreType::BOOL:
            return true;
        default:
            return false;
    }
}

Type const *donne_type_opacifié_racine(TypeOpaque const *type_opaque)
{
    Type const *résultat = type_opaque->type_opacifie;
    while (résultat->est_type_opaque()) {
        résultat = résultat->comme_type_opaque()->type_opacifie;
    }
    return résultat;
}

Type const *type_entier_sous_jacent(Type const *type)
{
    if (type->est_type_entier_constant()) {
        return TypeBase::Z32;
    }

    if (type->est_type_enum()) {
        return type->comme_type_enum()->type_sous_jacent;
    }

    if (type->est_type_erreur()) {
        return type->comme_type_erreur()->type_sous_jacent;
    }

    if (type->est_type_type_de_donnees()) {
        return TypeBase::Z64;
    }

    if (type->est_type_octet()) {
        return TypeBase::N8;
    }

    if (type->est_type_entier_naturel() || type->est_type_entier_relatif()) {
        return type;
    }

    return nullptr;
}

std::optional<uint32_t> est_type_de_base(TypeStructure const *type_dérivé,
                                         TypeStructure const *type_base_potentiel)
{
    POUR (type_dérivé->types_employés) {
        auto struct_employée = it->type->comme_type_structure();
        if (struct_employée == type_base_potentiel) {
            return it->decalage;
        }

        auto décalage_depuis_struct_employée = est_type_de_base(struct_employée,
                                                                type_base_potentiel);
        if (décalage_depuis_struct_employée) {
            return it->decalage + décalage_depuis_struct_employée.value();
        }
    }

    return {};
}

std::optional<uint32_t> est_type_de_base(Type const *type_dérivé, Type const *type_base_potentiel)
{
    if (type_dérivé->est_type_structure() && type_base_potentiel->est_type_structure()) {
        return est_type_de_base(type_dérivé->comme_type_structure(),
                                type_base_potentiel->comme_type_structure());
    }

    return {};
}

bool est_type_pointeur_nul(Type const *type)
{
    return type->est_type_pointeur() && type->comme_type_pointeur()->type_pointe == nullptr;
}

ResultatRechercheMembre trouve_index_membre_unique_type_compatible(TypeCompose const *type,
                                                                   Type const *type_a_tester)
{
    auto const pointeur_nul = est_type_pointeur_nul(type_a_tester);
    int index_membre = -1;
    int index_courant = 0;
    POUR (type->membres) {
        if (it.type == type_a_tester) {
            if (index_membre != -1) {
                return PlusieursMembres{-1};
            }

            index_membre = index_courant;
        }
        else if (type_a_tester->est_type_pointeur() && it.type->est_type_pointeur()) {
            if (pointeur_nul) {
                if (index_membre != -1) {
                    return PlusieursMembres{-1};
                }

                index_membre = index_courant;
            }
            else {
                auto type_pointe_de = type_a_tester->comme_type_pointeur()->type_pointe;
                auto type_pointe_vers = it.type->comme_type_pointeur()->type_pointe;

                if (est_type_de_base(type_pointe_de, type_pointe_vers)) {
                    if (index_membre != -1) {
                        return PlusieursMembres{-1};
                    }

                    index_membre = index_courant;
                }
            }
        }
        else if (est_type_entier(it.type) && type_a_tester->est_type_entier_constant()) {
            if (index_membre != -1) {
                return PlusieursMembres{-1};
            }

            index_membre = index_courant;
        }

        index_courant += 1;
    }

    if (index_membre == -1) {
        return AucunMembre{-1};
    }

    return IndexMembre{index_membre};
}

/* Calcule la « profondeur » du type : à savoir, le nombre de déréférencement du type (jusqu'à
 * arriver à un type racine) + 1.
 * Par exemple, *z32 a une profondeur de 2 (1 déréférencement de pointeur + 1), alors que []*z32 en
 * a une de 3. */
int donne_profondeur_type(Type const *type)
{
    auto profondeur_type = 1;
    auto type_courant = type;
    while (Type *sous_type = type_dereference_pour(type_courant)) {
        profondeur_type += 1;
        type_courant = sous_type;
    }
    return profondeur_type;
}

bool est_type_valide_pour_membre(Type const *membre_type)
{
    if (membre_type->est_type_rien()) {
        return false;
    }

    if (membre_type->est_type_variadique()) {
        return false;
    }

    return true;
}

bool peut_construire_union_via_rien(TypeUnion const *type_union)
{
    POUR (type_union->membres) {
        if (it.type->est_type_rien()) {
            return true;
        }
    }

    return false;
}

/* Décide si le type peut être utilisé pour les expressions d'indexages basiques du langage.
 * NOTE : les entiers relatifs ne sont pas considérées ici car nous utilisons cette décision pour
 * transtyper automatiquement vers le type cible (z64), et nous les gérons séparément. */
bool est_type_implicitement_utilisable_pour_indexage(Type const *type)
{
    if (type->est_type_entier_naturel()) {
        return true;
    }

    if (type->est_type_octet()) {
        return true;
    }

    if (type->est_type_enum()) {
        /* Pour l'instant, les énum_drapeaux ne sont pas utilisable, car les index peuvent être
         * arbitrairement larges. */
        return !type->comme_type_enum()->est_drapeau;
    }

    if (type->est_type_bool()) {
        return true;
    }

    if (type->est_type_type_de_donnees()) {
        /* Les type_de_données doivent pouvoir être utilisé pour indexer la table des types, car
         * leurs valeurs dépends de l'index du type dans ladite table. */
        return true;
    }

    if (type->est_type_opaque()) {
        return est_type_implicitement_utilisable_pour_indexage(
            type->comme_type_opaque()->type_opacifie);
    }

    return false;
}

bool peut_etre_type_constante(Type const *type)
{
    switch (type->genre) {
        /* Possible mais non supporté pour le moment. */
        case GenreType::STRUCTURE:
        /* Il n'est pas encore clair comment prendre le pointeur de la constante pour les tableaux
         * dynamiques. */
        case GenreType::TABLEAU_DYNAMIQUE:
        /* Sémantiquement, les variadiques ne peuvent être utilisées que pour les paramètres de
         * fonctions. */
        case GenreType::VARIADIQUE:
        /* Il n'est pas claire comment gérer les unions, les sûres doivent avoir un membre
         * actif, et les valeurs pour les sûres ou nonsûres doivent être transtypées sur le
         * lieu d'utilisation. */
        case GenreType::UNION:
        /* Un eini doit avoir une info-type, et prendre une valeur par pointeur, qui n'est pas
         * encore supporté pour les constantes. */
        case GenreType::EINI:
        /* Les tuples ne sont que pour les retours de fonctions. */
        case GenreType::TUPLE:
        case GenreType::REFERENCE:
        case GenreType::POINTEUR:
        case GenreType::POLYMORPHIQUE:
        case GenreType::RIEN:
        {
            return false;
        }
        default:
        {
            return true;
        }
    }
}

bool est_type_opacifié(Type const *type_dest, Type const *type_source)
{
    return type_dest->est_type_opaque() &&
           type_dest->comme_type_opaque()->type_opacifie == type_source;
}

bool est_type_fondamental(const Type *type)
{
    switch (type->genre) {
        case GenreType::EINI:
        case GenreType::CHAINE:
        case GenreType::RIEN:
        case GenreType::REFERENCE:
        case GenreType::UNION:
        case GenreType::STRUCTURE:
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::TABLEAU_FIXE:
        case GenreType::VARIADIQUE:
        case GenreType::POLYMORPHIQUE:
        case GenreType::TUPLE:
        {
            return false;
        }
        case GenreType::BOOL:
        case GenreType::OCTET:
        case GenreType::ENTIER_CONSTANT:
        case GenreType::ENTIER_NATUREL:
        case GenreType::ENTIER_RELATIF:
        case GenreType::REEL:
        case GenreType::POINTEUR:
        case GenreType::FONCTION:
        case GenreType::ENUM:
        case GenreType::ERREUR:
        case GenreType::TYPE_DE_DONNEES:
        {
            return true;
        }
        case GenreType::OPAQUE:
        {
            return est_type_fondamental(type->comme_type_opaque()->type_opacifie);
        }
    }

    return false;
}

void attentes_sur_types_si_drapeau_manquant(kuri::ensemblon<Type *, 16> const &types,
                                            int drapeau,
                                            kuri::tablet<Attente, 16> &attentes)
{
    auto visités = kuri::ensemblon<Type *, 16>();
    auto pile = kuri::pile<Type *>();

    pour_chaque_element(types, [&pile](auto &type) {
        pile.empile(type);
        return kuri::DécisionItération::Continue;
    });

    while (!pile.est_vide()) {
        auto type_courant = pile.depile();

        /* Les types variadiques ou pointeur nul peuvent avoir des types déréférencés nuls. */
        if (!type_courant) {
            continue;
        }

        if (visités.possède(type_courant)) {
            continue;
        }

        visités.insere(type_courant);

        if ((type_courant->drapeaux & drapeau) == 0) {
            attentes.ajoute(Attente::sur_type(type_courant));
        }

        switch (type_courant->genre) {
            case GenreType::POLYMORPHIQUE:
            case GenreType::TUPLE:
            case GenreType::EINI:
            case GenreType::CHAINE:
            case GenreType::RIEN:
            case GenreType::BOOL:
            case GenreType::OCTET:
            case GenreType::TYPE_DE_DONNEES:
            case GenreType::REEL:
            case GenreType::ENTIER_CONSTANT:
            case GenreType::ENTIER_NATUREL:
            case GenreType::ENTIER_RELATIF:
            case GenreType::ENUM:
            case GenreType::ERREUR:
            {
                break;
            }
            case GenreType::FONCTION:
            {
                auto type_fonction = type_courant->comme_type_fonction();
                POUR (type_fonction->types_entrees) {
                    pile.empile(it);
                }
                pile.empile(type_fonction->type_sortie);
                break;
            }
            case GenreType::UNION:
            case GenreType::STRUCTURE:
            {
                auto type_compose = static_cast<TypeCompose *>(type_courant);
                POUR (type_compose->membres) {
                    pile.empile(it.type);
                }
                break;
            }
            case GenreType::REFERENCE:
            {
                pile.empile(type_courant->comme_type_reference()->type_pointe);
                break;
            }
            case GenreType::POINTEUR:
            {
                pile.empile(type_courant->comme_type_pointeur()->type_pointe);
                break;
            }
            case GenreType::VARIADIQUE:
            {
                pile.empile(type_courant->comme_type_variadique()->type_pointe);
                break;
            }
            case GenreType::TABLEAU_DYNAMIQUE:
            {
                pile.empile(type_courant->comme_type_tableau_dynamique()->type_pointe);
                break;
            }
            case GenreType::TABLEAU_FIXE:
            {
                pile.empile(type_courant->comme_type_tableau_fixe()->type_pointe);
                break;
            }
            case GenreType::OPAQUE:
            {
                pile.empile(type_courant->comme_type_opaque()->type_opacifie);
                break;
            }
        }
    }
}

std::optional<Attente> attente_sur_type_si_drapeau_manquant(
    kuri::ensemblon<Type *, 16> const &types, int drapeau)
{
    kuri::tablet<Attente, 16> attentes;
    attentes_sur_types_si_drapeau_manquant(types, drapeau, attentes);

    if (attentes.taille()) {
        return attentes[0];
    }

    return {};
}

/* --------------------------------------------------------------------------- */

Trie::Noeud *Trie::StockageEnfants::trouve_noeud_pour_type(const Type *type)
{
    if (enfants.taille() >= TAILLE_MAX_ENFANTS_TABLET && table.taille() != 0) {
        return table.valeur_ou(type, nullptr);
    }

    POUR (enfants) {
        if (it->type == type) {
            return it;
        }
    }
    return nullptr;
}

int64_t Trie::StockageEnfants::taille() const
{
    if (enfants.taille() >= TAILLE_MAX_ENFANTS_TABLET && table.taille() != 0) {
        return table.taille();
    }
    return enfants.taille();
}

void Trie::StockageEnfants::ajoute(Noeud *noeud)
{
    if (enfants.taille() >= TAILLE_MAX_ENFANTS_TABLET) {
        if (table.taille() == 0) {
            POUR (enfants) {
                table.insère(it->type, it);
            }
        }

        table.insère(noeud->type, noeud);
        return;
    }

    enfants.ajoute(noeud);
}

Trie::Noeud *Trie::Noeud::trouve_noeud_pour_type(const Type *type_)
{
    return enfants.trouve_noeud_pour_type(type_);
}

Trie::Noeud *Trie::Noeud::trouve_noeud_sortie_pour_type(const Type *type_)
{
    return enfants_sortie.trouve_noeud_pour_type(type_);
}

Trie::TypeResultat Trie::trouve_type_ou_noeud_insertion(const kuri::tablet<Type *, 6> &entrees,
                                                        Type *type_sortie)
{
    if (racine == nullptr) {
        racine = noeuds.ajoute_element();
    }

    auto enfant_courant = racine;
    POUR (entrees) {
        auto enfant_suivant = enfant_courant->trouve_noeud_pour_type(it);
        if (!enfant_suivant) {
            enfant_suivant = ajoute_enfant(enfant_courant, it, false);
        }
        enfant_courant = enfant_suivant;
    }

    auto enfant_suivant = enfant_courant->trouve_noeud_sortie_pour_type(type_sortie);
    if (!enfant_suivant) {
        enfant_suivant = ajoute_enfant(enfant_courant, type_sortie, true);
    }

    if (enfant_suivant->enfants.taille() == 0) {
        /* Le type fonction n'est pas dans l'arbre, retournons le noeud qui devra le tenir. */
        return ajoute_enfant(enfant_suivant, nullptr, false);
    }

    /* Le type fonction est dans l'arbre, retournons-le. */
    return const_cast<TypeFonction *>(
        enfant_suivant->enfants.enfants[0]->type->comme_type_fonction());
}

Trie::Noeud *Trie::ajoute_enfant(Noeud *parent, const Type *type, bool est_sortie)
{
    auto enfant = noeuds.ajoute_element();
    enfant->type = type;
    if (est_sortie) {
        parent->enfants_sortie.ajoute(enfant);
    }
    else {
        parent->enfants.ajoute(enfant);
    }
    return enfant;
}

void imprime_genre_type_pour_assert(GenreType genre)
{
    std::cerr << "Le type est " << genre << "\n";
}
