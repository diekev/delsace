/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "instructions.hh"

#include <ostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "utilitaires/divers.hh"

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
    mémoire::deloge_tableau("valeur_structure", données.pointeur, données.capacité);
}

AtomeConstanteTableauFixe::~AtomeConstanteTableauFixe()
{
    mémoire::deloge_tableau("valeur_tableau", données.pointeur, données.capacité);
}

const Type *AtomeGlobale::donne_type_alloué() const
{
    return type->comme_type_pointeur()->type_pointé;
}

const Type *AccèsIndiceConstant::donne_type_accédé() const
{
    return accédé->type->comme_type_pointeur()->type_pointé;
}

VisibilitéSymbole AtomeGlobale::donne_visibilité_symbole() const
{
    if (!decl) {
        return VisibilitéSymbole::INTERNE;
    }

    return decl->visibilité_symbole;
}

PartageMémoire AtomeGlobale::donne_partage_mémoire() const
{
    if (!decl) {
        return PartageMémoire::GLOBAL;
    }
    return decl->partage_mémoire;
}

AtomeFonction::~AtomeFonction()
{
    /* À FAIRE : stocke ça quelque part dans un tableau_page. */
    mémoire::deloge("DonnéesExécutionFonction", données_exécution);
}

Instruction *AtomeFonction::dernière_instruction() const
{
    if (instructions.taille() == 0) {
        return nullptr;
    }
    return instructions[instructions.taille() - 1];
}

int AtomeFonction::nombre_d_instructions_avec_entrées_sorties() const
{
    /* +1 pour la sortie. */
    auto résultat = params_entrée.taille() + instructions.taille() + 1;
    auto type_sortie = param_sortie->donne_type_alloué();
    if (type_sortie->est_type_tuple()) {
        résultat += type_sortie->comme_type_tuple()->rubriques.taille();
    }
    return résultat;
}

int32_t AtomeFonction::numérote_instructions() const
{
    int32_t résultat = 0;

    POUR (params_entrée) {
        it->numéro = résultat++;
    }

    if (!param_sortie->type->est_type_rien()) {
        param_sortie->numéro = résultat++;

        if (decl && decl->params_sorties.taille() > 1) {
            POUR (decl->params_sorties) {
                auto inst = it->comme_déclaration_variable()->atome->comme_instruction();
                inst->numéro = résultat++;
            }
        }
    }

    POUR (instructions) {
        it->numéro = résultat++;
    }

    return résultat;
}

bool AtomeFonction::est_intrinsèque() const
{
    return decl && (decl->possède_drapeau(DrapeauxNoeudFonction::EST_INTRINSÈQUE) ||
                    decl->possède_drapeau(DrapeauxNoeudFonction::EST_SSE2));
}

InstructionAppel::InstructionAppel(NoeudExpression const *site_, Atome *appele_)
    : InstructionAppel(site_)
{
    auto type_fonction = appele_->type->comme_type_fonction();
    this->type = type_fonction->type_sortie;

    this->appelé = appele_;
}

InstructionAppel::InstructionAppel(NoeudExpression const *site_,
                                   Atome *appele_,
                                   kuri::tableau<Atome *, int> &&args_)
    : InstructionAppel(site_, appele_)
{
    this->args = std::move(args_);
}

DonnéesSymboleExterne *InstructionAppel::est_appel_intrinsèque() const
{
    if (!appelé->est_fonction()) {
        return nullptr;
    }

    auto fonction = appelé->comme_fonction();
    if (!fonction->est_intrinsèque()) {
        return nullptr;
    }

    return fonction->decl->données_externes;
}

InstructionAllocation::InstructionAllocation(NoeudExpression const *site_,
                                             Type const *type_,
                                             IdentifiantCode *ident_)
    : InstructionAllocation(site_)
{
    this->type = type_;
    this->ident = ident_;
}

const Type *InstructionAllocation::donne_type_alloué() const
{
    return type->comme_type_pointeur()->type_pointé;
}

InstructionRetour::InstructionRetour(NoeudExpression const *site_, Atome *valeur_)
    : InstructionRetour(site_)
{
    this->valeur = valeur_;
}

InstructionOpBinaire::InstructionOpBinaire(NoeudExpression const *site_,
                                           Type const *type_,
                                           OpérateurBinaire::Genre op_,
                                           Atome *valeur_gauche_,
                                           Atome *valeur_droite_)
    : InstructionOpBinaire(site_)
{
    this->type = type_;
    this->op = op_;
    this->valeur_gauche = valeur_gauche_;
    this->valeur_droite = valeur_droite_;
}

InstructionOpUnaire::InstructionOpUnaire(NoeudExpression const *site_,
                                         Type const *type_,
                                         OpérateurUnaire::Genre op_,
                                         Atome *valeur_)
    : InstructionOpUnaire(site_)
{
    this->type = type_;
    this->op = op_;
    this->valeur = valeur_;
}

InstructionChargeMem::InstructionChargeMem(NoeudExpression const *site_,
                                           Type const *type_,
                                           Atome *chargee_)
    : InstructionChargeMem(site_)
{
    this->type = type_;
    this->chargée = chargee_;
    if (type->est_type_pointeur()) {
        drapeaux |= DrapeauxAtome::EST_CHARGEABLE;
    }
}

InstructionStockeMem::InstructionStockeMem(NoeudExpression const *site_,
                                           Atome *ou_,
                                           Atome *valeur_)
    : InstructionStockeMem(site_)
{
    this->destination = ou_;
    this->source = valeur_;
}

InstructionLabel::InstructionLabel(NoeudExpression const *site_, int id_) : InstructionLabel(site_)
{
    this->id = id_;
}

InstructionBranche::InstructionBranche(NoeudExpression const *site_, InstructionLabel *label_)
    : InstructionBranche(site_)
{
    this->label = label_;
}

InstructionBrancheCondition::InstructionBrancheCondition(NoeudExpression const *site_,
                                                         Atome *condition_,
                                                         InstructionLabel *label_si_vrai_,
                                                         InstructionLabel *label_si_faux_)
    : InstructionBrancheCondition(site_)
{
    this->condition = condition_;
    this->label_si_vrai = label_si_vrai_;
    this->label_si_faux = label_si_faux_;
}

InstructionAccèsRubrique::InstructionAccèsRubrique(NoeudExpression const *site_,
                                                   Type const *type_,
                                                   Atome *accede_,
                                                   int indice_)
    : InstructionAccèsRubrique(site_)
{
    this->type = type_;
    this->accédé = accede_;
    this->indice = indice_;
}

const Type *InstructionAccèsRubrique::donne_type_accédé() const
{
    auto type_accédé = accédé->type;
    if (type_accédé->est_type_référence()) {
        return type_accédé->comme_type_référence()->type_pointé;
    }
    return type_accédé->comme_type_pointeur()->type_pointé;
}

const RubriqueTypeComposé &InstructionAccèsRubrique::donne_rubrique_accédé() const
{
    auto type_adressé = donne_type_accédé();
    type_adressé = donne_type_primitif(type_adressé);

    auto type_composé = type_adressé->comme_type_composé();
    /* Pour les unions, l'accès de rubrique se fait via le type structure qui est valeur unie
     * + index. */
    if (type_composé->est_type_union()) {
        type_composé = type_composé->comme_type_union()->type_structure;
    }

    return type_composé->rubriques[indice];
}

InstructionAccèsIndice::InstructionAccèsIndice(NoeudExpression const *site_,
                                               Type const *type_,
                                               Atome *accede_,
                                               Atome *indice_)
    : InstructionAccèsIndice(site_)
{
    this->type = type_;
    this->accédé = accede_;
    this->indice = indice_;
}

const Type *InstructionAccèsIndice::donne_type_accédé() const
{
    return accédé->type->comme_type_pointeur()->type_pointé;
}

kuri::chaine_statique chaine_pour_type_transtypage(TypeTranstypage const type)
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

TypeTranstypage type_transtypage_depuis_ident(IdentifiantCode const *ident)
{
#define ENUMERE_TYPE_TRANSTYPAGE_EX(genre, ident_)                                                \
    if (ident == ID::ident_) {                                                                    \
        return TypeTranstypage::genre;                                                            \
    }
    ENUMERE_TYPE_TRANSTYPAGE(ENUMERE_TYPE_TRANSTYPAGE_EX)
#undef ENUMERE_TYPE_TRANSTYPAGE_EX

    assert(false);
    return TypeTranstypage::BITS;
}

InstructionTranstype::InstructionTranstype(NoeudExpression const *site_,
                                           Type const *type_,
                                           Atome *valeur_,
                                           TypeTranstypage op_)
    : InstructionTranstype(site_)
{
    this->type = type_;
    this->valeur = valeur_;
    this->op = op_;
    if (type->est_type_pointeur()) {
        drapeaux |= DrapeauxAtome::EST_CHARGEABLE;
    }
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
    return stockage->destination == inst1;
}

bool est_accès_rubrique_ou_index(Instruction const *inst0, Instruction const *inst1)
{
    if (inst0->est_accès_rubrique()) {
        auto accès = inst0->comme_accès_rubrique();
        return accès->accédé == inst1;
    }

    if (inst0->est_accès_indice()) {
        auto accès = inst0->comme_accès_indice();
        return accès->accédé == inst1;
    }

    return false;
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
    return charge->chargée == inst1;
}

bool est_chargement_de(Atome const *chargement_potentiel, Atome const *valeur)
{
    if (!chargement_potentiel->est_instruction()) {
        return false;
    }

    auto const instruction = chargement_potentiel->comme_instruction();
    if (!instruction->est_charge()) {
        return false;
    }

    auto const chargement = instruction->comme_charge();
    return chargement->chargée == valeur;
}

InstructionAllocation const *est_stocke_alloc_depuis_charge_alloc(InstructionStockeMem const *inst)
{
    if (!est_allocation(inst->destination)) {
        return nullptr;
    }

    auto atome_source = inst->source;
    if (!atome_source->est_instruction()) {
        return nullptr;
    }

    auto instruction_source = atome_source->comme_instruction();
    if (!instruction_source->est_charge()) {
        return nullptr;
    }

    auto chargement = instruction_source->comme_charge();
    if (!est_allocation(chargement->chargée)) {
        return nullptr;
    }

    return static_cast<InstructionAllocation const *>(chargement->chargée);
}

bool est_stocke_alloc_incrémente(InstructionStockeMem const *inst)
{
    if (!est_allocation(inst->destination)) {
        return false;
    }

    auto alloc_destination = inst->destination->comme_instruction()->comme_alloc();

    auto atome_source = inst->source;
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
    return est_élément(inst->genre,
                       GenreInstruction::APPEL,
                       GenreInstruction::BRANCHE,
                       GenreInstruction::BRANCHE_CONDITION,
                       GenreInstruction::LABEL,
                       GenreInstruction::RETOUR,
                       GenreInstruction::STOCKE_MÉMOIRE,
                       GenreInstruction::INATTEIGNABLE);
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

bool est_instruction_comparaison(Atome const *atome)
{
    if (!atome->est_instruction()) {
        return false;
    }

    auto const inst = atome->comme_instruction();
    if (!inst->est_op_binaire()) {
        return false;
    }

    auto const op_binaire = inst->comme_op_binaire();
    return est_opérateur_comparaison(op_binaire->op);
}

bool est_volatile(Atome const *atome)
{
    auto résultat = false;
    if (atome->est_instruction()) {
        auto const inst = atome->comme_instruction();
        if (inst->est_alloc()) {
            auto const alloc = inst->comme_alloc();
            if (alloc->site && alloc->site->possède_drapeau(DrapeauxNoeud::EST_VOLATILE)) {
                résultat = true;
            }
        }
        else if (inst->est_accès_rubrique()) {
            auto const accès = inst->comme_accès_rubrique();
            auto const &rubrique = accès->donne_rubrique_accédé();
            résultat = rubrique.decl &&
                       rubrique.decl->possède_drapeau(DrapeauxNoeud::EST_VOLATILE);
        }
    }
    else if (atome->est_globale()) {
        auto const globale = atome->comme_globale();
        résultat = globale->decl && globale->decl->possède_drapeau(DrapeauxNoeud::EST_VOLATILE);
    }
    return résultat;
}

AccèsRubriqueFusionné fusionne_accès_rubriques(InstructionAccèsRubrique const *accès_rubrique)
{
    AccèsRubriqueFusionné résultat;

    while (true) {
        auto const &rubrique = accès_rubrique->donne_rubrique_accédé();

        résultat.accédé = accès_rubrique->accédé;
        résultat.décalage += rubrique.decalage;

        if (!accès_rubrique->accédé->est_instruction()) {
            break;
        }

        auto inst = accès_rubrique->accédé->comme_instruction();
        if (!inst->est_accès_rubrique()) {
            break;
        }

        accès_rubrique = inst->comme_accès_rubrique();
    }

    return résultat;
}

std::ostream &operator<<(std::ostream &os, GenreInstruction genre)
{
#define ENUMERE_GENRE_INSTRUCTION_EX(Genre, nom_classe, ident)                                    \
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

void VisiteuseAtome::réinitialise()
{
    visites.efface();
}

void VisiteuseAtome::visite_atome(Atome *racine,
                                  std::function<DécisionVisiteAtome(Atome *)> rappel)
{
    if (!racine) {
        return;
    }

    if (visites.possède(racine)) {
        return;
    }

    visites.insère(racine);

    if (rappel(racine) == DécisionVisiteAtome::ARRÊTE) {
        return;
    }

    switch (racine->genre_atome) {
        case Atome::Genre::TRANSTYPE_CONSTANT:
        {
            auto transtype_const = racine->comme_transtype_constant();
            visite_atome(transtype_const->valeur, rappel);
            break;
        }
        case Atome::Genre::ACCÈS_INDICE_CONSTANT:
        {
            auto inst_acces = racine->comme_accès_indice_constant();
            visite_atome(inst_acces->accédé, rappel);
            break;
        }
        case Atome::Genre::CONSTANTE_NULLE:
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
            POUR (structure_const->donne_atomes_rubriques()) {
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
                    visite_atome(appel->appelé, rappel);

                    POUR (appel->args) {
                        visite_atome(it, rappel);
                    }

                    break;
                }
                case GenreInstruction::CHARGE_MÉMOIRE:
                {
                    auto charge = inst->comme_charge();
                    visite_atome(charge->chargée, rappel);
                    break;
                }
                case GenreInstruction::STOCKE_MÉMOIRE:
                {
                    auto stocke = inst->comme_stocke_mem();
                    visite_atome(stocke->source, rappel);
                    visite_atome(stocke->destination, rappel);
                    break;
                }
                case GenreInstruction::OPÉRATION_UNAIRE:
                {
                    auto op = inst->comme_op_unaire();
                    visite_atome(op->valeur, rappel);
                    break;
                }
                case GenreInstruction::OPÉRATION_BINAIRE:
                {
                    auto op = inst->comme_op_binaire();
                    visite_atome(op->valeur_droite, rappel);
                    visite_atome(op->valeur_gauche, rappel);
                    break;
                }
                case GenreInstruction::ACCÈS_INDICE:
                {
                    auto acces = inst->comme_accès_indice();
                    visite_atome(acces->indice, rappel);
                    visite_atome(acces->accédé, rappel);
                    break;
                }
                case GenreInstruction::ACCÈS_RUBRIQUE:
                {
                    auto acces = inst->comme_accès_rubrique();
                    visite_atome(acces->accédé, rappel);
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
                case GenreInstruction::SÉLECTION:
                {
                    auto const sélection = inst->comme_sélection();
                    visite_atome(sélection->condition, rappel);
                    visite_atome(sélection->si_vrai, rappel);
                    visite_atome(sélection->si_faux, rappel);
                    break;
                }
                case GenreInstruction::ALLOCATION:
                case GenreInstruction::BRANCHE:
                case GenreInstruction::LABEL:
                case GenreInstruction::INATTEIGNABLE:
                case GenreInstruction::ARRÊT_DÉBUG:
                {
                    /* Pas de sous-atome. */
                    break;
                }
            }

            break;
        }
    }
}

void visite_atome(Atome *racine, std::function<DécisionVisiteAtome(Atome *)> rappel)
{
    VisiteuseAtome visiteuse{};
    visiteuse.visite_atome(racine, rappel);
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

/* ------------------------------------------------------------------------- */
/** \name UtilisationAtome.
 * \{ */

std::ostream &operator<<(std::ostream &os, UtilisationAtome const utilisation)
{
    if (utilisation == UtilisationAtome::AUCUNE) {
        os << "AUCUNE";
        return os;
    }

#define SI_DRAPEAU_UTILISE(drapeau)                                                               \
    if ((utilisation & UtilisationAtome::drapeau) != UtilisationAtome::AUCUNE) {                  \
        identifiants.ajoute(#drapeau);                                                            \
    }

    kuri::tablet<kuri::chaine_statique, 32> identifiants;

    SI_DRAPEAU_UTILISE(RACINE)
    SI_DRAPEAU_UTILISE(POUR_GLOBALE)
    SI_DRAPEAU_UTILISE(POUR_BRANCHE_CONDITION)
    SI_DRAPEAU_UTILISE(POUR_SOURCE_ÉCRITURE)
    SI_DRAPEAU_UTILISE(POUR_DESTINATION_ÉCRITURE)
    SI_DRAPEAU_UTILISE(POUR_OPÉRATEUR)
    SI_DRAPEAU_UTILISE(POUR_LECTURE)

    auto virgule = "";

    POUR (identifiants) {
        os << virgule << it;
        virgule = " | ";
    }

#undef SI_DRAPEAU_UTILISE

    return os;
}

/** \} */
