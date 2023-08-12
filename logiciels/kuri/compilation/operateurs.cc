/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "operateurs.hh"

#include "biblinternes/outils/assert.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "statistiques/statistiques.hh"

#include "espace_de_travail.hh"
#include "typage.hh"
#include "unite_compilation.hh"
#include "validation_semantique.hh"

static OperateurBinaire::Genre genre_op_binaire_pour_lexeme(GenreLexeme genre_lexeme,
                                                            IndiceTypeOp type_operandes)
{
    switch (genre_lexeme) {
        case GenreLexeme::PLUS:
        case GenreLexeme::PLUS_EGAL:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Addition_Reel;
            }

            return OperateurBinaire::Genre::Addition;
        }
        case GenreLexeme::MOINS:
        case GenreLexeme::MOINS_EGAL:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Soustraction_Reel;
            }

            return OperateurBinaire::Genre::Soustraction;
        }
        case GenreLexeme::FOIS:
        case GenreLexeme::MULTIPLIE_EGAL:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Multiplication_Reel;
            }

            return OperateurBinaire::Genre::Multiplication;
        }
        case GenreLexeme::DIVISE:
        case GenreLexeme::DIVISE_EGAL:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Division_Reel;
            }

            if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OperateurBinaire::Genre::Division_Naturel;
            }

            return OperateurBinaire::Genre::Division_Relatif;
        }
        case GenreLexeme::POURCENT:
        case GenreLexeme::MODULO_EGAL:
        {
            if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OperateurBinaire::Genre::Reste_Naturel;
            }

            return OperateurBinaire::Genre::Reste_Relatif;
        }
        case GenreLexeme::DECALAGE_DROITE:
        case GenreLexeme::DEC_DROITE_EGAL:
        {
            if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OperateurBinaire::Genre::Dec_Droite_Logique;
            }

            return OperateurBinaire::Genre::Dec_Droite_Arithm;
        }
        case GenreLexeme::DECALAGE_GAUCHE:
        case GenreLexeme::DEC_GAUCHE_EGAL:
        {
            return OperateurBinaire::Genre::Dec_Gauche;
        }
        case GenreLexeme::ESPERLUETTE:
        case GenreLexeme::ET_EGAL:
        {
            return OperateurBinaire::Genre::Et_Binaire;
        }
        case GenreLexeme::BARRE:
        case GenreLexeme::OU_EGAL:
        {
            return OperateurBinaire::Genre::Ou_Binaire;
        }
        case GenreLexeme::CHAPEAU:
        case GenreLexeme::OUX_EGAL:
        {
            return OperateurBinaire::Genre::Ou_Exclusif;
        }
        case GenreLexeme::INFERIEUR:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Comp_Inf_Reel;
            }

            if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OperateurBinaire::Genre::Comp_Inf_Nat;
            }

            return OperateurBinaire::Genre::Comp_Inf;
        }
        case GenreLexeme::INFERIEUR_EGAL:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Comp_Inf_Egal_Reel;
            }

            if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OperateurBinaire::Genre::Comp_Inf_Egal_Nat;
            }

            return OperateurBinaire::Genre::Comp_Inf_Egal;
        }
        case GenreLexeme::SUPERIEUR:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Comp_Sup_Reel;
            }

            if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OperateurBinaire::Genre::Comp_Sup_Nat;
            }

            return OperateurBinaire::Genre::Comp_Sup;
        }
        case GenreLexeme::SUPERIEUR_EGAL:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Comp_Sup_Egal_Reel;
            }

            if (type_operandes == IndiceTypeOp::ENTIER_NATUREL) {
                return OperateurBinaire::Genre::Comp_Sup_Egal_Nat;
            }

            return OperateurBinaire::Genre::Comp_Sup_Egal;
        }
        case GenreLexeme::EGALITE:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Comp_Egal_Reel;
            }
            return OperateurBinaire::Genre::Comp_Egal;
        }
        case GenreLexeme::DIFFERENCE:
        {
            if (type_operandes == IndiceTypeOp::REEL) {
                return OperateurBinaire::Genre::Comp_Inegal_Reel;
            }
            return OperateurBinaire::Genre::Comp_Inegal;
        }
        case GenreLexeme::CROCHET_OUVRANT:
        {
            return OperateurBinaire::Genre::Indexage;
        }
        default:
        {
            assert_rappel(false, [=]() {
                std::cerr << "Lexème inattendu lors de la résolution du genre d'opérateur : "
                          << chaine_du_genre_de_lexeme(genre_lexeme) << '\n';
            });
            return OperateurBinaire::Genre::Invalide;
        }
    }
}

static OperateurUnaire::Genre genre_op_unaire_pour_lexeme(GenreLexeme genre_lexeme)
{
    switch (genre_lexeme) {
        case GenreLexeme::PLUS_UNAIRE:
        {
            return OperateurUnaire::Genre::Positif;
        }
        case GenreLexeme::MOINS_UNAIRE:
        {
            return OperateurUnaire::Genre::Complement;
        }
        case GenreLexeme::TILDE:
        {
            return OperateurUnaire::Genre::Non_Binaire;
        }
        case GenreLexeme::EXCLAMATION:
        {
            return OperateurUnaire::Genre::Non_Logique;
        }
        default:
        {
            return OperateurUnaire::Genre::Invalide;
        }
    }
}

// types comparaisons :
// ==, !=, <, >, <=, =>
static GenreLexeme operateurs_comparaisons[] = {GenreLexeme::EGALITE,
                                                GenreLexeme::DIFFERENCE,
                                                GenreLexeme::INFERIEUR,
                                                GenreLexeme::SUPERIEUR,
                                                GenreLexeme::INFERIEUR_EGAL,
                                                GenreLexeme::SUPERIEUR_EGAL};

// types entiers et réels :
// +, -, *, / (assignés +=, -=, /=, *=)
static GenreLexeme operateurs_entiers_reels[] = {
    GenreLexeme::PLUS,
    GenreLexeme::MOINS,
    GenreLexeme::FOIS,
    GenreLexeme::DIVISE,
};

// types entiers :
// %, <<, >>, &, |, ^ (assignés %=, <<=, >>=, &, |, ^)
static GenreLexeme operateurs_entiers[] = {GenreLexeme::POURCENT,
                                           GenreLexeme::DECALAGE_GAUCHE,
                                           GenreLexeme::DECALAGE_DROITE,
                                           GenreLexeme::ESPERLUETTE,
                                           GenreLexeme::BARRE,
                                           GenreLexeme::CHAPEAU};

// types entiers unaires :
// ~
static GenreLexeme operateurs_entiers_unaires[] = {GenreLexeme::TILDE};

static bool est_commutatif(GenreLexeme id)
{
    switch (id) {
        default:
        {
            return false;
        }
        case GenreLexeme::PLUS:
        case GenreLexeme::FOIS:
        case GenreLexeme::EGALITE:
        case GenreLexeme::DIFFERENCE:
        {
            return true;
        }
    }
}

const char *chaine_pour_genre_op(OperateurBinaire::Genre genre)
{
    switch (genre) {
        case OperateurBinaire::Genre::Addition:
        {
            return "ajt";
        }
        case OperateurBinaire::Genre::Addition_Reel:
        {
            return "ajtr";
        }
        case OperateurBinaire::Genre::Soustraction:
        {
            return "sst";
        }
        case OperateurBinaire::Genre::Soustraction_Reel:
        {
            return "sstr";
        }
        case OperateurBinaire::Genre::Multiplication:
        {
            return "mul";
        }
        case OperateurBinaire::Genre::Multiplication_Reel:
        {
            return "mulr";
        }
        case OperateurBinaire::Genre::Division_Naturel:
        {
            return "divn";
        }
        case OperateurBinaire::Genre::Division_Relatif:
        {
            return "divz";
        }
        case OperateurBinaire::Genre::Division_Reel:
        {
            return "divr";
        }
        case OperateurBinaire::Genre::Reste_Naturel:
        {
            return "modn";
        }
        case OperateurBinaire::Genre::Reste_Relatif:
        {
            return "modz";
        }
        case OperateurBinaire::Genre::Comp_Egal:
        {
            return "eg";
        }
        case OperateurBinaire::Genre::Comp_Inegal:
        {
            return "neg";
        }
        case OperateurBinaire::Genre::Comp_Inf:
        {
            return "inf";
        }
        case OperateurBinaire::Genre::Comp_Inf_Egal:
        {
            return "infeg";
        }
        case OperateurBinaire::Genre::Comp_Sup:
        {
            return "sup";
        }
        case OperateurBinaire::Genre::Comp_Sup_Egal:
        {
            return "supeg";
        }
        case OperateurBinaire::Genre::Comp_Inf_Nat:
        {
            return "infn";
        }
        case OperateurBinaire::Genre::Comp_Inf_Egal_Nat:
        {
            return "infegn";
        }
        case OperateurBinaire::Genre::Comp_Sup_Nat:
        {
            return "supn";
        }
        case OperateurBinaire::Genre::Comp_Sup_Egal_Nat:
        {
            return "supegn";
        }
        case OperateurBinaire::Genre::Comp_Egal_Reel:
        {
            return "egr";
        }
        case OperateurBinaire::Genre::Comp_Inegal_Reel:
        {
            return "negr";
        }
        case OperateurBinaire::Genre::Comp_Inf_Reel:
        {
            return "infr";
        }
        case OperateurBinaire::Genre::Comp_Inf_Egal_Reel:
        {
            return "infegr";
        }
        case OperateurBinaire::Genre::Comp_Sup_Reel:
        {
            return "supr";
        }
        case OperateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            return "supegr";
        }
        case OperateurBinaire::Genre::Et_Binaire:
        {
            return "et";
        }
        case OperateurBinaire::Genre::Ou_Binaire:
        {
            return "ou";
        }
        case OperateurBinaire::Genre::Ou_Exclusif:
        {
            return "oux";
        }
        case OperateurBinaire::Genre::Dec_Gauche:
        {
            return "decg";
        }
        case OperateurBinaire::Genre::Dec_Droite_Arithm:
        {
            return "decda";
        }
        case OperateurBinaire::Genre::Dec_Droite_Logique:
        {
            return "decdl";
        }
        case OperateurBinaire::Genre::Indexage:
        case OperateurBinaire::Genre::Invalide:
        {
            return "invalide";
        }
    }

    return "inconnu";
}

const char *chaine_pour_genre_op(OperateurUnaire::Genre genre)
{
    switch (genre) {
        case OperateurUnaire::Genre::Positif:
        {
            return "plus";
        }
        case OperateurUnaire::Genre::Complement:
        {
            return "moins";
        }
        case OperateurUnaire::Genre::Non_Logique:
        {
            return "non";
        }
        case OperateurUnaire::Genre::Non_Binaire:
        {
            return "non";
        }
        case OperateurUnaire::Genre::Prise_Adresse:
        {
            return "addr";
        }
        case OperateurUnaire::Genre::Invalide:
        {
            return "invalide";
        }
    }

    return "inconnu";
}

inline int index_op_binaire(GenreLexeme lexeme)
{
    // À FAIRE: l'indice n'est pas bon, nous devrions utiliser le genre pour le bon type de données
    return static_cast<int>(genre_op_binaire_pour_lexeme(lexeme, IndiceTypeOp::ENTIER_NATUREL));
}

inline int index_op_unaire(GenreLexeme lexeme)
{
    return static_cast<int>(genre_op_unaire_pour_lexeme(lexeme));
}

constexpr inline int nombre_genre_op_binaires()
{
    int compte = 0;
#define ENUMERE_GENRE_OPBINAIRE_EX(x) compte += 1;
    ENUMERE_OPERATEURS_BINAIRE
#undef ENUMERE_GENRE_OPBINAIRE_EX
    return compte;
}

constexpr inline int nombre_genre_op_unaires()
{
    int compte = 0;
#define ENUMERE_GENRE_OPUNAIRE_EX(x) compte += 1;
    ENUMERE_OPERATEURS_UNAIRE
#undef ENUMERE_GENRE_OPUNAIRE_EX
    return compte;
}

void TableOperateurs::ajoute(GenreLexeme lexeme, OperateurBinaire *operateur)
{
    /* n'alloue de la mémoire que si nous ajoutons un opérateurs */
    operateurs_.redimensionne(nombre_genre_op_binaires());

    /* nous utilisons le lexème pour indexer afin que les opérateurs dépendants
     * du signe des types soient stockés ensemble */
    operateurs_[index_op_binaire(lexeme)].ajoute(operateur);
}

const TableOperateurs::type_conteneur &TableOperateurs::operateurs(GenreLexeme lexeme)
{
    /* retourne un tableau vide si aucun opérateur n'a été ajouté */
    if (operateurs_.est_vide()) {
        static type_conteneur tableau_vide;
        return tableau_vide;
    }

    return operateurs_[index_op_binaire(lexeme)];
}

Operateurs::Operateurs()
{
    operateurs_unaires.redimensionne(nombre_genre_op_unaires());
    operateurs_binaires.redimensionne(nombre_genre_op_binaires());
}

Operateurs::~Operateurs()
{
}

const Operateurs::type_conteneur_unaire &Operateurs::trouve_unaire(GenreLexeme id) const
{
    return operateurs_unaires[index_op_unaire(id)];
}

OperateurBinaire *Operateurs::ajoute_basique(GenreLexeme id,
                                             Type *type,
                                             Type *type_resultat,
                                             IndiceTypeOp indice_type)
{
    return ajoute_basique(id, type, type, type_resultat, indice_type);
}

OperateurBinaire *Operateurs::ajoute_basique(
    GenreLexeme id, Type *type1, Type *type2, Type *type_resultat, IndiceTypeOp indice_type)
{
    assert(type1);
    assert(type2);

    auto op = operateurs_binaires[index_op_binaire(id)].ajoute_element();
    op->type1 = type1;
    op->type2 = type2;
    op->type_resultat = type_resultat;
    op->est_commutatif = est_commutatif(id);
    op->est_basique = true;
    op->genre = genre_op_binaire_pour_lexeme(id, indice_type);
    type1->operateurs.ajoute(id, op);
    return op;
}

OperateurUnaire *Operateurs::ajoute_basique_unaire(GenreLexeme id, Type *type, Type *type_resultat)
{
    auto op = operateurs_unaires[index_op_unaire(id)].ajoute_element();
    op->type_operande = type;
    op->type_resultat = type_resultat;
    op->est_basique = true;
    op->genre = genre_op_unaire_pour_lexeme(id);
    return op;
}

void Operateurs::ajoute_perso(GenreLexeme id,
                              Type *type1,
                              Type *type2,
                              Type *type_resultat,
                              NoeudDeclarationEnteteFonction *decl)
{
    auto op = operateurs_binaires[index_op_binaire(id)].ajoute_element();
    op->type1 = type1;
    op->type2 = type2;
    op->type_resultat = type_resultat;
    op->est_commutatif = est_commutatif(id);
    op->est_basique = false;
    op->decl = decl;
    type1->operateurs.ajoute(id, op);

    if (id == GenreLexeme::CROCHET_OUVRANT) {
        type1->operateur_indexage = op;
    }
}

void Operateurs::ajoute_perso_unaire(GenreLexeme id,
                                     Type *type,
                                     Type *type_resultat,
                                     NoeudDeclarationEnteteFonction *decl)
{
    auto op = operateurs_unaires[index_op_unaire(id)].ajoute_element();
    op->type_operande = type;
    op->type_resultat = type_resultat;
    op->est_basique = false;
    op->decl = decl;
    op->genre = genre_op_unaire_pour_lexeme(id);
}

void Operateurs::ajoute_operateur_basique_enum(Typeuse const &typeuse, TypeEnum *type)
{
    auto const &type_bool = typeuse[TypeBase::BOOL];

    auto indice_type_op = IndiceTypeOp();
    if (type->type_donnees->est_entier_naturel()) {
        indice_type_op = IndiceTypeOp::ENTIER_NATUREL;
    }
    else {
        indice_type_op = IndiceTypeOp::ENTIER_RELATIF;
    }

    for (auto op : operateurs_comparaisons) {
        auto op_bin = this->ajoute_basique(op, type, type_bool, indice_type_op);

        if (op == GenreLexeme::EGALITE) {
            type->operateur_egt = op_bin;
        }
        else if (op == GenreLexeme::DIFFERENCE) {
            type->operateur_dif = op_bin;
        }
    }

    for (auto op : operateurs_entiers) {
        auto ptr_op = this->ajoute_basique(op, type, type, indice_type_op);

        if (op == GenreLexeme::ESPERLUETTE) {
            type->operateur_etb = ptr_op;
        }
        else if (op == GenreLexeme::BARRE) {
            type->operateur_oub = ptr_op;
        }
    }

    type->operateur_non = this->ajoute_basique_unaire(GenreLexeme::TILDE, type, type);
}

void Operateurs::ajoute_operateurs_basiques_pointeur(const Typeuse &typeuse, TypePointeur *type)
{
    auto indice = IndiceTypeOp::ENTIER_RELATIF;

    auto const &type_bool = typeuse[TypeBase::BOOL];

    ajoute_basique(GenreLexeme::EGALITE, type, type_bool, indice);
    ajoute_basique(GenreLexeme::DIFFERENCE, type, type_bool, indice);

    ajoute_basique(GenreLexeme::INFERIEUR, type, type_bool, indice);
    ajoute_basique(GenreLexeme::INFERIEUR_EGAL, type, type_bool, indice);
    ajoute_basique(GenreLexeme::SUPERIEUR, type, type_bool, indice);
    ajoute_basique(GenreLexeme::SUPERIEUR_EGAL, type, type_bool, indice);

    /* Pour l'arithmétique de pointeur nous n'utilisons que le type le plus
     * gros, la résolution de l'opérateur ajoutera une transformation afin
     * que le type plus petit soit transtyper à la bonne taille. */
    auto type_entier = typeuse[TypeBase::Z64];

    ajoute_basique(GenreLexeme::PLUS, type, type_entier, type, indice)->est_arithmetique_pointeur =
        true;
    ajoute_basique(GenreLexeme::MOINS, type, type_entier, type, indice)
        ->est_arithmetique_pointeur = true;
    ajoute_basique(GenreLexeme::MOINS, type, type, type_entier, indice)
        ->est_arithmetique_pointeur = true;
    ajoute_basique(GenreLexeme::PLUS_EGAL, type, type_entier, type, indice)
        ->est_arithmetique_pointeur = true;
    ajoute_basique(GenreLexeme::MOINS_EGAL, type, type_entier, type, indice)
        ->est_arithmetique_pointeur = true;

    type_entier = typeuse[TypeBase::N64];
    indice = IndiceTypeOp::ENTIER_NATUREL;

    ajoute_basique(GenreLexeme::PLUS, type, type_entier, type, indice)->est_arithmetique_pointeur =
        true;
    ajoute_basique(GenreLexeme::MOINS, type, type_entier, type, indice)
        ->est_arithmetique_pointeur = true;
    ajoute_basique(GenreLexeme::PLUS_EGAL, type, type_entier, type, indice)
        ->est_arithmetique_pointeur = true;
    ajoute_basique(GenreLexeme::MOINS_EGAL, type, type_entier, type, indice)
        ->est_arithmetique_pointeur = true;
}

void Operateurs::ajoute_operateurs_basiques_fonction(const Typeuse &typeuse, TypeFonction *type)
{
    auto indice = IndiceTypeOp::ENTIER_RELATIF;

    auto const &type_bool = typeuse[TypeBase::BOOL];

    ajoute_basique(GenreLexeme::EGALITE, type, type_bool, indice);
    ajoute_basique(GenreLexeme::DIFFERENCE, type, type_bool, indice);
}

void Operateurs::rassemble_statistiques(Statistiques &stats) const
{
    auto nombre_unaires = int64_t(0);
    auto memoire_unaires = operateurs_unaires.taille_memoire();

    POUR (operateurs_unaires) {
        memoire_unaires += it.memoire_utilisee();
        nombre_unaires += it.taille();
    }

    auto nombre_binaires = int64_t(0);
    auto memoire_binaires = operateurs_binaires.taille_memoire();

    POUR (operateurs_binaires) {
        memoire_binaires += it.memoire_utilisee();
        nombre_binaires += it.taille();
    }

    auto &stats_ops = stats.stats_operateurs;
    stats_ops.fusionne_entree({"OperateurUnaire", nombre_unaires, memoire_unaires});
    stats_ops.fusionne_entree({"OperateurBinaire", nombre_binaires, memoire_binaires});
}

std::optional<Attente> cherche_candidats_operateurs(EspaceDeTravail &espace,
                                                    Type *type1,
                                                    Type *type2,
                                                    GenreLexeme type_op,
                                                    kuri::tablet<OperateurCandidat, 10> &candidats)
{
    assert(type1);
    assert(type2);

    auto op_candidats = kuri::tablet<OperateurBinaire const *, 10>();

    POUR (type1->operateurs.operateurs(type_op).plage()) {
        op_candidats.ajoute(it);
    }

    if (type1 != type2) {
        POUR (type2->operateurs.operateurs(type_op).plage()) {
            op_candidats.ajoute(it);
        }
    }

    for (auto const op : op_candidats) {
        auto poids1_ou_attente = verifie_compatibilite(op->type1, type1);

        if (std::holds_alternative<Attente>(poids1_ou_attente)) {
            return std::get<Attente>(poids1_ou_attente);
        }

        auto poids1 = std::get<PoidsTransformation>(poids1_ou_attente);

        auto poids2_ou_attente = verifie_compatibilite(op->type2, type2);

        if (std::holds_alternative<Attente>(poids2_ou_attente)) {
            return std::get<Attente>(poids2_ou_attente);
        }

        auto poids2 = std::get<PoidsTransformation>(poids2_ou_attente);

        auto poids = poids1.poids * poids2.poids;

        /* Nous ajoutons également les opérateurs ayant un poids de 0 afin de pouvoir donner des
         * détails dans les messages d'erreurs. */
        auto candidat = OperateurCandidat{};
        candidat.op = op;
        candidat.poids = poids;
        candidat.transformation_type1 = poids1.transformation;
        candidat.transformation_type2 = poids2.transformation;

        candidats.ajoute(candidat);

        if (op->est_commutatif && poids != 1.0) {
            auto poids3_ou_attente = verifie_compatibilite(op->type1, type2);

            if (std::holds_alternative<Attente>(poids3_ou_attente)) {
                return std::get<Attente>(poids3_ou_attente);
            }

            auto poids3 = std::get<PoidsTransformation>(poids3_ou_attente);

            auto poids4_ou_attente = verifie_compatibilite(op->type2, type1);

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
            candidat.permute_operandes = true;

            candidats.ajoute(candidat);
        }
    }

    return {};
}

const OperateurUnaire *cherche_operateur_unaire(Operateurs const &operateurs,
                                                Type *type1,
                                                GenreLexeme type_op)
{
    auto &iter = operateurs.trouve_unaire(type_op);

    for (auto i = 0; i < iter.taille(); ++i) {
        auto op = &iter[i];

        if (op->type_operande == type1) {
            return op;
        }
    }

    return nullptr;
}

void enregistre_operateurs_basiques(Typeuse &typeuse, Operateurs &operateurs)
{
    Type *types_entiers_naturels[] = {
        typeuse[TypeBase::N8],
        typeuse[TypeBase::N16],
        typeuse[TypeBase::N32],
        typeuse[TypeBase::N64],
    };

    Type *types_entiers_relatifs[] = {
        typeuse[TypeBase::Z8],
        typeuse[TypeBase::Z16],
        typeuse[TypeBase::Z32],
        typeuse[TypeBase::Z64],
    };

    auto type_r32 = typeuse[TypeBase::R32];
    auto type_r64 = typeuse[TypeBase::R64];

    Type *types_reels[] = {type_r32, type_r64};

    auto type_entier_constant = typeuse[TypeBase::ENTIER_CONSTANT];
    auto type_octet = typeuse[TypeBase::OCTET];
    auto type_bool = typeuse[TypeBase::BOOL];

    for (auto op : operateurs_entiers_reels) {
        for (auto type : types_entiers_relatifs) {
            auto operateur = operateurs.ajoute_basique(
                op, type, type, IndiceTypeOp::ENTIER_RELATIF);

            if (op == GenreLexeme::PLUS) {
                type->operateur_ajt = operateur;
            }
            else if (op == GenreLexeme::MOINS) {
                type->operateur_sst = operateur;
            }
            else if (op == GenreLexeme::FOIS) {
                type->operateur_mul = operateur;
            }
            else if (op == GenreLexeme::DIVISE) {
                type->operateur_div = operateur;
            }
        }

        for (auto type : types_entiers_naturels) {
            auto operateur = operateurs.ajoute_basique(
                op, type, type, IndiceTypeOp::ENTIER_NATUREL);

            if (op == GenreLexeme::PLUS) {
                type->operateur_ajt = operateur;
            }
            else if (op == GenreLexeme::MOINS) {
                type->operateur_sst = operateur;
            }
            else if (op == GenreLexeme::FOIS) {
                type->operateur_mul = operateur;
            }
            else if (op == GenreLexeme::DIVISE) {
                type->operateur_div = operateur;
            }
        }

        for (auto type : types_reels) {
            auto operateur = operateurs.ajoute_basique(op, type, type, IndiceTypeOp::REEL);

            if (op == GenreLexeme::PLUS) {
                type->operateur_ajt = operateur;
            }
            else if (op == GenreLexeme::MOINS) {
                type->operateur_sst = operateur;
            }
            else if (op == GenreLexeme::FOIS) {
                type->operateur_mul = operateur;
            }
            else if (op == GenreLexeme::DIVISE) {
                type->operateur_div = operateur;
            }
        }

        operateurs.ajoute_basique(op, type_octet, type_octet, IndiceTypeOp::ENTIER_RELATIF);
        operateurs.ajoute_basique(
            op, type_entier_constant, type_entier_constant, IndiceTypeOp::ENTIER_NATUREL);
    }

    for (auto op : operateurs_comparaisons) {
        for (auto type : types_entiers_relatifs) {
            auto operateur = operateurs.ajoute_basique(
                op, type, type_bool, IndiceTypeOp::ENTIER_RELATIF);

            if (op == GenreLexeme::SUPERIEUR) {
                type->operateur_sup = operateur;
            }
            else if (op == GenreLexeme::SUPERIEUR_EGAL) {
                type->operateur_seg = operateur;
            }
            else if (op == GenreLexeme::INFERIEUR) {
                type->operateur_inf = operateur;
            }
            else if (op == GenreLexeme::INFERIEUR_EGAL) {
                type->operateur_ieg = operateur;
            }
            else if (op == GenreLexeme::EGALITE) {
                type->operateur_egt = operateur;
            }
        }

        for (auto type : types_entiers_naturels) {
            auto operateur = operateurs.ajoute_basique(
                op, type, type_bool, IndiceTypeOp::ENTIER_NATUREL);

            if (op == GenreLexeme::SUPERIEUR) {
                type->operateur_sup = operateur;
            }
            else if (op == GenreLexeme::SUPERIEUR_EGAL) {
                type->operateur_seg = operateur;
            }
            else if (op == GenreLexeme::INFERIEUR) {
                type->operateur_inf = operateur;
            }
            else if (op == GenreLexeme::INFERIEUR_EGAL) {
                type->operateur_ieg = operateur;
            }
            else if (op == GenreLexeme::EGALITE) {
                type->operateur_egt = operateur;
            }
        }

        for (auto type : types_reels) {
            auto operateur = operateurs.ajoute_basique(op, type, type_bool, IndiceTypeOp::REEL);

            if (op == GenreLexeme::SUPERIEUR) {
                type->operateur_sup = operateur;
            }
            else if (op == GenreLexeme::SUPERIEUR_EGAL) {
                type->operateur_seg = operateur;
            }
            else if (op == GenreLexeme::INFERIEUR) {
                type->operateur_inf = operateur;
            }
            else if (op == GenreLexeme::INFERIEUR_EGAL) {
                type->operateur_ieg = operateur;
            }
            else if (op == GenreLexeme::EGALITE) {
                type->operateur_egt = operateur;
            }
        }

        operateurs.ajoute_basique(op, type_octet, type_bool, IndiceTypeOp::ENTIER_RELATIF);
        operateurs.ajoute_basique(
            op, type_entier_constant, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    }

    for (auto op : operateurs_entiers) {
        for (auto type : types_entiers_relatifs) {
            operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_RELATIF);
        }

        for (auto type : types_entiers_naturels) {
            operateurs.ajoute_basique(op, type, type, IndiceTypeOp::ENTIER_NATUREL);
        }

        operateurs.ajoute_basique(op, type_octet, type_octet, IndiceTypeOp::ENTIER_RELATIF);
        operateurs.ajoute_basique(
            op, type_entier_constant, type_entier_constant, IndiceTypeOp::ENTIER_NATUREL);
    }

    POUR (operateurs_entiers_unaires) {
        for (auto type : types_entiers_relatifs) {
            operateurs.ajoute_basique_unaire(it, type, type);
        }

        for (auto type : types_entiers_naturels) {
            operateurs.ajoute_basique_unaire(it, type, type);
        }

        operateurs.ajoute_basique_unaire(it, type_octet, type_octet);
        operateurs.ajoute_basique_unaire(it, type_entier_constant, type_entier_constant);
    }

    // operateurs booléens & | ^ == !=
    operateurs.ajoute_basique(
        GenreLexeme::CHAPEAU, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    operateurs.ajoute_basique(
        GenreLexeme::ESPERLUETTE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    operateurs.ajoute_basique(
        GenreLexeme::BARRE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    operateurs.ajoute_basique(
        GenreLexeme::EGALITE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    operateurs.ajoute_basique(
        GenreLexeme::DIFFERENCE, type_bool, type_bool, IndiceTypeOp::ENTIER_NATUREL);

    // opérateurs unaires + - ~
    for (auto type : types_entiers_naturels) {
        operateurs.ajoute_basique_unaire(GenreLexeme::PLUS_UNAIRE, type, type);
        operateurs.ajoute_basique_unaire(GenreLexeme::MOINS_UNAIRE, type, type);
        operateurs.ajoute_basique_unaire(GenreLexeme::TILDE, type, type);
    }

    for (auto type : types_entiers_relatifs) {
        operateurs.ajoute_basique_unaire(GenreLexeme::PLUS_UNAIRE, type, type);
        operateurs.ajoute_basique_unaire(GenreLexeme::MOINS_UNAIRE, type, type);
        operateurs.ajoute_basique_unaire(GenreLexeme::TILDE, type, type);
    }

    for (auto type : types_reels) {
        operateurs.ajoute_basique_unaire(GenreLexeme::PLUS_UNAIRE, type, type);
        operateurs.ajoute_basique_unaire(GenreLexeme::MOINS_UNAIRE, type, type);
    }

    auto type_type_de_donnees = typeuse.type_type_de_donnees_;

    operateurs.op_comp_egal_types = operateurs.ajoute_basique(
        GenreLexeme::EGALITE, type_type_de_donnees, type_bool, IndiceTypeOp::ENTIER_NATUREL);
    operateurs.op_comp_diff_types = operateurs.ajoute_basique(
        GenreLexeme::DIFFERENCE, type_type_de_donnees, type_bool, IndiceTypeOp::ENTIER_NATUREL);
}
