/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "unite_compilation.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"
#include "metaprogramme.hh"
#include "typage.hh"

static constexpr auto CYCLES_MAXIMUM = 1000;

const char *chaine_rainson_d_etre(RaisonDEtre raison_d_etre)
{
#define ENUMERE_RAISON_D_ETRE_EX(Genre, nom, chaine)                                              \
    case RaisonDEtre::Genre:                                                                      \
        return chaine;
    switch (raison_d_etre) {
        ENUMERE_RAISON_D_ETRE(ENUMERE_RAISON_D_ETRE_EX)
    }
#undef ENUMERE_RAISON_D_ETRE_EX
    return "ceci ne devrait pas arriver";
}

std::ostream &operator<<(std::ostream &os, RaisonDEtre raison_d_etre)
{
    return os << chaine_rainson_d_etre(raison_d_etre);
}

bool UniteCompilation::est_bloquee() const
{
    auto toutes_les_unites_attendues_sont_bloquees = attente_est_bloquee();

    if (!toutes_les_unites_attendues_sont_bloquees) {
        return false;
    }

    auto visitees = kuri::ensemblon<UniteCompilation const *, 16>();
    visitees.insere(this);

    auto attendue = unite_attendue();
    while (attendue) {
        if (visitees.possede(attendue)) {
            /* La dépendance cyclique sera rapportée via le message d'erreur qui appelera
             * « chaine_attente_recursive() ». */
            return true;
        }
        visitees.insere(attendue);
        toutes_les_unites_attendues_sont_bloquees &= attendue->attente_est_bloquee();
        attendue = attendue->unite_attendue();
    }

    return toutes_les_unites_attendues_sont_bloquees;
}

/* Représente la condition pour laquelle l'attente est bloquée. */
struct ConditionBlocageAttente {
    PhaseCompilation phase{};
};

static std::optional<ConditionBlocageAttente> condition_blocage(Attente const &attente)
{
    if (attente.est<AttenteSurType>()) {
        return {{PhaseCompilation::PARSAGE_TERMINE}};
    }

    if (attente.est<AttenteSurSymbole>()) {
        return {{PhaseCompilation::PARSAGE_TERMINE}};
    }

    if (attente.est<AttenteSurDeclaration>()) {
        return {{PhaseCompilation::PARSAGE_TERMINE}};
    }

    if (attente.est<AttenteSurOperateur>()) {
        return {{PhaseCompilation::PARSAGE_TERMINE}};
    }

    if (attente.est<AttenteSurMetaProgramme>()) {
        /* À FAIRE : vérifie que le métaprogramme est en cours d'exécution ? */
        return {};
    }

    if (attente.est<AttenteSurInterfaceKuri>()) {
        return {{PhaseCompilation::PARSAGE_TERMINE}};
    }

    if (attente.est<AttenteSurMessage>()) {
        return {};
    }

    return {};
}

bool UniteCompilation::attente_est_bloquee() const
{
    auto const condition_potentielle = condition_blocage(m_attente);
    if (!condition_potentielle.has_value()) {
        /* Aucune condition potentille pour notre attente, donc nous ne sommes pas bloqués. */
        return false;
    }

    auto const condition = condition_potentielle.value();
    auto const phase_espace = espace->phase_courante();
    auto const id_phase_espace = espace->id_phase_courante();

    if (id_phase_espace != id_phase_cycle) {
        /* L'espace a changé de phase, nos cycles sont invalidés. */
        id_phase_cycle = id_phase_espace;
        cycle = 0;
        return false;
    }

    if (phase_espace < condition.phase) {
        /* L'espace n'a pas dépassé la phase limite, nos cycles sont invalides. */
        cycle = 0;
        return false;
    }

    /* L'espace est sur la phase ou après. Nous avons jusqu'à CYCLES_MAXIMUM pour être satisfaits.
     */
    return cycle > CYCLES_MAXIMUM;
}

kuri::chaine UniteCompilation::commentaire() const
{
    if (m_attente.est<AttenteSurType>()) {
        auto type_attendu = m_attente.type();
        return enchaine("(type) ", chaine_type(type_attendu));
    }

    if (m_attente.est<AttenteSurSymbole>()) {
        return enchaine("(symbole) ", m_attente.symbole()->ident->nom);
    }

    if (m_attente.est<AttenteSurDeclaration>()) {
        return enchaine("(decl) ", m_attente.declaration()->ident->nom);
    }

    if (m_attente.est<AttenteSurOperateur>()) {
        return enchaine("opérateur ", m_attente.operateur()->lexeme->chaine);
    }

    if (m_attente.est<AttenteSurMetaProgramme>()) {
        auto metaprogramme_attendu = m_attente.metaprogramme();
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
        else {
            resultat << " " << metaprogramme_attendu;
        }

        return resultat.chaine();
    }

    if (m_attente.est<AttenteSurRI>()) {
        auto ri_attendue = *m_attente.ri();
        if (ri_attendue == nullptr) {
            return "RI, mais la RI ne fut pas générée !";
        }

        if (ri_attendue->est_fonction()) {
            auto fonction = static_cast<AtomeFonction *>(ri_attendue);
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
            auto globale = static_cast<AtomeGlobale *>(ri_attendue);
            if (globale->ident) {
                return enchaine("RI de la globale ", globale->ident->nom);
            }
            return enchaine("RI d'une globale anonyme");
        }

        return enchaine("RI de quelque chose inconnue");
    }

    if (m_attente.est<AttenteSurInterfaceKuri>()) {
        return enchaine("(interface kuri) ", m_attente.interface_kuri()->nom);
    }

    if (m_attente.est<AttenteSurMessage>()) {
        return "message";
    }

    if (m_attente.est<AttenteSurLexage>()) {
        return "lexage fichier";
    }

    if (m_attente.est<AttenteSurParsage>()) {
        return "parsage fichier";
    }

    if (m_attente.est<AttenteSurChargement>()) {
        return "chargement fichier";
    }

    if (m_attente.est<AttenteSurNoeudCode>()) {
        return "noeud code";
    }

    return "ERREUR COMPILATRICE";
}

UniteCompilation *UniteCompilation::unite_attendue() const
{
    if (m_attente.est<AttenteSurType>()) {
        auto type_attendu = m_attente.type();

        if (!type_attendu) {
            return nullptr;
        }

        assert(m_attente.est_valide());
        auto decl = decl_pour_type(type_attendu);
        if (!decl) {
            /* « decl » peut être nulle si nous attendons sur la fonction d'initialisation d'un
             * type n'étant pas encore typé/parsé (par exemple les types de l'interface Kuri). */
            return nullptr;
        }

        return decl->unite;
    }

    if (m_attente.est<AttenteSurSymbole>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurDeclaration>()) {
        return m_attente.declaration()->unite;
    }

    if (m_attente.est<AttenteSurOperateur>()) {
        return m_attente.operateur()->unite;
    }

    if (m_attente.est<AttenteSurMetaProgramme>()) {
        auto metaprogramme_attendu = m_attente.metaprogramme();
        // À FAIRE(gestion) : le métaprogramme attend sur l'unité de la fonction
        // il nous faudra sans doute une raison pour l'attente (RI, CODE, etc.).
        return metaprogramme_attendu->fonction->unite;
    }

    if (m_attente.est<AttenteSurRI>()) {
        auto ri_attendue = *m_attente.ri();
        if (ri_attendue && ri_attendue->est_fonction()) {
            auto fonction = static_cast<AtomeFonction *>(ri_attendue);
            if (fonction->decl) {
                return fonction->decl->unite;
            }
        }
        return nullptr;
    }

    if (m_attente.est<AttenteSurInterfaceKuri>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurMessage>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurChargement>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurLexage>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurParsage>()) {
        return nullptr;
    }

    if (m_attente.est<AttenteSurNoeudCode>()) {
        return nullptr;
    }

    assert_rappel(!m_attente.est_valide(), [&]() {
        std::cerr << "L'attente est pour " << commentaire() << '\n';
        std::cerr << "La raison d'être de l'unité est " << raison_d_etre() << '\n';
    });
    return nullptr;
}

static void imprime_operateurs_pour(Erreur &e,
                                    Type &type,
                                    NoeudExpression const &operateur_attendu)
{
    auto &operateurs = type.operateurs.operateurs(operateur_attendu.lexeme->genre);

    if (operateurs.taille() == 0) {
        e.ajoute_message("\nNOTE : le type ", chaine_type(&type), " n'a aucun opérateur\n");
    }
    else {
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
}

void UniteCompilation::rapporte_erreur() const
{
    if (m_attente.est<AttenteSurSymbole>()) {
        espace->rapporte_erreur(m_attente.symbole(),
                                "Trop de cycles : arrêt de la compilation sur un symbole inconnu");
    }
    else if (m_attente.est<AttenteSurDeclaration>()) {
        auto decl = m_attente.declaration();
        auto unite_decl = decl->unite;
        auto erreur = espace->rapporte_erreur(
            decl,
            "Je ne peux pas continuer la compilation car une déclaration ne peut être typée.");

        // À FAIRE : ne devrait pas arriver
        if (unite_decl) {
            erreur.ajoute_message("Note : l'unité de compilation est dans cette état :\n")
                .ajoute_message(chaine_attentes_recursives(this))
                .ajoute_message("\n");
        }
    }
    else if (m_attente.est<AttenteSurType>()) {
        auto site = noeud;
        if (site && site->est_corps_fonction()) {
            auto corps = site->comme_corps_fonction();
            site = corps->arbre_aplatis[index_courant];
        }

        espace
            ->rapporte_erreur(site,
                              "Je ne peux pas continuer la compilation car je n'arrive "
                              "pas à déterminer un type pour l'expression",
                              erreur::Genre::TYPE_INCONNU)
            .ajoute_message("Note : le type attendu est ")
            .ajoute_message(chaine_type(m_attente.type()))
            .ajoute_message("\n")
            .ajoute_message("Note : l'unité de compilation est dans cette état :\n")
            .ajoute_message(chaine_attentes_recursives(this))
            .ajoute_message("\n");
    }
    else if (m_attente.est<AttenteSurInterfaceKuri>()) {
        espace
            ->rapporte_erreur(noeud,
                              "Trop de cycles : arrêt de la compilation car une "
                              "déclaration attend sur une interface de Kuri")
            .ajoute_message(
                "Note : l'interface attendue est ", m_attente.interface_kuri()->nom, "\n");
    }
    else if (m_attente.est<AttenteSurOperateur>()) {
        auto operateur_attendu = m_attente.operateur();
        if (operateur_attendu->est_expression_binaire() || operateur_attendu->est_indexage()) {
            auto expression_operation = static_cast<NoeudExpressionBinaire *>(operateur_attendu);
            auto type1 = expression_operation->operande_gauche->type;
            auto type2 = expression_operation->operande_droite->type;

            auto candidats = kuri::tablet<OperateurCandidat, 10>();
            auto resultat = cherche_candidats_operateurs(
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
            auto expression_operation = static_cast<NoeudExpressionUnaire *>(operateur_attendu);
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
    else {
        espace
            ->rapporte_erreur(noeud,
                              "Je ne peux pas continuer la compilation car une unité est "
                              "bloqué dans un cycle")
            .ajoute_message("\nNote : l'unité est dans l'état : ")
            .ajoute_message(chaine_attentes_recursives(this))
            .ajoute_message("\n");
    }
}

kuri::chaine chaine_attentes_recursives(UniteCompilation const *unite)
{
    if (!unite) {
        return "    L'unité est nulle !\n";
    }

    Enchaineuse fc;

    auto attendue = unite->unite_attendue();
    auto commentaire = unite->commentaire();

    if (!attendue) {
        fc << "    " << commentaire << " est bloquée !\n";
    }

    kuri::ensemble<UniteCompilation const *> unite_visite;
    unite_visite.insere(unite);

    while (attendue) {
        if (attendue->est_prete()) {
            fc << "    " << commentaire << " est prête !\n";
            break;
        }

        if (unite_visite.possede(attendue)) {
            fc << "    erreur : dépendance cyclique !\n";
            break;
        }

        fc << "    " << commentaire << " attend sur ";
        commentaire = attendue->commentaire();
        fc << commentaire << '\n';

        unite_visite.insere(attendue);

        attendue = attendue->unite_attendue();
    }

    return fc.chaine();
}

static bool attente_est_résolue(EspaceDeTravail *espace, Attente &attente)
{
    if (attente.est<AttenteSurType>()) {
        return (attente.type()->drapeaux & TYPE_FUT_VALIDE) != 0;
    }

    if (attente.est<AttenteSurSymbole>()) {
        auto p = espace->phase_courante();
        // À FAIRE : granularise ceci pour ne pas tenter de recompiler quelque chose
        // si le symbole ne fut pas encore défini (par exemple en utilisant un ensemble de symboles
        // définis depuis le dernier ajournement, dans GestionnaireCode::cree_taches).
        return p < PhaseCompilation::PARSAGE_TERMINE;
    }

    if (attente.est<AttenteSurDeclaration>()) {
        auto declaration_attendue = attente.declaration();
        if (!declaration_attendue->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
            return false;
        }

        if (declaration_attendue ==
            espace->compilatrice().interface_kuri->decl_creation_contexte) {
            /* Pour crée_contexte, change l'attente pour attendre sur la RI corps car il
             * nous faut le code. */
            attente = Attente::sur_ri(&declaration_attendue->comme_entete_fonction()->atome);
            return false;
        }

        return true;
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
    if (attente.est<AttenteSurOperateur>()) {
        auto p = espace->phase_courante();
        return p < PhaseCompilation::PARSAGE_TERMINE;
    }

    if (attente.est<AttenteSurMetaProgramme>()) {
        auto metaprogramme_attendu = attente.metaprogramme();
        return metaprogramme_attendu->fut_execute;
    }

    if (attente.est<AttenteSurInterfaceKuri>()) {
        auto interface_attendue = attente.interface_kuri();
        auto &compilatrice = espace->compilatrice();

        if (ident_est_pour_fonction_interface(interface_attendue)) {
            auto decl = compilatrice.interface_kuri->declaration_pour_ident(interface_attendue);
            if (!decl || !decl->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                return false;
            }

            if (decl->ident == ID::cree_contexte) {
                /* Pour crée_contexte, change l'attente pour attendre sur la RI corps car il
                 * nous faut le code. */
                attente = Attente::sur_ri(&decl->atome);
                return false;
            }

            return true;
        }

        assert(ident_est_pour_type_interface(interface_attendue));
        return est_type_interface_disponible(compilatrice.typeuse, interface_attendue);
    }

    if (attente.est<AttenteSurMessage>()) {
        return false;
    }

    if (attente.est<AttenteSurChargement>()) {
        auto fichier_attendu = attente.fichier_a_charger();
        return fichier_attendu->fut_charge;
    }

    if (attente.est<AttenteSurLexage>()) {
        auto fichier_attendu = attente.fichier_a_lexer();
        return fichier_attendu->fut_lexe;
    }

    if (attente.est<AttenteSurParsage>()) {
        auto fichier_attendu = attente.fichier_a_parser();
        return fichier_attendu->fut_parse;
    }

    if (attente.est<AttenteSurRI>()) {
        auto ri_attendue = attente.ri();
        return (*ri_attendue && (*ri_attendue)->ri_generee);
    }

    if (attente.est<AttenteSurNoeudCode>()) {
        /* Géré dans le GestionnaireCode. */
        return false;
    }

    return true;
}

void UniteCompilation::marque_prete_si_attente_resolue()
{
    if (est_prete()) {
        return;
    }

    if (attente_est_résolue(espace, m_attente)) {
        marque_prete();
    }
}
