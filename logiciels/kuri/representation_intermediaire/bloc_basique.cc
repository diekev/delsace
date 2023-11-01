/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "bloc_basique.hh"

#include <iostream>

#include "constructrice_ri.hh"
#include "espace_de_travail.hh"
#include "impression.hh"
#include "instructions.hh"
#include "log.hh"

/* ********************************************************************************************* */

static InstructionAllocation *alloc_ou_nul(Atome *atome)
{
    if (!atome->est_instruction()) {
        return nullptr;
    }

    auto inst = atome->comme_instruction();

    if (inst->est_alloc()) {
        return inst->comme_alloc();
    }

    if (inst->est_acces_membre()) {
        return alloc_ou_nul(inst->comme_acces_membre()->accede);
    }

    return nullptr;
}

/* ********************************************************************************************* */

void Bloc::ajoute_enfant(Bloc *enfant)
{
    enfant->ajoute_parent(this);

    POUR (enfants) {
        if (it == enfant) {
            return;
        }
    }

    enfants.ajoute(enfant);
}

void Bloc::remplace_enfant(Bloc *enfant, Bloc *par)
{
    enlève_du_tableau(enfants, enfant);
    ajoute_enfant(par);
    enfant->enlève_parent(this);
    par->ajoute_parent(this);

    auto inst = instructions.dernière();

    if (inst->est_branche()) {
        auto branche = inst->comme_branche();
        branche->label = par->label;
        return;
    }

    if (inst->est_branche_cond()) {
        auto branche_cond = inst->comme_branche_cond();
        auto label_si_vrai = branche_cond->label_si_vrai;
        auto label_si_faux = branche_cond->label_si_faux;

        if (label_si_vrai == enfant->label) {
            branche_cond->label_si_vrai = par->label;
        }

        if (label_si_faux == enfant->label) {
            branche_cond->label_si_faux = par->label;
        }

        return;
    }
}

void Bloc::remplace_parent(Bloc *parent, Bloc *par)
{
    enlève_du_tableau(parents, parent);
    ajoute_parent(par);
    par->ajoute_enfant(this);
}

void Bloc::enlève_parent(Bloc *parent)
{
    enlève_du_tableau(parents, parent);
}

void Bloc::enlève_enfant(Bloc *enfant)
{
    enlève_du_tableau(enfants, enfant);

    /* quand nous enlevons un enfant, il faut modifier la cible des branches potentielles */

    if (instructions.est_vide()) {
        return;
    }

    /* création_contexte n'a pas d'instruction de retour à la fin de son bloc, et après
     * l'enlignage, nous nous retrouvons avec un bloc vide à la fin de la fonction */
    if (enfant->instructions.est_vide()) {
        return;
    }

    auto inst = instructions.dernière();

    if (log_actif) {
        std::cerr << "-- dernière inststruction : ";
        imprime_instruction(inst, std::cerr);
    }

    if (inst->est_branche()) {
        // À FAIRE
        return;
    }

    if (inst->est_branche_cond()) {
        auto branche_cond = inst->comme_branche_cond();
        auto label_si_vrai = branche_cond->label_si_vrai;
        auto label_si_faux = branche_cond->label_si_faux;

        if (label_si_vrai == enfant->label) {
            branche_cond->label_si_vrai = label_si_faux;
        }
        else if (label_si_faux == enfant->label) {
            branche_cond->label_si_faux = label_si_vrai;
        }
        else {
            // assert(0);
        }

        return;
    }

    if (inst->est_retour()) {
        return;
    }

    log(std::cerr, "bloc ", label->id);
    assert(0);
}

bool Bloc::peut_fusionner_enfant()
{
    if (enfants.taille() == 0) {
        log(std::cerr, "enfant == 0");
        return false;
    }

    if (enfants.taille() > 1) {
        log(std::cerr, "enfants.taille() > 1");
        return false;
    }

    auto enfant = enfants[0];

    if (enfant->parents.taille() > 1) {
        log(std::cerr, "enfants.parents.taille() > 1");
        return false;
    }

    return true;
}

void Bloc::utilise_variable(InstructionAllocation *variable)
{
    if (!variable) {
        return;
    }

    for (auto var : this->variables_utilisees) {
        if (var == variable) {
            return;
        }
    }

    this->variables_utilisees.ajoute(variable);
    variable->blocs_utilisants += 1;
}

void Bloc::fusionne_enfant(Bloc *enfant)
{
    log(std::cerr,
        "S'apprête à fusionner le bloc ",
        enfant->label->id,
        " dans le bloc ",
        this->label->id);

    this->instructions.supprime_dernier();
    this->instructions.reserve_delta(enfant->instructions.taille());

    POUR (enfant->instructions) {
        this->instructions.ajoute(it);
    }

    this->variables_declarees.reserve(enfant->variables_declarees.taille() +
                                      this->variables_declarees.taille());
    POUR (enfant->variables_declarees) {
        this->variables_declarees.ajoute(it);
    }

    POUR (enfant->variables_utilisees) {
        it->blocs_utilisants -= 1;
        this->utilise_variable(it);
    }

    /* Supprime la référence à l'enfant dans la hiérarchie. */
    this->enlève_enfant(enfant);
    enfant->enlève_parent(this);

    /* Remplace l'enfant pour nou-même comme parent dans ses enfants. */
    POUR (enfant->enfants) {
        this->ajoute_enfant(it);
        it->remplace_parent(enfant, this);
    }

    /* À FAIRE : c'est quoi ça ? */
    POUR (this->enfants) {
        it->enlève_parent(enfant);
    }

    //		std::cerr << "-- enfants après fusion : ";
    //		POUR (this->enfants) {
    //			std::cerr << it->label->id << " ";
    //		}
    //		std::cerr << "\n";

    if (log_actif) {
        std::cerr << "-- bloc après fusion :\n";
        imprime_bloc(this, 0, std::cerr);
    }

    enfant->instructions.efface();
}

void Bloc::réinitialise()
{
    label = nullptr;
    est_atteignable = false;
    instructions.efface();
    parents.efface();
    enfants.efface();
    variables_declarees.efface();
    variables_utilisees.efface();
}

void Bloc::enlève_du_tableau(kuri::tableau<Bloc *, int> &tableau, Bloc *bloc)
{
    for (auto i = 0; i < tableau.taille(); ++i) {
        if (tableau[i] == bloc) {
            std::swap(tableau[i], tableau[tableau.taille() - 1]);
            tableau.redimensionne(tableau.taille() - 1);
            break;
        }
    }
}

void Bloc::ajoute_parent(Bloc *parent)
{
    POUR (parents) {
        if (it == parent) {
            return;
        }
    }

    parents.ajoute(parent);
}

void imprime_bloc(Bloc *bloc,
                  int decalage_instruction,
                  std::ostream &os,
                  bool surligne_inutilisees)
{
    os << "Bloc " << bloc->label->id << ' ';

    auto virgule = " [";
    if (bloc->parents.est_vide()) {
        os << virgule;
    }
    for (auto parent : bloc->parents) {
        os << virgule << parent->label->id;
        virgule = ", ";
    }
    os << "]";

    virgule = " [";
    if (bloc->enfants.est_vide()) {
        os << virgule;
    }
    for (auto enfant : bloc->enfants) {
        os << virgule << enfant->label->id;
        virgule = ", ";
    }
    os << "]\n";

    POUR (bloc->instructions) {
        it->numero = decalage_instruction++;
    }

    imprime_instructions(bloc->instructions, os, false, surligne_inutilisees);
}

void imprime_blocs(const kuri::tableau<Bloc *, int> &blocs, std::ostream &os)
{
    os << "=================== Blocs ===================\n";

    int decalage_instruction = 0;
    POUR (blocs) {
        imprime_bloc(it, decalage_instruction, os);
        decalage_instruction += it->instructions.taille();
    }
}

void construit_liste_variables_utilisées(Bloc *bloc)
{
    POUR (bloc->instructions) {
        if (it->est_alloc()) {
            auto alloc = it->comme_alloc();
            bloc->variables_declarees.ajoute(alloc);
            continue;
        }

        if (it->est_stocke_mem()) {
            auto stocke = it->comme_stocke_mem();
            bloc->utilise_variable(alloc_ou_nul(stocke->ou));
        }
        else if (it->est_acces_membre()) {
            auto membre = it->comme_acces_membre();
            bloc->utilise_variable(alloc_ou_nul(membre->accede));
        }
        else if (it->est_op_binaire()) {
            auto op = it->comme_op_binaire();
            bloc->utilise_variable(alloc_ou_nul(op->valeur_gauche));
            bloc->utilise_variable(alloc_ou_nul(op->valeur_droite));
        }
    }
}

static Bloc *trouve_bloc_pour_label(kuri::tableau<Bloc *, int> &blocs, InstructionLabel *label)
{
    POUR (blocs) {
        if (it->label == label) {
            return it;
        }
    }
    return nullptr;
}

static Bloc *crée_bloc_pour_label(kuri::tableau<Bloc *, int> &blocs,
                                  kuri::tableau<Bloc *, int> &blocs_libres,
                                  InstructionLabel *label)
{
    auto bloc = trouve_bloc_pour_label(blocs, label);
    if (bloc) {
        return bloc;
    }

    if (!blocs_libres.est_vide()) {
        bloc = blocs_libres.dernière();
        blocs_libres.supprime_dernier();
    }
    else {
        bloc = memoire::loge<Bloc>("Bloc");
    }

    bloc->label = label;
    blocs.ajoute(bloc);
    return bloc;
}

static void détruit_blocs(kuri::tableau<Bloc *, int> &blocs)
{
    POUR (blocs) {
        memoire::deloge("Bloc", it);
    }
    blocs.efface();
}

FonctionEtBlocs::~FonctionEtBlocs()
{
    détruit_blocs(blocs);
}

bool FonctionEtBlocs::convertis_en_blocs(EspaceDeTravail &espace, AtomeFonction *atome_fonc)
{
    fonction = atome_fonc;

    auto numero_instruction = atome_fonc->params_entrees.taille();

    POUR (atome_fonc->instructions) {
        it->numero = numero_instruction++;

        if (it->est_label()) {
            crée_bloc_pour_label(blocs, blocs_libres, it->comme_label());
        }
    }

    Bloc *bloc_courant = nullptr;
    POUR (atome_fonc->instructions) {
        if (it->est_label()) {
            bloc_courant = trouve_bloc_pour_label(blocs, it->comme_label());

            if (!bloc_courant) {
                espace.rapporte_erreur(it->site, "Erreur interne, aucun bloc pour le label");
                return false;
            }

            continue;
        }

        bloc_courant->instructions.ajoute(it);

        if (it->est_branche()) {
            auto bloc_cible = trouve_bloc_pour_label(blocs, it->comme_branche()->label);

            if (!bloc_cible) {
                espace.rapporte_erreur(
                    it->site,
                    "Erreur interne, aucun bloc pour le label de la branche inconditionnelle");
                return false;
            }
            bloc_courant->ajoute_enfant(bloc_cible);
            continue;
        }

        if (it->est_branche_cond()) {
            auto label_si_vrai = it->comme_branche_cond()->label_si_vrai;
            auto label_si_faux = it->comme_branche_cond()->label_si_faux;

            auto bloc_si_vrai = trouve_bloc_pour_label(blocs, label_si_vrai);
            auto bloc_si_faux = trouve_bloc_pour_label(blocs, label_si_faux);

            if (!bloc_si_vrai) {
                espace.rapporte_erreur(
                    it->site, "Erreur interne, aucun bloc pour le label de la branche si vrai");
                return false;
            }
            if (!bloc_si_faux) {
                espace.rapporte_erreur(
                    it->site, "Erreur interne, aucun bloc pour le label de la branche si faux");
                return false;
            }

            bloc_courant->ajoute_enfant(bloc_si_vrai);
            bloc_courant->ajoute_enfant(bloc_si_faux);
            continue;
        }
    }

    return true;
}

void FonctionEtBlocs::réinitialise()
{
    fonction = nullptr;
    les_blocs_ont_été_modifiés = false;

    POUR (blocs) {
        it->réinitialise();
        blocs_libres.ajoute(it);
    }

    blocs.efface();
}

void FonctionEtBlocs::marque_blocs_modifiés()
{
    les_blocs_ont_été_modifiés = true;
}

static void marque_blocs_atteignables(VisiteuseBlocs &visiteuse)
{
    visiteuse.prépare_pour_nouvelle_traversée();
    while (Bloc *bloc_courant = visiteuse.bloc_suivant()) {
        bloc_courant->est_atteignable = true;
    }
}

void FonctionEtBlocs::supprime_blocs_inatteignables(VisiteuseBlocs &visiteuse)
{
    /* Réinitalise les drapaux. */
    POUR (blocs) {
        it->est_atteignable = false;
    }

    marque_blocs_atteignables(visiteuse);

    auto résultat = std::stable_partition(
        blocs.begin(), blocs.end(), [](Bloc const *bloc) { return bloc->est_atteignable; });

    if (résultat == blocs.end()) {
        return;
    }

    auto nombre_de_nouveaux_blocs = int(std::distance(blocs.begin(), résultat));

    for (auto i = nombre_de_nouveaux_blocs; i < blocs.taille(); i++) {
        blocs[i]->réinitialise();
        blocs_libres.ajoute(blocs[i]);
    }

    blocs.redimensionne(nombre_de_nouveaux_blocs);
    les_blocs_ont_été_modifiés = true;
}

void FonctionEtBlocs::ajourne_instructions_fonction_si_nécessaire()
{
    if (!les_blocs_ont_été_modifiés) {
        return;
    }

    transfère_instructions_blocs_à_fonction(blocs, fonction);
    les_blocs_ont_été_modifiés = false;
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxiliaires.
 * \{ */

void transfère_instructions_blocs_à_fonction(kuri::tableau_statique<Bloc *> blocs,
                                             AtomeFonction *fonction)
{
#undef IMPRIME_STATS
#ifdef IMPRIME_STATS
    static int instructions_supprimées = 0;
    static int instructions_totales = 0;
    auto const ancien_compte = fonction->instructions.taille();
#endif

    auto nombre_instructions = 0;
    POUR (blocs) {
        nombre_instructions += 1 + it->instructions.taille();
    }

    fonction->instructions.redimensionne(nombre_instructions);

    int décalage_instruction = 0;

    POUR (blocs) {
        fonction->instructions[décalage_instruction++] = it->label;

        for (auto inst : it->instructions) {
            fonction->instructions[décalage_instruction++] = inst;
        }
    }

    fonction->instructions.redimensionne(décalage_instruction);

#ifdef IMPRIME_STATS
    auto const supprimées = (ancien_compte - fonction->instructions.taille());
    instructions_totales += ancien_compte;

    if (supprimées != 0) {
        instructions_supprimées += supprimées;
        std::cerr << "Supprimé " << instructions_supprimées << " / " << instructions_totales
                  << " instructions\n";
    }
#endif
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name VisiteuseBlocs
 * \{ */

VisiteuseBlocs::VisiteuseBlocs(const FonctionEtBlocs &fonction_et_blocs)
    : m_fonction_et_blocs(fonction_et_blocs)
{
}

void VisiteuseBlocs::prépare_pour_nouvelle_traversée()
{
    blocs_visités.efface();
    à_visiter.efface();
    à_visiter.enfile(m_fonction_et_blocs.blocs[0]);
}

bool VisiteuseBlocs::a_visité(Bloc *bloc) const
{
    return blocs_visités.possède(bloc);
}

Bloc *VisiteuseBlocs::bloc_suivant()
{
    while (!à_visiter.est_vide()) {
        auto bloc_courant = à_visiter.defile();

        if (blocs_visités.possède(bloc_courant)) {
            continue;
        }

        blocs_visités.insère(bloc_courant);

        POUR (bloc_courant->enfants) {
            à_visiter.enfile(it);
        }

        return bloc_courant;
    }

    return nullptr;
}

/** \} */
