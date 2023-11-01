/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "optimisations.hh"

#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "structures/table_hachage.hh"
#include "structures/tablet.hh"

#include "analyse.hh"
#include "bloc_basique.hh"
#include "constructrice_ri.hh"
#include "impression.hh"
#include "instructions.hh"
#include "log.hh"

/*
  À FAIRE(optimisations) :
  - crée toujours des blocs pour la RI, l'enlignage sera plus simple
  - bug dans la fusion des blocs, qui nous laissent avec des labels inconnus
  - déplace les instructions dans les blocs au plus près de leurs utilisations

  À FAIRE(enlignage) :
  - détecte les fonctions récursives, empêche leurs enlignages
  - enlignage ascendant (considère la fonction enlignée d'abord) ou descendant (considère la
  fonction où enligner d'abord)
  - problème avec l'enlignage : il semblerait que les pointeurs ne soit pas correctement « enlignés
  » pour les accès de membres
  - change la métriques pour être sur le nombre de lignes, et non le nombre d'instructions
 */

enum {
    PROPRE = 0,

    /* des fonctions sont à enlignées */
    REQUIERS_ENLIGNAGE = (1 << 0),

    /* des blocs sont vides ou furent insérés ou supprimés */
    REQUIERS_CORRECTION_BLOCS = (1 << 1),

    /* des constantes ont été détectées
     * - soit des nombres littéraux
     * - soit l'accès à des globales (taille de chaines...) */
    REQUIERS_PROPAGATION_CONSTANTES = (1 << 2),

    /* des boucles peuvent être « débouclées » */
    REQUIERS_DEBOUCLAGE = (1 << 3),

    /* du code peut être supprimé */
    REQUIERS_SUPPRESSION_CODE_MORT = (1 << 4),
};

/* À FAIRE(optimisations) : non-urgent
 * - Substitutrice, pour généraliser les substitions d'instructions
 * - copie des instructions (requiers de séparer les allocations des instructions de la
 * ConstructriceRI)
 */

#undef DEBOGUE_SUPPRESSION_CODE_MORT

struct CopieuseInstruction {
  private:
    kuri::table_hachage<Atome *, Atome *> copies{"Instructions copiées"};
    ConstructriceRI &constructrice;

  public:
    CopieuseInstruction(ConstructriceRI &constructrice_) : constructrice(constructrice_)
    {
    }

    void ajoute_substitution(Atome *a, Atome *b)
    {
        copies.insère(a, b);
    }

    kuri::tableau<Instruction *, int> copie_instructions(AtomeFonction *atome_fonction)
    {
        kuri::tableau<Instruction *, int> resultat;
        resultat.reserve(atome_fonction->instructions.taille());

        POUR (atome_fonction->instructions) {
            // s'il existe une substition pour cette instruction, ignore-là
            if (!it->est_label() && copies.possède(it)) {
                continue;
            }

            resultat.ajoute(static_cast<Instruction *>(copie_atome(it)));
        }

        return resultat;
    }

    Atome *copie_atome(Atome *atome)
    {
        if (atome == nullptr) {
            return nullptr;
        }

        // les constantes et les globales peuvent être partagées
        if (!atome->est_instruction()) {
            return atome;
        }

        auto inst = atome->comme_instruction();

        auto trouvee = false;
        auto valeur = copies.trouve(inst, trouvee);
        if (trouvee) {
            return valeur;
        }

        auto nouvelle_inst = static_cast<Instruction *>(nullptr);

        switch (inst->genre) {
            case Instruction::Genre::APPEL:
            {
                auto appel = inst->comme_appel();
                auto nouvelle_appel = constructrice.insts_appel.ajoute_element(inst->site);
                nouvelle_appel->drapeaux = appel->drapeaux;
                nouvelle_appel->appele = copie_atome(appel->appele);
                nouvelle_appel->adresse_retour = static_cast<InstructionAllocation *>(
                    copie_atome(appel->adresse_retour));

                nouvelle_appel->args.reserve(appel->args.taille());

                POUR (appel->args) {
                    nouvelle_appel->args.ajoute(copie_atome(it));
                }

                nouvelle_inst = nouvelle_appel;
                break;
            }
            case Instruction::Genre::CHARGE_MEMOIRE:
            {
                auto charge = inst->comme_charge();
                auto n_charge = constructrice.insts_charge_memoire.ajoute_element(inst->site);
                n_charge->chargee = copie_atome(charge->chargee);
                nouvelle_inst = n_charge;
                break;
            }
            case Instruction::Genre::STOCKE_MEMOIRE:
            {
                auto stocke = inst->comme_stocke_mem();
                auto n_stocke = constructrice.insts_stocke_memoire.ajoute_element(inst->site);
                n_stocke->ou = copie_atome(stocke->ou);
                n_stocke->valeur = copie_atome(stocke->valeur);
                nouvelle_inst = n_stocke;
                break;
            }
            case Instruction::Genre::OPERATION_UNAIRE:
            {
                auto op = inst->comme_op_unaire();
                auto n_op = constructrice.insts_opunaire.ajoute_element(inst->site);
                n_op->op = op->op;
                n_op->valeur = copie_atome(op->valeur);
                nouvelle_inst = n_op;
                break;
            }
            case Instruction::Genre::OPERATION_BINAIRE:
            {
                auto op = inst->comme_op_binaire();
                auto n_op = constructrice.insts_opbinaire.ajoute_element(inst->site);
                n_op->op = op->op;
                n_op->valeur_gauche = copie_atome(op->valeur_gauche);
                n_op->valeur_droite = copie_atome(op->valeur_droite);
                nouvelle_inst = n_op;
                break;
            }
            case Instruction::Genre::ACCEDE_INDEX:
            {
                auto acces = inst->comme_acces_index();
                auto n_acces = constructrice.insts_accede_index.ajoute_element(inst->site);
                n_acces->index = copie_atome(acces->index);
                n_acces->accede = copie_atome(acces->accede);
                nouvelle_inst = n_acces;
                break;
            }
            case Instruction::Genre::ACCEDE_MEMBRE:
            {
                auto acces = inst->comme_acces_membre();
                auto n_acces = constructrice.insts_accede_membre.ajoute_element(inst->site);
                n_acces->index = copie_atome(acces->index);
                n_acces->accede = copie_atome(acces->accede);
                nouvelle_inst = n_acces;
                break;
            }
            case Instruction::Genre::TRANSTYPE:
            {
                auto transtype = inst->comme_transtype();
                auto n_transtype = constructrice.insts_transtype.ajoute_element(inst->site);
                n_transtype->op = transtype->op;
                n_transtype->valeur = copie_atome(transtype->valeur);
                nouvelle_inst = n_transtype;
                break;
            }
            case Instruction::Genre::BRANCHE_CONDITION:
            {
                auto branche = inst->comme_branche_cond();
                auto n_branche = constructrice.insts_branche_condition.ajoute_element(inst->site);
                n_branche->condition = copie_atome(branche->condition);
                n_branche->label_si_faux =
                    copie_atome(branche->label_si_faux)->comme_instruction()->comme_label();
                n_branche->label_si_vrai =
                    copie_atome(branche->label_si_vrai)->comme_instruction()->comme_label();
                nouvelle_inst = n_branche;
                break;
            }
            case Instruction::Genre::BRANCHE:
            {
                auto branche = inst->comme_branche();
                auto n_branche = constructrice.insts_branche.ajoute_element(inst->site);
                n_branche->label = copie_atome(branche->label)->comme_instruction()->comme_label();
                nouvelle_inst = n_branche;
                break;
            }
            case Instruction::Genre::RETOUR:
            {
                auto retour = inst->comme_retour();
                auto n_retour = constructrice.insts_retour.ajoute_element(inst->site);
                n_retour->valeur = copie_atome(retour->valeur);
                nouvelle_inst = n_retour;
                break;
            }
            case Instruction::Genre::ALLOCATION:
            {
                auto alloc = inst->comme_alloc();
                auto n_alloc = constructrice.insts_allocation.ajoute_element(inst->site);
                n_alloc->ident = alloc->ident;
                nouvelle_inst = n_alloc;
                break;
            }
            case Instruction::Genre::LABEL:
            {
                auto label = inst->comme_label();
                auto n_label = constructrice.insts_label.ajoute_element(inst->site);
                n_label->id = label->id;
                nouvelle_inst = n_label;
                break;
            }
            case Instruction::Genre::INVALIDE:
            {
                break;
            }
        }

        if (nouvelle_inst) {
            nouvelle_inst->type = inst->type;
            ajoute_substitution(inst, nouvelle_inst);
        }

        return nouvelle_inst;
    }
};

void performe_enlignage(ConstructriceRI &constructrice,
                        kuri::tableau<Instruction *, int> &nouvelles_instructions,
                        AtomeFonction *fonction_appelee,
                        kuri::tableau<Atome *, int> const &arguments,
                        int &nombre_labels,
                        InstructionLabel *label_post,
                        InstructionAllocation *adresse_retour)
{

    auto copieuse = CopieuseInstruction(constructrice);

    for (auto i = 0; i < fonction_appelee->params_entrees.taille(); ++i) {
        auto parametre = fonction_appelee->params_entrees[i]->comme_instruction();
        auto atome = arguments[i];

        // À FAIRE : il faudrait que tous les arguments des fonctions soient des instructions (->
        // utilisation de temporaire)
        if (atome->genre_atome == Atome::Genre::INSTRUCTION) {
            auto inst = atome->comme_instruction();

            if (inst->genre == Instruction::Genre::CHARGE_MEMOIRE) {
                atome = inst->comme_charge()->chargee;
            }
            // À FAIRE : détection des pointeurs locaux plus robuste
            // détecte les cas où nous avons une référence à une variable
            else if (inst->est_alloc()) {
                auto type_pointe = inst->type->comme_type_pointeur()->type_pointe;
                if (type_pointe != atome->type) {
                    // remplace l'instruction de déréférence par l'atome
                    POUR (fonction_appelee->instructions) {
                        if (it->est_charge()) {
                            auto charge = it->comme_charge();

                            if (charge->chargee == parametre) {
                                copieuse.ajoute_substitution(charge, atome);
                            }
                        }
                    }
                }
            }
        }
        else if (atome->genre_atome == Atome::Genre::CONSTANTE) {
            POUR (fonction_appelee->instructions) {
                if (it->est_charge()) {
                    auto charge = it->comme_charge();

                    if (charge->chargee == parametre) {
                        copieuse.ajoute_substitution(charge, atome);
                    }
                }
            }
        }

        copieuse.ajoute_substitution(parametre, atome);
    }

    auto instructions_copiees = copieuse.copie_instructions(fonction_appelee);
    nouvelles_instructions.reserve_delta(instructions_copiees.taille());

    POUR (instructions_copiees) {
        if (it->genre == Instruction::Genre::LABEL) {
            auto label = it->comme_label();

            // saute le label d'entrée de la fonction
            if (label->id == 0) {
                continue;
            }

            label->id = nombre_labels++;
        }
        else if (it->genre == Instruction::Genre::RETOUR) {
            auto retour = it->comme_retour();

            if (retour->valeur) {
                auto stockage = constructrice.crée_stocke_mem(
                    nullptr, adresse_retour, retour->valeur, true);
                nouvelles_instructions.ajoute(stockage);
            }

            auto branche = constructrice.crée_branche(nullptr, label_post, true);
            nouvelles_instructions.ajoute(branche);
            continue;
        }

        nouvelles_instructions.ajoute(it);
    }
}

enum class SubstitutDans : int {
    ZERO = 0,
    CHARGE = (1 << 0),
    VALEUR_STOCKEE = (1 << 1),
    ADRESSE_STOCKEE = (1 << 2),

    TOUT = (CHARGE | VALEUR_STOCKEE | ADRESSE_STOCKEE),
};

DEFINIS_OPERATEURS_DRAPEAU(SubstitutDans)

struct Substitutrice {
  private:
    struct DonneesSubstitution {
        Atome *original = nullptr;
        Atome *substitut = nullptr;
        SubstitutDans substitut_dans = SubstitutDans::TOUT;
    };

    kuri::tablet<DonneesSubstitution, 16> substitutions{};

  public:
    void ajoute_substitution(Atome *original, Atome *substitut, SubstitutDans substitut_dans)
    {
        assert(original);
        assert(substitut);

        if (log_actif) {
            std::cerr << "Subtitut : ";
            imprime_atome(original, std::cerr);
            std::cerr << " avec ";
            imprime_atome(substitut, std::cerr);
            std::cerr << '\n';
        }

        POUR (substitutions) {
            if (it.original == original) {
                it.substitut = substitut;
                it.substitut_dans = substitut_dans;
                return;
            }
        }

        substitutions.ajoute({original, substitut, substitut_dans});
    }

    void reinitialise()
    {
        substitutions.efface();
    }

    Instruction *instruction_substituee(Instruction *instruction)
    {
        switch (instruction->genre) {
            case Instruction::Genre::CHARGE_MEMOIRE:
            {
                auto charge = instruction->comme_charge();

                POUR (substitutions) {
                    if (it.original == charge->chargee &&
                        (it.substitut_dans & SubstitutDans::CHARGE) != SubstitutDans::ZERO) {
                        charge->chargee = it.substitut;
                        break;
                    }

                    if (it.original == charge) {
                        return static_cast<Instruction *>(it.substitut);
                    }
                }

                return charge;
            }
            case Instruction::Genre::STOCKE_MEMOIRE:
            {
                auto stocke = instruction->comme_stocke_mem();

                POUR (substitutions) {
                    if (it.original == stocke->ou &&
                        (it.substitut_dans & SubstitutDans::ADRESSE_STOCKEE) !=
                            SubstitutDans::ZERO) {
                        stocke->ou = it.substitut;
                    }
                    else if (it.original == stocke->valeur &&
                             (it.substitut_dans & SubstitutDans::VALEUR_STOCKEE) !=
                                 SubstitutDans::ZERO) {
                        stocke->valeur = it.substitut;
                    }
                }

                return stocke;
            }
            case Instruction::Genre::OPERATION_BINAIRE:
            {
                auto op = instruction->comme_op_binaire();

                op->valeur_gauche = valeur_substituee(op->valeur_gauche);
                op->valeur_droite = valeur_substituee(op->valeur_droite);

                //				POUR (substitutions) {
                //					if (it.original == op->valeur_gauche) {
                //						op->valeur_gauche = it.substitut;
                //					}
                //					else if (it.original == op->valeur_droite) {
                //						op->valeur_droite = it.substitut;
                //					}
                //				}

                return op;
            }
            case Instruction::Genre::RETOUR:
            {
                auto retour = instruction->comme_retour();

                if (retour->valeur) {
                    retour->valeur = valeur_substituee(retour->valeur);
                }

                return retour;
            }
            case Instruction::Genre::ACCEDE_MEMBRE:
            {
                auto acces = instruction->comme_acces_membre();
                acces->accede = valeur_substituee(acces->accede);
                return acces;
            }
            case Instruction::Genre::APPEL:
            {
                auto appel = instruction->comme_appel();
                appel->appele = valeur_substituee(appel->appele);

                POUR (appel->args) {
                    it = valeur_substituee(it);
                }

                return appel;
            }
            default:
            {
                return instruction;
            }
        }
    }

    Atome *valeur_substituee(Atome *original)
    {
        POUR (substitutions) {
            if (it.original == original) {
                return it.substitut;
            }
        }

        return original;
    }
};

#undef DEBOGUE_ENLIGNAGE

// À FAIRE : définis de bonnes heuristiques pour l'enlignage
static bool est_candidate_pour_enlignage(AtomeFonction *fonction)
{
    log(std::cerr, "candidate pour enlignage : ", fonction->nom);

    /* appel d'une fonction externe */
    if (fonction->instructions.taille() == 0) {
        log(std::cerr, "-- ignore la candidate car il n'y a pas d'instructions...");
        return false;
    }

    if (fonction->instructions.taille() < 32) {
        return true;
    }

    if (fonction->decl) {
        if (fonction->decl->possède_drapeau(DrapeauxNoeudFonction::FORCE_ENLIGNE)) {
            return true;
        }

        if (fonction->decl->possède_drapeau(DrapeauxNoeudFonction::FORCE_HORSLIGNE)) {
            log(std::cerr, "-- ignore la candidate car nous forçons un horslignage...");
            return false;
        }
    }

    log(std::cerr, "-- ignore la candidate car il y a trop d'instructions...");
    return false;
}

bool enligne_fonctions(ConstructriceRI &constructrice, AtomeFonction *atome_fonc)
{
    auto nouvelle_instructions = kuri::tableau<Instruction *, int>();
    nouvelle_instructions.reserve(atome_fonc->instructions.taille());

    auto substitutrice = Substitutrice();
    auto nombre_labels = 0;
    auto nombre_fonctions_enlignees = 0;

    POUR (atome_fonc->instructions) {
        nombre_labels += it->genre == Instruction::Genre::LABEL;
    }

    POUR (atome_fonc->instructions) {
        if (it->genre != Instruction::Genre::APPEL) {
            nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
            continue;
        }

        auto appel = it->comme_appel();
        auto appele = appel->appele;

        if (appele->genre_atome != Atome::Genre::FONCTION) {
            nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
            continue;
        }

        auto atome_fonc_appelee = static_cast<AtomeFonction *>(appele);

        if (!est_candidate_pour_enlignage(atome_fonc_appelee)) {
            nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
            continue;
        }

        nouvelle_instructions.reserve_delta(atome_fonc_appelee->instructions.taille() + 1);

        // crée une nouvelle adresse retour pour faciliter la suppression de l'instruction de
        // stockage de la valeur de retour dans l'ancienne adresse
        auto adresse_retour = static_cast<InstructionAllocation *>(nullptr);

        if (!appel->type->est_type_rien()) {
            adresse_retour = constructrice.crée_allocation(nullptr, appel->type, nullptr, true);
            nouvelle_instructions.ajoute(adresse_retour);
        }

        auto label_post = constructrice.reserve_label(nullptr);
        label_post->id = nombre_labels++;

        performe_enlignage(constructrice,
                           nouvelle_instructions,
                           atome_fonc_appelee,
                           appel->args,
                           nombre_labels,
                           label_post,
                           adresse_retour);
        nombre_fonctions_enlignees += 1;

        atome_fonc_appelee->nombre_utilisations -= 1;

        nouvelle_instructions.ajoute(label_post);

        if (adresse_retour) {
            // nous ne substituons l'adresse que pour le chargement de sa valeur, ainsi lors du
            // stockage de la valeur l'ancienne adresse aura un compte d'utilisation de zéro et
            // l'instruction de stockage sera supprimée avec l'ancienne adresse dans la passe de
            // suppression de code mort
            auto charge = constructrice.crée_charge_mem(appel->site, adresse_retour, true);
            nouvelle_instructions.ajoute(charge);
            substitutrice.ajoute_substitution(appel, charge, SubstitutDans::VALEUR_STOCKEE);
        }
    }

#ifdef DEBOGUE_ENLIGNAGE
    std::cerr << "===== avant enlignage =====\n";
    imprime_fonction(atome_fonc, std::cerr);
#endif

    atome_fonc->instructions = std::move(nouvelle_instructions);

#ifdef DEBOGUE_ENLIGNAGE
    std::cerr << "===== après enlignage =====\n";
    imprime_fonction(atome_fonc, std::cerr);
#endif

    return nombre_fonctions_enlignees != 0;
}

#undef DEBOGUE_PROPAGATION

/* principalement pour détecter des accès à des membres */
static bool sont_equivalents(Atome *a, Atome *b)
{
    if (a == b) {
        return true;
    }

    if (a->genre_atome != b->genre_atome) {
        return false;
    }

    if (a->est_constante()) {
        auto const_a = static_cast<AtomeConstante *>(a);
        auto const_b = static_cast<AtomeConstante *>(b);

        if (const_a->genre != const_b->genre) {
            return false;
        }

        if (const_a->genre == AtomeConstante::Genre::VALEUR) {
            auto val_a = static_cast<AtomeValeurConstante *>(a);
            auto val_b = static_cast<AtomeValeurConstante *>(b);

            if (val_a->valeur.genre != val_b->valeur.genre) {
                return false;
            }

            if (val_a->valeur.valeur_entiere != val_b->valeur.valeur_entiere) {
                return false;
            }

            return true;
        }
    }

    if (a->est_instruction()) {
        auto inst_a = a->comme_instruction();
        auto inst_b = b->comme_instruction();

        if (inst_a->genre != inst_b->genre) {
            return false;
        }

        if (inst_a->est_acces_membre()) {
            auto ma = inst_a->comme_acces_membre();
            auto mb = inst_b->comme_acces_membre();

            if (!sont_equivalents(ma->accede, mb->accede)) {
                return false;
            }

            if (!sont_equivalents(ma->index, mb->index)) {
                return false;
            }

            return true;
        }
    }

    return false;
}

static bool operandes_sont_constantes(InstructionOpBinaire *op)
{
    if (op->valeur_droite->genre_atome != Atome::Genre::CONSTANTE) {
        return false;
    }

    if (op->valeur_gauche->genre_atome != Atome::Genre::CONSTANTE) {
        return false;
    }

    auto const_a = static_cast<AtomeConstante *>(op->valeur_gauche);
    auto const_b = static_cast<AtomeConstante *>(op->valeur_droite);

    if (const_a->genre != const_b->genre) {
        return false;
    }

    if (const_a->genre != AtomeConstante::Genre::VALEUR) {
        return false;
    }

    auto val_a = static_cast<AtomeValeurConstante *>(op->valeur_gauche);
    auto val_b = static_cast<AtomeValeurConstante *>(op->valeur_droite);

    if (val_a->valeur.genre != val_b->valeur.genre) {
        return false;
    }

    if (val_a->valeur.genre != AtomeValeurConstante::Valeur::Genre::ENTIERE) {
        return false;
    }

    auto va = val_a->valeur.valeur_entiere;
    auto vb = val_b->valeur.valeur_entiere;

    if (op->op == OpérateurBinaire::Genre::Addition) {
        val_a->valeur.valeur_entiere = va + vb;
        return true;
    }

    if (op->op == OpérateurBinaire::Genre::Multiplication) {
        val_a->valeur.valeur_entiere = va * vb;
        return true;
    }

    return false;
}

static bool propage_constantes_et_temporaires(kuri::tableau<Instruction *, int> &instructions)
{
    kuri::tablet<std::pair<Atome *, Atome *>, 16> dernieres_valeurs;
    kuri::tablet<InstructionAccedeMembre *, 16> acces_membres;

    auto renseigne_derniere_valeur = [&](Atome *ptr, Atome *valeur) {
        if (log_actif) {
            std::cerr << "Dernière valeur pour ";
            imprime_atome(ptr, std::cerr);
            std::cerr << " est ";
            imprime_atome(valeur, std::cerr);
            std::cerr << "\n";
        }

        POUR (dernieres_valeurs) {
            if (it.first == ptr) {
                it.second = valeur;
                return;
            }
        }

        dernieres_valeurs.ajoute({ptr, valeur});
    };

    auto substitutrice = Substitutrice();
    auto instructions_subtituees = false;

    POUR (instructions) {
        if (it->est_acces_membre()) {
            auto acces = it->comme_acces_membre();

            auto trouve = false;
            for (auto am : acces_membres) {
                if (sont_equivalents(acces, am)) {
                    substitutrice.ajoute_substitution(acces, am, SubstitutDans::TOUT);
                    trouve = true;
                    break;
                }
            }

            if (!trouve) {
                acces_membres.ajoute(acces);
            }
        }
    }

    POUR (instructions) {
        if (it->genre == Instruction::Genre::STOCKE_MEMOIRE) {
            auto stocke = it->comme_stocke_mem();

            stocke->ou = substitutrice.valeur_substituee(stocke->ou);
            stocke->valeur = substitutrice.valeur_substituee(stocke->valeur);
            renseigne_derniere_valeur(stocke->ou, stocke->valeur);

            for (auto dv : dernieres_valeurs) {
                if (sont_equivalents(dv.first, stocke->ou) && dv.first != stocke->ou) {
                    renseigne_derniere_valeur(dv.first, stocke->valeur);
                    break;
                }
            }
        }
        else if (it->genre == Instruction::Genre::CHARGE_MEMOIRE) {
            auto charge = it->comme_charge();

            for (auto dv : dernieres_valeurs) {
                if (sont_equivalents(dv.first, charge->chargee)) {
                    substitutrice.ajoute_substitution(it, dv.second, SubstitutDans::TOUT);
                    break;
                }
            }
        }
        else if (it->est_branche_cond()) {
            auto branche = it->comme_branche_cond();
            auto condition = branche->condition;

            if (condition->est_instruction()) {
                auto inst = condition->comme_instruction();
                branche->condition = substitutrice.instruction_substituee(inst);
            }
        }
        else if (it->est_op_binaire() && operandes_sont_constantes(it->comme_op_binaire())) {
            auto op = it->comme_op_binaire();
            substitutrice.ajoute_substitution(it, op->valeur_gauche, SubstitutDans::TOUT);
        }
        else {
            auto nouvelle_inst = substitutrice.instruction_substituee(it);

            if (nouvelle_inst != it) {
                instructions_subtituees = true;
            }

            if (nouvelle_inst->est_op_binaire() &&
                operandes_sont_constantes(nouvelle_inst->comme_op_binaire())) {
                auto op = nouvelle_inst->comme_op_binaire();
                substitutrice.ajoute_substitution(it, op->valeur_gauche, SubstitutDans::TOUT);
            }

            /* si nous avons un appel, il est possible que l'appel modifie l'un
             * des paramètres, donc réinitialise les substitutions dans le cas
             * où le programme sauvegarda une valeur avant l'appel */
            if (it->est_appel()) {
                dernieres_valeurs.efface();
                substitutrice.reinitialise();
            }

            it = nouvelle_inst;
        }
    }

    return instructions_subtituees;
}

bool propage_constantes_et_temporaires(kuri::tableau<Bloc *, int> &blocs)
{
    auto constantes_propagees = false;

#ifdef DEBOGUE_PROPAGATION
    std::cerr << "===== avant propagation =====\n";
    imprime_fonction(atome_fonc, std::cerr);
#endif

    POUR (blocs) {
        constantes_propagees |= propage_constantes_et_temporaires(it->instructions);
    }

#ifdef DEBOGUE_PROPAGATION
    std::cerr << "===== après propagation =====\n";
    imprime_fonction(atome_fonc, std::cerr);
#endif

    return constantes_propagees;
}

static void determine_assignations_inutiles(Bloc *bloc)
{
    using paire_atomes = std::pair<Atome *, InstructionStockeMem *>;
    auto anciennes_valeurs = kuri::tablet<paire_atomes, 16>();

    auto indique_valeur_chargee = [&](Atome *atome) {
        POUR (anciennes_valeurs) {
            if (it.first == atome) {
                it.second = nullptr;
                return;
            }
        }

        anciennes_valeurs.ajoute({atome, nullptr});
    };

    auto indique_valeur_stockee = [&](Atome *atome, InstructionStockeMem *stocke) {
        POUR (anciennes_valeurs) {
            if (it.first == atome) {
                if (it.second == nullptr) {
                    it.second = stocke;
                }
                else {
                    it.second->nombre_utilisations -= 1;
                    it.second = stocke;
                }

                return;
            }
        }

        anciennes_valeurs.ajoute({atome, stocke});
    };

    POUR (bloc->instructions) {
        if (it->est_stocke_mem()) {
            auto stocke = it->comme_stocke_mem();
            indique_valeur_stockee(stocke->ou, stocke);
        }
        else if (it->est_charge()) {
            auto charge = it->comme_charge();
            indique_valeur_chargee(charge->chargee);
        }
    }
}

static bool supprime_code_mort(kuri::tableau<Instruction *, int> &instructions)
{
    if (instructions.est_vide()) {
        return false;
    }

    /* rassemble toutes les instructions utilisées au début du tableau d'instructions */
    auto predicat = [](Instruction *inst) { return inst->nombre_utilisations != 0; };
    auto iter = std::stable_partition(instructions.begin(), instructions.end(), predicat);

    if (iter != instructions.end()) {
        /* ne supprime pas la mémoire, nous pourrions en avoir besoin */
        instructions.redimensionne(static_cast<int>(std::distance(instructions.begin(), iter)));
        return true;
    }

    return false;
}

/* Petit algorithme de suppression de code mort.
 *
 * Le code mort est pour le moment défini comme étant toute instruction ne participant pas au
 * résultat final, ou à une branche conditionnelle. Les fonctions ne retournant rien sont
 * considérées comme utile pour le moment, il faudra avoir un système pour détecter les effets
 * secondaire. Les fonctions dont la valeur de retour est ignorée sont supprimées malheureusement,
 * il faudra changer cela avant de tenter d'activer ce code.
 *
 * Il faudra gérer les cas suivants :
 * - inutilisation du retour d'une fonction, mais dont la fonction a des effets secondaires :
 * supprime la temporaire, mais garde la fonction
 * - modification, via un déréférencement, d'un paramètre d'une fonction, sans utiliser celui-ci
 * dans la fonction
 * - modification, via un déréférenecement, d'un pointeur venant d'une fonction sans retourner le
 * pointeur d'une fonction
 * - détecter quand nous avons une variable qui est réassignée
 *
 * une fonction possède des effets secondaires si :
 * -- elle modifie l'un de ses paramètres
 * -- elle possède une boucle ou un controle de flux non constant
 * -- elle est une fonction externe
 * -- elle appel une fonction ayant des effets secondaires
 *
 * erreur non-utilisation d'une variable
 * -- si la variable fût définie par l'utilisateur
 * -- variable définie par le compilateur : les temporaires dans la RI, le contexte implicite, les
 * it et index_it des boucles pour
 *
 * À FAIRE : vérifie que ceci ne vide pas les fonctions sans retour
 */
bool supprime_code_mort(kuri::tableau<Bloc *, int> &blocs)
{
    POUR (blocs) {
        for (auto inst : it->instructions) {
            inst->nombre_utilisations = 0;
        }
    }

    /* performe deux passes, car les boucles « pour » verraient les incrémentations de
     * leurs variables supprimées puisque nous ne marquons la variable comme utilisée
     * que lors de la visite de la condition du bloc après les incrémentations (nous
     * traversons les intructions en arrière pour que seules les dépendances du retour
     * soient considérées) */
    POUR (blocs) {
        marque_instructions_utilisées(it->instructions);
    }

    POUR (blocs) {
        marque_instructions_utilisées(it->instructions);
    }

    // détermine quels stockages sont utilisés
    POUR (blocs) {
        determine_assignations_inutiles(it);
    }

    auto code_mort_supprime = false;

    POUR (blocs) {
        if (log_actif) {
            imprime_bloc(it, 0, std::cerr, true);
        }

        code_mort_supprime |= supprime_code_mort(it->instructions);
    }

    return code_mort_supprime;
}

static int analyse_blocs(kuri::tableau<Bloc *, int> &blocs)
{
    int drapeaux = 0;

    POUR (blocs) {
        if (it->instructions.est_vide()) {
            drapeaux |= REQUIERS_CORRECTION_BLOCS;
            continue;
        }

        if (it->parents.est_vide() && it->enfants.est_vide() && blocs.taille() != 1) {
            drapeaux |= REQUIERS_CORRECTION_BLOCS;
            continue;
        }

        for (auto inst : it->instructions) {
            if (inst->est_branche_cond()) {
                auto branche_cond = inst->comme_branche_cond();

                if (branche_cond->label_si_vrai == branche_cond->label_si_faux) {
                    // condition inutile
                    // drapeaux |= REQUIERS_SUPPRESSION_CODE_MORT;
                }
            }
            else if (inst->est_op_binaire()) {
                // vérifie si les opérandes sont égales
            }
        }
    }

    return drapeaux;
}

static Bloc *trouve_bloc_utilisant_variable(kuri::tableau<Bloc *, int> const &blocs,
                                            Bloc *sauf_lui,
                                            InstructionAllocation *var)
{
    POUR (blocs) {
        if (it == sauf_lui) {
            continue;
        }

        for (auto v : it->variables_utilisees) {
            if (v == var) {
                return it;
            }
        }
    }

    return nullptr;
}

static void detecte_utilisations_variables(kuri::tableau<Bloc *, int> const &blocs)
{
    POUR (blocs) {
        construit_liste_variables_utilisées(it);
    }

    POUR (blocs) {
        for (auto decl : it->variables_declarees) {
            auto utilise = false;

            //			log(std::cerr, "Le bloc ", it->label->id, " déclare ",
            // it->variables_declarees.taille, " variables"); 			log(std::cerr, "Le bloc ",
            // it->label->id, " utilise ", it->variables_utilisees.taille, " variables");

            for (auto var : it->variables_utilisees) {
                if (var == decl) {
                    utilise = true;
                    break;
                }
            }

            if (!utilise) {
                //	log(std::cerr, "Le bloc ", it->label->id, " n'utilise pas la variable ",
                // decl->numero);

                if (decl->blocs_utilisants == 1) {
                    auto autre_bloc = trouve_bloc_utilisant_variable(blocs, it, decl);
                    //	log(std::cerr, "La variable peut être déplacée dans le bloc ",
                    // autre_bloc->label->id);

                    // supprime alloc du bloc
                    auto index = 0;

                    for (auto inst : it->instructions) {
                        if (inst == decl) {
                            break;
                        }

                        index += 1;
                    }

                    std::rotate(it->instructions.donnees() + index,
                                it->instructions.donnees() + index + 1,
                                it->instructions.donnees() + it->instructions.taille());
                    it->instructions.supprime_dernier();

                    // ajoute alloc dans bloc
                    autre_bloc->instructions.pousse_front(decl);
                    autre_bloc->variables_declarees.ajoute(decl);
                }
            }
            else {
                if (decl->blocs_utilisants <= 1) {
                    log(std::cerr, "La variable est une temporaire");
                }
            }
        }
    }
}

static void performe_passes_optimisation(kuri::tableau<Bloc *, int> &blocs)
{
    while (true) {
        if (log_actif) {
            imprime_blocs(blocs, std::cerr);
        }

        //		auto drapeaux = analyse_blocs(blocs);

        //		if (drapeaux == 0) {
        //			break;
        //		}

        int drapeaux = 0;

        auto travail_effectue = false;

        if ((drapeaux & REQUIERS_ENLIGNAGE) == REQUIERS_ENLIGNAGE) {
            // performe enlignage
            drapeaux &= REQUIERS_ENLIGNAGE;
        }

        if ((drapeaux & REQUIERS_DEBOUCLAGE) == REQUIERS_DEBOUCLAGE) {
            // performe débouclage
            drapeaux &= REQUIERS_DEBOUCLAGE;
        }

        // if ((drapeaux & REQUIERS_CORRECTION_BLOCS) == REQUIERS_CORRECTION_BLOCS) {
        // travail_effectue |= elimine_branches_inutiles(blocs);
        // drapeaux &= REQUIERS_CORRECTION_BLOCS;
        //}

        // supprime_temporaires(blocs);

        // if ((drapeaux & REQUIERS_PROPAGATION_CONSTANTES) == REQUIERS_PROPAGATION_CONSTANTES) {
        travail_effectue |= propage_constantes_et_temporaires(blocs);
        // drapeaux &= REQUIERS_PROPAGATION_CONSTANTES;
        //}

        // if ((drapeaux & REQUIERS_SUPPRESSION_CODE_MORT) == REQUIERS_SUPPRESSION_CODE_MORT) {
        travail_effectue |= supprime_code_mort(blocs);
        // drapeaux &= REQUIERS_SUPPRESSION_CODE_MORT;
        //}

        if (!travail_effectue) {
            break;
        }
    }
}

void optimise_code(EspaceDeTravail &espace,
                   ConstructriceRI &constructrice,
                   AtomeFonction *atome_fonc)
{
    // if (atome_fonc->nom ==
    // "_KF9Fondation14imprime_chaine_P0__E2_8contexte19KsContexteProgramme6format8Kschaine4args8KtKseini_S1_8Kschaine")
    // { 	std::cerr << "========= optimisation pour " << atome_fonc->nom << " =========\n";
    //	active_log();
    //}

    // while (enligne_fonctions(constructrice, atome_fonc)) {}
    enligne_fonctions(constructrice, atome_fonc);

    FonctionEtBlocs fonction_et_blocs;
    if (!fonction_et_blocs.convertis_en_blocs(espace, atome_fonc)) {
        return;
    }

    performe_passes_optimisation(fonction_et_blocs.blocs);

    transfère_instructions_blocs_à_fonction(fonction_et_blocs.blocs, atome_fonc);

    desactive_log();
}
