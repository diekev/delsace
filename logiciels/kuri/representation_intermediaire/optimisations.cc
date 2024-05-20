/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "optimisations.hh"

#include <iostream>

#include "arbre_syntaxique/noeud_expression.hh"

#include "utilitaires/log.hh"

#include "parsage/identifiant.hh"

#include "structures/table_hachage.hh"
#include "structures/tablet.hh"

#include "analyse.hh"
#include "bloc_basique.hh"
#include "constructrice_ri.hh"
#include "impression.hh"
#include "instructions.hh"

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

  Autres
 - détecte les blocs ne faisant que retourner (sans valeur) et remplace les branches vers ceux-ci
 par un retour
 - détecte les vérifications >= 0 sur des nombre naturels, remplace par vrai, supprime branches
 - expression du style (a & 16) == 10 => toujours faux (peut être généraliser avec un système de
 mathématiques d'intervalles)
 - détection de branches similaires (p.e. a == b && a == b)
 - détection de branches consécutives sur des valeurs consécutives (p.e. discrimination
 d'énumérations)
 - a <= b && b <= c -> (b - a) >= (c - a)
 */

static bool log_actif = false;

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
 * CompilatriceRI)
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
        kuri::tableau<Instruction *, int> résultat;
        résultat.réserve(atome_fonction->instructions.taille());

        POUR (atome_fonction->instructions) {
            // s'il existe une substition pour cette instruction, ignore-là
            if (!it->est_label() && copies.possède(it)) {
                continue;
            }

            résultat.ajoute(static_cast<Instruction *>(copie_atome(it)));
        }

        return résultat;
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
            case GenreInstruction::APPEL:
            {
                auto appel = inst->comme_appel();
                auto appelé = copie_atome(appel->appelé);

                kuri::tableau<Atome *, int> args;
                args.réserve(appel->args.taille());
                POUR (appel->args) {
                    args.ajoute(copie_atome(it));
                }

                auto nouvel_appel = constructrice.crée_appel(inst->site, appelé, std::move(args));
                nouvelle_inst = nouvel_appel;
                break;
            }
            case GenreInstruction::CHARGE_MEMOIRE:
            {
                auto charge = inst->comme_charge();
                auto source = copie_atome(charge->chargée);
                auto n_charge = constructrice.crée_charge_mem(inst->site, source);
                nouvelle_inst = n_charge;
                break;
            }
            case GenreInstruction::STOCKE_MEMOIRE:
            {
                auto stocke = inst->comme_stocke_mem();
                auto destination = copie_atome(stocke->destination);
                auto source = copie_atome(stocke->source);
                auto n_stocke = constructrice.crée_stocke_mem(inst->site, destination, source);
                nouvelle_inst = n_stocke;
                break;
            }
            case GenreInstruction::OPERATION_UNAIRE:
            {
                auto op = inst->comme_op_unaire();
                auto type_opération = op->op;
                auto n_valeur = copie_atome(op->valeur);
                auto n_op = constructrice.crée_op_unaire(
                    inst->site, inst->type, type_opération, n_valeur);
                nouvelle_inst = n_op->comme_instruction();
                break;
            }
            case GenreInstruction::OPERATION_BINAIRE:
            {
                auto op = inst->comme_op_binaire();
                auto type_opération = op->op;
                auto valeur_gauche = copie_atome(op->valeur_gauche);
                auto valeur_droite = copie_atome(op->valeur_droite);
                auto n_op = constructrice.crée_op_binaire(
                    inst->site, inst->type, type_opération, valeur_gauche, valeur_droite);
                nouvelle_inst = n_op->comme_instruction();
                break;
            }
            case GenreInstruction::ACCEDE_INDEX:
            {
                auto acces = inst->comme_acces_index();
                auto accedé = copie_atome(acces->accédé);
                auto index = copie_atome(acces->index);
                auto n_acces = constructrice.crée_accès_index(inst->site, accedé, index);
                nouvelle_inst = n_acces;
                break;
            }
            case GenreInstruction::ACCEDE_MEMBRE:
            {
                auto acces = inst->comme_acces_membre();
                auto accedé = copie_atome(acces->accédé);
                auto index = acces->index;
                auto n_acces = constructrice.crée_référence_membre(
                    inst->site, inst->type, accedé, index);
                nouvelle_inst = n_acces;
                break;
            }
            case GenreInstruction::TRANSTYPE:
            {
                auto transtype = inst->comme_transtype();
                auto op = transtype->op;
                auto n_valeur = copie_atome(transtype->valeur);
                auto n_transtype = constructrice.crée_transtype(
                    inst->site, inst->type, n_valeur, op);
                nouvelle_inst = n_transtype->comme_instruction()->comme_transtype();
                break;
            }
            case GenreInstruction::BRANCHE_CONDITION:
            {
                auto branche = inst->comme_branche_cond();
                auto n_condition = copie_atome(branche->condition);
                auto label_si_vrai =
                    copie_atome(branche->label_si_vrai)->comme_instruction()->comme_label();
                auto label_si_faux =
                    copie_atome(branche->label_si_faux)->comme_instruction()->comme_label();
                auto n_branche = constructrice.crée_branche_condition(
                    inst->site, n_condition, label_si_vrai, label_si_faux);
                nouvelle_inst = n_branche;
                break;
            }
            case GenreInstruction::BRANCHE:
            {
                auto branche = inst->comme_branche();
                auto label = copie_atome(branche->label)->comme_instruction()->comme_label();
                auto n_branche = constructrice.crée_branche(inst->site, label);
                nouvelle_inst = n_branche;
                break;
            }
            case GenreInstruction::RETOUR:
            {
                auto retour = inst->comme_retour();
                auto n_valeur = copie_atome(retour->valeur);
                auto n_retour = constructrice.crée_retour(inst->site, n_valeur);
                nouvelle_inst = n_retour;
                break;
            }
            case GenreInstruction::ALLOCATION:
            {
                auto alloc = inst->comme_alloc();
                auto ident = alloc->ident;
                auto n_alloc = constructrice.crée_allocation(
                    inst->site, alloc->donne_type_alloué(), ident, true);
                nouvelle_inst = n_alloc;
                break;
            }
            case GenreInstruction::LABEL:
            {
                auto label = inst->comme_label();
                auto n_label = constructrice.crée_label(inst->site);
                n_label->id = label->id;
                nouvelle_inst = n_label;
                break;
            }
            case GenreInstruction::INATTEIGNABLE:
            {
                nouvelle_inst = constructrice.crée_inatteignable(inst->site, true);
                break;
            }
            case GenreInstruction::SÉLECTION:
            {
                auto const sélection = inst->comme_sélection();
                auto nouvelle_sélection = constructrice.crée_sélection(inst->site, true);
                nouvelle_sélection->type = sélection->type;
                nouvelle_sélection->condition = sélection->condition;
                nouvelle_sélection->si_vrai = sélection->si_vrai;
                nouvelle_sélection->si_faux = sélection->si_faux;
                nouvelle_inst = nouvelle_sélection;
                break;
            }
        }

        if (nouvelle_inst) {
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

    for (auto i = 0; i < fonction_appelee->params_entrée.taille(); ++i) {
        auto parametre = fonction_appelee->params_entrée[i];
        auto atome = arguments[i];

        // À FAIRE : il faudrait que tous les arguments des fonctions soient des instructions (->
        // utilisation de temporaire)
        if (atome->genre_atome == Atome::Genre::INSTRUCTION) {
            auto inst = atome->comme_instruction();

            if (inst->genre == GenreInstruction::CHARGE_MEMOIRE) {
                atome = inst->comme_charge()->chargée;
            }
            // À FAIRE : détection des pointeurs locaux plus robuste
            // détecte les cas où nous avons une référence à une variable
            else if (inst->est_alloc()) {
                auto type_pointe = inst->comme_alloc()->donne_type_alloué();
                if (type_pointe != atome->type) {
                    // remplace l'instruction de déréférence par l'atome
                    POUR (fonction_appelee->instructions) {
                        if (it->est_charge()) {
                            auto charge = it->comme_charge();

                            if (charge->chargée == parametre) {
                                copieuse.ajoute_substitution(charge, atome);
                            }
                        }
                    }
                }
            }
        }
        else if (atome->est_constante()) {
            POUR (fonction_appelee->instructions) {
                if (it->est_charge()) {
                    auto charge = it->comme_charge();

                    if (charge->chargée == parametre) {
                        copieuse.ajoute_substitution(charge, atome);
                    }
                }
            }
        }

        copieuse.ajoute_substitution(parametre, atome);
    }

    auto instructions_copiees = copieuse.copie_instructions(fonction_appelee);
    nouvelles_instructions.réserve_delta(instructions_copiees.taille());

    POUR (instructions_copiees) {
        if (it->genre == GenreInstruction::LABEL) {
            auto label = it->comme_label();

            // saute le label d'entrée de la fonction
            if (label->id == 0) {
                continue;
            }

            label->id = nombre_labels++;
        }
        else if (it->genre == GenreInstruction::RETOUR) {
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
            dbg() << "Subtitut : " << imprime_atome(original) << " avec "
                  << imprime_atome(substitut);
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
            case GenreInstruction::CHARGE_MEMOIRE:
            {
                auto charge = instruction->comme_charge();

                POUR (substitutions) {
                    if (it.original == charge->chargée &&
                        (it.substitut_dans & SubstitutDans::CHARGE) != SubstitutDans::ZERO) {
                        charge->chargée = it.substitut;
                        break;
                    }

                    if (it.original == charge) {
                        return static_cast<Instruction *>(it.substitut);
                    }
                }

                return charge;
            }
            case GenreInstruction::STOCKE_MEMOIRE:
            {
                auto stocke = instruction->comme_stocke_mem();

                POUR (substitutions) {
                    if (it.original == stocke->destination &&
                        (it.substitut_dans & SubstitutDans::ADRESSE_STOCKEE) !=
                            SubstitutDans::ZERO) {
                        stocke->destination = it.substitut;
                    }
                    else if (it.original == stocke->source &&
                             (it.substitut_dans & SubstitutDans::VALEUR_STOCKEE) !=
                                 SubstitutDans::ZERO) {
                        stocke->source = it.substitut;
                    }
                }

                return stocke;
            }
            case GenreInstruction::OPERATION_BINAIRE:
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
            case GenreInstruction::RETOUR:
            {
                auto retour = instruction->comme_retour();

                if (retour->valeur) {
                    retour->valeur = valeur_substituee(retour->valeur);
                }

                return retour;
            }
            case GenreInstruction::ACCEDE_MEMBRE:
            {
                auto acces = instruction->comme_acces_membre();
                acces->accédé = valeur_substituee(acces->accédé);
                return acces;
            }
            case GenreInstruction::APPEL:
            {
                auto appel = instruction->comme_appel();
                appel->appelé = valeur_substituee(appel->appelé);

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
    dbg() << "candidate pour enlignage : " << fonction->nom;

    /* appel d'une fonction externe */
    if (fonction->instructions.taille() == 0) {
        dbg() << "-- ignore la candidate car il n'y a pas d'instructions...";
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
            dbg() << "-- ignore la candidate car nous forçons un horslignage...";
            return false;
        }
    }

    dbg() << "-- ignore la candidate car il y a trop d'instructions...";
    return false;
}

bool enligne_fonctions(ConstructriceRI &constructrice, AtomeFonction *atome_fonc)
{
    auto nouvelle_instructions = kuri::tableau<Instruction *, int>();
    nouvelle_instructions.réserve(atome_fonc->instructions.taille());

    auto substitutrice = Substitutrice();
    auto nombre_labels = 0;
    auto nombre_fonctions_enlignees = 0;

    POUR (atome_fonc->instructions) {
        nombre_labels += it->genre == GenreInstruction::LABEL;
    }

    POUR (atome_fonc->instructions) {
        if (it->genre != GenreInstruction::APPEL) {
            nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
            continue;
        }

        auto appel = it->comme_appel();
        auto appele = appel->appelé;

        if (appele->genre_atome != Atome::Genre::FONCTION) {
            nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
            continue;
        }

        auto atome_fonc_appelee = appele->comme_fonction();

        if (!est_candidate_pour_enlignage(atome_fonc_appelee)) {
            nouvelle_instructions.ajoute(substitutrice.instruction_substituee(it));
            continue;
        }

        nouvelle_instructions.réserve_delta(atome_fonc_appelee->instructions.taille() + 1);

        // crée une nouvelle adresse retour pour faciliter la suppression de l'instruction de
        // stockage de la valeur de retour dans l'ancienne adresse
        auto adresse_retour = static_cast<InstructionAllocation *>(nullptr);

        if (!appel->type->est_type_rien()) {
            adresse_retour = constructrice.crée_allocation(nullptr, appel->type, nullptr, true);
            nouvelle_instructions.ajoute(adresse_retour);
        }

        auto label_post = constructrice.réserve_label(nullptr);
        label_post->id = nombre_labels++;

        performe_enlignage(constructrice,
                           nouvelle_instructions,
                           atome_fonc_appelee,
                           appel->args,
                           nombre_labels,
                           label_post,
                           adresse_retour);
        nombre_fonctions_enlignees += 1;

        // atome_fonc_appelee->nombre_utilisations -= 1;

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
    dbg() << "===== avant enlignage =====\n" << imprime_fonction(atome_fonc);
#endif

    atome_fonc->instructions = std::move(nouvelle_instructions);

#ifdef DEBOGUE_ENLIGNAGE
    dbg() << "===== après enlignage =====\n" << imprime_fonction(atome_fonc);
#endif

    return nombre_fonctions_enlignees != 0;
}

#undef DEBOGUE_PROPAGATION

/* principalement pour détecter des accès à des membres */
#if 0
static bool sont_equivalents(Atome *a, Atome *b)
{
    if (a == b) {
        return true;
    }

    if (a->genre_atome != b->genre_atome) {
        return false;
    }

    if (a->est_constante_entière()) {
        auto const_a = a->comme_constante_entière();
        auto const_b = b->comme_constante_entière();
        return const_a->valeur == const_b->valeur;
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

            if (!sont_equivalents(ma->accédé, mb->accédé)) {
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
#endif

static void performe_passes_optimisation(kuri::tableau<Bloc *, int> &blocs)
{
    while (true) {
        if (log_actif) {
            dbg() << imprime_blocs(blocs);
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
        // travail_effectue |= propage_constantes_et_temporaires(blocs);
        // drapeaux &= REQUIERS_PROPAGATION_CONSTANTES;
        //}

        // if ((drapeaux & REQUIERS_SUPPRESSION_CODE_MORT) == REQUIERS_SUPPRESSION_CODE_MORT) {
        // travail_effectue |= supprime_code_mort(blocs);
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
    // { 	dbg() << "========= optimisation pour " << atome_fonc->nom << " =========";
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

    // desactive_log();
}
