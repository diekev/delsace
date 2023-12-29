/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "erreur.h"

#include <iostream>

#include "biblinternes/langage/erreur.hh"
#include "biblinternes/outils/chaine.hh"
#include "biblinternes/outils/numerique.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"
#include "parsage/lexemes.hh"
#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "monomorpheuse.hh"
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
    auto lexeme = expr->lexeme;
    auto fichier = espace.compilatrice().fichier(lexeme->fichier);
    auto etendue_expr = calcule_etendue_noeud(expr);
    auto ligne = fichier->tampon()[lexeme->ligne];
    return kuri::chaine_statique(&ligne[etendue_expr.pos_min],
                                 etendue_expr.pos_max - etendue_expr.pos_min);
}

void lance_erreur(const kuri::chaine &quoi,
                  EspaceDeTravail const &espace,
                  const NoeudExpression *site,
                  Genre type)
{
    espace.rapporte_erreur(site, quoi, type);
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
                          const NoeudExpression *site_original)
{
    espace
        .rapporte_erreur(site_redefinition, "Redéfinition du symbole !", Genre::VARIABLE_REDEFINIE)
        .ajoute_message("Le symbole fut déjà défini ici :\n\n")
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

void lance_erreur_fonction_inconnue(EspaceDeTravail const &espace,
                                    NoeudExpression const *b,
                                    kuri::tablet<ErreurAppariement, 10> const &erreurs)
{
    auto e = espace.rapporte_erreur(
        b, "Dans l'expression d'appel :", erreur::Genre::FONCTION_INCONNUE);

    if (erreurs.est_vide()) {
        e.ajoute_message("\nFonction inconnue : aucune candidate trouvée\n");
        e.ajoute_message("Vérifiez que la fonction existe bel et bien dans un fichier importé\n");
    }
    else {
        e.ajoute_message("Aucune candidate trouvée pour l'expression « ",
                         chaine_expression(espace, b->comme_appel()->expression),
                         " » !\n");

        for (auto &dc : erreurs) {
            auto decl = dc.noeud_decl;
            e.ajoute_message("\nCandidate :");

            if (decl != nullptr) {
                auto const &lexeme_df = decl->lexeme;
                auto fichier_df = espace.compilatrice().fichier(lexeme_df->fichier);
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

            if (dc.raison == MECOMPTAGE_ARGS) {
                e.ajoute_message("\tLe nombre d'arguments de la fonction est incorrect.\n");
                e.ajoute_message("\tRequiers ", dc.nombre_arguments.nombre_requis, " arguments\n");
                e.ajoute_message("\tObtenu ", dc.nombre_arguments.nombre_obtenu, " arguments\n");
                e.genre_erreur(erreur::Genre::NOMBRE_ARGUMENT);
            }
            else if (dc.raison == MENOMMAGE_ARG) {
                e.ajoute_site(dc.site_erreur);
                e.ajoute_message("Argument inconnu");

                if (decl && decl->genre == GenreNoeud::DECLARATION_CORPS_FONCTION) {
                    auto decl_fonc = decl->comme_entete_fonction();
                    e.ajoute_message("\tLes arguments de la fonction sont : \n");

                    for (auto i = 0; i < decl_fonc->params.taille(); ++i) {
                        auto param = decl_fonc->parametre_entree(i);
                        e.ajoute_message("\t\t", param->ident->nom, '\n');
                    }

                    e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
                }
                else if (decl && decl->genre == GenreNoeud::DECLARATION_STRUCTURE) {
                    auto decl_struct = decl->comme_type_structure();

                    if (decl_struct->est_polymorphe) {
                        e.ajoute_message("\tLes paramètres de la structure sont : \n");

                        POUR (*decl_struct->bloc_constantes->membres.verrou_lecture()) {
                            e.ajoute_message("\t\t", it->ident->nom, '\n');
                        }
                    }
                    else {
                        e.ajoute_message("\tLes membres de la structure sont : \n");

                        auto type_struct = decl_struct->type->comme_type_structure();
                        POUR (type_struct->membres) {
                            e.ajoute_message("\t\t", it.nom, '\n');
                        }
                    }

                    e.genre_erreur(erreur::Genre::MEMBRE_INCONNU);
                }
            }
            else if (dc.raison == RENOMMAGE_ARG) {
                e.genre_erreur(erreur::Genre::ARGUMENT_REDEFINI);
                e.ajoute_site(dc.site_erreur);
                e.ajoute_message("L'argument a déjà été nommé");
            }
            else if (dc.raison == MANQUE_NOM_APRES_VARIADIC) {
                e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
                e.ajoute_site(dc.site_erreur);
                e.ajoute_message("Nom d'argument manquant, les arguments doivent être nommés "
                                 "s'ils sont précédés d'arguments déjà nommés");
            }
            else if (dc.raison == NOMMAGE_ARG_POINTEUR_FONCTION) {
                e.ajoute_message(
                    "\tLes arguments d'un pointeur fonction ne peuvent être nommés\n");
                e.genre_erreur(erreur::Genre::ARGUMENT_INCONNU);
            }
            else if (dc.raison == TYPE_N_EST_PAS_FONCTION) {
                e.ajoute_message("\tAppel d'une variable n'étant pas un pointeur de fonction\n");
                e.genre_erreur(erreur::Genre::FONCTION_INCONNUE);
            }
            else if (dc.raison == TROP_D_EXPRESSION_POUR_UNION) {
                e.ajoute_message(
                    "\tOn ne peut initialiser qu'un seul membre d'une union à la fois\n");
                e.genre_erreur(erreur::Genre::NORMAL);
            }
            else if (dc.raison == EXPRESSION_MANQUANTE_POUR_UNION) {
                e.ajoute_message("\tOn doit initialiser au moins un membre de l'union\n");
                e.genre_erreur(erreur::Genre::NORMAL);
            }
            else if (dc.raison == EXPANSION_VARIADIQUE_FONCTION_EXTERNE) {
                e.ajoute_message("\tImpossible d'utiliser une expansion variadique dans une "
                                 "fonction variadique externe\n");
                e.genre_erreur(erreur::Genre::NORMAL);
            }
            else if (dc.raison == MULTIPLE_EXPANSIONS_VARIADIQUES) {
                e.ajoute_message("\tPlusieurs expansions variadiques trouvées\n");
                e.genre_erreur(erreur::Genre::NORMAL);
            }
            else if (dc.raison == EXPANSION_VARIADIQUE_APRES_ARGUMENTS_VARIADIQUES) {
                e.ajoute_message("\tTentative d'utiliser une expansion d'arguments variadiques "
                                 "alors que d'autres arguments ont déjà été précisés\n");
                e.genre_erreur(erreur::Genre::NORMAL);
            }
            else if (dc.raison == ARGUMENTS_VARIADIQEUS_APRES_EXPANSION_VARIAQUES) {
                e.ajoute_message("\tTentative d'ajouter des arguments variadiques supplémentaire "
                                 "alors qu'une expansion est également utilisée\n");
                e.genre_erreur(erreur::Genre::NORMAL);
            }
            else if (dc.raison == ARGUMENTS_MANQUANTS) {
                if (dc.arguments_manquants_.taille() == 1) {
                    e.ajoute_message("\tUn argument est manquant :\n");
                }
                else {
                    e.ajoute_message("\tPlusieurs arguments sont manquants :\n");
                }

                for (auto ident : dc.arguments_manquants_) {
                    e.ajoute_message("\t\t", ident->nom, '\n');
                }
            }
            else if (dc.raison == METYPAGE_ARG) {
                e.ajoute_message("\tLe type de l'argument '",
                                 chaine_expression(espace, dc.site_erreur),
                                 "' ne correspond pas à celui requis !\n");
                e.ajoute_message(
                    "\tRequiers : ", chaine_type(dc.type_arguments.type_attendu), '\n');
                e.ajoute_message(
                    "\tObtenu   : ", chaine_type(dc.type_arguments.type_obtenu), '\n');
                e.genre_erreur(erreur::Genre::TYPE_ARGUMENT);
            }
            else if (dc.raison == MONOMORPHISATION) {
                e.ajoute_message(dc.erreur_monomorphisation.message());
            }
        }
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
                                     int64_t index_acces)
{
    espace.rapporte_erreur(b, "Accès au tableau hors de ses limites !", Genre::NORMAL)
        .ajoute_message("\tLe tableau a une taille de ",
                        taille_tableau,
                        " (de type : ",
                        chaine_type(type_tableau),
                        ").\n")
        .ajoute_message("\tL'accès se fait à l'index ",
                        index_acces,
                        " (index maximal : ",
                        taille_tableau - 1,
                        ").\n");
}

struct CandidatMembre {
    int64_t distance = 0;
    kuri::chaine_statique chaine = "";
};

static auto trouve_candidat(kuri::ensemble<kuri::chaine_statique> const &membres,
                            kuri::chaine_statique const &nom_donne)
{
    auto candidat = CandidatMembre{};
    candidat.distance = 1000;

    membres.pour_chaque_element([&](kuri::chaine_statique nom_membre) {
        auto candidat_possible = CandidatMembre();
        candidat_possible.distance = distance_levenshtein(nom_donne, nom_membre);
        candidat_possible.chaine = nom_membre;

        if (candidat_possible.distance < candidat.distance) {
            candidat = candidat_possible;
        }

        return kuri::DécisionItération::Continue;
    });

    return candidat;
}

void membre_inconnu(EspaceDeTravail const &espace,
                    NoeudExpression const *acces,
                    NoeudExpression const *membre,
                    TypeCompose const *type)
{
    auto membres = kuri::ensemble<kuri::chaine_statique>();

    POUR (type->membres) {
        membres.insère(it.nom->nom);
    }

    const char *message;

    if (type->est_type_enum()) {
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
     * membre. */
    if (membre->est_appel()) {
        membre = membre->comme_appel()->expression;
    }

    auto candidat = trouve_candidat(membres, membre->ident->nom);

    auto e = espace.rapporte_erreur(
        acces, "Dans l'expression d'accès de membre", Genre::MEMBRE_INCONNU);
    e.ajoute_message("Le membre « ", membre->ident->nom, " » est inconnu !\n\n");

    if (membres.taille() == 0) {
        e.ajoute_message("Aucun membre connu !\n");
    }
    else {
        if (membres.taille() <= 32) {
            e.ajoute_message("Les membres ", message, " sont :\n");

            membres.pour_chaque_element(
                [&](kuri::chaine_statique it) { e.ajoute_message("\t", it, "\n"); });
        }
        else {
            /* Évitons de spammer la sortie. */
            e.ajoute_message("Note : la structure possède un nombre de membres trop important "
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

    auto lexeme = site->lexeme;
    auto fichier = espace.compilatrice().fichier(lexeme->fichier);

    if (fichier->source == SourceFichier::DISQUE) {
        enchaineuse << fichier->chemin();
    }
    else {
        enchaineuse << ".chaine_ajoutées";
    }
    enchaineuse << ':' << lexeme->ligne + 1 << '\n';

    auto const etendue = calcule_etendue_noeud(site);
    auto const pos = position_lexeme(*lexeme);
    auto const pos_mot = pos.pos;
    auto const ligne = fichier->tampon()[pos.index_ligne];
    enchaineuse << ligne;
    lng::erreur::imprime_caractere_vide(enchaineuse, etendue.pos_min, ligne);
    lng::erreur::imprime_tilde(enchaineuse, ligne, etendue.pos_min, pos_mot);
    enchaineuse << '^';
    lng::erreur::imprime_tilde(enchaineuse, ligne, pos_mot + 1, etendue.pos_max);
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

Erreur::Erreur(EspaceDeTravail const *espace_) : espace(espace_)
{
}

Erreur::~Erreur() noexcept(false)
{
    if (!fut_bougee) {
        espace->m_compilatrice.rapporte_erreur(espace, enchaineuse.chaine(), genre);
    }
}

Erreur &Erreur::ajoute_message(const kuri::chaine &m)
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

Erreur &Erreur::ajoute_conseil(const kuri::chaine &c)
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
    }

    return "ERREUR";
}

#define COULEUR_NORMALE "\033[0m"
#define COULEUR_CYAN_GRAS "\033[1;36m"

kuri::chaine genere_entete_erreur(EspaceDeTravail const *espace,
                                  SiteSource site,
                                  erreur::Genre genre,
                                  const kuri::chaine_statique message)
{
    auto flux = Enchaineuse();
    const auto chaine_erreur = chaine_pour_erreur(genre);

    flux << COULEUR_CYAN_GRAS << "-- ";
    flux << chaine_erreur << ' ';
    for (auto i = 0; i < 76 - chaine_erreur.taille(); ++i) {
        flux << '-';
    }
    flux << "\n\n" << COULEUR_NORMALE;

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
                 << site_métaprogramme.index_ligne + 1 << " :\n";
        }
        else {
            auto site_exécute = espace->site_source_pour(fichier->site);
            flux << "Dans le code ajouté à la compilation via " << site_exécute.fichier->chemin()
                 << ':' << site_exécute.index_ligne + 1 << " :\n";
        }
    }

    if (fichier) {
        if (genre != erreur::Genre::AVERTISSEMENT) {
            flux << "\nErreur : ";
        }
        else {
            flux << "\nAvertissement : ";
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

Erreur rapporte_erreur(EspaceDeTravail const *espace,
                       SiteSource site,
                       kuri::chaine const &message,
                       erreur::Genre genre)
{
    auto erreur = Erreur(espace);
    erreur.enchaineuse << genere_entete_erreur(espace, site, genre, message);
    return erreur;
}
