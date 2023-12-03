/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "instructions.hh"

#include <ostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "code_binaire.hh"

std::ostream &operator<<(std::ostream &os, Atome::Genre genre_atome)
{
#define ENUMERE_GENRE_ATOME_EX(__genre, __type, __ident)                                          \
    case Atome::Genre::__genre:                                                                   \
    {                                                                                             \
        os << #__genre;                                                                           \
        break;                                                                                    \
    }
    switch (genre_atome) {
        ENUMERE_GENRE_ATOME(ENUMERE_GENRE_ATOME_EX)
    }
#undef ENUMERE_GENRE_ATOME_EX
    return os;
}

AtomeConstanteStructure::~AtomeConstanteStructure()
{
    memoire::deloge_tableau("valeur_structure", données.pointeur, données.capacite);
}

AtomeConstanteTableauFixe::~AtomeConstanteTableauFixe()
{
    memoire::deloge_tableau("valeur_tableau", données.pointeur, données.capacite);
}

const Type *AtomeGlobale::donne_type_alloué() const
{
    return type->comme_type_pointeur()->type_pointe;
}

const Type *AccedeIndexConstant::donne_type_accédé() const
{
    return accede->type->comme_type_pointeur()->type_pointe;
}

VisibilitéSymbole AtomeGlobale::donne_visibilité_symbole() const
{
    if (!decl) {
        return VisibilitéSymbole::INTERNE;
    }

    return decl->visibilité_symbole;
}

AtomeFonction::~AtomeFonction()
{
    /* À FAIRE : stocke ça quelque part dans un tableau_page. */
    memoire::deloge("DonnéesExécutionFonction", données_exécution);
}

Instruction *AtomeFonction::derniere_instruction() const
{
    if (instructions.taille() == 0) {
        return nullptr;
    }
    return instructions[instructions.taille() - 1];
}

int AtomeFonction::nombre_d_instructions_avec_entrées_sorties() const
{
    /* +1 pour la sortie. */
    auto résultat = params_entrees.taille() + instructions.taille() + 1;
    auto type_sortie = param_sortie->donne_type_alloué();
    if (type_sortie->est_type_tuple()) {
        résultat += type_sortie->comme_type_tuple()->membres.taille();
    }
    return résultat;
}

const Type *InstructionAllocation::donne_type_alloué() const
{
    return type->comme_type_pointeur()->type_pointe;
}

const Type *InstructionAccedeMembre::donne_type_accédé() const
{
    auto type_accédé = accede->type;
    if (type_accédé->est_type_reference()) {
        return type_accédé->comme_type_reference()->type_pointe;
    }
    return type_accédé->comme_type_pointeur()->type_pointe;
}

const MembreTypeComposé &InstructionAccedeMembre::donne_membre_accédé() const
{
    auto type_adressé = donne_type_accédé();
    if (type_adressé->est_type_opaque()) {
        type_adressé = type_adressé->comme_type_opaque()->type_opacifie;
    }

    auto type_composé = type_adressé->comme_type_compose();
    /* Pour les unions, l'accès de membre se fait via le type structure qui est valeur unie
     * + index. */
    if (type_composé->est_type_union()) {
        type_composé = type_composé->comme_type_union()->type_structure;
    }

    return type_composé->membres[index];
}

const Type *InstructionAccedeIndex::donne_type_accédé() const
{
    return accede->type->comme_type_pointeur()->type_pointe;
}

bool est_valeur_constante(Atome const *atome)
{
    return atome->est_constante_booléenne() || atome->est_constante_caractère() ||
           atome->est_constante_entière() || atome->est_constante_réelle();
}

static bool est_constante_entière_de_valeur(Atome const *atome, uint64_t valeur)
{
    if (!atome->est_constante_entière()) {
        return false;
    }

    auto valeur_constante = atome->comme_constante_entière();
    return valeur_constante->valeur == valeur;
}

bool est_constante_entière_zéro(Atome const *atome)
{
    return est_constante_entière_de_valeur(atome, 0);
}

bool est_constante_entière_un(Atome const *atome)
{
    return est_constante_entière_de_valeur(atome, 1);
}

bool est_allocation(Atome const *atome)
{
    return atome->est_instruction() && atome->comme_instruction()->est_alloc();
}

bool est_locale_ou_globale(Atome const *atome)
{
    if (atome->est_globale()) {
        return true;
    }

    return est_allocation(atome);
}

bool est_stockage_vers(Instruction const *inst0, Instruction const *inst1)
{
    if (!inst0->est_stocke_mem()) {
        return false;
    }

    auto const stockage = inst0->comme_stocke_mem();
    return stockage->ou == inst1;
}

bool est_transtypage_de(Instruction const *inst0, Instruction const *inst1)
{
    if (!inst0->est_transtype()) {
        return false;
    }

    auto const transtype = inst0->comme_transtype();
    return transtype->valeur == inst1;
}

bool est_chargement_de(Instruction const *inst0, Instruction const *inst1)
{
    if (!inst0->est_charge()) {
        return false;
    }

    auto const charge = inst0->comme_charge();
    return charge->chargee == inst1;
}

InstructionAllocation const *est_stocke_alloc_depuis_charge_alloc(InstructionStockeMem const *inst)
{
    if (!est_allocation(inst->ou)) {
        return nullptr;
    }

    auto atome_source = inst->valeur;
    if (!atome_source->est_instruction()) {
        return nullptr;
    }

    auto instruction_source = atome_source->comme_instruction();
    if (!instruction_source->est_charge()) {
        return nullptr;
    }

    auto chargement = instruction_source->comme_charge();
    if (!est_allocation(chargement->chargee)) {
        return nullptr;
    }

    return static_cast<InstructionAllocation const *>(chargement->chargee);
}

bool est_stocke_alloc_incrémente(InstructionStockeMem const *inst)
{
    if (!est_allocation(inst->ou)) {
        return false;
    }

    auto alloc_destination = inst->ou->comme_instruction()->comme_alloc();

    auto atome_source = inst->valeur;
    if (!atome_source->est_instruction()) {
        return false;
    }

    auto instruction_source = atome_source->comme_instruction();
    if (!instruction_source->est_op_binaire()) {
        return false;
    }

    auto op_binaire = instruction_source->comme_op_binaire();
    if (op_binaire->op != OpérateurBinaire::Genre::Addition) {
        return false;
    }

    auto valeur_droite = op_binaire->valeur_droite;
    if (!est_constante_entière_un(valeur_droite)) {
        return false;
    }

    auto valeur_gauche = op_binaire->valeur_gauche;
    if (!valeur_gauche->est_instruction()) {
        return false;
    }
    if (!est_chargement_de(valeur_gauche->comme_instruction(), alloc_destination)) {
        return false;
    }

    return true;
}

bool est_opérateur_binaire_constant(Instruction const *inst)
{
    if (!inst->est_op_binaire()) {
        return false;
    }

    auto const op_binaire = inst->comme_op_binaire();
    auto const opérande_gauche = op_binaire->valeur_gauche;
    auto const opérande_droite = op_binaire->valeur_droite;

    return est_valeur_constante(opérande_gauche) && est_valeur_constante(opérande_droite);
}

bool est_constante_pointeur_nul(Atome const *atome)
{
    if (atome->est_constante_nulle()) {
        return true;
    }

    if (!atome->est_instruction()) {
        return false;
    }

    auto const inst = atome->comme_instruction();
    if (!inst->est_transtype()) {
        return false;
    }

    auto const transtype = inst->comme_transtype();
    return transtype->type->est_type_pointeur() && est_constante_pointeur_nul(transtype->valeur);
}

bool instruction_est_racine(Instruction const *inst)
{
    return dls::outils::est_element(inst->genre,
                                    GenreInstruction::APPEL,
                                    GenreInstruction::BRANCHE,
                                    GenreInstruction::BRANCHE_CONDITION,
                                    GenreInstruction::LABEL,
                                    GenreInstruction::RETOUR,
                                    GenreInstruction::STOCKE_MEMOIRE);
}

static Atome const *est_comparaison_avec_zéro_ou_nul(Instruction const *inst,
                                                     OpérateurBinaire::Genre genre_comp)
{
    if (!inst->est_op_binaire()) {
        return nullptr;
    }

    auto const op_binaire = inst->comme_op_binaire();
    if (op_binaire->op != genre_comp) {
        return nullptr;
    }

    if (!est_constante_entière_zéro(op_binaire->valeur_droite) &&
        !est_constante_pointeur_nul(op_binaire->valeur_droite)) {
        return nullptr;
    }

    return op_binaire->valeur_gauche;
}

Atome const *est_comparaison_égal_zéro_ou_nul(Instruction const *inst)
{
    return est_comparaison_avec_zéro_ou_nul(inst, OpérateurBinaire::Genre::Comp_Egal);
}

Atome const *est_comparaison_inégal_zéro_ou_nul(Instruction const *inst)
{
    return est_comparaison_avec_zéro_ou_nul(inst, OpérateurBinaire::Genre::Comp_Inegal);
}

AccèsMembreFusionné fusionne_accès_membres(InstructionAccedeMembre const *accès_membre)
{
    AccèsMembreFusionné résultat;

    while (true) {
        auto const &membre = accès_membre->donne_membre_accédé();

        résultat.accédé = accès_membre->accede;
        résultat.décalage += membre.decalage;

        if (!accès_membre->accede->est_instruction()) {
            break;
        }

        auto inst = accès_membre->accede->comme_instruction();
        if (!inst->est_acces_membre()) {
            break;
        }

        accès_membre = inst->comme_acces_membre();
    }

    return résultat;
}

std::ostream &operator<<(std::ostream &os, GenreInstruction genre)
{
#define ENUMERE_GENRE_INSTRUCTION_EX(Genre)                                                       \
    case GenreInstruction::Genre:                                                                 \
    {                                                                                             \
        os << #Genre;                                                                             \
        break;                                                                                    \
    }
    switch (genre) {
        ENUMERE_GENRE_INSTRUCTION(ENUMERE_GENRE_INSTRUCTION_EX)
    }
#undef ENUMERE_GENRE_INSTRUCTION_EX

    return os;
}

void VisiteuseAtome::reinitialise()
{
    visites.efface();
}

void VisiteuseAtome::visite_atome(Atome *racine, std::function<void(Atome *)> rappel)
{
    if (!racine) {
        return;
    }

    if (visites.possède(racine)) {
        return;
    }

    visites.insère(racine);

    rappel(racine);

    switch (racine->genre_atome) {
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype_const = racine->comme_transtype_constant();
            visite_atome(transtype_const->valeur, rappel);
            break;
        }
        case Atome::Genre::ACCÈS_INDEX_CONSTANT:
        {
            auto inst_acces = racine->comme_accès_index_constant();
            visite_atome(inst_acces->accede, rappel);
            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
        case Atome::Genre::CONSTANTE_TYPE:
        case Atome::Genre::CONSTANTE_RÉELLE:
        case Atome::Genre::CONSTANTE_ENTIÈRE:
        case Atome::Genre::CONSTANTE_BOOLÉENNE:
        case Atome::Genre::CONSTANTE_CARACTÈRE:
        case Atome::Genre::CONSTANTE_DONNÉES_CONSTANTES:
        case Atome::Genre::CONSTANTE_TAILLE_DE:
        case Atome::Genre::NON_INITIALISATION:
        {
            /* Pas de sous-atome. */
            break;
        }
        case Atome::Genre::CONSTANTE_STRUCTURE:
        {
            auto structure_const = racine->comme_constante_structure();
            POUR (structure_const->donne_atomes_membres()) {
                visite_atome(it, rappel);
            }
            break;
        }
        case Atome::Genre::CONSTANTE_TABLEAU_FIXE:
        {
            auto tableau_const = racine->comme_constante_tableau();
            POUR (tableau_const->donne_atomes_éléments()) {
                visite_atome(it, rappel);
            }
            break;
        }
        case Atome::Genre::INITIALISATION_TABLEAU:
        {
            auto init_tableau = racine->comme_initialisation_tableau();
            visite_atome(const_cast<AtomeConstante *>(init_tableau->valeur), rappel);
            break;
        }
        case Atome::Genre::GLOBALE:
        {
            auto globale = racine->comme_globale();
            visite_atome(globale->initialisateur, rappel);
            break;
        }
        case Atome::Genre::FONCTION:
        {
            /* Pour l'instant nous faisons la visite depuis les fonctions, inutile de les
             * traverser. */
            break;
        }
        case Atome::Genre::INSTRUCTION:
        {
            auto inst = racine->comme_instruction();

            switch (inst->genre) {
                case GenreInstruction::APPEL:
                {
                    auto appel = inst->comme_appel();

                    /* appele peut être un pointeur de fonction */
                    visite_atome(appel->appele, rappel);

                    POUR (appel->args) {
                        visite_atome(it, rappel);
                    }

                    break;
                }
                case GenreInstruction::CHARGE_MEMOIRE:
                {
                    auto charge = inst->comme_charge();
                    visite_atome(charge->chargee, rappel);
                    break;
                }
                case GenreInstruction::STOCKE_MEMOIRE:
                {
                    auto stocke = inst->comme_stocke_mem();
                    visite_atome(stocke->valeur, rappel);
                    visite_atome(stocke->ou, rappel);
                    break;
                }
                case GenreInstruction::OPERATION_UNAIRE:
                {
                    auto op = inst->comme_op_unaire();
                    visite_atome(op->valeur, rappel);
                    break;
                }
                case GenreInstruction::OPERATION_BINAIRE:
                {
                    auto op = inst->comme_op_binaire();
                    visite_atome(op->valeur_droite, rappel);
                    visite_atome(op->valeur_gauche, rappel);
                    break;
                }
                case GenreInstruction::ACCEDE_INDEX:
                {
                    auto acces = inst->comme_acces_index();
                    visite_atome(acces->index, rappel);
                    visite_atome(acces->accede, rappel);
                    break;
                }
                case GenreInstruction::ACCEDE_MEMBRE:
                {
                    auto acces = inst->comme_acces_membre();
                    visite_atome(acces->accede, rappel);
                    break;
                }
                case GenreInstruction::TRANSTYPE:
                {
                    auto transtype = inst->comme_transtype();
                    visite_atome(transtype->valeur, rappel);
                    break;
                }
                case GenreInstruction::BRANCHE_CONDITION:
                {
                    auto branche = inst->comme_branche_cond();
                    visite_atome(branche->condition, rappel);
                    break;
                }
                case GenreInstruction::RETOUR:
                {
                    auto retour = inst->comme_retour();
                    visite_atome(retour->valeur, rappel);
                    break;
                }
                case GenreInstruction::ALLOCATION:
                case GenreInstruction::INVALIDE:
                case GenreInstruction::BRANCHE:
                case GenreInstruction::LABEL:
                {
                    /* Pas de sous-atome. */
                    break;
                }
            }

            break;
        }
    }
}

void visite_atome(Atome *racine, std::function<void(Atome *)> rappel)
{
    VisiteuseAtome visiteuse{};
    visiteuse.visite_atome(racine, rappel);
}

void visite_opérandes_instruction(Instruction *inst, std::function<void(Atome *)> rappel)
{
    switch (inst->genre) {
        case GenreInstruction::APPEL:
        {
            auto appel = inst->comme_appel();

            /* appele peut être un pointeur de fonction */
            rappel(appel->appele);

            POUR (appel->args) {
                rappel(it);
            }

            break;
        }
        case GenreInstruction::CHARGE_MEMOIRE:
        {
            auto charge = inst->comme_charge();
            rappel(charge->chargee);
            break;
        }
        case GenreInstruction::STOCKE_MEMOIRE:
        {
            auto stocke = inst->comme_stocke_mem();
            rappel(stocke->valeur);
            rappel(stocke->ou);
            break;
        }
        case GenreInstruction::OPERATION_UNAIRE:
        {
            auto op = inst->comme_op_unaire();
            rappel(op->valeur);
            break;
        }
        case GenreInstruction::OPERATION_BINAIRE:
        {
            auto op = inst->comme_op_binaire();
            rappel(op->valeur_droite);
            rappel(op->valeur_gauche);
            break;
        }
        case GenreInstruction::ACCEDE_INDEX:
        {
            auto acces = inst->comme_acces_index();
            rappel(acces->index);
            rappel(acces->accede);
            break;
        }
        case GenreInstruction::ACCEDE_MEMBRE:
        {
            auto acces = inst->comme_acces_membre();
            rappel(acces->accede);
            break;
        }
        case GenreInstruction::TRANSTYPE:
        {
            auto transtype = inst->comme_transtype();
            rappel(transtype->valeur);
            break;
        }
        case GenreInstruction::BRANCHE_CONDITION:
        {
            auto branche = inst->comme_branche_cond();
            rappel(branche->condition);
            break;
        }
        case GenreInstruction::RETOUR:
        {
            auto retour = inst->comme_retour();
            if (retour->valeur) {
                rappel(retour->valeur);
            }
            break;
        }
        case GenreInstruction::ALLOCATION:
        case GenreInstruction::INVALIDE:
        case GenreInstruction::BRANCHE:
        case GenreInstruction::LABEL:
        {
            /* Pas de sous-atome. */
            break;
        }
    }
}

bool est_tableau_données_constantes(AtomeConstante const *constante)
{
    return constante->est_données_constantes();
}

bool est_globale_pour_tableau_données_constantes(AtomeGlobale const *globale)
{
    if (globale->est_externe) {
        return false;
    }

    if (!globale->initialisateur) {
        return false;
    }

    return est_tableau_données_constantes(globale->initialisateur);
}
