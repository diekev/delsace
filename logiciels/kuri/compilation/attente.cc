/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "representation_intermediaire/instructions.hh"

#include "structures/enchaineuse.hh"

#include "attente.hh"
#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "message.hh"
#include "metaprogramme.hh"
#include "typage.hh"
#include "unite_compilation.hh"

struct UniteCompilation;

#define NOM_RAPPEL_POUR_UNITÉ(__nom__) unite_pour_attente_##__nom__
#define RAPPEL_POUR_UNITÉ(__nom__)                                                                \
    static UniteCompilation *NOM_RAPPEL_POUR_UNITÉ(__nom__)(Attente const &attente)

#define NOM_RAPPEL_POUR_COMMENTAIRE(__nom__) commentaire_pour_##__nom__
#define RAPPEL_POUR_COMMENTAIRE(__nom__)                                                          \
    kuri::chaine NOM_RAPPEL_POUR_COMMENTAIRE(__nom__)(Attente const &attente)

#define NOM_RAPPEL_POUR_EST_RÉSOLUE(__nom__) est_résolue_sur_##__nom__
#define RAPPEL_POUR_EST_RÉSOLUE(__nom__)                                                          \
    bool NOM_RAPPEL_POUR_EST_RÉSOLUE(__nom__)(EspaceDeTravail * espace, Attente & attente)

#define NOM_RAPPEL_POUR_ERREUR(__nom__) émets_erreur_pour_attente_sur_##__nom__
#define RAPPEL_POUR_ERREUR(__nom__)                                                               \
    void NOM_RAPPEL_POUR_ERREUR(__nom__)(UniteCompilation const *unite, Attente const &attente)

/** -----------------------------------------------------------------
 * \{ */

static ConditionBlocageAttente condition_blocage_défaut(Attente const & /*attente*/)
{
    return {PhaseCompilation::PARSAGE_TERMINE};
}

static void émets_erreur_pour_attente_défaut(UniteCompilation const *unite, Attente const &attente)
{
    auto espace = unite->espace;
    auto noeud = unite->noeud;
    espace
        ->rapporte_erreur(noeud,
                          "Je ne peux pas continuer la compilation car une unité est "
                          "bloquée dans un cycle")
        .ajoute_message("\nNote : l'unité est dans l'état : ")
        .ajoute_message(unite->chaine_attentes_récursives())
        .ajoute_message("\n");
}

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurType
 * \{ */

RAPPEL_POUR_UNITÉ(type)
{
    auto type_attendu = attente.type();

    if (!type_attendu) {
        return nullptr;
    }

    assert(attente.est_valide());

    return type_attendu->unité;
}

RAPPEL_POUR_COMMENTAIRE(type)
{
    auto type_attendu = attente.type();
    return enchaine("(type) ", chaine_type(type_attendu));
}

RAPPEL_POUR_EST_RÉSOLUE(type)
{
    return attente.type()->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE);
}

RAPPEL_POUR_ERREUR(type)
{
    auto espace = unite->espace;

    /* À FAIRE : garantis un site valide pour chaque attente. */
    auto site = NoeudExpression::nul();
    switch (unite->donne_raison_d_être()) {
        case RaisonDEtre::AUCUNE:
        case RaisonDEtre::CHARGEMENT_FICHIER:
        case RaisonDEtre::LEXAGE_FICHIER:
        case RaisonDEtre::PARSAGE_FICHIER:
        case RaisonDEtre::CREATION_FONCTION_INIT_TYPE:
        case RaisonDEtre::CONVERSION_NOEUD_CODE:
        case RaisonDEtre::ENVOIE_MESSAGE:
        case RaisonDEtre::GENERATION_RI:
        case RaisonDEtre::GENERATION_RI_PRINCIPALE_MP:
        case RaisonDEtre::EXECUTION:
        case RaisonDEtre::LIAISON_PROGRAMME:
        case RaisonDEtre::GENERATION_CODE_MACHINE:
        {
            break;
        }
        case RaisonDEtre::TYPAGE:
        {
            site = unite->noeud;
            break;
        }
    }

    if (site && site->est_corps_fonction()) {
        if (unite->arbre_aplatis) {
            site = unite->arbre_aplatis->noeuds[unite->arbre_aplatis->index_courant];
        }
    }

    espace
        ->rapporte_erreur(site,
                          "Je ne peux pas continuer la compilation car je n'arrive "
                          "pas à déterminer un type pour l'expression",
                          erreur::Genre::TYPE_INCONNU)
        .ajoute_message("Note : le type attendu est ")
        .ajoute_message(chaine_type(attente.type()))
        .ajoute_message("\n")
        .ajoute_message("Note : l'unité de compilation est dans cette état :\n")
        .ajoute_message(unite->chaine_attentes_récursives())
        .ajoute_message("\n");
}

InfoTypeAttente info_type_attente_sur_type = {NOM_RAPPEL_POUR_UNITÉ(type),
                                              condition_blocage_défaut,
                                              NOM_RAPPEL_POUR_COMMENTAIRE(type),
                                              NOM_RAPPEL_POUR_EST_RÉSOLUE(type),
                                              NOM_RAPPEL_POUR_ERREUR(type)};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurDeclaration
 * \{ */

RAPPEL_POUR_UNITÉ(déclaration)
{
    auto adresse = donne_adresse_unité(attente.declaration());
    if (adresse) {
        return *adresse;
    }
    return nullptr;
}

RAPPEL_POUR_COMMENTAIRE(declaration)
{
    return enchaine("(decl) ", attente.declaration()->ident->nom);
}

RAPPEL_POUR_EST_RÉSOLUE(déclaration)
{
    auto declaration_attendue = attente.declaration();
    if (!declaration_attendue->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        return false;
    }

    // À FAIRE : est-ce nécessaire ?
    if (declaration_attendue == espace->compilatrice().interface_kuri->decl_creation_contexte) {
        /* Pour crée_contexte, change l'attente pour attendre sur la RI corps car il
         * nous faut le code. */
        attente = Attente::sur_ri(&declaration_attendue->comme_entete_fonction()->atome);
        return false;
    }

    return true;
}

RAPPEL_POUR_ERREUR(déclaration)
{
    auto espace = unite->espace;
    auto decl = attente.declaration();
    auto unite_decl = donne_adresse_unité(decl);
    auto erreur = espace->rapporte_erreur(
        decl, "Je ne peux pas continuer la compilation car une déclaration ne peut être typée.");

    // À FAIRE : ne devrait pas arriver
    if (unite_decl && *unite_decl) {
        erreur.ajoute_message("Note : l'unité de compilation est dans cette état :\n")
            .ajoute_message(unite->chaine_attentes_récursives())
            .ajoute_message("\n");
    }
}

InfoTypeAttente info_type_attente_sur_déclaration = {NOM_RAPPEL_POUR_UNITÉ(déclaration),
                                                     condition_blocage_défaut,
                                                     NOM_RAPPEL_POUR_COMMENTAIRE(declaration),
                                                     NOM_RAPPEL_POUR_EST_RÉSOLUE(déclaration),
                                                     NOM_RAPPEL_POUR_ERREUR(déclaration)};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurOperateur
 * \{ */

RAPPEL_POUR_UNITÉ(opérateur)
{
    return nullptr;
}

RAPPEL_POUR_COMMENTAIRE(opérateur)
{
    return enchaine("opérateur ", attente.operateur()->lexeme->chaine);
}

/* À FAIRE(gestion) : détermine comment détecter la disponibilité d'un opérateur.
 *
 * Il y a deux cas spéciaux, en dehors de la définition par l'utilisateur d'un opérateur :
 * - les énumérations, et
 * - les types de bases (hors tableaux, etc.).
 *
 * Il faudra sans doute ajouter un noeud syntaxique pour contenir les données des
 * opérateurs et créer un tel noeud pour tous les types, qui devra ensuite passer par la
 * validation de code.
 */
RAPPEL_POUR_EST_RÉSOLUE(opérateur)
{
    auto p = espace->phase_courante();
    return p < PhaseCompilation::PARSAGE_TERMINE;
}

static void imprime_operateurs_pour(Erreur &e,
                                    Type &type,
                                    NoeudExpression const &operateur_attendu)
{
    if (!type.table_opérateurs) {
        e.ajoute_message("\nNOTE : le type ", chaine_type(&type), " n'a aucun opérateur\n");
        return;
    }

    auto &operateurs = type.table_opérateurs->opérateurs(operateur_attendu.lexeme->genre);

    e.ajoute_message("\nNOTE : les opérateurs du type ", chaine_type(&type), " sont :\n");
    POUR (operateurs.plage()) {
        e.ajoute_message("    ",
                         chaine_type(it->type1),
                         " ",
                         operateur_attendu.lexeme->chaine,
                         " ",
                         chaine_type(it->type2),
                         "\n");
    }
}

RAPPEL_POUR_ERREUR(opérateur)
{
    auto espace = unite->espace;
    auto operateur_attendu = attente.operateur();
    if (operateur_attendu->est_expression_binaire() || operateur_attendu->est_indexage()) {
        auto expression_operation = operateur_attendu->comme_expression_binaire();
        auto type1 = expression_operation->operande_gauche->type;
        auto type2 = expression_operation->operande_droite->type;

        auto candidats = kuri::tablet<OpérateurCandidat, 10>();
        auto resultat = cherche_candidats_opérateurs(
            *espace, type1, type2, GenreLexeme::CROCHET_OUVRANT, candidats);

        Erreur e = espace->rapporte_erreur(operateur_attendu,
                                           "Je ne peux pas continuer la compilation car je "
                                           "n'arrive pas à déterminer quel opérateur appeler.",
                                           erreur::Genre::TYPE_INCONNU);

        if (!resultat.has_value()) {
            POUR (candidats) {
                auto op = it.op;
                if (!op || !op->decl) {
                    continue;
                }

                e.ajoute_message("Candidat :\n");
                e.ajoute_site(it.op->decl);

                if (it.transformation_type1.type == TypeTransformation::IMPOSSIBLE) {
                    e.ajoute_message(
                        "Impossible de convertir implicitement le type à gauche (qui est ",
                        chaine_type(type1),
                        ") vers ",
                        chaine_type(it.op->type1),
                        '\n');
                }

                if (it.transformation_type2.type == TypeTransformation::IMPOSSIBLE) {
                    e.ajoute_message(
                        "Impossible de convertir implicitement le type à droite (qui est ",
                        chaine_type(type2),
                        ") vers ",
                        chaine_type(it.op->type2),
                        '\n');
                }

                e.ajoute_message('\n');
            }
        }

        imprime_operateurs_pour(e, *type1, *operateur_attendu);

        if (type1 != type2) {
            imprime_operateurs_pour(e, *type2, *operateur_attendu);
        }

        e.ajoute_conseil("\nSi vous voulez performer une opération sur des types "
                         "non-communs, vous pouvez définir vos propres opérateurs avec "
                         "la syntaxe suivante :\n\n");
        e.ajoute_message("opérateur ",
                         operateur_attendu->lexeme->chaine,
                         " :: fonc (a: ",
                         chaine_type(type1),
                         ", b: ",
                         chaine_type(type2),
                         ")");
        e.ajoute_message(" -> TypeRetour\n");
        e.ajoute_message("{\n\tretourne ...\n}\n");
    }
    else {
        auto expression_operation = operateur_attendu->comme_expression_unaire();
        auto type_operande = expression_operation->operande->type;
        espace
            ->rapporte_erreur(operateur_attendu,
                              "Je ne peux pas continuer la compilation car je "
                              "n'arrive pas à déterminer quel opérateur appeler.",
                              erreur::Genre::TYPE_INCONNU)
            .ajoute_message("\nLe type à droite de l'opérateur est ")
            .ajoute_message(chaine_type(type_operande))
            .ajoute_message("\n\nMais aucun opérateur ne correspond à ces types-là.\n\n")
            .ajoute_conseil("Si vous voulez performer une opération sur des types "
                            "non-communs, vous pouvez définir vos propres opérateurs avec "
                            "la syntaxe suivante :\n\n")
            .ajoute_message("opérateur ",
                            operateur_attendu->lexeme->chaine,
                            " :: fonc (a: ",
                            chaine_type(type_operande),
                            ")")
            .ajoute_message(" -> TypeRetour\n")
            .ajoute_message("{\n\tretourne ...\n}\n");
    }
}

InfoTypeAttente info_type_attente_sur_opérateur = {NOM_RAPPEL_POUR_UNITÉ(opérateur),
                                                   condition_blocage_défaut,
                                                   NOM_RAPPEL_POUR_COMMENTAIRE(opérateur),
                                                   NOM_RAPPEL_POUR_EST_RÉSOLUE(opérateur),
                                                   NOM_RAPPEL_POUR_ERREUR(opérateur)};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurMetaProgramme
 * \{ */

RAPPEL_POUR_UNITÉ(métaprogramme)
{
    auto metaprogramme_attendu = attente.metaprogramme();
    // À FAIRE(gestion) : le métaprogramme attend sur l'unité de la fonction
    // il nous faudra sans doute une raison pour l'attente (RI, CODE, etc.).
    return metaprogramme_attendu->fonction->unité;
}

RAPPEL_POUR_COMMENTAIRE(métaprogramme)
{
    auto metaprogramme_attendu = attente.metaprogramme();
    auto resultat = Enchaineuse();
    resultat << "métaprogramme";

    if (metaprogramme_attendu->corps_texte) {
        resultat << " #corps_texte pour ";

        if (metaprogramme_attendu->corps_texte_pour_fonction) {
            resultat << metaprogramme_attendu->corps_texte_pour_fonction->ident->nom;
        }
        else if (metaprogramme_attendu->corps_texte_pour_structure) {
            resultat << metaprogramme_attendu->corps_texte_pour_structure->ident->nom;
        }
        else {
            resultat << " ERREUR COMPILATRICE";
        }
    }
    else if (metaprogramme_attendu->directive) {
        auto directive = metaprogramme_attendu->directive;
        auto expression = directive->expression;
        auto appel = expression->comme_appel();
        resultat << " " << appel->expression->ident->nom;
    }
    else {
        resultat << " " << metaprogramme_attendu;
    }

    resultat << " (fut exécuté : " << metaprogramme_attendu->fut_execute << ")\n";

    return resultat.chaine();
}

RAPPEL_POUR_EST_RÉSOLUE(métaprogramme)
{
    auto metaprogramme_attendu = attente.metaprogramme();
    return metaprogramme_attendu->fut_execute;
}

/* À FAIRE(condition blocage) : vérifie que le métaprogramme est en cours d'exécution ? */
InfoTypeAttente info_type_attente_sur_métaprogramme = {NOM_RAPPEL_POUR_UNITÉ(métaprogramme),
                                                       nullptr,
                                                       NOM_RAPPEL_POUR_COMMENTAIRE(métaprogramme),
                                                       NOM_RAPPEL_POUR_EST_RÉSOLUE(métaprogramme),
                                                       émets_erreur_pour_attente_défaut};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurRI
 * \{ */

RAPPEL_POUR_UNITÉ(ri)
{
    auto ri_attendue = *attente.ri();
    if (!ri_attendue || !ri_attendue->est_fonction()) {
        return nullptr;
    }
    auto fonction = ri_attendue->comme_fonction();
    if (!fonction->decl) {
        return nullptr;
    }
    return fonction->decl->unité;
}

RAPPEL_POUR_COMMENTAIRE(ri)
{
    auto ri_attendue = *attente.ri();
    if (ri_attendue == nullptr) {
        return "RI, mais la RI ne fut pas générée !";
    }

    if (ri_attendue->est_fonction()) {
        auto fonction = ri_attendue->comme_fonction();
        if (fonction->decl) {
            auto decl = fonction->decl;
            if (decl->ident) {
                return enchaine("RI de ", fonction->decl->ident->nom);
            }
            /* Utilisation du lexème, par exemple pour les opérateurs. */
            return enchaine("RI de ", fonction->decl->lexeme->chaine);
        }
        return enchaine("RI d'une fonction inconnue");
    }

    if (ri_attendue->est_globale()) {
        auto globale = ri_attendue->comme_globale();
        if (globale->ident) {
            return enchaine("RI de la globale ", globale->ident->nom);
        }
        return enchaine("RI d'une globale anonyme");
    }

    return enchaine("RI de quelque chose inconnue");
}

RAPPEL_POUR_EST_RÉSOLUE(ri)
{
    auto ri_attendue = attente.ri();
    return (*ri_attendue && (*ri_attendue)->possède_drapeau(DrapeauxAtome::RI_FUT_GÉNÉRÉE));
}

InfoTypeAttente info_type_attente_sur_ri = {NOM_RAPPEL_POUR_UNITÉ(ri),
                                            nullptr,
                                            NOM_RAPPEL_POUR_COMMENTAIRE(ri),
                                            NOM_RAPPEL_POUR_EST_RÉSOLUE(ri),
                                            émets_erreur_pour_attente_défaut};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurSymbole
 * \{ */

RAPPEL_POUR_COMMENTAIRE(symbole)
{
    return enchaine("(symbole) ", attente.symbole()->ident->nom);
}

RAPPEL_POUR_EST_RÉSOLUE(symbole)
{
    auto p = espace->phase_courante();
    // À FAIRE : granularise ceci pour ne pas tenter de recompiler quelque chose
    // si le symbole ne fut pas encore défini (par exemple en utilisant un ensemble de symboles
    // définis depuis le dernier ajournement, dans GestionnaireCode::crée_taches).
    return p < PhaseCompilation::PARSAGE_TERMINE;
}

RAPPEL_POUR_ERREUR(symbole)
{
    auto espace = unite->espace;
    espace
        ->rapporte_erreur(attente.symbole(),
                          "Trop de cycles : arrêt de la compilation sur un symbole inconnu")
        .ajoute_message("Le symbole attendu est « ", attente.symbole()->ident->nom, " »");
}

InfoTypeAttente info_type_attente_sur_symbole = {nullptr,
                                                 condition_blocage_défaut,
                                                 NOM_RAPPEL_POUR_COMMENTAIRE(symbole),
                                                 NOM_RAPPEL_POUR_EST_RÉSOLUE(symbole),
                                                 NOM_RAPPEL_POUR_ERREUR(symbole)};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurMessage
 * \{ */

RAPPEL_POUR_UNITÉ(message)
{
    auto données = attente.message();
    return données.unité;
}

RAPPEL_POUR_COMMENTAIRE(message)
{
    auto message = attente.message().message;
    auto resultat = Enchaineuse();
    resultat << "message " << message->genre;

    switch (message->genre) {
        case GenreMessage::FICHIER_OUVERT:
        case GenreMessage::FICHIER_FERME:
        {
            auto message_fichier = static_cast<MessageFichier *>(message);
            resultat << " " << message_fichier->chemin;
            break;
        }
        case GenreMessage::MODULE_OUVERT:
        case GenreMessage::MODULE_FERME:
        {
            auto message_module = static_cast<MessageModule *>(message);
            resultat << " " << message_module->chemin;
            break;
        }
        case GenreMessage::TYPAGE_CODE_TERMINE:
        {
            auto message_code = static_cast<MessageTypageCodeTermine *>(message);
            if (message_code->code) {
                resultat << " " << message_code->code->nom;
            }
            else {
                resultat << " noeud code non encore généré";
            }
            break;
        }
        case GenreMessage::PHASE_COMPILATION:
        {
            auto message_phase = static_cast<MessagePhaseCompilation *>(message);
            resultat << " " << message_phase->phase;
            break;
        }
    }

    resultat << " (espace \"" << message->espace->nom
             << "\"); message reçu : " << (message->message_recu ? "oui" : "non") << "; adresse "
             << message;

    return resultat.chaine();
}

RAPPEL_POUR_EST_RÉSOLUE(message)
{
    auto données = attente.message();
    return données.message->message_recu;
}

InfoTypeAttente info_type_attente_sur_message = {NOM_RAPPEL_POUR_UNITÉ(message),
                                                 nullptr,
                                                 NOM_RAPPEL_POUR_COMMENTAIRE(message),
                                                 NOM_RAPPEL_POUR_EST_RÉSOLUE(message),
                                                 émets_erreur_pour_attente_défaut};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurChargement
 * \{ */

RAPPEL_POUR_COMMENTAIRE(chargement)
{
    return "chargement fichier";
}

RAPPEL_POUR_EST_RÉSOLUE(chargement)
{
    auto fichier_attendu = attente.fichier_a_charger();
    return fichier_attendu->fut_chargé;
}

InfoTypeAttente info_type_attente_sur_chargement = {nullptr,
                                                    nullptr,
                                                    NOM_RAPPEL_POUR_COMMENTAIRE(chargement),
                                                    NOM_RAPPEL_POUR_EST_RÉSOLUE(chargement),
                                                    émets_erreur_pour_attente_défaut};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurLexage
 * \{ */

RAPPEL_POUR_COMMENTAIRE(lexage)
{
    return "lexage fichier";
}

RAPPEL_POUR_EST_RÉSOLUE(lexage)
{
    auto fichier_attendu = attente.fichier_a_lexer();
    return fichier_attendu->fut_lexé;
}

InfoTypeAttente info_type_attente_sur_lexage = {nullptr,
                                                nullptr,
                                                NOM_RAPPEL_POUR_COMMENTAIRE(lexage),
                                                NOM_RAPPEL_POUR_EST_RÉSOLUE(lexage),
                                                émets_erreur_pour_attente_défaut};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurParsage
 * \{ */

RAPPEL_POUR_COMMENTAIRE(parsage)
{
    return "parsage fichier";
}

RAPPEL_POUR_EST_RÉSOLUE(parsage)
{
    auto fichier_attendu = attente.fichier_a_parser();
    return fichier_attendu->fut_parsé;
}

InfoTypeAttente info_type_attente_sur_parsage = {nullptr,
                                                 nullptr,
                                                 NOM_RAPPEL_POUR_COMMENTAIRE(parsage),
                                                 NOM_RAPPEL_POUR_EST_RÉSOLUE(parsage),
                                                 émets_erreur_pour_attente_défaut};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurNoeudCode
 * \{ */

RAPPEL_POUR_UNITÉ(noeud_code)
{
    auto données_noeud_code = attente.noeud_code();
    return données_noeud_code.unité;
}

RAPPEL_POUR_COMMENTAIRE(noeud_code)
{
    return "noeud code";
}

RAPPEL_POUR_EST_RÉSOLUE(noeud_code)
{
    auto données_noeud_code = attente.noeud_code();
    return données_noeud_code.noeud->noeud_code != nullptr;
}

InfoTypeAttente info_type_attente_sur_noeud_code = {NOM_RAPPEL_POUR_UNITÉ(noeud_code),
                                                    nullptr,
                                                    NOM_RAPPEL_POUR_COMMENTAIRE(noeud_code),
                                                    NOM_RAPPEL_POUR_EST_RÉSOLUE(noeud_code),
                                                    émets_erreur_pour_attente_défaut};

/** \} */

/** -----------------------------------------------------------------
 * AttenteSurOpérateurPour
 * \{ */

RAPPEL_POUR_COMMENTAIRE(opérateur_pour)
{
    auto type = attente.opérateur_pour();
    return enchaine("opérateur 'pour' pour ", chaine_type(type));
}

RAPPEL_POUR_EST_RÉSOLUE(opérateur_pour)
{
    auto type = attente.opérateur_pour();
    return type->table_opérateurs != nullptr && type->table_opérateurs->opérateur_pour != nullptr;
}

RAPPEL_POUR_ERREUR(opérateur_pour)
{
    auto espace = unite->espace;
    auto noeud = unite->noeud;
    auto type = attente.opérateur_pour();

    auto message = enchaine(
        "Je ne pas continuer la compilation car une unité attend sur la déclaration "
        "d'un opérateur de boucle « pour » pour le type « ",
        chaine_type(type),
        " » mais aucun opérateur de boucle « pour » ne fut déclaré pour le type.");

    espace->rapporte_erreur(noeud, message);
}

InfoTypeAttente info_type_attente_sur_opérateur_pour = {
    nullptr,
    condition_blocage_défaut,
    NOM_RAPPEL_POUR_COMMENTAIRE(opérateur_pour),
    NOM_RAPPEL_POUR_EST_RÉSOLUE(opérateur_pour),
    NOM_RAPPEL_POUR_ERREUR(opérateur_pour)};

/** \} */
