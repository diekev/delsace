/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "impression.hh"

#include "biblinternes/outils/numerique.hh"

#include "compilation/typage.hh"

#include "parsage/identifiant.hh"

#include "instructions.hh"

static kuri::chaine_statique chaine_pour_type_transtypage(TypeTranstypage const type)
{
    switch (type) {
        case TypeTranstypage::AUGMENTE_NATUREL:
        {
            return "augmente_naturel";
        }
        case TypeTranstypage::AUGMENTE_RELATIF:
        {
            return "augmente_relatif";
        }
        case TypeTranstypage::AUGMENTE_REEL:
        {
            return "augmente_réel";
        }
        case TypeTranstypage::DIMINUE_NATUREL:
        {
            return "diminue_naturel";
        }
        case TypeTranstypage::DIMINUE_RELATIF:
        {
            return "diminue_relatif";
        }
        case TypeTranstypage::DIMINUE_REEL:
        {
            return "diminue_réel";
        }
        case TypeTranstypage::POINTEUR_VERS_ENTIER:
        {
            return "pointeur_vers_entier";
        }
        case TypeTranstypage::ENTIER_VERS_POINTEUR:
        {
            return "entier_vers_pointeur";
        }
        case TypeTranstypage::REEL_VERS_ENTIER:
        {
            return "réel_vers_entier";
        }
        case TypeTranstypage::ENTIER_VERS_REEL:
        {
            return "entier_vers_réel";
        }
        case TypeTranstypage::BITS:
        case TypeTranstypage::DEFAUT:
        {
            return "transtype_bits";
        }
    }

    return "erreur";
}

void imprime_information_atome(Atome const *atome, std::ostream &os)
{
    switch (atome->genre_atome) {
        case Atome::Genre::GLOBALE:
        {
            auto globale = atome->comme_globale();
            os << "globale " << (globale->ident ? globale->ident->nom : "anonyme") << " de type "
               << chaine_type(atome->type, false);
            break;
        }
        case Atome::Genre::CONSTANTE:
        {
            auto atome_const = static_cast<AtomeConstante const *>(atome);
            os << "constante de type " << chaine_type(atome->type, false);
            switch (atome_const->genre) {
                case AtomeConstante::Genre::GLOBALE:
                {
                    os << " représentant une globale";
                    break;
                }
                case AtomeConstante::Genre::FONCTION:
                {
                    os << " représentant une fonction";
                    break;
                }
                case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
                {
                    os << " représentant un transtypage constant";
                    break;
                }
                case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
                {
                    os << " représentant un indexage constant";
                    break;
                }
                case AtomeConstante::Genre::VALEUR:
                {
                    auto valeur_const = static_cast<AtomeValeurConstante const *>(atome);

                    switch (valeur_const->valeur.genre) {
                        case AtomeValeurConstante::Valeur::Genre::NULLE:
                        {
                            os << " représentant une valeur constante NULLE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::REELLE:
                        {
                            os << " représentant une valeur constante REELLE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::TYPE:
                        {
                            os << " représentant une valeur constante TYPE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::TAILLE_DE:
                        {
                            os << " représentant une valeur constante TAILLE_DE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::ENTIERE:
                        {
                            os << " représentant une valeur constante ENTIERE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
                        {
                            os << " représentant une valeur constante BOOLEENNE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::CARACTERE:
                        {
                            os << " représentant une valeur constante CARACTERE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
                        {
                            os << " représentant une valeur constante INDEFINIE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
                        {
                            os << " représentant une valeur constante STRUCTURE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
                        {
                            os << " représentant une valeur constante TABLEAU_FIXE";
                            break;
                        }
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
                        {
                            os << " représentant une valeur constante TABLEAU_DONNEES_CONSTANTES";
                            break;
                        }
                    }
                }
            }
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            os << "instruction";
            break;
        }
        case Atome::Genre::FONCTION:
        {
            os << "fonction";
            break;
        }
    }
}

static void imprime_atome_ex(Atome const *atome, std::ostream &os, bool pour_operande)
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
    else if (atome->genre_atome == Atome::Genre::CONSTANTE) {
        auto atome_const = static_cast<AtomeConstante const *>(atome);

        switch (atome_const->genre) {
            case AtomeConstante::Genre::GLOBALE:
            {
                break;
            }
            case AtomeConstante::Genre::FONCTION:
            {
                break;
            }
            case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
            {
                auto transtype_const = static_cast<TranstypeConstant const *>(atome_const);
                os << "  transtype ";
                imprime_atome_ex(transtype_const->valeur, os, true);
                os << " vers " << chaine_type(transtype_const->type, false) << '\n';
                break;
            }
            case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
            {
                auto acces = static_cast<AccedeIndexConstant const *>(atome);
                imprime_atome_ex(acces->accede, os, true);
                os << '[';
                imprime_atome_ex(acces->index, os, true);
                os << ']';
                break;
            }
            case AtomeConstante::Genre::VALEUR:
            {
                auto valeur_constante = static_cast<AtomeValeurConstante const *>(atome);

                switch (valeur_constante->valeur.genre) {
                    case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
                    {
                        os << valeur_constante->valeur.valeur_booleenne;
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::TYPE:
                    {
                        os << valeur_constante->valeur.type->index_dans_table_types;
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::ENTIERE:
                    {
                        os << valeur_constante->valeur.valeur_entiere;
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::REELLE:
                    {
                        os << valeur_constante->valeur.valeur_reelle;
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::NULLE:
                    {
                        os << "nul";
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::CARACTERE:
                    {
                        os << valeur_constante->valeur.valeur_entiere;
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
                    {
                        os << "indéfinie";
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::TAILLE_DE:
                    {
                        os << "taille_de(" << chaine_type(valeur_constante->valeur.type, false)
                           << ')';
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
                    {
                        auto type = static_cast<TypeCompose const *>(atome->type);
                        auto tableau_valeur = valeur_constante->valeur.valeur_structure.pointeur;

                        auto virgule = "{ ";

                        auto index_membre = 0;

                        POUR (type->membres) {
                            if (it.ne_doit_pas_être_dans_code_machine()) {
                                continue;
                            }
                            os << virgule;
                            os << it.nom->nom << " = ";
                            imprime_atome_ex(tableau_valeur[index_membre], os, true);
                            index_membre += 1;
                            virgule = ", ";
                        }

                        os << " }";
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
                    {
                        auto pointeur_tableau = valeur_constante->valeur.valeur_tableau.pointeur;
                        auto taille_tableau = valeur_constante->valeur.valeur_tableau.taille;

                        auto virgule = "[ ";

                        for (auto i = 0; i < taille_tableau; ++i) {
                            os << virgule;
                            imprime_atome_ex(pointeur_tableau[i], os, true);
                            virgule = ", ";
                        }

                        os << ((taille_tableau == 0) ? "[]" : " ]");
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
                    {
                        auto pointeur_donnnees = valeur_constante->valeur.valeur_tdc.pointeur;
                        auto taille_donnees = valeur_constante->valeur.valeur_tdc.taille;

                        auto virgule = "[ ";

                        for (auto i = 0; i < taille_donnees; ++i) {
                            auto octet = pointeur_donnnees[i];
                            os << virgule;
                            os << "0x";
                            os << dls::num::char_depuis_hex((octet & 0xf0) >> 4);
                            os << dls::num::char_depuis_hex(octet & 0x0f);
                            virgule = ", ";
                        }

                        os << ((taille_donnees == 0) ? "[]" : " ]");
                        break;
                    }
                }
            }
        }
    }
    else if (atome->genre_atome == Atome::Genre::FONCTION) {
        auto atome_fonction = static_cast<AtomeFonction const *>(atome);
        os << atome_fonction->nom;
    }
    else {
        auto inst_valeur = atome->comme_instruction();
        os << "%" << inst_valeur->numero;
    }
}

void imprime_atome(Atome const *atome, std::ostream &os)
{
    imprime_atome_ex(atome, os, false);
}

void imprime_instruction_ex(Instruction const *inst, std::ostream &os)
{
    switch (inst->genre) {
        case GenreInstruction::INVALIDE:
        {
            os << "  invalide";
            break;
        }
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

            os << "  charge " << chaine_type(inst->type, false);

            if (charge->genre_atome == Atome::Genre::GLOBALE) {
                os << " @globale" << charge;
            }
            else if (charge->genre_atome == Atome::Genre::CONSTANTE) {
                os << " @constante" << charge;
            }
            else {
                auto inst_chargee = charge->comme_instruction();
                os << " %" << inst_chargee->numero;
            }

            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();
            auto ou = inst_stocke->ou;

            os << "  stocke " << chaine_type(ou->type, false);

            if (ou->genre_atome == Atome::Genre::GLOBALE) {
                os << " @globale" << ou;
            }
            else {
                auto inst_chargee = ou->comme_instruction();
                os << " %" << inst_chargee->numero;
            }

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
            os << ", ";
            imprime_atome_ex(inst_acces->index, os, true);
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

void imprime_instruction(Instruction const *inst, std::ostream &os)
{
    imprime_instruction_ex(inst, os);
    os << '\n';
}

void imprime_fonction(AtomeFonction const *atome_fonc,
                      std::ostream &os,
                      bool inclus_nombre_utilisations,
                      bool surligne_inutilisees,
                      std::function<void(const Instruction &, std::ostream &)> rappel)
{
    os << "fonction " << atome_fonc->nom;

    auto virgule = "(";

    for (auto param : atome_fonc->params_entrees) {
        os << virgule;
        os << param->comme_instruction()->comme_alloc()->ident->nom << ' ';

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

    imprime_instructions(
        atome_fonc->instructions, os, inclus_nombre_utilisations, surligne_inutilisees, rappel);
}

int numérote_instructions(AtomeFonction const &fonction)
{
    int resultat = 0;

    POUR (fonction.params_entrees) {
        it->comme_instruction()->numero = resultat++;
    }

    if (!fonction.param_sortie->type->est_type_rien()) {
        fonction.param_sortie->comme_instruction()->numero = resultat++;
    }

    POUR (fonction.instructions) {
        it->numero = resultat++;
    }

    return resultat;
}

void imprime_instructions(kuri::tableau<Instruction *, int> const &instructions,
                          std::ostream &os,
                          bool inclus_nombre_utilisations,
                          bool surligne_inutilisees,
                          std::function<void(const Instruction &, std::ostream &)> rappel)
{
    auto max_utilisations = 0;

    POUR (instructions) {
        max_utilisations = std::max(max_utilisations, it->nombre_utilisations);
    }

    using dls::num::nombre_de_chiffres;

    POUR (instructions) {
        if (surligne_inutilisees && it->nombre_utilisations == 0) {
            os << "\033[0;31m";
        }

        if (inclus_nombre_utilisations) {
            auto nombre_zero_avant_numero = nombre_de_chiffres(max_utilisations) -
                                            nombre_de_chiffres(it->nombre_utilisations);
            os << '(';

            for (auto i = 0; i < nombre_zero_avant_numero; ++i) {
                os << ' ';
            }

            os << it->nombre_utilisations << ") ";
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

        if (surligne_inutilisees && it->nombre_utilisations == 0) {
            os << "\033[0m";
        }
    }

    os << '\n';
}
