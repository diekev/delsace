/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "operateurs.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "statistiques/statistiques.hh"

#include "compilatrice.hh"
#include "contexte.hh"
#include "espace_de_travail.hh"
#include "monomorpheuse.hh"
#include "typage.hh"
#include "unite_compilation.hh"
#include "validation_semantique.hh"

#include "utilitaires/log.hh"
#include "utilitaires/macros.hh"

#include "plateforme/windows.h"

static OpérateurBinaire::Genre genre_op_binaire_pour_lexeme(GenreLexème genre_lexeme,
                                                            IndiceTypeOp type_opérandes)
{
    switch (genre_lexeme) {
        case GenreLexème::PLUS:
        case GenreLexème::PLUS_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Addition_Reel;
            }

            return OpérateurBinaire::Genre::Addition;
        }
        case GenreLexème::MOINS:
        case GenreLexème::MOINS_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Soustraction_Reel;
            }

            return OpérateurBinaire::Genre::Soustraction;
        }
        case GenreLexème::FOIS:
        case GenreLexème::MULTIPLIE_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Multiplication_Reel;
            }

            return OpérateurBinaire::Genre::Multiplication;
        }
        case GenreLexème::DIVISE:
        case GenreLexème::DIVISE_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Division_Reel;
            }

            if (type_opérandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OpérateurBinaire::Genre::Division_Naturel;
            }

            return OpérateurBinaire::Genre::Division_Relatif;
        }
        case GenreLexème::POURCENT:
        case GenreLexème::MODULO_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OpérateurBinaire::Genre::Reste_Naturel;
            }

            return OpérateurBinaire::Genre::Reste_Relatif;
        }
        case GenreLexème::DECALAGE_DROITE:
        case GenreLexème::DEC_DROITE_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OpérateurBinaire::Genre::Dec_Droite_Logique;
            }

            return OpérateurBinaire::Genre::Dec_Droite_Arithm;
        }
        case GenreLexème::DECALAGE_GAUCHE:
        case GenreLexème::DEC_GAUCHE_EGAL:
        {
            return OpérateurBinaire::Genre::Dec_Gauche;
        }
        case GenreLexème::ESPERLUETTE:
        case GenreLexème::ET_EGAL:
        {
            return OpérateurBinaire::Genre::Et_Binaire;
        }
        case GenreLexème::BARRE:
        case GenreLexème::OU_EGAL:
        {
            return OpérateurBinaire::Genre::Ou_Binaire;
        }
        case GenreLexème::CHAPEAU:
        case GenreLexème::OUX_EGAL:
        {
            return OpérateurBinaire::Genre::Ou_Exclusif;
        }
        case GenreLexème::INFERIEUR:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Comp_Inf_Reel;
            }

            if (type_opérandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OpérateurBinaire::Genre::Comp_Inf_Nat;
            }

            return OpérateurBinaire::Genre::Comp_Inf;
        }
        case GenreLexème::INFERIEUR_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Comp_Inf_Egal_Reel;
            }

            if (type_opérandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OpérateurBinaire::Genre::Comp_Inf_Egal_Nat;
            }

            return OpérateurBinaire::Genre::Comp_Inf_Egal;
        }
        case GenreLexème::SUPERIEUR:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Comp_Sup_Reel;
            }

            if (type_opérandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OpérateurBinaire::Genre::Comp_Sup_Nat;
            }

            return OpérateurBinaire::Genre::Comp_Sup;
        }
        case GenreLexème::SUPERIEUR_EGAL:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Comp_Sup_Egal_Reel;
            }

            if (type_opérandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OpérateurBinaire::Genre::Comp_Sup_Egal_Nat;
            }

            return OpérateurBinaire::Genre::Comp_Sup_Egal;
        }
        case GenreLexème::EGALITE:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Comp_Egal_Reel;
            }
            return OpérateurBinaire::Genre::Comp_Egal;
        }
        case GenreLexème::DIFFÉRENCE:
        {
            if (type_opérandes == IndiceTypeOp::REEL) {
                return OpérateurBinaire::Genre::Comp_Inegal_Reel;
            }
            return OpérateurBinaire::Genre::Comp_Inegal;
        }
        case GenreLexème::CROCHET_OUVRANT:
        {
            return OpérateurBinaire::Genre::Indexage;
        }
        default:
        {
            assert_rappel(false, [=]() {
                dbg() << "Lexème inattendu lors de la résolution du genre d'opérateur : "
                      << chaine_du_genre_de_lexème(genre_lexeme);
            });
            return OpérateurBinaire::Genre::Invalide;
        }
    }
}

static OpérateurUnaire::Genre genre_op_unaire_pour_lexeme(GenreLexème genre_lexeme)
{
    switch (genre_lexeme) {
        case GenreLexème::PLUS_UNAIRE:
        {
            return OpérateurUnaire::Genre::Positif;
        }
        case GenreLexème::MOINS_UNAIRE:
        {
            return OpérateurUnaire::Genre::Négation;
        }
        case GenreLexème::TILDE:
        {
            return OpérateurUnaire::Genre::Négation_Binaire;
        }
        default:
        {
            return OpérateurUnaire::Genre::Invalide;
        }
    }
}

// types comparaisons :
// ==, !=, <, >, <=, =>
static GenreLexème opérateurs_comparaisons[] = {GenreLexème::EGALITE,
                                                GenreLexème::DIFFÉRENCE,
                                                GenreLexème::INFERIEUR,
                                                GenreLexème::SUPERIEUR,
                                                GenreLexème::INFERIEUR_EGAL,
                                                GenreLexème::SUPERIEUR_EGAL};

// types entiers et réels :
// +, -, *, / (assignés +=, -=, /=, *=)
static GenreLexème opérateurs_entiers_réels[] = {
    GenreLexème::PLUS,
    GenreLexème::MOINS,
    GenreLexème::FOIS,
    GenreLexème::DIVISE,
};

// types entiers :
// %, <<, >>, &, |, ^ (assignés %=, <<=, >>=, &, |, ^)
static GenreLexème opérateurs_entiers[] = {GenreLexème::POURCENT,
                                           GenreLexème::DECALAGE_GAUCHE,
                                           GenreLexème::DECALAGE_DROITE,
                                           GenreLexème::ESPERLUETTE,
                                           GenreLexème::BARRE,
                                           GenreLexème::CHAPEAU};

// types entiers unaires :
// + - ~
static GenreLexème opérateurs_entiers_unaires[] = {
    GenreLexème::PLUS_UNAIRE, GenreLexème::MOINS_UNAIRE, GenreLexème::TILDE};

static bool est_commutatif(GenreLexème id)
{
    switch (id) {
        default:
        {
            return false;
        }
        case GenreLexème::PLUS:
        case GenreLexème::FOIS:
        case GenreLexème::EGALITE:
        case GenreLexème::DIFFÉRENCE:
        {
            return true;
        }
    }
}

const char *chaine_pour_genre_op(OpérateurBinaire::Genre genre)
{
#define ENUMERE_GENRE_OPBINAIRE_EX(genre, id, op_code)                                            \
    case OpérateurBinaire::Genre::genre:                                                          \
        return #id;
    switch (genre) {
        ENUMERE_OPERATEURS_BINAIRE;
    }
#undef ENUMERE_GENRE_OPBINAIRE_EX

    return "inconnu";
}

std::ostream &operator<<(std::ostream &os, OpérateurBinaire::Genre genre)
{
    os << chaine_pour_genre_op(genre);
    return os;
}

const char *chaine_pour_genre_op(OpérateurUnaire::Genre genre)
{
#define ENUMERE_GENRE_OPUNAIRE_EX(genre, nom)                                                     \
    case OpérateurUnaire::Genre::genre:                                                           \
        return #nom;
    switch (genre) {
        ENUMERE_OPERATEURS_UNAIRE;
    }
#undef ENUMERE_GENRE_OPUNAIRE_EX

    return "inconnu";
}

std::ostream &operator<<(std::ostream &os, OpérateurUnaire::Genre genre)
{
    os << chaine_pour_genre_op(genre);
    return os;
}

inline int indice_op_binaire(GenreLexème lexeme)
{
    // À FAIRE: l'indice n'est pas bon, nous devrions utiliser le genre pour le bon type de données
    return static_cast<int>(genre_op_binaire_pour_lexeme(lexeme, IndiceTypeOp::ENTIER_NATUREL));
}

inline int indice_op_unaire(GenreLexème lexeme)
{
    return static_cast<int>(genre_op_unaire_pour_lexeme(lexeme));
}

constexpr inline int nombre_genre_op_binaires()
{
    int compte = 0;
#define ENUMERE_GENRE_OPBINAIRE_EX(x, y, z) compte += 1;
    ENUMERE_OPERATEURS_BINAIRE
#undef ENUMERE_GENRE_OPBINAIRE_EX
    return compte;
}

constexpr inline int nombre_genre_op_unaires()
{
    int compte = 0;
#define ENUMERE_GENRE_OPUNAIRE_EX(x, nom) compte += 1;
    ENUMERE_OPERATEURS_UNAIRE
#undef ENUMERE_GENRE_OPUNAIRE_EX
    return compte;
}

void TableOpérateurs::ajoute(GenreLexème lexème, OpérateurBinaire *opérateur)
{
    /* n'alloue de la mémoire que si nous ajoutons un opérateurs */
    opérateurs_.redimensionne(nombre_genre_op_binaires());

    /* nous utilisons le lexème pour indexer afin que les opérateurs dépendants
     * du signe des types soient stockés ensemble */
    opérateurs_[indice_op_binaire(lexème)].ajoute(opérateur);
}

OpérateurBinaire *TableOpérateurs::donne_opérateur(GenreLexème genre_lexème,
                                                   Type *type_opérande_droite) const
{
    auto &liste = this->opérateurs(genre_lexème);

    POUR (liste.plage()) {
        if (it->type2 == type_opérande_droite) {
            return it;
        }
    }

    return nullptr;
}

const TableOpérateurs::type_conteneur &TableOpérateurs::opérateurs(GenreLexème lexeme) const
{
    /* retourne un tableau vide si aucun opérateur n'a été ajouté */
    if (opérateurs_.est_vide()) {
        static type_conteneur tableau_vide;
        return tableau_vide;
    }

    return opérateurs_[indice_op_binaire(lexeme)];
}

int64_t TableOpérateurs::mémoire_utilisée() const
{
    int64_t résultat(0);

    résultat += opérateurs_.taille_mémoire();
    POUR (opérateurs_) {
        résultat += it.taille_mémoire();
    }

    return résultat;
}

RegistreDesOpérateurs::RegistreDesOpérateurs()
{
    opérateurs_unaires.redimensionne(nombre_genre_op_unaires());
    opérateurs_binaires.redimensionne(nombre_genre_op_binaires());
}

TableOpérateurs *RegistreDesOpérateurs::donne_ou_crée_table_opérateurs(Type *type)
{
    if (type->table_opérateurs) {
        return type->table_opérateurs;
    }

    auto résultat = m_tables_opérateurs.ajoute_élément();
    type->table_opérateurs = résultat;
    return résultat;
}

RegistreDesOpérateurs::~RegistreDesOpérateurs() = default;

const RegistreDesOpérateurs::type_conteneur_unaire &RegistreDesOpérateurs::trouve_unaire(
    GenreLexème id) const
{
    return opérateurs_unaires[indice_op_unaire(id)];
}

OpérateurBinaire *RegistreDesOpérateurs::ajoute_basique(GenreLexème id,
                                                        Type *type,
                                                        Type *type_résultat,
                                                        IndiceTypeOp indice_type)
{
    return ajoute_basique(id, type, type, type_résultat, indice_type);
}

OpérateurBinaire *RegistreDesOpérateurs::ajoute_basique(
    GenreLexème id, Type *type1, Type *type2, Type *type_résultat, IndiceTypeOp indice_type)
{
    assert(type1);
    assert(type2);

    auto table = donne_ou_crée_table_opérateurs(type1);

    auto op = opérateurs_binaires[indice_op_binaire(id)].ajoute_élément();
    op->type1 = type1;
    op->type2 = type2;
    op->type_résultat = type_résultat;
    op->est_commutatif = est_commutatif(id);
    op->est_basique = true;
    op->genre = genre_op_binaire_pour_lexeme(id, indice_type);
    table->ajoute(id, op);
    return op;
}

OpérateurUnaire *RegistreDesOpérateurs::ajoute_basique_unaire(GenreLexème id,
                                                              Type *type,
                                                              Type *type_résultat)
{
    auto op = opérateurs_unaires[indice_op_unaire(id)].ajoute_élément();
    op->type_opérande = type;
    op->type_résultat = type_résultat;
    op->est_basique = true;
    op->genre = genre_op_unaire_pour_lexeme(id);
    return op;
}

OpérateurBinaire *RegistreDesOpérateurs::crée_opérateur_binaire(
    GenreLexème id,
    Type *type1,
    Type *type2,
    Type *type_résultat,
    NoeudDéclarationEntêteFonction *decl)
{
    auto résultat = opérateurs_binaires[indice_op_binaire(id)].ajoute_élément();
    résultat->type1 = type1;
    résultat->type2 = type2;
    résultat->type_résultat = type_résultat;
    résultat->est_commutatif = est_commutatif(id);
    résultat->est_basique = false;
    résultat->decl = decl;
    résultat->est_assignation_composée = est_assignation_composée(id);
    résultat->genre = genre_op_binaire_pour_lexeme(id, IndiceTypeOp::ENTIER_NATUREL);
    return résultat;
}

void RegistreDesOpérateurs::ajoute_perso(GenreLexème id,
                                         Type *type1,
                                         Type *type2,
                                         Type *type_résultat,
                                         NoeudDéclarationEntêteFonction *decl)
{
    auto table = donne_ou_crée_table_opérateurs(type1);
    auto op = crée_opérateur_binaire(id, type1, type2, type_résultat, decl);
    table->ajoute(id, op);
    if (id == GenreLexeme::CROCHET_OUVRANT) {
        table->opérateur_indexage = op;
    }

    auto genre_opérateur_symétrique = donne_genre_lexème_pour_opérateur_symétrique(id);
    if (genre_opérateur_symétrique != GenreLexème::INCONNU) {
        crée_opérateur_symétrique(
            table, op, genre_opérateur_symétrique, type1, type2, type_résultat);
    }
}

void RegistreDesOpérateurs::crée_opérateur_symétrique(TableOpérateurs *table,
                                                      OpérateurBinaire *opérateur_source,
                                                      GenreLexème id,
                                                      Type *type1,
                                                      Type *type2,
                                                      Type *type_résultat)
{
    OpérateurBinaire *opérateur_symétrique = table->donne_opérateur(id, type2);
    if (!opérateur_symétrique) {
        opérateur_symétrique = crée_opérateur_binaire(id, type1, type2, type_résultat, nullptr);
        opérateur_symétrique->doit_être_synthétisé_depuis = opérateur_source;
        table->ajoute(id, opérateur_symétrique);
    }
}

void RegistreDesOpérateurs::ajoute_perso_unaire(GenreLexème id,
                                                Type *type,
                                                Type *type_résultat,
                                                NoeudDéclarationEntêteFonction *decl)
{
    auto op = opérateurs_unaires[indice_op_unaire(id)].ajoute_élément();
    op->type_opérande = type;
    op->type_résultat = type_résultat;
    op->est_basique = false;
    op->déclaration = decl;
    op->genre = genre_op_unaire_pour_lexeme(id);
}

void RegistreDesOpérateurs::ajoute_opérateur_basique_enum(Typeuse &typeuse, TypeEnum *type)
{
    auto table = donne_ou_crée_table_opérateurs(type);

    auto indice_type_op = IndiceTypeOp();
    if (type->type_sous_jacent->est_type_entier_naturel()) {
        indice_type_op = IndiceTypeOp::ENTIER_NATUREL;
    }
    else {
        indice_type_op = IndiceTypeOp::ENTIER_RELATIF;
    }

    ajoute_opérateurs_comparaison(typeuse, type, indice_type_op);
    ajoute_opérateurs_entiers(type, indice_type_op);

    table->opérateur_non = this->ajoute_basique_unaire(GenreLexème::TILDE, type, type);
}

void RegistreDesOpérateurs::ajoute_opérateurs_basiques_pointeur(Typeuse &typeuse,
                                                                TypePointeur *type)
{
    auto indice = IndiceTypeOp::ENTIER_RELATIF;

    ajoute_opérateurs_comparaison(typeuse, type, indice);

    /* Pour l'arithmétique de pointeur nous n'utilisons que le type le plus
     * gros, la résolution de l'opérateur ajoutera une transformation afin
     * que le type plus petit soit transtyper à la bonne taille. */
    auto type_entier = typeuse.type_z64;

    ajoute_basique(GenreLexème::PLUS, type, type_entier, type, indice)->est_arithmétique_pointeur =
        true;
    ajoute_basique(GenreLexème::MOINS, type, type_entier, type, indice)
        ->est_arithmétique_pointeur = true;
    ajoute_basique(GenreLexème::MOINS, type, type, type_entier, indice)
        ->est_arithmétique_pointeur = true;
    ajoute_basique(GenreLexème::PLUS_EGAL, type, type_entier, type, indice)
        ->est_arithmétique_pointeur = true;
    ajoute_basique(GenreLexème::MOINS_EGAL, type, type_entier, type, indice)
        ->est_arithmétique_pointeur = true;

    type_entier = typeuse.type_n64;
    indice = IndiceTypeOp::ENTIER_NATUREL;

    ajoute_basique(GenreLexème::PLUS, type, type_entier, type, indice)->est_arithmétique_pointeur =
        true;
    ajoute_basique(GenreLexème::MOINS, type, type_entier, type, indice)
        ->est_arithmétique_pointeur = true;
    ajoute_basique(GenreLexème::PLUS_EGAL, type, type_entier, type, indice)
        ->est_arithmétique_pointeur = true;
    ajoute_basique(GenreLexème::MOINS_EGAL, type, type_entier, type, indice)
        ->est_arithmétique_pointeur = true;
}

void RegistreDesOpérateurs::ajoute_opérateurs_basiques_fonction(Typeuse &typeuse,
                                                                TypeFonction *type)
{
    auto indice = IndiceTypeOp::ENTIER_RELATIF;

    auto const &type_bool = typeuse.type_bool;

    ajoute_basique(GenreLexème::EGALITE, type, type_bool, indice);
    ajoute_basique(GenreLexème::DIFFÉRENCE, type, type_bool, indice);
}

void RegistreDesOpérateurs::rassemble_statistiques(Statistiques &stats) const
{
    auto nombre_unaires = int64_t(0);
    auto memoire_unaires = opérateurs_unaires.taille_mémoire();

    POUR (opérateurs_unaires) {
        memoire_unaires += it.mémoire_utilisée();
        nombre_unaires += it.taille();
    }

    auto nombre_binaires = int64_t(0);
    auto memoire_binaires = opérateurs_binaires.taille_mémoire();

    POUR (opérateurs_binaires) {
        memoire_binaires += it.mémoire_utilisée();
        nombre_binaires += it.taille();
    }

    auto nombre_tables = m_tables_opérateurs.taille();
    auto mémoire_tables = m_tables_opérateurs.mémoire_utilisée();

    POUR_TABLEAU_PAGE (m_tables_opérateurs) {
        mémoire_tables += it.mémoire_utilisée();
    }

    auto &stats_ops = stats.stats_opérateurs;
    stats_ops.fusionne_entrée({"OpérateurUnaire", nombre_unaires, memoire_unaires});
    stats_ops.fusionne_entrée({"OpérateurBinaire", nombre_binaires, memoire_binaires});
    stats_ops.fusionne_entrée({"TableOpérateurs", nombre_tables, mémoire_tables});
}

void RegistreDesOpérateurs::ajoute_opérateurs_comparaison(Typeuse &typeuse,
                                                          Type *pour_type,
                                                          IndiceTypeOp indice)
{
    auto table = donne_ou_crée_table_opérateurs(pour_type);
    for (auto op : opérateurs_comparaisons) {
        auto op_bin = this->ajoute_basique(op, pour_type, typeuse.type_bool, indice);

        if (op == GenreLexème::SUPERIEUR) {
            table->opérateur_sup = op_bin;
        }
        else if (op == GenreLexème::SUPERIEUR_EGAL) {
            table->opérateur_seg = op_bin;
        }
        else if (op == GenreLexème::INFERIEUR) {
            table->opérateur_inf = op_bin;
        }
        else if (op == GenreLexème::INFERIEUR_EGAL) {
            table->opérateur_ieg = op_bin;
        }
        else if (op == GenreLexème::EGALITE) {
            table->opérateur_egt = op_bin;
        }
        else if (op == GenreLexème::DIFFÉRENCE) {
            table->opérateur_dif = op_bin;
        }
    }
}

void RegistreDesOpérateurs::ajoute_opérateurs_entiers_réel(Type *pour_type, IndiceTypeOp indice)
{
    auto table = donne_ou_crée_table_opérateurs(pour_type);
    for (auto op : opérateurs_entiers_réels) {
        auto op_bin = this->ajoute_basique(op, pour_type, pour_type, indice);

        if (op == GenreLexème::PLUS) {
            table->opérateur_ajt = op_bin;
        }
        else if (op == GenreLexème::MOINS) {
            table->opérateur_sst = op_bin;
        }
        else if (op == GenreLexème::FOIS) {
            table->opérateur_mul = op_bin;
        }
        else if (op == GenreLexème::DIVISE) {
            table->opérateur_div = op_bin;
        }
    }
}

void RegistreDesOpérateurs::ajoute_opérateurs_entiers(Type *pour_type, IndiceTypeOp indice)
{
    auto table = donne_ou_crée_table_opérateurs(pour_type);
    for (auto op : opérateurs_entiers) {
        auto ptr_op = this->ajoute_basique(op, pour_type, pour_type, indice);

        if (op == GenreLexème::ESPERLUETTE) {
            table->opérateur_etb = ptr_op;
        }
        else if (op == GenreLexème::BARRE) {
            table->opérateur_oub = ptr_op;
        }
    }
}

void RegistreDesOpérateurs::ajoute_opérateurs_entiers_unaires(Type *pour_type)
{
    for (auto op : opérateurs_entiers_unaires) {
        this->ajoute_basique_unaire(op, pour_type, pour_type);
    }
}

void RegistreDesOpérateurs::ajoute_opérateurs_basiques_au_besoin(Typeuse &typeuse, Type *type)
{
    if (!(type->est_type_pointeur() || type->est_type_fonction())) {
        return;
    }

    if (type->possède_drapeau(DrapeauxTypes::TYPE_POSSEDE_OPERATEURS_DE_BASE)) {
        return;
    }

    if (type->est_type_pointeur()) {
        auto type_pointeur = type->comme_type_pointeur();
        ajoute_opérateurs_basiques_pointeur(typeuse, type_pointeur);
    }
    else {
        auto type_fonction = type->comme_type_fonction();
        ajoute_opérateurs_basiques_fonction(typeuse, type_fonction);
    }

    type->drapeaux_type |= DrapeauxTypes::TYPE_POSSEDE_OPERATEURS_DE_BASE;
}

static bool est_polymorphique(OpérateurBinaire const *opérateur)
{
    if (!opérateur->decl) {
        return false;
    }
    return opérateur->decl->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE);
}

static void rassemble_opérateurs_pour_type(Type const &type,
                                           GenreLexème const type_op,
                                           kuri::tablet<OpérateurBinaire const *, 10> &résultat)
{
    if (!type.table_opérateurs) {
        return;
    }

    POUR (type.table_opérateurs->opérateurs(type_op).plage()) {
        résultat.ajoute(it);
    }
}

static void rassemble_opérateurs_pour_type(
    Type *type, GenreLexème type_op, kuri::tablet<OpérateurBinaire const *, 10> &op_candidats)
{
    rassemble_opérateurs_pour_type(*type, type_op, op_candidats);

    if (type->est_type_opaque()) {
        auto type_opacifié = type->comme_type_opaque()->type_opacifié;
        rassemble_opérateurs_pour_type(*type_opacifié, type_op, op_candidats);
    }

    auto polymorphe_type = donne_polymorphe_de_base(type);
    if (polymorphe_type) {
        rassemble_opérateurs_pour_type(*polymorphe_type, type_op, op_candidats);
    }
}

std::optional<Attente> cherche_candidats_opérateurs(EspaceDeTravail &espace,
                                                    NoeudExpression *expression,
                                                    Type *type1,
                                                    Type *type2,
                                                    GenreLexème type_op,
                                                    kuri::tablet<OpérateurCandidat, 10> &candidats)
{
    assert(type1);
    assert(type2);

    auto op_candidats = kuri::tablet<OpérateurBinaire const *, 10>();
    rassemble_opérateurs_pour_type(type1, type_op, op_candidats);

    if (type1 != type2) {
        rassemble_opérateurs_pour_type(type2, type_op, op_candidats);
    }

    for (auto const op : op_candidats) {
        if (est_polymorphique(op)) {
            Monomorpheuse monomorpheuse(espace, op->decl);

            if (!expression->est_référence_opérateur_binaire() &&
                !expression->est_expression_binaire()) {
                espace.rapporte_erreur(
                    expression,
                    "Erreur interne : le site de monomorphisation d'une expression binaire n'est "
                    "pas une référence à un opérateur ou une expression binaire.");
                break;
            }

            kuri::tablet<NoeudExpression *, 2> slots{};

            if (expression->est_expression_binaire()) {
                auto expression_binaire = expression->comme_expression_binaire();
                slots.ajoute(expression_binaire->opérande_gauche);
                slots.ajoute(expression_binaire->opérande_droite);
            }
            else {
                auto expression_binaire = expression->comme_référence_opérateur_binaire();
                slots.ajoute(expression_binaire->opérande_gauche);
                slots.ajoute(expression_binaire->opérande_droite);
            }

            auto résultat_monomorphisation = détermine_monomorphisation(
                monomorpheuse, op->decl, slots);
            if (std::holds_alternative<Attente>(résultat_monomorphisation)) {
                return std::get<Attente>(résultat_monomorphisation);
            }
            if (std::holds_alternative<ErreurMonomorphisation>(résultat_monomorphisation)) {
                continue;
            }

            auto arg_gauche = op->decl->parametre_entree(0);
            auto arg_droite = op->decl->parametre_entree(1);

            static constexpr double POIDS_POUR_ARGUMENT_POLYMORPHIQUE = 0.95;
            auto poids1 = 0.0;
            auto poids2 = 0.0;
            TransformationType transformation_type1{};
            TransformationType transformation_type2{};

            if (arg_gauche->type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
                auto résultat_type = monomorpheuse.résoud_type_final(arg_gauche->expression_type);
                if (std::holds_alternative<ErreurMonomorphisation>(résultat_type)) {
                    continue;
                }

                auto type_apparié_pesé = std::get<TypeAppariéPesé>(résultat_type);
                poids1 = type_apparié_pesé.poids_appariement * POIDS_POUR_ARGUMENT_POLYMORPHIQUE;
            }
            else {
                auto poids1_ou_attente = vérifie_compatibilité(op->type1, type1, true);
                if (std::holds_alternative<Attente>(poids1_ou_attente)) {
                    return std::get<Attente>(poids1_ou_attente);
                }
                auto info_poids = std::get<PoidsTransformation>(poids1_ou_attente);
                poids1 = info_poids.poids;
                transformation_type1 = info_poids.transformation;
            }

            if (arg_droite->type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
                auto résultat_type = monomorpheuse.résoud_type_final(arg_droite->expression_type);
                if (std::holds_alternative<ErreurMonomorphisation>(résultat_type)) {
                    continue;
                }

                auto type_apparié_pesé = std::get<TypeAppariéPesé>(résultat_type);
                poids2 = type_apparié_pesé.poids_appariement * POIDS_POUR_ARGUMENT_POLYMORPHIQUE;
            }
            else {
                auto poids2_ou_attente = vérifie_compatibilité(op->type2, type2, true);
                if (std::holds_alternative<Attente>(poids2_ou_attente)) {
                    return std::get<Attente>(poids2_ou_attente);
                }
                auto info_poids = std::get<PoidsTransformation>(poids2_ou_attente);
                poids2 = info_poids.poids;
                transformation_type2 = info_poids.transformation;
            }

            /* Nous ajoutons également les opérateurs ayant un poids de 0 afin de pouvoir donner
             * des détails dans les messages d'erreurs. */
            auto candidat = OpérateurCandidat{};
            candidat.op = op;
            candidat.poids = poids1 * poids2;
            candidat.transformation_type1 = transformation_type1;
            candidat.transformation_type2 = transformation_type2;
            copie_tablet_tableau(monomorpheuse.résultat_pour_monomorphisation(),
                                 candidat.items_monomorphisation);

            candidats.ajoute(candidat);
            continue;
        }

        auto poids1_ou_attente = vérifie_compatibilité(op->type1, type1, true);

        if (std::holds_alternative<Attente>(poids1_ou_attente)) {
            return std::get<Attente>(poids1_ou_attente);
        }

        auto poids1 = std::get<PoidsTransformation>(poids1_ou_attente);

        auto poids2_ou_attente = vérifie_compatibilité(op->type2, type2, true);

        if (std::holds_alternative<Attente>(poids2_ou_attente)) {
            return std::get<Attente>(poids2_ou_attente);
        }

        auto poids2 = std::get<PoidsTransformation>(poids2_ou_attente);

        auto poids = poids1.poids * poids2.poids;

        /* Nous ajoutons également les opérateurs ayant un poids de 0 afin de pouvoir donner des
         * détails dans les messages d'erreurs. */
        auto candidat = OpérateurCandidat{};
        candidat.op = op;
        candidat.poids = poids;
        candidat.transformation_type1 = poids1.transformation;
        candidat.transformation_type2 = poids2.transformation;

        candidats.ajoute(candidat);

        if (op->est_commutatif && poids != 1.0) {
            auto poids3_ou_attente = vérifie_compatibilité(op->type1, type2, true);

            if (std::holds_alternative<Attente>(poids3_ou_attente)) {
                return std::get<Attente>(poids3_ou_attente);
            }

            auto poids3 = std::get<PoidsTransformation>(poids3_ou_attente);

            auto poids4_ou_attente = vérifie_compatibilité(op->type2, type1, true);

            if (std::holds_alternative<Attente>(poids4_ou_attente)) {
                return std::get<Attente>(poids4_ou_attente);
            }

            auto poids4 = std::get<PoidsTransformation>(poids4_ou_attente);

            poids = poids3.poids * poids4.poids;

            candidat.op = op;
            candidat.poids = poids;
            // N'oublions pas de permuter les transformations.
            candidat.transformation_type1 = poids4.transformation;
            candidat.transformation_type2 = poids3.transformation;
            candidat.permute_opérandes = true;

            candidats.ajoute(candidat);
        }
    }

    /* Réduisons les poids des opérateurs simples si nous avons une assignation composée, afin de
     * privilégier les opérateurs d'assignation composée. */
    auto const pour_assignation_composée = est_assignation_composée(type_op);
    POUR (candidats) {
        if (!it.op->est_assignation_composée && pour_assignation_composée) {
            it.poids *= 0.5;
        }
    }

    return {};
}

static Attente attente_sur_opérateur_ou_type(NoeudExpression *noeud, Type *type1, Type *type2)
{
    auto est_énum_ou_référence_énum = [](Type *t) -> TypeEnum * {
        if (t->est_type_énum()) {
            return t->comme_type_énum();
        }

        if (t->est_type_référence() && t->comme_type_référence()->type_pointé->est_type_énum()) {
            return t->comme_type_référence()->type_pointé->comme_type_énum();
        }

        return nullptr;
    };

    auto type1_est_énum = est_énum_ou_référence_énum(type1);
    if (type1_est_énum &&
        !type1_est_énum->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return Attente::sur_type(type1_est_énum);
    }
    auto type2_est_énum = est_énum_ou_référence_énum(type2);
    if (type2_est_énum &&
        !type2_est_énum->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return Attente::sur_type(type2_est_énum);
    }
    return Attente::sur_opérateur(noeud);
}

RésultatRechercheOpérateur trouve_opérateur_pour_expression(
    Contexte *contexte, NoeudExpression *site, Type *type1, Type *type2, GenreLexème type_op)
{
    auto &espace = *contexte->espace;
    auto &typeuse = espace.typeuse;
    auto &registre = espace.opérateurs;
    registre->ajoute_opérateurs_basiques_au_besoin(typeuse, type1);
    registre->ajoute_opérateurs_basiques_au_besoin(typeuse, type2);

    auto candidats = kuri::tablet<OpérateurCandidat, 10>();
    auto attente_potentielle = cherche_candidats_opérateurs(
        espace, site, type1, type2, type_op, candidats);
    if (attente_potentielle.has_value()) {
        return attente_potentielle.value();
    }

    auto meilleur_candidat = OpérateurCandidat::nul();
    auto poids = 0.0;

    for (auto &candidat : candidats) {
        if (candidat.poids > poids) {
            poids = candidat.poids;
            meilleur_candidat = &candidat;
        }
    }

    if (meilleur_candidat == nullptr) {
        if (site) {
            return attente_sur_opérateur_ou_type(site, type1, type2);
        }

        /* Pour les erreurs dans les discriminations... */
        return false;
    }

    if (est_polymorphique(meilleur_candidat->op)) {
        auto [noeud_decl, doit_monomorpher] = monomorphise_au_besoin(
            contexte,
            meilleur_candidat->op->decl,
            site,
            std::move(meilleur_candidat->items_monomorphisation));

        if (doit_monomorpher ||
            !noeud_decl->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
            return Attente::sur_déclaration(noeud_decl);
        }
    }
    else if (meilleur_candidat->op->doit_être_synthétisé_depuis && !meilleur_candidat->op->decl) {
        return Attente::sur_synthétisation_opérateur(meilleur_candidat->op);
    }

    return *meilleur_candidat;
}

const OpérateurUnaire *cherche_opérateur_unaire(RegistreDesOpérateurs const &opérateurs,
                                                Type *type1,
                                                GenreLexème type_op)
{
    auto &iter = opérateurs.trouve_unaire(type_op);

    for (auto i = 0; i < iter.taille(); ++i) {
        auto op = &iter[i];

        if (op->type_opérande == type1) {
            return op;
        }
    }

    return nullptr;
}

void enregistre_opérateurs_basiques(Typeuse &typeuse, RegistreDesOpérateurs &registre)
{
    auto type_entier_constant = typeuse.type_entier_constant;
    auto type_octet = typeuse.type_octet;

    Type *types_entiers_naturels[] = {
        typeuse.type_n8,
        typeuse.type_n16,
        typeuse.type_n32,
        typeuse.type_n64,
        type_entier_constant,
    };

    Type *types_entiers_relatifs[] = {
        typeuse.type_z8,
        typeuse.type_z16,
        typeuse.type_z32,
        typeuse.type_z64,
        type_octet,
    };

    auto type_r32 = typeuse.type_r32;
    auto type_r64 = typeuse.type_r64;

    Type *types_réels[] = {type_r32, type_r64};

    auto type_bool = typeuse.type_bool;

    for (auto type : types_entiers_relatifs) {
        registre.ajoute_opérateurs_comparaison(typeuse, type, IndiceTypeOp::ENTIER_RELATIF);
        registre.ajoute_opérateurs_entiers_réel(type, IndiceTypeOp::ENTIER_RELATIF);
        registre.ajoute_opérateurs_entiers(type, IndiceTypeOp::ENTIER_RELATIF);
        registre.ajoute_opérateurs_entiers_unaires(type);
    }

    for (auto type : types_entiers_naturels) {
        registre.ajoute_opérateurs_comparaison(typeuse, type, IndiceTypeOp::ENTIER_NATUREL);
        registre.ajoute_opérateurs_entiers_réel(type, IndiceTypeOp::ENTIER_NATUREL);
        registre.ajoute_opérateurs_entiers(type, IndiceTypeOp::ENTIER_NATUREL);
        registre.ajoute_opérateurs_entiers_unaires(type);
    }

    for (auto type : types_réels) {
        registre.ajoute_opérateurs_comparaison(typeuse, type, IndiceTypeOp::REEL);
        registre.ajoute_opérateurs_entiers_réel(type, IndiceTypeOp::REEL);

        // opérateurs unaires + -
        registre.ajoute_basique_unaire(GenreLexème::PLUS_UNAIRE, type, type);
        registre.ajoute_basique_unaire(GenreLexème::MOINS_UNAIRE, type, type);
    }

    // opérateurs booléens & | ^ == !=
    registre.ajoute_basique(
        GenreLexème::CHAPEAU, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    registre.ajoute_basique(
        GenreLexème::ESPERLUETTE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    registre.ajoute_basique(
        GenreLexème::BARRE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    registre.ajoute_basique(
        GenreLexème::EGALITE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    registre.ajoute_basique(
        GenreLexème::DIFFÉRENCE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);

    auto type_type_de_données = typeuse.type_type_de_donnees_;

    registre.op_comp_égal_types = registre.ajoute_basique(
        GenreLexème::EGALITE, type_type_de_données, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    registre.op_comp_diff_types = registre.ajoute_basique(
        GenreLexème::DIFFÉRENCE, type_type_de_données, type_bool, IndiceTypeOp::ENTIER_NATUREL);
}

kuri::chaine_statique donne_chaine_lexème_pour_op_binaire(OpérateurBinaire::Genre op)
{
    switch (op) {
        case OpérateurBinaire::Genre::Addition:
        case OpérateurBinaire::Genre::Addition_Reel:
        {
            return "+";
        }
        case OpérateurBinaire::Genre::Soustraction:
        case OpérateurBinaire::Genre::Soustraction_Reel:
        {
            return "-";
        }
        case OpérateurBinaire::Genre::Multiplication:
        case OpérateurBinaire::Genre::Multiplication_Reel:
        {
            return "*";
        }
        case OpérateurBinaire::Genre::Division_Naturel:
        case OpérateurBinaire::Genre::Division_Relatif:
        case OpérateurBinaire::Genre::Division_Reel:
        {
            return "/";
        }
        case OpérateurBinaire::Genre::Reste_Naturel:
        case OpérateurBinaire::Genre::Reste_Relatif:
        {
            return "%";
        }
        case OpérateurBinaire::Genre::Comp_Egal:
        case OpérateurBinaire::Genre::Comp_Egal_Reel:
        {
            return "==";
        }
        case OpérateurBinaire::Genre::Comp_Inegal:
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        {
            return "!=";
        }
        case OpérateurBinaire::Genre::Comp_Inf:
        case OpérateurBinaire::Genre::Comp_Inf_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        {
            return "<";
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        {
            return "<=";
        }
        case OpérateurBinaire::Genre::Comp_Sup:
        case OpérateurBinaire::Genre::Comp_Sup_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        {
            return ">";
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            return ">=";
        }
        case OpérateurBinaire::Genre::Et_Binaire:
        {
            return "&";
        }
        case OpérateurBinaire::Genre::Ou_Binaire:
        {
            return "|";
        }
        case OpérateurBinaire::Genre::Ou_Exclusif:
        {
            return "^";
        }
        case OpérateurBinaire::Genre::Dec_Gauche:
        {
            return "<<";
        }
        case OpérateurBinaire::Genre::Dec_Droite_Arithm:
        case OpérateurBinaire::Genre::Dec_Droite_Logique:
        {
            return ">>";
        }
        case OpérateurBinaire::Genre::Invalide:
        case OpérateurBinaire::Genre::Indexage:
        {
            return "invalide";
        }
    }

    return "invalide";
}

bool est_opérateur_comparaison(OpérateurBinaire::Genre const genre)
{
    switch (genre) {
        case OpérateurBinaire::Genre::Soustraction:
        case OpérateurBinaire::Genre::Soustraction_Reel:
        case OpérateurBinaire::Genre::Division_Naturel:
        case OpérateurBinaire::Genre::Division_Relatif:
        case OpérateurBinaire::Genre::Division_Reel:
        case OpérateurBinaire::Genre::Reste_Naturel:
        case OpérateurBinaire::Genre::Reste_Relatif:
        case OpérateurBinaire::Genre::Dec_Gauche:
        case OpérateurBinaire::Genre::Dec_Droite_Arithm:
        case OpérateurBinaire::Genre::Dec_Droite_Logique:
        case OpérateurBinaire::Genre::Invalide:
        case OpérateurBinaire::Genre::Indexage:
        case OpérateurBinaire::Genre::Addition:
        case OpérateurBinaire::Genre::Addition_Reel:
        case OpérateurBinaire::Genre::Multiplication:
        case OpérateurBinaire::Genre::Multiplication_Reel:
        case OpérateurBinaire::Genre::Et_Binaire:
        case OpérateurBinaire::Genre::Ou_Binaire:
        case OpérateurBinaire::Genre::Ou_Exclusif:
        {
            return false;
        }
        case OpérateurBinaire::Genre::Comp_Egal:
        case OpérateurBinaire::Genre::Comp_Egal_Reel:
        case OpérateurBinaire::Genre::Comp_Inegal:
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        case OpérateurBinaire::Genre::Comp_Inf:
        case OpérateurBinaire::Genre::Comp_Inf_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        case OpérateurBinaire::Genre::Comp_Inf_Egal:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        case OpérateurBinaire::Genre::Comp_Sup:
        case OpérateurBinaire::Genre::Comp_Sup_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        case OpérateurBinaire::Genre::Comp_Sup_Egal:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            return true;
        }
    }

    return false;
}

bool est_opérateur_comparaison_ordre(OpérateurBinaire::Genre const genre)
{
    switch (genre) {
        case OpérateurBinaire::Genre::Soustraction:
        case OpérateurBinaire::Genre::Soustraction_Reel:
        case OpérateurBinaire::Genre::Division_Naturel:
        case OpérateurBinaire::Genre::Division_Relatif:
        case OpérateurBinaire::Genre::Division_Reel:
        case OpérateurBinaire::Genre::Reste_Naturel:
        case OpérateurBinaire::Genre::Reste_Relatif:
        case OpérateurBinaire::Genre::Dec_Gauche:
        case OpérateurBinaire::Genre::Dec_Droite_Arithm:
        case OpérateurBinaire::Genre::Dec_Droite_Logique:
        case OpérateurBinaire::Genre::Invalide:
        case OpérateurBinaire::Genre::Indexage:
        case OpérateurBinaire::Genre::Addition:
        case OpérateurBinaire::Genre::Addition_Reel:
        case OpérateurBinaire::Genre::Multiplication:
        case OpérateurBinaire::Genre::Multiplication_Reel:
        case OpérateurBinaire::Genre::Et_Binaire:
        case OpérateurBinaire::Genre::Ou_Binaire:
        case OpérateurBinaire::Genre::Ou_Exclusif:
        case OpérateurBinaire::Genre::Comp_Egal:
        case OpérateurBinaire::Genre::Comp_Egal_Reel:
        case OpérateurBinaire::Genre::Comp_Inegal:
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        {
            return false;
        }
        case OpérateurBinaire::Genre::Comp_Inf:
        case OpérateurBinaire::Genre::Comp_Inf_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        case OpérateurBinaire::Genre::Comp_Inf_Egal:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        case OpérateurBinaire::Genre::Comp_Sup:
        case OpérateurBinaire::Genre::Comp_Sup_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        case OpérateurBinaire::Genre::Comp_Sup_Egal:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            return true;
        }
    }

    return false;
}

bool peut_permuter_opérandes(OpérateurBinaire::Genre const genre)
{
    switch (genre) {
        case OpérateurBinaire::Genre::Soustraction:
        case OpérateurBinaire::Genre::Soustraction_Reel:
        case OpérateurBinaire::Genre::Division_Naturel:
        case OpérateurBinaire::Genre::Division_Relatif:
        case OpérateurBinaire::Genre::Division_Reel:
        case OpérateurBinaire::Genre::Reste_Naturel:
        case OpérateurBinaire::Genre::Reste_Relatif:
        case OpérateurBinaire::Genre::Dec_Gauche:
        case OpérateurBinaire::Genre::Dec_Droite_Arithm:
        case OpérateurBinaire::Genre::Dec_Droite_Logique:
        case OpérateurBinaire::Genre::Invalide:
        case OpérateurBinaire::Genre::Indexage:
        {
            return false;
        }
        case OpérateurBinaire::Genre::Addition:
        case OpérateurBinaire::Genre::Addition_Reel:
        case OpérateurBinaire::Genre::Multiplication:
        case OpérateurBinaire::Genre::Multiplication_Reel:
        case OpérateurBinaire::Genre::Comp_Egal:
        case OpérateurBinaire::Genre::Comp_Egal_Reel:
        case OpérateurBinaire::Genre::Comp_Inegal:
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        case OpérateurBinaire::Genre::Comp_Inf:
        case OpérateurBinaire::Genre::Comp_Inf_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        case OpérateurBinaire::Genre::Comp_Inf_Egal:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        case OpérateurBinaire::Genre::Comp_Sup:
        case OpérateurBinaire::Genre::Comp_Sup_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        case OpérateurBinaire::Genre::Comp_Sup_Egal:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        case OpérateurBinaire::Genre::Et_Binaire:
        case OpérateurBinaire::Genre::Ou_Binaire:
        case OpérateurBinaire::Genre::Ou_Exclusif:
        {
            return true;
        }
    }

    return false;
}

OpérateurBinaire::Genre donne_opérateur_pour_permutation_opérandes(
    OpérateurBinaire::Genre const genre)
{
    switch (genre) {
        case OpérateurBinaire::Genre::Soustraction:
        case OpérateurBinaire::Genre::Soustraction_Reel:
        case OpérateurBinaire::Genre::Division_Naturel:
        case OpérateurBinaire::Genre::Division_Relatif:
        case OpérateurBinaire::Genre::Division_Reel:
        case OpérateurBinaire::Genre::Reste_Naturel:
        case OpérateurBinaire::Genre::Reste_Relatif:
        case OpérateurBinaire::Genre::Dec_Gauche:
        case OpérateurBinaire::Genre::Dec_Droite_Arithm:
        case OpérateurBinaire::Genre::Dec_Droite_Logique:
        case OpérateurBinaire::Genre::Invalide:
        case OpérateurBinaire::Genre::Indexage:
        {
            return OpérateurBinaire::Genre::Invalide;
        }
        case OpérateurBinaire::Genre::Addition:
        case OpérateurBinaire::Genre::Addition_Reel:
        case OpérateurBinaire::Genre::Multiplication:
        case OpérateurBinaire::Genre::Multiplication_Reel:
        case OpérateurBinaire::Genre::Comp_Egal:
        case OpérateurBinaire::Genre::Comp_Egal_Reel:
        case OpérateurBinaire::Genre::Comp_Inegal:
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        case OpérateurBinaire::Genre::Et_Binaire:
        case OpérateurBinaire::Genre::Ou_Binaire:
        case OpérateurBinaire::Genre::Ou_Exclusif:
        {
            return genre;
        }
        case OpérateurBinaire::Genre::Comp_Inf:
        {
            return OpérateurBinaire::Genre::Comp_Sup;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Nat:
        {
            return OpérateurBinaire::Genre::Comp_Sup_Nat;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        {
            return OpérateurBinaire::Genre::Comp_Sup_Reel;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal:
        {
            return OpérateurBinaire::Genre::Comp_Sup_Egal;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
        {
            return OpérateurBinaire::Genre::Comp_Sup_Egal_Nat;
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        {
            return OpérateurBinaire::Genre::Comp_Sup_Egal_Reel;
        }
        case OpérateurBinaire::Genre::Comp_Sup:
        {
            return OpérateurBinaire::Genre::Comp_Inf;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Nat:
        {
            return OpérateurBinaire::Genre::Comp_Inf_Nat;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        {
            return OpérateurBinaire::Genre::Comp_Inf_Reel;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal:
        {
            return OpérateurBinaire::Genre::Comp_Inf_Egal;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
        {
            return OpérateurBinaire::Genre::Comp_Inf_Egal_Nat;
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            return OpérateurBinaire::Genre::Comp_Inf_Egal_Reel;
        }
    }

    return OpérateurBinaire::Genre::Invalide;
}

GenreLexème donne_genre_lexème_pour_opérateur_symétrique(GenreLexème genre)
{
    if (genre == GenreLexème::INFERIEUR) {
        return GenreLexème::SUPERIEUR_EGAL;
    }
    if (genre == GenreLexème::INFERIEUR_EGAL) {
        return GenreLexème::SUPERIEUR;
    }
    else if (genre == GenreLexème::SUPERIEUR) {
        return GenreLexème::INFERIEUR_EGAL;
    }
    else if (genre == GenreLexème::SUPERIEUR_EGAL) {
        return GenreLexème::INFERIEUR;
    }
    if (genre == GenreLexème::EGALITE) {
        return GenreLexème::DIFFÉRENCE;
    }
    if (genre == GenreLexème::DIFFÉRENCE) {
        return GenreLexème::EGALITE;
    }
    return GenreLexème::INCONNU;
}

kuri::chaine_statique donne_chaine_lexème_pour_op_binaire(GenreLexème genre)
{
    return donne_chaine_lexème_pour_op_binaire(
        genre_op_binaire_pour_lexeme(genre, IndiceTypeOp::ENTIER_NATUREL));
}
