/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include <cstdio>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Weffc++"
#    pragma GCC diagnostic ignored "-Wclass-memaccess"
#    pragma GCC diagnostic ignored "-Wshadow"
#    pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#    pragma GCC diagnostic ignored "-Wuseless-cast"
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#endif
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/OperationKinds.h>
#include <clang/AST/Stmt.h>
#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif

#include <clang-c/Index.h>

#include "biblinternes/json/json.hh"
#include "biblinternes/outils/conditions.h"
#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/dico_desordonne.hh"
#include "biblinternes/structures/dico_fixe.hh"

#include "structures/chaine.hh"
#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"
#include "structures/ensemble.hh"
#include "structures/pile.hh"
#include "structures/tableau.hh"

using dls::outils::est_element;

/* À FAIRE :
 * - 'auto'
 * - 'template' (FunctionTemplate, ClassTemplate)
 * - classes : public/protected/private, si supporté dans le langage
 * - les appels de constructeurs ont une paire de parenthèse extra
 * - les structs/unions/enum déclarés avec un typedef n'ont pas le bon type
 *   p.e. typedef struct X { } X_t sera X et non X_t => le petit frère du cursor
 *   possède cette information
 * - assert est mal converti
 * - conversion typage pour les opérateurs 'new' :
 *      int i = new int[a * b];
 *      devient
 *      i : *z32 = loge [a * b]z32
 *      et non
 *      i : [..]z32 = loge [a * b]z32
 * - gestion correcte des typedefs, notamment pour typedef struct XXX { ... } XXX;
 */

/* ------------------------------------------------------------------------- */
/** \nom Configuration
 * \{ */

struct RubriqueEmployée {
    dls::chaine nom{};
    dls::chaine type{};
    dls::chaine renomme{};
};

struct Configuration {
    dls::chaine fichier{};
    dls::chaine fichier_sortie{};
    kuri::tableau<dls::chaine> args{};
    kuri::tableau<dls::chaine> inclusions{};
    kuri::tableau<dls::chaine> modules_à_importer{};
    dls::chaine nom_bibliothèque{};
    /* Dépendances sur les bibliothèques internes ; celles installées dans modules/Kuri. */
    kuri::tableau<kuri::chaine> dépendances_biblinternes{};
    /* Dépendances sur les bibliothèques internes ; celles installées dans modules/Kuri. */
    kuri::tableau<kuri::chaine> dépendances_qt{};

    kuri::chemin_systeme dossier_source{};

    bool fichier_est_composite = false;
    dls::chaine fichier_tmp{};

    kuri::tableau<RubriqueEmployée> rubriques_employées{};
    kuri::ensemble<dls::chaine> fonctions_à_ignorer{};
};

static kuri::tableau<dls::chaine> parse_tableau_de_chaines(tori::ObjetDictionnaire *dico,
                                                           dls::chaine nom)
{
    kuri::tableau<dls::chaine> résultat;

    auto tableau = cherche_tableau(dico, nom);

    if (tableau != nullptr) {
        for (auto objet : tableau->valeur) {
            if (objet->type != tori::type_objet::CHAINE) {
                std::cerr << "Objet invalide dans le tableau de \"" << nom
                          << "\", nous ne voulons que "
                             "des chaines\n";
                std::cerr << "    NOTE : le type superflux est " << tori::chaine_type(objet->type)
                          << "\n";
                exit(1);
            }

            auto obj_chaine = extrait_chaine(objet.get());
            résultat.ajoute(obj_chaine->valeur);
        }
    }

    return résultat;
}

static dls::chaine parse_chaine(tori::ObjetDictionnaire *dico, dls::chaine nom)
{
    auto objet = cherche_chaine(dico, nom);
    if (objet != nullptr) {
        return objet->valeur;
    }
    return "";
}

static auto analyse_configuration(const char *chemin)
{
    auto config = Configuration{};
    auto obj = json::compile_script(chemin);

    if (obj == nullptr) {
        std::cerr << "La compilation du script a renvoyé un objet nul !\n";
        return config;
    }

    if (obj->type != tori::type_objet::DICTIONNAIRE) {
        std::cerr << "La compilation du script n'a pas produit de dictionnaire !\n";
        return config;
    }

    auto dico = tori::extrait_dictionnaire(obj.get());

    auto obj_fichier = dico->objet("fichier");
    if (obj_fichier == nullptr) {
        return config;
    }

    if (obj_fichier->type == tori::type_objet::CHAINE) {
        config.fichier = static_cast<tori::ObjetChaine *>(obj_fichier)->valeur;
        auto chemin_fichier_source = kuri::chemin_systeme(config.fichier.c_str());
        config.dossier_source = chemin_fichier_source.chemin_parent();
    }
    else if (obj_fichier->type == tori::type_objet::TABLEAU) {
        std::stringstream ss;

        kuri::tableau<kuri::chemin_systeme> dossiers;

        auto tableau = static_cast<tori::ObjetTableau *>(obj_fichier);
        POUR (tableau->valeur) {
            if (it->type != tori::type_objet::CHAINE) {
                std::cerr << "Objet invalide dans le tableau de \"fichier\", nous ne voulons que "
                             "des chaines\n";
                std::cerr << "    NOTE : le type superflux est " << tori::chaine_type(it->type)
                          << "\n";
                return config;
            }

            auto obj_chaine = static_cast<tori::ObjetChaine *>(it.get());
            std::ifstream ifs;
            ifs.open(obj_chaine->valeur.c_str());

            if (!ifs.is_open()) {
                std::cerr << "Impossible de lire le fichier \"" << obj_chaine->valeur << "\"";
                return config;
            }

            auto chaine = dls::chaine((std::istreambuf_iterator<char>(ifs)),
                                      (std::istreambuf_iterator<char>()));
            ss << chaine;

            auto dossier_fichier =
                kuri::chemin_systeme(obj_chaine->valeur.c_str()).chemin_parent();
            dossiers.ajoute(dossier_fichier);
        }

        if (tableau->valeur.taille() == 0) {
            std::cerr << "Aucun fichier spécifié\n";
            return config;
        }

        std::stringstream ss_tmp;
        ss_tmp << "/tmp/parcer-" << obj_fichier << ".h";

        auto nom_fichier_tmp = ss_tmp.str();

        std::ofstream fichier_tmp(nom_fichier_tmp.c_str());
        fichier_tmp << ss.str();

        config.fichier = nom_fichier_tmp;
        config.fichier_tmp = nom_fichier_tmp;
        config.fichier_est_composite = true;

        auto premier_dossier = dossiers[0];
        config.dossier_source = premier_dossier;

        for (int i = 1; i < dossiers.taille(); i++) {
            if (dossiers[i] != premier_dossier) {
                config.dossier_source = kuri::chemin_systeme("/tmp/");
                break;
            }
        }
    }
    else {
        std::cerr << "La propriété \"fichier\" doit être une chaine ou un tableau de chaines\n";
        std::cerr << "    NOTE : la propriété est de type " << tori::chaine_type(obj_fichier->type)
                  << "\n";
        return config;
    }

    config.args = parse_tableau_de_chaines(dico, "args");
    config.inclusions = parse_tableau_de_chaines(dico, "inclusions");
    config.modules_à_importer = parse_tableau_de_chaines(dico, "modules_à_importer");
    config.fichier_sortie = parse_chaine(dico, "sortie");
    config.nom_bibliothèque = parse_chaine(dico, "bibliothèque");

    auto tableau_rubriques_employées = tori::cherche_tableau(dico, "rubriques_employées");
    if (tableau_rubriques_employées != nullptr) {
        kuri::tableau<RubriqueEmployée> rubriques_employées{};

        POUR (tableau_rubriques_employées->valeur) {
            if (it->type != tori::type_objet::DICTIONNAIRE) {
                std::cerr
                    << "La propriété \"rubriques_employées\" doit être un tableau d'objets\n";
                std::cerr << "    NOTE : or, nous y avons trouvé un élément de type "
                          << tori::chaine_type(it->type) << "\n";
                exit(1);
            }

            auto dictionnaire_rubrique = static_cast<tori::ObjetDictionnaire *>(it.get());

            auto rubrique_employée = RubriqueEmployée{};
            rubrique_employée.nom = parse_chaine(dictionnaire_rubrique, "nom");
            rubrique_employée.type = parse_chaine(dictionnaire_rubrique, "type");
            rubrique_employée.renomme = parse_chaine(dictionnaire_rubrique, "renomme");

            if (rubrique_employée.nom.taille() == 0) {
                std::cerr
                    << "Propriété 'nom' manquante pour un élément de \"rubriques_employées\"\n";
                exit(1);
            }

            if (rubrique_employée.type.taille() == 0) {
                std::cerr
                    << "Propriété 'type' manquante pour un élément de \"rubriques_employées\"\n";
                exit(1);
            }

            rubriques_employées.ajoute(rubrique_employée);
        }

        config.rubriques_employées = rubriques_employées;
    }

    auto fonctions_à_ignorer = parse_tableau_de_chaines(dico, "fonctions_à_ignorer");
    POUR (fonctions_à_ignorer) {
        config.fonctions_à_ignorer.insère(it);
    }

    return config;
}

static std::optional<Configuration> valide_configuration(Configuration config)
{
    if (!kuri::chemin_systeme::existe(config.fichier.c_str())) {
        std::cerr << "Le fichier \"" << config.fichier << "\" n'existe pas !\n";
        return {};
    }

    return config;
}

static std::optional<Configuration> crée_config_depuis_json(int argc, char **argv)
{
    if (argc < 3) {
        std::cerr << "Utilisation -c " << argv[0] << " CONFIG.json\n";
        return {};
    }

    auto config = analyse_configuration(argv[2]);

    if (config.fichier == "") {
        return {};
    }

    return valide_configuration(config);
}

static bool commence_par(kuri::chaine_statique chn1, kuri::chaine_statique chn2)
{
    if (chn1.taille() < chn2.taille()) {
        return false;
    }

    auto sous_chaine = chn1.sous_chaine(0, chn2.taille());
    return sous_chaine == chn2;
}

static std::optional<Configuration> crée_config_pour_metaprogramme(int argc, char **argv)
{
    if (argc < 7) {
        std::cerr
            << "Utilisation " << argv[0]
            << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri -l BIBLIOTHEQUES\n";
        return {};
    }

    auto config = Configuration{};
    config.fichier = argv[1];
    config.dossier_source = kuri::chemin_systeme(argv[1]).chemin_parent();

    if (std::string(argv[2]) != "-b") {
        std::cerr << "Utilisation " << argv[0]
                  << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri\n";
        std::cerr << "Attendu '-b' après le nom du fichier d'entrée !\n";
        return {};
    }

    if (std::string(argv[4]) != "-o") {
        std::cerr << "Utilisation " << argv[0]
                  << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri\n";
        std::cerr << "Attendu '-o' après le nom de la bibliothèque !\n";
        return {};
    }

    if (std::string(argv[6]) != "-l") {
        std::cerr << "Utilisation " << argv[0]
                  << " FICHIER_SOURCE -b NOM_BIBLIOTHEQUE -o FICHIER_SORTIE.kuri\n";
        std::cerr << "Attendu '-l' après le nom du fichier !\n";
        return {};
    }

    config.nom_bibliothèque = argv[3];
    config.fichier_sortie = argv[5];

    /* À FAIRE : termine ceci. */
#if 0
    for (int i = 7; i < argc; i++) {
        auto chn = kuri::chaine(argv[i]);
        if (commence_par(chn, "dls::")) {
            chn = chn.sous_chaine(5);

            if (chn == "algorithmes_image") {
                continue;
            }

            if (chn == "memoire") {
                chn = "mémoire";
            }

            config.dépendances_biblinternes.ajoute(enchaine("libdls_", chn));

            //    std::cerr << "biblinterne : " << chn << '\n';

            // std::cerr << "lib" << chn << " :: #bibliothèque \"bib_" << chn << "\"\n";
            //            std::cerr << "#dépendance_bibliothèque " << config.nom_bibliothèque << "
            //            lib" << chn
            //                      << "\n";
        }
        else if (commence_par(chn, "Qt5::")) {
            // chn = chn.sous_chaine(5);
            // config.dépendances_qt.ajoute(chn);
        }
        else {
            std::cerr << argv[i] << '\n';
        }
    }
#endif

    return valide_configuration(config);
}

static kuri::tableau<const char *> parse_arguments_depuis_config(Configuration const &config)
{
    auto args = kuri::tableau<const char *>();

    for (auto const &arg : config.args) {
        args.ajoute(arg.c_str());
    }

    for (auto const &inclusion : config.inclusions) {
        args.ajoute("-I");
        args.ajoute(inclusion.c_str());
    }

    return args;
}

static std::optional<Configuration> crée_configuration_depuis_arguments(int argc, char **argv)
{
    if (argc == 3 && std::string(argv[1]) == "-c") {
        return crée_config_depuis_json(argc, argv);
    }
    return crée_config_pour_metaprogramme(argc, argv);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Utilitaires
 * \{ */

static dls::chaine converti_chaine(CXString string)
{
    auto c_str = clang_getCString(string);
    auto résultat = dls::chaine();
    if (c_str && strcmp(c_str, "") != 0) {
        résultat = dls::chaine(c_str);
    }
    return résultat;
}

static dls::chaine donne_nom_fichier(CXFile file)
{
    auto comment = clang_getFileName(file);
    auto résultat = converti_chaine(comment);
    clang_disposeString(comment);
    return résultat;
}

static dls::chaine donne_commentaire(CXCursor cursor)
{
    auto comment = clang_Cursor_getRawCommentText(cursor);
    auto résultat = converti_chaine(comment);
    clang_disposeString(comment);
    return résultat;
}

static dls::chaine donne_cursor_spelling(CXCursor cursor)
{
    auto spelling = clang_getCursorSpelling(cursor);
    auto résultat = converti_chaine(spelling);
    clang_disposeString(spelling);
    return résultat;
}

static dls::chaine donne_type_spelling(CXType cursor)
{
    auto spelling = clang_getTypeSpelling(cursor);
    auto résultat = converti_chaine(spelling);
    clang_disposeString(spelling);
    return résultat;
}

static std::optional<CXType> est_type_fonction(CXType type)
{
    if (type.kind == CXTypeKind::CXType_Pointer) {
        type = clang_getPointeeType(type);
    }

    auto type_résultat = clang_getResultType(type);
    if (type_résultat.kind == CXTypeKind::CXType_Invalid) {
        return {};
    }
    return type;
}

static std::optional<CXType> donne_type_retour_si_fonction(CXType type)
{
    if (type.kind == CXTypeKind::CXType_Pointer) {
        type = clang_getPointeeType(type);
    }

    auto type_résultat = clang_getResultType(type);
    if (type_résultat.kind == CXTypeKind::CXType_Invalid) {
        return {};
    }
    return type_résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Arbre Syntaxique
 *  À FAIRE : utilise celui de kuri ?
 * \{ */

enum class TypeSyntaxème {
    DÉFAUT,
    MDOULE,
    DÉCLARATION_ÉNUM,
    DÉCLARATION_STRUCT,
    DÉCLARATION_UNION,
    DÉCLARATION_FONCTION,
    DÉCLARATION_VARIABLE,
    DÉCLARATION_CONSTANTE,
    TYPEDEF,
    TRANSTYPAGE,
    EXPRESSION,
    EXPRESSION_UNAIRE,
    EXPRESSION_BINAIRE,
};

#define DECLARE_CONTRUCTEUR_DÉFAUT(nom, type)                                                     \
    nom() : Syntaxème(TypeSyntaxème::type)                                                        \
    {                                                                                             \
    }

#define DECLARE_CONTRUCTEUR_DÉFAUT_EXPRESSION(nom, type)                                          \
    nom() : Expression(TypeSyntaxème::type)                                                       \
    {                                                                                             \
    }

struct Syntaxème {
    TypeSyntaxème type_syntaxème = TypeSyntaxème::DÉFAUT;
    dls::chaine commentaire = "";

    std::optional<CXType> type_c{};

    bool est_prodéclaration_inutile = false;

    Syntaxème(TypeSyntaxème type_) : type_syntaxème(type_)
    {
    }

    virtual ~Syntaxème() = default;
};

struct Module : public Syntaxème {
    DECLARE_CONTRUCTEUR_DÉFAUT(Module, MDOULE)

    kuri::tableau<Syntaxème *> déclarations{};
};

struct DéclarationÉnum : public Syntaxème {
    DECLARE_CONTRUCTEUR_DÉFAUT(DéclarationÉnum, DÉCLARATION_ÉNUM)

    dls::chaine nom = "";
    CXType type_sous_jacent{};
    kuri::tableau<Syntaxème *> rubriques{};
};

struct DéclarationStruct : public Syntaxème {
    DECLARE_CONTRUCTEUR_DÉFAUT(DéclarationStruct, DÉCLARATION_STRUCT)

    dls::chaine nom = "";
    kuri::tableau<Syntaxème *> rubriques{};
};

struct DéclarationUnion : public Syntaxème {
    DECLARE_CONTRUCTEUR_DÉFAUT(DéclarationUnion, DÉCLARATION_UNION)

    dls::chaine nom = "";
    kuri::tableau<Syntaxème *> rubriques{};
};

struct Expression;

struct DéclarationVariable : public Syntaxème {
    EMPECHE_COPIE(DéclarationVariable);
    DECLARE_CONTRUCTEUR_DÉFAUT(DéclarationVariable, DÉCLARATION_VARIABLE)

    /* Pour les globales. */
    CX_StorageClass storage_class{};

    bool est_employée = false;

    dls::chaine nom{};
    Expression *expression = nullptr;
};

struct DéclarationConstante : public Syntaxème {
    EMPECHE_COPIE(DéclarationConstante);
    DECLARE_CONTRUCTEUR_DÉFAUT(DéclarationConstante, DÉCLARATION_CONSTANTE)

    dls::chaine nom{};
    Expression *expression = nullptr;
};

struct DéclarationFonction : public Syntaxème {
    DECLARE_CONTRUCTEUR_DÉFAUT(DéclarationFonction, DÉCLARATION_FONCTION)

    dls::chaine nom = "";
    CXType type_sortie = {};
    bool est_inline = false;
    kuri::tableau<DéclarationVariable *> paramètres{};
};

struct Typedef : public Syntaxème {
    EMPECHE_COPIE(Typedef);
    DECLARE_CONTRUCTEUR_DÉFAUT(Typedef, TYPEDEF)

    CXType type_défini{};
    CXType type_source{};
    DéclarationFonction *type_fonction = nullptr;
};

struct Expression : public Syntaxème {
    DECLARE_CONTRUCTEUR_DÉFAUT(Expression, EXPRESSION)

    Expression(TypeSyntaxème type_syntaxème_) : Syntaxème(type_syntaxème_)
    {
    }

    dls::chaine texte{};
};

struct ExpressionUnaire : public Expression {
    EMPECHE_COPIE(ExpressionUnaire);
    DECLARE_CONTRUCTEUR_DÉFAUT_EXPRESSION(ExpressionUnaire, EXPRESSION_UNAIRE)

    Expression *opérande = nullptr;
};

struct ExpressionBinaire : public Expression {
    EMPECHE_COPIE(ExpressionBinaire);
    DECLARE_CONTRUCTEUR_DÉFAUT_EXPRESSION(ExpressionBinaire, EXPRESSION_BINAIRE)

    Expression *gauche = nullptr;
    Expression *droite = nullptr;
};

struct Transtypage : public Expression {
    EMPECHE_COPIE(Transtypage);
    DECLARE_CONTRUCTEUR_DÉFAUT_EXPRESSION(Transtypage, TRANSTYPAGE)

    Expression *expression = nullptr;
    CXType type_vers{};
};

struct Syntaxeuse {
    kuri::tableau<Syntaxème *> syntaxèmes{};
    kuri::tableau<DéclarationStruct *> toutes_les_structures{};

    Module *module = nullptr;

    kuri::pile<Syntaxème *> noeud_courant{};

    ~Syntaxeuse()
    {
        POUR (syntaxèmes) {
            delete it;
        }
    }

    template <typename T>
    T *crée()
    {
        auto résultat = new T();
        syntaxèmes.ajoute(résultat);
        return résultat;
    }

    template <typename T>
    T *crée(CXCursor cursor)
    {
        auto résultat = this->crée<T>();
        résultat->commentaire = donne_commentaire(cursor);
        return résultat;
    }

    Expression *crée_expression(dls::chaine texte)
    {
        auto résultat = this->crée<Expression>();
        résultat->texte = texte;
        return résultat;
    }

    void ajoute_au_noeud_courant(Syntaxème *syntaxème)
    {
        auto parent = noeud_courant.haut();

        if (parent->type_syntaxème == TypeSyntaxème::MDOULE) {
            module->déclarations.ajoute(syntaxème);
        }
        else if (parent->type_syntaxème == TypeSyntaxème::DÉCLARATION_STRUCT) {
            auto structure = static_cast<DéclarationStruct *>(parent);
            structure->rubriques.ajoute(syntaxème);
        }
        else if (parent->type_syntaxème == TypeSyntaxème::DÉCLARATION_ÉNUM) {
            auto énum = static_cast<DéclarationÉnum *>(parent);
            énum->rubriques.ajoute(syntaxème);
        }
        else if (parent->type_syntaxème == TypeSyntaxème::DÉCLARATION_UNION) {
            auto union_ = static_cast<DéclarationUnion *>(parent);
            union_->rubriques.ajoute(syntaxème);
        }
    }
};

/** \} */

std::ostream &operator<<(std::ostream &stream, const CXString &str)
{
    stream << clang_getCString(str);
    clang_disposeString(str);
    return stream;
}

/* En fonction de là où ils apparaissent, les types anonymes sont de la forme :
 * ({anonymous|unnamed} at FILE:POS)
 * ou
 * ({anonymous|unnamed} {struct|union|enum} at FILE:POS)
 *
 * Donc nous utilions "FILE:POS)" comme « nom » pour les insérer et les trouver
 * dans la liste des typedefs afin de ne pas avoir à ce soucier de la
 * possibilité d'avoir un mot-clé dans la chaine.
 */
static dls::chaine trouve_nom_anonyme(dls::chaine chn)
{
    auto pos_anonymous = chn.trouve("(anonymous");

    if (pos_anonymous == -1) {
        pos_anonymous = chn.trouve("(unnamed");
        if (pos_anonymous == -1) {
            return "";
        }
    }

    auto pos_slash = chn.trouve_premier_de('/');
    return chn.sous_chaine(pos_slash);
}

using dico_typedefs = dls::dico_desordonne<dls::chaine, kuri::tableau<dls::chaine>>;

static dls::chaine converti_type(CXType const &type, dico_typedefs const &typedefs);

static dls::chaine convertis_type_fonction(CXType const &type, dico_typedefs const &typedefs)
{
    auto nombre_args = clang_getNumArgTypes(type);

    auto flux = std::stringstream();
    flux << "fonc(";

    for (int i = 0; i < nombre_args; i++) {
        if (i > 0) {
            flux << ", ";
        }
        flux << converti_type(clang_getArgType(type, uint32_t(i)), typedefs);
    }

    flux << ")(" << converti_type(clang_getResultType(type), typedefs) << ")";

    return flux.str();
}

static dls::chaine converti_type(CXType const &type, dico_typedefs const &typedefs)
{
    static auto dico_type = dls::cree_dico(dls::paire{CXType_Void, dls::vue_chaine("rien")},
                                           dls::paire{CXType_Bool, dls::vue_chaine("bool")},
                                           dls::paire{CXType_Char_U, dls::vue_chaine("n8")},
                                           dls::paire{CXType_UChar, dls::vue_chaine("n8")},
                                           dls::paire{CXType_UShort, dls::vue_chaine("n16")},
                                           dls::paire{CXType_UInt, dls::vue_chaine("n32")},
                                           dls::paire{CXType_ULong, dls::vue_chaine("n64")},
                                           dls::paire{CXType_ULongLong, dls::vue_chaine("n64")},
                                           dls::paire{CXType_Char_S, dls::vue_chaine("z8")},
                                           dls::paire{CXType_SChar, dls::vue_chaine("z8")},
                                           dls::paire{CXType_Short, dls::vue_chaine("z16")},
                                           dls::paire{CXType_Int, dls::vue_chaine("z32")},
                                           dls::paire{CXType_Long, dls::vue_chaine("z64")},
                                           dls::paire{CXType_LongLong, dls::vue_chaine("z64")},
                                           dls::paire{CXType_Float, dls::vue_chaine("r32")},
                                           dls::paire{CXType_Double, dls::vue_chaine("r64")},
                                           dls::paire{CXType_LongDouble, dls::vue_chaine("r64")});

    auto kind = type.kind;

    auto plg_type = dico_type.trouve(kind);

    if (!plg_type.est_finie()) {
        return plg_type.front().second;
    }

    auto type_fonction_opt = est_type_fonction(type);
    if (type_fonction_opt.has_value()) {
        return convertis_type_fonction(type_fonction_opt.value(), typedefs);
    }

    auto flux = std::stringstream();

    switch (kind) {
        default:
        {
            flux << "(cas défaut) " << kind << " : " << donne_type_spelling(type);
            break;
        }
        case CXType_Invalid:
        {
            flux << "invalide";
            break;
        }
        case CXType_Pointer: /* p.e. float * */
        {
            flux << "*" << converti_type(clang_getPointeeType(type), typedefs);
            break;
        }
        case CXType_ConstantArray: /* p.e. float [4] */
        {
            auto taille = clang_getArraySize(type);
            flux << "[" << taille << "]";
            flux << converti_type(clang_getArrayElementType(type), typedefs);
            break;
        }
        case CXType_LValueReference: /* p.e. float & */
        {
            flux << "&" << converti_type(clang_getPointeeType(type), typedefs);
            break;
        }
        case CXType_Typedef:
        {
            auto spelling = donne_type_spelling(type);
            if (spelling.trouve("const ") == 0) {
                spelling = spelling.sous_chaine(6);
            }

            if (spelling == "bool" || spelling == "r16") {
                return spelling;
            }

            /* Détecte typedef int32_t int, etc., mais uniquement pour les types primitifs de C. */
            auto canonique = clang_getCanonicalType(type);
            plg_type = dico_type.trouve(canonique.kind);
            if (!plg_type.est_finie()) {
                if (typedefs.trouve(spelling) != typedefs.fin()) {
                    return plg_type.front().second;
                }
            }

            return spelling;
        }
        case CXType_IncompleteArray: /* p.e. float [] */
        {
            flux << "*" << converti_type(clang_getArrayElementType(type), typedefs);
            break;
        }
        case CXType_Auto:
        case CXType_Enum:
        case CXType_Record:     /* p.e. struct Vecteur */
        case CXType_Elaborated: /* p.e. struct Vecteur */
        {
            /* pour les types anonymes */
            auto nom_anonymous = trouve_nom_anonyme(donne_type_spelling(type));

            if (nom_anonymous != "") {
                auto iter_typedefs = typedefs.trouve(nom_anonymous);

                if (iter_typedefs != typedefs.fin()) {
                    return iter_typedefs->second[0];
                }

                return nom_anonymous;
            }

            auto spelling = donne_type_spelling(type);
            if (spelling.trouve("const ") == 0) {
                spelling = spelling.sous_chaine(6);
            }

            if (spelling.trouve("struct ") == 0) {
                spelling = spelling.sous_chaine(7);
                if (spelling == "stat") {
                    return "struct_stat";
                }
                return spelling;
            }
            if (spelling.trouve("union ") == 0) {
                return spelling.sous_chaine(6);
            }
            if (spelling.trouve("enum ") == 0) {
                return spelling.sous_chaine(5);
            }
            std::cerr << "Impossible de convertir le type nommé '" << spelling << "'\n";
            exit(1);
        }
    }

    return flux.str();
}

static auto rassemble_enfants(CXCursor cursor)
{
    auto enfants = kuri::tableau<CXCursor>();

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) {
            auto ptr_enfants = static_cast<kuri::tableau<CXCursor> *>(client_data);
            ptr_enfants->ajoute(c);
            return CXChildVisit_Continue;
        },
        &enfants);

    return enfants;
}

void imprime_asa(CXCursor c, int tab, std::ostream &os)
{
    for (auto i = 0; i < tab; ++i) {
        os << ' ' << ' ';
    }

    os << "Cursor '" << clang_getCursorSpelling(c) << "' of kind '"
       << clang_getCursorKindSpelling(clang_getCursorKind(c)) << "' of type '"
       << clang_getTypeSpelling(clang_getCursorType(c)) << "'\n";

    auto enfants = rassemble_enfants(c);

    for (auto enfant : enfants) {
        imprime_asa(enfant, tab + 1, os);
    }
}

static inline const clang::Stmt *getCursorStmt(CXCursor c)
{
    auto est_stmt = !est_element(
        c.kind, CXCursor_ObjCSuperClassRef, CXCursor_ObjCProtocolRef, CXCursor_ObjCClassRef);

    return est_stmt ? static_cast<const clang::Stmt *>(c.data[1]) : nullptr;
}

static inline const clang::Expr *getCursorExpr(CXCursor c)
{
    return clang::dyn_cast_or_null<clang::Expr>(getCursorStmt(c));
}

static dls::chaine determine_operateur_binaire(CXCursor cursor, CXTranslationUnit trans_unit)
{
    /* Méthode tirée de
     * https://www.mail-archive.com/cfe-commits@cs.uiuc.edu/msg95414.html
     * https://github.com/llvm-mirror/clang/blob/master/tools/libclang/CXCursor.cpp
     * https://github.com/pybee/sealang/blob/f4c1b0a9f3203912b6367d8de4ab7508517e60ef/sealang/sealang.cpp
     */
    auto expr = getCursorExpr(cursor);

    if (expr != nullptr) {
        auto op = clang::cast<clang::BinaryOperator>(expr);
        return op->getOpcodeStr().str();
    }

    /* Si la méthode au-dessus échoue, utilise celle-ci tirée de
     * https://stackoverflow.com/questions/23227812/get-operator-type-for-cxcursor-binaryoperator
     */

    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    dls::chaine résultat;

    for (unsigned i = 0; i < nombre_tokens; i++) {
        auto loc_tok = clang_getTokenLocation(trans_unit, tokens[i]);
        auto loc_cur = clang_getCursorLocation(cursor);

        if (clang_equalLocations(loc_cur, loc_tok) == 0) {
            CXString s = clang_getTokenSpelling(trans_unit, tokens[i]);
            résultat = converti_chaine(s);
            break;
        }
    }

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);
    return résultat;
}

static auto est_operateur_unaire(CXString const &str)
{
    auto c_str = clang_getCString(str);

    const char *operateurs[] = {"+", "-", "++", "--", "!", "&", "~", "*"};

    for (auto op : operateurs) {
        if (strcmp(c_str, op) == 0) {
            return true;
        }
    }

    return false;
}

static auto determine_operateur_unaire(CXCursor cursor, CXTranslationUnit trans_unit)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        return dls::chaine();
    }

    auto spelling = clang_getTokenSpelling(trans_unit, tokens[0]);

    /* les opérateurs post-fix sont après */
    if (!est_operateur_unaire(spelling)) {
        clang_disposeString(spelling);

        spelling = clang_getTokenSpelling(trans_unit, tokens[nombre_tokens - 1]);
    }

    dls::chaine chn = clang_getCString(spelling);
    clang_disposeString(spelling);

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    return chn;
}

static auto determine_expression_unaire(CXCursor cursor, CXTranslationUnit trans_unit)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        return dls::chaine();
    }

    auto spelling = clang_getTokenSpelling(trans_unit, tokens[0]);

    dls::chaine chn = clang_getCString(spelling);
    clang_disposeString(spelling);

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    return chn;
}

// https://stackoverflow.com/questions/10692015/libclang-get-primitive-value
static dls::chaine donne_chaine_pour_litérale(CXCursor cursor, CXTranslationUnit trans_unit)
{
    auto expr = getCursorExpr(cursor);

    if (expr != nullptr) {
        if (cursor.kind == CXCursorKind::CXCursor_IntegerLiteral) {
            auto op = clang::cast<clang::IntegerLiteral>(expr);
            std::stringstream stream;
            stream << op->getValue().getLimitedValue();
            return stream.str();
        }
    }

    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        return donne_cursor_spelling(cursor);
    }

    dls::chaine résultat;

    if (cursor.kind == CXCursorKind::CXCursor_CXXBoolLiteralExpr) {
        CXString s = clang_getTokenSpelling(trans_unit, tokens[0]);
        const char *str = clang_getCString(s);
        résultat = ((strcmp(str, "true") == 0) ? "vrai" : "faux");
        clang_disposeString(s);
    }
    else if (cursor.kind == CXCursorKind::CXCursor_FloatingLiteral) {
        /* il faut se débarasser du 'f' final */
        auto s = clang_getTokenSpelling(trans_unit, tokens[0]);
        const char *str = clang_getCString(s);
        auto len = strlen(str);

        if (str[len - 1] == 'f') {
            len = len - 1;
        }

        résultat = dls::chaine(str, int64_t(len));
        clang_disposeString(s);
    }
    else {
        résultat = converti_chaine(clang_getTokenSpelling(trans_unit, tokens[0]));
    }

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);
    return résultat;
}

static auto determine_nom_anomyme(CXCursor cursor, dico_typedefs &typedefs, int &nombre_anonyme)
{
    auto spelling = donne_cursor_spelling(cursor);
    if (spelling.taille()) {
        return spelling;
    }

    /* le type peut avoir l'information : typedef struct {} TYPE */
    spelling = donne_type_spelling(clang_getCursorType(cursor));

    if (spelling != "") {
        auto nom_anonymous = trouve_nom_anonyme(spelling);

        if (nom_anonymous != "") {
            auto nom = "anonyme" + dls::vers_chaine(nombre_anonyme++);
            kuri::tableau<dls::chaine, int64_t> tabl;
            tabl.ajoute(nom);
            typedefs.insere({nom_anonymous, tabl});
            return nom;
        }

        return spelling;
    }

    return "anonyme" + dls::vers_chaine(nombre_anonyme++);
}

static auto compare_token(CXToken token, CXTranslationUnit trans_unit, const char *str)
{
    auto spelling = clang_getTokenSpelling(trans_unit, token);
    auto c_str = clang_getCString(spelling);
    auto ok = strcmp(c_str, str) == 0;
    clang_disposeString(spelling);
    return ok;
}

static auto trouve_decalage(CXToken *tokens,
                            unsigned nombre_tokens,
                            int decalage,
                            CXTranslationUnit trans_unit)
{
    if (compare_token(tokens[decalage], trans_unit, ";")) {
        return 0;
    }

    auto dec = 0;

    for (auto i = decalage; i < nombre_tokens; ++i) {
        if (compare_token(tokens[i], trans_unit, ";")) {
            break;
        }

        dec += 1;
    }

    return dec;
}

static auto tokens_typealias(CXCursor cursor, CXTranslationUnit trans_unit, dico_typedefs &dico)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        clang_disposeTokens(trans_unit, tokens, nombre_tokens);
        return;
    }

    auto nom = converti_chaine(clang_getTokenSpelling(trans_unit, tokens[1]));
    auto morceaux = kuri::tableau<dls::chaine>();

    for (auto i = 3u; i < nombre_tokens; ++i) {
        auto spelling = clang_getTokenSpelling(trans_unit, tokens[i]);
        morceaux.ajoute(converti_chaine(spelling));
        std::cerr << morceaux.dernier_élément();
    }
    std::cerr << '\n';

    dico.insere({nom, morceaux});

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);
}

struct EnfantsBoucleFor {
    CXCursor const *enfant_init = nullptr;
    CXCursor const *enfant_comp = nullptr;
    CXCursor const *enfant_inc = nullptr;
    CXCursor const *enfant_bloc = nullptr;
};

static EnfantsBoucleFor determine_enfants_for(CXCursor cursor,
                                              CXTranslationUnit trans_unit,
                                              kuri::tableau<CXCursor> const &enfants)
{
    CXSourceRange range = clang_getCursorExtent(cursor);
    CXToken *tokens = nullptr;
    unsigned nombre_tokens = 0;
    clang_tokenize(trans_unit, range, &tokens, &nombre_tokens);

    if (tokens == nullptr) {
        return {};
    }

    auto res = EnfantsBoucleFor{};
    auto decalage_enfant = 0;

    auto decalage = 2;

    auto dec = trouve_decalage(tokens, nombre_tokens, decalage, trans_unit);

    if (dec == 0) {
        decalage += 1;
    }
    else {
        res.enfant_init = &enfants[decalage_enfant++];
        decalage += dec + 1;
    }

    dec = trouve_decalage(tokens, nombre_tokens, decalage, trans_unit);

    if (dec == 0) {
        decalage += 1;
    }
    else {
        res.enfant_comp = &enfants[decalage_enfant++];
        decalage += dec + 1;
    }

    if (!compare_token(tokens[decalage], trans_unit, ")")) {
        res.enfant_inc = &enfants[decalage_enfant++];
    }

    clang_disposeTokens(trans_unit, tokens, nombre_tokens);

    res.enfant_bloc = &enfants[decalage_enfant];

    return res;
}

static dls::chaine donne_préfixe_valeur_énum(dls::chaine const &nom_énum)
{
    auto résultat = nom_énum;
    if (résultat[résultat.taille() - 1] != '_') {
        résultat += "_";
    }

    for (auto &c : résultat) {
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }
    }

    return résultat;
}

static dls::chaine donne_nom_constante_énum_sans_préfixe(dls::chaine const &nom_constante,
                                                         dls::chaine const &préfixe_énum)
{
    if (nom_constante.taille() < préfixe_énum.taille()) {
        return nom_constante;
    }

    auto préfixe_potentiel = nom_constante.sous_chaine(0, préfixe_énum.taille());
    if (préfixe_potentiel != préfixe_énum) {
        return nom_constante;
    }

    auto résultat = nom_constante.sous_chaine(préfixe_potentiel.taille());
    if (résultat.taille() == 0) {
        return nom_constante;
    }

    return résultat;
}

struct Convertisseuse {
    Syntaxeuse syntaxeuse{};
    kuri::chemin_systeme fichier_source{};
    kuri::chemin_systeme fichier_entete{};

    int profondeur = 0;
    /* pour les structures, unions, et énumérations anonymes */
    int nombre_anonymes = 0;

    kuri::pile<dls::chaine> noms_structure{};

    dico_typedefs typedefs{};

    kuri::ensemble<CXCursorKind> cursors_non_pris_en_charges{};
    kuri::ensemble<kuri::chaine> fichiers_ignorés{};

    kuri::ensemble<kuri::chaine> modules_importes{};

    kuri::ensemble<dls::chaine> commentaires_imprimés{};

    dls::chaine nom_bibliothèque_sûr{};

    Configuration *config = nullptr;

    dls::chaine m_préfixe_énum_courant{};

    void ajoute_typedef(dls::chaine &&nom_typedef, dls::chaine &&nom_type)
    {
        kuri::tableau<dls::chaine> tabl;
        tabl.ajoute(nom_type);
        typedefs.insere({nom_typedef, tabl});
    }

    auto imprime_commentaire(CXCursor cursor, std::ostream &os)
    {
        auto comment = clang_Cursor_getRawCommentText(cursor);
        auto c_str = clang_getCString(comment);

        if (c_str != nullptr) {
            auto chn = dls::chaine(c_str);

            if (chn != "") {
                imprime_tab(os);
                os << chn << '\n';
            }
        }

        clang_disposeString(comment);
    }

    void rapporte_cursor_non_pris_en_charge(CXCursor cursor, std::ostream &flux_sortie)
    {
        cursors_non_pris_en_charges.insère(clang_getCursorKind(cursor));
        auto loc = clang_getCursorLocation(cursor);
        CXFile file;
        unsigned line;
        unsigned column;
        unsigned offset;
        clang_getExpansionLocation(loc, &file, &line, &column, &offset);

        flux_sortie << donne_nom_fichier(file) << ":" << line << ":" << column << ":\n";
        flux_sortie << "Cursor '" << clang_getCursorSpelling(cursor) << "' of kind '"
                    << clang_getCursorKindSpelling(clang_getCursorKind(cursor)) << "' of type '"
                    << clang_getTypeSpelling(clang_getCursorType(cursor)) << "'\n";
    }

    void convertis(CXTranslationUnit trans_unit, std::ostream &flux_sortie)
    {
        CXCursor cursor = clang_getTranslationUnitCursor(trans_unit);
        // imprime_asa(cursor, 0, std::cout);

        auto module = syntaxeuse.crée<Module>();
        syntaxeuse.module = module;
        syntaxeuse.noeud_courant.empile(module);
        convertis(cursor, trans_unit, flux_sortie);
        assert(syntaxeuse.noeud_courant.haut() == syntaxeuse.module);

        marque_prodéclarations_inutiles();
        marque_rubriques_employées();

        POUR (config->modules_à_importer) {
            flux_sortie << "importe " << it << "\n";
        }
        if (config->modules_à_importer.taille()) {
            flux_sortie << "\n";
        }

        if (config->nom_bibliothèque != "") {
            flux_sortie << "lib" << nom_bibliothèque_sûr << " :: #bibliothèque \""
                        << config->nom_bibliothèque << "\"\n\n";

            for (auto &dép : config->dépendances_biblinternes) {
                flux_sortie << "#dépendance_bibliothèque lib" << nom_bibliothèque_sûr << " " << dép
                            << "\n";
            }
            if (!config->dépendances_biblinternes.est_vide()) {
                flux_sortie << "\n";
            }
            for (auto &dép : config->dépendances_qt) {
                flux_sortie << "libQt5" << dép << " :: #bibliothèque \"Qt5" << dép << "\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << nom_bibliothèque_sûr << " libQt5"
                            << dép << "\n";
            }
            if (!config->dépendances_qt.est_vide()) {
                flux_sortie << "libQt5Core :: #bibliothèque \"Qt5Core\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << nom_bibliothèque_sûr
                            << " libQt5Core\n";
                flux_sortie << "\n";
                flux_sortie << "libQt5Gui :: #bibliothèque \"Qt5Gui\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << nom_bibliothèque_sûr
                            << " libQt5Gui\n";
                flux_sortie << "\n";
                flux_sortie << "libqt_entetes :: #bibliothèque \"qt_entetes\"\n";
                flux_sortie << "#dépendance_bibliothèque lib" << nom_bibliothèque_sûr
                            << " libqt_entetes\n";
                flux_sortie << "\n";
            }
        }

        ajoute_imports_pour_structures(syntaxeuse.module, flux_sortie);

        profondeur = -2;
        imprime_arbre(syntaxeuse.module, flux_sortie);
    }

    bool doit_ignorer_fichier(kuri::chemin_systeme chemin_fichier)
    {
        if (chemin_fichier == fichier_source || chemin_fichier == fichier_entete) {
            return false;
        }

        auto chemin_parent = kuri::chaine(chemin_fichier.chemin_parent());
        while (chemin_parent.taille() != 0 && chemin_parent != "/") {
            if (chemin_parent == kuri::chaine_statique(config->dossier_source)) {
                return false;
            }

            chemin_parent = kuri::chemin_systeme(chemin_parent).chemin_parent();
        }

        return true;
    }

    void convertis(CXCursor cursor, CXTranslationUnit trans_unit, std::ostream &flux_sortie)
    {
        switch (cursor.kind) {
            default:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_InclusionDirective:
            case CXCursorKind::CXCursor_MacroExpansion:
            {
                break;
            }
            case CXCursorKind::CXCursor_MacroDefinition:
            {
                // if (!clang_Cursor_isMacroFunctionLike(cursor)) {
                //     imprime_asa(cursor, 0, std::cerr);
                // }
                break;
            }
            case CXCursorKind::CXCursor_Namespace:
            case CXCursorKind::CXCursor_TranslationUnit:
            {
                /* À FAIRE : conversion correcte des espaces de nom, cela
                 * demandera peut-être de savoir comment bien déclarer les
                 * modules et espaces de noms dans Kuri. */
                auto enfants = rassemble_enfants(cursor);

                for (auto enfant : enfants) {
                    auto loc = clang_getCursorLocation(enfant);
                    CXFile file;
                    unsigned line;
                    unsigned column;
                    unsigned offset;
                    clang_getExpansionLocation(loc, &file, &line, &column, &offset);

                    auto nom_fichier = donne_nom_fichier(file);
                    auto nom_fichier_c = kuri::chemin_systeme(
                        kuri::chaine_statique(nom_fichier.c_str(), nom_fichier.taille()));

                    //  À FAIRE: option pour controler ceci.
                    if (doit_ignorer_fichier(nom_fichier_c)) {
                        fichiers_ignorés.insère(kuri::chaine_statique(nom_fichier_c));
                        continue;
                    }

                    convertis(enfant, trans_unit, flux_sortie);
                }

                break;
            }
            case CXCursorKind::CXCursor_StructDecl:
            case CXCursorKind::CXCursor_ClassDecl:
            {
                auto enfants = rassemble_enfants(cursor);

                auto enfants_filtres = kuri::tableau<CXCursor>();

                for (auto enfant : enfants) {
                    if (enfant.kind == CXCursorKind::CXCursor_VisibilityAttr) {
                        continue;
                    }

                    enfants_filtres.ajoute(enfant);
                }

                auto structure = syntaxeuse.crée<DéclarationStruct>(cursor);
                structure->nom = determine_nom_anomyme(cursor, typedefs, nombre_anonymes);

                if (!enfants_filtres.est_vide()) {
                    syntaxeuse.noeud_courant.empile(structure);

                    int64_t index_enfant = 0;
                    while (index_enfant < enfants_filtres.taille()) {
                        auto enfant = enfants_filtres[index_enfant];
                        index_enfant += 1;

                        auto bit_field = clang_Cursor_isBitField(enfant);
                        // clang_getFieldDeclBitWidth(enfant)
                        if (bit_field != 0) {
                            while (index_enfant < enfants_filtres.taille()) {
                                auto enfant2 = enfants_filtres[index_enfant];
                                if (!clang_Cursor_isBitField(enfant2)) {
                                    break;
                                }
                                // À FAIRE : vérifie que le type est le même
                                index_enfant += 1;
                            }

                            auto variable = syntaxeuse.crée<DéclarationVariable>(cursor);
                            variable->nom = "bitfield" + dls::vers_chaine(index_enfant);
                            variable->type_c = clang_getCursorType(enfant);

                            structure->rubriques.ajoute(variable);
                            continue;
                        }

                        convertis(enfant, trans_unit, flux_sortie);
                    }

                    syntaxeuse.noeud_courant.depile();
                }

                if (structure->rubriques.taille() == 0) {
                    // Prodéclaration, ajoute au module.
                    syntaxeuse.module->déclarations.ajoute(structure);
                }
                else {
                    // À FAIRE : assert(est_anonyme) si le noeud courant n'est pas le module ?
                    syntaxeuse.ajoute_au_noeud_courant(structure);
                }

                syntaxeuse.toutes_les_structures.ajoute(structure);
                break;
            }
            case CXCursorKind::CXCursor_UnionDecl:
            {
                auto union_ = syntaxeuse.crée<DéclarationUnion>(cursor);
                union_->nom = determine_nom_anomyme(cursor, typedefs, nombre_anonymes);

                syntaxeuse.noeud_courant.empile(union_);
                converti_enfants(cursor, trans_unit, flux_sortie);
                syntaxeuse.noeud_courant.depile();

                syntaxeuse.ajoute_au_noeud_courant(union_);
                break;
            }
            case CXCursorKind::CXCursor_FieldDecl:
            {
                auto variable = syntaxeuse.crée<DéclarationVariable>(cursor);
                variable->nom = donne_cursor_spelling(cursor);
                variable->type_c = clang_getCursorType(cursor);
                syntaxeuse.ajoute_au_noeud_courant(variable);
                break;
            }
            case CXCursorKind::CXCursor_EnumDecl:
            {
                auto énum = syntaxeuse.crée<DéclarationÉnum>(cursor);
                énum->nom = determine_nom_anomyme(cursor, typedefs, nombre_anonymes);
                énum->type_sous_jacent = clang_getEnumDeclIntegerType(cursor);

                syntaxeuse.noeud_courant.empile(énum);
                converti_enfants(cursor, trans_unit, flux_sortie);
                syntaxeuse.noeud_courant.depile();

                syntaxeuse.ajoute_au_noeud_courant(énum);
                break;
            }
            case CXCursorKind::CXCursor_EnumConstantDecl:
            {
                auto déclaration = syntaxeuse.crée<DéclarationConstante>(cursor);
                déclaration->nom = donne_cursor_spelling(cursor);

                auto enfants = rassemble_enfants(cursor);
                if (!enfants.est_vide()) {
                    assert(enfants.taille() == 1);
                    déclaration->expression = parse_expression(
                        enfants[0], trans_unit, flux_sortie);
                }

                auto noeud_courant = syntaxeuse.noeud_courant.haut();
                if (noeud_courant->type_syntaxème != TypeSyntaxème::DÉCLARATION_ÉNUM) {
                    std::cerr << "Le noeud courant n'est celui d'une déclaration d'énum\n";
                    exit(1);
                }

                static_cast<DéclarationÉnum *>(noeud_courant)->rubriques.ajoute(déclaration);
                break;
            }
            case CXCursorKind::CXCursor_FunctionDecl:
            {
                converti_declaration_fonction(cursor, trans_unit, false, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_Constructor:
            case CXCursorKind::CXCursor_Destructor:
            case CXCursorKind::CXCursor_CXXMethod:
            {
                converti_declaration_fonction(cursor, trans_unit, true, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_TypeRef:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                /* pour les constructeurs entre autres */
                flux_sortie << clang_getTypeSpelling(clang_getCursorType(cursor));
#endif
                break;
            }
            case CXCursorKind::CXCursor_CXXAccessSpecifier:
            {
#if 0
                auto acces = clang_getCXXAccessSpecifier(cursor);

                switch (acces) {
                    case CX_CXXInvalidAccessSpecifier:
                    {
                        break;
                    }
                    case CX_CXXPublic:
                    {
                        break;
                    }
                    case CX_CXXProtected:
                    {
                        break;
                    }
                    case CX_CXXPrivate:
                    {
                        break;
                    }
                }
#endif
                break;
            }
            case CXCursorKind::CXCursor_TypedefDecl:
            {
                /* typedef a b;
                 * clang_getCursorType(cursor) => b
                 * clang_getTypedefDeclUnderlyingType(cursor) => a
                 */
                auto type_source = clang_getTypedefDeclUnderlyingType(cursor);
                auto type_défini = clang_getCursorType(cursor);

                auto typedef_ = syntaxeuse.crée<Typedef>(cursor);
                typedef_->type_défini = type_défini;

                auto type_résultat_opt = donne_type_retour_si_fonction(type_source);
                if (type_résultat_opt.has_value()) {
                    auto fonction = syntaxeuse.crée<DéclarationFonction>();
                    fonction->type_sortie = type_résultat_opt.value();

                    auto enfants = rassemble_enfants(cursor);

                    POUR (enfants) {
                        if (it.kind != CXCursorKind::CXCursor_ParmDecl) {
                            continue;
                        }

                        auto variable = syntaxeuse.crée<DéclarationVariable>();
                        variable->nom = donne_cursor_spelling(it);
                        variable->type_c = clang_getCursorType(it);
                        fonction->paramètres.ajoute(variable);
                    }

                    typedef_->type_fonction = fonction;
                }
                else {
                    typedef_->type_source = type_source;
                }

                syntaxeuse.ajoute_au_noeud_courant(typedef_);
                break;
            }
            case CXCursorKind::CXCursor_TypeAliasDecl:
            {
                tokens_typealias(cursor, trans_unit, typedefs);
                break;
            }
            case CXCursorKind::CXCursor_DeclStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                imprime_commentaire(cursor, flux_sortie);

                /* Un DeclStmt peut être :
                 * soit int x = 0;
                 * soit int x = 0, y = 0, z = 0;
                 *
                 * Dans le deuxième cas, la virgule n'est pas considérée comme
                 * un opérateur binaire, et les différentes expressions sont
                 * les filles du DeclStmt. Donc pour proprement tenir en compte
                 * ce cas, on rassemble et converti les enfants en insérant un
                 * point-virgule quand nécessaire.
                 */
                auto enfants = rassemble_enfants(cursor);

                for (auto i = 0; i < enfants.taille(); ++i) {
                    convertis(enfants[i], trans_unit, flux_sortie);

                    if (enfants.taille() > 1 && i < enfants.taille() - 1) {
                        flux_sortie << '\n';
                        --profondeur;
                        imprime_tab(flux_sortie);
                        ++profondeur;
                    }
                }
#endif
                break;
            }
            case CXCursorKind::CXCursor_VarDecl:
            {
                auto enfants = rassemble_enfants(cursor);
                auto cxtype = clang_getCursorType(cursor);

                auto nombre_enfants = enfants.taille();
                auto decalage = 0;

                if (cxtype.kind == CXTypeKind::CXType_ConstantArray) {
                    /* le premier enfant est la taille du tableau */
                    nombre_enfants -= 1;
                    decalage += 1;
                }

                /* les variables déclarées comme étant des pointeurs de
                 * fonctions ont les types des arguments comme enfants */
                for (auto const &enfant : enfants) {
                    switch (enfant.kind) {
                        default:
                        {
                            break;
                        }
                        /* pour certaines déclarations dans les codes C, le premier
                         * enfant semble être une référence vers le type
                         * (p.e. struct Vecteur) */
                        case CXCursorKind::CXCursor_TypeRef:
                        case CXCursorKind::CXCursor_ParmDecl:
                        case CXCursorKind::CXCursor_VisibilityAttr:
                        {
                            decalage += 1;
                            nombre_enfants -= 1;
                        }
                    }
                }

                if (nombre_enfants > 1) {
                    std::cerr << "Trop d'expressions pour la variable "
                              << donne_cursor_spelling(cursor) << " : " << nombre_enfants << "\n";
                    exit(1);
                }

                auto variable = syntaxeuse.crée<DéclarationVariable>(cursor);
                variable->nom = donne_cursor_spelling(cursor);
                variable->type_c = clang_getCursorType(cursor);
                variable->storage_class = clang_Cursor_getStorageClass(cursor);

                if (nombre_enfants == 1) {
                    variable->expression = parse_expression(
                        enfants[decalage], trans_unit, flux_sortie);
                }
                syntaxeuse.ajoute_au_noeud_courant(variable);
                break;
            }
        }
    }

    Expression *parse_expression(CXCursor cursor,
                                 CXTranslationUnit trans_unit,
                                 std::ostream &flux_sortie)
    {
        switch (cursor.kind) {
            default:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
                break;
            }
            case CXCursorKind::CXCursor_CXXThisExpr:
            {
                return syntaxeuse.crée_expression("this");
            }
            case CXCursorKind::CXCursor_CallExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                /* le premier enfant nous fournis soit le nom de la fonction,
                 * soit l'expression this.nom si la fonction est appelée depuis
                 * une méthode de classe */
                if (enfants.taille() > 0) {
                    convertis(enfants[0], trans_unit, flux_sortie);
                }
                else {
                    flux_sortie << clang_getCursorSpelling(cursor);
                }

                auto virgule = "(";

                for (auto i = 1; i < enfants.taille(); ++i) {
                    flux_sortie << virgule;
                    convertis(enfants[i], trans_unit, flux_sortie);
                    virgule = ", ";
                }

                /* pour les constructeurs implicites, il n'y a pas de premier enfant */
                if (enfants.taille() <= 1) {
                    flux_sortie << '(';
                }

                flux_sortie << ')';
#endif
                break;
            }
            case CXCursorKind::CXCursor_InitListExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                auto virgule = "[ ";

                for (auto enfant : enfants) {
                    flux_sortie << virgule;
                    convertis(enfant, trans_unit, flux_sortie);
                    virgule = ", ";
                }

                flux_sortie << " ]";
#endif
                break;
            }
            case CXCursorKind::CXCursor_CompoundStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                /* NOTE : un CompoundStmt correspond à un bloc, et peut donc contenir plusieurs
                 * instructions, par exemple :
                 *
                 * int a = 0;
                 * int b = 5;
                 * retourne a + b;
                 *
                 * est un CompoundStmt, et non trois différentes instructions.
                 */

                auto enfants = rassemble_enfants(cursor);
                auto debut = true;

                for (auto enfant : enfants) {
                    auto besoin_nouvelle_ligne = est_element(enfant.kind,
                                                             CXCursorKind::CXCursor_IfStmt,
                                                             CXCursorKind::CXCursor_WhileStmt,
                                                             CXCursorKind::CXCursor_ForStmt,
                                                             CXCursorKind::CXCursor_DoStmt,
                                                             CXCursorKind::CXCursor_ReturnStmt);

                    if (besoin_nouvelle_ligne && !debut) {
                        flux_sortie << '\n';
                    }

                    debut = false;

                    imprime_tab(flux_sortie);
                    convertis(enfant, trans_unit, flux_sortie);

                    flux_sortie << '\n';
                }
#endif
                break;
            }
            case CXCursorKind::CXCursor_IfStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "si ";
                convertis(enfants[0], trans_unit, flux_sortie);

                flux_sortie << " {\n";
                auto non_compound = enfants[1].kind != CXCursorKind::CXCursor_CompoundStmt;

                if (non_compound) {
                    imprime_tab(flux_sortie);
                }

                convertis(enfants[1], trans_unit, flux_sortie);

                if (non_compound) {
                    flux_sortie << '\n';
                }

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}";

                if (enfants.taille() == 3) {
                    flux_sortie << "\n";
                    --profondeur;
                    imprime_tab(flux_sortie);
                    ++profondeur;
                    flux_sortie << "sinon ";
                    if (enfants[2].kind == CXCursorKind::CXCursor_IfStmt) {
                        convertis(enfants[2], trans_unit, flux_sortie);
                    }
                    else {
                        flux_sortie << "{\n";
                        non_compound = enfants[2].kind != CXCursorKind::CXCursor_CompoundStmt;

                        if (non_compound) {
                            imprime_tab(flux_sortie);
                        }

                        convertis(enfants[2], trans_unit, flux_sortie);

                        if (non_compound) {
                            flux_sortie << '\n';
                        }

                        --profondeur;
                        imprime_tab(flux_sortie);
                        ++profondeur;
                        flux_sortie << "}";
                    }
                }
#endif
                break;
            }
            case CXCursorKind::CXCursor_WhileStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "tantque ";
                convertis(enfants[0], trans_unit, flux_sortie);

                flux_sortie << " {\n";
                --profondeur;
                convertis(enfants[1], trans_unit, flux_sortie);
                imprime_tab(flux_sortie);
                ++profondeur;

                flux_sortie << "}";
#endif
                break;
            }
            case CXCursorKind::CXCursor_DoStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "répète {\n";
                --profondeur;
                convertis(enfants[0], trans_unit, flux_sortie);
                imprime_tab(flux_sortie);
                ++profondeur;

                flux_sortie << "} tantque ";
                convertis(enfants[1], trans_unit, flux_sortie);
#endif
                break;
            }
            case CXCursorKind::CXCursor_ForStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                /* Transforme :
                 * for (int i = 0; i < 10; ++i) {
                 *		...
                 * }
                 *
                 * en :
                 *
                 * i = 0
                 * tantque i < 10 {
                 *		...
                 *		i += 1
                 * }
                 */
                auto enfants = rassemble_enfants(cursor);
                auto enfants_for = determine_enfants_for(cursor, trans_unit, enfants);

                /* int i = 0 */
                if (enfants_for.enfant_init) {
                    flux_sortie << "dyn ";
                    convertis(*enfants_for.enfant_init, trans_unit, flux_sortie);
                    flux_sortie << '\n';
                }

                /* i < 10 */
                if (enfants_for.enfant_comp) {
                    --profondeur;
                    imprime_tab(flux_sortie);
                    ++profondeur;
                    flux_sortie << "tantque ";
                    convertis(*enfants_for.enfant_comp, trans_unit, flux_sortie);
                    flux_sortie << " {\n";
                }
                else {
                    --profondeur;
                    imprime_tab(flux_sortie);
                    ++profondeur;
                    flux_sortie << "boucle {\n";
                }

                /* ... */
                if (enfants_for.enfant_bloc) {
                    convertis(*enfants_for.enfant_bloc, trans_unit, flux_sortie);
                }

                /* ++i */
                if (enfants_for.enfant_inc) {
                    imprime_tab(flux_sortie);
                    convertis(*enfants_for.enfant_inc, trans_unit, flux_sortie);
                    flux_sortie << '\n';
                }

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}";
#endif
                break;
            }
            case CXCursorKind::CXCursor_BreakStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                flux_sortie << "arrête";
#endif
                break;
            }
            case CXCursorKind::CXCursor_ContinueStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                flux_sortie << "continue";
#endif
                break;
            }
            case CXCursorKind::CXCursor_ReturnStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                flux_sortie << "retourne ";
                converti_enfants(cursor, trans_unit, flux_sortie);
#endif
                break;
            }
            case CXCursorKind::CXCursor_SwitchStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "discr ";
                convertis(enfants[0], trans_unit, flux_sortie);
                flux_sortie << " {\n";
                convertis(enfants[1], trans_unit, flux_sortie);

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}\n";
#endif
                break;
            }
            case CXCursorKind::CXCursor_DefaultStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                /* À FAIRE : gestion propre du cas défaut, le langage possède un
                 * 'sinon' qu'il faudra utiliser correctement */
                converti_enfants(cursor, trans_unit, flux_sortie);
#endif
                break;
            }
            case CXCursorKind::CXCursor_CaseStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                /* L'arbre des cas est ainsi :
                 * case
                 *	valeur
                 *	case | bloc
                 *
                 * Le case | bloc dépend de si on a plusieurs cas successif avec
                 * le même bloc, p.e. :
                 *
                 * case 0:
                 * case 1:
                 * case 2:
                 * {
                 *	...
                 * }
                 *
                 * donc nous rassemblons tous les enfants récursivement jusqu'à
                 * ce que nous ayons un bloc, qui sera l'enfant restant
                 */

                auto enfants = rassemble_enfants(cursor);

                /* valeur */
                convertis(enfants[0], trans_unit, flux_sortie);

                auto cas_similaires = kuri::tableau<CXCursor>();

                /* case | bloc */
                auto enfant = enfants[1];

                while (enfant.kind == CXCursorKind::CXCursor_CaseStmt) {
                    auto petits_enfants = rassemble_enfants(enfant);
                    cas_similaires.ajoute(petits_enfants[0]);
                    enfant = petits_enfants[1];
                }

                for (auto const &cas : cas_similaires) {
                    flux_sortie << ", ";
                    convertis(cas, trans_unit, flux_sortie);
                }

                flux_sortie << " {\n";
                convertis(enfant, trans_unit, flux_sortie);

                --profondeur;
                imprime_tab(flux_sortie);
                ++profondeur;
                flux_sortie << "}\n";
#endif
                break;
            }
            case CXCursorKind::CXCursor_ParenExpr:
            {
                auto paren = syntaxeuse.crée<ExpressionUnaire>(cursor);
                paren->texte = "(";

                auto enfants = rassemble_enfants(cursor);
                if (enfants.taille() != 1) {
                    std::cerr << "L'expression de parenthèse a plusieurs enfants : "
                              << enfants.taille() << "\n";
                    exit(1);
                }

                paren->opérande = parse_expression(enfants[0], trans_unit, flux_sortie);
                return paren;
            }
            case CXCursorKind::CXCursor_IntegerLiteral:
            case CXCursorKind::CXCursor_CharacterLiteral:
            case CXCursorKind::CXCursor_StringLiteral:
            case CXCursorKind::CXCursor_FloatingLiteral:
            case CXCursorKind::CXCursor_CXXBoolLiteralExpr:
            {
                auto chn = donne_chaine_pour_litérale(cursor, trans_unit);
                return syntaxeuse.crée_expression(chn);
            }
            case CXCursorKind::CXCursor_GNUNullExpr:
            case CXCursorKind::CXCursor_CXXNullPtrLiteralExpr:
            {
                return syntaxeuse.crée_expression("nul");
            }
            case CXCursorKind::CXCursor_ArraySubscriptExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);
                assert(enfants.taille() == 2);

                convertis(enfants[0], trans_unit, flux_sortie);
                flux_sortie << '[';
                convertis(enfants[1], trans_unit, flux_sortie);
                flux_sortie << ']';
#endif
                break;
            }
            case CXCursorKind::CXCursor_MemberRefExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                if (enfants.taille() == 1) {
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << '.';
                    flux_sortie << clang_getCursorSpelling(cursor);
                }
                else {
                    /* this implicit */
                    flux_sortie << "this.";
                    flux_sortie << clang_getCursorSpelling(cursor);
                }
#endif
                break;
            }
            case CXCursorKind::CXCursor_BinaryOperator:
            case CXCursorKind::CXCursor_CompoundAssignOperator:
            {
                auto enfants = rassemble_enfants(cursor);
                assert(enfants.taille() == 2);

                auto binaire = syntaxeuse.crée<ExpressionBinaire>(cursor);
                binaire->texte = determine_operateur_binaire(cursor, trans_unit);
                binaire->gauche = parse_expression(enfants[0], trans_unit, flux_sortie);
                binaire->droite = parse_expression(enfants[1], trans_unit, flux_sortie);
                return binaire;
            }
            case CXCursorKind::CXCursor_UnaryOperator:
            {
                auto enfants = rassemble_enfants(cursor);
                if (enfants.taille() != 1) {
                    std::cerr << "L'expression unaire a plusieurs enfants : " << enfants.taille()
                              << "\n";
                    exit(1);
                }

                auto chn = determine_operateur_unaire(cursor, trans_unit);

                if (chn == "++" || chn == "--") {
                    rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
                    break;
                }

                auto unaire = syntaxeuse.crée<ExpressionUnaire>(cursor);

                if (chn == "*") {
                    unaire->texte = "mémoire";
                }
                else if (chn == "&") {
                    unaire->texte = "*";
                }
                else {
                    unaire->texte = chn;
                }

                unaire->opérande = parse_expression(enfants[0], trans_unit, flux_sortie);
                return unaire;
            }
            case CXCursorKind::CXCursor_ConditionalOperator:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);
                assert(enfants.taille() == 3);

                flux_sortie << "si ";
                convertis(enfants[0], trans_unit, flux_sortie);
                flux_sortie << " { ";
                convertis(enfants[1], trans_unit, flux_sortie);
                flux_sortie << " } sinon { ";
                convertis(enfants[2], trans_unit, flux_sortie);
                flux_sortie << " } ";
#endif
                break;
            }
            case CXCursorKind::CXCursor_DeclRefExpr:
            {
                return syntaxeuse.crée_expression(donne_cursor_spelling(cursor));
            }
            case CXCursorKind::CXCursor_UnexposedExpr:
            {
                auto enfants = rassemble_enfants(cursor);
                if (enfants.taille() != 1) {
                    std::cerr << "Plusieurs enfants sur UnexposedExpr : " << enfants.taille()
                              << "\n";
                    exit(1);
                }
                return parse_expression(enfants[0], trans_unit, flux_sortie);
            }
            case CXCursorKind::CXCursor_UnexposedDecl:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                converti_enfants(cursor, trans_unit, flux_sortie);
#endif
                break;
            }
            case CXCursorKind::CXCursor_CStyleCastExpr:
            case CXCursorKind::CXCursor_CXXFunctionalCastExpr:
            case CXCursorKind::CXCursor_CXXStaticCastExpr:
            case CXCursorKind::CXCursor_CXXDynamicCastExpr:
            case CXCursorKind::CXCursor_CXXConstCastExpr:
            case CXCursorKind::CXCursor_CXXReinterpretCastExpr:
            {
                auto transtypage = syntaxeuse.crée<Transtypage>(cursor);

                auto enfants = rassemble_enfants(cursor);

                if (enfants.taille() == 1) {
                    transtypage->expression = parse_expression(
                        enfants[0], trans_unit, flux_sortie);
                    transtypage->type_vers = clang_getCursorType(cursor);
                }
                else if (enfants.taille() == 2) {
                    /* par exemple :
                     * - static_cast<decltype(a)>(b)
                     * - (typeof(a))(b)
                     */

                    transtypage->expression = parse_expression(
                        enfants[1], trans_unit, flux_sortie);
                    transtypage->type_vers = clang_getCursorType(enfants[0]);
                }

                return transtypage;
            }
            case CXCursorKind::CXCursor_UnaryExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto chn = determine_expression_unaire(cursor, trans_unit);

                if (chn == "sizeof") {
                    flux_sortie << "taille_de(";
                    flux_sortie << converti_type_sizeof(cursor, trans_unit, typedefs);
                    flux_sortie << ")";
                }
                else {
                    converti_enfants(cursor, trans_unit, flux_sortie);
                }
#endif
                break;
            }
            case CXCursorKind::CXCursor_StmtExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                converti_enfants(cursor, trans_unit, flux_sortie);
#endif
                break;
            }
            case CXCursorKind::CXCursor_CXXFinalAttr:
            case CXCursorKind::CXCursor_CXXOverrideAttr:
            case CXCursorKind::CXCursor_PackedAttr:
            case CXCursorKind::CXCursor_VisibilityAttr:
            {
                /* ignore pour le moment les attributs */
                break;
            }
            case CXCursorKind::CXCursor_NullStmt:
            {
                /* les lignes ne consistant que d'un ';' */
                break;
            }
            case CXCursorKind::CXCursor_CXXNewExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);
                auto cxtype = clang_getCursorType(cursor);

                flux_sortie << "loge ";

                if (enfants.est_vide()) {
                    flux_sortie << converti_type(cxtype, typedefs, true);
                }
                else {
                    /* tableau */
                    flux_sortie << '[';
                    convertis(enfants[0], trans_unit, flux_sortie);
                    flux_sortie << ']';
                    flux_sortie << converti_type(cxtype, typedefs, true);
                }
#endif
                break;
            }
            case CXCursorKind::CXCursor_CXXDeleteExpr:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                flux_sortie << "déloge ";
                converti_enfants(cursor, trans_unit, flux_sortie);
#endif
                break;
            }
            case CXCursorKind::CXCursor_CXXForRangeStmt:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                auto enfants = rassemble_enfants(cursor);

                flux_sortie << "pour ";
                flux_sortie << clang_getCursorSpelling(enfants[0]);
                flux_sortie << " dans ";
                flux_sortie << clang_getCursorSpelling(enfants[1]);
                flux_sortie << " {\n";
                convertis(enfants[2], trans_unit, flux_sortie);
                imprime_tab(flux_sortie);
                flux_sortie << "}\n";
#endif
                break;
            }
            case CXCursorKind::CXCursor_NamespaceRef:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                flux_sortie << clang_getCursorSpelling(cursor) << '.';
#endif
                break;
            }
            case CXCursorKind::CXCursor_TemplateRef:
            {
                rapporte_cursor_non_pris_en_charge(cursor, flux_sortie);
#if 0
                flux_sortie << clang_getCursorSpelling(cursor) << '.';
#endif
                break;
            }
        }
        return nullptr;
    }

    void imprime_tab(std::ostream &flux_sortie)
    {
        for (auto i = 0; i < profondeur; ++i) {
            flux_sortie << "    ";
        }
    }

    void converti_enfants(CXCursor cursor, CXTranslationUnit trans_unit, std::ostream &flux_sortie)
    {
        auto enfants = rassemble_enfants(cursor);
        converti_enfants(enfants, trans_unit, flux_sortie);
    }

    void converti_enfants(kuri::tableau<CXCursor> const &enfants,
                          CXTranslationUnit trans_unit,
                          std::ostream &flux_sortie)
    {
        for (auto enfant : enfants) {
            convertis(enfant, trans_unit, flux_sortie);
        }
    }

    void converti_declaration_fonction(CXCursor cursor,
                                       CXTranslationUnit trans_unit,
                                       bool est_methode_cpp,
                                       std::ostream &flux_sortie)
    {
        auto enfants = rassemble_enfants(cursor);

        bool est_declaration = enfants.est_vide();
        CXCursor enfant_bloc;

        if (!est_declaration) {
            est_declaration = enfants.dernier_élément().kind !=
                              CXCursorKind::CXCursor_CompoundStmt;

            if (!est_declaration) {
                /* Nous n'avons pas une déclaration */
                enfant_bloc = enfants.dernier_élément();
                enfants.supprime_dernier();
            }
        }

        auto nom_fonction = donne_cursor_spelling(cursor);
        if (config->fonctions_à_ignorer.possède(nom_fonction)) {
            return;
        }

        auto fonction = syntaxeuse.crée<DéclarationFonction>(cursor);
        fonction->nom = nom_fonction;
        fonction->type_sortie = clang_getCursorResultType(cursor);
        fonction->est_inline = clang_Cursor_isFunctionInlined(cursor);

#if 0
        if (!noms_structure.est_vide()) {
            flux_sortie << virgule;
            flux_sortie << "this : *" << noms_structure.haut();
        }
#endif

        for (auto i = 0; i < enfants.taille(); ++i) {
            auto param = enfants[i];

            if (est_methode_cpp && param.kind == CXCursorKind::CXCursor_TypeRef) {
#if 0
                flux_sortie << virgule;
                flux_sortie << "this : &";
                flux_sortie << converti_type(param, typedefs);
#endif
                continue;
            }

            /* les premiers enfants peuvent être des infos sur la fonctions */
            if (param.kind != CXCursorKind::CXCursor_ParmDecl) {
                continue;
            }

            auto variable = syntaxeuse.crée<DéclarationVariable>(param);
            variable->nom = donne_cursor_spelling(param);
            if (variable->nom == "") {
                variable->nom = "anonyme" + dls::vers_chaine(i);
            }
            variable->type_c = clang_getCursorType(param);
            fonction->paramètres.ajoute(variable);
        }

#if 0
        if (!est_declaration) {
            flux_sortie << '\n';
            flux_sortie << "{\n";
            convertis(enfant_bloc, trans_unit, flux_sortie);
            flux_sortie << "}\n\n";
        }
#endif

        syntaxeuse.ajoute_au_noeud_courant(fonction);
    }

    void imprime_arbre(Syntaxème *syntaxème, std::ostream &os)
    {
        if (!syntaxème) {
            return;
        }

        profondeur += 1;

        if (syntaxème->commentaire.taille() != 0) {
            /* Les typedefs et les structures qu'ils définissent ont les mêmes commentaires. */
            if (!commentaires_imprimés.possède(syntaxème->commentaire)) {
                imprime_tab(os);
                os << syntaxème->commentaire << '\n';
                commentaires_imprimés.insère(syntaxème->commentaire);
            }
        }

        switch (syntaxème->type_syntaxème) {
            case TypeSyntaxème::DÉFAUT:
            {
                os << "!!!!!!!!!!!!!!!! Erreur\n";
                break;
            }
            case TypeSyntaxème::MDOULE:
            {
                auto module = static_cast<Module *>(syntaxème);
                POUR (module->déclarations) {
                    if (doit_ignorer_déclaration(it)) {
                        continue;
                    }

                    imprime_arbre(it, os);
                    os << "\n";
                }
                break;
            }
            case TypeSyntaxème::DÉCLARATION_ÉNUM:
            {
                auto énum = static_cast<DéclarationÉnum *>(syntaxème);

                imprime_tab(os);
                os << énum->nom << " :: énum ";
                os << converti_type(énum->type_sous_jacent, typedefs);
                os << " {\n";

                m_préfixe_énum_courant = donne_préfixe_valeur_énum(énum->nom);
                POUR (énum->rubriques) {
                    imprime_arbre(it, os);
                }
                m_préfixe_énum_courant = "";

                imprime_tab(os);
                os << "}\n";
                break;
            }
            case TypeSyntaxème::DÉCLARATION_STRUCT:
            {
                auto structure = static_cast<DéclarationStruct *>(syntaxème);

                imprime_tab(os);
                os << structure->nom << " :: struct #externe";

                if (structure->rubriques.taille() == 0) {
                    os << "\n";
                    break;
                }

                os << " {\n";

                POUR (structure->rubriques) {
                    if (it->est_prodéclaration_inutile) {
                        continue;
                    }
                    imprime_arbre(it, os);
                }

                imprime_tab(os);
                os << "}\n";
                break;
            }
            case TypeSyntaxème::DÉCLARATION_UNION:
            {
                auto union_ = static_cast<DéclarationUnion *>(syntaxème);

                imprime_tab(os);
                os << union_->nom << " :: union nonsûr {\n";

                POUR (union_->rubriques) {
                    imprime_arbre(it, os);
                }

                imprime_tab(os);
                os << "}\n";
                break;
            }
            case TypeSyntaxème::DÉCLARATION_FONCTION:
            {
                auto fonction = static_cast<DéclarationFonction *>(syntaxème);

                imprime_tab(os);
                os << fonction->nom << " :: fonc ";

                kuri::chaine_statique virgule = "(";
                POUR (fonction->paramètres) {
                    os << virgule << it->nom << ": "
                       << converti_type(it->type_c.value(), typedefs);
                    virgule = ", ";
                }

                if (fonction->paramètres.taille() == 0) {
                    os << virgule;
                }

                os << ") -> " << converti_type(fonction->type_sortie, typedefs);
                os << " #externe lib" << nom_bibliothèque_sûr << "\n";
                break;
            }
            case TypeSyntaxème::DÉCLARATION_VARIABLE:
            {
                auto variable = static_cast<DéclarationVariable *>(syntaxème);

                imprime_tab(os);

                if (variable->est_employée) {
                    os << "empl ";
                }

                os << variable->nom << (variable->expression ? " : " : ": ");
                os << converti_type(variable->type_c.value(), typedefs);

                if (variable->expression) {
                    os << " = ";
                    imprime_arbre(variable->expression, os);
                }

                if (variable->storage_class == CX_StorageClass::CX_SC_Extern) {
                    os << " #externe lib" << nom_bibliothèque_sûr;
                }

                os << "\n";
                break;
            }
            case TypeSyntaxème::DÉCLARATION_CONSTANTE:
            {
                auto constante = static_cast<DéclarationConstante *>(syntaxème);

                imprime_tab(os);

                os << donne_nom_constante_énum_sans_préfixe(constante->nom,
                                                            m_préfixe_énum_courant);

                if (constante->expression) {
                    os << " :: ";
                    imprime_arbre(constante->expression, os);
                }

                os << "\n";
                break;
            }
            case TypeSyntaxème::TYPEDEF:
            {
                auto typedef_ = static_cast<Typedef *>(syntaxème);

                imprime_tab(os);
                os << converti_type(typedef_->type_défini, typedefs) << " :: ";

                if (typedef_->type_fonction) {
                    auto fonction = typedef_->type_fonction;

                    kuri::chaine_statique virgule = "fonc(";
                    POUR (fonction->paramètres) {
                        os << virgule << converti_type(it->type_c.value(), typedefs);
                        virgule = ", ";
                    }

                    if (fonction->paramètres.taille() == 0) {
                        os << virgule;
                    }

                    os << ")(" << converti_type(fonction->type_sortie, typedefs) << ")";
                }
                else {
                    os << converti_type(typedef_->type_source, typedefs);
                }

                os << "\n";
                break;
            }
            case TypeSyntaxème::TRANSTYPAGE:
            {
                auto transtypage = static_cast<Transtypage *>(syntaxème);

                imprime_arbre(transtypage->expression, os);
                os << " comme " << converti_type(transtypage->type_vers, typedefs);
                break;
            }
            case TypeSyntaxème::EXPRESSION:
            {
                auto expression = static_cast<Expression *>(syntaxème);
                os << donne_nom_constante_énum_sans_préfixe(expression->texte,
                                                            m_préfixe_énum_courant);
                break;
            }
            case TypeSyntaxème::EXPRESSION_UNAIRE:
            {
                auto unaire = static_cast<ExpressionUnaire *>(syntaxème);
                if (unaire->texte == "(") {
                    os << "(";
                    imprime_arbre(unaire->opérande, os);
                    os << ")";
                }
                else if (unaire->texte == "mémoire") {
                    os << "mémoire(";
                    imprime_arbre(unaire->opérande, os);
                    os << ")";
                }
                else {
                    os << unaire->texte;
                    imprime_arbre(unaire->opérande, os);
                }
                break;
            }
            case TypeSyntaxème::EXPRESSION_BINAIRE:
            {
                auto binaire = static_cast<ExpressionBinaire *>(syntaxème);
                imprime_arbre(binaire->gauche, os);
                os << ' ' << binaire->texte << ' ';
                imprime_arbre(binaire->droite, os);
                break;
            }
        }

        profondeur -= 1;
    }

    bool doit_ignorer_déclaration(Syntaxème *syntaxème)
    {
        if (syntaxème->est_prodéclaration_inutile) {
            return true;
        }

        if (syntaxème->type_syntaxème == TypeSyntaxème::TYPEDEF) {
            auto typedef_ = static_cast<Typedef *>(syntaxème);

            auto nom_type_défini = converti_type(typedef_->type_défini, typedefs);
            if (nom_type_défini == "bool" || nom_type_défini == "r16") {
                return true;
            }

            if (!typedef_->type_fonction) {
                auto nom_type_source = converti_type(typedef_->type_source, typedefs);
                if (nom_type_source == nom_type_défini) {
                    /* Par exemple : typedef struct XYZ { } XYZ; */
                    return true;
                }
            }

            return false;
        }

        if (syntaxème->type_syntaxème == TypeSyntaxème::DÉCLARATION_STRUCT) {
            auto structure = static_cast<DéclarationStruct *>(syntaxème);
            return structure->nom == "ContexteKuri";
        }

        return false;
    }

    void ajoute_imports_pour_structures(Module *module, std::ostream &flux_sortie)
    {
        POUR (module->déclarations) {
            if (it->type_syntaxème != TypeSyntaxème::DÉCLARATION_STRUCT) {
                continue;
            }

            auto structure = static_cast<DéclarationStruct *>(it);
            auto nom = structure->nom;

            if (structure->rubriques.taille() != 0) {
                /* N'importe les modules que si la structure est externe. */
                continue;
            }

            // À FAIRE : paramétrise ceci
            if (nom == "AdaptriceMaillage" || nom == "Interruptrice" ||
                nom == "ContexteEvaluation") {
                if (!modules_importes.possède("Géométrie3D")) {
                    flux_sortie << "importe Géométrie3D\n\n";
                    modules_importes.insère("Géométrie3D");
                }
            }
        }
    }

    void marque_prodéclarations_inutiles()
    {
        kuri::tableau<DéclarationStruct *> structures;

        for (int64_t i = 0; i < syntaxeuse.toutes_les_structures.taille(); i++) {
            auto structure1 = syntaxeuse.toutes_les_structures[i];
            if (structure1->nom.trouve("anonyme") == 0) {
                continue;
            }

            auto trouvée = false;

            POUR (structures) {
                if (it->nom != structure1->nom) {
                    continue;
                }

                trouvée = true;

                if (it->rubriques.taille() == structure1->rubriques.taille()) {
                    if (it->rubriques.taille() != 0) {
                        std::cerr << "Redéfinition de la structure '" << it->nom
                                  << "' avec des rubriques\n";
                        exit(1);
                    }

                    structure1->est_prodéclaration_inutile = true;
                    break;
                }

                if (it->rubriques.taille() == 0) {
                    it->est_prodéclaration_inutile = true;
                    it = structure1;
                    break;
                }

                structure1->est_prodéclaration_inutile = true;
                break;
            }

            if (!trouvée) {
                structures.ajoute(structure1);
            }
        }
    }

    void marque_rubriques_employées()
    {
        if (config->rubriques_employées.taille() == 0) {
            return;
        }

        for (auto &structure : syntaxeuse.toutes_les_structures) {
            for (auto &rubrique : structure->rubriques) {
                if (rubrique->type_syntaxème != TypeSyntaxème::DÉCLARATION_VARIABLE) {
                    continue;
                }

                auto variable = static_cast<DéclarationVariable *>(rubrique);

                POUR (config->rubriques_employées) {
                    if (it.nom == variable->nom && variable->type_c.has_value() &&
                        converti_type(variable->type_c.value(), typedefs) == it.type) {
                        variable->est_employée = true;
                        if (it.renomme != "") {
                            variable->nom = it.renomme;
                        }
                        break;
                    }
                }
            }
        }
    }
};

void imprime_ligne(std::string tampon, uint32_t ligne, uint32_t colonne, uint32_t decalage)
{
    int ligne_courante = 1;
    size_t position_ligne = 0;

    while (ligne_courante != ligne) {
        if (tampon[position_ligne] == '\n') {
            ligne_courante += 1;
        }

        position_ligne += 1;
    }

    size_t position_fin_ligne = position_ligne;

    while (ligne_courante != ligne + 1) {
        if (tampon[position_fin_ligne] == '\n') {
            ligne_courante += 1;
        }

        position_fin_ligne += 1;
    }

    std::cerr << ligne << ":" << colonne << " |"
              << std::string(&tampon[position_ligne], position_fin_ligne - position_ligne);
}

int main(int argc, char **argv)
{
    auto config_optionnelle = crée_configuration_depuis_arguments(argc, argv);

    if (!config_optionnelle.has_value()) {
        return 1;
    }

    auto config = config_optionnelle.value();
    auto args = parse_arguments_depuis_config(config);

    args.ajoute("-fparse-all-comments");

    CXIndex index = clang_createIndex(0, 0);
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        config.fichier.c_str(),
        args.données(),
        static_cast<int>(args.taille()),
        nullptr,
        0,
        CXTranslationUnit_None | CXTranslationUnit_DetailedPreprocessingRecord |
            CXTranslationUnit_IncludeBriefCommentsInCodeCompletion);

    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting.\n";
        exit(-1);
    }

    auto fichier_source = kuri::chemin_systeme(config.fichier.c_str());
    auto fichier_entete = fichier_source;
    fichier_entete = fichier_entete.remplace_extension(".hh");

    if (!kuri::chemin_systeme::existe(fichier_entete)) {
        fichier_entete = fichier_entete.remplace_extension(".h");

        if (!kuri::chemin_systeme::existe(fichier_entete)) {
            fichier_entete = "";
        }
    }

    auto nombre_diagnostics = clang_getNumDiagnostics(unit);

    if (nombre_diagnostics != 0) {
        std::cerr << "Il y a " << nombre_diagnostics << " diagnostics\n";

        for (auto i = 0u; i < nombre_diagnostics; ++i) {
            auto diag = clang_getDiagnostic(unit, i);
            std::cerr << clang_getDiagnosticSpelling(diag) << '\n';

            auto loc = clang_getDiagnosticLocation(diag);

            CXFile file = clang_getFile(unit, config.fichier.c_str());

            size_t taille = 0;
            auto tampon = clang_getFileContents(unit, file, &taille);

            uint32_t ligne = 0;
            uint32_t colonne = 0;
            uint32_t decalage = 0;
            auto filename = clang_getFileName(file);
            std::cerr << filename << ":\n";
            clang_getExpansionLocation(loc, &file, &ligne, &colonne, &decalage);
            imprime_ligne(std::string(tampon, taille), ligne, colonne, decalage);
            clang_disposeDiagnostic(diag);
        }

        exit(-1);
    }

    auto convertisseuse = Convertisseuse();
    convertisseuse.fichier_source = fichier_source;
    convertisseuse.fichier_entete = fichier_entete;
    convertisseuse.nom_bibliothèque_sûr = config.nom_bibliothèque;
    POUR (convertisseuse.nom_bibliothèque_sûr) {
        if (it == '.' || it == '-') {
            it = '_';
        }
    }
    convertisseuse.config = &config;
    convertisseuse.ajoute_typedef("size_t", "ulong");
    convertisseuse.ajoute_typedef("ssize_t", "long");
    convertisseuse.ajoute_typedef("std::size_t", "ulong");
    convertisseuse.ajoute_typedef("uint8_t", "uchar");
    convertisseuse.ajoute_typedef("uint16_t", "ushort");
    convertisseuse.ajoute_typedef("uint32_t", "uint");
    convertisseuse.ajoute_typedef("uint64_t", "ulong");
    convertisseuse.ajoute_typedef("int8_t", "char");
    convertisseuse.ajoute_typedef("int16_t", "short");
    convertisseuse.ajoute_typedef("int32_t", "int");
    convertisseuse.ajoute_typedef("int64_t", "long");
    convertisseuse.ajoute_typedef("uintptr_t", "n64");
    convertisseuse.ajoute_typedef("intptr_t", "z64");
    /* Hack afin de convertir les types half vers notre langage, ceci empêche d'y ajouter les
     * typedefs devant être utilisés afin de faire compiler le code C puisque ni half ni r16
     * n'existent en C. */
    convertisseuse.ajoute_typedef("r16", "r16");
    convertisseuse.ajoute_typedef("half", "r16");
    convertisseuse.ajoute_typedef("ptrdiff_t", "z64");

    if (WCHAR_WIDTH == 16) {
        convertisseuse.ajoute_typedef("wchar_t", "n16");
    }
    else if (WCHAR_WIDTH == 32) {
        convertisseuse.ajoute_typedef("wchar_t", "n32");
    }

    if (config.fichier_sortie != "") {
        std::ofstream fichier(config.fichier_sortie.c_str());
        fichier << "/* stats-code ignore fichier */\n\n";
        convertisseuse.convertis(unit, fichier);
    }
    else {
        convertisseuse.convertis(unit, std::cerr);
    }

    if (convertisseuse.cursors_non_pris_en_charges.taille() != 0) {
        std::cerr << "Les cursors non pris en charges sont :\n";

        convertisseuse.cursors_non_pris_en_charges.pour_chaque_element([&](auto kind) {
            std::cerr << '\t' << clang_getCursorKindSpelling(kind) << " (" << kind << ')' << '\n';
        });
    }

#if 0
    if (convertisseuse.fichiers_ignorés.taille() != 0) {
        std::cerr << "Les fichers suivants furent ignorés :\n";

        convertisseuse.fichiers_ignorés.pour_chaque_element(
            [&](auto fichier) { std::cerr << '\t' << fichier << '\n'; });
    }
#endif

    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);

    if (config.fichier_est_composite && config.fichier_tmp.taille() != 0) {
        remove(config.fichier_tmp.c_str());
    }

    return 0;
}
