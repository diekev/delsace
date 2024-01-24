/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "analyse.hh"

#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"
#include "utilitaires/log.hh"

#include "structures/ensemble.hh"
#include "structures/file.hh"

#include "utilitaires/calcul.hh"

#include "bloc_basique.hh"
#include "impression.hh"
#include "instructions.hh"
#include "visite_instructions.hh"

/* ********************************************************************************************* */

/* Détecte le manque de retour. Toutes les fonctions, y compris celles ne retournant rien doivent
 * avoir une porte de sortie.
 *
 * L'algorithme essaye de suivre tous les chemins possibles dans la fonction afin de vérifier que
 * tous ont un retour défini.
 */
static bool détecte_retour_manquant(EspaceDeTravail &espace,
                                    FonctionEtBlocs const &fonction_et_blocs,
                                    VisiteuseBlocs &visiteuse)
{
    visiteuse.prépare_pour_nouvelle_traversée();

    while (Bloc *bloc_courant = visiteuse.bloc_suivant()) {
        if (!bloc_courant->instructions.est_vide()) {
            continue;
        }

        // À FAIRE : précise en quoi une instruction de retour manque.
        auto const atome = fonction_et_blocs.fonction;
        espace
            .rapporte_erreur(atome->decl,
                             "Alors que je traverse tous les chemins possibles à travers une "
                             "fonction, j'ai trouvé un chemin qui ne retourne pas de la fonction.")
            .ajoute_message("Erreur : instruction de retour manquante !");
        return false;
    }

    return true;
}

/* ********************************************************************************************* */

// Il reste des choses à faire pour activer ceci
#define ANALYSE_RI_PEUT_VERIFIER_VARIABLES_INUTILISEES

static auto incrémente_nombre_utilisations_récursif(Atome *racine) -> void
{
    racine->drapeaux |= DrapeauxAtome::EST_UTILISÉ;

    switch (racine->genre_atome) {
        case Atome::Genre::GLOBALE:
        case Atome::Genre::FONCTION:
        case Atome::Genre::CONSTANTE_NULLE:
        case Atome::Genre::CONSTANTE_TYPE:
        case Atome::Genre::CONSTANTE_RÉELLE:
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        case Atome::Genre::CONSTANTE_STRUCTURE:
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        case Atome::Genre::TRANSTYPE_CONSTANT:
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        case Atome::Genre::INITIALISATION_TABLEAU:
        case Atome::Genre::NON_INITIALISATION:
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        {
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = racine->comme_instruction();
            visite_opérandes_instruction(inst, [](Atome *atome_locale) {
                atome_locale->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
            });
            break;
        }
    }
}

static Atome const *déréférence_instruction(Instruction const *inst)
{
    if (inst->est_acces_index()) {
        auto acces = inst->comme_acces_index();
        return acces->accédé;
    }

    if (inst->est_acces_membre()) {
        auto acces = inst->comme_acces_membre();
        return acces->accédé;
    }

    // pour les déréférencements de pointeurs
    if (inst->est_charge()) {
        auto charge = inst->comme_charge();
        return charge->chargée;
    }

    if (inst->est_transtype()) {
        auto transtype = inst->comme_transtype();
        return transtype->valeur;
    }

    return inst;
}

static Atome const *cible_finale_stockage(InstructionStockeMem const *stocke)
{
    Atome const *destination = stocke->destination;

    while (!est_locale_ou_globale(destination)) {
        if (!destination->est_instruction()) {
            break;
        }

        auto inst = destination->comme_instruction();
        destination = déréférence_instruction(inst);
    }

    return destination;
}

/* Retourne vrai si un paramètre ou une globale fut utilisée lors de la production de l'atome. */
static bool paramètre_ou_globale_fut_utilisé(Atome *atome)
{
    auto résultat = false;
    visite_atome(atome, [&résultat](Atome const *visite) {
        /* À FAIRE(analyse_ri) : utiliser nombre_utilisations nous donne des faux-négatifs : une
         * variable non-utilisée peut être marquée comme utilisée si elle dépend d'un paramètre ou
         * d'une globale. */
        /* À FAIRE(analyse_ri) : le contexte implicite parasite également la détection d'une
         * expression non-utilisée. */
        if ((visite->possède_drapeau(DrapeauxAtome::EST_PARAMÈTRE_FONCTION)) ||
            visite->est_globale() || visite->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
            résultat = true;
        }
    });
    return résultat;
}

void marque_instructions_utilisées(kuri::tableau<Instruction *, int> &instructions)
{
    for (auto i = instructions.taille() - 1; i >= 0; --i) {
        auto it = instructions[i];

        if (it->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
            continue;
        }

        switch (it->genre) {
            case GenreInstruction::BRANCHE:
            case GenreInstruction::BRANCHE_CONDITION:
            case GenreInstruction::LABEL:
            case GenreInstruction::RETOUR:
            {
                incrémente_nombre_utilisations_récursif(it);
                break;
            }
            case GenreInstruction::APPEL:
            {
                auto appel = it->comme_appel();

                if (appel->type->est_type_rien()) {
                    incrémente_nombre_utilisations_récursif(it);
                }

                break;
            }
            case GenreInstruction::STOCKE_MEMOIRE:
            {
                auto stocke = it->comme_stocke_mem();
                auto cible = cible_finale_stockage(stocke);

                if ((cible->possède_drapeau(DrapeauxAtome::EST_PARAMÈTRE_FONCTION)) ||
                    cible->possède_drapeau(DrapeauxAtome::EST_UTILISÉ) || cible->est_globale()) {
                    incrémente_nombre_utilisations_récursif(stocke);
                }
                else {
                    /* Vérifie si l'instruction de stockage prend la valeur d'une globale ou d'un
                     * paramètre. */
                    if (paramètre_ou_globale_fut_utilisé(stocke->source)) {
                        incrémente_nombre_utilisations_récursif(stocke);
                    }
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

#ifdef ANALYSE_RI_PEUT_VERIFIER_VARIABLES_INUTILISEES

static bool est_atome_controllant_le_flux(Atome const *a)
{
    if (!a->est_instruction()) {
        return a->est_globale();
    }

    if (a->etat & EST_PARAMETRE_FONCTION) {
        return true;
    }

    auto const inst = a->comme_instruction();

    if (inst->est_retour() || inst->est_branche_cond()) {
        return true;
    }

    if (inst->est_appel()) {
        auto const appel = inst->comme_appel();

        if (appel->appele->est_fonction()) {
            auto const fonction = static_cast<AtomeFonction const *>(appel->appele);
            if (fonction->decl && fonction->decl->est_initialisation_type) {
                return false;
            }
        }
        return true;
    }

    if (inst->est_stocke_mem()) {
        auto const stockage = inst->comme_stocke_mem();

        auto resultat = false;
        visite_atome(stockage->ou, [&](Atome *courant) {
            if (courant->est_globale()) {
                resultat = true;
            }
            else if (courant->etat & EST_PARAMETRE_FONCTION) {
                resultat = true;
            }
        });
        return resultat;
    }

    return false;
}

struct Graphe {
  private:
    using connexion = std::pair<Atome *, Atome *>;

    kuri::tableau<connexion> connexions{};

  public:
    /* a est utilisé par b */
    void ajoute_connexion(Atome *a, Atome *b)
    {
        connexions.ajoute({a, b});
    }

    bool est_utilise(Atome *a) const
    {
        POUR (connexions) {
            if (it.first != a) {
                continue;
            }

            // std::cerr << "Atome " << a << " est utilisé par " << it.second << '\n';

            if (atome_est_utilise_dans_le_controle_de_flux(a, it.second)) {
                //                std::cerr << "Atome " << a->ident->nom << " est utilisé par "
                //                          <<
                //                          static_cast<int>(it.second->comme_instruction()->genre)
                //                          << "\n";
                return true;
            }
        }

        // std::cerr << "Atome " << a->ident->nom << " n'est pas utilisé !\n";
        return false;
    }

    bool atome_est_utilise_dans_le_controle_de_flux(Atome const *variable, Atome *a) const
    {
        if (est_atome_controllant_le_flux(a)) {
            return true;
        }

        a->nombre_utilisations = 1;

        /* Les atomes peuvent dépendre d'eux-mêmes (it = it + 1). */
        kuri::ensemblon<Atome *, 16> visites;

        kuri::file<Atome *> a_visiter;
        a_visiter.enfile(a);

        while (!a_visiter.est_vide()) {
            Atome *courant = a_visiter.defile();

            courant->nombre_utilisations = 1;

            if (visites.possede(courant)) {
                continue;
            }

            visites.insere(courant);

            POUR (connexions) {
                if (it.first != courant) {
                    continue;
                }

                if (est_atome_controllant_le_flux(it.second)) {
                    return true;
                }

                if (it.second->est_instruction()) {
                    auto inst = it.second->comme_instruction();

                    if (inst->est_acces_membre()) {
                        a_visiter.enfile(inst->comme_acces_membre()->accede);
                    }
                    else if (inst->est_acces_index()) {
                        a_visiter.enfile(inst->comme_acces_index()->accede);
                    }
                    //                    else if (inst->est_stocke_mem()) {
                    //                        if (variable->etat & EST_PARAMETRE_FONCTION) {
                    //                            return true;
                    //                        }
                    //                    }
                }

                it.second->nombre_utilisations = 1;
                a_visiter.enfile(it.second);
            }
        }

        return false;
    }
};

static kuri::tableau<InstructionAllocation *> allocations_a_verifier(AtomeFonction *atome)
{
    kuri::tableau<InstructionAllocation *> resultat;

    POUR (atome->params_entrees) {
        it->etat = EST_PARAMETRE_FONCTION;
        resultat.ajoute(it->comme_instruction()->comme_alloc());
    }

    POUR (atome->instructions) {
        if (!it->est_alloc()) {
            continue;
        }

        if (it->ident == nullptr) {
            continue;
        }

        /* Les variables d'indexion des boucles pour peuvent ne pas être utilisées. */
        if (it->ident == ID::it || it->ident == ID::index_it) {
            continue;
        }

        /* '_' est un peu spécial, il sers à définir une variable qui ne sera pas
         * utilisée, bien que ceci ne soit pas encore formalisé dans le langage. */
        if (it->ident->nom == "_") {
            continue;
        }

        resultat.ajoute(it->comme_alloc());
    }

    return resultat;
}

static NoeudDeclarationVariable *declaration_pour_allocation(InstructionAllocation *alloc)
{
    if (!alloc->site) {
        return nullptr;
    }

    if (alloc->site->est_declaration_variable()) {
        return alloc->site->comme_declaration_variable();
    }

    if (alloc->site->est_empl()) {
        auto variable = alloc->site->comme_empl()->expression;

        if (variable->est_declaration_variable()) {
            return variable->comme_declaration_variable();
        }
    }

    return nullptr;
}

static void imprime_declarations_inutilisees(
    EspaceDeTravail &espace,
    AtomeFonction *atome,
    kuri::tableau<InstructionAllocation *> const &allocs_inutilisees)
{
    POUR (allocs_inutilisees) {
        if (it->etat & EST_PARAMETRE_FONCTION) {
            espace.rapporte_avertissement(it->site, "Paramètre inutilisé");
        }
        else {
            espace.rapporte_avertissement(it->site, "Variable locale inutilisée");
        }
    }

    //    POUR (atome->instructions) {
    //        if (!it->est_appel() || it->nombre_utilisations != 0) {
    //            continue;
    //        }

    //        espace.rapporte_avertissement(it->site, "Retour de fonction inutilisé");
    //    }
}

static bool detecte_declarations_inutilisees_graphe(EspaceDeTravail &espace, AtomeFonction *atome)
{
    Graphe g;

    // Ne prend en compte :
    // - les stockages vers un paramètre d'entrée

    POUR (atome->instructions) {
        if (it->est_stocke_mem()) {
            auto const stockage = it->comme_stocke_mem();
            g.ajoute_connexion(stockage->valeur, stockage->ou);
            // g.ajoute_connexion(stockage->valeur, stockage);
            continue;
        }

        visite_atome(it, [&](Atome *atome_courant) {
            // std::cerr << "Atome " << atome_courant << " est utilisé par " << it << '\n';
            g.ajoute_connexion(atome_courant, it);
        });
    }

    kuri::tableau<InstructionAllocation *> allocs_inutilisees;
    auto allocs_a_verifier = allocations_a_verifier(atome);

    POUR (allocs_a_verifier) {
        if (g.est_utilise(it)) {
            continue;
        }

        auto decl_alloc = declaration_pour_allocation(it);
        if (decl_alloc && possede_annotation(decl_alloc, "inutilisée")) {
            continue;
        }

        allocs_inutilisees.ajoute(it);
    }

    // imprime_fonction(atome, std::cerr, false, true);

    imprime_declarations_inutilisees(espace, atome, allocs_inutilisees);

    return true;
}

static bool detecte_declarations_inutilisees_chargements(EspaceDeTravail &espace,
                                                         AtomeFonction *atome)
{
    POUR (atome->instructions) {
        if (it->est_stocke_mem()) {
            it->comme_stocke_mem()->ou->etat |= EST_STOCKE;
        }
        else if (it->est_charge()) {
            it->comme_charge()->chargee->etat |= EST_CHARGE;
        }
    }

    kuri::tableau<InstructionAllocation *> allocs_inutilisees;
    auto allocs_a_verifier = allocations_a_verifier(atome);

    POUR (allocs_a_verifier) {
        if (it->etat & EST_CHARGE) {
            continue;
        }

        if ((it->etat & (EST_STOCKE | EST_PARAMETRE_FONCTION)) ==
            (EST_STOCKE | EST_PARAMETRE_FONCTION)) {
            continue;
        }

        auto decl_alloc = declaration_pour_allocation(it);
        if (decl_alloc && possede_annotation(decl_alloc, "inutilisée")) {
            continue;
        }

        allocs_inutilisees.ajoute(it);
    }

    imprime_fonction(atome, std::cerr, false, true);

    imprime_declarations_inutilisees(espace, atome, allocs_inutilisees);

    return true;
}

static bool detecte_declarations_inutilisees_compte_utilisation(EspaceDeTravail &espace,
                                                                AtomeFonction *atome)
{
    POUR (atome->params_entrees) {
        it->drapeaux |= DrapeauxAtome::EST_PARAMÈTRE_FONCTION;
    }

    atome->param_sortie->drapeaux |= DrapeauxAtome::EST_PARAMÈTRE_FONCTION;

    POUR (atome->instructions) {
        if (!it->est_alloc()) {
            continue;
        }

        /* Les variables d'indexion des boucles pour peuvent ne pas être utilisées. */
        if (it->ident == ID::it || it->ident == ID::index_it) {
            it->nombre_utilisations += 1;
            continue;
        }

        /* '_' sers à définir une variable qui ne sera pas utilisée. */
        if (it->ident == ID::_) {
            it->nombre_utilisations += 1;
            continue;
        }
    }

    /* Deux passes pour prendre en compte les variables d'itérations des boucles. */
    marque_instructions_utilisées(atome->instructions);
    marque_instructions_utilisées(atome->instructions);

    kuri::tableau<InstructionAllocation *> allocs_inutilisees;

    POUR (atome->params_entrees) {
        if (it->nombre_utilisations != 0) {
            continue;
        }

        auto decl_alloc = it->site;

        /* Si le site n'est pas une déclaration de variable (le contexte implicite n'a pas de
         * site propre, celui de la fonction est utilisé), ajoutons-la à la liste des
         * allocations non-utilisées pour avoir un avertissement. */
        if (!decl_alloc || !decl_alloc->est_declaration_variable()) {
            allocs_inutilisees.ajoute(alloc);
            continue;
        }

        auto decl_var = decl_alloc->comme_declaration_variable();
        if (!possède_annotation(decl_var, "inutilisée")) {
            allocs_inutilisees.ajoute(it);
        }
    }

    POUR (atome->instructions) {
        if (!it->est_alloc() || it->nombre_utilisations != 0) {
            continue;
        }

        auto alloc = it->comme_alloc();
        if (alloc->ident == nullptr) {
            continue;
        }

        allocs_inutilisees.ajoute(alloc);
    }

#    if 0
    if (allocs_inutilisees.taille() != 0) {
        dbg() << imprime_fonction(atome, false, true);
    }
#    endif

    imprime_declarations_inutilisees(espace, atome, allocs_inutilisees);

    return true;
}

/**
 * Trouve les paramètres, variables locales, ou les retours d'appels de fonctions non-utilisés.
 *
 * À FAIRE(analyse_ri) :
 * - fonctions nichées inutilisées
 * - retours appels inutilisées
 * - les valeurs des itérations des boucles « pour » inutilisées dans le programme, sont marquées
 *   comme utilisées ici car elles le sont via l'incrémentation : il faudra un système plus subtil
 *   par exemple :
 *    pour i dans tabs {
 *      imprime("\t")
 *    }
 *   « i » est inutilisé, mais ne génère pas d'avertissement
 */
static bool detecte_declarations_inutilisees(EspaceDeTravail &espace, AtomeFonction *atome)
{
    auto const decl = atome->decl;

    /* Ignore les fonctions d'initalisation des types car les paramètres peuvent ne pas être
     * utilisés, par exemple pour la fonction d'initialisation du type « rien ». */
    if (decl && decl->est_initialisation_type) {
        return true;
    }

#    if 1
    //    if (!decl || !decl->possède_drapeau(DEBOGUE)) {
    //        return true;
    //    }

    detecte_declarations_inutilisees_graphe(espace, atome);
#    else
    detecte_declarations_inutilisees_compte_utilisation(espace, atome);
#    endif
    return true;
}
#endif

/* ******************************************************************************************** */

static bool atome_est_pour_création_contexte(Compilatrice const &compilatrice,
                                             AtomeFonction const *atome)
{
    auto interface = compilatrice.interface_kuri;

    /* atome->decl peut être nulle, vérifions d'abord que la fonction #création_contexte existe
     * déjà. */
    if (!interface->decl_creation_contexte) {
        return false;
    }

    return interface->decl_creation_contexte == atome->decl;
}

static bool détecte_blocs_invalides(EspaceDeTravail &espace,
                                    FonctionEtBlocs const &fonction_et_blocs)
{
    auto atome = fonction_et_blocs.fonction;

    POUR (fonction_et_blocs.blocs) {
        if (it->instructions.est_vide()) {
            auto site = it->label->site ? it->label->site : atome->decl;
            espace.rapporte_erreur(site, "Erreur interne : bloc vide dans la RI !\n");
            return false;
        }

        /* Nous pouvons avoir du code après un retour, par exemple le code après une discrimination
         * dont toutes les branches retournent de la fonction. */
        auto branche_ou_retour_rencontré = false;
        POUR_NOMME (inst, it->instructions) {
            if (inst->est_branche_ou_retourne()) {
                branche_ou_retour_rencontré = true;
                continue;
            }

            if (branche_ou_retour_rencontré && inst->site) {
                espace.rapporte_erreur(
                    inst->site, "Erreur interne : plusieurs branches ou retour dans un bloc.\n");
                return false;
            }
        }

        auto di = it->instructions.dernier_élément();
        if (di->est_branche_ou_retourne()) {
            continue;
        }

        /* La fonction #création_contexte n'a pas de retour, puisque ses instructions sont copiées
         * dans d'autres fonctions. */
        if (atome_est_pour_création_contexte(espace.compilatrice(), atome)) {
            continue;
        }

        espace.rapporte_erreur(
            di->site, "Erreur interne : un bloc ne finit pas par une branche ou un retour !\n");
        return false;
    }

    return true;
}

/* ******************************************************************************************** */

static void supprime_blocs_vides(FonctionEtBlocs &fonction_et_blocs, VisiteuseBlocs &visiteuse)
{
    auto bloc_modifié = false;

    POUR (fonction_et_blocs.blocs) {
        if (it->instructions.taille() != 1 || it->parents.taille() == 0) {
            continue;
        }

        auto di = it->instructions.dernier_élément();
        if (!di->est_branche()) {
            continue;
        }

        auto branche = di->comme_branche();

        for (auto parent : it->parents) {
            auto di_parent = parent->instructions.dernier_élément();

            if (di_parent->est_branche()) {
                di_parent->comme_branche()->label = branche->label;

                it->enfants[0]->remplace_parent(it, parent);
                parent->enlève_enfant(it);

                bloc_modifié = true;
            }
            else if (di_parent->est_branche_cond()) {
                auto branche_cond = di_parent->comme_branche_cond();
                if (branche_cond->label_si_vrai == it->label) {
                    branche_cond->label_si_vrai = branche->label;
                    it->enfants[0]->remplace_parent(it, parent);
                    parent->enlève_enfant(it);

                    bloc_modifié = true;
                }
                if (branche_cond->label_si_faux == it->label) {
                    branche_cond->label_si_faux = branche->label;
                    it->enfants[0]->remplace_parent(it, parent);
                    parent->enlève_enfant(it);

                    bloc_modifié = true;
                }
            }
        }
    }

    if (!bloc_modifié) {
        return;
    }

    fonction_et_blocs.supprime_blocs_inatteignables(visiteuse);
}

/* ******************************************************************************************** */

/**
 * Supprime les branches inconditionnelles d'un bloc à l'autre lorsque le bloc de la branche est le
 * seul ancêtre du bloc cible. Les instructions du bloc cible sont ajoutées au bloc ancêtre, et la
 * branche est supprimée.
 * Remplace les branches conditionnelles dont les cibles sont le même bloc par une branche
 * inconditionnelle.
 */
static void supprime_branches_inutiles(FonctionEtBlocs &fonction_et_blocs,
                                       ConstructriceRI &constructrice,
                                       VisiteuseBlocs &visiteuse)
{
    auto bloc_modifié = false;

    for (auto i = 0; i < fonction_et_blocs.blocs.taille(); ++i) {
        auto it = fonction_et_blocs.blocs[i];

        if (it->instructions.est_vide()) {
            /* Le bloc fut fusionné ici. */
            continue;
        }

        auto di = it->instructions.dernier_élément();

        if (di->est_branche_cond()) {
            auto branche = di->comme_branche_cond();
            if (branche->label_si_faux == branche->label_si_vrai) {
                /* Remplace par une branche. */
                auto nouvelle_branche = constructrice.crée_branche(
                    branche->site, branche->label_si_faux, true);
                it->instructions[it->instructions.taille() - 1] = nouvelle_branche;
                bloc_modifié = true;
                i -= 1;
                continue;
            }

            auto condition = branche->condition;
            if (est_valeur_constante(condition)) {
                auto valeur_constante = condition->comme_constante_booléenne();
                InstructionLabel *label_cible;
                if (valeur_constante->valeur) {
                    label_cible = branche->label_si_vrai;
                    it->enfants[1]->déconnecte_pour_branche_morte(it);
                }
                else {
                    label_cible = branche->label_si_faux;
                    it->enfants[0]->déconnecte_pour_branche_morte(it);
                }

                /* Remplace par une branche. */
                auto nouvelle_branche = constructrice.crée_branche(
                    branche->site, label_cible, true);
                it->instructions[it->instructions.taille() - 1] = nouvelle_branche;
                bloc_modifié = true;
                i -= 1;
                assert(it->enfants.taille() == 1);
            }

            continue;
        }

        if (!di->est_branche()) {
            continue;
        }

        auto bloc_enfant = it->enfants[0];
        if (bloc_enfant->parents.taille() != 1) {
            continue;
        }

        it->fusionne_enfant(bloc_enfant);
        bloc_enfant->instructions.efface();
        /* Regère ce bloc au cas où le nouvelle enfant serait également une branche. */
        if (it->instructions.dernier_élément()->est_branche()) {
            i -= 1;
        }
        bloc_modifié = true;
    }

    if (!bloc_modifié) {
        return;
    }

    fonction_et_blocs.supprime_blocs_inatteignables(visiteuse);
}

/* ******************************************************************************************** */

/* Les sources possibles de l'adresse d'un atome (à savoir, d'un pointeur). */
enum class SourceAdresseAtome : unsigned char {
    /* La source est inconnue, également utilisée comme valeur nulle. */
    INCONNUE,
    /* Nous avons l'adresse d'une globale. */
    GLOBALE,
    /* Nous avons une constante représentant une adresse. */
    CONSTANTE,
    /* Nous avons un paramètre d'entrée de la fonction. */
    PARAMÈTRE_ENTRÉE,
    /* Nous avons un paramètre de sortie de la fonction. */
    PARAMÈTRE_SORTIE,
    /* Nous avons l'adresse d'une locale. */
    LOCALE,
    /* Nous avons une adresse retourner par un appel de fonction. */
    VALEUR_RETOUR_FONCTION,
};

static std::ostream &operator<<(std::ostream &os, SourceAdresseAtome type)
{
    switch (type) {
        case SourceAdresseAtome::INCONNUE:
        {
            os << "INCONNUE";
            break;
        }
        case SourceAdresseAtome::GLOBALE:
        {
            os << "GLOBALE";
            break;
        }
        case SourceAdresseAtome::CONSTANTE:
        {
            os << "CONSTANTE";
            break;
        }
        case SourceAdresseAtome::PARAMÈTRE_ENTRÉE:
        {
            os << "PARAMETRE_ENTREE";
            break;
        }
        case SourceAdresseAtome::PARAMÈTRE_SORTIE:
        {
            os << "PARAMETRE_SORTIE";
            break;
        }
        case SourceAdresseAtome::LOCALE:
        {
            os << "LOCALE";
            break;
        }
        case SourceAdresseAtome::VALEUR_RETOUR_FONCTION:
        {
            os << "LOCALE";
            break;
        }
    }

    return os;
}

static inline bool est_stockage_adresse_valide(SourceAdresseAtome source,
                                               SourceAdresseAtome destination)
{
    if (source == SourceAdresseAtome::CONSTANTE) {
        return true;
    }

    if (source != SourceAdresseAtome::LOCALE) {
        return true;
    }

    return destination == SourceAdresseAtome::LOCALE ||
           destination == SourceAdresseAtome::CONSTANTE ||
           destination == SourceAdresseAtome::VALEUR_RETOUR_FONCTION;
}

static bool est_stockage_valide(InstructionStockeMem const &stockage,
                                SourceAdresseAtome source,
                                SourceAdresseAtome destination)
{
    auto const valeur = stockage.source;

    /* Nous ne sommes intéressés que par les stockage d'adresses. */
    if (!valeur->type->est_type_pointeur()) {
        return true;
    }

    if (est_stockage_adresse_valide(source, destination)) {
        return true;
    }

    return false;
}

static SourceAdresseAtome détermine_source_adresse_atome(
    AtomeFonction const &fonction,
    Atome const &atome,
    kuri::tableau<SourceAdresseAtome> const &sources)
{
    if (atome.est_globale() || atome.est_fonction()) {
        return SourceAdresseAtome::GLOBALE;
    }

    /* Pour « nul », mais également les arithmétiques de pointeurs, ou encore les pointeurs connus
     * lors de la compilation. */
    if (atome.est_constante_nulle() || atome.est_transtype_constant() ||
        atome.est_accès_index_constant() || atome.est_constante_structure()) {
        return SourceAdresseAtome::CONSTANTE;
    }

    POUR (fonction.params_entrée) {
        if (&atome == it) {
            return SourceAdresseAtome::PARAMÈTRE_ENTRÉE;
        }
    }

    if (&atome == fonction.param_sortie) {
        return SourceAdresseAtome::PARAMÈTRE_SORTIE;
    }

    if (atome.est_instruction()) {
        return sources[atome.comme_instruction()->numero];
    }

    return SourceAdresseAtome::LOCALE;
}

static void rapporte_erreur_stockage_invalide(
    EspaceDeTravail &espace,
    AtomeFonction const &fonction,
    InstructionStockeMem const &stockage,
    SourceAdresseAtome source,
    SourceAdresseAtome destination,
    kuri::tableau<SourceAdresseAtome> const &sources,
    kuri::tableau<SourceAdresseAtome> const &sources_pour_charge)
{
#undef IMPRIME_INFORMATIONS

#ifdef IMPRIME_INFORMATIONS
    Enchaineuse sortie;
    auto const valeur = stockage.valeur;
    sortie << "Stockage d'un pointeur de " << source << " vers " << destination << '\n';

    sortie << "--------- Instruction défaillante :\n";
    sortie << imprime_instruction(&stockage);
    sortie << "\n";
    sortie << "--------- Dernière valeur :\n";
    if (valeur->est_instruction()) {
        sortie << imprime_instruction(valeur->comme_instruction());
    }
    else {
        sortie << imprime_atome(valeur);
    }
    sortie << "\n";
    sortie << "--------- Fonction défaillante :\n";
    imprime_fonction(
        &fonction, sortie, false, false, [&](const Instruction &instruction, Enchaineuse &os) {
            os << " " << sources[instruction.numero]
               << " (si charge : " << sources_pour_charge[instruction.numero] << ")";
        });
    sortie << "\n";
    dbg() << sortie.chaine();
#else
    static_cast<void>(fonction);
    static_cast<void>(source);
    static_cast<void>(sources);
    static_cast<void>(sources_pour_charge);
#endif

    if (destination == SourceAdresseAtome::PARAMÈTRE_SORTIE) {
        espace.rapporte_erreur(stockage.site, "Retour d'une adresse locale.");
    }
    else if (destination == SourceAdresseAtome::GLOBALE) {
        espace.rapporte_erreur(stockage.site, "Stockage d'une adresse locale dans une globale.");
    }
    else {
        espace.rapporte_erreur(stockage.site, "Stockage d'une adresse locale.");
    }
}

/* Essaie de détecter si nous retournons une adresse locale, qui sera invalide après le retour de
 * la fonction.
 * À FAIRE : tests
 * - adresses dans des tableaux ou structures mixtes étant retournés
 */
static bool détecte_utilisations_adresses_locales(EspaceDeTravail &espace,
                                                  AtomeFonction const &fonction)
{
    /* La fonction de création de contexte prend des adresses locales, mais elle n'est pas une
     * vraie fonction. */
    if (fonction.decl &&
        (fonction.decl->ident == ID::crée_contexte ||
         fonction.decl->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME))) {
        return true;
    }

    auto const taille_sources = fonction.numérote_instructions();

    /* Pour chaque instruction, stocke la source de l'adresse. */
    kuri::tableau<SourceAdresseAtome> sources(taille_sources);
    POUR (sources) {
        it = SourceAdresseAtome::INCONNUE;
    }

    /* Afin de différencier l'adresse d'une variable de celle de son contenu, ceci stocke pour
     * chaque instruction la source de l'adresse pointée par la variable. */
    kuri::tableau<SourceAdresseAtome> sources_pour_charge = sources;

    for (auto i = 0; i < fonction.params_entrée.taille(); i++) {
        sources[i] = SourceAdresseAtome::PARAMÈTRE_ENTRÉE;
        sources_pour_charge[i] = SourceAdresseAtome::PARAMÈTRE_ENTRÉE;
    }

    int index = fonction.params_entrée.taille();
    sources[index] = SourceAdresseAtome::PARAMÈTRE_SORTIE;
    sources_pour_charge[index] = SourceAdresseAtome::PARAMÈTRE_SORTIE;

    POUR (fonction.instructions) {
        if (it->est_alloc()) {
            sources[it->numero] = SourceAdresseAtome::LOCALE;
            continue;
        }

        /* INCOMPLET : il serait bien de savoir ce qui est retourné : l'adresse d'une globale, d'un
         * paramètre, etc. */
        if (it->est_appel()) {
            sources[it->numero] = SourceAdresseAtome::VALEUR_RETOUR_FONCTION;
            sources_pour_charge[it->numero] = SourceAdresseAtome::VALEUR_RETOUR_FONCTION;
            continue;
        }

        if (it->est_charge()) {
            if (it->comme_charge()->chargée->est_instruction()) {
                auto inst = it->comme_charge()->chargée->comme_instruction();
                sources[it->numero] = sources_pour_charge[inst->numero];
            }
            else {
                sources[it->numero] = détermine_source_adresse_atome(
                    fonction, *it->comme_charge()->chargée, sources);
            }

            continue;
        }

        if (it->est_transtype()) {
            sources[it->numero] = détermine_source_adresse_atome(
                fonction, *it->comme_transtype()->valeur, sources);
            continue;
        }

        if (it->est_acces_membre()) {
            auto accede = it->comme_acces_membre()->accédé;
            sources[it->numero] = détermine_source_adresse_atome(fonction, *accede, sources);
            if (accede->est_instruction()) {
                sources_pour_charge[it->numero] =
                    sources_pour_charge[accede->comme_instruction()->numero];
            }
            continue;
        }

        if (it->est_acces_index()) {
            auto accede = it->comme_acces_index()->accédé;
            sources[it->numero] = détermine_source_adresse_atome(fonction, *accede, sources);
            if (accede->est_instruction()) {
                sources_pour_charge[it->numero] =
                    sources_pour_charge[accede->comme_instruction()->numero];
            }
            continue;
        }

        if (it->est_stocke_mem()) {
            auto const stockage = it->comme_stocke_mem();
            auto const ou = stockage->destination;
            auto const valeur = stockage->source;

            auto const source_adresse_destination = détermine_source_adresse_atome(
                fonction, *ou, sources);
            auto const source_adresse_source = détermine_source_adresse_atome(
                fonction, *valeur, sources);

            if (!est_stockage_valide(
                    *stockage, source_adresse_source, source_adresse_destination)) {
                rapporte_erreur_stockage_invalide(espace,
                                                  fonction,
                                                  *stockage,
                                                  source_adresse_source,
                                                  source_adresse_destination,
                                                  sources,
                                                  sources_pour_charge);
                return false;
            }

            if (ou->est_instruction()) {
                sources_pour_charge[ou->comme_instruction()->numero] = source_adresse_source;
            }

            continue;
        }
    }

    return true;
}

/* ------------------------------------------------------------------------- */
/** \name Diagnostique pour les opérateurs binaires.
 * \{ */

static bool est_opérateur_comparaison_ordre(OpérateurBinaire::Genre genre)
{
    return genre == OpérateurBinaire::Genre::Comp_Inf ||
           genre == OpérateurBinaire::Genre::Comp_Inf_Egal ||
           genre == OpérateurBinaire::Genre::Comp_Sup ||
           genre == OpérateurBinaire::Genre::Comp_Sup_Egal;
}

static bool est_comparaison_pointeur_nul(InstructionOpBinaire const *op_binaire)
{
    auto const genre = op_binaire->op;
    if (!est_opérateur_comparaison_ordre(genre)) {
        return false;
    }

    /* À FAIRE : quand les instructions seront canonicalisées, supprime la version dépréciée. */
    auto const opérande_droite = op_binaire->valeur_droite;
    auto const opérande_gauche = op_binaire->valeur_gauche;

    return (est_constante_pointeur_nul(opérande_droite) &&
            opérande_gauche->type->est_type_pointeur()) ||
           (est_constante_pointeur_nul(opérande_gauche) &&
            opérande_droite->type->est_type_pointeur());
}

static bool est_comparaison_ordonnée_naturel_zéro(InstructionOpBinaire const *op_binaire)
{
    auto const genre = op_binaire->op;
    auto const opérande_droite = op_binaire->valeur_droite;
    auto const opérande_gauche = op_binaire->valeur_gauche;

    /* À FAIRE : canonicalisation. */

    /* Détecte 0 <= naturel. */
    if (genre == OpérateurBinaire::Genre::Comp_Inf_Egal_Nat ||
        genre == OpérateurBinaire::Genre::Comp_Inf_Nat) {
        return est_constante_entière_zéro(opérande_gauche) &&
               opérande_droite->type->est_type_entier_naturel();
    }

    /* Détecte naturel >= 0. */
    if (genre == OpérateurBinaire::Genre::Comp_Sup_Egal_Nat ||
        genre == OpérateurBinaire::Genre::Comp_Sup_Nat) {
        return est_constante_entière_zéro(opérande_droite) &&
               opérande_gauche->type->est_type_entier_naturel();
    }

    return false;
}

static bool est_comparaison_adresse_fonction(InstructionOpBinaire const *op_binaire)
{
    auto gauche = op_binaire->valeur_gauche;
    auto droite = op_binaire->valeur_droite;

    /* À FAIRE : canonicalisation. */
    if ((gauche->est_fonction() && est_constante_pointeur_nul(droite)) ||
        (droite->est_fonction() && est_constante_pointeur_nul(gauche))) {
        return true;
    }

    return false;
}

static bool détecte_opérateurs_binaires_suspicieux(EspaceDeTravail &espace,
                                                   FonctionEtBlocs const &fonction_et_blocs)
{
    POUR_NOMME (bloc, fonction_et_blocs.blocs) {
        if (!bloc->possède_instruction_de_genre(GenreInstruction::OPERATION_BINAIRE)) {
            continue;
        }

        POUR (bloc->instructions) {
            if (!it->est_op_binaire()) {
                continue;
            }

            if (est_comparaison_adresse_fonction(it->comme_op_binaire())) {
                espace.rapporte_erreur(it->site,
                                       "Comparaison d'une adresse de fonction avec nul. La "
                                       "comparaison est toujours vrai "
                                       "et peut-être n'est pas ce que vous vouliez.");
                return false;
            }

            if (est_comparaison_pointeur_nul(it->comme_op_binaire())) {
                espace.rapporte_erreur(it->site,
                                       "Comparaison ordonnée d'un pointeur avec nul; veuillez "
                                       "utiliser une comparaison d'égalité.");
                return false;
            }

            if (est_comparaison_ordonnée_naturel_zéro(it->comme_op_binaire())) {
                espace.rapporte_avertissement(
                    it->site,
                    "La comparaison ordonnée d'un entier naturel avec zéro est toujours vrai.");
                continue;
            }
        }
    }

    return true;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Pureté d'une fonction.
 * \{ */

#if 0
/* Pour référence https://en.wikipedia.org/wiki/Pure_function */
static bool fonction_est_pure(AtomeFonction const *fonction)
{
    if (fonction->est_externe) {
        /* À FAIRE : note les fonctions externes pures comme tel. */
        return false;
    }

    POUR (fonction->params_entrees) {
        it->drapeaux |= DrapeauxAtome::EST_PARAMÈTRE_FONCTION;
    }

    POUR (fonction->instructions) {
        if (it->est_appel()) {
            /* À FAIRE : test si la fonction est pur. */
            return false;
        }

        if (it->est_op_binaire()) {
            auto op_binaire = it->comme_op_binaire();
            /* Le mode d'arrondissement des nombres réels peut être modifié lors de l'exécution.
             * Ainsi, tous les opérations binaires sont impures. */
            if (op_binaire->valeur_droite->type->est_type_reel()) {
                return false;
            }
        }

        if (it->est_charge()) {
            auto charge = it->comme_charge();
            /* À FAIRE : uniquement si la valeur globale est écrite dans la sortie. */
            if (charge->chargée->est_globale()) {
                return false;
            }
        }

        if (it->est_stocke_mem()) {
            auto stocke = it->comme_stocke_mem();
            auto cible = cible_finale_stockage(stocke);
            /* À FAIRE : alias de la mémoire. */
            if (cible->est_globale() || (cible->etat == EST_PARAMETRE_FONCTION)) {
                /* Modification d'une globale ou d'un paramètre. */
                return false;
            }
        }
    }

    return true;
}
#endif

/** \} */

/* Puisque les init_de peuvent être partagées, et alors requierent un transtypage, cette fonction
 * retourne le décalage + 1 à utiliser si la fonction est une fonction d'initialisation.
 * Retourne :
 *    0 si la fonction n'est pas une initialisation potentielle
 *    1 si initialisation potentielle (décalage effectif de 0)
 *    2 si initialisation potentielle avec un transtypage (décalage effectif de 1)
 */
static int est_appel_initialisation(Instruction const *inst0, Instruction const *inst1)
{
    if (!inst0->est_appel()) {
        return 0;
    }

    /* Ne vérifions pas que l'appelée est une initialisation de type, ce pourrait être déguisé via
     * un pointeur de fonction. */
    auto appel = inst0->comme_appel();
    if (appel->args.taille() != 1) {
        return 0;
    }

    auto arg = appel->args[0];
    if (arg == inst1) {
        return 1;
    }

    if (arg->est_instruction()) {
        if (est_transtypage_de(arg->comme_instruction(), inst1)) {
            return 2;
        }
    }

    return 0;
}

#define ASSIGNE_SI_EGAUX(a, b, c)                                                                 \
    if (a == b) {                                                                                 \
        a = c;                                                                                    \
    }

static bool remplace_instruction_par_atome(Atome *utilisateur,
                                           Instruction const *à_remplacer,
                                           Atome *nouvelle_valeur,
                                           bool *branche_conditionnelle_fut_changée)
{
    if (!utilisateur->est_instruction()) {
        return false;
    }

    auto utilisatrice = utilisateur->comme_instruction();

    if (utilisatrice->est_appel()) {
        return false;
    }

    if (utilisatrice->est_stocke_mem()) {
        auto stockage = utilisatrice->comme_stocke_mem();
        ASSIGNE_SI_EGAUX(stockage->destination, à_remplacer, nouvelle_valeur)
        ASSIGNE_SI_EGAUX(stockage->source, à_remplacer, nouvelle_valeur)
    }
    else if (utilisatrice->est_op_binaire()) {
        auto op_binaire = utilisatrice->comme_op_binaire();
        ASSIGNE_SI_EGAUX(op_binaire->valeur_droite, à_remplacer, nouvelle_valeur)
        ASSIGNE_SI_EGAUX(op_binaire->valeur_gauche, à_remplacer, nouvelle_valeur)
    }
    else if (utilisatrice->est_op_unaire()) {
        auto op_unaire = utilisatrice->comme_op_unaire();
        ASSIGNE_SI_EGAUX(op_unaire->valeur, à_remplacer, nouvelle_valeur)
    }
    else if (utilisatrice->est_branche_cond()) {
        auto branche_cond = utilisatrice->comme_branche_cond();
        ASSIGNE_SI_EGAUX(branche_cond->condition, à_remplacer, nouvelle_valeur)
        if (branche_conditionnelle_fut_changée) {
            *branche_conditionnelle_fut_changée = true;
        }
    }
    else if (utilisatrice->est_transtype()) {
        auto transtype = utilisatrice->comme_transtype();
        ASSIGNE_SI_EGAUX(transtype->valeur, à_remplacer, nouvelle_valeur)
    }
    else if (utilisatrice->est_acces_membre()) {
        auto membre = utilisatrice->comme_acces_membre();
        ASSIGNE_SI_EGAUX(membre->accédé, à_remplacer, nouvelle_valeur)
    }
    else if (utilisatrice->est_acces_index()) {
        auto index = utilisatrice->comme_acces_index();
        ASSIGNE_SI_EGAUX(index->accédé, à_remplacer, nouvelle_valeur)
        ASSIGNE_SI_EGAUX(index->index, à_remplacer, nouvelle_valeur)
    }
    else {
        return false;
    }

    return true;
}

static bool supprime_allocations_temporaires(Graphe const &g, Bloc *bloc)
{
    for (int i = 0; i < bloc->instructions.taille() - 3; i++) {
        auto inst0 = bloc->instructions[i + 0];
        auto inst1 = bloc->instructions[i + 1];
        auto inst2 = bloc->instructions[i + 2];

        if (!inst0->est_alloc()) {
            continue;
        }

        if (!est_stockage_vers(inst1, inst0)) {
            continue;
        }

        if (!est_chargement_de(inst2, inst0)) {
            continue;
        }

        /* Si l'allocation n'est pas uniquement dans ce bloc, ce n'est pas une temporaire. */
        if (!g.est_uniquement_utilisé_dans_bloc(inst0, bloc->donne_id())) {
            continue;
        }

        /* Si le chargement n'est pas uniquement dans ce bloc, ce n'est pas une temporaire.
         * Ceci survient notamment dans la génération de code pour les vérifications des bornes des
         * tableaux ou chaines. */
        if (!g.est_uniquement_utilisé_dans_bloc(inst2, bloc->donne_id())) {
            continue;
        }

        auto est_utilisee_uniquement_pour_charge_et_stocke = true;
        g.visite_utilisateurs(inst0, [&](Atome const *utilisateur) {
            if (utilisateur != inst1 && utilisateur != inst2) {
                est_utilisee_uniquement_pour_charge_et_stocke = false;
            }
        });

        if (!est_utilisee_uniquement_pour_charge_et_stocke) {
            continue;
        }

        /* Si le chargement est réutilisé, nous n'avons sans doute pas une temporaire. */
        if (g.nombre_d_utilisateurs(inst2) != 1) {
            continue;
        }

        g.visite_utilisateurs(inst2, [&](Atome *utilisateur) {
            auto nouvelle_valeur = inst1->comme_stocke_mem()->source;
            if (!remplace_instruction_par_atome(utilisateur, inst2, nouvelle_valeur, nullptr)) {
                return;
            }

            bloc->tag_instruction_à_supprimer(inst0);
            bloc->tag_instruction_à_supprimer(inst1);
            bloc->tag_instruction_à_supprimer(inst2);
        });
    }

    return bloc->supprime_instructions_à_supprimer();
}

static std::optional<int> trouve_stockage_dans_bloc(Bloc const *bloc,
                                                    Instruction const *alloc,
                                                    int début_recherche)
{
    for (int i = début_recherche; i < bloc->instructions.taille() - 1; i++) {
        auto decalage = est_appel_initialisation(bloc->instructions[i], alloc);
        if (decalage != 0) {
            return i - (decalage - 1);
        }

        if (est_stockage_vers(bloc->instructions[i], alloc)) {
            return i;
        }
    }

    return {};
}

static bool rapproche_allocations_des_stockages(Bloc *bloc)
{
    auto bloc_modifié = false;
    for (int i = 0; i < bloc->instructions.taille() - 2; i++) {
        auto inst_i = bloc->instructions[i];

        if (!inst_i->est_alloc()) {
            continue;
        }

        auto const index_stockage = trouve_stockage_dans_bloc(bloc, inst_i, i + 1);
        if (!index_stockage.has_value()) {
            continue;
        }

        std::rotate(&bloc->instructions[i],
                    &bloc->instructions[i + 1],
                    &bloc->instructions[index_stockage.value()]);

        bloc_modifié = true;
    }

    return bloc_modifié;
}

static void valide_fonction(EspaceDeTravail &espace, AtomeFonction const &fonction)
{
    POUR (fonction.instructions) {
        visite_opérandes_instruction(it, [&](Atome *atome_courant) {
            if (!atome_courant->est_instruction()) {
                return;
            }

            auto inst = atome_courant->comme_instruction();

            if (inst->possède_drapeau(DrapeauxAtome::EST_À_SUPPRIMER)) {
                dbg() << "La fonction est " << nom_humainement_lisible(fonction.decl) << '\n'
                      << *fonction.decl << '\n'
                      << "L'instruction supprimée est " << imprime_instruction(inst) << "\n"
                      << "L'utilisateur est " << imprime_instruction(it);
                espace.rapporte_erreur(
                    inst->site,
                    "Erreur interne : la fonction référence une instruction supprimée");
            }
        });
    }
}

static void supprime_allocations_temporaires(FonctionEtBlocs &fonction_et_blocs)
{
    auto &graphe = fonction_et_blocs.donne_graphe_ajourné();

    auto bloc_modifié = false;
    POUR (fonction_et_blocs.blocs) {
        if (!it->possède_instruction_de_genre(GenreInstruction::ALLOCATION)) {
            continue;
        }

        bloc_modifié |= rapproche_allocations_des_stockages(it);
        bloc_modifié |= supprime_allocations_temporaires(graphe, it);
    }

    if (!bloc_modifié) {
        return;
    }

    fonction_et_blocs.marque_blocs_modifiés();
}

/* ***************************************************************************************** */

struct Calculatrice {
    template <typename Opération>
    static uint64_t applique_opération_entier(AtomeConstanteEntière const *opérande_gauche,
                                              AtomeConstanteEntière const *opérande_droite)
    {
        auto const type = opérande_gauche->type;
        if (type->est_type_entier_naturel()) {
            if (type->taille_octet == 1) {
                return applique_opération_entier_ex<Opération, uint8_t>(opérande_gauche->valeur,
                                                                        opérande_droite->valeur);
            }
            if (type->taille_octet == 2) {
                return applique_opération_entier_ex<Opération, uint16_t>(opérande_gauche->valeur,
                                                                         opérande_droite->valeur);
            }
            if (type->taille_octet == 4) {
                return applique_opération_entier_ex<Opération, uint32_t>(opérande_gauche->valeur,
                                                                         opérande_droite->valeur);
            }
            return applique_opération_entier_ex<Opération, uint64_t>(opérande_gauche->valeur,
                                                                     opérande_droite->valeur);
        }
        if (type->taille_octet == 1) {
            auto résultat = applique_opération_entier_ex<Opération, int8_t>(
                opérande_gauche->valeur, opérande_droite->valeur);
            return uint8_t(résultat);
        }
        if (type->taille_octet == 2) {
            auto résultat = applique_opération_entier_ex<Opération, int16_t>(
                opérande_gauche->valeur, opérande_droite->valeur);
            return uint16_t(résultat);
        }
        if (type->taille_octet == 4 || type->est_type_entier_constant()) {
            auto résultat = applique_opération_entier_ex<Opération, int32_t>(
                opérande_gauche->valeur, opérande_droite->valeur);
            return uint32_t(résultat);
        }
        auto résultat = applique_opération_entier_ex<Opération, int64_t>(opérande_gauche->valeur,
                                                                         opérande_droite->valeur);
        return uint64_t(résultat);
    }

    template <typename Opération>
    static double applique_opération_réel(AtomeConstanteRéelle const *opérande_gauche,
                                          AtomeConstanteRéelle const *opérande_droite)
    {
        assert(opérande_gauche->type == opérande_droite->type);

        auto const type = opérande_gauche->type;
        if (type->taille_octet == 2) {
            /* À FAIRE(r16). */
            return 0.0;
        }
        if (type->taille_octet == 4 || type->est_type_entier_constant()) {
            auto résultat = applique_opération_réel_ex<Opération, float>(opérande_gauche->valeur,
                                                                         opérande_droite->valeur);
            return double(résultat);
        }
        auto résultat = applique_opération_réel_ex<Opération, double>(opérande_gauche->valeur,
                                                                      opérande_droite->valeur);
        return résultat;
    }

    template <typename Opération>
    static bool applique_comparaison_entier(AtomeConstanteEntière const *opérande_gauche,
                                            AtomeConstanteEntière const *opérande_droite)
    {
        assert(opérande_gauche->type == opérande_droite->type);

        auto const type = opérande_gauche->type;
        if (type->est_type_entier_naturel()) {
            if (type->taille_octet == 1) {
                return applique_comparaison_entier_ex<Opération, uint8_t>(opérande_gauche->valeur,
                                                                          opérande_droite->valeur);
            }
            if (type->taille_octet == 2) {
                return applique_comparaison_entier_ex<Opération, uint16_t>(
                    opérande_gauche->valeur, opérande_droite->valeur);
            }
            if (type->taille_octet == 4) {
                return applique_comparaison_entier_ex<Opération, uint32_t>(
                    opérande_gauche->valeur, opérande_droite->valeur);
            }
            return applique_comparaison_entier_ex<Opération, uint64_t>(opérande_gauche->valeur,
                                                                       opérande_droite->valeur);
        }
        if (type->taille_octet == 1) {
            return applique_comparaison_entier_ex<Opération, int8_t>(opérande_gauche->valeur,
                                                                     opérande_droite->valeur);
        }
        if (type->taille_octet == 2) {
            return applique_comparaison_entier_ex<Opération, int16_t>(opérande_gauche->valeur,
                                                                      opérande_droite->valeur);
        }
        if (type->taille_octet == 4 || type->est_type_entier_constant()) {
            return applique_comparaison_entier_ex<Opération, int32_t>(opérande_gauche->valeur,
                                                                      opérande_droite->valeur);
        }
        return applique_comparaison_entier_ex<Opération, int64_t>(opérande_gauche->valeur,
                                                                  opérande_droite->valeur);
    }

    template <typename Opération>
    static bool applique_comparaison_réel(AtomeConstanteRéelle const *opérande_gauche,
                                          AtomeConstanteRéelle const *opérande_droite)
    {
        assert(opérande_gauche->type == opérande_droite->type);

        auto const type = opérande_gauche->type;
        if (type->taille_octet == 2) {
            /* À FAIRE(r16). */
            return 0.0;
        }
        if (type->taille_octet == 4 || type->est_type_entier_constant()) {
            return applique_comparaison_réel_ex<Opération, float>(opérande_gauche->valeur,
                                                                  opérande_droite->valeur);
        }
        return applique_comparaison_réel_ex<Opération, double>(opérande_gauche->valeur,
                                                               opérande_droite->valeur);
    }

  private:
    template <typename Opération, typename T>
    static T applique_opération_entier_ex(uint64_t gauche, uint64_t droite)
    {
        return Opération::applique_opération(static_cast<T>(gauche), static_cast<T>(droite));
    }

    template <typename Opération, typename T>
    static T applique_opération_réel_ex(double gauche, double droite)
    {
        return Opération::applique_opération(static_cast<T>(gauche), static_cast<T>(droite));
    }

    template <typename Opération, typename T>
    static bool applique_comparaison_entier_ex(uint64_t gauche, uint64_t droite)
    {
        return Opération::applique_opération(static_cast<T>(gauche), static_cast<T>(droite));
    }

    template <typename Opération, typename T>
    static bool applique_comparaison_réel_ex(double gauche, double droite)
    {
        return Opération::applique_opération(static_cast<T>(gauche), static_cast<T>(droite));
    }
};

AtomeConstante *évalue_opérateur_binaire(InstructionOpBinaire const *inst,
                                         ConstructriceRI &constructrice)
{
    auto const opérande_gauche = inst->valeur_gauche;
    auto const opérande_droite = inst->valeur_droite;

#define APPLIQUE_OPERATION_ENTIER(nom)                                                            \
    auto résultat = Calculatrice::applique_opération_entier<nom>(                                 \
        opérande_gauche->comme_constante_entière(), opérande_droite->comme_constante_entière());  \
    return constructrice.crée_constante_nombre_entier(inst->type, résultat)

#define APPLIQUE_OPERATION_REEL(nom)                                                              \
    auto résultat = Calculatrice::applique_opération_réel<nom>(                                   \
        opérande_gauche->comme_constante_réelle(), opérande_droite->comme_constante_réelle());    \
    return constructrice.crée_constante_nombre_réel(inst->type, résultat)

#define APPLIQUE_COMPARAISON_ENTIER(nom)                                                          \
    auto résultat = Calculatrice::applique_comparaison_entier<nom>(                               \
        opérande_gauche->comme_constante_entière(), opérande_droite->comme_constante_entière());  \
    return constructrice.crée_constante_booléenne(résultat)

#define APPLIQUE_COMPARAISON_REEL(nom)                                                            \
    auto résultat = Calculatrice::applique_comparaison_réel<nom>(                                 \
        opérande_gauche->comme_constante_réelle(), opérande_droite->comme_constante_réelle());    \
    return constructrice.crée_constante_booléenne(résultat)

    switch (inst->op) {
        case OpérateurBinaire::Genre::Addition:
        {
            APPLIQUE_OPERATION_ENTIER(Addition);
        }
        case OpérateurBinaire::Genre::Addition_Reel:
        {
            APPLIQUE_OPERATION_REEL(Addition);
        }
        case OpérateurBinaire::Genre::Soustraction:
        {
            APPLIQUE_OPERATION_ENTIER(Soustraction);
        }
        case OpérateurBinaire::Genre::Soustraction_Reel:
        {
            APPLIQUE_OPERATION_REEL(Soustraction);
        }
        case OpérateurBinaire::Genre::Multiplication:
        {
            APPLIQUE_OPERATION_ENTIER(Multiplication);
        }
        case OpérateurBinaire::Genre::Multiplication_Reel:
        {
            APPLIQUE_OPERATION_REEL(Multiplication);
        }
        case OpérateurBinaire::Genre::Division_Naturel:
        case OpérateurBinaire::Genre::Division_Relatif:
        {
            APPLIQUE_OPERATION_ENTIER(Division);
        }
        case OpérateurBinaire::Genre::Division_Reel:
        {
            APPLIQUE_OPERATION_REEL(Division);
        }
        case OpérateurBinaire::Genre::Reste_Naturel:
        case OpérateurBinaire::Genre::Reste_Relatif:
        {
            APPLIQUE_OPERATION_ENTIER(Modulo);
        }
        case OpérateurBinaire::Genre::Comp_Egal:
        {
            APPLIQUE_COMPARAISON_ENTIER(Égal);
        }
        case OpérateurBinaire::Genre::Comp_Egal_Reel:
        {
            APPLIQUE_COMPARAISON_REEL(Égal);
        }
        case OpérateurBinaire::Genre::Comp_Inegal:
        {
            APPLIQUE_COMPARAISON_ENTIER(Différent);
        }
        case OpérateurBinaire::Genre::Comp_Inegal_Reel:
        {
            APPLIQUE_COMPARAISON_REEL(Différent);
        }
        case OpérateurBinaire::Genre::Comp_Inf:
        case OpérateurBinaire::Genre::Comp_Inf_Nat:
        {
            APPLIQUE_COMPARAISON_ENTIER(Inférieur);
        }
        case OpérateurBinaire::Genre::Comp_Inf_Reel:
        {
            APPLIQUE_COMPARAISON_REEL(Inférieur);
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal:
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
        {
            APPLIQUE_COMPARAISON_ENTIER(InférieurÉgal);
        }
        case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
        {
            APPLIQUE_COMPARAISON_REEL(InférieurÉgal);
        }
        case OpérateurBinaire::Genre::Comp_Sup:
        case OpérateurBinaire::Genre::Comp_Sup_Nat:
        {
            APPLIQUE_COMPARAISON_ENTIER(Supérieur);
        }
        case OpérateurBinaire::Genre::Comp_Sup_Reel:
        {
            APPLIQUE_COMPARAISON_REEL(Supérieur);
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal:
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
        {
            APPLIQUE_COMPARAISON_ENTIER(SupérieurÉgal);
        }
        case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
        {
            APPLIQUE_COMPARAISON_REEL(SupérieurÉgal);
        }
        case OpérateurBinaire::Genre::Et_Binaire:
        {
            APPLIQUE_OPERATION_ENTIER(ConjonctionBinaire);
        }
        case OpérateurBinaire::Genre::Ou_Binaire:
        {
            APPLIQUE_OPERATION_ENTIER(DisjonctionBinaire);
        }
        case OpérateurBinaire::Genre::Ou_Exclusif:
        {
            APPLIQUE_OPERATION_ENTIER(DisjonctionBinaireExclusive);
        }
        case OpérateurBinaire::Genre::Dec_Gauche:
        {
            APPLIQUE_OPERATION_ENTIER(DécalageGauche);
        }
        case OpérateurBinaire::Genre::Dec_Droite_Arithm:
        case OpérateurBinaire::Genre::Dec_Droite_Logique:
        {
            APPLIQUE_OPERATION_ENTIER(DécalageDroite);
        }
        case OpérateurBinaire::Genre::Indexage:
        case OpérateurBinaire::Genre::Invalide:
        {
            break;
        }
    }

    return nullptr;
}

static bool supprime_op_binaires_constants(Bloc *bloc,
                                           Graphe &graphe,
                                           ConstructriceRI &constructrice,
                                           bool *branche_conditionnelle_fut_changée)
{
    if (!bloc->possède_instruction_de_genre(GenreInstruction::OPERATION_BINAIRE)) {
        return false;
    }

    POUR_NOMME (inst, bloc->instructions) {
        if (!est_opérateur_binaire_constant(inst)) {
            continue;
        }

        auto résultat = évalue_opérateur_binaire(inst->comme_op_binaire(), constructrice);
        if (!résultat) {
            continue;
        }

        graphe.visite_utilisateurs(inst, [&](Atome *utilisateur) {
            if (!remplace_instruction_par_atome(
                    utilisateur, inst, résultat, branche_conditionnelle_fut_changée)) {
                return;
            }

            bloc->tag_instruction_à_supprimer(inst);
        });
    }

    return bloc->supprime_instructions_à_supprimer();
}

static void supprime_op_binaires_constants(FonctionEtBlocs &fonction_et_blocs,
                                           ConstructriceRI &constructrice,
                                           bool *branche_conditionnelle_fut_changée)
{
    auto &graphe = fonction_et_blocs.donne_graphe_ajourné();

    auto bloc_modifié = false;
    POUR (fonction_et_blocs.blocs) {
        bloc_modifié |= supprime_op_binaires_constants(
            it, graphe, constructrice, branche_conditionnelle_fut_changée);
    }

    if (bloc_modifié) {
        fonction_et_blocs.marque_blocs_modifiés();
    }
}

/* ******************************************************************************************* */

Atome *peut_remplacer_instruction_binaire_par_opérande(InstructionOpBinaire const *op_binaire)
{
    if (op_binaire->op == OpérateurBinaire::Genre::Soustraction) {
        auto droite = op_binaire->valeur_droite;

        if (est_constante_entière_zéro(droite)) {
            return op_binaire->valeur_gauche;
        }

        return nullptr;
    }

    if (op_binaire->op == OpérateurBinaire::Genre::Addition) {
        auto droite = op_binaire->valeur_droite;

        if (est_constante_entière_zéro(droite)) {
            return op_binaire->valeur_gauche;
        }

        return nullptr;
    }

    if (op_binaire->op == OpérateurBinaire::Genre::Multiplication) {
        auto droite = op_binaire->valeur_droite;

        if (est_constante_entière_un(droite)) {
            return op_binaire->valeur_gauche;
        }

        if (est_constante_entière_zéro(droite)) {
            return droite;
        }

        return nullptr;
    }

    if (op_binaire->op == OpérateurBinaire::Genre::Division_Naturel ||
        op_binaire->op == OpérateurBinaire::Genre::Division_Relatif) {
        auto droite = op_binaire->valeur_droite;

        if (est_constante_entière_un(droite)) {
            return op_binaire->valeur_gauche;
        }

        return nullptr;
    }

    if (op_binaire->op == OpérateurBinaire::Genre::Ou_Binaire ||
        op_binaire->op == OpérateurBinaire::Genre::Dec_Droite_Arithm ||
        op_binaire->op == OpérateurBinaire::Genre::Dec_Droite_Logique ||
        op_binaire->op == OpérateurBinaire::Genre::Dec_Gauche) {
        auto droite = op_binaire->valeur_droite;

        if (est_constante_entière_zéro(droite)) {
            return op_binaire->valeur_gauche;
        }

        return nullptr;
    }

    if (op_binaire->op == OpérateurBinaire::Genre::Et_Binaire) {
        auto droite = op_binaire->valeur_droite;

        if (est_constante_entière_zéro(droite)) {
            return droite;
        }

        return nullptr;
    }

    return nullptr;
}

static bool supprime_op_binaires_inutiles(Bloc *bloc,
                                          Graphe &graphe,
                                          bool *branche_conditionnelle_fut_changée)
{
    if (!bloc->possède_instruction_de_genre(GenreInstruction::OPERATION_BINAIRE)) {
        return false;
    }

    POUR_NOMME (inst, bloc->instructions) {
        if (!inst->est_op_binaire()) {
            continue;
        }

        auto op_binaire = inst->comme_op_binaire();
        auto remplacement = peut_remplacer_instruction_binaire_par_opérande(op_binaire);
        if (!remplacement) {
            continue;
        }

        graphe.visite_utilisateurs(inst, [&](Atome *utilisateur) {
            if (!remplace_instruction_par_atome(
                    utilisateur, inst, remplacement, branche_conditionnelle_fut_changée)) {
                return;
            }

            bloc->tag_instruction_à_supprimer(inst);
        });
    }

    return bloc->supprime_instructions_à_supprimer();
}

static void supprime_op_binaires_inutiles(FonctionEtBlocs &fonction_et_blocs,
                                          bool *branche_conditionnelle_fut_changée)
{
    auto &graphe = fonction_et_blocs.donne_graphe_ajourné();

    auto bloc_modifié = false;
    POUR (fonction_et_blocs.blocs) {
        bloc_modifié |= supprime_op_binaires_inutiles(
            it, graphe, branche_conditionnelle_fut_changée);
    }

    if (bloc_modifié) {
        fonction_et_blocs.marque_blocs_modifiés();
    }
}

/* ********************************************************************************************
 */

/* Performe différentes analyses de la RI. Ces analyses nous servent à valider un peu plus la
 * structures du programme. Nous pourrions les faire lors de la validation sémantique, mais ce
 * serait un peu plus complexe car l'arbre syntaxique, contrairement à la RI, a plus de cas
 * spéciaux.
 *
 * À FAIRE(analyse_ri) :
 * - membre actifs des unions
 */
void ContexteAnalyseRI::analyse_ri(EspaceDeTravail &espace,
                                   ConstructriceRI &constructrice,
                                   AtomeFonction *atome)
{
    reinitialise();

    if (atome->est_externe || atome->instructions.est_vide()) {
        return;
    }

#if 0
    POUR (atome->instructions) {
        if (!it->type) {
            continue;
        }
        assert_rappel(!it->type->est_type_entier_constant(), [&]() {
            dbg() << "Entier constant dans " << atome->nom;
            dbg() << imprime_arbre_instruction(it);
            dbg() << imprime_fonction(atome);
        });
    }
#endif

    if (!fonction_et_blocs.convertis_en_blocs(espace, atome)) {
        return;
    }

    if (!détecte_blocs_invalides(espace, fonction_et_blocs)) {
        return;
    }

    if (!détecte_retour_manquant(espace, fonction_et_blocs, visiteuse)) {
        return;
    }

    if (!détecte_utilisations_adresses_locales(espace, *atome)) {
        return;
    }

    supprime_blocs_vides(fonction_et_blocs, visiteuse);

    supprime_branches_inutiles(fonction_et_blocs, constructrice, visiteuse);

    supprime_allocations_temporaires(fonction_et_blocs);

    /* À faire après la supressions des allocations temporaires. */
    if (!détecte_opérateurs_binaires_suspicieux(espace, fonction_et_blocs)) {
        return;
    }

    auto branche_conditionnelle_fut_changée = false;
    supprime_op_binaires_constants(
        fonction_et_blocs, constructrice, &branche_conditionnelle_fut_changée);

    supprime_op_binaires_inutiles(fonction_et_blocs, &branche_conditionnelle_fut_changée);

    if (branche_conditionnelle_fut_changée) {
        supprime_branches_inutiles(fonction_et_blocs, constructrice, visiteuse);
    }

    fonction_et_blocs.ajourne_instructions_fonction_si_nécessaire();

    valide_fonction(espace, *atome);

#ifdef ANALYSE_RI_PEUT_VERIFIER_VARIABLES_INUTILISEES
    if (!détecte_déclarations_inutilisées(espace, atome)) {
        return;
    }
#endif

    if (atome->decl->possède_drapeau(DrapeauxNoeudFonction::CLICHÉ_RI_FINALE_FUT_REQUIS)) {
        dbg() << imprime_fonction(atome);
    }
}

void ContexteAnalyseRI::reinitialise()
{
    fonction_et_blocs.réinitialise();
}
