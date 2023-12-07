/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "coulisse_asm.hh"

#include <iostream>

#include "structures/enchaineuse.hh"
#include "structures/table_hachage.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/instructions.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "broyage.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "typage.hh"

static constexpr const char *RSP = "rsp";

struct GeneratriceCodeASM {
    kuri::table_hachage<Atome const *, kuri::chaine> table_valeurs{"Valeurs locales ASM"};
    kuri::table_hachage<Atome const *, kuri::chaine> table_globales{"Valeurs globales ASM"};
    EspaceDeTravail &m_espace;
    AtomeFonction const *m_fonction_courante = nullptr;

    // les atomes pour les chaines peuvent être générés plusieurs fois (notamment
    // pour celles des noms des fonctions pour les traces d'appel), utilisons un
    // index pour les rendre uniques
    int index_chaine = 0;

    int taille_allouee = 0;

    GeneratriceCodeASM(EspaceDeTravail &espace);

    EMPECHE_COPIE(GeneratriceCodeASM);

    kuri::chaine genere_code_pour_atome(Atome *atome, Enchaineuse &os, bool pour_globale);

    void genere_code_pour_instruction(Instruction const *inst, Enchaineuse &os);

    void genere_code(kuri::tableau_statique<AtomeGlobale *> globales,
                     kuri::tableau_statique<AtomeFonction *> fonctions,
                     Enchaineuse &os);
};

GeneratriceCodeASM::GeneratriceCodeASM(EspaceDeTravail &espace) : m_espace(espace)
{
}

kuri::chaine GeneratriceCodeASM::genere_code_pour_atome(Atome *atome,
                                                        Enchaineuse &os,
                                                        bool pour_globale)
{
    switch (atome->genre_atome) {
        case Atome::Genre::FONCTION:
        {
            auto atome_fonc = atome->comme_fonction();
            return atome_fonc->nom;
        }
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            return "";
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            return "";
        }
        case Atome::Genre::CONSTANTE_NULLE:
        {
            return "0";
        }
        case Atome::Genre::CONSTANTE_TYPE:
        {
            return "";
        }
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        {
            return "";
        }
        case Atome::Genre::CONSTANTE_INDEX_TABLE_TYPE:
        {
            return "";
        }
        case Atome::Genre::CONSTANTE_RÉELLE:
        {
            auto constante_réelle = atome->comme_constante_réelle();
            return enchaine(constante_réelle->valeur);
        }
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        {
            auto constante_entière = atome->comme_constante_entière();
            return enchaine(constante_entière->valeur);
        }
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        {
            auto constante_booléenne = atome->comme_constante_booléenne();
            return enchaine(constante_booléenne->valeur);
        }
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        {
            auto caractère = atome->comme_constante_caractère();
            return enchaine(caractère->valeur);
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            return "";
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            return "";
        }
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        {
            return "";
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            return "";
        }
        case Atome::Genre::NON_INITIALISATION:
        {
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
        case GenreInstruction::INVALIDE:
        {
            break;
        }
        case GenreInstruction::ALLOCATION:
        {
            /* il faut faire de la place sur la pile
             * @Incomplet : vérifie l'alignement.
             * @Incomplet : fusionne plusieurs telles instructions.
             * @Incomplet : assigne l'adresse à l'atome */
            auto type_pointeur = inst->type->comme_type_pointeur();
            taille_allouee += static_cast<int>(type_pointeur->type_pointe->taille_octet);
            os << "  sub " << RSP << ' ' << type_pointeur->type_pointe->taille_octet << '\n';
            break;
        }
        case GenreInstruction::APPEL:
        {
            auto appel = inst->comme_appel();

            /* @Incomplet: chargement des paramètres dans les registres */
            POUR (appel->args) {
                genere_code_pour_atome(it, os, false);
            }

            os << "  call " << genere_code_pour_atome(appel->appele, os, false) << '\n';
            break;
        }
        case GenreInstruction::BRANCHE:
        {
            auto inst_branche = inst->comme_branche();
            os << "  jmp label" << inst_branche->label->id << '\n';
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto inst_branche = inst->comme_branche_cond();
            os << "  jmpz label" << inst_branche->label_si_faux->id << '\n';
            os << "  jmp label" << inst_branche->label_si_vrai->id << '\n';
            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            /* @Incomplet: charge depuis où? */
            os << "  mov \n";
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            /* @Incomplet: met où? */
            os << "  mov \n";
            break;
        }
        case GenreInstruction::LABEL:
        {
            auto inst_label = inst->comme_label();
            os << "\nlabel" << inst_label->id << ":\n";
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto inst_un = inst->comme_op_unaire();

            switch (inst_un->op) {
                case OpérateurUnaire::Genre::Positif:
                {
                    genere_code_pour_atome(inst_un->valeur, os, false);
                    break;
                }
                case OpérateurUnaire::Genre::Invalide:
                {
                    break;
                }
                case OpérateurUnaire::Genre::Complement:
                {
                    break;
                }
                case OpérateurUnaire::Genre::Non_Binaire:
                {
                    break;
                }
            }

            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto inst_bin = inst->comme_op_binaire();

            /* @Incomplet: charge dans les registres */
            switch (inst_bin->op) {
                case OpérateurBinaire::Genre::Addition:
                case OpérateurBinaire::Genre::Addition_Reel:
                {
                    os << "  add \n";
                    break;
                }
                case OpérateurBinaire::Genre::Soustraction:
                case OpérateurBinaire::Genre::Soustraction_Reel:
                {
                    os << "  sub \n";
                    break;
                }
                case OpérateurBinaire::Genre::Multiplication:
                case OpérateurBinaire::Genre::Multiplication_Reel:
                {
                    os << "  mul \n";
                    break;
                }
                case OpérateurBinaire::Genre::Division_Naturel:
                case OpérateurBinaire::Genre::Division_Relatif:
                case OpérateurBinaire::Genre::Division_Reel:
                {
                    os << "  div \n";
                    break;
                }
                case OpérateurBinaire::Genre::Reste_Naturel:
                case OpérateurBinaire::Genre::Reste_Relatif:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Egal:
                case OpérateurBinaire::Genre::Comp_Egal_Reel:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Inegal:
                case OpérateurBinaire::Genre::Comp_Inegal_Reel:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Inf:
                case OpérateurBinaire::Genre::Comp_Inf_Nat:
                case OpérateurBinaire::Genre::Comp_Inf_Reel:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Inf_Egal:
                case OpérateurBinaire::Genre::Comp_Inf_Egal_Nat:
                case OpérateurBinaire::Genre::Comp_Inf_Egal_Reel:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Sup:
                case OpérateurBinaire::Genre::Comp_Sup_Nat:
                case OpérateurBinaire::Genre::Comp_Sup_Reel:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Comp_Sup_Egal:
                case OpérateurBinaire::Genre::Comp_Sup_Egal_Nat:
                case OpérateurBinaire::Genre::Comp_Sup_Egal_Reel:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Et_Binaire:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Ou_Binaire:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Ou_Exclusif:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Dec_Gauche:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Dec_Droite_Arithm:
                case OpérateurBinaire::Genre::Dec_Droite_Logique:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Indexage:
                {
                    break;
                }
                case OpérateurBinaire::Genre::Invalide:
                {
                    break;
                }
            }

            break;
        }
        case GenreInstruction::RETOUR:
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
        case GenreInstruction::ACCEDE_INDEX:
        {
            /* @Incomplet: [ptr + décalage] */
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            /* @Incomplet: [ptr + décalage] */
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            /* @Incomplet: les types de transtypage */
            break;
        }
    }
}

void GeneratriceCodeASM::genere_code(kuri::tableau_statique<AtomeGlobale *> globales,
                                     kuri::tableau_statique<AtomeFonction *> fonctions,
                                     Enchaineuse &os)
{
    // prédéclare les globales pour éviter les problèmes de références cycliques
    //    POUR_TABLEAU_PAGE (globales) {
    //        auto valeur_globale = &it;

    //        if (!valeur_globale->est_constante) {
    //            continue;
    //        }

    //        auto type = valeur_globale->donne_type_alloué();

    //        os << "static const " << nom_broye_type(type) << ' ';

    //        if (valeur_globale->ident) {
    //            auto nom_globale = broye_nom_simple(valeur_globale->ident);
    //            os << nom_globale;
    //            table_globales[valeur_globale] = "&" + nom_globale;
    //        }
    //        else {
    //            auto nom_globale = "globale" + dls::vers_chaine(valeur_globale);
    //            os << nom_globale;
    //            table_globales[valeur_globale] = "&" + broye_nom_simple(nom_globale);
    //        }

    //        os << ";\n";
    //   }

    os << "section .text\n";

    // prédéclare ensuite les fonction pour éviter les problèmes de
    // dépendances cycliques, mais aussi pour prendre en compte les cas où
    // les globales utilises des fonctions dans leurs initialisations
    POUR (fonctions) {
        if (!it->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
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
        if (!it->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
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

        auto numero_inst = it->params_entrees.taille();

        for (auto inst : it->instructions) {
            inst->numero = numero_inst++;
            genere_code_pour_instruction(inst, os);
        }

        m_fonction_courante = nullptr;
        os << "\n";
    }
}

std::optional<ErreurCoulisse> CoulisseASM::génère_code_impl(const ArgsGénérationCode & /*args*/)
{
    return {};
}

std::optional<ErreurCoulisse> CoulisseASM::crée_fichier_objet_impl(
    const ArgsCréationFichiersObjets &args)
{
    auto &compilatrice_ri = *args.compilatrice_ri;
    auto &espace = *args.espace;
    auto const &programme = *args.programme;

    std::ostream &fichier_sortie = std::cerr;
    Enchaineuse enchaineuse;

    /* Convertis le programme sous forme de représentation intermédiaire. */
    auto repr_inter_programme = représentation_intermédiaire_programme(
        espace, compilatrice_ri, programme);

    if (!repr_inter_programme.has_value()) {
        return ErreurCoulisse{"Impossible d'obtenir la représentation intermédiaire du programme"};
    }

    // genere_code_debut_fichier(enchaineuse, compilatrice.racine_kuri);

    // genere_code_pour_types(compilatrice, graphe, enchaineuse);

    auto generatrice = GeneratriceCodeASM(espace);
    generatrice.genere_code(repr_inter_programme->donne_globales(),
                            repr_inter_programme->donne_fonctions(),
                            enchaineuse);

    enchaineuse.imprime_dans_flux(fichier_sortie);

    return {};
}

std::optional<ErreurCoulisse> CoulisseASM::crée_exécutable_impl(const ArgsLiaisonObjets & /*args*/)
{
    return ErreurCoulisse{"La création de compilat n'est pas encore implémenté."};
}
