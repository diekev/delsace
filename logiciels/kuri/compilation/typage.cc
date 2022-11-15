/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "typage.hh"

#include "biblinternes/outils/assert.hh"

#include <algorithm>

#include "parsage/identifiant.hh"
#include "parsage/outils_lexemes.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "graphe_dependance.hh"
#include "operateurs.hh"

#include "statistiques/statistiques.hh"

#include "structures/pile.hh"

/* ************************************************************************** */

struct DonneesTypeCommun {
    TypeBase val_enum;
    GenreLexeme dt[2];
};

static DonneesTypeCommun donnees_types_communs[] = {
    {TypeBase::PTR_N8, {GenreLexeme::POINTEUR, GenreLexeme::N8}},
    {TypeBase::PTR_N16, {GenreLexeme::POINTEUR, GenreLexeme::N16}},
    {TypeBase::PTR_N32, {GenreLexeme::POINTEUR, GenreLexeme::N32}},
    {TypeBase::PTR_N64, {GenreLexeme::POINTEUR, GenreLexeme::N64}},
    {TypeBase::PTR_Z8, {GenreLexeme::POINTEUR, GenreLexeme::Z8}},
    {TypeBase::PTR_Z16, {GenreLexeme::POINTEUR, GenreLexeme::Z16}},
    {TypeBase::PTR_Z32, {GenreLexeme::POINTEUR, GenreLexeme::Z32}},
    {TypeBase::PTR_Z64, {GenreLexeme::POINTEUR, GenreLexeme::Z64}},
    {TypeBase::PTR_R16, {GenreLexeme::POINTEUR, GenreLexeme::R16}},
    {TypeBase::PTR_R32, {GenreLexeme::POINTEUR, GenreLexeme::R32}},
    {TypeBase::PTR_R64, {GenreLexeme::POINTEUR, GenreLexeme::R64}},
    {TypeBase::PTR_EINI, {GenreLexeme::POINTEUR, GenreLexeme::EINI}},
    {TypeBase::PTR_CHAINE, {GenreLexeme::POINTEUR, GenreLexeme::CHAINE}},
    {TypeBase::PTR_RIEN, {GenreLexeme::POINTEUR, GenreLexeme::RIEN}},
    {TypeBase::PTR_NUL, {GenreLexeme::POINTEUR, GenreLexeme::NUL}},
    {TypeBase::PTR_BOOL, {GenreLexeme::POINTEUR, GenreLexeme::BOOL}},
    {TypeBase::PTR_OCTET, {GenreLexeme::POINTEUR, GenreLexeme::OCTET}},

    {TypeBase::REF_N8, {GenreLexeme::REFERENCE, GenreLexeme::N8}},
    {TypeBase::REF_N16, {GenreLexeme::REFERENCE, GenreLexeme::N16}},
    {TypeBase::REF_N32, {GenreLexeme::REFERENCE, GenreLexeme::N32}},
    {TypeBase::REF_N64, {GenreLexeme::REFERENCE, GenreLexeme::N64}},
    {TypeBase::REF_Z8, {GenreLexeme::REFERENCE, GenreLexeme::Z8}},
    {TypeBase::REF_Z16, {GenreLexeme::REFERENCE, GenreLexeme::Z16}},
    {TypeBase::REF_Z32, {GenreLexeme::REFERENCE, GenreLexeme::Z32}},
    {TypeBase::REF_Z64, {GenreLexeme::REFERENCE, GenreLexeme::Z64}},
    {TypeBase::REF_R16, {GenreLexeme::REFERENCE, GenreLexeme::R16}},
    {TypeBase::REF_R32, {GenreLexeme::REFERENCE, GenreLexeme::R32}},
    {TypeBase::REF_R64, {GenreLexeme::REFERENCE, GenreLexeme::R64}},
    {TypeBase::REF_EINI, {GenreLexeme::REFERENCE, GenreLexeme::EINI}},
    {TypeBase::REF_CHAINE, {GenreLexeme::REFERENCE, GenreLexeme::CHAINE}},
    {TypeBase::REF_RIEN, {GenreLexeme::REFERENCE, GenreLexeme::RIEN}},
    {TypeBase::REF_BOOL, {GenreLexeme::REFERENCE, GenreLexeme::BOOL}},

    {TypeBase::TABL_N8, {GenreLexeme::TABLEAU, GenreLexeme::N8}},
    {TypeBase::TABL_N16, {GenreLexeme::TABLEAU, GenreLexeme::N16}},
    {TypeBase::TABL_N32, {GenreLexeme::TABLEAU, GenreLexeme::N32}},
    {TypeBase::TABL_N64, {GenreLexeme::TABLEAU, GenreLexeme::N64}},
    {TypeBase::TABL_Z8, {GenreLexeme::TABLEAU, GenreLexeme::Z8}},
    {TypeBase::TABL_Z16, {GenreLexeme::TABLEAU, GenreLexeme::Z16}},
    {TypeBase::TABL_Z32, {GenreLexeme::TABLEAU, GenreLexeme::Z32}},
    {TypeBase::TABL_Z64, {GenreLexeme::TABLEAU, GenreLexeme::Z64}},
    {TypeBase::TABL_R16, {GenreLexeme::TABLEAU, GenreLexeme::R16}},
    {TypeBase::TABL_R32, {GenreLexeme::TABLEAU, GenreLexeme::R32}},
    {TypeBase::TABL_R64, {GenreLexeme::TABLEAU, GenreLexeme::R64}},
    {TypeBase::TABL_EINI, {GenreLexeme::TABLEAU, GenreLexeme::EINI}},
    {TypeBase::TABL_CHAINE, {GenreLexeme::TABLEAU, GenreLexeme::CHAINE}},
    {TypeBase::TABL_BOOL, {GenreLexeme::TABLEAU, GenreLexeme::BOOL}},
    {TypeBase::TABL_OCTET, {GenreLexeme::TABLEAU, GenreLexeme::OCTET}},
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

/* ************************************************************************** */

Type *Type::cree_entier(unsigned taille_octet, bool est_naturel)
{
    auto type = memoire::loge<Type>("Type");
    type->genre = est_naturel ? GenreType::ENTIER_NATUREL : GenreType::ENTIER_RELATIF;
    type->taille_octet = taille_octet;
    type->alignement = taille_octet;
    type->drapeaux |= (TYPE_FUT_VALIDE | TYPE_EST_NORMALISE);
    return type;
}

Type *Type::cree_entier_constant()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::ENTIER_CONSTANT;
    type->drapeaux |= (TYPE_FUT_VALIDE);
    return type;
}

Type *Type::cree_reel(unsigned taille_octet)
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::REEL;
    type->taille_octet = taille_octet;
    type->alignement = taille_octet;
    type->drapeaux |= (TYPE_FUT_VALIDE | TYPE_EST_NORMALISE);
    return type;
}

Type *Type::cree_rien()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::RIEN;
    type->taille_octet = 0;
    type->drapeaux |= (TYPE_FUT_VALIDE | TYPE_EST_NORMALISE);
    return type;
}

Type *Type::cree_bool()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::BOOL;
    type->taille_octet = 1;
    type->alignement = 1;
    type->drapeaux |= (TYPE_FUT_VALIDE | TYPE_EST_NORMALISE);
    return type;
}

Type *Type::cree_octet()
{
    auto type = memoire::loge<Type>("Type");
    type->genre = GenreType::OCTET;
    type->taille_octet = 1;
    type->alignement = 1;
    type->drapeaux |= (TYPE_FUT_VALIDE | TYPE_EST_NORMALISE);
    return type;
}

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
    this->marque_polymorphique();
    this->drapeaux |= (TYPE_FUT_VALIDE);
}

void TypeFonction::marque_polymorphique()
{
    POUR (types_entrees) {
        if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
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
    if (type_sortie && type_sortie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }
}

TypeCompose *TypeCompose::cree_eini()
{
    auto type = memoire::loge<TypeCompose>("TypeCompose");
    type->genre = GenreType::EINI;
    type->taille_octet = 16;
    type->alignement = 8;
    type->drapeaux = (TYPE_EST_NORMALISE);
    return type;
}

TypeCompose *TypeCompose::cree_chaine()
{
    auto type = memoire::loge<TypeCompose>("TypeCompose");
    type->genre = GenreType::CHAINE;
    type->taille_octet = 16;
    type->alignement = 8;
    type->drapeaux = (TYPE_EST_NORMALISE);
    return type;
}

void TypeCompose::marque_polymorphique()
{
    POUR (membres) {
        if (it.type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
            return;
        }
    }
}

TypeTableauFixe::TypeTableauFixe(Type *type_pointe_,
                                 int taille_,
                                 kuri::tableau<TypeCompose::Membre, int> &&membres_)
    : TypeTableauFixe()
{
    assert(type_pointe_);

    this->membres = std::move(membres_);
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
                                           kuri::tableau<TypeCompose::Membre, int> &&membres_)
    : TypeTableauDynamique()
{
    assert(type_pointe_);

    this->membres = std::move(membres_);
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
                               kuri::tableau<TypeCompose::Membre, int> &&membres_)
    : TypeVariadique()
{
    this->type_pointe = type_pointe_;

    if (type_pointe_ && type_pointe_->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
    }

    this->membres = std::move(membres_);
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

void TypeTuple::marque_polymorphique()
{
    POUR (membres) {
        if (it.type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            this->drapeaux |= TYPE_EST_POLYMORPHIQUE;
            return;
        }
    }
}

/* ************************************************************************** */

static Type *cree_type_pour_lexeme(GenreLexeme lexeme)
{
    switch (lexeme) {
        case GenreLexeme::BOOL:
        {
            return Type::cree_bool();
        }
        case GenreLexeme::OCTET:
        {
            return Type::cree_octet();
        }
        case GenreLexeme::N8:
        {
            return Type::cree_entier(1, true);
        }
        case GenreLexeme::Z8:
        {
            return Type::cree_entier(1, false);
        }
        case GenreLexeme::N16:
        {
            return Type::cree_entier(2, true);
        }
        case GenreLexeme::Z16:
        {
            return Type::cree_entier(2, false);
        }
        case GenreLexeme::N32:
        {
            return Type::cree_entier(4, true);
        }
        case GenreLexeme::Z32:
        {
            return Type::cree_entier(4, false);
        }
        case GenreLexeme::N64:
        {
            return Type::cree_entier(8, true);
        }
        case GenreLexeme::Z64:
        {
            return Type::cree_entier(8, false);
        }
        case GenreLexeme::R16:
        {
            return Type::cree_reel(2);
        }
        case GenreLexeme::R32:
        {
            return Type::cree_reel(4);
        }
        case GenreLexeme::R64:
        {
            return Type::cree_reel(8);
        }
        case GenreLexeme::RIEN:
        {
            return Type::cree_rien();
        }
        default:
        {
            return nullptr;
        }
    }
}

Typeuse::Typeuse(dls::outils::Synchrone<GrapheDependance> &g,
                 dls::outils::Synchrone<Operateurs> &o)
    : graphe_(g), operateurs_(o)
{
    /* initialise les types communs */
    types_communs.redimensionne(static_cast<long>(TypeBase::TOTAL));

    type_eini = TypeCompose::cree_eini();
    type_chaine = TypeCompose::cree_chaine();

    types_communs[static_cast<long>(TypeBase::N8)] = cree_type_pour_lexeme(GenreLexeme::N8);
    types_communs[static_cast<long>(TypeBase::N16)] = cree_type_pour_lexeme(GenreLexeme::N16);
    types_communs[static_cast<long>(TypeBase::N32)] = cree_type_pour_lexeme(GenreLexeme::N32);
    types_communs[static_cast<long>(TypeBase::N64)] = cree_type_pour_lexeme(GenreLexeme::N64);
    types_communs[static_cast<long>(TypeBase::Z8)] = cree_type_pour_lexeme(GenreLexeme::Z8);
    types_communs[static_cast<long>(TypeBase::Z16)] = cree_type_pour_lexeme(GenreLexeme::Z16);
    types_communs[static_cast<long>(TypeBase::Z32)] = cree_type_pour_lexeme(GenreLexeme::Z32);
    types_communs[static_cast<long>(TypeBase::Z64)] = cree_type_pour_lexeme(GenreLexeme::Z64);
    types_communs[static_cast<long>(TypeBase::R16)] = cree_type_pour_lexeme(GenreLexeme::R16);
    types_communs[static_cast<long>(TypeBase::R32)] = cree_type_pour_lexeme(GenreLexeme::R32);
    types_communs[static_cast<long>(TypeBase::R64)] = cree_type_pour_lexeme(GenreLexeme::R64);
    types_communs[static_cast<long>(TypeBase::EINI)] = type_eini;
    types_communs[static_cast<long>(TypeBase::CHAINE)] = type_chaine;
    types_communs[static_cast<long>(TypeBase::RIEN)] = cree_type_pour_lexeme(GenreLexeme::RIEN);
    types_communs[static_cast<long>(TypeBase::BOOL)] = cree_type_pour_lexeme(GenreLexeme::BOOL);
    types_communs[static_cast<long>(TypeBase::OCTET)] = cree_type_pour_lexeme(GenreLexeme::OCTET);
    types_communs[static_cast<long>(TypeBase::ENTIER_CONSTANT)] = Type::cree_entier_constant();

    for (auto i = static_cast<long>(TypeBase::N8);
         i <= static_cast<long>(TypeBase::ENTIER_CONSTANT);
         ++i) {
        if (i == static_cast<long>(TypeBase::EINI) || i == static_cast<long>(TypeBase::CHAINE)) {
            continue;
        }

        types_simples->ajoute(types_communs[i]);
    }

    type_type_de_donnees_ = types_type_de_donnees->ajoute_element(nullptr);

    // nous devons créer le pointeur nul avant les autres types, car nous en avons besoin pour
    // définir les opérateurs pour les pointeurs
    auto ptr_nul = types_pointeurs->ajoute_element(nullptr);

    types_communs[static_cast<long>(TypeBase::PTR_NUL)] = ptr_nul;

    for (auto &donnees : donnees_types_communs) {
        auto const idx = static_cast<long>(donnees.val_enum);
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
                std::cerr << "Genre de type non-géré : " << chaine_du_lexeme(donnees.dt[0])
                          << '\n';
            });
        }

        types_communs[idx] = type;
    }

    type_contexte = reserve_type_structure(nullptr);
    type_info_type_ = reserve_type_structure(nullptr);

    auto membres_eini = kuri::tableau<TypeCompose::Membre, int>();
    membres_eini.ajoute(
        {nullptr, types_communs[static_cast<long>(TypeBase::PTR_RIEN)], ID::pointeur, 0});
    membres_eini.ajoute({nullptr, type_pointeur_pour(type_info_type_), ID::info, 8});
    type_eini->membres = std::move(membres_eini);
    type_eini->drapeaux |= (TYPE_FUT_VALIDE | TYPE_EST_NORMALISE);

    auto membres_chaine = kuri::tableau<TypeCompose::Membre, int>();
    membres_chaine.ajoute(
        {nullptr, types_communs[static_cast<long>(TypeBase::PTR_Z8)], ID::pointeur, 0});
    membres_chaine.ajoute(
        {nullptr, types_communs[static_cast<long>(TypeBase::Z64)], ID::taille, 8});
    type_chaine->membres = std::move(membres_chaine);
    type_chaine->drapeaux |= (TYPE_FUT_VALIDE | TYPE_EST_NORMALISE);

    POUR (types_communs) {
        it->drapeaux |= TYPE_EST_NORMALISE;
    }
}

Typeuse::~Typeuse()
{
#define DELOGE_TYPES(Type, Tableau)                                                               \
    for (auto ptr : *Tableau.verrou_lecture()) {                                                  \
        memoire::deloge(#Type, ptr);                                                              \
    }

    DELOGE_TYPES(TypePointeur, types_simples);

    memoire::deloge("TypeCompose", type_eini);
    memoire::deloge("TypeCompose", type_chaine);

#undef DELOGE_TYPES
}

Type *Typeuse::type_pour_lexeme(GenreLexeme lexeme)
{
    switch (lexeme) {
        case GenreLexeme::BOOL:
        {
            return types_communs[static_cast<long>(TypeBase::BOOL)];
        }
        case GenreLexeme::OCTET:
        {
            return types_communs[static_cast<long>(TypeBase::OCTET)];
        }
        case GenreLexeme::N8:
        {
            return types_communs[static_cast<long>(TypeBase::N8)];
        }
        case GenreLexeme::Z8:
        {
            return types_communs[static_cast<long>(TypeBase::Z8)];
        }
        case GenreLexeme::N16:
        {
            return types_communs[static_cast<long>(TypeBase::N16)];
        }
        case GenreLexeme::Z16:
        {
            return types_communs[static_cast<long>(TypeBase::Z16)];
        }
        case GenreLexeme::N32:
        {
            return types_communs[static_cast<long>(TypeBase::N32)];
        }
        case GenreLexeme::Z32:
        {
            return types_communs[static_cast<long>(TypeBase::Z32)];
        }
        case GenreLexeme::N64:
        {
            return types_communs[static_cast<long>(TypeBase::N64)];
        }
        case GenreLexeme::Z64:
        {
            return types_communs[static_cast<long>(TypeBase::Z64)];
        }
        case GenreLexeme::R16:
        {
            return types_communs[static_cast<long>(TypeBase::R16)];
        }
        case GenreLexeme::R32:
        {
            return types_communs[static_cast<long>(TypeBase::R32)];
        }
        case GenreLexeme::R64:
        {
            return types_communs[static_cast<long>(TypeBase::R64)];
        }
        case GenreLexeme::CHAINE:
        {
            return types_communs[static_cast<long>(TypeBase::CHAINE)];
        }
        case GenreLexeme::EINI:
        {
            return types_communs[static_cast<long>(TypeBase::EINI)];
        }
        case GenreLexeme::RIEN:
        {
            return types_communs[static_cast<long>(TypeBase::RIEN)];
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
        return ((*this)[TypeBase::PTR_NUL])->comme_pointeur();
    }

    auto types_pointeurs_ = types_pointeurs.verrou_ecriture();

    if (type->type_pointeur) {
        return type->type_pointeur;
    }

    auto resultat = types_pointeurs_->ajoute_element(type);

    if (insere_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(resultat, type);
    }

    if (ajoute_operateurs) {
        operateurs_->ajoute_operateurs_basiques_pointeur(*this, resultat);
    }

    type->type_pointeur = resultat;

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
    auto membres = kuri::tableau<TypeCompose::Membre, int>();
    membres.ajoute({nullptr, type_pointeur_pour(type_pointe), ID::pointeur, 0});
    membres.ajoute({nullptr, types_communs[static_cast<long>(TypeBase::Z64)], ID::taille, 0});

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

    auto membres = kuri::tableau<TypeCompose::Membre, int>();
    membres.ajoute({nullptr, type_pointeur_pour(type_pointe), ID::pointeur, 0});
    membres.ajoute({nullptr, types_communs[static_cast<long>(TypeBase::Z64)], ID::taille, 8});
    membres.ajoute({nullptr, types_communs[static_cast<long>(TypeBase::Z64)], ID::capacite, 16});

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

    auto membres = kuri::tableau<TypeCompose::Membre, int>();
    membres.ajoute({nullptr, type_pointeur_pour(type_pointe), ID::pointeur, 0});
    membres.ajoute({nullptr, types_communs[static_cast<long>(TypeBase::Z64)], ID::taille, 8});
    membres.ajoute({nullptr, types_communs[static_cast<long>(TypeBase::Z64)], ID::capacite, 16});

    auto type = types_variadiques_->ajoute_element(type_pointe, std::move(membres));

    if (type_pointe != nullptr) {
        /* crée un tableau dynamique correspond pour que la génération */
        auto tableau_dyn = type_tableau_dynamique(type_pointe);

        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_pointe);
        graphe->connecte_type_type(type, tableau_dyn);

        type->type_tableau_dyn = tableau_dyn;
    }
    else {
        /* Pour les types variadiques externes, nous ne pouvons générer de fonction
         * d'initialisations, donc marque le type comme ayant eu sa fonction générée. */
        type->drapeaux |= (INITIALISATION_TYPE_FUT_CREEE | UNITE_POUR_INITIALISATION_FUT_CREE);
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
        operateurs_->ajoute_operateurs_basiques_fonction(*this, type);
    }

    auto graphe = graphe_.verrou_ecriture();

    POUR (type->types_entrees) {
        graphe->connecte_type_type(type, it);
    }

    // À FAIRE(architecture) : voir commentaire dans TypeFonction::marque_polymorphique()
    if (type_sortie) {
        graphe->connecte_type_type(type, type_sortie);
    }

    return type;
}

TypeTypeDeDonnees *Typeuse::type_type_de_donnees(Type *type_connu)
{
    if (type_connu == nullptr) {
        return type_type_de_donnees_;
    }

    auto types_type_de_donnees_ = types_type_de_donnees.verrou_ecriture();

    if ((type_connu->drapeaux & POSSEDE_TYPE_TYPE_DE_DONNEES) != 0) {
        POUR_TABLEAU_PAGE ((*types_type_de_donnees_)) {
            if (it.type_connu == type_connu) {
                return &it;
            }
        }
    }

    return types_type_de_donnees_->ajoute_element(type_connu);
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

TypeUnion *Typeuse::union_anonyme(const kuri::tablet<TypeCompose::Membre, 6> &membres)
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

    type->est_anonyme = true;
    type->drapeaux |= (TYPE_FUT_VALIDE);

    type->marque_polymorphique();

    if ((type->drapeaux & TYPE_EST_POLYMORPHIQUE) == 0) {
        calcule_taille_type_compose(type, false, 0);
        type->cree_type_structure(*this, type->decalage_index);
    }

    return type;
}

TypeEnum *Typeuse::reserve_type_erreur(NoeudEnum *decl)
{
    auto type = reserve_type_enum(decl);
    type->genre = GenreType::ERREUR;

    return type;
}

TypePolymorphique *Typeuse::cree_polymorphique(IdentifiantCode *ident)
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

TypeOpaque *Typeuse::cree_opaque(NoeudDeclarationTypeOpaque *decl, Type *type_opacifie)
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

TypeTuple *Typeuse::cree_tuple(const kuri::tablet<TypeCompose::Membre, 6> &membres)
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

    type->marque_polymorphique();

    if ((type->drapeaux & TYPE_EST_POLYMORPHIQUE) == 0) {
        calcule_taille_type_compose(type, false, 0);
    }

    type->drapeaux |= (TYPE_FUT_VALIDE);

    return type;
}

void Typeuse::rassemble_statistiques(Statistiques &stats) const
{
#define DONNEES_ENTREE(Type, Tableau)                                                             \
#    Type, Tableau->taille(), Tableau->taille() * (taille_de(Type *) + taille_de(Type))

    auto &stats_types = stats.stats_types;

    stats_types.fusionne_entree({DONNEES_ENTREE(Type, types_simples)});
    stats_types.fusionne_entree({DONNEES_ENTREE(TypePointeur, types_pointeurs)});
    stats_types.fusionne_entree({DONNEES_ENTREE(TypeReference, types_references)});
    stats_types.fusionne_entree({DONNEES_ENTREE(TypeTypeDeDonnees, types_type_de_donnees)});
    stats_types.fusionne_entree({DONNEES_ENTREE(TypePolymorphique, types_polymorphiques)});

    auto memoire_membres_structures = 0l;
    POUR_TABLEAU_PAGE ((*types_structures.verrou_lecture())) {
        memoire_membres_structures += it.membres.taille_memoire();
    }
    stats_types.fusionne_entree(
        {DONNEES_ENTREE(TypeStructure, types_structures) + memoire_membres_structures});

    auto memoire_membres_enums = 0l;
    POUR_TABLEAU_PAGE ((*types_enums.verrou_lecture())) {
        memoire_membres_enums += it.membres.taille_memoire();
    }
    stats_types.fusionne_entree({DONNEES_ENTREE(TypeEnum, types_enums) + memoire_membres_enums});

    auto memoire_membres_unions = 0l;
    POUR_TABLEAU_PAGE ((*types_unions.verrou_lecture())) {
        memoire_membres_unions += it.membres.taille_memoire();
    }
    stats_types.fusionne_entree(
        {DONNEES_ENTREE(TypeUnion, types_unions) + memoire_membres_unions});

    auto memoire_membres_tuples = 0l;
    POUR_TABLEAU_PAGE ((*types_tuples.verrou_lecture())) {
        memoire_membres_tuples += it.membres.taille_memoire();
    }
    stats_types.fusionne_entree(
        {DONNEES_ENTREE(TypeTuple, types_tuples) + memoire_membres_tuples});

    auto memoire_membres_tfixes = 0l;
    POUR_TABLEAU_PAGE ((*types_tableaux_fixes.verrou_lecture())) {
        memoire_membres_tfixes += it.membres.taille_memoire();
    }
    stats_types.fusionne_entree(
        {DONNEES_ENTREE(TypeTableauFixe, types_tableaux_fixes) + memoire_membres_tfixes});

    auto memoire_membres_tdyns = 0l;
    POUR_TABLEAU_PAGE ((*types_tableaux_dynamiques.verrou_lecture())) {
        memoire_membres_tdyns += it.membres.taille_memoire();
    }
    stats_types.fusionne_entree(
        {DONNEES_ENTREE(TypeTableauDynamique, types_tableaux_dynamiques) + memoire_membres_tdyns});

    auto memoire_membres_tvars = 0l;
    POUR_TABLEAU_PAGE ((*types_variadiques.verrou_lecture())) {
        memoire_membres_tvars += it.membres.taille_memoire();
    }
    stats_types.fusionne_entree(
        {DONNEES_ENTREE(TypeVariadique, types_variadiques) + memoire_membres_tvars});

    auto memoire_params_fonctions = 0l;
    POUR_TABLEAU_PAGE ((*types_fonctions.verrou_lecture())) {
        memoire_params_fonctions += it.types_entrees.taille_memoire();
    }
    stats_types.fusionne_entree(
        {DONNEES_ENTREE(TypeFonction, types_fonctions) + memoire_params_fonctions});

    // les types communs sont dans les types simples, ne comptons que la mémoire du tableau
    stats_types.fusionne_entree(
        {"TypeCommun", types_communs.taille(), types_communs.taille_memoire()});

    stats_types.fusionne_entree(
        {"eini",
         1,
         taille_de(TypeCompose) + taille_de(TypeCompose *) + type_eini->membres.taille_memoire()});
    stats_types.fusionne_entree({"chaine",
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

/* ************************************************************************** */

static void chaine_type_structure(Enchaineuse &enchaineuse, const TypeStructure *type_structure)
{
    enchaineuse << type_structure->nom->nom;
    auto decl = type_structure->decl;
    const char *virgule = "(";
    if (decl->est_monomorphisation) {
        POUR ((*decl->bloc_constantes->membres.verrou_lecture())) {
            enchaineuse << virgule;
            enchaineuse << it->ident->nom << ": ";

            if (it->type->est_type_de_donnees()) {
                enchaineuse << chaine_type(it->type->comme_type_de_donnees()->type_connu);
            }
            else {
                enchaineuse << it->comme_declaration_variable()->valeur_expression;
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

static void chaine_type(Enchaineuse &enchaineuse, const Type *type)
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
            chaine_type(enchaineuse, static_cast<TypeReference const *>(type)->type_pointe);
            return;
        }
        case GenreType::POINTEUR:
        {
            auto const type_pointe = type->comme_pointeur()->type_pointe;
            if (type_pointe == nullptr) {
                enchaineuse.ajoute("type_de(nul)");
            }
            else {
                enchaineuse.ajoute("*");
                chaine_type(enchaineuse, type_pointe);
            }
            return;
        }
        case GenreType::UNION:
        {
            auto type_structure = static_cast<TypeStructure const *>(type);

            if (!type_structure->nom || !type_structure->decl) {
                enchaineuse.ajoute("union.anonyme");
                return;
            }

            chaine_type_structure(enchaineuse, type_structure);
            return;
        }
        case GenreType::STRUCTURE:
        {
            auto type_structure = static_cast<TypeStructure const *>(type);

            if (!type_structure->nom || !type_structure->decl) {
                enchaineuse.ajoute("struct.anonyme");
                return;
            }

            chaine_type_structure(enchaineuse, type_structure);
            return;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            enchaineuse.ajoute("[]");
            chaine_type(enchaineuse, static_cast<TypeTableauDynamique const *>(type)->type_pointe);
            return;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tabl = static_cast<TypeTableauFixe const *>(type);

            enchaineuse << "[" << type_tabl->taille << "]";
            chaine_type(enchaineuse, type_tabl->type_pointe);
            return;
        }
        case GenreType::VARIADIQUE:
        {
            enchaineuse << "...";
            chaine_type(enchaineuse, static_cast<TypeVariadique const *>(type)->type_pointe);
            return;
        }
        case GenreType::FONCTION:
        {
            auto type_fonc = static_cast<TypeFonction const *>(type);

            enchaineuse << "fonc";

            auto virgule = '(';

            POUR (type_fonc->types_entrees) {
                enchaineuse << virgule;
                chaine_type(enchaineuse, it);
                virgule = ',';
            }

            if (type_fonc->types_entrees.est_vide()) {
                enchaineuse << virgule;
            }
            enchaineuse << ')';

            enchaineuse << '(';
            chaine_type(enchaineuse, type_fonc->type_sortie);
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
            auto type_de_donnees = type->comme_type_de_donnees();
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
            enchaineuse << "(";
            chaine_type(enchaineuse, type_opaque->type_opacifie);
            enchaineuse << ")";
            return;
        }
        case GenreType::TUPLE:
        {
            auto type_tuple = static_cast<TypeTuple const *>(type);
            enchaineuse << "tuple ";

            auto virgule = '(';
            POUR (type_tuple->membres) {
                enchaineuse << virgule;
                chaine_type(enchaineuse, it.type);
                virgule = ',';
            }

            enchaineuse << ')';
            return;
        }
    }
}

kuri::chaine chaine_type(const Type *type)
{
    Enchaineuse enchaineuse;
    chaine_type(enchaineuse, type);
    return enchaineuse.chaine();
}

Type *type_dereference_pour(Type *type)
{
    if (type->genre == GenreType::POINTEUR) {
        return type->comme_pointeur()->type_pointe;
    }

    if (type->genre == GenreType::REFERENCE) {
        return type->comme_reference()->type_pointe;
    }

    if (type->genre == GenreType::TABLEAU_FIXE) {
        return type->comme_tableau_fixe()->type_pointe;
    }

    if (type->genre == GenreType::TABLEAU_DYNAMIQUE) {
        return type->comme_tableau_dynamique()->type_pointe;
    }

    if (type->genre == GenreType::VARIADIQUE) {
        return type->comme_variadique()->type_pointe;
    }

    if (type->est_opaque()) {
        return type_dereference_pour(type->comme_opaque()->type_opacifie);
    }

    return nullptr;
}

void rassemble_noms_type_polymorphique(Type *type, kuri::tableau<kuri::chaine_statique> &noms)
{
    if (type->genre == GenreType::FONCTION) {
        auto type_fonction = type->comme_fonction();

        POUR (type_fonction->types_entrees) {
            if (it->drapeaux & TYPE_EST_POLYMORPHIQUE) {
                rassemble_noms_type_polymorphique(it, noms);
            }
        }

        if (type_fonction->type_sortie->drapeaux & TYPE_EST_POLYMORPHIQUE) {
            rassemble_noms_type_polymorphique(type_fonction->type_sortie, noms);
        }

        return;
    }

    if (type->genre == GenreType::TUPLE) {
        auto type_tuple = type->comme_tuple();

        POUR (type_tuple->membres) {
            if (it.drapeaux & TYPE_EST_POLYMORPHIQUE) {
                rassemble_noms_type_polymorphique(it.type, noms);
            }
        }
    }

    while (type->genre != GenreType::POLYMORPHIQUE) {
        type = type_dereference_pour(type);
    }

    noms.ajoute(type->comme_polymorphique()->ident->nom);
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

void TypeUnion::cree_type_structure(Typeuse &typeuse, unsigned alignement_membre_actif)
{
    assert(!est_nonsure);

    type_structure = typeuse.reserve_type_structure(nullptr);

    if (type_le_plus_grand) {
        auto membres_ = kuri::tableau<TypeCompose::Membre, int>(2);
        membres_[0] = {nullptr, type_le_plus_grand, ID::valeur, 0};
        membres_[1] = {nullptr, typeuse[TypeBase::Z32], ID::membre_actif, alignement_membre_actif};
        type_structure->membres = std::move(membres_);
    }
    else {
        auto membres_ = kuri::tableau<TypeCompose::Membre, int>(1);
        membres_[0] = {nullptr, typeuse[TypeBase::Z32], ID::membre_actif, alignement_membre_actif};
        type_structure->membres = std::move(membres_);
    }

    type_structure->taille_octet = this->taille_octet;
    type_structure->alignement = this->alignement;
    type_structure->nom = this->nom;
    type_structure->est_anonyme = this->est_anonyme;
    // Il nous faut la déclaration originelle afin de pouvoir utiliser un typedef différent
    // dans la coulisse pour chaque monomorphisation.
    type_structure->decl = this->decl;
    type_structure->union_originelle = this;
    /* L'initialisation est créée avec le type de l'union et non celui de la structure. */
    type_structure->drapeaux |= (TYPE_FUT_VALIDE | INITIALISATION_TYPE_FUT_CREEE |
                                 UNITE_POUR_INITIALISATION_FUT_CREE);

    typeuse.graphe_->connecte_type_type(this, type_structure);
}

/* Pour la génération de RI, les types doivent être normalisés afin de se rapprocher de la manière
 * dont ceux-ci sont « représenter » dans la machine. Ceci se fait en :
 * - remplaçant les références par des pointeurs
 * - convertissant les unions en leurs « types machines » : une structure pour les unions sûres, le
 * type le plus grand pour les sûres
 */
Type *normalise_type(Typeuse &typeuse, Type *type)
{
    if (type == nullptr) {
        return type;
    }

    auto resultat = type;

    if (type->genre == GenreType::POINTEUR) {
        auto type_pointeur = type->comme_pointeur();
        auto type_normalise = normalise_type(typeuse, type_pointeur->type_pointe);

        if (type_normalise != type_pointeur) {
            resultat = typeuse.type_pointeur_pour(type_pointeur->type_pointe, false);
        }
    }
    else if (type->genre == GenreType::UNION) {
        auto type_union = type->comme_union();

        if (type_union->est_nonsure) {
            resultat = type_union->type_le_plus_grand;
        }
        else {
            resultat = type_union->type_structure;
        }
    }
    else if (type->genre == GenreType::TABLEAU_FIXE) {
        auto type_tableau_fixe = type->comme_tableau_fixe();
        resultat = normalise_type(typeuse, type_tableau_fixe->type_pointe);
        resultat = typeuse.type_tableau_fixe(type_tableau_fixe->type_pointe,
                                             type_tableau_fixe->taille);
    }
    else if (type->genre == GenreType::TABLEAU_DYNAMIQUE) {
        auto type_tableau_dyn = type->comme_tableau_dynamique();
        auto type_normalise = normalise_type(typeuse, type_tableau_dyn->type_pointe);

        if (type_normalise != type_tableau_dyn->type_pointe) {
            resultat = typeuse.type_tableau_dynamique(type_tableau_dyn->type_pointe);
        }
    }
    else if (type->genre == GenreType::VARIADIQUE) {
        auto type_variadique = type->comme_variadique();
        auto type_normalise = normalise_type(typeuse, type_variadique->type_pointe);

        if (type_normalise != type_variadique) {
            resultat = typeuse.type_variadique(type_variadique->type_pointe);
        }
    }
    else if (type->genre == GenreType::REFERENCE) {
        auto type_reference = type->comme_reference();
        auto type_normalise = normalise_type(typeuse, type_reference->type_pointe);
        resultat = typeuse.type_pointeur_pour(type_normalise, false);
    }
    else if (type->genre == GenreType::FONCTION) {
        auto type_fonction = type->comme_fonction();

        auto types_entrees = kuri::tablet<Type *, 6>();
        types_entrees.reserve(type_fonction->types_entrees.taille());

        POUR (type_fonction->types_entrees) {
            types_entrees.ajoute(normalise_type(typeuse, it));
        }

        auto type_sortie = normalise_type(typeuse, type_fonction->type_sortie);
        resultat = typeuse.type_fonction(types_entrees, type_sortie, false);
    }
    else if (type->genre == GenreType::TUPLE) {
        auto type_tuple = type->comme_tuple();

        auto types_membres = kuri::tablet<TypeCompose::Membre, 6>();

        POUR (type_tuple->membres) {
            types_membres.ajoute({nullptr, normalise_type(typeuse, it.type)});
        }

        resultat = typeuse.cree_tuple(types_membres);
    }

    // À FAIRE: comprends pourquoi ceci peut-être nul...
    if (resultat) {
        resultat->drapeaux |= TYPE_EST_NORMALISE;
    }

    return resultat;
}

static inline uint marge_pour_alignement(const uint alignement, const uint taille_octet)
{
    return (alignement - (taille_octet % alignement)) % alignement;
}

template <bool COMPACTE>
void calcule_taille_structure(TypeCompose *type, uint32_t alignement_desire)
{
    auto decalage = 0u;
    auto alignement_max = 0u;

    POUR (type->membres) {
        if (it.drapeaux & TypeStructure::Membre::EST_CONSTANT) {
            continue;
        }

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
            decalage += marge_pour_alignement(alignement_type, decalage);
        }

        it.decalage = decalage;
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
    if (type->genre == GenreType::UNION) {
        auto type_union = type->comme_union();

        auto max_alignement = 0u;
        auto taille_union = 0u;
        auto type_le_plus_grand = Type::nul();

        POUR (type->membres) {
            if (it.drapeaux & TypeStructure::Membre::EST_CONSTANT) {
                continue;
            }

            auto type_membre = it.type;
            auto taille = type_membre->taille_octet;

            /* Ignore les membres qui n'ont pas de type. */
            if (type_membre->est_rien()) {
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
    else if (type->genre == GenreType::STRUCTURE || type->est_tuple()) {
        if (compacte) {
            calcule_taille_structure<true>(type, alignement_desire);
        }
        else {
            calcule_taille_structure<false>(type, alignement_desire);
        }
    }
}

static kuri::tablet<kuri::chaine_statique, 6> noms_hierarchie(NoeudBloc *bloc)
{
    kuri::tablet<kuri::chaine_statique, 6> noms;

    while (bloc) {
        if (bloc->ident) {
            noms.ajoute(bloc->ident->nom);
        }

        bloc = bloc->bloc_parent;
    }

    return noms;
}

static kuri::chaine nom_portable(NoeudBloc *bloc, kuri::chaine_statique nom)
{
    auto const noms = noms_hierarchie(bloc);

    Enchaineuse enchaineuse;
    for (auto i = noms.taille() - 1; i >= 0; --i) {
        enchaineuse.ajoute(noms[i]);
    }
    enchaineuse.ajoute(nom);

    return enchaineuse.chaine();
}

static kuri::chaine nom_hierarchique(NoeudBloc *bloc, IdentifiantCode const *ident)
{
    auto const noms = noms_hierarchie(bloc);

    Enchaineuse enchaineuse;
    /* -2 pour éviter le nom du module. */
    for (auto i = noms.taille() - 2; i >= 0; --i) {
        enchaineuse.ajoute(noms[i]);
        enchaineuse.ajoute(".");
    }
    enchaineuse.ajoute(ident ? ident->nom : "anonyme");

    return enchaineuse.chaine();
}

const kuri::chaine &TypeStructure::nom_portable()
{
    if (nom_portable_ != "") {
        return nom_portable_;
    }

    nom_portable_ = ::nom_portable(decl ? decl->bloc_parent : nullptr, nom->nom);
    return nom_portable_;
}

kuri::chaine_statique TypeStructure::nom_hierarchique()
{
    if (nom_hierarchique_ != "") {
        return nom_hierarchique_;
    }

    nom_hierarchique_ = ::nom_hierarchique(decl ? decl->bloc_parent : nullptr, nom);
    return nom_hierarchique_;
}

const kuri::chaine &TypeUnion::nom_portable()
{
    if (nom_portable_ != "") {
        return nom_portable_;
    }

    nom_portable_ = ::nom_portable(decl ? decl->bloc_parent : nullptr, nom->nom);
    return nom_portable_;
}

kuri::chaine_statique TypeUnion::nom_hierarchique()
{
    if (nom_hierarchique_ != "") {
        return nom_hierarchique_;
    }

    nom_hierarchique_ = ::nom_hierarchique(decl ? decl->bloc_parent : nullptr, nom);
    return nom_hierarchique_;
}

const kuri::chaine &TypeEnum::nom_portable()
{
    if (nom_portable_ != "") {
        return nom_portable_;
    }

    nom_portable_ = ::nom_portable(decl ? decl->bloc_parent : nullptr, nom->nom);
    return nom_portable_;
}

kuri::chaine_statique TypeEnum::nom_hierarchique()
{
    if (nom_hierarchique_ != "") {
        return nom_hierarchique_;
    }

    nom_hierarchique_ = ::nom_hierarchique(decl ? decl->bloc_parent : nullptr, nom);
    return nom_hierarchique_;
}

const kuri::chaine &TypeOpaque::nom_portable()
{
    if (nom_portable_ != "") {
        return nom_portable_;
    }

    nom_portable_ = ::nom_portable(decl->bloc_parent, ident->nom);
    return nom_portable_;
}

kuri::chaine_statique TypeOpaque::nom_hierarchique()
{
    if (nom_hierarchique_ != "") {
        return nom_hierarchique_;
    }

    nom_hierarchique_ = ::nom_hierarchique(decl->bloc_parent, ident);
    return nom_hierarchique_;
}

NoeudDeclaration *decl_pour_type(const Type *type)
{
    if (type->est_structure()) {
        return type->comme_structure()->decl;
    }

    if (type->est_enum()) {
        return type->comme_enum()->decl;
    }

    if (type->est_erreur()) {
        return type->comme_erreur()->decl;
    }

    if (type->est_union()) {
        return type->comme_union()->decl;
    }

    return nullptr;
}

bool est_type_polymorphique(Type *type)
{
    if (type->est_polymorphique()) {
        return true;
    }

    if (type->drapeaux & TYPE_EST_POLYMORPHIQUE) {
        return true;
    }

    auto decl = decl_pour_type(type);
    if (decl && decl->est_structure() && decl->comme_structure()->est_polymorphe) {
        return true;
    }

    return false;
}

std::optional<Attente> attente_sur_type_si_drapeau_manquant(
    kuri::ensemblon<Type *, 16> const &types_utilises, int drapeau)
{
    auto visites = kuri::ensemblon<Type *, 16>();
    auto pile = kuri::pile<Type *>();

    pour_chaque_element(types_utilises, [&pile](auto &type) {
        pile.empile(type);
        return kuri::DecisionIteration::Continue;
    });

    while (!pile.est_vide()) {
        auto type_courant = pile.depile();

        /* Les types variadiques ou pointeur nul peuvent avoir des types déréférencés nuls. */
        if (!type_courant) {
            continue;
        }

        if (visites.possede(type_courant)) {
            continue;
        }

        visites.insere(type_courant);

        if ((type_courant->drapeaux & drapeau) == 0) {
            return Attente::sur_type(type_courant);
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
                auto type_fonction = type_courant->comme_fonction();
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
                pile.empile(type_courant->comme_reference()->type_pointe);
                break;
            }
            case GenreType::POINTEUR:
            {
                pile.empile(type_courant->comme_pointeur()->type_pointe);
                break;
            }
            case GenreType::VARIADIQUE:
            {
                pile.empile(type_courant->comme_variadique()->type_pointe);
                break;
            }
            case GenreType::TABLEAU_DYNAMIQUE:
            {
                pile.empile(type_courant->comme_tableau_dynamique()->type_pointe);
                break;
            }
            case GenreType::TABLEAU_FIXE:
            {
                pile.empile(type_courant->comme_tableau_fixe()->type_pointe);
                break;
            }
            case GenreType::OPAQUE:
            {
                pile.empile(type_courant->comme_opaque()->type_opacifie);
                break;
            }
        }
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

long Trie::StockageEnfants::taille() const
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
                table.insere(it->type, it);
            }
        }

        table.insere(noeud->type, noeud);
        return;
    }

    enfants.ajoute(noeud);
}

Trie::Noeud *Trie::Noeud::trouve_noeud_pour_type(const Type *type)
{
    return enfants.trouve_noeud_pour_type(type);
}

Trie::Noeud *Trie::Noeud::trouve_noeud_sortie_pour_type(const Type *type)
{
    return enfants_sortie.trouve_noeud_pour_type(type);
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
    return const_cast<TypeFonction *>(enfant_suivant->enfants.enfants[0]->type->comme_fonction());
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
