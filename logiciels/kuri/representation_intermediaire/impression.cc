/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "impression.hh"

#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/typage.hh"

#include "parsage/identifiant.hh"

#include "instructions.hh"

static void imprime_information_atome(Atome const *atome, Enchaineuse &os)
{
    os << atome->genre_atome;

    if (atome->est_globale()) {
        auto globale = atome->comme_globale();
        if (globale->ident) {
            os << " nommée " << globale->ident->nom;
        }
        else {
            os << " anonyme";
        }
    }
    else if (atome->est_fonction()) {
        auto fonction = atome->comme_fonction();
        if (fonction->decl) {
            os << ' ' << nom_humainement_lisible(fonction->decl);
        }
        else {
            os << ' ' << fonction->nom;
        }
    }
    else if (atome->est_instruction()) {
        auto inst = atome->comme_instruction();
        os << " de genre " << inst->genre;
    }

    if (atome->type) {
        os << " de type " << chaine_type(atome->type, OptionsImpressionType::AUCUNE);
    }
}

[[nodiscard]] kuri::chaine imprime_information_atome(Atome const *atome)
{
    Enchaineuse sortie;
    imprime_information_atome(atome, sortie);
    return sortie.chaine();
}

static void imprime_nom_instruction(Instruction const *inst, Enchaineuse &os)
{
    if (inst->est_alloc()) {
        auto alloc = inst->comme_alloc();
        if (alloc->ident) {
            os << "%" << alloc->ident->nom;
            return;
        }
    }

    os << "%" << inst->numero;
}

static void imprime_atome_ex(Atome const *atome,
                             Enchaineuse &os,
                             OptionsImpressionType options,
                             bool pour_operande)
{
    switch (atome->genre_atome) {
        case Atome::Genre::GLOBALE:
        {
            auto globale = atome->comme_globale();
            if (pour_operande) {
                os << chaine_type(globale->type, options) << ' ';
            }
            else {
                os << "globale ";
            }

            if (globale->ident) {
                os << "@" << globale->ident->nom;
            }
            else {
                os << "@globale" << atome;
            }

            if (!pour_operande) {
                os << " = ";

                if (globale->initialisateur) {
                    imprime_atome_ex(globale->initialisateur, os, options, true);
                }
                else {
                    os << chaine_type(type_déréférencé_pour(atome->type), options);
                }

                os << '\n';
            }
            break;
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype_const = atome->comme_transtype_constant();
            if (!pour_operande) {
                os << "  ";
            }
            os << chaine_type(atome->type) << " ";

            os << "transtype ";
            imprime_atome_ex(transtype_const->valeur, os, options, true);
            os << " vers " << chaine_type(transtype_const->type, options);

            if (!pour_operande) {
                os << '\n';
            }
            break;
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            auto acces = static_cast<AccedeIndexConstant const *>(atome);
            imprime_atome_ex(acces->accede, os, options, true);
            os << '[' << acces->index << ']';
            break;
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            os << chaine_type(atome->type, options) << ' '
               << atome->comme_constante_booléenne()->valeur;
            break;
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            os << chaine_type(atome->type, options) << ' '
               << atome->comme_constante_type()->type_de_données->index_dans_table_types;
            break;
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            os << chaine_type(atome->type, options) << ' '
               << atome->comme_constante_entière()->valeur;
            break;
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            os << chaine_type(atome->type, options) << ' '
               << atome->comme_constante_réelle()->valeur;
            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            if (atome->type == TypeBase::PTR_NUL) {
                os << "*nul" << ' ';
            }
            else {
                os << chaine_type(atome->type, options) << ' ';
            }
            os << "nul";
            break;
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            os << chaine_type(atome->type, options) << ' '
               << atome->comme_constante_caractère()->valeur;
            break;
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            auto type_de_données = atome->comme_taille_de()->type_de_données;
            os << chaine_type(atome->type, options) << ' ' << "taille_de("
               << chaine_type(type_de_données, options) << ')';
            break;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure_const = atome->comme_constante_structure();
            auto type = static_cast<TypeCompose const *>(atome->type);
            auto atomes_membres = structure_const->donne_atomes_membres();

            if (pour_operande) {
                os << chaine_type(atome->type, options) << ' ';
            }

            auto virgule = "{ ";

            POUR_INDEX (type->donne_membres_pour_code_machine()) {
                os << virgule;
                os << it.nom->nom << " = ";
                imprime_atome_ex(atomes_membres[index_it], os, options, true);
                virgule = ", ";
            }

            os << " }";
            break;
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            auto données = atome->comme_données_constantes();
            auto tableau_données = données->donne_données();

            os << "données_constantes " << chaine_type(atome->type, options);
            auto virgule = " [ ";

            POUR (tableau_données) {
                auto octet = it;
                os << virgule;
                os << "0x";
                os << dls::num::char_depuis_hex((octet & 0xf0) >> 4);
                os << dls::num::char_depuis_hex(octet & 0x0f);
                virgule = ", ";
            }

            os << ((tableau_données.taille() == 0) ? "[..]" : " ]");
            break;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau_const = atome->comme_constante_tableau();
            auto éléments = tableau_const->donne_atomes_éléments();

            os << chaine_type(atome->type) << " ";

            auto virgule = "[ ";

            POUR (éléments) {
                os << virgule;
                imprime_atome_ex(it, os, options, true);
                virgule = ", ";
            }

            os << ((éléments.taille() == 0) ? "[..]" : " ]");
            break;
        }
        case Atome::Genre::FONCTION:
        {
            auto atome_fonction = atome->comme_fonction();

            if (pour_operande) {
                os << chaine_type(atome_fonction->type, options) << " ";
            }

            os << atome_fonction->nom;
            break;
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            auto const init_tableau = atome->comme_initialisation_tableau();
            os << chaine_type(atome->type, options) << " init_tableau ";
            imprime_atome_ex(init_tableau->valeur, os, options, true);
            break;
        }
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        {
            auto const index_table = atome->comme_index_table_type();
            os << chaine_type(atome->type, options) << " index_de("
               << chaine_type(index_table->type_de_données, options) << ")";
            break;
        }
        case Atome::Genre::NON_INITIALISATION:
        {
            os << chaine_type(atome->type, options) << " ---";
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst_valeur = atome->comme_instruction();
            os << chaine_type(inst_valeur->type, options) << ' ';
            imprime_nom_instruction(inst_valeur, os);
            break;
        }
    }
}

[[nodiscard]] kuri::chaine imprime_atome(Atome const *atome, OptionsImpressionType options)
{
    Enchaineuse sortie;
    imprime_atome_ex(atome, sortie, options, false);
    return sortie.chaine();
}

[[nodiscard]] kuri::chaine imprime_atome(Atome const *atome)
{
    return imprime_atome(atome, OptionsImpressionType::AUCUNE);
}

static void déclare_instruction(Instruction const *inst, Enchaineuse &os)
{
    if (inst->est_label()) {
        return;
    }

    os << "  ";

    if (!inst->type || inst->type->est_type_rien()) {
        return;
    }

    imprime_nom_instruction(inst, os);
    os << " = ";
}

static void imprime_instruction_ex(Instruction const *inst,
                                   Enchaineuse &os,
                                   OptionsImpressionType options)
{
    déclare_instruction(inst, os);

    switch (inst->genre) {
        case GenreInstruction::ALLOCATION:
        {
            auto alloc = inst->comme_alloc();
            os << "alloue " << chaine_type(alloc->donne_type_alloué(), options);
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto inst_appel = inst->comme_appel();
            os << "appel ";
            imprime_atome_ex(inst_appel->appele, os, options, true);

            auto virgule = "(";

            POUR (inst_appel->args) {
                os << virgule;
                imprime_atome_ex(it, os, options, true);
                virgule = ", ";
            }

            if (inst_appel->args.est_vide()) {
                os << virgule;
            }

            os << ")";

            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();
            os << "branche %" << inst_branche->label->numero << '\n';
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            os << "si ";
            imprime_atome_ex(inst_branche->condition, os, options, true);
            os << " alors %" << inst_branche->label_si_vrai->numero << " sinon %"
               << inst_branche->label_si_faux->numero << '\n';
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto inst_charge = inst->comme_charge();
            auto charge = inst_charge->chargee;

            os << "charge ";
            imprime_atome_ex(charge, os, options, true);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();
            auto ou = inst_stocke->ou;

            os << "stocke ";
            imprime_atome_ex(ou, os, options, true);
            os << ", ";
            imprime_atome_ex(inst_stocke->valeur, os, options, true);
            break;
        }
        case GenreInstruction::LABEL:
        {
            auto inst_label = inst->comme_label();
            os << "label " << inst_label->id;
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();
            os << chaine_pour_genre_op(inst_un->op) << ' ';
            imprime_atome_ex(inst_un->valeur, os, options, true);
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();
            os << chaine_pour_genre_op(inst_bin->op) << ' ';
            imprime_atome_ex(inst_bin->valeur_gauche, os, options, true);
            os << ", ";
            imprime_atome_ex(inst_bin->valeur_droite, os, options, true);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto inst_retour = inst->comme_retour();
            os << "retourne";
            if (inst_retour->valeur != nullptr) {
                auto atome = inst_retour->valeur;
                os << ' ';
                imprime_atome_ex(atome, os, options, true);
            }
            os << '\n';
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            auto inst_acces = inst->comme_acces_index();
            os << "index ";
            imprime_atome_ex(inst_acces->accede, os, options, true);
            os << ", ";
            imprime_atome_ex(inst_acces->index, os, options, true);
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto inst_acces = inst->comme_acces_membre();
            os << "membre ";
            imprime_atome_ex(inst_acces->accede, os, options, true);
            os << ", " << inst_acces->index;
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto inst_transtype = inst->comme_transtype();
            os << chaine_pour_type_transtypage(inst_transtype->op) << " ";
            imprime_atome_ex(inst_transtype->valeur, os, options, true);
            os << " vers " << chaine_type(inst_transtype->type, options);
            break;
        }
    }
}

[[nodiscard]] kuri::chaine imprime_instruction(Instruction const *inst,
                                               OptionsImpressionType options)
{
    Enchaineuse sortie;
    imprime_instruction_ex(inst, sortie, options);
    return sortie.chaine();
}

[[nodiscard]] kuri::chaine imprime_instruction(Instruction const *inst)
{
    return imprime_instruction(inst, OptionsImpressionType::AUCUNE);
}

kuri::chaine imprime_arbre_instruction(Instruction const *racine)
{
    kuri::tableau<Instruction const *> instructions;
    visite_atome(const_cast<Instruction *>(racine), [&](Atome *atome) {
        if (atome->est_instruction()) {
            instructions.ajoute(atome->comme_instruction());
        }
    });

    Enchaineuse sortie;
    for (auto i = instructions.taille() - 1; i >= 0; i--) {
        imprime_instruction_ex(instructions[i], sortie, OptionsImpressionType::AUCUNE);
        sortie << '\n';
    }

    return sortie.chaine();
}

void imprime_instructions(kuri::tableau<Instruction *, int> const &instructions,
                          Enchaineuse &os,
                          OptionsImpressionType options,
                          bool surligne_inutilisees,
                          std::function<void(const Instruction &, Enchaineuse &)> rappel)
{
    POUR (instructions) {
        if (surligne_inutilisees && !it->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
            os << "\033[0;31m";
        }

        imprime_instruction_ex(it, os, options);

        if (rappel) {
            rappel(*it, os);
        }

        os << '\n';

        if (surligne_inutilisees && !it->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
            os << "\033[0m";
        }
    }

    os << '\n';
}

[[nodiscard]] kuri::chaine imprime_instructions(
    kuri::tableau<Instruction *, int> const &instructions,
    OptionsImpressionType options,
    bool surligne_inutilisees,
    std::function<void(Instruction const &, Enchaineuse &)> rappel)
{
    Enchaineuse sortie;
    imprime_instructions(instructions, sortie, options, surligne_inutilisees, rappel);
    return sortie.chaine();
}

void imprime_fonction(AtomeFonction const *atome_fonc,
                      Enchaineuse &os,
                      OptionsImpressionType options,
                      bool surligne_inutilisees,
                      std::function<void(const Instruction &, Enchaineuse &)> rappel)
{
    os << "fonction " << atome_fonc->nom;

    auto virgule = "(";

    for (auto param : atome_fonc->params_entrees) {
        os << virgule;
        os << param->ident->nom << ' ';

        auto type_pointeur = param->type->comme_type_pointeur();
        os << chaine_type(type_pointeur->type_pointe, options);

        virgule = ", ";
    }

    if (atome_fonc->params_entrees.taille() == 0) {
        os << virgule;
    }

    auto type_fonction = atome_fonc->type->comme_type_fonction();

    os << ") -> ";
    os << atome_fonc->param_sortie->ident->nom << " ";
    os << chaine_type(type_fonction->type_sortie, options);
    os << '\n';

    atome_fonc->numérote_instructions();

    imprime_instructions(atome_fonc->instructions, os, options, surligne_inutilisees, rappel);
}

[[nodiscard]] kuri::chaine imprime_fonction(
    AtomeFonction const *atome_fonc,
    OptionsImpressionType options,
    bool surligne_inutilisees,
    std::function<void(Instruction const &, Enchaineuse &)> rappel)
{
    Enchaineuse sortie;
    imprime_fonction(atome_fonc, sortie, options, surligne_inutilisees, rappel);
    return sortie.chaine();
}

[[nodiscard]] kuri::chaine imprime_fonction(
    AtomeFonction const *atome_fonc,
    bool surligne_inutilisees,
    std::function<void(Instruction const &, Enchaineuse &)> rappel)
{
    return imprime_fonction(
        atome_fonc, OptionsImpressionType::AUCUNE, surligne_inutilisees, rappel);
}

[[nodiscard]] kuri::chaine imprime_commentaire_instruction(Instruction const *inst)
{
    Enchaineuse sortie;
    if (inst->est_acces_membre()) {
        auto inst_acces = inst->comme_acces_membre();
        sortie << "Nous accédons à ";
        if (inst_acces->accede->est_instruction()) {
            sortie << inst_acces->accede->comme_instruction()->genre << '\n';
        }
        else {
            imprime_information_atome(inst_acces->accede, sortie);
            sortie << '\n';
        }
    }
    else if (inst->est_op_binaire()) {
        auto op_binaire = inst->comme_op_binaire();
        sortie << "Nous opérons entre " << chaine_type(op_binaire->valeur_gauche->type) << " et "
               << chaine_type(op_binaire->valeur_droite->type) << '\n';
    }
    else {
        sortie << "Nous avec avons une instruction de genre " << inst->genre << '\n';
    }
    return sortie.chaine();
}
