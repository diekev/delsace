/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "bloc_basique.hh"

#include "constructrice_ri.hh"
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
    enleve_du_tableau(enfants, enfant);
    ajoute_enfant(par);
    enfant->enleve_parent(this);
    par->ajoute_parent(this);

    auto inst = instructions.derniere();

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

void Bloc::enleve_parent(Bloc *parent)
{
    enleve_du_tableau(parents, parent);
}

void Bloc::enleve_enfant(Bloc *enfant)
{
    enleve_du_tableau(enfants, enfant);

    /* quand nous enlevons un enfant, il faut modifier la cible des branches potentielles */

    if (instructions.est_vide()) {
        return;
    }

    /* création_contexte n'a pas d'instruction de retour à la fin de son bloc, et après
     * l'enlignage, nous nous retrouvons avec un bloc vide à la fin de la fonction */
    if (enfant->instructions.est_vide()) {
        return;
    }

    auto inst = instructions.derniere();

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

    this->enleve_enfant(enfant);

    POUR (enfant->enfants) {
        this->ajoute_enfant(it);
    }

    POUR (this->enfants) {
        it->enleve_parent(enfant);
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

void Bloc::enleve_du_tableau(kuri::tableau<Bloc *, int> &tableau, Bloc *bloc)
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

    imprime_instructions(
        bloc->instructions, decalage_instruction, os, false, surligne_inutilisees);
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

void construit_liste_variables_utilisees(Bloc *bloc)
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

Bloc *bloc_pour_label(kuri::tableau<Bloc *, int> &blocs, InstructionLabel *label)
{
    POUR (blocs) {
        if (it->label == label) {
            return it;
        }
    }

    auto bloc = memoire::loge<Bloc>("Bloc");
    bloc->label = label;
    blocs.ajoute(bloc);
    return bloc;
}

/* NOTE: blocs___ est pour rassembler tous les blocs créés et trouver un bloc selon un label pour
 * la création des blocs. À RETRAVAILLER. */
kuri::tableau<Bloc *, int> convertis_en_blocs(AtomeFonction *atome_fonc,
                                              kuri::tableau<Bloc *, int> &blocs___)
{
    kuri::tableau<Bloc *, int> resultat{};

    Bloc *bloc_courant = nullptr;
    auto numero_instruction = atome_fonc->params_entrees.taille();

    POUR (atome_fonc->instructions) {
        it->numero = numero_instruction++;
    }

    POUR (atome_fonc->instructions) {
        if (it->est_label()) {
            bloc_courant = bloc_pour_label(blocs___, it->comme_label());
            resultat.ajoute(bloc_courant);
            continue;
        }

        bloc_courant->instructions.ajoute(it);

        if (it->est_branche()) {
            auto bloc_cible = bloc_pour_label(blocs___, it->comme_branche()->label);
            bloc_courant->ajoute_enfant(bloc_cible);
            continue;
        }

        if (it->est_branche_cond()) {
            auto label_si_vrai = it->comme_branche_cond()->label_si_vrai;
            auto label_si_faux = it->comme_branche_cond()->label_si_faux;

            auto bloc_si_vrai = bloc_pour_label(blocs___, label_si_vrai);
            auto bloc_si_faux = bloc_pour_label(blocs___, label_si_faux);

            bloc_courant->ajoute_enfant(bloc_si_vrai);
            bloc_courant->ajoute_enfant(bloc_si_faux);
            continue;
        }
    }

    return resultat;
}
