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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "impression.hh"

#include "biblinternes/outils/numerique.hh"

#include "compilation/typage.hh"

#include "parsage/identifiant.hh"

#include "instructions.hh"

void imprime_atome(Atome const *atome, std::ostream &os)
{
    if (atome->genre_atome == Atome::Genre::CONSTANTE) {
        auto atome_const = static_cast<AtomeConstante const *>(atome);

        switch (atome_const->genre) {
            case AtomeConstante::Genre::GLOBALE:
            {
                break;
            }
            case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
            {
                auto transtype_const = static_cast<TranstypeConstant const *>(atome_const);
                os << "  transtype ";
                imprime_atome(transtype_const->valeur, os);
                os << " vers " << chaine_type(transtype_const->type) << '\n';
                break;
            }
            case AtomeConstante::Genre::OP_UNAIRE_CONSTANTE:
            {
                break;
            }
            case AtomeConstante::Genre::OP_BINAIRE_CONSTANTE:
            {
                break;
            }
            case AtomeConstante::Genre::ACCES_INDEX_CONSTANT:
            {
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
                        os << valeur_constante->valeur.valeur_reelle;
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
                    {
                        os << "indéfinie";
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
                    {
                        auto type = static_cast<TypeCompose *>(atome->type);
                        auto tableau_valeur = valeur_constante->valeur.valeur_structure.pointeur;

                        auto virgule = "{ ";

                        auto index_membre = 0;

                        POUR (type->membres) {
                            os << virgule;
                            os << it.nom << " = ";
                            imprime_atome(tableau_valeur[index_membre], os);
                            index_membre += 1;
                            virgule = ", ";
                        }

                        os << " }";
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
                    {
                        os << "À FAIRE(tableau fixe) : impression de la valeur pour la RI";
                        break;
                    }
                    case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
                    {
                        os << "À FAIRE(ri) : tableau données constantes";
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

void imprime_instruction(Instruction const *inst, std::ostream &os)
{
    switch (inst->genre) {
        case Instruction::Genre::INVALIDE:
        {
            os << "  invalide\n";
            break;
        }
        case Instruction::Genre::ALLOCATION:
        {
            auto type_pointeur = inst->type->comme_pointeur();
            os << "  alloue " << chaine_type(type_pointeur->type_pointe) << ' ';

            if (inst->ident != nullptr) {
                os << inst->ident->nom << '\n';
            }
            else {
                os << "val" << inst->numero << '\n';
            }

            break;
        }
        case Instruction::Genre::APPEL:
        {
            auto inst_appel = inst->comme_appel();
            os << "  appel " << chaine_type(inst_appel->type) << ' ';
            imprime_atome(inst_appel->appele, os);

            auto virgule = "(";

            POUR (inst_appel->args) {
                os << virgule;
                imprime_atome(it, os);
                virgule = ", ";
            }

            if (inst_appel->args.est_vide()) {
                os << virgule;
            }

            os << ")\n";

            break;
        }
        case Instruction::Genre::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();
            os << "  branche %" << inst_branche->label->numero << "\n";
            break;
        }
        case Instruction::Genre::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            os << "  si ";
            imprime_atome(inst_branche->condition, os);
            os << " alors %" << inst_branche->label_si_vrai->numero << " sinon %"
               << inst_branche->label_si_faux->numero << '\n';
            break;
        }
        case Instruction::Genre::CHARGE_MEMOIRE:
        {
            auto inst_charge = inst->comme_charge();
            auto charge = inst_charge->chargee;

            os << "  charge " << chaine_type(inst->type);

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

            os << '\n';
            break;
        }
        case Instruction::Genre::STOCKE_MEMOIRE:
        {
            auto inst_stocke = inst->comme_stocke_mem();
            auto ou = inst_stocke->ou;

            os << "  stocke " << chaine_type(ou->type);

            if (ou->genre_atome == Atome::Genre::GLOBALE) {
                os << " @globale" << ou;
            }
            else {
                auto inst_chargee = ou->comme_instruction();
                os << " %" << inst_chargee->numero;
            }

            os << ", " << chaine_type(inst_stocke->valeur->type) << ' ';
            imprime_atome(inst_stocke->valeur, os);
            os << '\n';
            break;
        }
        case Instruction::Genre::LABEL:
        {
            auto inst_label = inst->comme_label();
            os << "label " << inst_label->id << '\n';
            break;
        }
        case Instruction::Genre::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();
            os << "  " << chaine_pour_genre_op(inst_un->op) << ' ' << chaine_type(inst_un->type);
            imprime_atome(inst_un->valeur, os);
            os << '\n';
            break;
        }
        case Instruction::Genre::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();
            os << "  " << chaine_pour_genre_op(inst_bin->op) << ' ' << chaine_type(inst_bin->type)
               << ' ';
            imprime_atome(inst_bin->valeur_gauche, os);
            os << ", ";
            imprime_atome(inst_bin->valeur_droite, os);
            os << '\n';
            break;
        }
        case Instruction::Genre::RETOUR:
        {
            auto inst_retour = inst->comme_retour();
            os << "  retourne ";
            if (inst_retour->valeur != nullptr) {
                auto atome = inst_retour->valeur;
                os << chaine_type(atome->type);
                os << ' ';

                imprime_atome(atome, os);
            }
            os << '\n';
            break;
        }
        case Instruction::Genre::ACCEDE_INDEX:
        {
            auto inst_acces = inst->comme_acces_index();
            os << "  index " << chaine_type(inst_acces->type) << ' ';
            imprime_atome(inst_acces->accede, os);
            os << ", ";
            imprime_atome(inst_acces->index, os);
            os << '\n';
            break;
        }
        case Instruction::Genre::ACCEDE_MEMBRE:
        {
            auto inst_acces = inst->comme_acces_membre();
            os << "  membre " << chaine_type(inst_acces->type) << ' ';
            imprime_atome(inst_acces->accede, os);
            os << ", ";
            imprime_atome(inst_acces->index, os);
            os << '\n';
            break;
        }
        case Instruction::Genre::TRANSTYPE:
        {
            auto inst_transtype = inst->comme_transtype();
            os << "  transtype (" << static_cast<int>(inst_transtype->op) << ") ";
            imprime_atome(inst_transtype->valeur, os);
            os << " vers " << chaine_type(inst_transtype->type) << '\n';
            break;
        }
    }
}

void imprime_fonction(AtomeFonction const *atome_fonc,
                      std::ostream &os,
                      bool inclus_nombre_utilisations,
                      bool surligne_inutilisees)
{
    os << "fonction " << atome_fonc->nom;

    auto virgule = "(";

    for (auto param : atome_fonc->params_entrees) {
        os << virgule;
        os << param->ident->nom << ' ';

        auto type_pointeur = param->type->comme_pointeur();
        os << chaine_type(type_pointeur->type_pointe);

        virgule = ", ";
    }

    if (atome_fonc->params_entrees.taille() == 0) {
        os << virgule;
    }

    auto type_fonction = atome_fonc->type->comme_fonction();

    virgule = ") -> ";
    os << chaine_type(type_fonction->type_sortie);
    os << '\n';

    auto numero_instruction = atome_fonc->params_entrees.taille();
    imprime_instructions(atome_fonc->instructions,
                         numero_instruction,
                         os,
                         inclus_nombre_utilisations,
                         surligne_inutilisees);
}

void imprime_instructions(kuri::tableau<Instruction *, int> const &instructions,
                          int numero_de_base,
                          std::ostream &os,
                          bool inclus_nombre_utilisations,
                          bool surligne_inutilisees)
{
    auto numero_instruction = numero_de_base;
    auto max_utilisations = 0;

    POUR (instructions) {
        it->numero = numero_instruction++;
        max_utilisations = std::max(max_utilisations, it->nombre_utilisations);
    }

    using dls::num::nombre_de_chiffres;

    POUR (instructions) {
        if (surligne_inutilisees && it->nombre_utilisations == 0) {
            std::cerr << "\033[0;31m";
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

        imprime_instruction(it, os);

        if (surligne_inutilisees && it->nombre_utilisations == 0) {
            std::cerr << "\033[0m";
        }
    }

    os << '\n';
}
