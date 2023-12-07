/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include "broyage.hh"

#include <iostream>

#include "biblinternes/langage/unicode.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/numerique.hh"

#include "structures/enchaineuse.hh"

#include "parsage/identifiant.hh"
#include "parsage/modules.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "typage.hh"
#include "utilitaires/log.hh"

static void broye_nom_simple(Enchaineuse &enchaineuse, kuri::chaine_statique const &nom)
{
    auto debut = nom.pointeur();
    auto fin = nom.pointeur() + nom.taille();

    while (debut < fin) {
        auto no = lng::nombre_octets(debut);

        switch (no) {
            case 0:
            {
                debut += 1;
                break;
            }
            case 1:
            {
                enchaineuse.pousse_caractere(*debut);
                break;
            }
            default:
            {
                for (int i = 0; i < no; ++i) {
                    enchaineuse.pousse_caractere('x');
                    enchaineuse.pousse_caractere(
                        dls::num::char_depuis_hex(static_cast<char>((debut[i] & 0xf0) >> 4)));
                    enchaineuse.pousse_caractere(
                        dls::num::char_depuis_hex(static_cast<char>(debut[i] & 0x0f)));
                }

                break;
            }
        }

        debut += no;
    }
}

kuri::chaine_statique Broyeuse::broye_nom_simple(kuri::chaine_statique const &nom)
{
    stockage_temp.réinitialise();
    ::broye_nom_simple(stockage_temp, nom);
    return chaine_finale_pour_stockage_temp();
}

kuri::chaine_statique Broyeuse::broye_nom_simple(IdentifiantCode *ident)
{
    if (!ident) {
        return "";
    }

    if (ident->nom_broye != "") {
        return ident->nom_broye;
    }

    ident->nom_broye = broye_nom_simple(ident->nom);
    return ident->nom_broye;
}

int64_t Broyeuse::mémoire_utilisée() const
{
    return stockage_chaines.mémoire_utilisée() + stockage_chaines.mémoire_utilisée();
}

/* Broye le nom d'un type.
 *
 * Convention :
 * Kv : argument variadique
 * KP : pointeur
 * KR : référence
 * KT : tableau fixe, suivi de la taille
 * Kt : tableau dynamique
 * Ks : introduit un type scalaire, suivi de la chaine du type
 * Kf : fonction
 * Kc : coroutine
 *
 * Un type scalaire est un type de base, ou un type du programme.
 *
 * Exemples :
 * *z8 devient KPKsz8
 * &[]Foo devient KRKtKsFoo
 */
static void nom_broye_type(Enchaineuse &enchaineuse, Type *type)
{
    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        {
            assert_rappel(false,
                          [&]() { dbg() << "Obtenu un type polymorphique dans le broyage !"; });
            break;
        }
        case GenreType::EINI:
        {
            enchaineuse << "Kseini";
            break;
        }
        case GenreType::CHAINE:
        {
            enchaineuse << "Kschaine";
            break;
        }
        case GenreType::RIEN:
        {
            enchaineuse << "Ksrien";
            break;
        }
        case GenreType::BOOL:
        {
            enchaineuse << "Ksbool";
            break;
        }
        case GenreType::OCTET:
        {
            enchaineuse << "Ksoctet";
            break;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            enchaineuse << "Ksz32";
            break;
        }
        case GenreType::ENTIER_NATUREL:
        {
            if (type->taille_octet == 1) {
                enchaineuse << "Ksn8";
            }
            else if (type->taille_octet == 2) {
                enchaineuse << "Ksn16";
            }
            else if (type->taille_octet == 4) {
                enchaineuse << "Ksn32";
            }
            else if (type->taille_octet == 8) {
                enchaineuse << "Ksn64";
            }

            break;
        }
        case GenreType::ENTIER_RELATIF:
        {
            if (type->taille_octet == 1) {
                enchaineuse << "Ksz8";
            }
            else if (type->taille_octet == 2) {
                enchaineuse << "Ksz16";
            }
            else if (type->taille_octet == 4) {
                enchaineuse << "Ksz32";
            }
            else if (type->taille_octet == 8) {
                enchaineuse << "Ksz64";
            }

            break;
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 2) {
                enchaineuse << "Ksr16";
            }
            else if (type->taille_octet == 4) {
                enchaineuse << "Ksr32";
            }
            else if (type->taille_octet == 8) {
                enchaineuse << "Ksr64";
            }

            break;
        }
        case GenreType::REFERENCE:
        {
            enchaineuse << "KR";
            nom_broye_type(enchaineuse, type->comme_type_reference()->type_pointe);
            break;
        }
        case GenreType::POINTEUR:
        {
            enchaineuse << "KP";

            auto type_pointe = type->comme_type_pointeur()->type_pointe;

            if (type_pointe == nullptr) {
                enchaineuse << "nul";
            }
            else {
                nom_broye_type(enchaineuse, type_pointe);
            }

            break;
        }
        case GenreType::UNION:
        {
            auto type_union = static_cast<TypeUnion const *>(type);
            enchaineuse << "Ks";
            broye_nom_simple(enchaineuse, donne_nom_portable(const_cast<TypeUnion *>(type_union)));

            // ajout du pointeur au nom afin de différencier les différents types anonymes ou
            // monomorphisations
            if (type_union->est_anonyme ||
                (type_union->decl && type_union->decl->est_monomorphisation)) {
                enchaineuse << type_union;
            }

            break;
        }
        case GenreType::STRUCTURE:
        {
            auto type_structure = static_cast<TypeStructure const *>(type);
            enchaineuse << "Ks";
            broye_nom_simple(enchaineuse,
                             donne_nom_portable(const_cast<TypeStructure *>(type_structure)));

            // ajout du pointeur au nom afin de différencier les différents types anonymes ou
            // monomorphisations
            if (type_structure->est_anonyme ||
                (type_structure->decl && type_structure->decl->est_monomorphisation)) {
                enchaineuse << type_structure;
            }

            break;
        }
        case GenreType::VARIADIQUE:
        {
            auto type_pointe = type->comme_type_variadique()->type_pointe;

            // les arguments variadiques sont transformés en tableaux, donc utilise Kt
            if (type_pointe != nullptr) {
                enchaineuse << "Kt";
                nom_broye_type(enchaineuse, type_pointe);
            }
            else {
                enchaineuse << "Kv";
            }

            break;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        {
            enchaineuse << "Kt";
            nom_broye_type(enchaineuse, type->comme_type_tableau_dynamique()->type_pointe);
            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tabl = type->comme_type_tableau_fixe();

            enchaineuse << "KT";
            enchaineuse << type_tabl->taille;
            nom_broye_type(enchaineuse, type_tabl->type_pointe);
            break;
        }
        case GenreType::FONCTION:
        {
            enchaineuse << "Kf";
            enchaineuse << type;
            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum *>(type);
            enchaineuse << "Ks";
            broye_nom_simple(enchaineuse, donne_nom_portable(type_enum));
            break;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            enchaineuse << "Kstype_de_donnees";
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            enchaineuse << "Ks";
            broye_nom_simple(enchaineuse, donne_nom_portable(type_opaque));
            /* inclus le nom du type opacifié afin de prendre en compte les monomorphisations */
            nom_broye_type(enchaineuse, type_opaque->type_opacifie);
            break;
        }
        case GenreType::TUPLE:
        {
            enchaineuse << "Kl" << type;
            break;
        }
    }
}

kuri::chaine_statique Broyeuse::nom_broyé_type(Type *type)
{
    if (type->nom_broye != "") {
        return type->nom_broye;
    }

    stockage_temp.réinitialise();
    ::nom_broye_type(stockage_temp, type);

    type->nom_broye = chaine_finale_pour_stockage_temp();

    return type->nom_broye;
}

static const char *nom_pour_operateur(Lexeme const &lexeme)
{
    switch (lexeme.genre) {
        default:
        {
            assert_rappel(false, [&]() {
                dbg() << "Lexème inattendu pour les opérateurs dans le broyage de nom : "
                      << chaine_du_genre_de_lexème(lexeme.genre);
            });
            break;
        }
        case GenreLexeme::INFERIEUR:
        {
            return "inf";
        }
        case GenreLexeme::INFERIEUR_EGAL:
        {
            return "infeg";
        }
        case GenreLexeme::SUPERIEUR:
        {
            return "sup";
        }
        case GenreLexeme::SUPERIEUR_EGAL:
        {
            return "supeg";
        }
        case GenreLexeme::DIFFERENCE:
        {
            return "dif";
        }
        case GenreLexeme::EGALITE:
        {
            return "egl";
        }
        case GenreLexeme::PLUS:
        {
            return "plus";
        }
        case GenreLexeme::PLUS_UNAIRE:
        {
            return "pls_unr";
        }
        case GenreLexeme::MOINS:
        {
            return "moins";
        }
        case GenreLexeme::MOINS_UNAIRE:
        {
            return "mns_unr";
        }
        case GenreLexeme::FOIS:
        {
            return "mul";
        }
        case GenreLexeme::DIVISE:
        {
            return "div";
        }
        case GenreLexeme::DECALAGE_DROITE:
        {
            return "dcd";
        }
        case GenreLexeme::DECALAGE_GAUCHE:
        {
            return "dcg";
        }
        case GenreLexeme::POURCENT:
        {
            return "mod";
        }
        case GenreLexeme::ESPERLUETTE:
        {
            return "et";
        }
        case GenreLexeme::BARRE:
        {
            return "ou";
        }
        case GenreLexeme::TILDE:
        {
            return "non";
        }
        case GenreLexeme::CHAPEAU:
        {
            return "oux";
        }
        case GenreLexeme::CROCHET_OUVRANT:
        {
            return "oux";
        }
    }

    return "inconnu";
}

/* format :
 * _K préfixe pour tous les noms de Kuri
 * C, E, F, S, U : coroutine, énum, fonction, structure, ou union
 * paire(s) : longueur + chaine ascii pour module et nom
 *
 * pour les fonctions :
 * _E[N]_ : introduit les paramètres d'entrées, N = nombre de paramètres
 * paire(s) : noms des paramètres et de leurs types, préfixés par leurs tailles
 * _S[N]_ : introduit les paramètres d'entrées, N = nombre de paramètres
 *
 * types des paramètres pour les fonctions
 *
 * fonc test(x : z32) : z32 (module Test)
 * -> _KF4Test4test_P2_E1_1x3z32_S1_3z32
 */
kuri::chaine_statique Broyeuse::broye_nom_fonction(
    NoeudDeclarationEnteteFonction *decl,
    kuri::tablet<IdentifiantCode *, 6> const &noms_hiérarchie)
{
    stockage_temp.réinitialise();

    auto type_fonc = decl->type->comme_type_fonction();

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME)) {
        stockage_temp << "metaprogramme" << decl;
        return chaine_finale_pour_stockage_temp();
    }

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
        stockage_temp << "initialise_" << decl->type_initialisé();
        return chaine_finale_pour_stockage_temp();
    }

    /* Prépare les données afin de ne pas polluer l'enchaineuse. */
    kuri::tablet<kuri::chaine_statique, 6> noms_broyés_hiérarchie;
    POUR (noms_hiérarchie) {
        noms_broyés_hiérarchie.ajoute(broye_nom_simple(it));
    }
    auto nom_ascii_fonction = broye_nom_simple(decl->ident);

    decl->bloc_constantes->membres.avec_verrou_lecture(
        [&](kuri::tableau<NoeudDeclaration *, int> const &membres) {
            POUR (membres) {
                broye_nom_simple(it->ident);

                auto type = it->type;
                if (type->est_type_type_de_donnees() &&
                    type->comme_type_type_de_donnees()->type_connu) {
                    type = type->comme_type_type_de_donnees()->type_connu;
                }

                nom_broyé_type(type);
            }
        });

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = decl->parametre_entree(i);
        broye_nom_simple(param->ident);
        nom_broyé_type(param->type);
    }

    nom_broyé_type(type_fonc->type_sortie);

    /* Crée le nom broyé. */
    stockage_temp.réinitialise();

    /* Module et nom. */
    stockage_temp << "_K";
    stockage_temp << (decl->est_coroutine ? "C" : "F");

    for (auto i = noms_broyés_hiérarchie.taille() - 1; i >= 0; --i) {
        auto nom_broyé_hiérarchie = noms_broyés_hiérarchie[i];
        stockage_temp << nom_broyé_hiérarchie.taille();
        stockage_temp << nom_broyé_hiérarchie;
    }

    /* nom de la fonction */
    if (decl->est_operateur) {
        stockage_temp << "operateur" << nom_pour_operateur(*decl->lexeme);
    }
    else {
        stockage_temp << nom_ascii_fonction.taille();
        stockage_temp << nom_ascii_fonction;
    }

    /* paramètres */
    stockage_temp << "_P";
    stockage_temp << decl->bloc_constantes->nombre_de_membres();
    stockage_temp << "_";

    decl->bloc_constantes->membres.avec_verrou_lecture(
        [&](kuri::tableau<NoeudDeclaration *, int> const &membres) {
            POUR (membres) {
                auto nom_ascii = it->ident->nom_broye;
                stockage_temp << nom_ascii.taille();
                stockage_temp << nom_ascii;

                auto type = it->type;
                if (type->est_type_type_de_donnees() &&
                    type->comme_type_type_de_donnees()->type_connu) {
                    type = type->comme_type_type_de_donnees()->type_connu;
                }

                auto nom_broye = type->nom_broye;
                stockage_temp << nom_broye.taille();
                stockage_temp << nom_broye;
            }
        });

    /* entrées */
    stockage_temp << "_E";
    stockage_temp << decl->params.taille();
    stockage_temp << "_";

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = decl->parametre_entree(i);

        auto nom_ascii = param->ident->nom_broye;
        stockage_temp << nom_ascii.taille();
        stockage_temp << nom_ascii;

        auto nom_broye = param->type->nom_broye;
        stockage_temp << nom_broye.taille();
        stockage_temp << nom_broye;
    }

    /* sorties */
    stockage_temp << "_S";
    stockage_temp << "_";

    auto nom_broye = type_fonc->type_sortie->nom_broye;
    stockage_temp << nom_broye.taille();
    stockage_temp << nom_broye;

    /* Ajout du pointeur car les fonctions nichées dans des fonctions polymorphiques peuvent finir
     * avec le même nom broyé. */
    stockage_temp << decl;

    return chaine_finale_pour_stockage_temp();
}

kuri::chaine_statique Broyeuse::chaine_finale_pour_stockage_temp()
{
    auto resultat_temp = stockage_temp.chaine_statique();
    return stockage_chaines.ajoute_chaine_statique(resultat_temp);
}
