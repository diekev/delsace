/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "analyse.hh"

#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"

#include "structures/ensemble.hh"
#include "structures/file.hh"

#include "bloc_basique.hh"
#include "impression.hh"
#include "instructions.hh"

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

#if 0
    // La génération de RI peut mettre des labels après des instructions « si » ou « discr » qui
    // sont les seules instructions de la fonction, donc nous pouvons avoir des blocs vides en fin
    // de fonctions. Mais ce peut également être du code mort après un retour.
    POUR (fonction_et_blocs.blocs) {
        if (!visiteuse.a_visité(it)) {
            imprime_fonction(atome, std::cerr);
            imprime_blocs(fonction_et_blocs.blocs, std::cerr);
            espace
                .rapporte_erreur(atome->decl,
                                 "Erreur interne, un ou plusieurs blocs n'ont pas été visité !")
                .ajoute_message("Le premier bloc non visité est le bloc ", it->label->id);
            return false;
        }
    }
#endif

    return true;
}

/* ********************************************************************************************* */

// Il reste des choses à faire pour activer ceci
#undef ANALYSE_RI_PEUT_VERIFIER_VARIABLES_INUTILISEES

static auto incrémente_nombre_utilisations_récursif(Atome *racine) -> void
{
    racine->nombre_utilisations += 1;

    switch (racine->genre_atome) {
        case Atome::Genre::GLOBALE:
        case Atome::Genre::FONCTION:
        case Atome::Genre::CONSTANTE:
        {
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = racine->comme_instruction();

            switch (inst->genre) {
                case Instruction::Genre::APPEL:
                {
                    auto appel = inst->comme_appel();

                    /* appele peut être un pointeur de fonction */
                    incrémente_nombre_utilisations_récursif(appel->appele);

                    POUR (appel->args) {
                        incrémente_nombre_utilisations_récursif(it);
                    }

                    break;
                }
                case Instruction::Genre::CHARGE_MEMOIRE:
                {
                    auto charge = inst->comme_charge();
                    incrémente_nombre_utilisations_récursif(charge->chargee);
                    break;
                }
                case Instruction::Genre::STOCKE_MEMOIRE:
                {
                    auto stocke = inst->comme_stocke_mem();
                    incrémente_nombre_utilisations_récursif(stocke->valeur);
                    incrémente_nombre_utilisations_récursif(stocke->ou);
                    break;
                }
                case Instruction::Genre::OPERATION_UNAIRE:
                {
                    auto op = inst->comme_op_unaire();
                    incrémente_nombre_utilisations_récursif(op->valeur);
                    break;
                }
                case Instruction::Genre::OPERATION_BINAIRE:
                {
                    auto op = inst->comme_op_binaire();
                    incrémente_nombre_utilisations_récursif(op->valeur_droite);
                    incrémente_nombre_utilisations_récursif(op->valeur_gauche);
                    break;
                }
                case Instruction::Genre::ACCEDE_INDEX:
                {
                    auto acces = inst->comme_acces_index();
                    incrémente_nombre_utilisations_récursif(acces->index);
                    incrémente_nombre_utilisations_récursif(acces->accede);
                    break;
                }
                case Instruction::Genre::ACCEDE_MEMBRE:
                {
                    auto acces = inst->comme_acces_membre();
                    incrémente_nombre_utilisations_récursif(acces->index);
                    incrémente_nombre_utilisations_récursif(acces->accede);
                    break;
                }
                case Instruction::Genre::TRANSTYPE:
                {
                    auto transtype = inst->comme_transtype();
                    incrémente_nombre_utilisations_récursif(transtype->valeur);
                    break;
                }
                case Instruction::Genre::BRANCHE_CONDITION:
                {
                    auto branche = inst->comme_branche_cond();
                    incrémente_nombre_utilisations_récursif(branche->condition);
                    break;
                }
                case Instruction::Genre::RETOUR:
                {
                    auto retour = inst->comme_retour();

                    if (retour->valeur) {
                        incrémente_nombre_utilisations_récursif(retour->valeur);
                    }

                    break;
                }
                case Instruction::Genre::ALLOCATION:
                case Instruction::Genre::INVALIDE:
                case Instruction::Genre::BRANCHE:
                case Instruction::Genre::LABEL:
                {
                    break;
                }
            }

            break;
        }
    }
}

enum {
    EST_PARAMETRE_FONCTION = (1 << 1),
};

static Atome *déréférence_instruction(Instruction *inst)
{
    if (inst->est_acces_index()) {
        auto acces = inst->comme_acces_index();
        return acces->accede;
    }

    if (inst->est_acces_membre()) {
        auto acces = inst->comme_acces_membre();
        return acces->accede;
    }

    // pour les déréférencements de pointeurs
    if (inst->est_charge()) {
        auto charge = inst->comme_charge();
        return charge->chargee;
    }

    if (inst->est_transtype()) {
        auto transtype = inst->comme_transtype();
        return transtype->valeur;
    }

    return inst;
}

static bool est_locale_ou_globale(Atome const *atome)
{
    if (atome->est_globale()) {
        return true;
    }

    if (atome->est_instruction()) {
        auto inst = atome->comme_instruction();
        return inst->est_alloc();
    }

    return false;
}

static Atome *cible_finale_stockage(InstructionStockeMem *stocke)
{
    auto ou = stocke->ou;
    auto ancien_ou = ou;

    while (!est_locale_ou_globale(ou)) {
        if (!ou->est_instruction()) {
            break;
        }

        auto inst = ou->comme_instruction();
        ou = déréférence_instruction(inst);

        if (ou == ancien_ou) {
            std::cerr << "Boucle infinie !!!!!!\n";
            imprime_atome(ou, std::cerr);

            if (ou->est_instruction()) {
                imprime_instruction(ou->comme_instruction(), std::cerr);
            }

            std::cerr << "\n";
        }

        ancien_ou = ou;
    }

    return ou;
}

/* Retourne vrai si un paramètre ou une globale fut utilisée lors de la production de l'atome. */
static bool paramètre_ou_globale_fut_utilisé(Atome *atome)
{
    auto resultat = false;
    visite_atome(atome, [&resultat](Atome const *visite) {
        /* À FAIRE(analyse_ri) : utiliser nombre_utilisations nous donne des faux-négatifs : une
         * variable non-utilisée peut être marquée comme utilisée si elle dépend d'un paramètre ou
         * d'une globale. */
        /* À FAIRE(analyse_ri) : le contexte implicite parasite également la détection d'une
         * expression non-utilisée. */
        if ((visite->etat & EST_PARAMETRE_FONCTION) || visite->est_globale() ||
            visite->nombre_utilisations != 0) {
            resultat = true;
        }
    });
    return resultat;
}

void marque_instructions_utilisées(kuri::tableau<Instruction *, int> &instructions)
{
    for (auto i = instructions.taille() - 1; i >= 0; --i) {
        auto it = instructions[i];

        if (it->nombre_utilisations != 0) {
            continue;
        }

        switch (it->genre) {
            case Instruction::Genre::BRANCHE:
            case Instruction::Genre::BRANCHE_CONDITION:
            case Instruction::Genre::LABEL:
            case Instruction::Genre::RETOUR:
            {
                incrémente_nombre_utilisations_récursif(it);
                break;
            }
            case Instruction::Genre::APPEL:
            {
                auto appel = it->comme_appel();

                if (appel->type->est_type_rien()) {
                    incrémente_nombre_utilisations_récursif(it);
                }

                break;
            }
            case Instruction::Genre::STOCKE_MEMOIRE:
            {
                auto stocke = it->comme_stocke_mem();
                auto cible = cible_finale_stockage(stocke);

                if ((cible->etat & EST_PARAMETRE_FONCTION) || cible->nombre_utilisations != 0 ||
                    cible->est_globale()) {
                    incrémente_nombre_utilisations_récursif(stocke);
                }
                else {
                    /* Vérifie si l'instruction de stockage prend la valeur d'une globale ou d'un
                     * paramètre. */
                    if (paramètre_ou_globale_fut_utilisé(stocke->valeur)) {
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
static bool détecte_déclarations_inutilisées(EspaceDeTravail &espace, AtomeFonction *atome)
{
    /* Ignore les fonctions d'initalisation des types car les paramètres peuvent ne pas être
     * utilisés, par exemple pour la fonction d'initialisation du type « rien ». */
    if (atome->decl &&
        atome->decl->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
        return true;
    }

    POUR (atome->params_entrees) {
        it->etat = EST_PARAMETRE_FONCTION;
    }

    atome->param_sortie->etat = EST_PARAMETRE_FONCTION;

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

        auto alloc = it->comme_instruction()->comme_alloc();
        auto decl_alloc = alloc->site;

        /* Si le site n'est pas une déclaration de variable (le contexte implicite n'a pas de
         * site propre, celui de la fonction est utilisé), ajoutons-la à la liste des
         * allocations non-utilisées pour avoir un avertissement. */
        if (!decl_alloc || !decl_alloc->est_declaration_variable()) {
            allocs_inutilisees.ajoute(alloc);
            continue;
        }

        auto decl_var = decl_alloc->comme_declaration_variable();
        if (!possède_annotation(decl_var, "inutilisée")) {
            allocs_inutilisees.ajoute(alloc);
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
        imprime_fonction(atome, std::cerr, false, true);
    }
#    endif

    POUR (allocs_inutilisees) {
        if (it->etat & EST_PARAMETRE_FONCTION) {
            espace.rapporte_avertissement(it->site, "Paramètre inutilisé");
        }
        else {
            espace.rapporte_avertissement(it->site, "Variable locale inutilisée");
        }
    }

    POUR (atome->instructions) {
        if (!it->est_appel() || it->nombre_utilisations != 0) {
            continue;
        }

        espace.rapporte_avertissement(it->site, "Retour de fonction inutilisé");
    }

    return true;
}
#endif

/* ******************************************************************************************** */

static bool atome_est_pour_création_contexte(Compilatrice &compilatrice, AtomeFonction *atome)
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

        auto di = it->instructions.dernière();

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

        auto di = it->instructions.dernière();
        if (!di->est_branche()) {
            continue;
        }

        auto branche = di->comme_branche();

        for (auto parent : it->parents) {
            auto di_parent = parent->instructions.dernière();

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
void supprime_branches_inutiles(FonctionEtBlocs &fonction_et_blocs, VisiteuseBlocs &visiteuse)
{
    auto bloc_modifié = false;

    for (auto i = 0; i < fonction_et_blocs.blocs.taille(); ++i) {
        auto it = fonction_et_blocs.blocs[i];

        if (it->instructions.est_vide()) {
            /* Le bloc fut fusionné ici. */
            continue;
        }

        auto di = it->instructions.dernière();

        if (di->est_branche_cond()) {
            auto branche = di->comme_branche_cond();
            if (branche->label_si_faux == branche->label_si_vrai) {
                /* Remplace par une branche.
                 * À FAIRE : crée une instruction. */
                auto nouvelle_branche = reinterpret_cast<InstructionBranche *>(branche);
                nouvelle_branche->genre = Instruction::Genre::BRANCHE;
                nouvelle_branche->label = branche->label_si_faux;
                bloc_modifié = true;
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
        if (it->instructions.dernière()->est_branche()) {
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
    auto const valeur = stockage.valeur;

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
    if (atome.est_constante()) {
        return SourceAdresseAtome::CONSTANTE;
    }

    POUR (fonction.params_entrees) {
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
    auto const valeur = stockage.valeur;
    std::cerr << "Stockage d'un pointeur de " << source << " vers " << destination << '\n';

    std::cerr << "--------- Instruction défaillante :\n";
    imprime_instruction(&stockage, std::cerr);
    std::cerr << "\n";
    std::cerr << "--------- Dernière valeur :\n";
    if (valeur->est_instruction()) {
        imprime_instruction(valeur->comme_instruction(), std::cerr);
    }
    else {
        imprime_atome(valeur, std::cerr);
    }
    std::cerr << "\n";
    std::cerr << "--------- Fonction défaillante :\n";
    imprime_fonction(
        &fonction, std::cerr, false, false, [&](const Instruction &instruction, std::ostream &os) {
            os << " " << sources[instruction.numero]
               << " (si charge : " << sources_pour_charge[instruction.numero] << ")";
        });
    std::cerr << "\n";
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
    if (fonction.decl && fonction.decl->ident == ID::crée_contexte) {
        return true;
    }

    auto const taille_sources = numérote_instructions(fonction);

    /* Pour chaque instruction, stocke la source de l'adresse. */
    kuri::tableau<SourceAdresseAtome> sources(taille_sources);
    POUR (sources) {
        it = SourceAdresseAtome::INCONNUE;
    }

    /* Afin de différencier l'adresse d'une variable de celle de son contenu, ceci stocke pour
     * chaque instruction la source de l'adresse pointée par la variable. */
    kuri::tableau<SourceAdresseAtome> sources_pour_charge = sources;

    for (auto i = 0; i < fonction.params_entrees.taille(); i++) {
        sources[i] = SourceAdresseAtome::PARAMÈTRE_ENTRÉE;
        sources_pour_charge[i] = SourceAdresseAtome::PARAMÈTRE_ENTRÉE;
    }

    int index = fonction.params_entrees.taille();
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
            if (it->comme_charge()->chargee->est_instruction()) {
                auto inst = it->comme_charge()->chargee->comme_instruction();
                sources[it->numero] = sources_pour_charge[inst->numero];
            }
            else {
                sources[it->numero] = détermine_source_adresse_atome(
                    fonction, *it->comme_charge()->chargee, sources);
            }

            continue;
        }

        if (it->est_transtype()) {
            sources[it->numero] = détermine_source_adresse_atome(
                fonction, *it->comme_transtype()->valeur, sources);
            continue;
        }

        if (it->est_acces_membre()) {
            auto accede = it->comme_acces_membre()->accede;
            sources[it->numero] = détermine_source_adresse_atome(fonction, *accede, sources);
            if (accede->est_instruction()) {
                sources_pour_charge[it->numero] =
                    sources_pour_charge[accede->comme_instruction()->numero];
            }
            continue;
        }

        if (it->est_acces_index()) {
            auto accede = it->comme_acces_index()->accede;
            sources[it->numero] = détermine_source_adresse_atome(fonction, *accede, sources);
            if (accede->est_instruction()) {
                sources_pour_charge[it->numero] =
                    sources_pour_charge[accede->comme_instruction()->numero];
            }
            continue;
        }

        if (it->est_stocke_mem()) {
            auto const stockage = it->comme_stocke_mem();
            auto const ou = stockage->ou;
            auto const valeur = stockage->valeur;

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

/** ******************************************************************************************
 * \name Graphe
 * \{
 */

void Graphe::ajoute_connexion(Atome *a, Atome *b, int index_bloc)
{
    connexions.ajoute({a, b, index_bloc});

    if (connexions_pour_inst.possède(a)) {
        auto &idx = connexions_pour_inst.trouve_ref(a);
        idx.ajoute(static_cast<int>(connexions.taille() - 1));
    }
    else {
        kuri::tablet<int, 4> idx;
        idx.ajoute(static_cast<int>(connexions.taille() - 1));
        connexions_pour_inst.insère(a, idx);
    }
}

void Graphe::construit(const kuri::tableau<Instruction *, int> &instructions, int index_bloc)
{
    POUR (instructions) {
        visite_opérandes_instruction(
            it, [&](Atome *atome_courant) { ajoute_connexion(atome_courant, it, index_bloc); });
    }
}

bool Graphe::est_uniquement_utilisé_dans_bloc(Instruction *inst, int index_bloc) const
{
    auto idx = connexions_pour_inst.valeur_ou(inst, {});
    POUR (idx) {
        auto &connexion = connexions[it];
        if (index_bloc != connexion.index_bloc) {
            return false;
        }
    }

    return true;
}

void Graphe::réinitialise()
{
    connexions_pour_inst.reinitialise();
    connexions.efface();
}

template <typename Fonction>
void Graphe::visite_utilisateurs(Instruction *inst, Fonction rappel) const
{
    auto idx = connexions_pour_inst.valeur_ou(inst, {});
    POUR (idx) {
        auto &connexion = connexions[it];
        rappel(connexion.utilisateur);
    }
}

/** \} */

static bool est_stockage_vers(Instruction const *inst0, Instruction const *inst1)
{
    if (!inst0->est_stocke_mem()) {
        return false;
    }

    auto const stockage = inst0->comme_stocke_mem();
    return stockage->ou == inst1;
}

static bool est_transtypage_de(Instruction const *inst0, Instruction const *inst1)
{
    if (!inst0->est_transtype()) {
        return false;
    }

    auto const transtype = inst0->comme_transtype();
    return transtype->valeur == inst1;
}

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

static bool est_chargement_de(Instruction const *inst0, Instruction const *inst1)
{
    if (!inst0->est_charge()) {
        return false;
    }

    auto const charge = inst0->comme_charge();
    return charge->chargee == inst1;
}

#define ASSIGNE_SI_EGAUX(a, b, c)                                                                 \
    if (a == b) {                                                                                 \
        a = c;                                                                                    \
    }

enum {
    EST_A_SUPPRIMER = 123,
};

static bool supprime_allocations_temporaires(Graphe const &g, Bloc *bloc, int index_bloc)
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
        if (!g.est_uniquement_utilisé_dans_bloc(inst0, index_bloc)) {
            continue;
        }

        /* Si le chargement n'est pas uniquement dans ce bloc, ce n'est pas une temporaire.
         * Ceci survient notamment dans la génération de code pour les vérifications des bornes des
         * tableaux ou chaines. */
        if (!g.est_uniquement_utilisé_dans_bloc(inst2, index_bloc)) {
            continue;
        }

        auto est_utilisee_uniquement_pour_charge_et_stocke = true;
        g.visite_utilisateurs(inst0, [&](Atome *utilisateur) {
            if (utilisateur != inst1 && utilisateur != inst2) {
                est_utilisee_uniquement_pour_charge_et_stocke = false;
            }
        });

        if (!est_utilisee_uniquement_pour_charge_et_stocke) {
            continue;
        }

        g.visite_utilisateurs(inst2, [&](Atome *utilisateur) {
            if (!utilisateur->est_instruction()) {
                return;
            }

            auto utilisatrice = utilisateur->comme_instruction();
            auto nouvelle_valeur = inst1->comme_stocke_mem()->valeur;

            if (utilisatrice->est_appel()) {
                return;
            }

            if (utilisatrice->est_stocke_mem()) {
                auto stockage = utilisatrice->comme_stocke_mem();
                ASSIGNE_SI_EGAUX(stockage->ou, inst2, nouvelle_valeur)
                ASSIGNE_SI_EGAUX(stockage->valeur, inst2, nouvelle_valeur)
            }
            else if (utilisatrice->est_op_binaire()) {
                auto op_binaire = utilisatrice->comme_op_binaire();
                ASSIGNE_SI_EGAUX(op_binaire->valeur_droite, inst2, nouvelle_valeur)
                ASSIGNE_SI_EGAUX(op_binaire->valeur_gauche, inst2, nouvelle_valeur)
            }
            else if (utilisatrice->est_op_unaire()) {
                auto op_unaire = utilisatrice->comme_op_unaire();
                ASSIGNE_SI_EGAUX(op_unaire->valeur, inst2, nouvelle_valeur)
            }
            else if (utilisatrice->est_branche_cond()) {
                auto branche_cond = utilisatrice->comme_branche_cond();
                ASSIGNE_SI_EGAUX(branche_cond->condition, inst2, nouvelle_valeur)
            }
            else if (utilisatrice->est_transtype()) {
                auto transtype = utilisatrice->comme_transtype();
                ASSIGNE_SI_EGAUX(transtype->valeur, inst2, nouvelle_valeur)
            }
            else if (utilisatrice->est_acces_membre()) {
                auto membre = utilisatrice->comme_acces_membre();
                ASSIGNE_SI_EGAUX(membre->accede, inst2, nouvelle_valeur)
            }
            else if (utilisatrice->est_acces_index()) {
                auto index = utilisatrice->comme_acces_index();
                ASSIGNE_SI_EGAUX(index->accede, inst2, nouvelle_valeur)
                ASSIGNE_SI_EGAUX(index->index, inst2, nouvelle_valeur)
            }
            else {
                return;
            }

            inst0->etat = EST_A_SUPPRIMER;
            inst1->etat = EST_A_SUPPRIMER;
            inst2->etat = EST_A_SUPPRIMER;
        });
    }

    auto nouvelle_fin = std::stable_partition(
        bloc->instructions.debut(), bloc->instructions.fin(), [](Instruction *inst) {
            return inst->etat != EST_A_SUPPRIMER;
        });

    auto nouvelle_taille = std::distance(bloc->instructions.debut(), nouvelle_fin);

    auto const bloc_modifié = nouvelle_taille != bloc->instructions.taille();
    bloc->instructions.redimensionne(static_cast<int>(nouvelle_taille));
    return bloc_modifié;
}

static std::optional<int> trouve_stockage_dans_bloc(Bloc *bloc,
                                                    Instruction *alloc,
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

            if (inst->etat == EST_A_SUPPRIMER) {
                std::cerr << "La fonction est " << nom_humainement_lisible(fonction.decl) << '\n';
                std::cerr << *fonction.decl << '\n';

                std::cerr << "L'instruction supprimée est ";
                imprime_instruction(inst, std::cerr);
                std::cerr << "\n";

                std::cerr << "L'utilisateur est ";
                imprime_instruction(it, std::cerr);
                std::cerr << "\n";

                espace.rapporte_erreur(
                    inst->site,
                    "Erreur interne : la fonction référence une instruction supprimée");
            }
        });
    }
}

static void supprime_allocations_temporaires(Graphe &graphe, FonctionEtBlocs &fonction_et_blocs)
{
    auto index_bloc = 0;
    POUR (fonction_et_blocs.blocs) {
        graphe.construit(it->instructions, index_bloc++);
    }

    index_bloc = 0;
    auto bloc_modifié = false;
    POUR (fonction_et_blocs.blocs) {
        bloc_modifié |= rapproche_allocations_des_stockages(it);
        bloc_modifié |= supprime_allocations_temporaires(graphe, it, index_bloc++);
    }

    if (!bloc_modifié) {
        return;
    }

    fonction_et_blocs.marque_blocs_modifiés();
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
void ContexteAnalyseRI::analyse_ri(EspaceDeTravail &espace, AtomeFonction *atome)
{
    reinitialise();

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

    supprime_branches_inutiles(fonction_et_blocs, visiteuse);

    supprime_allocations_temporaires(graphe, fonction_et_blocs);

    fonction_et_blocs.ajourne_instructions_fonction_si_nécessaire();

    valide_fonction(espace, *atome);

#ifdef ANALYSE_RI_PEUT_VERIFIER_VARIABLES_INUTILISEES
    if (!détecte_déclarations_inutilisées(espace, atome)) {
        return;
    }
#endif

    if (atome->decl->possède_drapeau(DrapeauxNoeudFonction::CLICHÉ_RI_FINALE_FUT_REQUIS)) {
        imprime_fonction(atome, std::cerr);
    }
}

void ContexteAnalyseRI::reinitialise()
{
    graphe.réinitialise();
    fonction_et_blocs.réinitialise();
}
