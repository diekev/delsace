/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "typage.hh"

#include <algorithm>

#include "arbre_syntaxique/allocatrice.hh"
#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"
#include "parsage/outils_lexemes.hh"

#include "compilatrice.hh"
#include "graphe_dependance.hh"

#include "statistiques/statistiques.hh"

#include "structures/pile.hh"

#include "utilitaires/divers.hh"
#include "utilitaires/log.hh"
#include "utilitaires/macros.hh"

/* ------------------------------------------------------------------------- */
/** \name Création de types de bases.
 * \{ */

static TypeCompose *crée_type_eini()
{
    auto type = mémoire::loge<TypeCompose>("TypeCompose");
    type->ident = ID::eini;
    type->genre = GenreNoeud::EINI;
    type->taille_octet = 16;
    type->alignement = 8;
    return type;
}

static TypeCompose *crée_type_chaine()
{
    auto type = mémoire::loge<TypeCompose>("TypeCompose");
    type->ident = ID::chaine;
    type->genre = GenreNoeud::CHAINE;
    type->taille_octet = 16;
    type->alignement = 8;
    return type;
}

static Type *crée_type_entier(IdentifiantCode *ident, unsigned taille_octet, bool est_naturel)
{
    auto type = mémoire::loge<Type>("Type");
    type->ident = ident;
    type->genre = est_naturel ? GenreNoeud::ENTIER_NATUREL : GenreNoeud::ENTIER_RELATIF;
    type->taille_octet = taille_octet;
    type->alignement = taille_octet;
    type->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    return type;
}

static Type *crée_type_reel(IdentifiantCode *ident, unsigned taille_octet)
{
    auto type = mémoire::loge<Type>("Type");
    type->ident = ident;
    type->genre = GenreNoeud::RÉEL;
    type->taille_octet = taille_octet;
    type->alignement = taille_octet;
    type->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    return type;
}

static Type *crée_type_rien()
{
    auto type = mémoire::loge<Type>("Type");
    type->ident = ID::rien;
    type->genre = GenreNoeud::RIEN;
    type->taille_octet = 0;
    type->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    return type;
}

static Type *crée_type_bool()
{
    auto type = mémoire::loge<Type>("Type");
    type->ident = ID::bool_;
    type->genre = GenreNoeud::BOOL;
    type->taille_octet = 1;
    type->alignement = 1;
    type->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    return type;
}

static Type *crée_type_adresse_fonction()
{
    auto type = mémoire::loge<Type>("Type");
    type->ident = ID::adresse_fonction;
    type->genre = GenreNoeud::TYPE_ADRESSE_FONCTION;
    type->taille_octet = 8;
    type->alignement = 8;
    type->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    return type;
}

/** \} */

static void initialise_type_pointeur(TypePointeur *résultat, Type *type_pointe_)
{
    résultat->type_pointé = type_pointe_;
    résultat->taille_octet = 8;
    résultat->alignement = 8;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    if (type_pointe_) {
        if (type_pointe_->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
        }

        type_pointe_->drapeaux_type |= DrapeauxTypes::POSSEDE_TYPE_POINTEUR;
    }
}

static void initialise_type_référence(TypeReference *résultat, Type *type_pointe_)
{
    assert(type_pointe_);

    résultat->type_pointé = type_pointe_;
    résultat->taille_octet = 8;
    résultat->alignement = 8;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    if (type_pointe_->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }

    type_pointe_->drapeaux_type |= DrapeauxTypes::POSSEDE_TYPE_REFERENCE;
}

static void initialise_type_fonction(TypeFonction *résultat,
                                     kuri::tablet<Type *, 6> const &entrees,
                                     Type *sortie)
{
    résultat->types_entrées.réserve(static_cast<int>(entrees.taille()));
    POUR (entrees) {
        résultat->types_entrées.ajoute(it);
    }

    résultat->type_sortie = sortie;
    résultat->taille_octet = 8;
    résultat->alignement = 8;
    marque_polymorphique(résultat);
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
}

static void initialise_type_tableau_fixe(TypeTableauFixe *résultat,
                                         Type *type_pointe_,
                                         int taille_,
                                         kuri::tableau<RubriqueTypeComposé, int> &&rubriques_)
{
    assert(type_pointe_);
    assert(taille_ > 0);

    résultat->rubriques = std::move(rubriques_);
    résultat->nombre_de_rubriques_réelles = 0;
    résultat->type_pointé = type_pointe_;
    résultat->taille = taille_;
    résultat->alignement = type_pointe_->alignement;
    résultat->taille_octet = type_pointe_->taille_octet * static_cast<unsigned>(taille_);
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    if (type_pointe_->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }

    type_pointe_->drapeaux_type |= DrapeauxTypes::POSSEDE_TYPE_TABLEAU_FIXE;
}

static void initialise_type_tableau_dynamique(TypeTableauDynamique *résultat,
                                              Type *type_pointe_,
                                              kuri::tableau<RubriqueTypeComposé, int> &&rubriques_)
{
    assert(type_pointe_);

    résultat->rubriques = std::move(rubriques_);
    résultat->nombre_de_rubriques_réelles = résultat->rubriques.taille();
    résultat->type_pointé = type_pointe_;
    résultat->taille_octet = 24;
    résultat->alignement = 8;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    if (type_pointe_->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }

    type_pointe_->drapeaux_type |= DrapeauxTypes::POSSEDE_TYPE_TABLEAU_DYNAMIQUE;
}

static void initialise_type_tranche(NoeudDéclarationTypeTranche *résultat,
                                    Type *type_élément,
                                    kuri::tableau<RubriqueTypeComposé, int> &&rubriques_)
{
    assert(type_élément);

    résultat->rubriques = std::move(rubriques_);
    résultat->nombre_de_rubriques_réelles = résultat->rubriques.taille();
    résultat->type_élément = type_élément;
    résultat->taille_octet = 16;
    résultat->alignement = 8;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    if (type_élément->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }

    type_élément->drapeaux_type |= DrapeauxTypes::POSSEDE_TYPE_TRANCHE;
}

static void initialise_type_variadique(TypeVariadique *résultat,
                                       Type *type_pointe_,
                                       kuri::tableau<RubriqueTypeComposé, int> &&rubriques_)
{
    résultat->type_pointé = type_pointe_;

    if (type_pointe_ && type_pointe_->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }

    résultat->rubriques = std::move(rubriques_);
    résultat->nombre_de_rubriques_réelles = résultat->rubriques.taille();
    résultat->taille_octet = 16;
    résultat->alignement = 8;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
}

static void initialise_type_type_de_données(Typeuse &typeuse,
                                            TypeTypeDeDonnees *résultat,
                                            Type *type_connu_)
{
    résultat->ident = ID::type_de_données;
    résultat->type_code_machine = typeuse.type_z64;
    // un type 'type' est un genre de pointeur déguisé, donc donnons lui les mêmes caractéristiques
    résultat->taille_octet = 8;
    résultat->alignement = 8;
    résultat->type_connu = type_connu_;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    if (type_connu_) {
        type_connu_->drapeaux_type |= DrapeauxTypes::POSSEDE_TYPE_TYPE_DE_DONNEES;
    }
}

static void initialise_type_polymorphique(TypePolymorphique *résultat, IdentifiantCode *ident_)
{
    résultat->ident = ident_;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
}

static void initialise_type_opaque(TypeOpaque *résultat, Type *opacifie)
{
    résultat->type_opacifié = opacifie;
    résultat->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    résultat->taille_octet = opacifie->taille_octet;
    résultat->alignement = opacifie->alignement;

    if (opacifie->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        résultat->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }
}

/* ************************************************************************** */

static Type *crée_type_pour_lexeme(GenreLexème lexeme)
{
    switch (lexeme) {
        case GenreLexème::BOOL:
        {
            return crée_type_bool();
        }
        case GenreLexème::N8:
        {
            return crée_type_entier(ID::n8, 1, true);
        }
        case GenreLexème::Z8:
        {
            return crée_type_entier(ID::z8, 1, false);
        }
        case GenreLexème::N16:
        {
            return crée_type_entier(ID::n16, 2, true);
        }
        case GenreLexème::Z16:
        {
            return crée_type_entier(ID::z16, 2, false);
        }
        case GenreLexème::N32:
        {
            return crée_type_entier(ID::n32, 4, true);
        }
        case GenreLexème::Z32:
        {
            return crée_type_entier(ID::z32, 4, false);
        }
        case GenreLexème::N64:
        {
            return crée_type_entier(ID::n64, 8, true);
        }
        case GenreLexème::Z64:
        {
            return crée_type_entier(ID::z64, 8, false);
        }
        case GenreLexème::R16:
        {
            return crée_type_reel(ID::r16, 2);
        }
        case GenreLexème::R32:
        {
            return crée_type_reel(ID::r32, 4);
        }
        case GenreLexème::R64:
        {
            return crée_type_reel(ID::r64, 8);
        }
        case GenreLexème::RIEN:
        {
            return crée_type_rien();
        }
        case GenreLexème::ADRESSE_FONCTION:
        {
            return crée_type_adresse_fonction();
        }
        default:
        {
            return nullptr;
        }
    }
}

#define VERROUILLE(x) std::unique_lock<std::mutex> verrou(mutex_##x)

Typeuse::Typeuse(kuri::Synchrone<GrapheDépendance> &g) : graphe_(g)
{
    alloc = mémoire::loge<AllocatriceNoeud>("AllocatriceNoeud");

    /* initialise les types communs */
    type_n8 = crée_type_pour_lexeme(GenreLexème::N8);
    types_simples->ajoute(type_n8);
    type_n16 = crée_type_pour_lexeme(GenreLexème::N16);
    types_simples->ajoute(type_n16);
    type_n32 = crée_type_pour_lexeme(GenreLexème::N32);
    types_simples->ajoute(type_n32);
    type_n64 = crée_type_pour_lexeme(GenreLexème::N64);
    types_simples->ajoute(type_n64);
    type_z8 = crée_type_pour_lexeme(GenreLexème::Z8);
    types_simples->ajoute(type_z8);
    type_z16 = crée_type_pour_lexeme(GenreLexème::Z16);
    types_simples->ajoute(type_z16);
    type_z32 = crée_type_pour_lexeme(GenreLexème::Z32);
    types_simples->ajoute(type_z32);
    type_z64 = crée_type_pour_lexeme(GenreLexème::Z64);
    types_simples->ajoute(type_z64);
    type_r16 = crée_type_pour_lexeme(GenreLexème::R16);
    types_simples->ajoute(type_r16);
    type_r32 = crée_type_pour_lexeme(GenreLexème::R32);
    types_simples->ajoute(type_r32);
    type_r64 = crée_type_pour_lexeme(GenreLexème::R64);
    types_simples->ajoute(type_r64);
    type_rien = crée_type_pour_lexeme(GenreLexème::RIEN);
    types_simples->ajoute(type_rien);
    type_bool = crée_type_pour_lexeme(GenreLexème::BOOL);
    types_simples->ajoute(type_bool);

    type_entier_constant = alloc->m_noeuds_type_entier_constant.ajoute_élément();
    type_entier_constant->type_opacifié = type_z32;
    type_entier_constant->ident = ID::entier_constant;
    type_entier_constant->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    type_octet = alloc->m_noeuds_type_octet.ajoute_élément();
    type_octet->type_opacifié = type_n8;
    type_octet->ident = ID::octet;
    type_octet->taille_octet = 1;
    type_octet->alignement = 1;
    type_octet->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    type_dff_adr = crée_opaque_défaut(type_n64, ID::dff_adr);
    type_adr_plt_nat = crée_opaque_défaut(type_n64, ID::adr_plt_nat);
    type_adr_plt_rel = crée_opaque_défaut(type_z64, ID::adr_plt_rel);
    type_taille_mnat = crée_opaque_défaut(type_n64, ID::taille_mnat);
    type_taille_mrel = crée_opaque_défaut(type_z64, ID::taille_mrel);
    type_nbr_nat = crée_opaque_défaut(type_n32, ID::nbr_nat);
    type_nbr_rel = crée_opaque_défaut(type_z32, ID::nbr_rel);
    type_nbf_flt = crée_opaque_défaut(type_r32, ID::nbf_flt);

    type_adresse_fonction = crée_type_pour_lexeme(GenreLexème::ADRESSE_FONCTION);
    types_simples->ajoute(type_adresse_fonction);

    type_tranche_octet = crée_type_tranche(type_octet, true);

    type_ref_n8 = type_reference_pour(type_n8);
    type_ref_n64 = type_reference_pour(type_n64);
    type_ref_z8 = type_reference_pour(type_z8);

    type_ptr_n8 = type_pointeur_pour(type_n8);
    type_ptr_z8 = type_pointeur_pour(type_z8);
    type_ptr_rien = type_pointeur_pour(type_rien);
    type_ptr_octet = type_pointeur_pour(type_octet);

    type_tabl_n8 = type_tableau_dynamique(type_n8);

    type_rien->drapeaux_type |= (DrapeauxTypes::TYPE_NE_REQUIERS_PAS_D_INITIALISATION |
                                 DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE);

    type_eini = crée_type_eini();
    type_chaine = crée_type_chaine();

    type_type_de_donnees_ = alloc->m_noeuds_type_type_de_données.ajoute_élément();
    initialise_type_type_de_données(*this, type_type_de_donnees_, nullptr);

    // nous devons créer le pointeur nul avant les autres types, car nous en avons besoin pour
    // définir les opérateurs pour les pointeurs
    type_ptr_nul = alloc->m_noeuds_type_pointeur.ajoute_élément();
    initialise_type_pointeur(type_ptr_nul, nullptr);

    auto rubriques_eini = kuri::tableau<RubriqueTypeComposé, int>();
    rubriques_eini.ajoute({nullptr, type_ptr_rien, ID::pointeur, 0});
    /* À FAIRE : type_info_type_ n'est pas encore parsé. */
    rubriques_eini.ajoute({nullptr, type_pointeur_pour(type_info_type_), ID::info, 8});
    type_eini->rubriques = std::move(rubriques_eini);
    type_eini->nombre_de_rubriques_réelles = type_eini->rubriques.taille();
    type_eini->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    auto rubriques_chaine = kuri::tableau<RubriqueTypeComposé, int>();
    rubriques_chaine.ajoute({nullptr, type_ptr_z8, ID::pointeur, 0});
    rubriques_chaine.ajoute({nullptr, type_z64, ID::taille, 8});
    type_chaine->rubriques = std::move(rubriques_chaine);
    type_chaine->nombre_de_rubriques_réelles = type_chaine->rubriques.taille();
    type_chaine->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
}

Typeuse::~Typeuse()
{
    for (auto ptr : *types_simples.verrou_ecriture()) {
        mémoire::deloge("Type", ptr);
    }

    mémoire::deloge("TypeCompose", type_chaine);
    mémoire::deloge("TypeCompose", type_eini);
    mémoire::deloge("AllocatriceNoeud", alloc);
}

void Typeuse::crée_tâches_précompilation(Compilatrice &compilatrice)
{
    auto gestionnaire = compilatrice.gestionnaire_code.verrou_ecriture();
    auto espace = compilatrice.espace_de_travail_défaut;

    /* Crée les fonctions d'initialisations de type qui seront partagées avec d'autres types.
     * Les fonctions pour les entiers sont partagées avec les énums, celle de *rien, avec les
     * autres pointeurs et les fonctions. */
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_n8);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_n16);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_n32);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_n64);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_z8);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_z16);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_z32);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_z64);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_ptr_rien);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_adresse_fonction);
    gestionnaire->requiers_initialisation_type(espace, compilatrice.typeuse.type_tranche_octet);
}

Type *Typeuse::type_pour_lexeme(GenreLexème lexeme)
{
    switch (lexeme) {
        case GenreLexème::BOOL:
        {
            return type_bool;
        }
        case GenreLexème::OCTET:
        {
            return type_octet;
        }
        case GenreLexème::N8:
        {
            return type_n8;
        }
        case GenreLexème::Z8:
        {
            return type_z8;
        }
        case GenreLexème::N16:
        {
            return type_n16;
        }
        case GenreLexème::Z16:
        {
            return type_z16;
        }
        case GenreLexème::N32:
        {
            return type_n32;
        }
        case GenreLexème::Z32:
        {
            return type_z32;
        }
        case GenreLexème::N64:
        {
            return type_n64;
        }
        case GenreLexème::Z64:
        {
            return type_z64;
        }
        case GenreLexème::R16:
        {
            return type_r16;
        }
        case GenreLexème::R32:
        {
            return type_r32;
        }
        case GenreLexème::R64:
        {
            return type_r64;
        }
        case GenreLexème::CHAINE:
        {
            return type_chaine;
        }
        case GenreLexème::EINI:
        {
            return type_eini;
        }
        case GenreLexème::RIEN:
        {
            return type_rien;
        }
        case GenreLexème::TYPE_DE_DONNÉES:
        {
            return type_type_de_donnees_;
        }
        case GenreLexème::ADRESSE_FONCTION:
        {
            return type_adresse_fonction;
        }
        default:
        {
            return nullptr;
        }
    }
}

TypePointeur *Typeuse::type_pointeur_pour(Type *type, bool insere_dans_graphe)
{
    if (!type) {
        return type_ptr_nul;
    }

    VERROUILLE(types_pointeurs);

    if (type->type_pointeur) {
        return type->type_pointeur;
    }

    auto résultat = alloc->m_noeuds_type_pointeur.ajoute_élément();
    initialise_type_pointeur(résultat, type);

    if (insere_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(résultat, type);
    }

    type->type_pointeur = résultat;

    /* Tous les pointeurs sont des adresses, il est donc inutile de créer des fonctions spécifiques
     * pour chacun d'entre eux.
     * Lors de la création de la typeuse, les fonction sauvegardées sont nulles. */
    if (init_type_pointeur) {
        assigne_fonction_init(résultat, init_type_pointeur);
    }

    return résultat;
}

TypeReference *Typeuse::type_reference_pour(Type *type)
{
    VERROUILLE(types_references);

    if (type->possède_drapeau(DrapeauxTypes::POSSEDE_TYPE_REFERENCE)) {
        POUR_TABLEAU_PAGE (alloc->m_noeuds_type_référence) {
            if (it.type_pointé == type) {
                return &it;
            }
        }
    }

    auto résultat = alloc->m_noeuds_type_référence.ajoute_élément();
    initialise_type_référence(résultat, type);

    auto graphe = graphe_.verrou_ecriture();
    graphe->connecte_type_type(résultat, type);

    return résultat;
}

TypeTableauFixe *Typeuse::type_tableau_fixe(Type *type_pointe, int taille, bool insere_dans_graphe)
{
    assert(taille);
    VERROUILLE(types_tableaux_fixes);

    if (type_pointe->possède_drapeau(DrapeauxTypes::POSSEDE_TYPE_TABLEAU_FIXE)) {
        POUR_TABLEAU_PAGE (alloc->m_noeuds_type_tableau_fixe) {
            if (it.type_pointé == type_pointe && it.taille == taille) {
                return &it;
            }
        }
    }

    // les décalages sont à zéros car ceci n'est pas vraiment une structure
    auto rubriques = kuri::tableau<RubriqueTypeComposé, int>();
    rubriques.ajoute({nullptr,
                      type_z64,
                      ID::taille,
                      0,
                      uint64_t(taille),
                      nullptr,
                      RubriqueTypeComposé::EST_CONSTANT});

    auto type = alloc->m_noeuds_type_tableau_fixe.ajoute_élément();
    initialise_type_tableau_fixe(type, type_pointe, taille, std::move(rubriques));

    /* À FAIRE: nous pouvons être en train de traverser le graphe lors de la création du type,
     * alors n'essayons pas de créer une dépendance car nous aurions un verrou mort. */
    if (insere_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_pointe);
    }

    return type;
}

TypeTableauFixe *Typeuse::type_tableau_fixe(NoeudExpression const *expression_taille,
                                            Type *type_élément)
{
    VERROUILLE(types_tableaux_fixes);

    if (type_élément->possède_drapeau(DrapeauxTypes::POSSEDE_TYPE_TABLEAU_FIXE)) {
        POUR_TABLEAU_PAGE (alloc->m_noeuds_type_tableau_fixe) {
            if (it.expression_taille == expression_taille && it.type_pointé == type_élément) {
                return &it;
            }
        }
    }

    auto type = alloc->m_noeuds_type_tableau_fixe.ajoute_élément();
    type->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    type->expression_taille = const_cast<NoeudExpression *>(expression_taille);
    type->type_pointé = type_élément;

    return type;
}

TypeTableauDynamique *Typeuse::type_tableau_dynamique(Type *type_pointe, bool insere_dans_graphe)
{
    VERROUILLE(types_tableaux_dynamiques);

    if (type_pointe->possède_drapeau(DrapeauxTypes::POSSEDE_TYPE_TABLEAU_DYNAMIQUE)) {
        POUR_TABLEAU_PAGE (alloc->m_noeuds_type_tableau_dynamique) {
            if (it.type_pointé == type_pointe) {
                return &it;
            }
        }
    }

    auto rubriques = kuri::tableau<RubriqueTypeComposé, int>();
    rubriques.ajoute({nullptr, type_pointeur_pour(type_pointe), ID::pointeur, 0});
    rubriques.ajoute({nullptr, type_z64, ID::taille, 8});
    rubriques.ajoute({nullptr, type_z64, ID::capacite, 16});

    auto type = alloc->m_noeuds_type_tableau_dynamique.ajoute_élément();
    initialise_type_tableau_dynamique(type, type_pointe, std::move(rubriques));

    /* À FAIRE: nous pouvons être en train de traverser le graphe lors de la création du type,
     * alors n'essayons pas de créer une dépendance car nous aurions un verrou mort. */
    if (insere_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_pointe);
    }

    return type;
}

NoeudDéclarationTypeTranche *Typeuse::crée_type_tranche(Type *type_élément,
                                                        bool insère_dans_graphe)
{
    VERROUILLE(types_tranches);

    if (type_élément->possède_drapeau(DrapeauxTypes::POSSEDE_TYPE_TRANCHE)) {
        POUR_TABLEAU_PAGE (alloc->m_noeuds_type_tranche) {
            if (it.type_élément == type_élément) {
                return &it;
            }
        }
    }

    auto rubriques = kuri::tableau<RubriqueTypeComposé, int>();
    rubriques.ajoute({nullptr, type_pointeur_pour(type_élément), ID::pointeur, 0});
    rubriques.ajoute({nullptr, type_z64, ID::taille, 8});

    auto type = alloc->m_noeuds_type_tranche.ajoute_élément();
    initialise_type_tranche(type, type_élément, std::move(rubriques));

    /* À FAIRE: nous pouvons être en train de traverser le graphe lors de la création du type,
     * alors n'essayons pas de créer une dépendance car nous aurions un verrou mort. */
    if (insère_dans_graphe) {
        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_élément);
    }

    return type;
}

TypeVariadique *Typeuse::type_variadique(Type *type_pointe)
{
    VERROUILLE(types_variadiques);

    POUR_TABLEAU_PAGE (alloc->m_noeuds_type_variadique) {
        if (it.type_pointé == type_pointe) {
            return &it;
        }
    }

    auto rubriques = kuri::tableau<RubriqueTypeComposé, int>();
    rubriques.ajoute({nullptr, type_pointeur_pour(type_pointe), ID::pointeur, 0});
    rubriques.ajoute({nullptr, type_z64, ID::taille, 8});

    auto type = alloc->m_noeuds_type_variadique.ajoute_élément();
    initialise_type_variadique(type, type_pointe, std::move(rubriques));

    if (type_pointe != nullptr) {
        /* Crée une tranche correspondante pour la génération de code. */
        auto tranche = crée_type_tranche(type_pointe);

        auto graphe = graphe_.verrou_ecriture();
        graphe->connecte_type_type(type, type_pointe);
        graphe->connecte_type_type(type, tranche);

        type->type_tranche = tranche;
    }
    else {
        /* Pour les types variadiques externes, nous ne pouvons générer de fonction
         * d'initialisations.
         * INITIALISATION_TYPE_FUT_CREEE est à cause de attente_sur_type_si_drapeau_manquant.  */
        type->drapeaux_type |= (DrapeauxTypes::TYPE_NE_REQUIERS_PAS_D_INITIALISATION |
                                DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE);
    }

    return type;
}

TypeFonction *Typeuse::discr_type_fonction(TypeFonction *it,
                                           kuri::tablet<Type *, 6> const &entrees)
{
    if (it->types_entrées.taille() != entrees.taille()) {
        return nullptr;
    }

    for (int i = 0; i < it->types_entrées.taille(); ++i) {
        if (it->types_entrées[i] != entrees[i]) {
            return nullptr;
        }
    }

    return it;
}

TypeFonction *Typeuse::type_fonction(kuri::tablet<Type *, 6> const &entrees, Type *type_sortie)
{
    VERROUILLE(types_fonctions);

    auto candidat = trie.trouve_type_ou_noeud_insertion(entrees, type_sortie);

    if (std::holds_alternative<TypeFonction *>(candidat)) {
        /* Le type est dans le Trie, retournons-le. */
        return std::get<TypeFonction *>(candidat);
    }

    /* Créons un nouveau type. */
    auto type = alloc->m_noeuds_type_fonction.ajoute_élément();
    initialise_type_fonction(type, entrees, type_sortie);

    /* Insère le type dans le Trie. */
    auto noeud = std::get<Trie::Noeud *>(candidat);
    noeud->type = type;

    auto graphe = graphe_.verrou_ecriture();

    POUR (type->types_entrées) {
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

    VERROUILLE(types_type_de_donnees);

    if (type_connu->possède_drapeau(DrapeauxTypes::POSSEDE_TYPE_TYPE_DE_DONNEES)) {
        return table_types_de_donnees.valeur_ou(type_connu, nullptr);
    }

    auto résultat = alloc->m_noeuds_type_type_de_données.ajoute_élément();
    initialise_type_type_de_données(*this, résultat, type_connu);
    table_types_de_donnees.insère(type_connu, résultat);
    return résultat;
}

TypeStructure *Typeuse::réserve_type_structure()
{
    return alloc->m_noeuds_type_structure.ajoute_élément();
}

TypeUnion *Typeuse::union_anonyme(Lexème const *lexeme,
                                  NoeudBloc *bloc_parent,
                                  const kuri::tablet<RubriqueTypeComposé, 6> &rubriques)
{
    VERROUILLE(types_unions);

    POUR_TABLEAU_PAGE (alloc->m_noeuds_type_union) {
        if (!it.est_anonyme) {
            continue;
        }

        if (it.rubriques.taille() != rubriques.taille()) {
            continue;
        }

        auto type_apparie = true;

        for (auto i = 0; i < it.rubriques.taille(); ++i) {
            if (it.rubriques[i].type != rubriques[i].type) {
                type_apparie = false;
                break;
            }
        }

        if (type_apparie) {
            return &it;
        }
    }

    auto type = alloc->m_noeuds_type_union.ajoute_élément();
    type->ident = ID::anonyme;
    type->lexème = lexeme;
    type->bloc_parent = bloc_parent;

    type->rubriques.réserve(static_cast<int>(rubriques.taille()));
    POUR (rubriques) {
        type->rubriques.ajoute(it);
    }
    type->nombre_de_rubriques_réelles = type->rubriques.taille();

    type->est_anonyme = true;
    type->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);

    marque_polymorphique(type);

    if (!type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        calcule_taille_type_composé(type, false, 0);
        crée_type_structure(*this, type, type->décalage_indice);
    }

    return type;
}

TypePolymorphique *Typeuse::crée_polymorphique(IdentifiantCode *ident)
{
    VERROUILLE(types_polymorphiques);

    // pour le moment un ident nul est utilisé pour les types polymorphiques des
    // structures dans les types des fonction (foo :: fonc (p: Polymorphe(T = $T)),
    // donc ne déduplique pas ces types pour éviter les problèmes quand nous validons
    // ces expresssions, car les données associées doivent être spécifiques à chaque
    // déclaration
    if (ident) {
        POUR_TABLEAU_PAGE (alloc->m_noeuds_type_polymorphique) {
            if (it.ident == ident) {
                return &it;
            }
        }
    }

    auto résultat = alloc->m_noeuds_type_polymorphique.ajoute_élément();
    initialise_type_polymorphique(résultat, ident);
    return résultat;
}

TypeOpaque *Typeuse::monomorphe_opaque(NoeudDéclarationTypeOpaque const *decl,
                                       Type *type_monomorphique)
{
    VERROUILLE(types_opaques);

    POUR_TABLEAU_PAGE (alloc->m_noeuds_type_opaque) {
        if (it.polymorphe_de_base != decl) {
            continue;
        }

        if (it.type_opacifié == type_monomorphique) {
            return &it;
        }
    }

    auto type = alloc->m_noeuds_type_opaque.ajoute_élément();
    initialise_type_opaque(type, type_monomorphique);
    type->ident = decl->ident;
    type->polymorphe_de_base = decl;
    type->drapeaux |= DrapeauxNoeud::DECLARATION_FUT_VALIDEE;
    graphe_->connecte_type_type(type, type_monomorphique);
    return type;
}

TypeTuple *Typeuse::crée_tuple(const kuri::tablet<RubriqueTypeComposé, 6> &rubriques)
{
    VERROUILLE(types_tuples);

    POUR_TABLEAU_PAGE (alloc->m_noeuds_type_tuple) {
        if (it.rubriques.taille() != rubriques.taille()) {
            continue;
        }

        auto trouve = true;

        for (auto i = 0; i < rubriques.taille(); ++i) {
            if (it.rubriques[i].type != rubriques[i].type) {
                trouve = false;
                break;
            }
        }

        if (trouve) {
            return &it;
        }
    }

    auto type = alloc->m_noeuds_type_tuple.ajoute_élément();
    type->rubriques.réserve(static_cast<int>(rubriques.taille()));

    POUR (rubriques) {
        type->rubriques.ajoute(it);
        graphe_->connecte_type_type(type, it.type);
    }
    type->nombre_de_rubriques_réelles = type->rubriques.taille();

    marque_polymorphique(type);

    type->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    type->drapeaux_type |= (DrapeauxTypes::TYPE_NE_REQUIERS_PAS_D_INITIALISATION |
                            DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE);

    return type;
}

void Typeuse::rassemble_statistiques(Statistiques &stats) const
{
    auto mémoire = int64_t(0);

    mémoire += trie.noeuds.mémoire_utilisée();

    POUR_TABLEAU_PAGE (trie.noeuds) {
        mémoire += it.enfants.table.taille_mémoire();
        mémoire += it.enfants_sortie.table.taille_mémoire();
    }

    mémoire += m_infos_types_vers_types.taille_mémoire();
    mémoire += table_types_de_donnees.taille_mémoire();
    mémoire += types_simples->taille_mémoire();
    mémoire += taille_de(AllocatriceNoeud);

    stats.ajoute_mémoire_utilisée("Typeuse", mémoire);

    alloc->rassemble_statistiques(stats);
}

void Typeuse::définis_info_type_pour_type(const InfoType *info_type, const Type *type)
{
    VERROUILLE(infos_types_vers_types);
    m_infos_types_vers_types.insère(info_type, type);
}

NoeudDéclaration const *Typeuse::decl_pour_info_type(InfoType const *info_type)
{
    VERROUILLE(infos_types_vers_types);
    bool trouvé = false;
    auto résultat = m_infos_types_vers_types.trouve(info_type, trouvé);
    if (!résultat) {
        return nullptr;
    }

    if (!résultat->lexème) {
        return nullptr;
    }

    return résultat;
}

NoeudDéclarationTypeOpaque *Typeuse::crée_opaque_défaut(Type *type_opacifié,
                                                        IdentifiantCode *ident)
{
    auto résultat = alloc->m_noeuds_type_opaque.ajoute_élément();
    résultat->type_opacifié = type_opacifié;
    résultat->ident = ident;
    résultat->alignement = type_opacifié->alignement;
    résultat->taille_octet = type_opacifié->taille_octet;
    résultat->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    return résultat;
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions diverses pour les types.
 * \{ */

void assigne_fonction_init(Type *type, NoeudDéclarationEntêteFonction *fonction)
{
    type->fonction_init = fonction;
    type->drapeaux_type |= DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE;
}

/* Retourne vrai si le type à besoin d'une fonction d'initialisation que celle-ci soit partagée
 * ou non.
 */
bool requiers_fonction_initialisation(Type const *type)
{
    return !type->possède_drapeau(DrapeauxTypes::TYPE_NE_REQUIERS_PAS_D_INITIALISATION);
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
    if (type->possède_drapeau(DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE)) {
        return false;
    }

    if (est_type_polymorphique(type)) {
        return false;
    }

    return true;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux rubriques des types composés.
 * \{ */

std::optional<InformationRubriqueTypeCompose> donne_rubrique_pour_type(
    TypeCompose const *type_composé, Type const *type)
{
    POUR_INDICE (type_composé->rubriques) {
        if (it.type == type) {
            return InformationRubriqueTypeCompose{it, indice_it};
        }
    }

    return {};
}

std::optional<InformationRubriqueTypeCompose> donne_rubrique_pour_nom(
    TypeCompose const *type_composé, IdentifiantCode const *nom_rubrique)
{
    POUR_INDICE (type_composé->rubriques) {
        if (it.nom == nom_rubrique) {
            return InformationRubriqueTypeCompose{it, indice_it};
        }
    }

    return {};
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Accès aux noms hiérarchiques des types.
 * \{ */

kuri::chaine_statique donne_nom_hiérarchique(TypeUnion *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = chaine_type(type, false);
    return type->nom_hiérarchique_;
}

kuri::chaine_statique donne_nom_hiérarchique(TypeEnum *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = chaine_type(type, false);
    return type->nom_hiérarchique_;
}

kuri::chaine_statique donne_nom_hiérarchique(TypeOpaque *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = chaine_type(type, false);
    return type->nom_hiérarchique_;
}

kuri::chaine_statique donne_nom_hiérarchique(TypeStructure *type)
{
    if (type->nom_hiérarchique_ != "") {
        return type->nom_hiérarchique_;
    }

    type->nom_hiérarchique_ = chaine_type(type, false);
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

kuri::chaine_statique donne_nom_portable(TypeUnion *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->bloc_parent, type->ident->nom);
    return type->nom_portable_;
}

kuri::chaine_statique donne_nom_portable(TypeEnum *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->bloc_parent, type->ident->nom);
    return type->nom_portable_;
}

kuri::chaine_statique donne_nom_portable(TypeOpaque *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->bloc_parent, type->ident->nom);
    return type->nom_portable_;
}

kuri::chaine_statique donne_nom_portable(TypeStructure *type)
{
    if (type->nom_portable_ != "") {
        return type->nom_portable_;
    }

    type->nom_portable_ = nom_portable(type->bloc_parent, type->ident->nom);
    return type->nom_portable_;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Marquage des types comme étant polymorphiques.
 * \{ */

void marque_polymorphique(TypeFonction *type)
{
    POUR (type->types_entrées) {
        if (it->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            type->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
            return;
        }
    }

    // À FAIRE(architecture) : il est possible que le type_sortie soit nul car ce peut être une
    // union non encore validée, donc son type_le_plus_grand ou type_structure n'est pas encore
    // généré. Il y a plusieurs problèmes à résoudre :
    // - une unité de compilation ne doit aller en RI tant qu'une de ses dépendances n'est pas
    // encore validée (requiers de se débarrasser du graphe et utiliser les unités comme « noeud »)
    // - la gestion des types polymorphiques est à revoir, notamment la manière ils sont stockés
    // - nous ne devrions pas marquer comme polymorphique lors de la génération de RI
    if (type->type_sortie &&
        type->type_sortie->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        type->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
    }
}

void marque_polymorphique(TypeCompose *type)
{
    POUR (type->rubriques) {
        if (it.type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            type->drapeaux_type |= DrapeauxTypes::TYPE_EST_POLYMORPHIQUE;
            return;
        }
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Fonctions pour les unions.
 * \{ */

void crée_type_structure(Typeuse &typeuse, TypeUnion *type, unsigned alignement_rubrique_active)
{
    assert(!type->est_nonsure);

    type->type_structure = typeuse.réserve_type_structure();

    /* À FAIRE : ceci ne fonctionnera pas pour des architectures en 32-bit.
     * Il faudra remplacer le type_le_plus_grand par une union et transtyper proprement dans les
     * accès. */
    if (type->type_le_plus_grand) {
        auto rubriques_ = kuri::tableau<RubriqueTypeComposé, int>(2);
        rubriques_[0] = {nullptr, type->type_le_plus_grand, ID::valeur, 0};
        rubriques_[1] = {
            nullptr, typeuse.type_z32, ID::rubrique_active, alignement_rubrique_active};
        type->type_structure->rubriques = std::move(rubriques_);
        type->type_structure->nombre_de_rubriques_réelles =
            type->type_structure->rubriques.taille();
    }
    else {
        auto rubriques_ = kuri::tableau<RubriqueTypeComposé, int>(1);
        rubriques_[0] = {
            nullptr, typeuse.type_z32, ID::rubrique_active, alignement_rubrique_active};
        type->type_structure->rubriques = std::move(rubriques_);
        type->type_structure->nombre_de_rubriques_réelles =
            type->type_structure->rubriques.taille();
    }

    type->type_structure->bloc_parent = type->bloc_parent;
    type->type_structure->lexème = type->lexème;
    type->type_structure->ident = type->ident;
    type->type_structure->taille_octet = type->taille_octet;
    type->type_structure->alignement = type->alignement;
    type->type_structure->ident = type->ident;
    type->type_structure->est_anonyme = type->est_anonyme;
    type->type_structure->est_monomorphisation = type->est_monomorphisation;
    type->type_structure->est_polymorphe = type->est_polymorphe;
    type->type_structure->bloc_constantes = type->bloc_constantes;
    type->type_structure->union_originelle = type;
    /* L'initialisation est créée avec le type de l'union et non celui de la structure. */
    type->type_structure->drapeaux |= (DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
    type->type_structure->drapeaux_type |= (DrapeauxTypes::INITIALISATION_TYPE_FUT_CREEE |
                                            DrapeauxTypes::UNITE_POUR_INITIALISATION_FUT_CREE);

    typeuse.graphe_->connecte_type_type(type, type->type_structure);
}

/** \} */

/* ************************************************************************** */

struct ParenthèseParamètres {
    kuri::chaine_statique début{};
    kuri::chaine_statique fin{};
};

static ParenthèseParamètres donne_parenthèses_paramètres_polymorphiques(
    OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_PARENTHÈSE_PARAMÈTRE)) {
        return {"_", ""};
    }
    return {"(", ")"};
}

static ParenthèseParamètres donne_parenthèses_fonction(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_PARENTHÈSE_FONCTION)) {
        return {"_", ""};
    }
    return {"(", ")"};
}

static kuri::chaine_statique donne_séparateur_paramètres_fonction(
    OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_PARENTHÈSE_FONCTION)) {
        return "_";
    }
    return ", ";
}

static kuri::chaine_statique donne_séparateur_rubriques_union_anonyme(
    OptionsImpressionType const options)
{
    if (drapeau_est_actif(options,
                          OptionsImpressionType::NORMALISE_SÉPARATEUR_RUBRIQUES_ANONYMES)) {
        return "_";
    }
    return " | ";
}

static kuri::chaine_statique donne_séparateur_hiérarchie(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_SÉPARATEUR_HIÉRARCHIE)) {
        return "_";
    }
    return ".";
}

static kuri::chaine_statique donne_séparateur_paramètres(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_PARENTHÈSE_PARAMÈTRE)) {
        return "_";
    }
    return ", ";
}

static kuri::chaine_statique donne_spécifiant_pointeur(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_SPÉCIFIANT_TYPE)) {
        return "KP";
    }
    return "*";
}

static kuri::chaine_statique donne_spécifiant_référence(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_SPÉCIFIANT_TYPE)) {
        return "KR";
    }
    return "&";
}

static kuri::chaine_statique donne_spécifiant_variadique(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_SPÉCIFIANT_TYPE)) {
        return "Kv";
    }
    return "...";
}

static ParenthèseParamètres donne_spécifiant_tableau_fixe(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_SPÉCIFIANT_TYPE)) {
        return {"KT", "_"};
    }
    return {"[", "]"};
}

static ParenthèseParamètres donne_spécifiant_tableau(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_SPÉCIFIANT_TYPE)) {
        return {"Kt", "_"};
    }
    return {"[..", "]"};
}

static ParenthèseParamètres donne_spécifiant_tranche(OptionsImpressionType const options)
{
    if (drapeau_est_actif(options, OptionsImpressionType::NORMALISE_SPÉCIFIANT_TYPE)) {
        return {"Kz", "_"};
    }
    return {"[", "]"};
}

static void nom_hiérarchique(Enchaineuse &enchaineuse,
                             NoeudBloc *bloc,
                             kuri::chaine_statique ident,
                             OptionsImpressionType options)
{
    auto const noms = donne_les_noms_de_la_hiérarchie(bloc);

    /* -2 pour éviter le nom du module. */
    for (auto i = noms.taille() - 2; i >= 0; --i) {
        enchaineuse.ajoute(noms[i]->nom);
        enchaineuse.ajoute(donne_séparateur_hiérarchie(options));
    }
    enchaineuse.ajoute(ident);
}

static void chaine_type_structure(Enchaineuse &enchaineuse,
                                  const NoeudDéclarationClasse *decl,
                                  OptionsImpressionType options)
{
    nom_hiérarchique(enchaineuse, decl->bloc_parent, decl->ident->nom, options);

    auto parenthèses = donne_parenthèses_paramètres_polymorphiques(options);

    auto virgule = parenthèses.début;
    if (decl->est_monomorphisation) {
        POUR ((*decl->bloc_constantes->rubriques.verrou_lecture())) {
            enchaineuse << virgule;

            if (drapeau_est_actif(options,
                                  OptionsImpressionType::AJOUTE_PARAMÈTRES_POLYMORPHIQUE)) {
                enchaineuse << it->ident->nom << ": ";
            }

            if (it->type->est_type_type_de_données()) {
                auto nouvelles_options = options;
                if (drapeau_est_actif(options,
                                      OptionsImpressionType::NORMALISE_PARENTHÈSE_PARAMÈTRE)) {
                    nouvelles_options |= OptionsImpressionType::NORMALISE_PARENTHÈSE_FONCTION;
                    nouvelles_options |= OptionsImpressionType::NORMALISE_SÉPARATEUR_HIÉRARCHIE;
                    nouvelles_options |= OptionsImpressionType::NORMALISE_SPÉCIFIANT_TYPE;
                }

                enchaineuse << chaine_type(it->type->comme_type_type_de_données()->type_connu,
                                           nouvelles_options);
            }
            else {
                enchaineuse << it->comme_déclaration_constante()->valeur_expression;
            }

            virgule = donne_séparateur_paramètres(options);
        }
        enchaineuse << parenthèses.fin;
    }
    else if (decl->est_polymorphe) {
        POUR ((*decl->bloc_constantes->rubriques.verrou_lecture())) {
            enchaineuse << virgule;
            enchaineuse << '$' << it->ident->nom;
            virgule = donne_séparateur_paramètres(options);
        }
        enchaineuse << parenthèses.fin;
    }
}

static void chaine_type(Enchaineuse &enchaineuse, const Type *type, OptionsImpressionType options)
{
    if (type == nullptr) {
        enchaineuse.ajoute("nul");
        return;
    }

    if (drapeau_est_actif(options, OptionsImpressionType::INCLUS_HIÉRARCHIE)) {
        auto nh = donne_les_noms_de_la_hiérarchie(type->bloc_parent);
        for (auto i = nh.taille() - 1; i >= 0; --i) {
            if (nh[i]->nom) {
                enchaineuse.ajoute(nh[i]->nom);
                enchaineuse.ajoute(donne_séparateur_hiérarchie(options));
            }
        }
    }

    switch (type->genre) {
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::RIEN:
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::RÉEL:
        {
            enchaineuse.ajoute(type->ident->nom);
            return;
        }
        case GenreNoeud::RÉFÉRENCE:
        {
            enchaineuse.ajoute(donne_spécifiant_référence(options));
            chaine_type(
                enchaineuse, static_cast<TypeReference const *>(type)->type_pointé, options);
            return;
        }
        case GenreNoeud::POINTEUR:
        {
            auto const type_pointe = type->comme_type_pointeur()->type_pointé;
            if (type_pointe == nullptr) {
                enchaineuse.ajoute("type_de(nul)");
            }
            else {
                enchaineuse.ajoute(donne_spécifiant_pointeur(options));
                chaine_type(enchaineuse, type_pointe, options);
            }
            return;
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = static_cast<TypeUnion const *>(type);

            if (type_union->est_anonyme) {
                auto séparateur = donne_séparateur_rubriques_union_anonyme(options);
                auto virgule = kuri::chaine_statique("");
                POUR (type_union->donne_rubriques_pour_code_machine()) {
                    enchaineuse << virgule << chaine_type(it.type, options);
                    virgule = séparateur;
                }
                return;
            }

            chaine_type_structure(enchaineuse, type_union, options);
            return;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto type_structure = static_cast<TypeStructure const *>(type);

            if (!type_structure->ident) {
                enchaineuse.ajoute("struct.anonyme");
                return;
            }

            chaine_type_structure(enchaineuse, type_structure, options);
            return;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto parenthèse = donne_spécifiant_tableau(options);
            enchaineuse << parenthèse.début << parenthèse.fin;
            chaine_type(enchaineuse,
                        static_cast<TypeTableauDynamique const *>(type)->type_pointé,
                        options);
            return;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto parenthèse = donne_spécifiant_tranche(options);
            enchaineuse << parenthèse.début << parenthèse.fin;
            chaine_type(enchaineuse, type->comme_type_tranche()->type_élément, options);
            return;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tabl = static_cast<TypeTableauFixe const *>(type);

            auto parenthèse = donne_spécifiant_tableau_fixe(options);

            enchaineuse << parenthèse.début;

            if (type_tabl->expression_taille) {
                enchaineuse << "$" << type_tabl->expression_taille->ident->nom;
            }
            else {
                enchaineuse << type_tabl->taille;
            }

            enchaineuse << parenthèse.fin;
            chaine_type(enchaineuse, type_tabl->type_pointé, options);
            return;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_variadique = static_cast<TypeVariadique const *>(type);
            enchaineuse << donne_spécifiant_variadique(options);
            /* N'imprime rien pour les types variadiques externes. */
            if (type_variadique->type_pointé) {
                chaine_type(enchaineuse, type_variadique->type_pointé, options);
            }
            return;
        }
        case GenreNoeud::FONCTION:
        {
            auto type_fonc = static_cast<TypeFonction const *>(type);

            enchaineuse << "fonc";
            auto parenthèses = donne_parenthèses_fonction(options);

            auto virgule = parenthèses.début;

            POUR (type_fonc->types_entrées) {
                enchaineuse << virgule;
                chaine_type(enchaineuse, it, options);
                virgule = donne_séparateur_paramètres_fonction(options);
            }

            if (type_fonc->types_entrées.est_vide()) {
                enchaineuse << virgule;
            }
            enchaineuse << parenthèses.fin;

            auto const retourne_tuple = type_fonc->type_sortie->est_type_tuple();

            if (!retourne_tuple) {
                enchaineuse << parenthèses.début;
            }

            chaine_type(enchaineuse, type_fonc->type_sortie, options);

            if (!retourne_tuple) {
                enchaineuse << parenthèses.fin;
            }
            return;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            nom_hiérarchique(enchaineuse, type->bloc_parent, type->ident->nom, options);
            return;
        }
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            auto type_de_donnees = type->comme_type_type_de_données();
            enchaineuse << "type_de_données";
            if (type_de_donnees->type_connu) {
                enchaineuse << '(' << chaine_type(type_de_donnees->type_connu) << ')';
            }
            return;
        }
        case GenreNoeud::POLYMORPHIQUE:
        {
            auto type_polymorphique = static_cast<TypePolymorphique const *>(type);
            enchaineuse << "$" << type_polymorphique->ident->nom;
            return;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = static_cast<TypeOpaque const *>(type);
            nom_hiérarchique(enchaineuse, type->bloc_parent, type->ident->nom, options);

            if (drapeau_est_actif(options,
                                  OptionsImpressionType::AJOUTE_PARAMÈTRES_POLYMORPHIQUE)) {
                enchaineuse << "(";
                chaine_type(enchaineuse, type_opaque->type_opacifié, options);
                enchaineuse << ")";
            }
            return;
        }
        case GenreNoeud::TUPLE:
        {
            auto type_tuple = static_cast<TypeTuple const *>(type);
            auto virgule = "(";
            POUR (type_tuple->rubriques) {
                enchaineuse << virgule;
                chaine_type(enchaineuse, it.type, options);
                virgule = ", ";
            }

            enchaineuse << ")";
            return;
        }
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            enchaineuse << "adresse_fonction";
            return;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }
}

kuri::chaine chaine_type(const Type *type, OptionsImpressionType options)
{
    Enchaineuse enchaineuse;
    chaine_type(enchaineuse, type, options);
    return enchaineuse.chaine();
}

kuri::chaine chaine_type(const Type *type, bool ajoute_nom_paramètres_polymorphiques)
{
    OptionsImpressionType options = OptionsImpressionType::AUCUNE;
    if (ajoute_nom_paramètres_polymorphiques) {
        options |= OptionsImpressionType::AJOUTE_PARAMÈTRES_POLYMORPHIQUE;
    }

    return chaine_type(type, options);
}

Type *type_déréférencé_pour(Type const *type)
{
    if (type->est_type_pointeur()) {
        return type->comme_type_pointeur()->type_pointé;
    }

    if (type->est_type_référence()) {
        return type->comme_type_référence()->type_pointé;
    }

    if (type->est_type_tableau_fixe()) {
        return type->comme_type_tableau_fixe()->type_pointé;
    }

    if (type->est_type_tableau_dynamique()) {
        return type->comme_type_tableau_dynamique()->type_pointé;
    }

    if (type->est_type_tranche()) {
        return type->comme_type_tranche()->type_élément;
    }

    if (type->est_type_variadique()) {
        return type->comme_type_variadique()->type_pointé;
    }

    if (type->est_type_opaque()) {
        return type_déréférencé_pour(type->comme_type_opaque()->type_opacifié);
    }

    return nullptr;
}

bool est_type_booléen_implicite(Type *type)
{
    return est_élément(type->genre,
                       GenreNoeud::BOOL,
                       GenreNoeud::CHAINE,
                       GenreNoeud::EINI,
                       GenreNoeud::ENTIER_CONSTANT,
                       GenreNoeud::ENTIER_NATUREL,
                       GenreNoeud::ENTIER_RELATIF,
                       GenreNoeud::FONCTION,
                       GenreNoeud::TYPE_ADRESSE_FONCTION,
                       GenreNoeud::POINTEUR,
                       GenreNoeud::TABLEAU_DYNAMIQUE,
                       GenreNoeud::TYPE_TRANCHE);
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

    POUR (type->donne_rubriques_pour_code_machine()) {
        assert_rappel(it.type->taille_octet != 0, [&] {
            dbg() << "Taille octet de 0 pour le type « " << chaine_type(it.type) << " »";
        });

        if (!COMPACTE) {
            /* Ajout de rembourrage entre les rubriques si le type n'est pas compacte. */
            auto alignement_type = it.type->alignement;

            assert_rappel(it.type->alignement != 0, [&] {
                dbg() << "Alignement de 0 pour le type « " << chaine_type(it.type) << " »";
            });

            alignement_max = std::max(alignement_type, alignement_max);
            auto rembourrage = marge_pour_alignement(alignement_type, decalage);
            const_cast<RubriqueTypeComposé &>(it).rembourrage = rembourrage;
            decalage += rembourrage;
        }

        const_cast<RubriqueTypeComposé &>(it).decalage = decalage;
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

void calcule_taille_type_composé(TypeCompose *type, bool compacte, uint32_t alignement_desire)
{
    if (type->est_type_union()) {
        auto type_union = type->comme_type_union();

        auto max_alignement = 0u;
        auto taille_union = 0u;
        auto type_le_plus_grand = Type::nul();

        POUR (type->donne_rubriques_pour_code_machine()) {
            auto type_rubrique = it.type;
            auto taille = type_rubrique->taille_octet;

            /* Ignore les rubriques qui n'ont pas de type. */
            if (type_rubrique->est_type_rien()) {
                continue;
            }

            max_alignement = std::max(type_rubrique->alignement, max_alignement);

            assert_rappel(it.type->alignement != 0, [&] {
                dbg() << "Dans le calcul de la taille du type : " << chaine_type(type) << '\n'
                      << "Alignement de 0 pour le type « " << chaine_type(it.type) << " » ";
            });

            assert_rappel(it.type->taille_octet != 0, [&] {
                dbg() << "Taille octet de 0 pour le type « " << chaine_type(it.type) << " »";
            });

            if (taille > taille_union) {
                type_le_plus_grand = type_rubrique;
                taille_union = taille;
            }
        }

        /* Pour les unions sûres, il nous faut prendre en compte le
         * rubrique supplémentaire. */
        if (!type_union->est_nonsure) {
            /* Il est possible que tous les rubriques soit de type « rien » ou que l'union soit
             * déclarée sans rubrique. */
            if (taille_union != 0) {
                /* ajoute une marge d'alignement */
                taille_union += marge_pour_alignement(max_alignement, taille_union);
            }

            type_union->décalage_indice = taille_union;

            /* ajoute la taille du rubrique actif */
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

bool est_type_polymorphique(Type const *type)
{
    if (type->est_type_polymorphique()) {
        return true;
    }

    if (type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        return true;
    }

    if (type->est_type_structure() && type->comme_type_structure()->est_polymorphe) {
        return true;
    }

    if (type->est_type_union() && type->comme_type_union()->est_polymorphe) {
        return true;
    }

    return false;
}

bool est_type_tableau_fixe(Type const *type)
{
    auto type_primitif = donne_type_primitif(type);
    return type_primitif->est_type_tableau_fixe();
}

bool est_pointeur_vers_tableau_fixe(Type const *type)
{
    if (!type->est_type_pointeur()) {
        return false;
    }

    auto const type_pointeur = type->comme_type_pointeur();

    if (!type_pointeur->type_pointé) {
        return false;
    }

    return est_type_tableau_fixe(type_pointeur->type_pointé);
}

bool est_type_sse2(Type const *type)
{
    return type->est_déclaration_classe() && type->comme_déclaration_classe()->est_sse2;
}

/* Retourne vrai si le type possède un info type qui est seulement une instance de InfoType et non
 * un type dérivé. */
bool est_structure_info_type_défaut(GenreNoeud genre)
{
    switch (genre) {
        case GenreNoeud::EINI:
        case GenreNoeud::RIEN:
        case GenreNoeud::CHAINE:
        case GenreNoeud::TYPE_DE_DONNÉES:
        case GenreNoeud::RÉEL:
        case GenreNoeud::OCTET:
        case GenreNoeud::BOOL:
            return true;
        default:
            return false;
    }
}

Type const *donne_type_opacifié_racine(TypeOpaque const *type_opaque)
{
    Type const *résultat = type_opaque->type_opacifié;
    while (résultat->est_type_opaque()) {
        résultat = résultat->comme_type_opaque()->type_opacifié;
    }
    return résultat;
}

Type const *type_entier_sous_jacent(Type const *type)
{
    if (type->est_type_énum()) {
        return type->comme_type_énum()->type_sous_jacent;
    }

    if (type->est_type_erreur()) {
        return type->comme_type_erreur()->type_sous_jacent;
    }

    if (type->est_type_type_de_données()) {
        return type->comme_type_type_de_données()->type_code_machine;
    }

    if (type->est_type_entier_naturel() || type->est_type_entier_relatif()) {
        return type;
    }

    if (type->est_type_opaque()) {
        return type_entier_sous_jacent(type->comme_type_opaque()->type_opacifié);
    }

    return nullptr;
}

Type const *donne_type_primitif(Type const *type)
{
    while (true) {
        if (type->est_type_énum()) {
            return type->comme_type_énum()->type_sous_jacent;
        }

        if (type->est_type_erreur()) {
            return type->comme_type_erreur()->type_sous_jacent;
        }

        if (type->est_type_type_de_données()) {
            return type->comme_type_type_de_données()->type_code_machine;
        }

        if (type->est_type_entier_naturel() || type->est_type_entier_relatif()) {
            return type;
        }

        if (type->est_type_opaque()) {
            type = type->comme_type_opaque()->type_opacifié;
            continue;
        }

        break;
    }

    return type;
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
    return type->est_type_pointeur() && type->comme_type_pointeur()->type_pointé == nullptr;
}

ResultatRechercheRubrique trouve_indice_rubrique_unique_type_compatible(TypeCompose const *type,
                                                                        Type const *type_a_tester)
{
    auto const pointeur_nul = est_type_pointeur_nul(type_a_tester);
    int indice_rubrique = -1;
    int indice_courant = 0;
    POUR (type->rubriques) {
        if (it.type == type_a_tester) {
            if (indice_rubrique != -1) {
                return PlusieursRubriques{-1};
            }

            indice_rubrique = indice_courant;
        }
        else if (type_a_tester->est_type_pointeur() && it.type->est_type_pointeur()) {
            if (pointeur_nul) {
                if (indice_rubrique != -1) {
                    return PlusieursRubriques{-1};
                }

                indice_rubrique = indice_courant;
            }
            else {
                auto type_pointe_de = type_a_tester->comme_type_pointeur()->type_pointé;
                auto type_pointe_vers = it.type->comme_type_pointeur()->type_pointé;

                if (est_type_de_base(type_pointe_de, type_pointe_vers)) {
                    if (indice_rubrique != -1) {
                        return PlusieursRubriques{-1};
                    }

                    indice_rubrique = indice_courant;
                }
            }
        }
        else if (est_type_entier(it.type) && type_a_tester->est_type_entier_constant()) {
            if (indice_rubrique != -1) {
                return PlusieursRubriques{-1};
            }

            indice_rubrique = indice_courant;
        }

        indice_courant += 1;
    }

    if (indice_rubrique == -1) {
        return AucunRubrique{-1};
    }

    return IndexRubrique{indice_rubrique};
}

/* Calcule la « profondeur » du type : à savoir, le nombre de déréférencement du type (jusqu'à
 * arriver à un type racine) + 1.
 * Par exemple, *z32 a une profondeur de 2 (1 déréférencement de pointeur + 1), alors que [..]*z32
 * en a une de 3. */
int donne_profondeur_type(Type const *type)
{
    auto profondeur_type = 1;
    auto type_courant = type;
    while (Type *sous_type = type_déréférencé_pour(type_courant)) {
        profondeur_type += 1;
        type_courant = sous_type;
    }
    return profondeur_type;
}

bool est_type_valide_pour_rubrique(Type const *rubrique_type)
{
    if (rubrique_type->est_type_rien()) {
        return false;
    }

    if (rubrique_type->est_type_variadique()) {
        return false;
    }

    return true;
}

bool peut_construire_union_via_rien(TypeUnion const *type_union)
{
    POUR (type_union->rubriques) {
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

    if (type->est_type_énum()) {
        /* Pour l'instant, les énum_drapeaux ne sont pas utilisable, car les index peuvent être
         * arbitrairement larges. */
        return !type->est_type_enum_drapeau();
    }

    if (type->est_type_bool()) {
        return true;
    }

    if (type->est_type_type_de_données()) {
        /* Les type_de_données doivent pouvoir être utilisé pour indexer la table des types, car
         * leurs valeurs dépends de l'index du type dans ladite table. */
        return true;
    }

    if (type->est_type_opaque()) {
        return est_type_implicitement_utilisable_pour_indexage(
            type->comme_type_opaque()->type_opacifié);
    }

    return false;
}

bool peut_etre_type_constante(Type const *type)
{
    switch (type->genre) {
        /* Possible mais non supporté pour le moment. */
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        /* Il n'est pas encore clair comment prendre le pointeur de la constante pour les tableaux
         * dynamiques. */
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        /* Sémantiquement, les variadiques ne peuvent être utilisées que pour les paramètres de
         * fonctions. */
        case GenreNoeud::VARIADIQUE:
        /* Il n'est pas claire comment gérer les unions, les sûres doivent avoir une rubrique
         * actif, et les valeurs pour les sûres ou nonsûres doivent être transtypées sur le
         * lieu d'utilisation. */
        case GenreNoeud::DÉCLARATION_UNION:
        /* Un eini doit avoir une info-type, et prendre une valeur par pointeur, qui n'est pas
         * encore supporté pour les constantes. */
        case GenreNoeud::EINI:
        /* Les tuples ne sont que pour les retours de fonctions. */
        case GenreNoeud::TUPLE:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::RIEN:
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
           type_dest->comme_type_opaque()->type_opacifié == type_source;
}

bool est_type_fondamental(const Type *type)
{
    switch (type->genre) {
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::RIEN:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::DÉCLARATION_UNION:
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TABLEAU_FIXE:
        case GenreNoeud::TYPE_TRANCHE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::TUPLE:
        {
            return false;
        }
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::RÉEL:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::FONCTION:
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::TYPE_DE_DONNÉES:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            return true;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            return est_type_fondamental(type->comme_type_opaque()->type_opacifié);
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    return false;
}

template <typename Predicat>
static void attentes_sur_types_si_condition_échoue(kuri::ensemblon<Type *, 16> const &types,
                                                   kuri::tablet<Attente, 16> &attentes,
                                                   Predicat &&prédicat)
{
    auto visités = kuri::ensemblon<Type *, 16>();
    auto pile = kuri::pile<Type *>();

    pour_chaque_élément(types, [&pile](auto &type) {
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

        visités.insère(type_courant);

        if (!prédicat(type_courant)) {
            attentes.ajoute(Attente::sur_type(type_courant));
        }

        switch (type_courant->genre) {
            case GenreNoeud::POLYMORPHIQUE:
            case GenreNoeud::TUPLE:
            case GenreNoeud::EINI:
            case GenreNoeud::CHAINE:
            case GenreNoeud::RIEN:
            case GenreNoeud::BOOL:
            case GenreNoeud::OCTET:
            case GenreNoeud::TYPE_DE_DONNÉES:
            case GenreNoeud::RÉEL:
            case GenreNoeud::ENTIER_CONSTANT:
            case GenreNoeud::ENTIER_NATUREL:
            case GenreNoeud::ENTIER_RELATIF:
            case GenreNoeud::DÉCLARATION_ÉNUM:
            case GenreNoeud::ERREUR:
            case GenreNoeud::ENUM_DRAPEAU:
            case GenreNoeud::TYPE_ADRESSE_FONCTION:
            {
                break;
            }
            case GenreNoeud::FONCTION:
            {
                auto type_fonction = type_courant->comme_type_fonction();
                POUR (type_fonction->types_entrées) {
                    pile.empile(it);
                }
                pile.empile(type_fonction->type_sortie);
                break;
            }
            case GenreNoeud::DÉCLARATION_UNION:
            case GenreNoeud::DÉCLARATION_STRUCTURE:
            {
                auto type_compose = type_courant->comme_type_composé();
                POUR (type_compose->donne_rubriques_pour_code_machine()) {
                    pile.empile(it.type);
                }
                break;
            }
            case GenreNoeud::RÉFÉRENCE:
            {
                pile.empile(type_courant->comme_type_référence()->type_pointé);
                break;
            }
            case GenreNoeud::POINTEUR:
            {
                pile.empile(type_courant->comme_type_pointeur()->type_pointé);
                break;
            }
            case GenreNoeud::VARIADIQUE:
            {
                pile.empile(type_courant->comme_type_variadique()->type_pointé);
                break;
            }
            case GenreNoeud::TABLEAU_DYNAMIQUE:
            {
                pile.empile(type_courant->comme_type_tableau_dynamique()->type_pointé);
                break;
            }
            case GenreNoeud::TYPE_TRANCHE:
            {
                pile.empile(type_courant->comme_type_tranche()->type_élément);
                break;
            }
            case GenreNoeud::TABLEAU_FIXE:
            {
                pile.empile(type_courant->comme_type_tableau_fixe()->type_pointé);
                break;
            }
            case GenreNoeud::DÉCLARATION_OPAQUE:
            {
                pile.empile(type_courant->comme_type_opaque()->type_opacifié);
                break;
            }
            CAS_POUR_NOEUDS_HORS_TYPES:
            {
                assert_rappel(false, [&]() {
                    dbg() << "Noeud non-géré pour type : " << type_courant->genre;
                });
                break;
            }
        }
    }
}

void attentes_sur_types_si_drapeau_manquant(kuri::ensemblon<Type *, 16> const &types,
                                            DrapeauxTypes drapeau,
                                            kuri::tablet<Attente, 16> &attentes)
{
    attentes_sur_types_si_condition_échoue(
        types, attentes, [&](Type *type) { return type->possède_drapeau(drapeau); });
}

void attentes_sur_types_si_drapeau_manquant(kuri::ensemblon<Type *, 16> const &types,
                                            DrapeauxNoeud drapeau,
                                            kuri::tablet<Attente, 16> &attentes)
{
    attentes_sur_types_si_condition_échoue(
        types, attentes, [&](Type *type) { return type->possède_drapeau(drapeau); });
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
        racine = noeuds.ajoute_élément();
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
    auto enfant = noeuds.ajoute_élément();
    enfant->type = type;
    if (est_sortie) {
        parent->enfants_sortie.ajoute(enfant);
    }
    else {
        parent->enfants.ajoute(enfant);
    }
    return enfant;
}

bool est_type_entier(const Type *type)
{
    return type->est_type_entier_naturel() || type->est_type_entier_relatif();
}

bool est_type_ptr_rien(const Type *type)
{
    return type->est_type_pointeur() && type->comme_type_pointeur()->type_pointé &&
           type->comme_type_pointeur()->type_pointé->genre == GenreNoeud::RIEN;
}

bool est_type_ptr_octet(const Type *type)
{
    return type->est_type_pointeur() && type->comme_type_pointeur()->type_pointé &&
           type->comme_type_pointeur()->type_pointé->genre == GenreNoeud::OCTET;
}

/* ------------------------------------------------------------------------- */
/** \name VisiteuseType.
 * \{ */

void VisiteuseType::visite_type(Type *type, std::function<void(Type *)> rappel)
{
    if (!type) {
        return;
    }

    if (visites.possède(type)) {
        return;
    }

    visites.insère(type);
    rappel(type);

    if (visite_types_fonctions_init && type->fonction_init) {
        visite_type(type->fonction_init->type, rappel);
    }

    switch (type->genre) {
        case GenreNoeud::EINI:
        {
            break;
        }
        case GenreNoeud::CHAINE:
        {
            break;
        }
        case GenreNoeud::RIEN:
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::RÉEL:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            break;
        }
        case GenreNoeud::RÉFÉRENCE:
        {
            auto reference = type->comme_type_référence();
            visite_type(reference->type_pointé, rappel);
            break;
        }
        case GenreNoeud::POINTEUR:
        {
            auto pointeur = type->comme_type_pointeur();
            visite_type(pointeur->type_pointé, rappel);
            break;
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type->comme_type_union();
            POUR (type_union->rubriques) {
                visite_type(it.type, rappel);
            }
            break;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto type_structure = type->comme_type_structure();
            POUR (type_structure->rubriques) {
                visite_type(it.type, rappel);
            }
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto tableau = type->comme_type_tableau_dynamique();
            visite_type(tableau->type_pointé, rappel);
            break;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto tranche = type->comme_type_tranche();
            visite_type(tranche->type_élément, rappel);
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto tableau = type->comme_type_tableau_fixe();
            visite_type(tableau->type_pointé, rappel);
            break;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto variadique = type->comme_type_variadique();
            visite_type(variadique->type_pointé, rappel);
            break;
        }
        case GenreNoeud::FONCTION:
        {
            auto fonction = type->comme_type_fonction();
            POUR (fonction->types_entrées) {
                visite_type(it, rappel);
            }
            visite_type(fonction->type_sortie, rappel);
            break;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        {
            auto type_enum = static_cast<TypeEnum *>(type);
            visite_type(type_enum->type_sous_jacent, rappel);
            break;
        }
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            break;
        }
        case GenreNoeud::POLYMORPHIQUE:
        {
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            visite_type(type_opaque->type_opacifié, rappel);
            break;
        }
        case GenreNoeud::TUPLE:
        {
            auto type_tuple = type->comme_type_tuple();
            POUR (type_tuple->rubriques) {
                visite_type(it.type, rappel);
            }
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }
}

NoeudDéclarationClasse const *donne_polymorphe_de_base(Type const *type)
{
    if (!type->est_déclaration_classe()) {
        return nullptr;
    }

    auto classe = type->comme_déclaration_classe();
    return classe->polymorphe_de_base;
}

/** \} */
