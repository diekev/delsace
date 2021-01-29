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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "coulisse_asm.hh"

#include "biblinternes/outils/enchaineuse.hh"

#include "broyage.hh"
#include "compilatrice.hh"
#include "erreur.h"
#include "typage.hh"

static constexpr const char *RSP = "rsp";

struct GeneratriceCodeASM {
    dls::dico<Atome const *, dls::chaine> table_valeurs{};
    dls::dico<Atome const *, dls::chaine> table_globales{};
    EspaceDeTravail &m_espace;
    AtomeFonction const *m_fonction_courante = nullptr;

    // les atomes pour les chaines peuvent être générés plusieurs fois (notamment
    // pour celles des noms des fonctions pour les traces d'appel), utilisons un
    // index pour les rendre uniques
    int index_chaine = 0;

	int taille_allouee = 0;

    GeneratriceCodeASM(EspaceDeTravail &espace);

    COPIE_CONSTRUCT(GeneratriceCodeASM);

    dls::chaine genere_code_pour_atome(Atome *atome, Enchaineuse &os, bool pour_globale);

    void genere_code_pour_instruction(Instruction const *inst, Enchaineuse &os);

    void genere_code(tableau_page<AtomeGlobale> const &globales, kuri::tableau<AtomeFonction *> const &fonctions, Enchaineuse &os);
};

GeneratriceCodeASM::GeneratriceCodeASM(EspaceDeTravail &espace)
    : m_espace(espace)
{}

dls::chaine GeneratriceCodeASM::genere_code_pour_atome(Atome *atome, Enchaineuse &os, bool pour_globale)
{
    switch (atome->genre_atome) {
        case Atome::Genre::FONCTION:
        {
            auto atome_fonc = static_cast<AtomeFonction const *>(atome);
            return atome_fonc->nom;
        }
        case Atome::Genre::CONSTANTE:
        {
            auto atome_const = static_cast<AtomeConstante const *>(atome);

            switch (atome_const->genre) {
                case AtomeConstante::Genre::GLOBALE:
                {
                    return "";
                }
                case AtomeConstante::Genre::TRANSTYPE_CONSTANT:
                {
                    return "";
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
                    return "";
                }
                case AtomeConstante::Genre::VALEUR:
                {
                    auto valeur_const = static_cast<AtomeValeurConstante const *>(atome);

                    switch (valeur_const->valeur.genre) {
                        case AtomeValeurConstante::Valeur::Genre::NULLE:
                        {
                            return "";
                        }
                        case AtomeValeurConstante::Valeur::Genre::TYPE:
                        {
                            return "";
                        }
                        case AtomeValeurConstante::Valeur::Genre::REELLE:
                        {
                            return "";
                        }
                        case AtomeValeurConstante::Valeur::Genre::ENTIERE:
                        {
							return dls::vers_chaine(valeur_const->valeur.valeur_entiere);
                        }
                        case AtomeValeurConstante::Valeur::Genre::BOOLEENNE:
                        {
                            return dls::vers_chaine(valeur_const->valeur.valeur_booleenne);
                        }
                        case AtomeValeurConstante::Valeur::Genre::CARACTERE:
                        {
                            return dls::vers_chaine(valeur_const->valeur.valeur_entiere);
                        }
                        case AtomeValeurConstante::Valeur::Genre::INDEFINIE:
                        {
                            return "";
                        }
                        case AtomeValeurConstante::Valeur::Genre::STRUCTURE:
                        {
                            return "";
                        }
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE:
                        {
                            return "";
                        }
                        case AtomeValeurConstante::Valeur::Genre::TABLEAU_DONNEES_CONSTANTES:
                        {
                            return "";
                        }
                    }
                }
            }

            return "";
        }
        case Atome::Genre::INSTRUCTION:
        {
            return "";
        }
        case Atome::Genre::GLOBALE:
        {
            return "";
        }
    }

    return "";
}

void GeneratriceCodeASM::genere_code_pour_instruction(const Instruction *inst, Enchaineuse &os)
{
    switch (inst->genre) {
        case Instruction::Genre::INVALIDE:
        {
            break;
        }
        case Instruction::Genre::ALLOCATION:
        {
			/* il faut faire de la place sur la pile
			 * @Incomplet : vérifie l'alignement.
			 * @Incomplet : fusionne plusieurs telles instructions.
			 * @Incomplet : assigne l'adresse à l'atome */
			auto type_pointeur = inst->type->comme_pointeur();
			taille_allouee += type_pointeur->type_pointe->taille_octet;
			os << "  sub " << RSP << ' ' << type_pointeur->type_pointe->taille_octet << '\n';
            break;
        }
        case Instruction::Genre::APPEL:
        {
            auto appel = inst->comme_appel();

			/* @Incomplet: chargement des paramètres dans les registres */
            POUR (appel->args) {
                genere_code_pour_atome(it, os, false);
            }

            os << "  call " << genere_code_pour_atome(appel->appele, os, false) << '\n';
            break;
        }
        case Instruction::Genre::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();
            os << "  jmp label" << inst_branche->label->id << '\n';
            break;
        }
        case Instruction::Genre::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            os << "  jmpz label" << inst_branche->label_si_faux->id << '\n';
            os << "  jmp label" << inst_branche->label_si_vrai->id << '\n';
            break;
        }
        case Instruction::Genre::CHARGE_MEMOIRE:
        {
			/* @Incomplet: charge depuis où? */
			os << "  mov \n";
            break;
        }
        case Instruction::Genre::STOCKE_MEMOIRE:
        {
			/* @Incomplet: met où? */
			os << "  mov \n";
            break;
        }
        case Instruction::Genre::LABEL:
        {
            auto inst_label = inst->comme_label();
            os << "\nlabel" << inst_label->id << ":\n";
            break;
        }
        case Instruction::Genre::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();

            switch (inst_un->op) {
                case OperateurUnaire::Genre::Positif:
                {
					genere_code_pour_atome(inst_un->valeur, os, false);
					break;
                }
                case OperateurUnaire::Genre::Invalide:
                {
                    break;
                }
                case OperateurUnaire::Genre::Complement:
                {
                    break;
                }
                case OperateurUnaire::Genre::Non_Binaire:
                {
                    break;
                }
                case OperateurUnaire::Genre::Non_Logique:
                {
                    break;
                }
                case OperateurUnaire::Genre::Prise_Adresse:
                {
                    break;
                }
            }

            break;
        }
        case Instruction::Genre::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();

			/* @Incomplet: charge dans les registres */
            switch (inst_bin->op) {
                case OperateurBinaire::Genre::Addition:
                case OperateurBinaire::Genre::Addition_Reel:
                {
					os << "  add \n";
                    break;
                }
                case OperateurBinaire::Genre::Soustraction:
                case OperateurBinaire::Genre::Soustraction_Reel:
                {
					os << "  sub \n";
                    break;
                }
                case OperateurBinaire::Genre::Multiplication:
                case OperateurBinaire::Genre::Multiplication_Reel:
                {
					os << "  mul \n";
                    break;
                }
                case OperateurBinaire::Genre::Division_Naturel:
                case OperateurBinaire::Genre::Division_Relatif:
                case OperateurBinaire::Genre::Division_Reel:
                {
					os << "  div \n";
                    break;
                }
                case OperateurBinaire::Genre::Reste_Naturel:
                case OperateurBinaire::Genre::Reste_Relatif:
                {
                    break;
                }
                case OperateurBinaire::Genre::Comp_Egal:
                case OperateurBinaire::Genre::Comp_Egal_Reel:
                {
                    break;
                }
                case OperateurBinaire::Genre::Comp_Inegal:
                case OperateurBinaire::Genre::Comp_Inegal_Reel:
                {
                    break;
                }
                case OperateurBinaire::Genre::Comp_Inf:
                case OperateurBinaire::Genre::Comp_Inf_Nat:
                case OperateurBinaire::Genre::Comp_Inf_Reel:
                {
                    break;
                }
                case OperateurBinaire::Genre::Comp_Inf_Egal:
                case OperateurBinaire::Genre::Comp_Inf_Egal_Nat:
                case OperateurBinaire::Genre::Comp_Inf_Egal_Reel:
                {
                    break;
                }
                case OperateurBinaire::Genre::Comp_Sup:
                case OperateurBinaire::Genre::Comp_Sup_Nat:
                case OperateurBinaire::Genre::Comp_Sup_Reel:
                {
                    break;
                }
                case OperateurBinaire::Genre::Comp_Sup_Egal:
                case OperateurBinaire::Genre::Comp_Sup_Egal_Nat:
                case OperateurBinaire::Genre::Comp_Sup_Egal_Reel:
                {
                    break;
                }
                case OperateurBinaire::Genre::Et_Binaire:
                {
                    break;
                }
                case OperateurBinaire::Genre::Ou_Binaire:
                {
                    break;
                }
                case OperateurBinaire::Genre::Ou_Exclusif:
                {
                    break;
                }
                case OperateurBinaire::Genre::Dec_Gauche:
                {
                    break;
                }
                case OperateurBinaire::Genre::Dec_Droite_Arithm:
                case OperateurBinaire::Genre::Dec_Droite_Logique:
                {
                    break;
                }
                case OperateurBinaire::Genre::Invalide:
                {
                    break;
                }
            }

            break;
        }
        case Instruction::Genre::RETOUR:
        {
            auto inst_retour = inst->comme_retour();

			/* @Incomplet: restore la pile */
			os << "  add " << RSP << ' ' << taille_allouee << '\n';

			os << "  ret ";

			if (inst_retour->valeur != nullptr) {
				os << genere_code_pour_atome(inst_retour->valeur, os, false);
            }

			os << "\n";
            break;
        }
        case Instruction::Genre::ACCEDE_INDEX:
        {
			/* @Incomplet: [ptr + décalage] */
            break;
        }
        case Instruction::Genre::ACCEDE_MEMBRE:
        {
			/* @Incomplet: [ptr + décalage] */
            break;
        }
        case Instruction::Genre::TRANSTYPE:
        {
			/* @Incomplet: les types de transtypage */
            break;
        }
    }
}

void GeneratriceCodeASM::genere_code(const tableau_page<AtomeGlobale> &globales, const kuri::tableau<AtomeFonction *> &fonctions, Enchaineuse &os)
{
    // prédéclare les globales pour éviter les problèmes de références cycliques
    POUR_TABLEAU_PAGE (globales) {
//        auto valeur_globale = &it;

//        if (!valeur_globale->est_constante) {
//            continue;
//        }

//        auto type = valeur_globale->type->comme_pointeur()->type_pointe;

//        os << "static const " << nom_broye_type(type) << ' ';

//        if (valeur_globale->ident) {
//            auto nom_globale = broye_nom_simple(valeur_globale->ident->nom);
//            os << nom_globale;
//            table_globales[valeur_globale] = "&" + broye_nom_simple(nom_globale);
//        }
//        else {
//            auto nom_globale = "globale" + dls::vers_chaine(valeur_globale);
//            os << nom_globale;
//            table_globales[valeur_globale] = "&" + broye_nom_simple(nom_globale);
//        }

//        os << ";\n";
    }

    os << "section .text\n";

    // prédéclare ensuite les fonction pour éviter les problèmes de
    // dépendances cycliques, mais aussi pour prendre en compte les cas où
    // les globales utilises des fonctions dans leurs initialisations
    POUR (fonctions) {
        if (it->nombre_utilisations == 0) {
            continue;
        }

        if (it->est_externe) {
            os << "extern " << it->nom << "\n";
        }
        else {
            os << "global " << it->nom << "\n";
        }
    }

    // définis ensuite les globales
//    POUR_TABLEAU_PAGE (globales) {
//    }

    // définis enfin les fonction
    POUR (fonctions) {
        if (it->nombre_utilisations == 0) {
            continue;
        }

        if (it->est_externe) {
            continue;
        }

//        if (!it->sanstrace) {
//            os << "INITIALISE_TRACE_APPEL(\"";

//            if (it->lexeme != nullptr) {
//                auto fichier = m_espace.fichier(it->lexeme->fichier);
//                os << it->lexeme->chaine << "\", "
//                   << it->lexeme->chaine.taille() << ", \""
//                   << fichier->nom << ".kuri\", "
//                   << fichier->nom.taille() + 5 << ", ";
//            }
//            else {
//                os << it->nom << "\", "
//                   << it->nom.taille() << ", "
//                   << "\"???\", 3, ";
//            }

//            os << it->nom << ");\n";
//        }

        os << it->nom << ":\n";
        m_fonction_courante = it;
		taille_allouee = 0;

        auto numero_inst = static_cast<int>(it->params_entrees.taille);

        for (auto inst : it->instructions) {
            inst->numero = numero_inst++;
            genere_code_pour_instruction(inst, os);
        }

        m_fonction_courante = nullptr;
        os << "\n";
    }
}

bool CoulisseASM::cree_fichier_objet(Compilatrice &/*compilatrice*/, EspaceDeTravail &espace, ConstructriceRI &constructrice_ri)
{
	std::ostream &fichier_sortie = std::cerr;
	Enchaineuse enchaineuse;

	espace.typeuse.construit_table_types();

	if (espace.fonction_principale == nullptr) {
		erreur::fonction_principale_manquante(espace);
	}

	auto fonction_principale = espace.fonction_principale->noeud_dependance;

	//genere_code_debut_fichier(enchaineuse, compilatrice.racine_kuri);

	//genere_code_pour_types(compilatrice, graphe, enchaineuse);

	dls::ensemble<AtomeFonction *> utilises;
	kuri::tableau<AtomeFonction *> fonctions;
	auto &graphe = espace.graphe_dependance;
	graphe->rassemble_fonctions_utilisees(fonction_principale, fonctions, utilises);

	// génère finalement la fonction __principale qui sers de pont entre __point_d_entree_systeme et principale
	auto atome_principale = constructrice_ri.genere_ri_pour_fonction_principale(&espace);
	fonctions.ajoute(atome_principale);

	auto generatrice = GeneratriceCodeASM(espace);
	generatrice.genere_code(espace.globales, fonctions, enchaineuse);

	enchaineuse.imprime_dans_flux(fichier_sortie);

	return true;
}

bool CoulisseASM::cree_executable(Compilatrice &/*compilatrice*/, EspaceDeTravail &/*espace*/)
{
	return false;
}
