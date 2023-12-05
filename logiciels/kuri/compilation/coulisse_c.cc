/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "coulisse_c.hh"

#include <fstream>
#include <iostream>
#include <set>

#include "biblinternes/outils/numerique.hh"
#include "biblinternes/structures/tableau_page.hh"

#include "structures/chemin_systeme.hh"
#include "structures/table_hachage.hh"

#include "parsage/identifiant.hh"
#include "parsage/outils_lexemes.hh"

#include "utilitaires/poule_de_taches.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "broyage.hh"
#include "compilatrice.hh"
#include "environnement.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "typage.hh"

#include "representation_intermediaire/impression.hh"

/* Défini si les structures doivent avoir des membres explicites. Sinon, le code généré utilisera
 * des tableaux d'octets pour toutes les structures. */
#define TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES

#undef IMPRIME_COMMENTAIRE

/* Noms de base pour le code généré. Une seule lettre pour minimiser le code. */
static const char *nom_base_chaine = "C";
static const char *nom_base_fonction = "F";
static const char *nom_base_globale = "G";
static const char *nom_base_label = "L";
static const char *nom_base_type = "T";
static const char *nom_base_variable = "V";

/* ------------------------------------------------------------------------- */
/** \name Utilitaires locaux.
 * \{ */

static bool transtypage_est_utile(InstructionTranstype const *transtype)
{
    auto const type_source = transtype->valeur->type;
    auto const type_cible = transtype->type;

    /* Les types opaques sont traités comme les types opacifiés dans le code C.
     * Le code peut avoir des instructions de transtypes d'opaque vers opacifié, et vice versa, car
     * notre RI est typée comme l'arbre syntaxique. Or, ISO C interdit de transtyper vers le même
     * type pour les types non-scalaire. Donc détectons ces cas, et n'émettons pas de transtype
     * pour ceux-ci.
     */
    if (est_type_opacifié(type_source, type_cible) || est_type_opacifié(type_cible, type_source)) {
        return false;
    }

    return true;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Déclaration de GénératriceCodeC.
 * \{ */

struct GénératriceCodeC {
    kuri::tableau<kuri::chaine_statique> table_valeurs{};
    kuri::table_hachage<Atome const *, kuri::chaine_statique> table_globales{"Valeurs globales C"};
    kuri::table_hachage<AtomeFonction const *, kuri::chaine_statique> table_fonctions{
        "Noms fonctions C"};
    kuri::table_hachage<Type const *, kuri::chaine_statique> table_types{"Noms types C"};
    EspaceDeTravail &m_espace;
    AtomeFonction const *m_fonction_courante = nullptr;

    Broyeuse &broyeuse;

    /* Les atomes pour les chaines peuvent être générés plusieurs fois (notamment
     * pour celles des noms des fonctions pour les traces d'appel), donc nous suffixons les noms
     * des variables avec un index pour les rendre uniques. L'index est incrémenté à chaque
     * génération de code pour une chaine. */
    int index_chaine = 0;

    /* Pour les noms des globales anonymes. */
    int nombre_de_globales = 0;

    /* Pour les noms des fonctions. */
    int nombre_de_fonctions = 0;

    /* Pour les noms des fonctions. */
    int nombre_de_types = 0;

    Enchaineuse enchaineuse_tmp{};
    Enchaineuse stockage_chn{};

    /* Si une chaine est trop large pour le stockage de chaines statiques, nous la stockons ici. */
    kuri::tableau<kuri::chaine> chaines_trop_larges_pour_stockage_chn{};

    /* Définis si la génération de code d'atome est pour l'initialisation d'un tableau fixe.
     * Utilisé notamment pour ne pas imprimer les noms dans les structures constantes. */
    bool pour_init_tableau = false;

    template <typename... Ts>
    kuri::chaine_statique enchaine(Ts &&...ts)
    {
        enchaineuse_tmp.réinitialise();
        ((enchaineuse_tmp << ts), ...);
        return stockage_chn.ajoute_chaine_statique(enchaineuse_tmp.chaine_statique());
    }

    GénératriceCodeC(EspaceDeTravail &espace, Broyeuse &broyeuse_);

    EMPECHE_COPIE(GénératriceCodeC);

    kuri::chaine_statique génère_code_pour_atome(Atome const *atome,
                                                 Enchaineuse &os,
                                                 bool pour_globale);

    void génère_code_pour_instruction(Instruction const *inst, Enchaineuse &os);

    void déclare_globale(Enchaineuse &os, AtomeGlobale const *valeur_globale, bool pour_entete);

    void déclare_fonction(Enchaineuse &os, AtomeFonction const *atome_fonc, bool pour_entête);

    void génère_code(ProgrammeRepreInter const &repr_inter, CoulisseC &coulisse, Enchaineuse &os);

    void génère_code(const ProgrammeRepreInter &repr_inter_programme, CoulisseC &coulisse);

    void génère_code_entête(ProgrammeRepreInter const &repr_inter, Enchaineuse &os);

    void génère_code_fonction(const AtomeFonction *atome_fonc, Enchaineuse &os);
    void vide_enchaineuse_dans_fichier(CoulisseC &coulisse, Enchaineuse &os);

    void génère_code_pour_tableaux_données_constantes(Enchaineuse &os,
                                                      const ProgrammeRepreInter &repr_inter,
                                                      bool pour_entête);

    kuri::chaine_statique donne_nom_pour_instruction(Instruction const *instruction);

    kuri::chaine_statique donne_nom_pour_globale(const AtomeGlobale *valeur_globale,
                                                 bool pour_entête);

    kuri::chaine_statique donne_nom_pour_fonction(const AtomeFonction *fonction);

    kuri::chaine_statique donne_nom_pour_type(Type const *type);

    bool préserve_symboles() const;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name TypeC.
 * Données pour générer la déclaration d'un type dans le code C.
 * \{ */

struct TypeC {
    Type const *type_kuri = nullptr;
    kuri::chaine_statique nom = "";
    kuri::chaine_statique typedef_ = "";
    bool code_machine_fut_généré = false;
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Déclaration de ConvertisseuseTypeC.
 * \{ */

struct ConvertisseuseTypeC {
  private:
    mutable tableau_page<TypeC> types_c{};
    Enchaineuse enchaineuse_tmp{};
    Enchaineuse stockage_chn{};
    Broyeuse &broyeuse;
    GénératriceCodeC &génératrice_code;

    kuri::table_hachage<Type const *, TypeC *> table_types_c{""};

    template <typename... Ts>
    kuri::chaine_statique enchaine(Ts &&...ts)
    {
        enchaineuse_tmp.réinitialise();
        ((enchaineuse_tmp << ts), ...);
        return stockage_chn.ajoute_chaine_statique(enchaineuse_tmp.chaine_statique());
    }

  public:
    ConvertisseuseTypeC(Broyeuse &broyeuse_, GénératriceCodeC &génératrice_code_)
        : broyeuse(broyeuse_), génératrice_code(génératrice_code_)
    {
    }

    TypeC &type_c_pour(Type const *type);

    bool typedef_fut_généré(const Type *type_kuri);

    void génère_typedef(Type *type, Enchaineuse &enchaineuse);

    /* Pour la génération de code pour les types, nous devons d'abord nous assurer que tous les
     * types ont un typedef afin de simplifier la génération de code pour les déclaration de
     * variables : avec un typedef `int *a[3]` devient simplement `Tableau3PointeurInt a;`
     * (PointeurTableau3Int est utilisé pour démontrer la chose, dans le langage ce serait plutôt
     * KT3KPKsz32).
     *
     * Pour les structures (ou unions) nous devons également nous assurer que le code des
     * structures utilisées par valeur pour leurs membres ont leurs codes générés avant celui de la
     * structure parent.
     */
    void génère_code_pour_type(Type const *type, Enchaineuse &enchaineuse);

    void génère_déclaration_structure(Enchaineuse &enchaineuse,
                                      TypeStructure const *type_structure,
                                      int quoi);
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Implémentation de ConvertisseuseTypeC.
 * \{ */

TypeC &ConvertisseuseTypeC::type_c_pour(Type const *type)
{
    auto type_c = table_types_c.valeur_ou(type, nullptr);
    if (type_c) {
        return *type_c;
    }

    type_c = types_c.ajoute_element();
    type_c->type_kuri = type;
    type_c->nom = génératrice_code.donne_nom_pour_type(type);
    table_types_c.insère(type, type_c);
    return *type_c;
}

bool ConvertisseuseTypeC::typedef_fut_généré(Type const *type_kuri)
{
    auto &type_c = type_c_pour(type_kuri);
    return type_c.typedef_ != "";
}

void ConvertisseuseTypeC::génère_typedef(Type *type, Enchaineuse &enchaineuse)
{
    if (type->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
        return;
    }

    auto &type_c = type_c_pour(type);

    if (type_c.typedef_ != "") {
        return;
    }

#ifdef IMPRIME_COMMENTAIRE
    enchaineuse << "// " << chaine_type(type) << " (" << type->genre << ')' << '\n';
#endif

    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        {
            /* Aucun typedef. */
            return;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            type_c.typedef_ = "int32_t";
            break;
        }
        case GenreType::ERREUR:
        case GenreType::ENUM:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);
            génère_typedef(type_enum->type_sous_jacent, enchaineuse);
            auto nom_broye_type_donnees = génératrice_code.donne_nom_pour_type(
                type_enum->type_sous_jacent);
            type_c.typedef_ = nom_broye_type_donnees;
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            génère_typedef(type_opaque->type_opacifie, enchaineuse);
            auto nom_broye_type_opacifie = génératrice_code.donne_nom_pour_type(
                type_opaque->type_opacifie);
            type_c.typedef_ = nom_broye_type_opacifie;
            break;
        }
        case GenreType::BOOL:
        {
            type_c.typedef_ = "uint8_t";
            break;
        }
        case GenreType::OCTET:
        {
            type_c.typedef_ = "uint8_t";
            break;
        }
        case GenreType::ENTIER_NATUREL:
        {
            if (type->taille_octet == 1) {
                type_c.typedef_ = "uint8_t";
            }
            else if (type->taille_octet == 2) {
                type_c.typedef_ = "uint16_t";
            }
            else if (type->taille_octet == 4) {
                type_c.typedef_ = "uint32_t";
            }
            else if (type->taille_octet == 8) {
                type_c.typedef_ = "uint64_t";
            }

            break;
        }
        case GenreType::ENTIER_RELATIF:
        {
            if (type->taille_octet == 1) {
                type_c.typedef_ = "int8_t";
            }
            else if (type->taille_octet == 2) {
                type_c.typedef_ = "int16_t";
            }
            else if (type->taille_octet == 4) {
                type_c.typedef_ = "int32_t";
            }
            else if (type->taille_octet == 8) {
                type_c.typedef_ = "int64_t";
            }

            break;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            type_c.typedef_ = "int64_t";
            break;
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 2) {
                type_c.typedef_ = "uint16_t";
            }
            else if (type->taille_octet == 4) {
                type_c.typedef_ = "float";
            }
            else if (type->taille_octet == 8) {
                type_c.typedef_ = "double";
            }

            break;
        }
        case GenreType::REFERENCE:
        {
            auto type_pointe = type->comme_type_reference()->type_pointe;
            génère_typedef(type_pointe, enchaineuse);
            auto &type_c_pointe = type_c_pour(type_pointe);
            type_c.typedef_ = enchaine(type_c_pointe.nom, "*");
            break;
        }
        case GenreType::POINTEUR:
        {
            auto type_pointe = type->comme_type_pointeur()->type_pointe;

            if (type_pointe) {
                génère_typedef(type_pointe, enchaineuse);
                auto &type_c_pointe = type_c_pour(type_pointe);
                type_c.typedef_ = enchaine(type_c_pointe.nom, "*");
            }
            else {
                type_c.typedef_ = "Ksnul*";
            }

            break;
        }
        case GenreType::STRUCTURE:
        {
            auto type_struct = type->comme_type_structure();

            if (type_struct->decl && type_struct->decl->est_polymorphe) {
                /* Aucun typedef. */
                type_c.typedef_ = ".";
                return;
            }

            auto nom_struct = broyeuse.broye_nom_simple(donne_nom_portable(type_struct));

            // struct anomyme
            if (type_struct->est_anonyme) {
                type_c.typedef_ = enchaine("struct ", nom_struct, type_struct);
            }
            else if (type_struct->decl && type_struct->decl->est_monomorphisation) {
                type_c.typedef_ = enchaine("struct ", nom_struct, type_struct);
            }
            else {
                type_c.typedef_ = enchaine("struct ", nom_struct);
            }

            break;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_type_union();
            POUR (type_union->membres) {
                génère_typedef(it.type, enchaineuse);
            }

            auto nom_union = broyeuse.broye_nom_simple(donne_nom_portable(type_union));

            if (type_union->est_anonyme) {
                type_c.typedef_ = enchaine("struct ", nom_union, type_union->type_structure);
            }
            else {
                auto decl = type_union->decl;
                if (decl->est_nonsure || decl->est_externe) {
                    auto type_le_plus_grand = type_union->type_le_plus_grand;
                    type_c.typedef_ = génératrice_code.donne_nom_pour_type(type_le_plus_grand);
                }
                else if (type_union->decl && type_union->decl->est_monomorphisation) {
                    type_c.typedef_ = enchaine("struct ", nom_union, type_union->type_structure);
                }
                else {
                    type_c.typedef_ = enchaine("struct ", nom_union);
                }
            }

            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            type_c.typedef_ = enchaine("struct TableauFixe_", type_c.nom);
            break;
        }
        case GenreType::VARIADIQUE:
        {
            auto variadique = type->comme_type_variadique();
            /* Garantie la génération du typedef pour les types tableaux des variadiques. */
            if (!variadique->type_tableau_dynamique) {
                type_c.typedef_ = "...";
                return;
            }

            auto &type_c_tableau = type_c_pour(variadique->type_tableau_dynamique);
            if (type_c_tableau.typedef_ == "") {
                génère_typedef(variadique->type_tableau_dynamique, enchaineuse);
            }

            /* Nous utilisons le type du tableau, donc initialisons avec un typedef symbolique
             * pour ne plus revenir ici. */
            type_c.typedef_ = ".";
            return;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            auto type_pointe = type->comme_type_tableau_dynamique()->type_pointe;

            if (type_pointe == nullptr) {
                /* Aucun typedef. */
                type_c.typedef_ = ".";
                return;
            }

            génère_typedef(type_pointe, enchaineuse);
            type_c.typedef_ = enchaine("struct Tableau_", type_c.nom);
            break;
        }
        case GenreType::FONCTION:
        {
            auto type_fonc = type->comme_type_fonction();

            POUR (type_fonc->types_entrees) {
                génère_typedef(it, enchaineuse);
            }

            génère_typedef(type_fonc->type_sortie, enchaineuse);

            auto nouveau_nom_broye = Enchaineuse();
            nouveau_nom_broye << "Kf" << type_fonc->types_entrees.taille();

            auto const &nom_broye_sortie = génératrice_code.donne_nom_pour_type(
                type_fonc->type_sortie);

            /* Crée le préfixe. */
            enchaineuse_tmp.réinitialise();
            enchaineuse_tmp << nom_broye_sortie << " (*";
            auto prefixe = stockage_chn.ajoute_chaine_statique(enchaineuse_tmp.chaine_statique());

            /* Réinitialise pour le suffixe. */
            enchaineuse_tmp.réinitialise();

            auto virgule = "(";

            POUR (type_fonc->types_entrees) {
                auto const &nom_broye_dt = génératrice_code.donne_nom_pour_type(it);

                enchaineuse_tmp << virgule;
                enchaineuse_tmp << nom_broye_dt;
                nouveau_nom_broye << nom_broye_dt;
                virgule = ",";
            }

            if (type_fonc->types_entrees.taille() == 0) {
                enchaineuse_tmp << virgule;
                virgule = ",";
            }

            nouveau_nom_broye << 1;
            nouveau_nom_broye << nom_broye_sortie;

            enchaineuse_tmp << ")";
            auto suffixe = stockage_chn.ajoute_chaine_statique(enchaineuse_tmp.chaine_statique());

            if (génératrice_code.préserve_symboles()) {
                type->nom_broye = stockage_chn.ajoute_chaine_statique(
                    nouveau_nom_broye.chaine_statique());
            }
            else {
                type->nom_broye = génératrice_code.donne_nom_pour_type(type);
            }

            type_c.nom = type->nom_broye;

            type_c.typedef_ = enchaine(prefixe, type->nom_broye, ")", suffixe);
            enchaineuse << "typedef " << type_c.typedef_ << ";\n\n";
            /* Les typedefs pour les fonctions ont une syntaxe différente, donc retournons
             * directement. */
            return;
        }
        case GenreType::EINI:
        {
            type_c.typedef_ = "eini";
            break;
        }
        case GenreType::RIEN:
        {
            type_c.typedef_ = "void";
            break;
        }
        case GenreType::CHAINE:
        {
            type_c.typedef_ = "chaine";
            break;
        }
        case GenreType::TUPLE:
        {
            type_c.typedef_ = enchaine("struct ", type_c.nom);
            break;
        }
    }

    enchaineuse << "typedef " << type_c.typedef_ << ' ' << type_c.nom << ";\n";
}

enum {
    STRUCTURE,
    STRUCTURE_ANONYME,
};

void ConvertisseuseTypeC::génère_code_pour_type(const Type *type, Enchaineuse &enchaineuse)
{
    if (!type) {
        /* Les types variadiques externes, ou encore les types pointés des pointeurs nuls
         * peuvent être nuls. */
        return;
    }

    auto &type_c = type_c_pour(type);

    if (type_c.code_machine_fut_généré) {
        return;
    }

    if (type->est_type_structure()) {
        auto type_struct = type->comme_type_structure();

        if (type_struct->decl && type_struct->decl->est_polymorphe) {
            return;
        }

        type_c.code_machine_fut_généré = true;
        POUR (type_struct->membres) {
            if (it.type->est_type_pointeur()) {
                continue;
            }
            /* Une fonction peut retourner via un tuple la structure dont nous essayons de
             * générer le type. Évitons de générer le code du tuple avant la génération du code
             * de cette structure. */
            if (it.type->est_type_fonction()) {
                continue;
            }
            génère_code_pour_type(it.type, enchaineuse);
        }

        auto quoi = type_struct->est_anonyme ? STRUCTURE_ANONYME : STRUCTURE;
        génère_déclaration_structure(enchaineuse, type_struct, quoi);

        POUR (type_struct->membres) {
            if (it.type->est_type_pointeur()) {
                génère_code_pour_type(it.type->comme_type_pointeur()->type_pointe, enchaineuse);
                continue;
            }
        }
    }
    else if (type->est_type_tuple()) {
        auto type_tuple = type->comme_type_tuple();

        if (type_tuple->possède_drapeau(DrapeauxTypes::TYPE_EST_POLYMORPHIQUE)) {
            return;
        }

        type_c.code_machine_fut_généré = true;
        POUR (type_tuple->membres) {
            génère_code_pour_type(it.type, enchaineuse);
        }

        auto nom_broyé = génératrice_code.donne_nom_pour_type(type_tuple);

        enchaineuse << "typedef struct " << nom_broyé << " {\n";

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
        enchaineuse << "  union {\n";
        enchaineuse << "    uint8_t d[" << type->taille_octet << "];\n";
        enchaineuse << "    struct {\n";
#endif
        POUR_INDEX (type_tuple->membres) {
            enchaineuse << "      " << génératrice_code.donne_nom_pour_type(it.type) << " _"
                        << index_it << ";\n";
        }

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
        enchaineuse << "    };\n";  // struct
        enchaineuse << "  };\n";    // union
#endif

        enchaineuse << "} " << nom_broyé << ";\n";
    }
    else if (type->est_type_union()) {
        auto type_union = type->comme_type_union();
        type_c.code_machine_fut_généré = true;
        POUR (type_union->membres) {
            génère_code_pour_type(it.type, enchaineuse);
        }
        génère_code_pour_type(type_union->type_structure, enchaineuse);
    }
    else if (type->est_type_enum()) {
        auto type_enum = type->comme_type_enum();
        génère_code_pour_type(type_enum->type_sous_jacent, enchaineuse);
    }
    else if (type->est_type_tableau_fixe()) {
        auto tableau_fixe = type->comme_type_tableau_fixe();
        génère_code_pour_type(tableau_fixe->type_pointe, enchaineuse);
        auto const &nom_broyé = génératrice_code.donne_nom_pour_type(type);
        enchaineuse << "typedef struct TableauFixe_" << nom_broyé << "{\n  "
                    << génératrice_code.donne_nom_pour_type(tableau_fixe->type_pointe);
        enchaineuse << " d[" << type->comme_type_tableau_fixe()->taille << "];\n";
        enchaineuse << " } TableauFixe_" << nom_broyé << ";\n\n";
    }
    else if (type->est_type_tableau_dynamique()) {
        auto tableau_dynamique = type->comme_type_tableau_dynamique();
        auto type_élément = tableau_dynamique->type_pointe;

        if (type_élément == nullptr) {
            return;
        }

        génère_code_pour_type(type_élément, enchaineuse);

        if (!type_c.code_machine_fut_généré) {
            auto const &nom_broye = génératrice_code.donne_nom_pour_type(type);
            enchaineuse << "typedef struct Tableau_" << nom_broye;
            enchaineuse << "{\n";

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
            enchaineuse << "  union {\n";
            enchaineuse << "    uint8_t d[" << type->taille_octet << "];\n";
            enchaineuse << "    struct {\n";
#endif
            enchaineuse << "      " << génératrice_code.donne_nom_pour_type(type_élément)
                        << " *pointeur;\n";
            enchaineuse << "      int64_t taille;\n"
                        << "      int64_t " << broyeuse.broye_nom_simple(ID::capacite) << ";\n";

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
            enchaineuse << "    };\n";  // struct
            enchaineuse << "  };\n";    // union
#endif
            enchaineuse << "} Tableau_" << nom_broye << ";\n\n";
        }
    }
    else if (type->est_type_opaque()) {
        auto opaque = type->comme_type_opaque();
        génère_code_pour_type(opaque->type_opacifie, enchaineuse);
    }
    else if (type->est_type_fonction()) {
        auto type_fonction = type->comme_type_fonction();
        POUR (type_fonction->types_entrees) {
            génère_code_pour_type(it, enchaineuse);
        }
        génère_code_pour_type(type_fonction->type_sortie, enchaineuse);
    }
    else if (type->est_type_pointeur()) {
        génère_code_pour_type(type->comme_type_pointeur()->type_pointe, enchaineuse);
    }
    else if (type->est_type_reference()) {
        génère_code_pour_type(type->comme_type_reference()->type_pointe, enchaineuse);
    }
    else if (type->est_type_variadique()) {
        génère_code_pour_type(type->comme_type_variadique()->type_pointe, enchaineuse);
        génère_code_pour_type(type->comme_type_variadique()->type_tableau_dynamique, enchaineuse);
    }

    type_c.code_machine_fut_généré = true;
}

void ConvertisseuseTypeC::génère_déclaration_structure(Enchaineuse &enchaineuse,
                                                       const TypeStructure *type_structure,
                                                       int quoi)
{
    auto nom_broyé = broyeuse.broye_nom_simple(
        donne_nom_portable(const_cast<TypeStructure *>(type_structure)));

    if (type_structure->decl && type_structure->decl->est_monomorphisation) {
        nom_broyé = enchaine(nom_broyé, type_structure);
    }

    if (type_structure->decl && type_structure->decl->est_externe) {
        if (type_structure->membres.taille() == 0) {
            enchaineuse << "typedef struct " << nom_broyé << " " << nom_broyé << ";\n\n";
            return;
        }
    }

    if (quoi == STRUCTURE) {
        enchaineuse << "typedef struct " << nom_broyé << "{\n";
    }
    else if (quoi == STRUCTURE_ANONYME) {
        enchaineuse << "typedef struct " << nom_broyé;
        enchaineuse << type_structure;
        enchaineuse << "{\n";
    }

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
    enchaineuse << "union {\n";
    enchaineuse << "    uint8_t d[" << type_structure->taille_octet << "];\n";
    enchaineuse << "    struct {\n";
#endif

    POUR (type_structure->donne_membres_pour_code_machine()) {
        enchaineuse << "      " << génératrice_code.donne_nom_pour_type(it.type) << ' ';

        /* Cas pour les structures vides. */
        if (it.nom == ID::chaine_vide) {
            enchaineuse << "membre_invisible"
                        << ";\n";
        }
        else {
            enchaineuse << broyeuse.broye_nom_simple(it.nom) << ";\n";
        }
    }

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
    enchaineuse << "    };\n";  // struct
    enchaineuse << "  };\n";    // union
#endif
    enchaineuse << "} ";

    if (type_structure->decl) {
        if (type_structure->decl->est_compacte) {
            enchaineuse << " __attribute__((packed)) ";
        }

        if (type_structure->decl->alignement_desire != 0) {
            enchaineuse << " __attribute__((aligned(" << type_structure->decl->alignement_desire
                        << "))) ";
        }
    }

    enchaineuse << nom_broyé;

    if (quoi == STRUCTURE_ANONYME) {
        enchaineuse << type_structure;
    }

    enchaineuse << ";\n\n";
}

/** \} */

/* ************************************************************************** */

static void génère_code_début_fichier(Enchaineuse &enchaineuse, kuri::chaine const &racine_kuri)
{
    enchaineuse << "#include <" << racine_kuri << "/fichiers/r16_c.h>\n";

    auto const préambule = R"(
#include <stdint.h>

#ifdef __GNUC__
#  define INUTILISE(x) INUTILISE_ ## x __attribute__((__unused__))
#else
#  define INUTILISE(x) INUTILISE_ ## x
#endif

#ifdef __GNUC__
#  define TOUJOURS_ENLIGNE __attribute__((always_inline)) inline
#else
#  define TOUJOURS_ENLIGNE
#endif

#ifdef __GNUC__
#  define TOUJOURS_HORSLIGNE __attribute__((noinline))
#else
#  define TOUJOURS_HORSLIGNE
#endif

#if __GNUC__ >= 4
#  define SYMBOLE_PUBLIC __attribute__ ((visibility ("default")))
#  define SYMBOLE_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#  define SYMBOLE_PUBLIC
#  define SYMBOLE_LOCAL
#endif

#ifdef __GNUC__
#  define VARIABLE_INUTILISEE __attribute__((unused))
#else
#  define VARIABLE_INUTILISEE
#endif

#ifndef bool
typedef uint8_t bool;
#endif

#define __point_d_entree_systeme main

typedef uint8_t octet;
typedef void Ksnul;
typedef int8_t ** KPKPKsz8;
)";

    enchaineuse << préambule;

    /* Déclaration des types de bases*/

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
    auto const types_chaine_et_eini = R"(
typedef struct chaine {
    union {
        uint8_t d[16];
        struct {
            char *pointeur;
            int64_t taille;
        };
    };
} chaine;

typedef struct eini {
    union {
        uint8_t d[16];
        struct {
            void *pointeur;
            struct KuriInfoType *info;
        };
    };
} eini;
)";
#else
    auto const types_chaine_et_eini = R"(
typedef struct chaine {
    char *pointeur;
    int64_t taille;
} chaine;

typedef struct eini {
    void *pointeur;
    struct KuriInfoType *info;
} eini;
)";
#endif

    enchaineuse << types_chaine_et_eini;
    /* Pas beau, mais un pointeur de fonction peut être un pointeur vers une fonction
     * de LibC dont les arguments variadiques ne sont pas typés. */
    enchaineuse << "#define Kv ...\n\n";

    /* À FAIRE : meilleure manière de faire ceci ? Il nous faudra alors remplacer l'appel à
     * "__principale" par un appel à "principale". On pourrait également définir ceci selon le nom
     * de la fonction principale définie par les programmes. */
    enchaineuse << "#define __principale principale\n\n";
}

/* Documentation GCC pour la visibilité : https://gcc.gnu.org/wiki/Visibility */
static kuri::chaine_statique donne_chaine_pour_visibilité(VisibilitéSymbole visibilité)
{
    switch (visibilité) {
        case VisibilitéSymbole::EXPORTÉ:
        {
            return "SYMBOLE_PUBLIC ";
        }
        case VisibilitéSymbole::INTERNE:
        {
            return "SYMBOLE_LOCAL ";
        }
    }

    return "";
}

static void déclare_visibilité_globale(Enchaineuse &os,
                                       AtomeGlobale const *valeur_globale,
                                       bool pour_entête)
{
    if (valeur_globale->est_externe) {
        os << "extern ";
        return;
    }

    if (pour_entête) {
        os << donne_chaine_pour_visibilité(valeur_globale->donne_visibilité_symbole());
    }

    if (valeur_globale->est_constante) {
        if (pour_entête) {
            os << "extern ";
        }
        os << "const ";
    }
}

/* ------------------------------------------------------------------------- */
/** \name Implémentation de GénératriceCodeC.
 * \{ */

GénératriceCodeC::GénératriceCodeC(EspaceDeTravail &espace, Broyeuse &broyeuse_)
    : m_espace(espace), broyeuse(broyeuse_)
{
}

kuri::chaine_statique GénératriceCodeC::génère_code_pour_atome(Atome const *atome,
                                                               Enchaineuse &os,
                                                               bool pour_globale)
{
    switch (atome->genre_atome) {
        case Atome::Genre::FONCTION:
        {
            auto atome_fonc = atome->comme_fonction();

            if (atome_fonc->decl &&
                atome_fonc->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
                return atome_fonc->decl->données_externes->nom_symbole;
            }

            return donne_nom_pour_fonction(atome_fonc);
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = atome->comme_instruction();
            return table_valeurs[inst->numero];
        }
        case Atome::Genre::GLOBALE:
        {
            return table_globales.valeur_ou(atome, "");
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype_const = atome->comme_transtype_constant();
            auto valeur = génère_code_pour_atome(transtype_const->valeur, os, pour_globale);
            return enchaine("(", donne_nom_pour_type(transtype_const->type), ")(", valeur, ")");
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            auto inst_accès = atome->comme_accès_index_constant();
            auto valeur_accédée = génère_code_pour_atome(inst_accès->accede, os, false);

            if (inst_accès->accede->genre_atome == Atome::Genre::GLOBALE &&
                est_globale_pour_tableau_données_constantes(inst_accès->accede->comme_globale())) {
                /* Les tableaux de données constantes doivent toujours être accéder par un index de
                 * 0, donc ce doit être légitime de simplement retourné le code de l'atome. */
                return valeur_accédée;
            }

            if (est_type_tableau_fixe(inst_accès->donne_type_accédé())) {
                valeur_accédée = enchaine(valeur_accédée, ".d");
            }

            return enchaine(valeur_accédée, "[", inst_accès->index, "]");
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            return "0";
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            auto type = atome->comme_constante_type()->type_de_données;
            if (type->est_type_type_de_donnees()) {
                auto type_de_données = type->comme_type_type_de_donnees();
                if (type_de_données->type_connu) {
                    return enchaine(type_de_données->type_connu->index_dans_table_types);
                }
            }
            return enchaine(type->index_dans_table_types);
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type = atome->comme_taille_de()->type_de_données;
            return enchaine(type->taille_octet);
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = atome->comme_constante_réelle();
            auto type = constante_réelle->type;

            if (type->taille_octet == 4) {
                return enchaine("(float)", constante_réelle->valeur);
            }

            return enchaine("(double)", constante_réelle->valeur);
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = atome->comme_constante_entière();
            auto type = constante_entière->type;
            auto valeur_entière = constante_entière->valeur;

            if (type->est_type_entier_naturel()) {
                if (type->taille_octet == 1) {
                    return enchaine(static_cast<uint32_t>(valeur_entière));
                }
                else if (type->taille_octet == 2) {
                    return enchaine(static_cast<uint32_t>(valeur_entière));
                }
                else if (type->taille_octet == 4) {
                    return enchaine(static_cast<uint32_t>(valeur_entière));
                }
                else if (type->taille_octet == 8) {
                    return enchaine(valeur_entière, "UL");
                }
            }
            else {
                if (type->taille_octet == 1) {
                    return enchaine(static_cast<int>(valeur_entière));
                }
                else if (type->taille_octet == 2) {
                    return enchaine(static_cast<int>(valeur_entière));
                }
                else if (type->taille_octet == 4 || type->taille_octet == 0) {
                    return enchaine(static_cast<int>(valeur_entière));
                }
                else if (type->taille_octet == 8) {
                    return enchaine(valeur_entière, "L");
                }
            }

            return "";
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            /* Convertis vers une valeur entière pour éviter les problèmes
             * d'instantiation de templates (Enchaineuse ne gère pas les valeurs
             * booléennes, et si elle devait, elle imprimerait "vrai" ou "faux",
             * qui ne sont pas des identifiants valides en C). */
            auto constante_booléenne = atome->comme_constante_booléenne();
            return enchaine(constante_booléenne->valeur ? 1 : 0);
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = atome->comme_constante_caractère();
            return enchaine(caractère->valeur);
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure = atome->comme_constante_structure();
            auto type = structure->type->comme_type_compose();
            auto tableau_valeur = structure->donne_atomes_membres();
            auto résultat = Enchaineuse();

            auto virgule = "{ ";
            // ceci car il peut n'y avoir qu'un seul membre de type tableau qui
            // n'est pas initialisé
            auto virgule_placee = false;

            POUR_INDEX (type->donne_membres_pour_code_machine()) {
                résultat << virgule;
                virgule_placee = true;

                if (!pour_init_tableau) {
                    résultat << ".";
                    if (it.nom == ID::chaine_vide) {
                        résultat << "membre_invisible";
                    }
                    else {
                        résultat << broyeuse.broye_nom_simple(it.nom);
                    }
                    résultat << " = ";
                }
                résultat << génère_code_pour_atome(tableau_valeur[index_it], os, pour_globale);

                virgule = ", ";
            }

            if (!virgule_placee) {
                résultat << "{ 0";
            }

            résultat << " }";

            if (pour_globale) {
                return stockage_chn.ajoute_chaine_statique(résultat.chaine_statique());
            }

            auto nom = enchaine(nom_base_chaine, index_chaine++);
            os << "  " << donne_nom_pour_type(type) << " " << nom << " = " << résultat.chaine()
               << ";\n";
            return nom;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau = atome->comme_constante_tableau();
            auto éléments = tableau->donne_atomes_éléments();
            auto résultat = Enchaineuse();

            auto virgule = "{ .d = { ";
            auto nombre_de_tampons = 1;

            POUR (éléments) {
                résultat << virgule;
                résultat << génère_code_pour_atome(it, os, pour_globale);
                virgule = ", ";

                if (résultat.nombre_tampons() > nombre_de_tampons) {
                    /* Retourne à la ligne car GCC à du mal avec des chaines trop
                     * grandes. */
                    résultat << "\n";
                    nombre_de_tampons = résultat.nombre_tampons();
                }
            }

            if (éléments.taille() == 0) {
                résultat << "{}";
            }
            else {
                résultat << " } }";
            }

            if (résultat.nombre_tampons() > 1) {
                auto chaine_resultat = résultat.chaine();
                chaines_trop_larges_pour_stockage_chn.ajoute(chaine_resultat);
                return chaines_trop_larges_pour_stockage_chn.dernière();
            }

            return stockage_chn.ajoute_chaine_statique(résultat.chaine_statique());
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            auto constante = atome->comme_données_constantes();
            auto données = constante->donne_données();

            enchaineuse_tmp.réinitialise();

            auto virgule = "{ ";

            POUR (données) {
                enchaineuse_tmp << virgule;
                enchaineuse_tmp << "0x";
                enchaineuse_tmp << dls::num::char_depuis_hex((it & 0xf0) >> 4);
                enchaineuse_tmp << dls::num::char_depuis_hex(it & 0x0f);
                virgule = ", ";
            }

            if (données.taille() == 0) {
                enchaineuse_tmp << "{";
            }

            enchaineuse_tmp << " }";

            return stockage_chn.ajoute_chaine_statique(enchaineuse_tmp.chaine_statique());
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            auto init_tableau = atome->comme_initialisation_tableau();

            auto ancien_pour_init_tableau = pour_init_tableau;
            pour_init_tableau = true;
            auto valeur = génère_code_pour_atome(init_tableau->valeur, os, pour_globale);
            pour_init_tableau = ancien_pour_init_tableau;

            enchaineuse_tmp.réinitialise();

            /* Ne mettre qu'une seule fois la valeur suffit. */
            enchaineuse_tmp << "{ " << valeur << " }";

            if (enchaineuse_tmp.nombre_tampons() > 1) {
                auto chaine_resultat = enchaineuse_tmp.chaine();
                chaines_trop_larges_pour_stockage_chn.ajoute(chaine_resultat);
                return chaines_trop_larges_pour_stockage_chn.dernière();
            }

            return stockage_chn.ajoute_chaine_statique(enchaineuse_tmp.chaine_statique());
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            return "{}";
        }
    }

    return "";
}

/* Utilisée pour transtyper le type du deuxième argument afin de faire taire GCC (char** vs. signed
 * char**). */
static bool est_appel_init_contexte(InstructionAppel const *inst_appel)
{
    auto appelée = inst_appel->appele;
    if (!appelée->est_fonction()) {
        return false;
    }
    auto fonction = appelée->comme_fonction();
    if (!fonction->decl) {
        return false;
    }
    return fonction->decl->ident == ID::__init_contexte_kuri;
}

void GénératriceCodeC::génère_code_pour_instruction(const Instruction *inst, Enchaineuse &os)
{
    switch (inst->genre) {
        case GenreInstruction::INVALIDE:
        {
            os << "  invalide\n";
            break;
        }
        case GenreInstruction::ALLOCATION:
        {
            auto type_pointeur = inst->type->comme_type_pointeur();
            os << "  ";

            auto const alloc = inst->comme_alloc();
            auto const est_ignorée = alloc->ident == ID::_;
            if (est_ignorée) {
                os << "VARIABLE_INUTILISEE ";
            }

            os << donne_nom_pour_type(type_pointeur->type_pointe);
            auto nom = donne_nom_pour_instruction(inst);
            os << ' ' << nom;
            os << ";\n";
            table_valeurs[inst->numero] = enchaine("&", nom);
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto inst_appel = inst->comme_appel();
            auto const est_init_contexte = est_appel_init_contexte(inst_appel);

            auto arguments = kuri::tablet<kuri::chaine, 10>();

            POUR (inst_appel->args) {
                arguments.ajoute(génère_code_pour_atome(it, os, false));
            }

            os << "  ";

            auto type_fonction = inst_appel->appele->type->comme_type_fonction();
            if (!type_fonction->type_sortie->est_type_rien()) {
                auto nom_ret = donne_nom_pour_instruction(inst);
                os << donne_nom_pour_type(inst_appel->type) << ' ' << nom_ret << " = ";
                table_valeurs[inst->numero] = nom_ret;
            }

            os << génère_code_pour_atome(inst_appel->appele, os, false);

            auto virgule = "(";

            POUR_INDEX (arguments) {
                os << virgule;
                if (est_init_contexte && index_it == 1) {
                    os << "(signed char **)";
                }
                os << it;
                virgule = ", ";
            }

            if (inst_appel->args.taille() == 0) {
                os << virgule;
            }

            os << ");\n";

            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();
            os << "  goto " << nom_base_label << inst_branche->label->id << ";\n";
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            auto condition = génère_code_pour_atome(inst_branche->condition, os, false);
            os << "  if (" << condition;
            os << ") goto " << nom_base_label << inst_branche->label_si_vrai->id << "; ";
            os << " else goto " << nom_base_label << inst_branche->label_si_faux->id << ";\n";
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto inst_charge = inst->comme_charge();
            auto charge = inst_charge->chargee;
            auto valeur = génère_code_pour_atome(charge, os, false);

            assert(valeur != "");

            auto valeur_chargée = kuri::chaine_statique();
            if (valeur.pointeur()[0] == '&') {
                /* Puisque les tableaux fixes sont des structures qui ne sont que, à travers le
                 * code généré, accédées via '.', nous devons déréférencer la variable ici, mais
                 * toujours prendre l'adresse. La prise d'adresse se fera alors par rapport au
                 * membre de la structure qui est le tableau, et sert également à proprement
                 * générer le code pour les indexages. */
                if (est_pointeur_vers_tableau_fixe(
                        charge->type->comme_type_pointeur()->type_pointe)) {
                    valeur_chargée = enchaine("&(*", valeur.sous_chaine(1), ")");
                }
                else {
                    valeur_chargée = valeur.sous_chaine(1);
                }
            }
            else {
                valeur_chargée = enchaine("(*", valeur, ")");
            }

            table_valeurs[inst->numero] = valeur_chargée;
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();
            auto valeur = génère_code_pour_atome(inst_stocke->valeur, os, false);
            auto destination = inst_stocke->ou;
            auto valeur_destination = génère_code_pour_atome(destination, os, false);

            if (valeur_destination.pointeur()[0] == '&') {
                valeur_destination = valeur_destination.sous_chaine(1);
            }
            else {
                valeur_destination = enchaine("(*", valeur_destination, ")");
            }

            os << "  " << valeur_destination << " = " << valeur << ";\n";

            break;
        }
        case GenreInstruction::LABEL:
        {
            auto inst_label = inst->comme_label();
            /* Le premier label est inutile (en C). */
            if (inst_label->id != 0) {
                os << "\n" << nom_base_label << inst_label->id << ":;\n";
            }
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();
            auto valeur = génère_code_pour_atome(inst_un->valeur, os, false);
            auto nom = donne_nom_pour_instruction(inst);

            os << "  " << donne_nom_pour_type(inst_un->type) << " " << nom << " = ";

            switch (inst_un->op) {
                case OpérateurUnaire::Genre::Positif:
                {
                    break;
                }
                case OpérateurUnaire::Genre::Invalide:
                {
                    break;
                }
                case OpérateurUnaire::Genre::Complement:
                {
                    os << '-';
                    break;
                }
                case OpérateurUnaire::Genre::Non_Binaire:
                {
                    os << '~';
                    break;
                }
            }

            os << valeur;
            os << ";\n";

            table_valeurs[inst->numero] = nom;
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();
            auto valeur_gauche = génère_code_pour_atome(inst_bin->valeur_gauche, os, false);
            auto valeur_droite = génère_code_pour_atome(inst_bin->valeur_droite, os, false);
            auto nom = donne_nom_pour_instruction(inst);

            os << "  " << donne_nom_pour_type(inst_bin->type) << " " << nom << " = ";

            os << valeur_gauche;

            switch (inst_bin->op) {
                case OpérateurBinaire::Genre::Addition:
                case OpérateurBinaire::Genre::Addition_Reel:
                {
                    os << " + ";
                    break;
                }
                case OpérateurBinaire::Genre::Soustraction:
                case OpérateurBinaire::Genre::Soustraction_Reel:
                {
                    os << " - ";
                    break;
                }
                case OpérateurBinaire::Genre::Multiplication:
                case OpérateurBinaire::Genre::Multiplication_Reel:
                {
                    os << " * ";
                    break;
                }
                case OpérateurBinaire::Genre::Division_Naturel:
                case OpérateurBinaire::Genre::Division_Relatif:
                case OpérateurBinaire::Genre::Division_Reel:
                {
                    os << " / ";
                    break;
                }
                case OpérateurBinaire::Genre::Reste_Naturel:
                case OpérateurBinaire::Genre::Reste_Relatif:
                {
                    os << " % ";
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Egal:
                case OpérateurBinaire::Genre::Comp_Egal_Reel:
                {
                    os << " == ";
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Inegal:
                case OpérateurBinaire::Genre::Comp_Inegal_Reel:
                {
                    os << " != ";
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Inf:
                case OpérateurBinaire::Genre::Comp_Inf_Nat:
                case OpérateurBinaire::Genre::Comp_Inf_Reel:
                {
                    os << " < ";
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Inf_Egal:
                case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
                case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
                {
                    os << " <= ";
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Sup:
                case OpérateurBinaire::Genre::Comp_Sup_Nat:
                case OpérateurBinaire::Genre::Comp_Sup_Reel:
                {
                    os << " > ";
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Sup_Egal:
                case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
                case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
                {
                    os << " >= ";
                    break;
                }
                case OpérateurBinaire::Genre::Et_Binaire:
                {
                    os << " & ";
                    break;
                }
                case OpérateurBinaire::Genre::Ou_Binaire:
                {
                    os << " | ";
                    break;
                }
                case OpérateurBinaire::Genre::Ou_Exclusif:
                {
                    os << " ^ ";
                    break;
                }
                case OpérateurBinaire::Genre::Dec_Gauche:
                {
                    os << " << ";
                    break;
                }
                case OpérateurBinaire::Genre::Dec_Droite_Arithm:
                case OpérateurBinaire::Genre::Dec_Droite_Logique:
                {
                    os << " >> ";
                    break;
                }
                case OpérateurBinaire::Genre::Invalide:
                case OpérateurBinaire::Genre::Indexage:
                {
                    os << " invalide ";
                    break;
                }
            }

            os << valeur_droite;
            os << ";\n";

            table_valeurs[inst->numero] = nom;

            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto inst_retour = inst->comme_retour();
            if (inst_retour->valeur != nullptr) {
                auto atome = inst_retour->valeur;
                auto valeur_retour = génère_code_pour_atome(atome, os, false);
                os << "  return";
                os << ' ';
                os << valeur_retour;
            }
            else {
                os << "  return";
            }
            os << ";\n";
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            auto inst_accès = inst->comme_acces_index();
            auto valeur_accédée = génère_code_pour_atome(inst_accès->accede, os, false);
            auto valeur_index = génère_code_pour_atome(inst_accès->index, os, false);

            if (inst_accès->accede->genre_atome == Atome::Genre::GLOBALE &&
                est_globale_pour_tableau_données_constantes(inst_accès->accede->comme_globale())) {
                /* Nous devons transtyper l'adresse, qui se trouve dans les données constantes,
                 * vers le type cible. */
                valeur_accédée = enchaine(
                    "&((", donne_nom_pour_type(inst_accès->type), ")", valeur_accédée, ")");
            }
            else if (est_type_tableau_fixe(inst_accès->donne_type_accédé())) {
                valeur_accédée = enchaine(valeur_accédée, ".d");
            }

            auto valeur = enchaine(valeur_accédée, "[", valeur_index, "]");
            table_valeurs[inst->numero] = valeur;
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto inst_accès = inst->comme_acces_membre();

            auto accédée = inst_accès->accede;
            auto valeur_accédée = broyeuse.broye_nom_simple(
                génère_code_pour_atome(accédée, os, false));

            assert(valeur_accédée != "");

            if (valeur_accédée.pointeur()[0] == '&') {
                valeur_accédée = enchaine(valeur_accédée, ".");
            }
            else {
                valeur_accédée = enchaine("&", valeur_accédée, "->");
            }

            auto const &membre = inst_accès->donne_membre_accédé();

#ifdef TOUTES_LES_STRUCTURES_SONT_DES_TABLEAUX_FIXES
            auto nom_type = donne_nom_pour_type(membre.type);
            valeur_accédée = enchaine(
                "&(*(", nom_type, " *)", valeur_accédée, "d[", membre.decalage, "])");
#else
            /* Cas pour les structures vides (dans leurs fonctions d'initialisation). */
            if (membre.nom == ID::chaine_vide) {
                valeur_accédée = enchaine(valeur_accédée, "membre_invisible");
            }
            else if (inst_accès->donne_type_accédé()->est_type_tuple()) {
                valeur_accédée = enchaine(valeur_accédée, "_", index_membre);
            }
            else {
                valeur_accédée = enchaine(valeur_accédée, broyeuse.broye_nom_simple(membre.nom));
            }
#endif

            table_valeurs[inst->numero] = valeur_accédée;
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto inst_transtype = inst->comme_transtype();
            auto valeur = génère_code_pour_atome(inst_transtype->valeur, os, false);
            if (transtypage_est_utile(inst_transtype)) {
                valeur = enchaine(
                    "((", donne_nom_pour_type(inst_transtype->type), ")(", valeur, "))");
            }
            table_valeurs[inst->numero] = valeur;
            break;
        }
    }
}

void GénératriceCodeC::déclare_globale(Enchaineuse &os,
                                       const AtomeGlobale *valeur_globale,
                                       bool pour_entete)
{
    déclare_visibilité_globale(os, valeur_globale, pour_entete);

    auto type = valeur_globale->donne_type_alloué();
    os << donne_nom_pour_type(type) << ' ';

    auto nom_globale = donne_nom_pour_globale(valeur_globale, pour_entete);
    os << nom_globale;
    table_globales.insère(valeur_globale, enchaine("&", nom_globale));
}

static bool paramètre_est_marqué_comme_inutilisée(NoeudDeclarationEnteteFonction const *entête,
                                                  int index)
{
    if (!entête) {
        return false;
    }

    auto const param = entête->parametre_entree(index);
    return param->possède_drapeau(DrapeauxNoeud::EST_MARQUÉE_INUTILISÉE);
}

/* Pour garantir que les déclarations des fonctions externes correspondent à ce qu'elles doivent
 * être. */
static std::optional<kuri::chaine_statique> type_paramètre_pour_fonction_clé(
    NoeudDeclarationEnteteFonction const *entête, int index)
{
    if (!entête) {
        return {};
    }

    if (entête->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        if (entête->ident->nom == "memcpy" && index == 1) {
            return "const void *";
        }
        return {};
    }

    if (entête->ident == ID::__point_d_entree_systeme) {
        if (index == 1) {
            return "char **";
        }

        return {};
    }

    return {};
}

/* Pour une liste des attributs GCC pour les fonctions :
 * https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html
 */
void GénératriceCodeC::déclare_fonction(Enchaineuse &os,
                                        const AtomeFonction *atome_fonc,
                                        bool pour_entête)
{
    if (atome_fonc->decl &&
        atome_fonc->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE)) {
        return;
    }

#ifdef IMPRIME_COMMENTAIRE
    os << "// " << atome_fonc->nom << '\n';
#endif

    if (atome_fonc->enligne) {
        os << "static TOUJOURS_ENLIGNE ";

        if (atome_fonc->decl &&
            atome_fonc->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
            os << "__attribute__ ((nonnull (1))) ";
        }
    }
    else {
        if (atome_fonc->decl &&
            atome_fonc->decl->possède_drapeau(DrapeauxNoeudFonction::FORCE_HORSLIGNE)) {
            os << "TOUJOURS_HORSLIGNE ";
        }

        if (pour_entête && atome_fonc->decl && !atome_fonc->est_externe) {
            os << donne_chaine_pour_visibilité(atome_fonc->decl->visibilité_symbole);
        }
    }

    auto type_fonction = atome_fonc->type->comme_type_fonction();
    os << donne_nom_pour_type(type_fonction->type_sortie) << " ";
    os << donne_nom_pour_fonction(atome_fonc);

    auto virgule = "(";

    POUR_INDEX (atome_fonc->params_entrees) {
        auto est_paramètre_inutilisé = paramètre_est_marqué_comme_inutilisée(atome_fonc->decl,
                                                                             index_it);

        os << virgule;

        auto type_pointeur = it->type->comme_type_pointeur();
        auto type_param = type_pointeur->type_pointe;
        auto type_opt = type_paramètre_pour_fonction_clé(atome_fonc->decl, index_it);
        if (type_opt.has_value()) {
            os << type_opt.value();
        }
        else {
            os << donne_nom_pour_type(type_param) << ' ';
        }

        /* Dans le cas des fonctions variadiques externes, si le paramètres n'est pas typé
         * (void fonction(...)), n'imprime pas de nom. */
        if (type_param->est_type_variadique() &&
            type_param->comme_type_variadique()->type_pointe == nullptr) {
            continue;
        }

        if (est_paramètre_inutilisé) {
            os << "INUTILISE(";
        }

        os << donne_nom_pour_instruction(it);

        if (est_paramètre_inutilisé) {
            os << ")";
        }

        virgule = ", ";
    }

    if (atome_fonc->params_entrees.taille() == 0) {
        os << virgule;
    }

    os << ")";
}

void GénératriceCodeC::génère_code_entête(ProgrammeRepreInter const &repr_inter, Enchaineuse &os)
{
    /* Commençons par rassembler les tableaux de données constantes. */
    auto données_constantes = repr_inter.donne_données_constantes();
    if (données_constantes.has_value()) {
        POUR (données_constantes.value()->tableaux_constants) {
            auto nom_globale = enchaine("&DC[", it.décalage_dans_données_constantes, "]");
            table_globales.insère(it.globale, nom_globale);
        }
    }

    /* Déclarons les globales. */
    POUR (repr_inter.donne_globales()) {
        déclare_globale(os, it, true);
        os << ";\n";
    }

    génère_code_pour_tableaux_données_constantes(os, repr_inter, true);

    /* Déclarons ensuite les fonctions. */
    POUR (repr_inter.donne_fonctions()) {
        numérote_instructions(*it);
        déclare_fonction(os, it, true);
        os << ";\n\n";
    }

    /* Définissons ensuite les fonctions devant être enlignées. */
    POUR (repr_inter.donne_fonctions_enlignées()) {
        génère_code_fonction(it, os);
    }
}

void GénératriceCodeC::génère_code_fonction(AtomeFonction const *atome_fonc, Enchaineuse &os)
{
    déclare_fonction(os, atome_fonc, false);

    table_valeurs.redimensionne(atome_fonc->nombre_d_instructions_avec_entrées_sorties());

    for (auto param : atome_fonc->params_entrees) {
        table_valeurs[param->numero] = enchaine("&", donne_nom_pour_instruction(param));
    }

    os << "\n{\n";

    m_fonction_courante = atome_fonc;

    /* Créons une variable locale pour la valeur de sortie. */
    auto type_fonction = atome_fonc->type->comme_type_fonction();
    if (!type_fonction->type_sortie->est_type_rien()) {
        auto param = atome_fonc->param_sortie;
        auto type_pointeur = param->type->comme_type_pointeur();
        os << donne_nom_pour_type(type_pointeur->type_pointe) << ' ';
        os << donne_nom_pour_instruction(param);
        os << ";\n";

        table_valeurs[param->numero] = enchaine("&", donne_nom_pour_instruction(param));
    }

    /* Générons le code pour les accès de membres des retours multiples. */
    if (atome_fonc->decl && atome_fonc->decl->params_sorties.taille() > 1) {
        for (auto &param : atome_fonc->decl->params_sorties) {
            auto inst = param->comme_declaration_variable()->atome->comme_instruction();
            génère_code_pour_instruction(inst, os);
        }
    }

    for (auto inst : atome_fonc->instructions) {
        génère_code_pour_instruction(inst, os);
    }

    m_fonction_courante = nullptr;

    os << "}\n\n";
}

void GénératriceCodeC::vide_enchaineuse_dans_fichier(CoulisseC &coulisse, Enchaineuse &os)
{
    auto fichier = coulisse.ajoute_fichier_c();
    std::ofstream of;
    of.open(
        std::string(fichier.chemin_fichier.pointeur(), size_t(fichier.chemin_fichier.taille())));
    os.imprime_dans_flux(of);
    of.close();
    os.réinitialise();
}

kuri::chaine_statique GénératriceCodeC::donne_nom_pour_instruction(const Instruction *instruction)
{
    /* Puisqu'il n'y a pas de blocs dans le code généré, plusieurs variablees avec le même
     * nom mais des types différents peuvent exister dans le bloc de la fonction. Nous
     * devons rendre les noms uniques pour éviter des collisions. Nous faisons ceci en
     * ajoutant le numéro de l'instruction au nom de la variable. */
    if (préserve_symboles()) {
        if (est_allocation(instruction) && instruction->comme_alloc()->ident) {
            auto ident = instruction->comme_alloc()->ident;
            return enchaine(broyeuse.broye_nom_simple(ident), "_", instruction->numero);
        }
    }

    return enchaine(nom_base_variable, instruction->numero);
}

kuri::chaine_statique GénératriceCodeC::donne_nom_pour_globale(const AtomeGlobale *valeur_globale,
                                                               bool pour_entête)
{
    if (préserve_symboles()) {
        if (valeur_globale->ident) {
            return broyeuse.broye_nom_simple(valeur_globale->ident);
        }
    }
    else {
        if (valeur_globale->est_externe) {
            return valeur_globale->ident->nom;
        }
    }

    if (!pour_entête) {
        /* Nous devons déjà avoir généré la globale. */
        auto nom_globale = table_globales.valeur_ou(valeur_globale, "GLOBALE_INCONNUE");
        return nom_globale.sous_chaine(1);
    }

    return enchaine(nom_base_globale, nombre_de_globales++);
}

kuri::chaine_statique GénératriceCodeC::donne_nom_pour_fonction(AtomeFonction const *fonction)
{
    if (préserve_symboles()) {
        return fonction->nom;
    }

    if (fonction->est_externe ||
        fonction->decl->possède_drapeau(DrapeauxNoeudFonction::EST_RACINE)) {
        return fonction->nom;
    }

    auto trouvé = false;
    auto nom = table_fonctions.trouve(fonction, trouvé);
    if (!trouvé) {
        nom = enchaine(nom_base_fonction, nombre_de_fonctions++);
        table_fonctions.insère(fonction, nom);
    }

    return nom;
}

kuri::chaine_statique GénératriceCodeC::donne_nom_pour_type(Type const *type)
{
    if (préserve_symboles()) {
        return broyeuse.nom_broyé_type(const_cast<Type *>(type));
    }

    if (type->est_type_variadique()) {
        auto type_tableau = type->comme_type_variadique()->type_tableau_dynamique;
        if (!type_tableau) {
            return "Kv";
        }

        type = type_tableau;
    }

    auto trouvé = false;
    auto nom = table_types.trouve(type, trouvé);
    if (!trouvé) {
        nom = enchaine(nom_base_type, nombre_de_types++);
        table_types.insère(type, nom);
    }

    return nom;
}

bool GénératriceCodeC::préserve_symboles() const
{
    return m_espace.compilatrice().arguments.préserve_symboles;
}

void GénératriceCodeC::génère_code(ProgrammeRepreInter const &repr_inter,
                                   CoulisseC &coulisse,
                                   Enchaineuse &os)
{
    os.réinitialise();
    os << "#include \"compilation_kuri.h\"\n";

    génère_code_pour_tableaux_données_constantes(os, repr_inter, false);

    /* À FAIRE : les types de pointeurs de fonctions ne peuvent être convertis vers des types de
     * pointeurs d'objets. Nous devrons avoir des types distincts et supprimer *rien. En attendant,
     * désactivation de l'avertissement "pedantic" car les globales des traces d'appel prennent des
     * pointeurs de fonctions comme paramètres convertis vers *rien. */
    auto const désactive_pedantic = R"(
#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#endif
)";
    auto const active_pedantic = R"(
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
)";

    os << désactive_pedantic;

    /* Définis les globales. */
    POUR (repr_inter.donne_globales_internes()) {
        auto valeur_initialisateur = kuri::chaine_statique();

        if (it->initialisateur) {
            valeur_initialisateur = génère_code_pour_atome(it->initialisateur, os, true);
        }

        déclare_globale(os, it, false);

        if (it->initialisateur) {
            os << " = " << valeur_initialisateur;
        }

        os << ";\n";
    }
    os << active_pedantic;

    /* Vide l'enchaineuse sauf si nous compilons un fichier objet car nous devons n'avoir qu'un
     * seul fichier "*.o". */
    if (m_espace.options.resultat != ResultatCompilation::FICHIER_OBJET) {
        vide_enchaineuse_dans_fichier(coulisse, os);
        os << "#include \"compilation_kuri.h\"\n";
    }

    /* Nombre maximum d'instructions par fichier, afin d'avoir une taille cohérente entre tous les
     * fichiers. */
    constexpr auto nombre_instructions_max_par_fichier = 50000;
    int nombre_instructions = 0;

    /* Définis les fonctions. */
    POUR (repr_inter.donne_fonctions_horslignées()) {
        génère_code_fonction(it, os);
        nombre_instructions += it->instructions.taille();

        /* Vide l'enchaineuse si nous avons dépassé le maximum d'instructions, sauf si nous
         * compilons un fichier objet car nous devons n'avoir qu'un seul fichier "*.o". */
        if (m_espace.options.resultat != ResultatCompilation::FICHIER_OBJET &&
            nombre_instructions > nombre_instructions_max_par_fichier) {
            vide_enchaineuse_dans_fichier(coulisse, os);
            os << "#include \"compilation_kuri.h\"\n";
            nombre_instructions = 0;
        }
    }

    if (m_espace.options.resultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
        os << "static __attribute__((constructor)) void "
              "initialise_kuri()\n{\n__point_d_entree_dynamique();\n}\n";
        os << "static __attribute__((destructor)) void "
              "issitialise_kuri()\n{\n__point_de_sortie_dynamique();\n}\n";
    }

    if (nombre_instructions != 0) {
        vide_enchaineuse_dans_fichier(coulisse, os);
    }
}

void GénératriceCodeC::génère_code(ProgrammeRepreInter const &repr_inter_programme,
                                   CoulisseC &coulisse)
{
    Enchaineuse enchaineuse;
    ConvertisseuseTypeC convertisseuse_type_c(broyeuse, *this);
    génère_code_début_fichier(enchaineuse, m_espace.compilatrice().racine_kuri);

    POUR (repr_inter_programme.donne_types()) {
        convertisseuse_type_c.génère_typedef(it, enchaineuse);
    }

    POUR (repr_inter_programme.donne_types()) {
        convertisseuse_type_c.génère_code_pour_type(it, enchaineuse);
    }

    génère_code_entête(repr_inter_programme, enchaineuse);

    auto chemin_fichier_entete = kuri::chemin_systeme::chemin_temporaire("compilation_kuri.h");
    std::ofstream of(vers_std_path(chemin_fichier_entete));
    enchaineuse.imprime_dans_flux(of);
    of.close();

    génère_code(repr_inter_programme, coulisse, enchaineuse);
}

void GénératriceCodeC::génère_code_pour_tableaux_données_constantes(
    Enchaineuse &os, ProgrammeRepreInter const &repr_inter, bool pour_entête)
{
    auto opt_données_constantes = repr_inter.donne_données_constantes();
    if (!opt_données_constantes.has_value()) {
        return;
    }

    auto données_constantes = opt_données_constantes.value();
    POUR (données_constantes->tableaux_constants) {
        auto nom_globale = enchaine("&DC[", it.décalage_dans_données_constantes, "]");
        table_globales.insère(it.globale, nom_globale);
    }

    if (pour_entête) {
        os << "extern ";
    }

    os << "_Alignas(" << données_constantes->alignement_désiré << ") ";
    os << "const uint8_t DC[" << données_constantes->taille_données_tableaux_constants << "]";

    if (pour_entête) {
        os << ";\n";
        return;
    }

    auto virgule = " = {\n";
    auto compteur = 0;
    POUR (données_constantes->tableaux_constants) {
        auto tableau = it.tableau->donne_données();

        for (auto i = 0; i < it.rembourrage; ++i) {
            compteur++;
            os << virgule;
            if ((compteur % 20) == 0) {
                os << "\n";
            }
            os << "0x0";
            virgule = ", ";
        }

        POUR_NOMME (octet, tableau) {
            compteur++;
            os << virgule;
            if ((compteur % 20) == 0) {
                os << "\n";
            }
            os << "0x";
            os << dls::num::char_depuis_hex((octet & 0xf0) >> 4);
            os << dls::num::char_depuis_hex(octet & 0x0f);
            virgule = ", ";
        }
    }
    os << "};\n";
}

/** \} */

bool CoulisseC::génère_code_impl(const ArgsGénérationCode &args)
{
    auto &compilatrice_ri = *args.compilatrice_ri;
    auto &espace = *args.espace;
    auto const &programme = *args.programme;

    m_bibliothèques.efface();

    /* Convertis le programme sous forme de représentation intermédiaire. */
    auto repr_inter_programme = représentation_intermédiaire_programme(
        espace, compilatrice_ri, programme);

    if (!repr_inter_programme.has_value()) {
        return false;
    }

    auto génératrice = GénératriceCodeC(espace, *args.broyeuse);
    génératrice.génère_code(*repr_inter_programme, *this);
    m_bibliothèques = repr_inter_programme->donne_bibliothèques_utilisées();
    return true;
}

bool CoulisseC::crée_fichier_objet_impl(const ArgsCréationFichiersObjets &args)
{
    auto &espace = *args.espace;

#ifdef CMAKE_BUILD_TYPE_PROFILE
    return true;
#else
#    ifndef NDEBUG
    auto poule_de_tâches = kuri::PouleDeTâchesEnSérie{};
#    else
    auto poule_de_tâches = kuri::PouleDeTâchesSousProcessus{};
#    endif

    POUR (m_fichiers) {
        poule_de_tâches.ajoute_tâche([&]() {
            kuri::chaine nom_sortie = it.chemin_fichier_objet;
            if (espace.options.resultat == ResultatCompilation::FICHIER_OBJET) {
                nom_sortie = nom_sortie_resultat_final(espace.options);
            }

            auto commande = commande_pour_fichier_objet(
                espace.options, it.chemin_fichier, nom_sortie);
            auto err = exécute_commande_externe_erreur(commande);
            if (err.has_value()) {
                it.erreur_fichier_objet = err.value().message;
            }
        });
    }

    poule_de_tâches.attends_sur_tâches();

    POUR (m_fichiers) {
        if (it.erreur_fichier_objet.taille() == 0) {
            continue;
        }

        std::cerr << it.erreur_fichier_objet << '\n';
        return false;
    }

    return true;
#endif
}

bool CoulisseC::crée_exécutable_impl(const ArgsLiaisonObjets &args)
{
    auto &compilatrice = *args.compilatrice;
    auto &espace = *args.espace;

#ifdef CMAKE_BUILD_TYPE_PROFILE
    return true;
#else
    if (!compile_objet_r16(compilatrice.racine_kuri, espace.options.architecture)) {
        return false;
    }

    kuri::tablet<kuri::chaine_statique, 16> fichiers_objet;
    POUR (m_fichiers) {
        fichiers_objet.ajoute(it.chemin_fichier_objet);
    }

    auto commande = commande_pour_liaison(espace.options, fichiers_objet, m_bibliothèques);
    return exécute_commande_externe(commande);
#endif
}

CoulisseC::FichierC &CoulisseC::ajoute_fichier_c()
{
    auto nom_base_fichier = kuri::chemin_systeme::chemin_temporaire(
        enchaine("compilation_kuri", m_fichiers.taille()));

    auto nom_fichier = enchaine(nom_base_fichier, ".c");
    auto nom_fichier_objet = nom_fichier_objet_pour(nom_base_fichier);

    FichierC résultat = {nom_fichier, nom_fichier_objet};
    m_fichiers.ajoute(résultat);
    return m_fichiers.dernière();
}
