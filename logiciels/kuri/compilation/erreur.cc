/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "erreur.h"

#include <iostream>

#ifndef _MSC_VER
#    include <unistd.h>
#endif

#include "arbre_syntaxique/etendue_code_source.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"
#include "parsage/lexemes.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "monomorpheuse.hh"
#include "validation_expression_appel.hh"
#include "validation_semantique.hh"

namespace erreur {

const char *chaine_erreur(Genre genre)
{
    switch (genre) {
#define ENUMERE_GENRE_ERREUR_EX(genre)                                                            \
    case Genre::genre:                                                                            \
        return #genre;
        ENUMERE_GENRES_ERREUR
#undef ENUMERE_GENRE_ERREUR_EX
    }
    return "Ceci ne devrait pas s'afficher";
}

std::ostream &operator<<(std::ostream &os, Genre genre)
{
    os << chaine_erreur(genre);
    return os;
}

kuri::chaine_statique chaine_expression(EspaceDeTravail const &espace, const NoeudExpression *expr)
{
    auto lexeme = expr->lexème;
    auto fichier = espace.fichier(lexeme->fichier);
    auto etendue_expr = donne_étendue_source_noeud(expr);
    auto ligne = fichier->tampon()[lexeme->ligne];
    return ligne.sous_chaine(etendue_expr.colonne_début, etendue_expr.colonne_fin);
}

void redefinition_fonction(EspaceDeTravail const &espace,
                           const NoeudExpression *site_redefinition,
                           const NoeudExpression *site_original)
{
    espace
        .rapporte_erreur(
            site_redefinition, "Redéfinition de la fonction !", Genre::FONCTION_REDEFINIE)
        .ajoute_message("La fonction fut déjà définie ici :\n\n")
        .ajoute_site(site_original);
}

void redefinition_symbole(EspaceDeTravail const &espace,
                          const NoeudExpression *site_redefinition,
                          const NoeudDéclaration *site_original)
{
    kuri::chaine_statique message = "Le symbole fut déjà défini ici :\n\n";

    if (site_original->est_déclaration_variable()) {
        auto déclaration_variable = site_original->comme_déclaration_variable();
        auto déclaration_source = déclaration_variable->déclaration_vient_d_un_emploi;
        if (déclaration_source) {
            message = "Le symbole fut défini via l'instruction 'empl' ici :\n\n:";
        }
    }

    espace
        .rapporte_erreur(site_redefinition, "Redéfinition du symbole !", Genre::VARIABLE_REDEFINIE)
        .ajoute_message(message)
        .ajoute_site(site_original);
}

void lance_erreur_transtypage_impossible(const Type *type_cible,
                                         const Type *type_expression,
                                         EspaceDeTravail const &espace,
                                         const NoeudExpression *site_expression,
                                         const NoeudExpression *site)
{
    espace
        .rapporte_erreur(site,
                         "Aucune conversion connue pour transformer vers le type cible",
                         Genre::TYPE_ARGUMENT)
        .ajoute_message("Le type de l'expression '",
                        chaine_expression(espace, site_expression),
                        "' ne peut être transformer vers le type cible !\n")
        .ajoute_message("Type cible           : ", chaine_type(type_cible), "\n")
        .ajoute_message("Type de l'expression : ", chaine_type(type_expression), "\n\n");
}

void lance_erreur_assignation_type_differents(const Type *type_gauche,
                                              const Type *type_droite,
                                              EspaceDeTravail const &espace,
                                              const NoeudExpression *site)
{
    espace
        .rapporte_erreur(
            site, "Ne peut pas assigner des types différents !", Genre::ASSIGNATION_MAUVAIS_TYPE)
        .ajoute_message("Type à gauche : ", chaine_type(type_gauche), "\n")
        .ajoute_message("Type à droite : ", chaine_type(type_droite), "\n");
}

void lance_erreur_type_operation(const Type *type_gauche,
                                 const Type *type_droite,
                                 EspaceDeTravail const &espace,
                                 const NoeudExpression *site)
{
    espace.rapporte_erreur(site, "Type incompatible pour l'opération !", Genre::TYPE_DIFFERENTS)
        .ajoute_message("Type à gauche : ", chaine_type(type_gauche), "\n")
        .ajoute_message("Type à droite : ", chaine_type(type_droite), "\n");
}

static void imprime_erreur_pour_erreur_fonction(Erreur &e,
                                                EspaceDeTravail const &espace,
                                                ErreurAppariement const &dc)
{
    auto decl = dc.noeud_decl;

    switch (dc.raison) {
        case RaisonErreurAppariement::MÉCOMPTAGE_ARGS:
        {
            e.ajoute_message("\tLe nombre d'arguments de la fonction est incorrect.\n");
            e.ajoute_message("\tRequiers ", dc.nombre_arguments.nombre_requis, " arguments\n");
            e.ajoute_message("\tObtenu ", dc.nombre_arguments.nombre_obtenu, " arguments\n");
            e.genre_erreur(erreur::Genre::NOMBRE_ARGUMENT);
            break;
        }
        case RaisonErreurAppariement::MÉNOMMAGE_ARG:
        {
            e.ajoute_site(dc.site_erreur);
            e.ajoute_message("\tArgument « ", dc.nom_arg->nom, " » inconnu.\n");

            if (decl && decl->genre == GenreNoeud::DÉCLARATION_CORPS_FONCTION) {
                auto decl_fonc = decl->comme_entête_fonction();
                e.ajoute_message("\tLes arguments de la fonction sont : \n");

                for (auto i = 0; i < decl_fonc->params.taille(); ++i) {
                    auto param = decl_fonc->parametre_entree(i);
                    e.ajoute_message("\t\t", param->ident->nom, '\n');
                }

                e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
            }
            else if (decl && decl->genre == GenreNoeud::DÉCLARATION_STRUCTURE) {
                auto decl_struct = decl->comme_type_structure();

                if (decl_struct->est_polymorphe) {
                    e.ajoute_message("\tLes paramètres de la structure sont : \n");

                    POUR (*decl_struct->bloc_constantes->rubriques.verrou_lecture()) {
                        e.ajoute_message("\t\t", it->ident->nom, '\n');
                    }
                }
                else {
                    e.ajoute_message("\tLes rubriques de la structure sont : \n");

                    POUR (decl_struct->rubriques) {
                        e.ajoute_message("\t\t- ", it.nom->nom, '\n');
                    }
                }

                e.genre_erreur(erreur::Genre::RUBRIQUE_INCONNUE);
            }
            break;
        }
        case RaisonErreurAppariement::NOMMAGE_MANQUANT_POUR_CUISSON:
        {
            e.ajoute_site(dc.site_erreur);
            e.ajoute_message("\tL'argument doit être nommé pour une cuisson.");
            break;
        }
        case RaisonErreurAppariement::RENOMMAGE_ARG:
        {
            e.genre_erreur(erreur::Genre::ARGUMENT_REDEFINI);
            e.ajoute_site(dc.site_erreur);
            e.ajoute_message("L'argument a déjà été nommé");
            break;
        }
        case RaisonErreurAppariement::MANQUE_NOM_APRÈS_VARIADIC:
        {
            e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
            e.ajoute_site(dc.site_erreur);
            e.ajoute_message("Nom d'argument manquant, les arguments doivent être nommés "
                             "s'ils sont précédés d'arguments déjà nommés");
            break;
        }
        case RaisonErreurAppariement::NOMMAGE_ARG_POINTEUR_FONCTION:
        {
            e.ajoute_message("\tLes arguments d'un pointeur fonction ne peuvent être nommés\n");
            e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
            break;
        }
        case RaisonErreurAppariement::TYPE_N_EST_PAS_FONCTION:
        {
            e.ajoute_message("\tAppel d'une variable n'étant pas un pointeur de fonction\n");
            e.genre_erreur(erreur::Genre::FONCTION_INCONNUE);
            break;
        }
        case RaisonErreurAppariement::TROP_D_EXPRESSION_POUR_UNION:
        {
            e.ajoute_message(
                "\tOn ne peut initialiser qu'un seul rubrique d'une union à la fois\n");
            e.genre_erreur(erreur::Genre::NORMAL);
            break;
        }
        case RaisonErreurAppariement::EXPRESSION_MANQUANTE_POUR_UNION:
        {
            e.ajoute_message("\tOn doit initialiser au moins une rubrique de l'union\n");
            e.genre_erreur(erreur::Genre::NORMAL);
            break;
        }
        case RaisonErreurAppariement::EXPANSION_VARIADIQUE_FONCTION_EXTERNE:
        {
            e.ajoute_message("\tImpossible d'utiliser une expansion variadique dans une "
                             "fonction variadique externe\n");
            e.genre_erreur(erreur::Genre::NORMAL);
            break;
        }
        case RaisonErreurAppariement::MULTIPLE_EXPANSIONS_VARIADIQUES:
        {
            e.ajoute_message("\tPlusieurs expansions variadiques trouvées\n");
            e.genre_erreur(erreur::Genre::NORMAL);
            break;
        }
        case RaisonErreurAppariement::EXPANSION_VARIADIQUE_APRÈS_ARGUMENTS_VARIADIQUES:
        {
            e.ajoute_message("\tTentative d'utiliser une expansion d'arguments variadiques "
                             "alors que d'autres arguments ont déjà été précisés\n");
            e.genre_erreur(erreur::Genre::NORMAL);
            break;
        }
        case RaisonErreurAppariement::ARGUMENTS_VARIADIQEUS_APRÈS_EXPANSION_VARIAQUES:
        {
            e.ajoute_message("\tTentative d'ajouter des arguments variadiques supplémentaire "
                             "alors qu'une expansion est également utilisée\n");
            e.genre_erreur(erreur::Genre::NORMAL);
            break;
        }
        case RaisonErreurAppariement::ARGUMENTS_MANQUANTS:
        {
            if (dc.arguments_manquants_.taille() == 1) {
                e.ajoute_message("\tUn argument est manquant :\n");
            }
            else {
                e.ajoute_message("\tPlusieurs arguments sont manquants :\n");
            }

            for (auto ident : dc.arguments_manquants_) {
                e.ajoute_message("\t\t", ident->nom, '\n');
            }
            break;
        }
        case RaisonErreurAppariement::MÉTYPAGE_ARG:
        {
            e.ajoute_message("\tLe type de l'argument '",
                             chaine_expression(espace, dc.site_erreur),
                             "' ne correspond pas à celui requis !\n");
            e.ajoute_message("\tRequiers : ", chaine_type(dc.type_arguments.type_attendu), '\n');
            e.ajoute_message("\tObtenu   : ", chaine_type(dc.type_arguments.type_obtenu), '\n');
            e.genre_erreur(erreur::Genre::TYPE_ARGUMENT);
            break;
        }
        case RaisonErreurAppariement::MONOMORPHISATION:
        {
            e.ajoute_message(dc.erreur_monomorphisation.message());
            break;
        }
        case RaisonErreurAppariement::AUCUNE_RAISON:
        {
            e.ajoute_message("Aucune raison donnée.\n");
            break;
        }
    }
}

void lance_erreur_fonction_inconnue(EspaceDeTravail const &espace,
                                    NoeudExpression const *b,
                                    kuri::tablet<ErreurAppariement, 10> const &erreurs)
{
    auto e = espace.rapporte_erreur(
        b, "Dans l'expression d'appel :", erreur::Genre::FONCTION_INCONNUE);

    if (erreurs.est_vide()) {
        e.ajoute_message("\nFonction inconnue : aucune candidate trouvée\n");
        e.ajoute_message("Vérifiez que la fonction existe bel et bien dans un fichier importé\n");
        return;
    }
    e.ajoute_message("Aucune candidate trouvée pour l'expression « ",
                     chaine_expression(espace, b->comme_appel()->expression),
                     " » !\n");

    for (auto &dc : erreurs) {
        auto decl = dc.noeud_decl;
        e.ajoute_message("\nCandidate :");

        if (decl != nullptr) {
            auto const &lexeme_df = decl->lexème;
            auto fichier_df = espace.fichier(lexeme_df->fichier);
            auto pos_df = position_lexeme(*lexeme_df);

            e.ajoute_message(' ',
                             decl->ident->nom,
                             " (trouvée à ",
                             fichier_df->chemin(),
                             ':',
                             pos_df.numero_ligne,
                             ")\n");
        }
        else {
            e.ajoute_message('\n');
        }

        imprime_erreur_pour_erreur_fonction(e, espace, dc);
    }
}

void lance_erreur_fonction_nulctx(EspaceDeTravail const &espace,
                                  NoeudExpression const *appl_fonc,
                                  NoeudExpression const *decl_fonc,
                                  NoeudExpression const *decl_appel)
{
    espace
        .rapporte_erreur(
            appl_fonc,
            "Ne peut appeler une fonction avec contexte dans une fonction sans contexte !",
            Genre::APPEL_INVALIDE)
        .ajoute_message("Note : la fonction est appelée dans « ",
                        decl_fonc->ident->nom,
                        " » qui fut déclarée sans contexte via #!nulctx.\n\n")
        .ajoute_message("« ", decl_fonc->ident->nom, " » fut déclarée ici :\n")
        .ajoute_site(decl_fonc)
        .ajoute_message("\n\n")
        .ajoute_message("« ", decl_appel->ident->nom, " » fut déclarée ici :\n")
        .ajoute_site(decl_appel)
        .ajoute_message("\n\n");
}

void lance_erreur_acces_hors_limites(EspaceDeTravail const &espace,
                                     NoeudExpression const *b,
                                     int64_t taille_tableau,
                                     Type const *type_tableau,
                                     int64_t indice_acces)
{
    espace.rapporte_erreur(b, "Accès au tableau hors de ses limites !", Genre::NORMAL)
        .ajoute_message("\tLe tableau a une taille de ",
                        taille_tableau,
                        " (de type : ",
                        chaine_type(type_tableau),
                        ").\n")
        .ajoute_message("\tL'accès se fait à l'index ",
                        indice_acces,
                        " (index maximal : ",
                        taille_tableau - 1,
                        ").\n");
}

struct CandidatRubrique {
    int64_t distance = 0;
    kuri::chaine_statique chaine = "";
};

static auto trouve_candidat(kuri::ensemble<kuri::chaine_statique> const &rubriques,
                            kuri::chaine_statique const &nom_donne)
{
    auto candidat = CandidatRubrique{};
    candidat.distance = 1000;

    rubriques.pour_chaque_element([&](kuri::chaine_statique nom_rubrique) {
        auto candidat_possible = CandidatRubrique();
        candidat_possible.distance = distance_levenshtein(nom_donne, nom_rubrique);
        candidat_possible.chaine = nom_rubrique;

        if (candidat_possible.distance < candidat.distance) {
            candidat = candidat_possible;
        }

        return kuri::DécisionItération::Continue;
    });

    return candidat;
}

void rubrique_inconnu(EspaceDeTravail const &espace,
                      NoeudExpression const *acces,
                      NoeudExpression const *rubrique,
                      TypeCompose const *type)
{
    auto rubriques = kuri::ensemble<kuri::chaine_statique>();

    POUR (type->rubriques) {
        rubriques.insère(it.nom->nom);
    }

    const char *message;

    if (type->est_type_énum()) {
        message = "de l'énumération";
    }
    else if (type->est_type_union()) {
        message = "de l'union";
    }
    else if (type->est_type_erreur()) {
        message = "de l'erreur";
    }
    else {
        message = "de la structure";
    }

    /* Les discriminations sur des unions peuvent avoir des expressions d'appel pour capturer le
     * rubrique. */
    if (rubrique->est_appel()) {
        rubrique = rubrique->comme_appel()->expression;
    }

    auto candidat = trouve_candidat(rubriques, rubrique->ident->nom);

    auto e = espace.rapporte_erreur(
        acces, "Dans l'expression d'accès de rubrique", Genre::RUBRIQUE_INCONNUE);
    e.ajoute_message("Le rubrique « ", rubrique->ident->nom, " » est inconnu !\n\n");

    if (rubriques.taille() == 0) {
        e.ajoute_message("Aucune rubrique connu !\n");
    }
    else {
        if (rubriques.taille() <= 32) {
            e.ajoute_message("Les rubriques ", message, " sont :\n");

            rubriques.pour_chaque_element(
                [&](kuri::chaine_statique it) { e.ajoute_message("\t", it, "\n"); });
        }
        else {
            /* Évitons de spammer la sortie. */
            e.ajoute_message("Note : la structure possède un nombre de rubriques trop important "
                             "pour tous les afficher.\n");
        }

        e.ajoute_message("\nCandidat possible : ", candidat.chaine, "\n");
    }
}

void valeur_manquante_discr(EspaceDeTravail const &espace,
                            NoeudExpression const *expression,
                            kuri::ensemble<kuri::chaine_statique> const &valeurs_manquantes)
{
    auto e = espace.rapporte_erreur(
        expression, "Dans l'expression de discrimination", Genre::NORMAL);

    if (valeurs_manquantes.taille() == 1) {
        e.ajoute_message("Une valeur n'est pas prise en compte :\n");
    }
    else {
        e.ajoute_message("Plusieurs valeurs ne sont pas prises en compte :\n");
    }

    valeurs_manquantes.pour_chaque_element([&](kuri::chaine_statique it) {
        e.ajoute_message("\t", it, "\n");
        return kuri::DécisionItération::Continue;
    });
}

void fonction_principale_manquante(EspaceDeTravail const &espace)
{
    espace.rapporte_erreur_sans_site("impossible de trouver la fonction principale")
        .ajoute_message("Veuillez vérifier qu'elle soit bien présente dans un module");
}

void imprime_site(Enchaineuse &enchaineuse,
                  const EspaceDeTravail &espace,
                  const NoeudExpression *site)
{
    if (site == nullptr) {
        return;
    }

    auto lexeme = site->lexème;
    auto fichier = espace.fichier(lexeme->fichier);

    if (fichier->source == SourceFichier::DISQUE) {
        enchaineuse << fichier->chemin();
    }
    else {
        enchaineuse << ".chaine_ajoutées";
    }
    enchaineuse << ':' << lexeme->ligne + 1 << '\n';

    auto const etendue = donne_étendue_source_noeud(site);
    auto const pos = position_lexeme(*lexeme);
    auto const pos_mot = pos.pos;
    auto const ligne = fichier->tampon()[pos.indice_ligne];
    enchaineuse << ligne;
    enchaineuse.imprime_caractère_vide(etendue.colonne_début, ligne);
    enchaineuse.imprime_tilde(ligne, etendue.colonne_début, pos_mot);
    enchaineuse << '^';
    enchaineuse.imprime_tilde(ligne, pos_mot + 1, etendue.colonne_fin);
    enchaineuse << '\n';
}

kuri::chaine imprime_site(const EspaceDeTravail &espace, const NoeudExpression *site)
{
    if (site == nullptr) {
        return "aucun site";
    }

    Enchaineuse enchaineuse;
    imprime_site(enchaineuse, espace, site);
    return enchaineuse.chaine();
}

}  // namespace erreur

/* ************************************************************************** */

Erreur::Erreur(EspaceDeTravail const *espace_, bool est_avertissement_)
    : espace(espace_), est_avertissement(est_avertissement_)
{
}

Erreur::~Erreur() noexcept(false)
{
    if (fut_bougee) {
        return;
    }

    if (est_avertissement) {
        espace->m_compilatrice.rapporte_avertissement(enchaineuse.chaine());
    }
    else {
        espace->m_compilatrice.rapporte_erreur(espace, enchaineuse.chaine(), genre);
    }
}

Erreur &Erreur::ajoute_message(kuri::chaine_statique m)
{
    enchaineuse << m;
    return *this;
}

Erreur &Erreur::ajoute_site(const NoeudExpression *site)
{
    assert(espace);
    imprime_ligne_avec_message(enchaineuse, espace->site_source_pour(site), "");
    enchaineuse << '\n';
    return *this;
}

Erreur &Erreur::ajoute_conseil(kuri::chaine_statique c)
{
    enchaineuse << "\033[4mConseil\033[00m : " << c;
    return *this;
}

static kuri::chaine_statique chaine_pour_erreur(erreur::Genre genre)
{
    switch (genre) {
        default:
        {
            return "ERREUR";
        }
        case erreur::Genre::LEXAGE:
        {
            return "ERREUR DE LEXAGE";
        }
        case erreur::Genre::SYNTAXAGE:
        {
            return "ERREUR DE SYNTAXAGE";
        }
        case erreur::Genre::TYPE_INCONNU:
        case erreur::Genre::TYPE_DIFFERENTS:
        case erreur::Genre::TYPE_ARGUMENT:
        {
            return "ERREUR DE TYPAGE";
        }
        case erreur::Genre::AVERTISSEMENT:
        {
            return "AVERTISSEMENT";
        }
        case erreur::Genre::INFO:
        {
            return "INFO";
        }
    }

    return "ERREUR";
}

#define COULEUR_NORMALE "\033[0m"
#define COULEUR_CYAN_GRAS "\033[1;36m"

static bool est_dirigé_vers_un_terminal()
{
#ifdef _MSC_VER
    // À FAIRE(windows)
    return true;
#else
    return isatty(STDOUT_FILENO) && isatty(STDERR_FILENO);
#endif
}

static kuri::chaine génère_entête_erreur(EspaceDeTravail const *espace,
                                         ParamètresErreurExterne const &params,
                                         erreur::Genre genre)
{
    auto flux = Enchaineuse();
    const auto chaine_erreur = chaine_pour_erreur(erreur::Genre::NORMAL);

    if (est_dirigé_vers_un_terminal()) {
        flux << COULEUR_CYAN_GRAS;
    }

    flux << "-- ";
    flux << chaine_erreur << ' ';
    for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
        flux << '-';
    }
    flux << "\n\n";

    if (est_dirigé_vers_un_terminal()) {
        flux << COULEUR_NORMALE;
    }

    flux << "Dans l'espace de travail « " << espace->nom << " » :\n";

    if (genre == erreur::Genre::AVERTISSEMENT) {
        flux << "\nAvertissement : ";
    }
    else if (genre == erreur::Genre::INFO) {
        flux << "\nInfo : ";
    }
    else {
        flux << "\nErreur : ";
    }

    imprime_ligne_avec_message(flux,
                               "",
                               params.chemin_fichier,
                               params.texte_ligne,
                               params.numéro_ligne,
                               params.indice_colonne,
                               params.indice_colonne_début,
                               params.indice_colonne_fin);

    flux << '\n';

    if (params.message.taille() != 0) {
        flux << params.message;
        flux << '\n';
    }

    flux << '\n';

    return flux.chaine();
}

kuri::chaine genere_entete_erreur(EspaceDeTravail const *espace,
                                  SiteSource site,
                                  erreur::Genre genre,
                                  const kuri::chaine_statique message)
{
    auto flux = Enchaineuse();
    const auto chaine_erreur = chaine_pour_erreur(genre);

    if (est_dirigé_vers_un_terminal()) {
        flux << COULEUR_CYAN_GRAS;
    }

    flux << "-- ";
    flux << chaine_erreur << ' ';
    for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
        flux << '-';
    }
    flux << "\n\n";

    if (est_dirigé_vers_un_terminal()) {
        flux << COULEUR_NORMALE;
    }

    flux << "Dans l'espace de travail « " << espace->nom << " » :\n";

    auto fichier = site.fichier;
    /* Fichier peut être nul si nous n'avons pas de site. */
    if (fichier && fichier->source == SourceFichier::CHAINE_AJOUTÉE) {
        if (fichier->métaprogramme_corps_texte) {
            auto métaprogramme = fichier->métaprogramme_corps_texte;
            auto site_métaprogramme =
                métaprogramme->corps_texte_pour_fonction ?
                    espace->site_source_pour(métaprogramme->corps_texte_pour_fonction) :
                    espace->site_source_pour(métaprogramme->corps_texte_pour_structure);
            flux << "Dans le code ajouté à la compilation par le #corps_texte situé "
                 << site_métaprogramme.fichier->chemin() << ':'
                 << site_métaprogramme.indice_ligne + 1 << " :\n";
        }
        else {
            auto site_exécute = fichier->espace_pour_site->site_source_pour(fichier->site);
            flux << "Dans le code ajouté à la compilation via " << site_exécute.fichier->chemin()
                 << ':' << site_exécute.indice_ligne + 1 << " :\n";
        }
    }

    if (fichier) {
        if (genre == erreur::Genre::AVERTISSEMENT) {
            flux << "\nAvertissement : ";
        }
        else if (genre == erreur::Genre::INFO) {
            flux << "\nInfo : ";
        }
        else {
            flux << "\nErreur : ";
        }

        imprime_ligne_avec_message(flux, site, "");
    }

    flux << '\n';

    if (message) {
        flux << message;
        flux << '\n';
    }

    flux << '\n';

    return flux.chaine();
}

Erreur rapporte_avertissement(EspaceDeTravail const *espace,
                              SiteSource site,
                              kuri::chaine_statique message)
{
    auto erreur = Erreur(espace, true);
    erreur.enchaineuse << genere_entete_erreur(
        espace, site, erreur::Genre::AVERTISSEMENT, message);
    return erreur;
}

Erreur rapporte_info(EspaceDeTravail const *espace, SiteSource site, kuri::chaine_statique message)
{
    auto erreur = Erreur(espace, true);
    erreur.enchaineuse << genere_entete_erreur(espace, site, erreur::Genre::INFO, message);
    return erreur;
}

Erreur rapporte_erreur(EspaceDeTravail const *espace,
                       SiteSource site,
                       kuri::chaine_statique message,
                       erreur::Genre genre)
{
    auto erreur = Erreur(espace, false);
    erreur.enchaineuse << genere_entete_erreur(espace, site, genre, message);
    return erreur;
}

Erreur rapporte_avertissement(EspaceDeTravail const *espace, ParamètresErreurExterne const &params)
{
    auto erreur = Erreur(espace, false);
    erreur.enchaineuse << génère_entête_erreur(espace, params, erreur::Genre::AVERTISSEMENT);
    return erreur;
}

Erreur rapporte_info(EspaceDeTravail const *espace, ParamètresErreurExterne const &params)
{
    auto erreur = Erreur(espace, false);
    erreur.enchaineuse << génère_entête_erreur(espace, params, erreur::Genre::INFO);
    return erreur;
}

Erreur rapporte_erreur(EspaceDeTravail const *espace, ParamètresErreurExterne const &params)
{
    auto erreur = Erreur(espace, false);
    erreur.enchaineuse << génère_entête_erreur(espace, params, erreur::Genre::NORMAL);
    return erreur;
}
