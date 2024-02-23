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

#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "typage.hh"
#include "utilitaires/log.hh"

static void broye_nom_simple(Enchaineuse &enchaineuse, kuri::chaine_statique nom)
{
    auto début = nom.pointeur();
    auto fin = nom.pointeur() + nom.taille();

    while (début < fin) {
        auto no = lng::nombre_octets(début);

        switch (no) {
            case 0:
            {
                début += 1;
                break;
            }
            case 1:
            {
                enchaineuse.ajoute_caractère(*début);
                break;
            }
            default:
            {
                for (int i = 0; i < no; ++i) {
                    enchaineuse.ajoute_caractère('x');
                    enchaineuse.ajoute_caractère(
                        dls::num::char_depuis_hex(static_cast<char>((début[i] & 0xf0) >> 4)));
                    enchaineuse.ajoute_caractère(
                        dls::num::char_depuis_hex(static_cast<char>(début[i] & 0x0f)));
                }

                break;
            }
        }

        début += no;
    }
}

kuri::chaine_statique Broyeuse::broye_nom_simple(kuri::chaine_statique nom)
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

struct HiérarchieDeNoms {
    NoeudDeclarationSymbole const *feuille = nullptr;
    IdentifiantCode const *ident_module = nullptr;
    kuri::tablet<NoeudDeclarationSymbole const *, 6> hiérarchie{};
};

static HiérarchieDeNoms donne_hiérarchie_nom(NoeudDeclarationSymbole const *symbole)
{
    HiérarchieDeNoms résultat;
    résultat.feuille = symbole;

    kuri::ensemblon<NoeudExpression *, 6> noeuds_visités;

    résultat.hiérarchie.ajoute(symbole);

    if (symbole->est_declaration_classe() && symbole->comme_declaration_classe()->est_anonyme) {
        /* Place les unions anonymes dans le contexte globale car sinon nous aurions des
         * dépendances cycliques quand la première définition fut rencontrée dans le type de retour
         * d'une fonction. */
        return résultat;
    }

    auto bloc = symbole->bloc_parent;
    while (bloc) {
        if (bloc->appartiens_à_fonction) {
            if (!noeuds_visités.possède(bloc->appartiens_à_fonction)) {
                résultat.hiérarchie.ajoute(bloc->appartiens_à_fonction);
                noeuds_visités.insère(bloc->appartiens_à_fonction);
            }
        }
        else if (bloc->appartiens_à_type) {
            if (!noeuds_visités.possède(bloc->appartiens_à_type)) {
                résultat.hiérarchie.ajoute(bloc->appartiens_à_type);
                noeuds_visités.insère(bloc->appartiens_à_type);
            }
        }

        if (bloc->bloc_parent == nullptr) {
            /* Bloc du module. */
            résultat.ident_module = bloc->ident;
            break;
        }

        bloc = bloc->bloc_parent;
    }

    return résultat;
}

static void imprime_hiérarchie(HiérarchieDeNoms const &hiérarchie)
{
    dbg() << "============ Hiérarchie";

    if (hiérarchie.ident_module && hiérarchie.ident_module != ID::chaine_vide) {
        dbg() << "-- " << hiérarchie.ident_module->nom;
    }

    for (auto i = hiérarchie.hiérarchie.taille() - 1; i >= 0; i -= 1) {
        auto noeud = hiérarchie.hiérarchie[i];
        dbg() << "-- " << nom_humainement_lisible(noeud);
    }
}

static void broye_nom_hiérarchique(Enchaineuse &enchaineuse, HiérarchieDeNoms const &hiérarchie);

static void broye_nom_type(Enchaineuse &enchaineuse, Type *type, bool pour_hiérarchie);

static const char *nom_pour_operateur(Lexème const &lexeme);

static void ajoute_broyage_constantes(Enchaineuse &enchaineuse, NoeudBloc *bloc);

static void ajoute_broyage_paramètres(Enchaineuse &enchaineuse,
                                      NoeudDeclarationEnteteFonction const *entête)
{
    if (entête->params.est_vide()) {
        return;
    }

    enchaineuse << "_E" << entête->params.taille() << "_";

    for (auto i = 0; i < entête->params.taille(); ++i) {
        auto param = entête->parametre_entree(i);

        auto nom_ascii = param->ident->nom_broye;
        enchaineuse << nom_ascii.taille();
        enchaineuse << nom_ascii;

        auto nom_broye = param->type->nom_broye;
        enchaineuse << nom_broye.taille();
        enchaineuse << nom_broye;
    }
}

static void broye_nom_fonction(Enchaineuse &enchaineuse,
                               NoeudDeclarationEnteteFonction const *entête,
                               bool pour_hiérarchie,
                               bool est_feuille_hiérarchie)
{
    /* Module et nom. */
    if (!est_feuille_hiérarchie) {
        enchaineuse << "_K";
        enchaineuse << (entête->est_coroutine ? "C" : "F");
    }

    if (!pour_hiérarchie) {
        auto hiérachie = donne_hiérarchie_nom(entête);
        broye_nom_hiérarchique(enchaineuse, hiérachie);
        return;
    }

    /* nom de la fonction */
    if (entête->est_operateur) {
        enchaineuse << "operateur" << nom_pour_operateur(*entête->lexeme);
    }
    else {
        // À FAIRE
        //        auto nom_ascii_fonction = broye_nom_simple(entête->ident);
        //        enchaineuse << nom_ascii_fonction.taille();
        //        enchaineuse << nom_ascii_fonction;

        broye_nom_simple(enchaineuse, entête->ident->nom);
    }

    /* paramètres */
    ajoute_broyage_constantes(enchaineuse, entête->bloc_constantes);

    /* entrées */
    ajoute_broyage_paramètres(enchaineuse, entête);

    /* sortie. */
    enchaineuse << "_S_";

    auto type_fonc = entête->type->comme_type_fonction();
    auto nom_broye = type_fonc->type_sortie->nom_broye;
    enchaineuse << nom_broye.taille();
    enchaineuse << nom_broye;
}

static void broye_nom_hiérarchique(Enchaineuse &enchaineuse, HiérarchieDeNoms const &hiérarchie)
{
    auto virgule = kuri::chaine_statique("");

    if (hiérarchie.ident_module && hiérarchie.ident_module != ID::chaine_vide) {
        virgule = "_";
        broye_nom_simple(enchaineuse, hiérarchie.ident_module->nom);
    }

    for (auto i = hiérarchie.hiérarchie.taille() - 1; i >= 0; i -= 1) {
        auto noeud = hiérarchie.hiérarchie[i];

        enchaineuse << virgule;

        if (noeud->est_entete_fonction()) {
            broye_nom_fonction(
                enchaineuse, noeud->comme_entete_fonction(), true, noeud == hiérarchie.feuille);
        }
        else {
            auto type = const_cast<Type *>(noeud->comme_declaration_type());
            broye_nom_type(enchaineuse, type, true);
        }

        virgule = "_";
    }
}

static void ajoute_broyage_constantes(Enchaineuse &enchaineuse, NoeudBloc *bloc)
{
    if (!bloc || bloc->membres->est_vide()) {
        return;
    }

    enchaineuse << "_P" << bloc->membres->taille();

    POUR (*bloc->membres.verrou_lecture()) {
        auto constante = it->comme_declaration_constante();
        auto type_membre = constante->type;
        if (type_membre->est_type_type_de_donnees()) {
            /* Préfixe pour un type. */
            enchaineuse << "_T";
            type_membre = type_membre->comme_type_type_de_donnees()->type_connu;
            broye_nom_type(enchaineuse, type_membre, false);
        }
        else {
            /* Préfixe pour une valeur. */
            enchaineuse << "_V";
            auto valeur = constante->valeur_expression;
            if (valeur.est_booléenne() || valeur.est_entière()) {
                enchaineuse << constante->valeur_expression;
            }
            else if (valeur.est_chaine()) {
                broye_nom_simple(enchaineuse, valeur.chaine()->lexeme->chaine);
            }
            else if (valeur.est_fonction()) {
                broye_nom_fonction(enchaineuse, valeur.fonction(), true, false);
            }
            else if (valeur.est_type()) {
                broye_nom_type(enchaineuse, valeur.type(), false);
            }
            else {
                // À FAIRE : imprime la valeur selon le type en faisant en sorte que la
                // valeur ne contient que des caractères légaux.
            }

            broye_nom_type(enchaineuse, type_membre, false);
        }
    }
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
 * &[..]Foo devient KRKtKsFoo
 */
static void broye_nom_type(Enchaineuse &enchaineuse, Type *type, bool pour_hiérarchie)
{
    switch (type->genre) {
        case GenreNoeud::POLYMORPHIQUE:
        {
            assert_rappel(false,
                          [&]() { dbg() << "Obtenu un type polymorphique dans le broyage !"; });
            break;
        }
        case GenreNoeud::EINI:
        case GenreNoeud::CHAINE:
        case GenreNoeud::RIEN:
        case GenreNoeud::BOOL:
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::REEL:
        case GenreNoeud::TYPE_DE_DONNEES:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            enchaineuse << "Ks" << type->ident->nom;
            break;
        }
        case GenreNoeud::REFERENCE:
        {
            enchaineuse << "KR";
            broye_nom_type(enchaineuse, type->comme_type_reference()->type_pointe, false);
            break;
        }
        case GenreNoeud::POINTEUR:
        {
            enchaineuse << "KP";

            auto type_élément = type->comme_type_pointeur()->type_pointe;

            if (type_élément == nullptr) {
                enchaineuse << "nul";
            }
            else {
                broye_nom_type(enchaineuse, type_élément, false);
            }

            break;
        }
        case GenreNoeud::DECLARATION_UNION:
        {
            auto type_union = static_cast<TypeUnion const *>(type);

            if (!pour_hiérarchie) {
                enchaineuse << "Ks";
                auto hiérarchie = donne_hiérarchie_nom(type);
                broye_nom_hiérarchique(enchaineuse, hiérarchie);
            }
            else {
                if (type->ident) {
                    broye_nom_simple(enchaineuse, type->ident->nom);
                }
                else {
                    enchaineuse << "union_anonyme";
                }

                // ajout du pointeur au nom afin de différencier les différents types anonymes ou
                // monomorphisations
                if (type_union->est_anonyme) {
                    enchaineuse << type_union->type_structure;
                }
                else if (type_union->est_monomorphisation) {
                    ajoute_broyage_constantes(enchaineuse, type_union->bloc_constantes);
                }
            }

            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            if (!pour_hiérarchie) {
                enchaineuse << "Ks";
                auto hiérarchie = donne_hiérarchie_nom(type);
                broye_nom_hiérarchique(enchaineuse, hiérarchie);
            }
            else {
                if (type->ident) {
                    broye_nom_simple(enchaineuse, type->ident->nom);
                }
                else {
                    enchaineuse << "struct_anonyme";
                }

                auto type_structure = static_cast<TypeStructure const *>(type);
                // ajout du pointeur au nom afin de différencier les différents types anonymes ou
                // monomorphisations
                if (type_structure->est_anonyme) {
                    enchaineuse << type_structure;
                }
                else if (type_structure->est_monomorphisation) {
                    ajoute_broyage_constantes(enchaineuse, type_structure->bloc_constantes);
                }
            }

            break;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_élément = type->comme_type_variadique()->type_pointe;

            // les arguments variadiques sont transformés en tranches, donc utilise Kz
            if (type_élément != nullptr) {
                enchaineuse << "Kz";
                broye_nom_type(enchaineuse, type_élément, false);
            }
            else {
                enchaineuse << "Kv";
            }

            break;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            enchaineuse << "Kz";
            broye_nom_type(enchaineuse, type->comme_type_tranche()->type_élément, false);
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            enchaineuse << "Kt";
            broye_nom_type(enchaineuse, type->comme_type_tableau_dynamique()->type_pointe, false);
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tabl = type->comme_type_tableau_fixe();

            enchaineuse << "KT";
            enchaineuse << type_tabl->taille;
            broye_nom_type(enchaineuse, type_tabl->type_pointe, false);
            break;
        }
        case GenreNoeud::FONCTION:
        {
            auto const type_fonction = type->comme_type_fonction();
            enchaineuse << "Kf";
            enchaineuse << type_fonction->types_entrees.taille();

            POUR (type_fonction->types_entrees) {
                broye_nom_type(enchaineuse, it, false);
            }

            enchaineuse << 1;
            broye_nom_type(enchaineuse, type_fonction->type_sortie, false);

            break;
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        {
            if (!pour_hiérarchie) {
                enchaineuse << "Ks";
                auto hiérarchie = donne_hiérarchie_nom(type);
                broye_nom_hiérarchique(enchaineuse, hiérarchie);
            }
            else {
                broye_nom_simple(enchaineuse, type->ident->nom);
            }

            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();

            if (!pour_hiérarchie) {
                enchaineuse << "Ks";
                auto hiérarchie = donne_hiérarchie_nom(type);
                broye_nom_hiérarchique(enchaineuse, hiérarchie);
            }

            broye_nom_simple(enchaineuse, type->ident->nom);
            /* inclus le nom du type opacifié afin de prendre en compte les monomorphisations */
            broye_nom_type(enchaineuse, type_opaque->type_opacifie, false);
            break;
        }
        case GenreNoeud::TUPLE:
        {
            enchaineuse << "KlTuple" << type;
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
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
    ::broye_nom_type(stockage_temp, type, false);

    type->nom_broye = chaine_finale_pour_stockage_temp();
    stockage_temp.réinitialise();

    /* Pour supprimer les accents dans les types fondementaux. */
    type->nom_broye = broye_nom_simple(type->nom_broye);

    return type->nom_broye;
}

static const char *nom_pour_operateur(Lexème const &lexeme)
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
        case GenreLexème::INFERIEUR:
        {
            return "inf";
        }
        case GenreLexème::INFERIEUR_EGAL:
        {
            return "infeg";
        }
        case GenreLexème::SUPERIEUR:
        {
            return "sup";
        }
        case GenreLexème::SUPERIEUR_EGAL:
        {
            return "supeg";
        }
        case GenreLexème::DIFFERENCE:
        {
            return "dif";
        }
        case GenreLexème::EGALITE:
        {
            return "egl";
        }
        case GenreLexème::PLUS:
        {
            return "plus";
        }
        case GenreLexème::PLUS_UNAIRE:
        {
            return "pls_unr";
        }
        case GenreLexème::MOINS:
        {
            return "moins";
        }
        case GenreLexème::MOINS_UNAIRE:
        {
            return "mns_unr";
        }
        case GenreLexème::FOIS:
        {
            return "mul";
        }
        case GenreLexème::DIVISE:
        {
            return "div";
        }
        case GenreLexème::DECALAGE_DROITE:
        {
            return "dcd";
        }
        case GenreLexème::DECALAGE_GAUCHE:
        {
            return "dcg";
        }
        case GenreLexème::POURCENT:
        {
            return "mod";
        }
        case GenreLexème::ESPERLUETTE:
        {
            return "et";
        }
        case GenreLexème::BARRE:
        {
            return "ou";
        }
        case GenreLexème::TILDE:
        {
            return "non";
        }
        case GenreLexème::CHAPEAU:
        {
            return "oux";
        }
        case GenreLexème::CROCHET_OUVRANT:
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

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME)) {
        stockage_temp << "metaprogramme" << decl;
        return chaine_finale_pour_stockage_temp();
    }

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
        auto const options = OptionsImpressionType::POUR_FONCTION_INITIALISATION;
        auto nom_type = chaine_type(decl->type_initialisé(), options);
        auto nom_type_broyé = broye_nom_simple(nom_type);
        stockage_temp.réinitialise();
        stockage_temp << "initialise_" << nom_type_broyé;
        return chaine_finale_pour_stockage_temp();
    }

    /* Prépare les données afin de ne pas polluer l'enchaineuse. */
    kuri::tablet<kuri::chaine_statique, 6> noms_broyés_hiérarchie;
    POUR (noms_hiérarchie) {
        noms_broyés_hiérarchie.ajoute(broye_nom_simple(it));
    }

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

    auto type_fonc = decl->type->comme_type_fonction();
    nom_broyé_type(type_fonc->type_sortie);

    stockage_temp << "_K";
    stockage_temp << (decl->est_coroutine ? "C" : "F");
    ::broye_nom_fonction(stockage_temp, decl, false, true);
    return chaine_finale_pour_stockage_temp();
}

kuri::chaine_statique Broyeuse::chaine_finale_pour_stockage_temp()
{
    auto résultat_temp = stockage_temp.chaine_statique();
    return stockage_chaines.ajoute_chaine_statique(résultat_temp);
}
