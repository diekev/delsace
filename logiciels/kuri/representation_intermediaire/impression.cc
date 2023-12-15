/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "impression.hh"

#include <iostream>

#include "biblinternes/outils/numerique.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/typage.hh"

#include "parsage/identifiant.hh"

#include "instructions.hh"

static kuri::chaine_statique chaine_pour_type_transtypage(TypeTranstypage const type)
{
#define ENUMERE_TYPE_TRANSTYPAGE_EX(genre, ident)                                                 \
    case TypeTranstypage::genre:                                                                  \
    {                                                                                             \
        return #ident;                                                                            \
    }

    switch (type) {
        ENUMERE_TYPE_TRANSTYPAGE(ENUMERE_TYPE_TRANSTYPAGE_EX)
    }

#undef ENUMERE_TYPE_TRANSTYPAGE_EX
    return "erreur";
}

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
        os << " de type " << chaine_type(atome->type, false);
    }
}

[[nodiscard]] kuri::chaine imprime_information_atome(Atome const *atome)
{
    Enchaineuse sortie;
    imprime_information_atome(atome, sortie);
    return sortie.chaine();
}

static void imprime_atome_ex(Atome const *atome, Enchaineuse &os, bool pour_operande)
{
    if (atome->genre_atome == Atome::Genre::GLOBALE) {
        auto globale = atome->comme_globale();
        if (globale->ident) {
            os << "@" << globale->ident->nom;
        }
        else {
            os << "@globale" << atome;
        }

        if (!pour_operande) {
            os << " = globale " << chaine_type(type_dereference_pour(atome->type), false);

            if (globale->initialisateur) {
                os << ' ';
                imprime_atome_ex(globale->initialisateur, os, true);
            }
            os << '\n';
        }
    }
    else if (atome->genre_atome == Atome::Genre::TRANSTYPE_CONSTANT) {
        auto transtype_const = atome->comme_transtype_constant();
        if (!pour_operande) {
            os << "  ";
        }
        os << "transtype ";
        imprime_atome_ex(transtype_const->valeur, os, true);
        os << " vers " << chaine_type(transtype_const->type, false);

        if (!pour_operande) {
            os << '\n';
        }
    }
    else if (atome->genre_atome == Atome::Genre::ACCÈS_INDEX_CONSTANT) {
        auto acces = static_cast<AccedeIndexConstant const *>(atome);
        imprime_atome_ex(acces->accede, os, true);
        os << '[' << acces->index << ']';
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_BOOLÉENNE) {
        os << atome->comme_constante_booléenne()->valeur;
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_TYPE) {
        os << atome->comme_constante_type()->type_de_données->index_dans_table_types;
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_ENTIÈRE) {
        os << atome->comme_constante_entière()->valeur;
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_RÉELLE) {
        os << atome->comme_constante_réelle()->valeur;
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_NULLE) {
        os << "nul";
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_CARACTÈRE) {
        os << atome->comme_constante_caractère()->valeur;
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_TAILLE_DE) {
        auto type_de_données = atome->comme_taille_de()->type_de_données;
        os << "taille_de(" << chaine_type(type_de_données, false) << ')';
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_STRUCTURE) {
        auto structure_const = atome->comme_constante_structure();
        auto type = static_cast<TypeCompose const *>(atome->type);
        auto atomes_membres = structure_const->donne_atomes_membres();

        auto virgule = "{ ";

        POUR_INDEX (type->donne_membres_pour_code_machine()) {
            os << virgule;
            os << it.nom->nom << " = ";
            imprime_atome_ex(atomes_membres[index_it], os, true);
            virgule = ", ";
        }

        os << " }";
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES) {
        auto données = atome->comme_données_constantes();
        auto tableau_données = données->donne_données();

        auto virgule = "[ ";

        POUR (tableau_données) {
            auto octet = it;
            os << virgule;
            os << "0x";
            os << dls::num::char_depuis_hex((octet & 0xf0) >> 4);
            os << dls::num::char_depuis_hex(octet & 0x0f);
            virgule = ", ";
        }

        os << ((tableau_données.taille() == 0) ? "[]" : " ]");
    }
    else if (atome->genre_atome == Atome::Genre::CONSTANTE_TABLEAU_FIXE) {
        auto tableau_const = atome->comme_constante_tableau();
        auto éléments = tableau_const->donne_atomes_éléments();

        auto virgule = "[ ";

        POUR (éléments) {
            os << virgule;
            imprime_atome_ex(it, os, true);
            virgule = ", ";
        }

        os << ((éléments.taille() == 0) ? "[]" : " ]");
    }
    else if (atome->genre_atome == Atome::Genre::FONCTION) {
        auto atome_fonction = atome->comme_fonction();
        os << atome_fonction->nom;
    }
    else {
        auto inst_valeur = atome->comme_instruction();
        os << "%" << inst_valeur->numero;
    }
}

[[nodiscard]] kuri::chaine imprime_atome(Atome const *atome)
{
    Enchaineuse sortie;
    imprime_atome_ex(atome, sortie, false);
    return sortie.chaine();
}

static void imprime_instruction_ex(Instruction const *inst, Enchaineuse &os)
{
    switch (inst->genre) {
        case GenreInstruction::ALLOCATION:
        {
            auto alloc = inst->comme_alloc();
            auto type_pointeur = inst->type->comme_type_pointeur();
            os << "  alloue " << chaine_type(type_pointeur->type_pointe, false) << ' ';

            if (alloc->ident != nullptr) {
                os << alloc->ident->nom;
            }
            else {
                os << "val" << inst->numero;
            }

            break;
        }
        case GenreInstruction::APPEL:
        {
            auto inst_appel = inst->comme_appel();
            os << "  appel " << chaine_type(inst_appel->type, false) << ' ';
            imprime_atome_ex(inst_appel->appele, os, true);

            auto virgule = "(";

            POUR (inst_appel->args) {
                os << virgule;
                imprime_atome_ex(it, os, true);
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
            os << "  branche %" << inst_branche->label->numero;
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            os << "  si ";
            imprime_atome_ex(inst_branche->condition, os, true);
            os << " alors %" << inst_branche->label_si_vrai->numero << " sinon %"
               << inst_branche->label_si_faux->numero;
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto inst_charge = inst->comme_charge();
            auto charge = inst_charge->chargee;

            os << "  charge " << chaine_type(inst->type, false) << ' ';
            imprime_atome_ex(charge, os, true);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();
            auto ou = inst_stocke->ou;

            os << "  stocke " << chaine_type(ou->type, false) << ' ';
            imprime_atome_ex(ou, os, true);
            os << ", " << chaine_type(inst_stocke->valeur->type, false) << ' ';
            imprime_atome_ex(inst_stocke->valeur, os, true);
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
            os << "  " << chaine_pour_genre_op(inst_un->op) << ' '
               << chaine_type(inst_un->type, false) << ' ';
            imprime_atome_ex(inst_un->valeur, os, true);
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();
            os << "  " << chaine_pour_genre_op(inst_bin->op) << ' '
               << chaine_type(inst_bin->type, false) << ' ';
            imprime_atome_ex(inst_bin->valeur_gauche, os, true);
            os << ", ";
            imprime_atome_ex(inst_bin->valeur_droite, os, true);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto inst_retour = inst->comme_retour();
            os << "  retourne ";
            if (inst_retour->valeur != nullptr) {
                auto atome = inst_retour->valeur;
                os << chaine_type(atome->type, false);
                os << ' ';

                imprime_atome_ex(atome, os, true);
            }
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            auto inst_acces = inst->comme_acces_index();
            os << "  index " << chaine_type(inst_acces->type, false) << ' ';
            imprime_atome_ex(inst_acces->accede, os, true);
            os << ", ";
            imprime_atome_ex(inst_acces->index, os, true);
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto inst_acces = inst->comme_acces_membre();
            os << "  membre " << chaine_type(inst_acces->type, false) << ' ';
            imprime_atome_ex(inst_acces->accede, os, true);
            os << ", " << inst_acces->index;
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto inst_transtype = inst->comme_transtype();
            os << "  " << chaine_pour_type_transtypage(inst_transtype->op) << " ";
            imprime_atome_ex(inst_transtype->valeur, os, true);
            os << " vers " << chaine_type(inst_transtype->type, false);
            break;
        }
    }
}

[[nodiscard]] kuri::chaine imprime_instruction(Instruction const *inst)
{
    Enchaineuse sortie;
    imprime_instruction_ex(inst, sortie);
    sortie << '\n';
    return sortie.chaine();
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
        imprime_instruction_ex(instructions[i], sortie);
        sortie << '\n';
    }

    return sortie.chaine();
}

int numérote_instructions(AtomeFonction const &fonction)
{
    int résultat = 0;

    POUR (fonction.params_entrees) {
        it->numero = résultat++;
    }

    if (!fonction.param_sortie->type->est_type_rien()) {
        fonction.param_sortie->numero = résultat++;

        auto decl = fonction.decl;
        if (decl && decl->params_sorties.taille() > 1) {
            POUR (decl->params_sorties) {
                auto inst = it->comme_declaration_variable()->atome->comme_instruction();
                inst->numero = résultat++;
            }
        }
    }

    POUR (fonction.instructions) {
        it->numero = résultat++;
    }

    return résultat;
}

void imprime_instructions(kuri::tableau<Instruction *, int> const &instructions,
                          Enchaineuse &os,
                          bool surligne_inutilisees,
                          std::function<void(const Instruction &, Enchaineuse &)> rappel)
{
    using dls::num::nombre_de_chiffres;

    POUR (instructions) {
        if (surligne_inutilisees && !it->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
            os << "\033[0;31m";
        }

        auto nombre_zero_avant_numero = nombre_de_chiffres(instructions.taille()) -
                                        nombre_de_chiffres(it->numero);

        for (auto i = 0; i < nombre_zero_avant_numero; ++i) {
            os << ' ';
        }

        os << "%" << it->numero << ' ';

        imprime_instruction_ex(it, os);

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
    bool surligne_inutilisees,
    std::function<void(Instruction const &, Enchaineuse &)> rappel)
{
    Enchaineuse sortie;
    imprime_instructions(instructions, sortie, surligne_inutilisees, rappel);
    return sortie.chaine();
}

void imprime_fonction(AtomeFonction const *atome_fonc,
                      Enchaineuse &os,
                      bool surligne_inutilisees,
                      std::function<void(const Instruction &, Enchaineuse &)> rappel)
{
    os << "fonction " << atome_fonc->nom;

    auto virgule = "(";

    for (auto param : atome_fonc->params_entrees) {
        os << virgule;
        os << param->ident->nom << ' ';

        auto type_pointeur = param->type->comme_type_pointeur();
        os << chaine_type(type_pointeur->type_pointe, false);

        virgule = ", ";
    }

    if (atome_fonc->params_entrees.taille() == 0) {
        os << virgule;
    }

    auto type_fonction = atome_fonc->type->comme_type_fonction();

    os << ") -> ";
    os << chaine_type(type_fonction->type_sortie, false);
    os << '\n';

    numérote_instructions(*atome_fonc);

    imprime_instructions(atome_fonc->instructions, os, surligne_inutilisees, rappel);
}

[[nodiscard]] kuri::chaine imprime_fonction(
    AtomeFonction const *atome_fonc,
    bool surligne_inutilisees,
    std::function<void(Instruction const &, Enchaineuse &)> rappel)
{
    Enchaineuse sortie;
    imprime_fonction(atome_fonc, sortie, surligne_inutilisees, rappel);
    return sortie.chaine();
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
