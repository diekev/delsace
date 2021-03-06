﻿/*
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

#include "constructrice_ri.hh"

#include <fstream>

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/sauvegardeuse_etat.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "compilatrice.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "parsage/outils_lexemes.hh"
#include "statistiques/statistiques.hh"

#include "analyse.hh"
#include "impression.hh"
#include "optimisations.hh"

#include "structures/enchaineuse.hh"

/* À FAIRE : (représentation intermédiaire, non-urgent)
 * - copie les tableaux fixes quand nous les assignations (a = b -> copie_mem(a, b))
 * - crée une branche lors de l'insertion d'un label si la dernière instruction
 *   n'en est pas une (pour mimiquer le comportement des blocs de LLVM)
 */

/* ************************************************************************** */

#define IDENT_CODE(x) m_compilatrice.table_identifiants->identifiant_pour_chaine((x))

ConstructriceRI::ConstructriceRI(Compilatrice &compilatrice) : m_compilatrice(compilatrice)
{
}

ConstructriceRI::~ConstructriceRI()
{
}

void ConstructriceRI::genere_ri_pour_noeud(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    m_espace = espace;
    genere_ri_pour_noeud(noeud);
}

void ConstructriceRI::genere_ri_pour_fonction_metaprogramme(
    EspaceDeTravail *espace, NoeudDeclarationEnteteFonction *fonction)
{
    m_espace = espace;
    genere_ri_pour_fonction_metaprogramme(fonction);
}

AtomeFonction *ConstructriceRI::genere_ri_pour_fonction_principale(EspaceDeTravail *espace)
{
    m_espace = espace;
    return genere_ri_pour_fonction_principale();
}

AtomeFonction *ConstructriceRI::genere_fonction_init_globales_et_appel(
    EspaceDeTravail *espace,
    const kuri::tableau<AtomeGlobale *> &globales,
    AtomeFonction *fonction_pour)
{
    m_espace = espace;
    return genere_fonction_init_globales_et_appel(globales, fonction_pour);
}

AtomeConstante *ConstructriceRI::cree_constante_entiere(Type *type, unsigned long long valeur)
{
    return atomes_constante.ajoute_element(type, valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_type(Type *pointeur_type)
{
    return atomes_constante.ajoute_element(m_espace->typeuse.type_type_de_donnees_, pointeur_type);
}

AtomeConstante *ConstructriceRI::cree_z32(unsigned long long valeur)
{
    return cree_constante_entiere(m_espace->typeuse[TypeBase::Z32], valeur);
}

AtomeConstante *ConstructriceRI::cree_z64(unsigned long long valeur)
{
    return cree_constante_entiere(m_espace->typeuse[TypeBase::Z64], valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_reelle(Type *type, double valeur)
{
    return atomes_constante.ajoute_element(type, valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_structure(
    Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    return atomes_constante.ajoute_element(type, std::move(valeurs));
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_fixe(
    Type *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto atome = atomes_constante.ajoute_element(type, std::move(valeurs));
    atome->valeur.genre = AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE;
    return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_donnees_constantes(
    Type *type, kuri::tableau<char> &&donnees_constantes)
{
    return atomes_constante.ajoute_element(type, std::move(donnees_constantes));
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_donnees_constantes(Type *type,
                                                                           char *pointeur,
                                                                           long taille)
{
    return atomes_constante.ajoute_element(type, pointeur, taille);
}

AtomeGlobale *ConstructriceRI::cree_globale(Type *type,
                                            AtomeConstante *initialisateur,
                                            bool est_externe,
                                            bool est_constante)
{
    return m_espace->cree_globale(type, initialisateur, est_externe, est_constante);
}

AtomeConstante *ConstructriceRI::cree_tableau_global(Type *type,
                                                     kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto taille_tableau = static_cast<int>(valeurs.taille());
    auto type_tableau = m_espace->typeuse.type_tableau_fixe(type, taille_tableau);
    auto tableau_fixe = cree_constante_tableau_fixe(type_tableau, std::move(valeurs));

    return cree_tableau_global(tableau_fixe);
}

AtomeConstante *ConstructriceRI::cree_tableau_global(AtomeConstante *tableau_fixe)
{
    auto type_tableau_fixe = tableau_fixe->type->comme_tableau_fixe();
    auto globale_tableau_fixe = cree_globale(type_tableau_fixe, tableau_fixe, false, true);
    auto ptr_premier_element = cree_acces_index_constant(globale_tableau_fixe, cree_z64(0));
    auto valeur_taille = cree_z64(static_cast<unsigned>(type_tableau_fixe->taille));
    auto type_tableau_dyn = m_espace->typeuse.type_tableau_dynamique(
        type_tableau_fixe->type_pointe);

    auto membres = kuri::tableau<AtomeConstante *>(3);
    membres[0] = ptr_premier_element;
    membres[1] = valeur_taille;
    membres[2] = valeur_taille;

    return cree_constante_structure(type_tableau_dyn, std::move(membres));
}

AtomeConstante *ConstructriceRI::cree_constante_booleenne(bool valeur)
{
    return atomes_constante.ajoute_element(m_espace->typeuse[TypeBase::BOOL], valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_caractere(Type *type, unsigned long long valeur)
{
    auto atome = atomes_constante.ajoute_element(type, valeur);
    atome->valeur.genre = AtomeValeurConstante::Valeur::Genre::CARACTERE;
    return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_nulle(Type *type)
{
    return atomes_constante.ajoute_element(type);
}

InstructionBranche *ConstructriceRI::cree_branche(NoeudExpression *site_,
                                                  InstructionLabel *label,
                                                  bool cree_seulement)
{
    auto inst = insts_branche.ajoute_element(site_, label);

    if (!cree_seulement) {
        fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

InstructionBrancheCondition *ConstructriceRI::cree_branche_condition(
    NoeudExpression *site_,
    Atome *valeur,
    InstructionLabel *label_si_vrai,
    InstructionLabel *label_si_faux)
{
    auto inst = insts_branche_condition.ajoute_element(
        site_, valeur, label_si_vrai, label_si_faux);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionLabel *ConstructriceRI::cree_label(NoeudExpression *site_)
{
    auto inst = insts_label.ajoute_element(site_, nombre_labels++);
    insere_label(inst);
    return inst;
}

InstructionLabel *ConstructriceRI::reserve_label(NoeudExpression *site_)
{
    return insts_label.ajoute_element(site_, nombre_labels++);
}

void ConstructriceRI::insere_label(InstructionLabel *label)
{
    fonction_courante->instructions.ajoute(label);
}

InstructionRetour *ConstructriceRI::cree_retour(NoeudExpression *site_, Atome *valeur)
{
    auto inst = insts_retour.ajoute_element(site_, valeur);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

AtomeFonction *ConstructriceRI::genere_fonction_init_globales_et_appel(
    const kuri::tableau<AtomeGlobale *> &globales, AtomeFonction *fonction_pour)
{
    auto nom_fontion = enchaine("init_globale", fonction_pour);

    auto types_entrees = dls::tablet<Type *, 6>(1);
    types_entrees[0] = m_espace->typeuse.type_contexte;

    auto type_sortie = m_espace->typeuse[TypeBase::RIEN];

    Atome *param_contexte = cree_allocation(nullptr, types_entrees[0], ID::contexte);

    auto params = kuri::tableau<Atome *, int>(1);
    params[0] = param_contexte;

    auto fonction = m_espace->cree_fonction(nullptr, nom_fontion, std::move(params));
    fonction->type = m_espace->typeuse.type_fonction(types_entrees, type_sortie, false);

    this->fonction_courante = fonction;
    this->m_pile.efface();

    auto constructeurs = m_espace->constructeurs_globaux.verrou_lecture();

    POUR (globales) {
        for (auto &constructeur : *constructeurs) {
            if (it != constructeur.atome) {
                continue;
            }

            genere_ri_transformee_pour_noeud(
                constructeur.expression, nullptr, constructeur.transformation);
            auto valeur = depile_valeur();

            if (!valeur) {
                continue;
            }

            cree_stocke_mem(nullptr, it, valeur);
        }
    }

    cree_retour(nullptr, nullptr);

    // crée l'appel de cette fonction et ajoute là au début de la fonction_pour

    this->fonction_courante = fonction_pour;
    param_contexte = nullptr;

    POUR (fonction_pour->instructions) {
        if (it->est_alloc() && it->comme_alloc()->ident == ID::contexte) {
            param_contexte = it;
            break;
        }
    }

    auto param_appel = kuri::tableau<Atome *, int>(1);
    param_appel[0] = cree_charge_mem(nullptr, param_contexte);

    cree_appel(nullptr, nullptr, fonction, std::move(param_appel));

    std::rotate(fonction_pour->instructions.begin() + fonction_pour->decalage_appel_init_globale +
                    1,
                fonction_pour->instructions.end() - 2,
                fonction_pour->instructions.end());

    this->fonction_courante = nullptr;

    return fonction;
}

InstructionAllocation *ConstructriceRI::cree_allocation(NoeudExpression *site_,
                                                        Type *type,
                                                        IdentifiantCode *ident,
                                                        bool cree_seulement)
{
    /* le résultat d'une instruction d'allocation est l'adresse de la variable. */
    type = normalise_type(m_espace->typeuse, type);
    assert_rappel(type == nullptr || (type->drapeaux & TYPE_EST_NORMALISE) != 0, [=]() {
        std::cerr << "Le type '" << chaine_type(type) << "' n'est pas normalisé\n";
    });
    auto type_pointeur = m_espace->typeuse.type_pointeur_pour(type, false);
    auto inst = insts_allocation.ajoute_element(site_, type_pointeur, ident);
    inst->decalage_pile = taille_allouee;
    taille_allouee += (type == nullptr) ? 8 : static_cast<int>(type->taille_octet);

    /* Nous utilisons pour l'instant cree_allocation pour les paramètres des
     * fonctions, et la fonction_courante est nulle lors de cette opération.
     */
    if (fonction_courante && !cree_seulement) {
        fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

bool est_reference_compatible_pointeur(Type *type_pointeur, Type *type_ref)
{
    if (!type_ref->est_reference()) {
        return false;
    }

    if (type_ref->comme_reference()->type_pointe != type_pointeur->comme_pointeur()->type_pointe) {
        return false;
    }

    return true;
}

static bool est_type_opacifie(Type *type_pointe, Type *valeur)
{
    return type_pointe->est_opaque() && type_pointe->comme_opaque()->type_opacifie == valeur;
}

InstructionStockeMem *ConstructriceRI::cree_stocke_mem(NoeudExpression *site_,
                                                       Atome *ou,
                                                       Atome *valeur,
                                                       bool cree_seulement)
{
    assert_rappel(ou->type->genre == GenreType::POINTEUR, [&]() {
        std::cerr << "Le type n'est pas un pointeur : " << chaine_type(ou->type) << '\n';
        erreur::imprime_site(*m_espace, site_);
    });

    auto type_pointeur = ou->type->comme_pointeur();
    assert_rappel(
        type_pointeur->type_pointe == valeur->type ||
            est_reference_compatible_pointeur(type_pointeur->type_pointe, valeur->type) ||
            (type_pointeur->type_pointe->genre == GenreType::TYPE_DE_DONNEES &&
             type_pointeur->type_pointe->genre == valeur->type->genre) ||
            est_type_opacifie(type_pointeur->type_pointe, valeur->type),
        [&]() {
            std::cerr << "\ttype_pointeur->type_pointe : "
                      << chaine_type(type_pointeur->type_pointe) << " ("
                      << type_pointeur->type_pointe << ") "
                      << ", valeur->type : " << chaine_type(valeur->type) << " (" << valeur->type
                      << ") " << '\n';

            erreur::imprime_site(*m_espace, site_);
        });

    auto type = valeur->type;
    // assert_rappel((type->drapeaux & TYPE_EST_NORMALISE) != 0, [=](){ std::cerr << "Le type '" <<
    // chaine_type(type) << "' n'est pas normalisé\n"; });

    auto inst = insts_stocke_memoire.ajoute_element(site_, type, ou, valeur);

    if (!cree_seulement) {
        fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

InstructionChargeMem *ConstructriceRI::cree_charge_mem(NoeudExpression *site_,
                                                       Atome *ou,
                                                       bool cree_seulement)
{
    /* nous chargeons depuis une adresse en mémoire, donc nous devons avoir un pointeur */
    assert_rappel(
        ou->type->genre == GenreType::POINTEUR || ou->type->genre == GenreType::REFERENCE, [&]() {
            std::cerr << "Le type est '" << chaine_type(ou->type) << "'\n";
            erreur::imprime_site(*m_espace, site_);
        });
    auto type_pointeur = ou->type->comme_pointeur();

    assert_rappel(
        ou->genre_atome == Atome::Genre::INSTRUCTION || ou->genre_atome == Atome::Genre::GLOBALE,
        [=]() {
            std::cerr << "Le genre de l'atome est : " << static_cast<int>(ou->genre_atome)
                      << ".\n";
        });

    auto type = type_pointeur->type_pointe;
    // assert_rappel((type->drapeaux & TYPE_EST_NORMALISE) != 0, [=](){ std::cerr << "Le type '" <<
    // chaine_type(type) << "' n'est pas normalisé\n"; });

    auto inst = insts_charge_memoire.ajoute_element(site_, type, ou);

    if (!cree_seulement) {
        fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

InstructionAppel *ConstructriceRI::cree_appel(NoeudExpression *site_,
                                              Lexeme const *lexeme,
                                              Atome *appele)
{
    // incrémente le nombre d'utilisation au cas où nous appelerions une fonction
    // lorsque que nous enlignons une fonction, son nombre d'utilisations sera décrémentée,
    // et si à 0, nous pourrons ignorer la génération de code final pour celle-ci
    appele->nombre_utilisations += 1;
    auto inst = insts_appel.ajoute_element(site_, lexeme, appele);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionAppel *ConstructriceRI::cree_appel(NoeudExpression *site_,
                                              Lexeme const *lexeme,
                                              Atome *appele,
                                              kuri::tableau<Atome *, int> &&args)
{
    // voir commentaire plus haut
    appele->nombre_utilisations += 1;
    auto inst = insts_appel.ajoute_element(site_, lexeme, appele, std::move(args));
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionOpUnaire *ConstructriceRI::cree_op_unaire(NoeudExpression *site_,
                                                     Type *type,
                                                     OperateurUnaire::Genre op,
                                                     Atome *valeur)
{
    auto inst = insts_opunaire.ajoute_element(site_, type, op, valeur);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionOpBinaire *ConstructriceRI::cree_op_binaire(NoeudExpression *site_,
                                                       Type *type,
                                                       OperateurBinaire::Genre op,
                                                       Atome *valeur_gauche,
                                                       Atome *valeur_droite)
{
    auto inst = insts_opbinaire.ajoute_element(site_, type, op, valeur_gauche, valeur_droite);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionOpBinaire *ConstructriceRI::cree_op_comparaison(NoeudExpression *site_,
                                                           OperateurBinaire::Genre op,
                                                           Atome *valeur_gauche,
                                                           Atome *valeur_droite)
{
    return cree_op_binaire(
        site_, m_espace->typeuse[TypeBase::BOOL], op, valeur_gauche, valeur_droite);
}

InstructionAccedeIndex *ConstructriceRI::cree_acces_index(NoeudExpression *site_,
                                                          Atome *accede,
                                                          Atome *index)
{
    auto type_pointe = static_cast<Type *>(nullptr);
    if (accede->genre_atome == Atome::Genre::CONSTANTE) {
        type_pointe = accede->type;
    }
    else {
        assert_rappel(accede->type->genre == GenreType::POINTEUR, [=]() {
            std::cerr << "Type accédé : '" << chaine_type(accede->type) << "'\n";
        });
        auto type_pointeur = accede->type->comme_pointeur();
        type_pointe = type_pointeur->type_pointe;
    }

    assert_rappel(dls::outils::est_element(
                      type_pointe->genre, GenreType::POINTEUR, GenreType::TABLEAU_FIXE) ||
                      (type_pointe->est_opaque() &&
                       dls::outils::est_element(type_pointe->comme_opaque()->type_opacifie->genre,
                                                GenreType::POINTEUR,
                                                GenreType::TABLEAU_FIXE)),
                  [=]() { std::cerr << "Type accédé : '" << chaine_type(accede->type) << "'\n"; });

    auto type = m_espace->typeuse.type_pointeur_pour(type_dereference_pour(type_pointe), false);

    // assert_rappel((type->drapeaux & TYPE_EST_NORMALISE) != 0, [=](){ std::cerr << "Le type '" <<
    // chaine_type(type) << "' n'est pas normalisé\n"; });

    auto inst = insts_accede_index.ajoute_element(site_, type, accede, index);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionAccedeMembre *ConstructriceRI::cree_reference_membre(NoeudExpression *site_,
                                                                Atome *accede,
                                                                int index,
                                                                bool cree_seulement)
{
    assert_rappel(accede->type->genre == GenreType::POINTEUR ||
                      accede->type->genre == GenreType::REFERENCE,
                  [=]() { std::cerr << "Type accédé : '" << chaine_type(accede->type) << "'\n"; });
    auto type_pointeur = accede->type->comme_pointeur();

    auto type_pointe = type_pointeur->type_pointe;
    if (type_pointe->est_opaque()) {
        type_pointe = type_pointe->comme_opaque()->type_opacifie;
    }

    assert_rappel(est_type_compose(type_pointe), [&]() {
        std::cerr << "Type accédé : '" << chaine_type(type_pointe) << "'\n";
        erreur::imprime_site(*espace(), site_);
    });
    assert_rappel(type_pointe->genre != GenreType::UNION,
                  [=]() { std::cerr << "Type accédé : '" << chaine_type(type_pointe) << "'\n"; });

    auto type_compose = static_cast<TypeCompose *>(type_pointe);
    auto type = type_compose->membres[index].type;

    /* nous retournons un pointeur vers le membre */
    type = m_espace->typeuse.type_pointeur_pour(type, false);
    // assert_rappel((type->drapeaux & TYPE_EST_NORMALISE) != 0, [=](){ std::cerr << "Le type '" <<
    // chaine_type(type) << "' n'est pas normalisé\n"; });

    auto inst = insts_accede_membre.ajoute_element(
        site_, type, accede, cree_z64(static_cast<unsigned>(index)));
    if (!cree_seulement) {
        fonction_courante->instructions.ajoute(inst);
    }
    return inst;
}

Instruction *ConstructriceRI::cree_reference_membre_et_charge(NoeudExpression *site_,
                                                              Atome *accede,
                                                              int index)
{
    auto inst = cree_reference_membre(site_, accede, index);
    return cree_charge_mem(site_, inst);
}

InstructionTranstype *ConstructriceRI::cree_transtype(NoeudExpression *site_,
                                                      Type *type,
                                                      Atome *valeur,
                                                      TypeTranstypage op)
{
    // std::cerr << __func__ << ", type : " << chaine_type(type) << ", valeur " <<
    // chaine_type(valeur->type) << '\n';
    auto inst = insts_transtype.ajoute_element(site_, type, valeur, op);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

TranstypeConstant *ConstructriceRI::cree_transtype_constant(Type *type, AtomeConstante *valeur)
{
    return transtypes_constants.ajoute_element(type, valeur);
}

OpUnaireConstant *ConstructriceRI::cree_op_unaire_constant(Type *type,
                                                           OperateurUnaire::Genre op,
                                                           AtomeConstante *valeur)
{
    return op_unaires_constants.ajoute_element(type, op, valeur);
}

OpBinaireConstant *ConstructriceRI::cree_op_binaire_constant(Type *type,
                                                             OperateurBinaire::Genre op,
                                                             AtomeConstante *valeur_gauche,
                                                             AtomeConstante *valeur_droite)
{
    return op_binaires_constants.ajoute_element(type, op, valeur_gauche, valeur_droite);
}

OpBinaireConstant *ConstructriceRI::cree_op_comparaison_constant(OperateurBinaire::Genre op,
                                                                 AtomeConstante *valeur_gauche,
                                                                 AtomeConstante *valeur_droite)
{
    return cree_op_binaire_constant(
        m_espace->typeuse[TypeBase::BOOL], op, valeur_gauche, valeur_droite);
}

AccedeIndexConstant *ConstructriceRI::cree_acces_index_constant(AtomeConstante *accede,
                                                                AtomeConstante *index)
{
    assert_rappel(accede->type->genre == GenreType::POINTEUR,
                  [=]() { std::cerr << "Type accédé : '" << chaine_type(accede->type) << "'\n"; });
    auto type_pointeur = accede->type->comme_pointeur();
    assert_rappel(
        dls::outils::est_element(
            type_pointeur->type_pointe->genre, GenreType::POINTEUR, GenreType::TABLEAU_FIXE),
        [=]() {
            std::cerr << "Type accédé : '" << chaine_type(type_pointeur->type_pointe) << "'\n";
        });

    auto type = m_espace->typeuse.type_pointeur_pour(
        type_dereference_pour(type_pointeur->type_pointe), false);

    return accede_index_constants.ajoute_element(type, accede, index);
}

void ConstructriceRI::genere_ri_pour_noeud(NoeudExpression *noeud)
{
    if (noeud->substitution) {
        genere_ri_pour_noeud(noeud->substitution);
        return;
    }

    switch (noeud->genre) {
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::EXPRESSION_PLAGE:
        case GenreNoeud::EXPRESSION_VIRGULE:
        case GenreNoeud::INSTRUCTION_CHARGE:
        case GenreNoeud::INSTRUCTION_EMPL:
        case GenreNoeud::INSTRUCTION_IMPORTE:
        case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
        case GenreNoeud::DECLARATION_MODULE:
        case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:
        {
            break;
        }
        /* ceux-ci sont simplifiés */
        case GenreNoeud::DIRECTIVE_CUISINE:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Erreur interne : une directive #cuisine ne fut pas simplifiée !\n";
                erreur::imprime_site(*m_espace, noeud);
            });

            break;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        {
            auto inst = noeud->comme_si_statique();

            if (!inst->condition_est_vraie && inst->bloc_si_faux) {
                assert_rappel(false, [&]() {
                    std::cerr << "Erreur interne : une directive #si ne fut pas simplifiée !\n";
                    erreur::imprime_site(*m_espace, noeud);
                });
            }

            break;
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        {
            auto directive = noeud->comme_execute();

            if (directive->ident != ID::assert_) {
                assert_rappel(false, [&]() {
                    std::cerr << "Erreur interne : un directive ne fut pas simplifié !\n";
                    erreur::imprime_site(*m_espace, noeud);
                });
            }

            break;
        }
        case GenreNoeud::EXPRESSION_PARENTHESE:
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ENUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        case GenreNoeud::INSTRUCTION_POUR:
        case GenreNoeud::INSTRUCTION_RETIENS:
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Erreur interne : un noeud ne fut pas simplifié !\n";
                std::cerr << "Le noeud est de genre : " << noeud->genre << '\n';
                erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        {
            auto decl = noeud->comme_entete_fonction();
            genere_ri_pour_fonction(decl);
            if (!decl->est_externe) {
                assert(decl->corps->possede_drapeau(DECLARATION_FUT_VALIDEE));
            }
            break;
        }
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            auto corps = noeud->comme_corps_fonction();
            assert(corps->possede_drapeau(DECLARATION_FUT_VALIDEE));
            genere_ri_pour_fonction(corps->entete);
            break;
        }
        case GenreNoeud::INSTRUCTION_DIFFERE:
        {
            noeud->bloc_parent->instructions_differees.ajoute(noeud->comme_differe());
            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSEE:
        {
            auto noeud_bloc = noeud->comme_bloc();

            POUR (*noeud_bloc->membres.verrou_lecture()) {
                if (it->genre != GenreNoeud::DECLARATION_VARIABLE) {
                    continue;
                }

                auto decl_var = it->comme_declaration_variable();

                if (!decl_var->declaration_vient_d_un_emploi) {
                    continue;
                }

                auto ident_var_employee = decl_var->declaration_vient_d_un_emploi->ident;
                auto pointeur_struct = decl_var->declaration_vient_d_un_emploi->atome;

                if (pointeur_struct == nullptr) {
                    pointeur_struct = cree_allocation(
                        decl_var,
                        decl_var->declaration_vient_d_un_emploi->type,
                        ident_var_employee);
                    decl_var->declaration_vient_d_un_emploi->atome = pointeur_struct;
                }

                auto type_employe = decl_var->declaration_vient_d_un_emploi->type;
                if (type_employe->est_pointeur() || type_employe->est_reference()) {
                    pointeur_struct = cree_charge_mem(decl_var, pointeur_struct);
                }

                decl_var->atome = cree_reference_membre(
                    decl_var, pointeur_struct, decl_var->index_membre_employe);
            }

            auto ancienne_taille_allouee = taille_allouee;

            POUR (*noeud_bloc->expressions.verrou_lecture()) {
                if (it->est_entete_fonction()) {
                    continue;
                }
                genere_ri_pour_noeud(it);
            }

            auto derniere_instruction = fonction_courante->derniere_instruction();

            if (derniere_instruction->genre != Instruction::Genre::RETOUR) {
                /* Génère le code pour toutes les instructions différées de ce bloc. */
                genere_ri_insts_differees(noeud_bloc, noeud_bloc->bloc_parent);
            }

            taille_allouee = ancienne_taille_allouee;

            break;
        }
        case GenreNoeud::EXPRESSION_APPEL:
        {
            auto expr_appel = noeud->comme_appel();

            auto args = kuri::tableau<Atome *, int>();
            args.reserve(expr_appel->parametres_resolus.taille());

            if (!expr_appel->possede_drapeau(FORCE_NULCTX)) {
                args.ajoute(cree_charge_mem(expr_appel, contexte));
            }

            auto ancien_pour_appel = m_noeud_pour_appel;
            m_noeud_pour_appel = expr_appel;

            POUR (expr_appel->parametres_resolus) {
                genere_ri_pour_expression_droite(it, nullptr);
                auto valeur = depile_valeur();

                /* crée une temporaire pour simplifier l'enlignage, car nous devrons
                 * remplacer les locales par les expressions passées, et il est plus
                 * simple de remplacer une allocation par une autre */
                auto alloc = cree_allocation(it, valeur->type, nullptr);
                cree_stocke_mem(it, alloc, valeur);
                args.ajoute(cree_charge_mem(it, alloc));
            }

            m_noeud_pour_appel = ancien_pour_appel;

            genere_ri_pour_expression_droite(expr_appel->expression, nullptr);
            auto atome_fonc = depile_valeur();

            assert_rappel(atome_fonc && atome_fonc->type != nullptr, [&] {
                if (atome_fonc == nullptr) {
                    std::cerr << "L'atome est nul !\n";
                }
                else if (atome_fonc->type == nullptr) {
                    std::cerr << "Le type de l'atome est nul !\n";
                }

                erreur::imprime_site(*m_espace, expr_appel);
            });

            assert_rappel(atome_fonc->type->est_fonction(), [&] {
                std::cerr << "L'atome n'est pas de type fonction mais de type "
                          << chaine_type(atome_fonc->type) << " !\n";
                erreur::imprime_site(*espace(), expr_appel);
                erreur::imprime_site(*espace(), expr_appel->expression);
                if (!expr_appel->expression->substitution) {
                    std::cerr << "L'appelée n'a pas de substitution !\n";
                }
            });

            auto type_fonction = atome_fonc->type->comme_fonction();
            InstructionAllocation *adresse_retour = nullptr;

            if (!type_fonction->type_sortie->est_rien()) {
                adresse_retour = cree_allocation(nullptr, type_fonction->type_sortie, nullptr);
            }

            auto valeur = cree_appel(expr_appel, noeud->lexeme, atome_fonc, std::move(args));
            valeur->adresse_retour = adresse_retour;

            if (adresse_retour) {
                cree_stocke_mem(noeud, valeur->adresse_retour, valeur);
                empile_valeur(adresse_retour);
                return;
            }

            empile_valeur(valeur);
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto expr_ref = noeud->comme_reference_declaration();
            auto decl_ref = expr_ref->declaration_referee;

            assert_rappel(decl_ref, [&]() {
                erreur::imprime_site(*m_espace, noeud);
                std::cerr << "La référence à la déclaration est nulle " << noeud->ident->nom
                          << " (" << chaine_type(noeud->type) << ")\n";
            });

            if (decl_ref->est_entete_fonction()) {
                auto atome_fonc = m_espace->trouve_ou_insere_fonction(
                    *this, decl_ref->comme_entete_fonction());
                // voir commentaire dans cree_appel
                atome_fonc->nombre_utilisations += 1;
                empile_valeur(atome_fonc);
                return;
            }

            if (decl_ref->possede_drapeau(EST_GLOBALE)) {
                empile_valeur(m_espace->trouve_ou_insere_globale(decl_ref));
                return;
            }

            auto locale = static_cast<NoeudDeclarationSymbole *>(decl_ref)->atome;
            assert_rappel(locale, [&]() {
                erreur::imprime_site(*m_espace, noeud);
                std::cerr << "Aucune locale trouvée pour " << noeud->ident->nom << " ("
                          << chaine_type(noeud->type) << ")\n";
                std::cerr << "\nLa locale fut déclarée ici :\n";
                erreur::imprime_site(*m_espace, decl_ref);
            });
            empile_valeur(locale);
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        {
            genere_ri_pour_acces_membre(noeud->comme_reference_membre());
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        {
            genere_ri_pour_acces_membre_union(noeud->comme_reference_membre_union());
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        {
            auto type_de_donnees = noeud->type->comme_type_de_donnees();

            if (type_de_donnees->type_connu) {
                empile_valeur(cree_constante_type(type_de_donnees->type_connu));
                return;
            }

            empile_valeur(cree_constante_type(type_de_donnees));
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto expr_ass = noeud->comme_assignation_variable();

            POUR (expr_ass->donnees_exprs.plage()) {
                auto expression = it.expression;

                auto ancienne_expression_gauche = expression_gauche;
                expression_gauche = false;
                genere_ri_pour_noeud(expression);
                expression_gauche = ancienne_expression_gauche;

                if (it.multiple_retour) {
                    auto valeur_tuple = depile_valeur();

                    for (auto i = 0; i < it.variables.taille(); ++i) {
                        auto var = it.variables[i];
                        auto &transformation = it.transformations[i];
                        genere_ri_pour_noeud(var);
                        auto pointeur = depile_valeur();

                        auto valeur = cree_reference_membre(expression, valeur_tuple, i);
                        transforme_valeur(expression, valeur, transformation, pointeur);
                        depile_valeur();
                    }
                }
                else {
                    auto valeur = depile_valeur();

                    for (auto i = 0; i < it.variables.taille(); ++i) {
                        auto var = it.variables[i];
                        auto &transformation = it.transformations[i];
                        genere_ri_pour_noeud(var);
                        auto pointeur = depile_valeur();

                        transforme_valeur(expression, valeur, transformation, pointeur);
                        depile_valeur();
                    }
                }
            }

            break;
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            genere_ri_pour_declaration_variable(noeud->comme_declaration_variable());
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            empile_valeur(
                cree_constante_reelle(noeud->type, noeud->comme_litterale_reel()->valeur));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        {
            if (noeud->type->est_reel()) {
                empile_valeur(cree_constante_reelle(
                    noeud->type, static_cast<double>(noeud->comme_litterale_entier()->valeur)));
            }
            else {
                empile_valeur(
                    cree_constante_entiere(noeud->type, noeud->comme_litterale_entier()->valeur));
            }

            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        {
            auto lit_chaine = noeud->comme_litterale_chaine();
            auto chaine = compilatrice().gerante_chaine->chaine_pour_adresse(lit_chaine->valeur);
            auto constante = cree_chaine(chaine);

            assert_rappel((noeud->lexeme->chaine.taille() != 0 && chaine.taille() != 0) ||
                              (noeud->lexeme->chaine.taille() == 0 && chaine.taille() == 0),
                          [&]() {
                              erreur::imprime_site(*espace(), noeud);
                              std::cerr << "La chaine n'est pas de la bonne taille !\n";
                              std::cerr << "Le lexème a une chaine taille de "
                                        << noeud->lexeme->chaine.taille()
                                        << " alors que la chaine littérale a une taille de "
                                        << chaine.taille() << '\n';
                              std::cerr << "L'index de la chaine est de " << lit_chaine->valeur
                                        << '\n';
                          });

            if (fonction_courante == nullptr) {
                empile_valeur(constante);
                return;
            }

            auto alloc = cree_allocation(noeud, m_espace->typeuse.type_chaine, nullptr);
            cree_stocke_mem(noeud, alloc, constante);
            empile_valeur(alloc);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        {
            empile_valeur(cree_constante_booleenne(noeud->comme_litterale_bool()->valeur));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            // À FAIRE : caractères Unicode
            auto caractere = static_cast<unsigned char>(
                noeud->comme_litterale_caractere()->valeur);
            empile_valeur(cree_constante_caractere(noeud->type, caractere));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        {
            empile_valeur(cree_constante_nulle(noeud->type));
            break;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            auto expr_bin = noeud->comme_expression_binaire();

            if (dls::outils::est_element(
                    noeud->lexeme->genre, GenreLexeme::BARRE_BARRE, GenreLexeme::ESP_ESP)) {
                genere_ri_pour_expression_logique(noeud, nullptr);
                return;
            }

            genere_ri_pour_expression_droite(expr_bin->operande_gauche, nullptr);
            auto valeur_gauche = depile_valeur();
            genere_ri_pour_expression_droite(expr_bin->operande_droite, nullptr);
            auto valeur_droite = depile_valeur();
            auto resultat = cree_op_binaire(
                noeud, noeud->type, expr_bin->op->genre, valeur_gauche, valeur_droite);

            auto alloc = cree_allocation(noeud, expr_bin->type, nullptr);
            cree_stocke_mem(noeud, alloc, resultat);
            empile_valeur(alloc);
            break;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto expr_bin = noeud->comme_indexage();
            auto type_gauche = expr_bin->operande_gauche->type;

            if (type_gauche->est_opaque()) {
                type_gauche = type_gauche->comme_opaque()->type_opacifie;
            }

            genere_ri_pour_noeud(expr_bin->operande_gauche);
            auto pointeur = depile_valeur();
            genere_ri_pour_expression_droite(expr_bin->operande_droite, nullptr);
            auto valeur = depile_valeur();

            if (type_gauche->genre == GenreType::POINTEUR) {
                empile_valeur(cree_acces_index(noeud, pointeur, valeur));
                return;
            }

            /* À CONSIDÉRER :
             * - directive pour ne pas générer le code de vérification,
             *   car les branches nuisent à la vitesse d'exécution des
             *   programmes
             * - tests redondants ou inutiles, par exemple :
             *    - ceci génère deux fois la même instruction
             *      x[i] = 0;
             *      y = x[i];
             *    - ceci génère une instruction inutile
             *	    dyn x : [6]z32;
             *      x[0] = 8;
             */

            auto genere_protection_limites =
                [this, noeud](Atome *acces_taille, Atome *valeur_, AtomeFonction *fonction) {
                    auto label1 = reserve_label(noeud);
                    auto label2 = reserve_label(noeud);
                    auto label3 = reserve_label(noeud);
                    auto label4 = reserve_label(noeud);

                    auto condition = cree_op_comparaison(
                        noeud, OperateurBinaire::Genre::Comp_Inf, valeur_, cree_z64(0));
                    cree_branche_condition(noeud, condition, label1, label2);

                    insere_label(label1);

                    auto params = kuri::tableau<Atome *, int>(3);
                    params[0] = cree_charge_mem(noeud, contexte);
                    params[1] = acces_taille;
                    params[2] = valeur_;
                    cree_appel(noeud, noeud->lexeme, fonction, std::move(params));

                    insere_label(label2);

                    condition = cree_op_comparaison(
                        noeud, OperateurBinaire::Genre::Comp_Sup_Egal, valeur_, acces_taille);
                    cree_branche_condition(noeud, condition, label3, label4);

                    insere_label(label3);

                    params = kuri::tableau<Atome *, int>(3);
                    params[0] = cree_charge_mem(noeud, contexte);
                    params[1] = acces_taille;
                    params[2] = valeur_;
                    cree_appel(noeud, noeud->lexeme, fonction, std::move(params));

                    insere_label(label4);
                };

            // À FAIRE : les fonctions sans contexte ne peuvent pas avoir des vérifications de
            // limites

            if (type_gauche->genre == GenreType::TABLEAU_FIXE) {
                if (contexte != nullptr && noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto type_tableau_fixe = type_gauche->comme_tableau_fixe();
                    auto acces_taille = cree_z64(static_cast<unsigned>(type_tableau_fixe->taille));
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_espace->trouve_ou_insere_fonction(
                            *this, m_espace->interface_kuri->decl_panique_tableau));
                }
                empile_valeur(cree_acces_index(noeud, pointeur, valeur));
                return;
            }

            if (type_gauche->genre == GenreType::TABLEAU_DYNAMIQUE ||
                type_gauche->genre == GenreType::VARIADIQUE) {
                if (contexte != nullptr && noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto acces_taille = cree_reference_membre_et_charge(noeud, pointeur, 1);
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_espace->trouve_ou_insere_fonction(
                            *this, m_espace->interface_kuri->decl_panique_tableau));
                }
                pointeur = cree_reference_membre(noeud, pointeur, 0);
                empile_valeur(cree_acces_index(noeud, pointeur, valeur));
                return;
            }

            if (type_gauche->genre == GenreType::CHAINE) {
                if (contexte != nullptr && noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto acces_taille = cree_reference_membre_et_charge(noeud, pointeur, 1);
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_espace->trouve_ou_insere_fonction(
                            *this, m_espace->interface_kuri->decl_panique_chaine));
                }
                pointeur = cree_reference_membre(noeud, pointeur, 0);
                empile_valeur(cree_acces_index(noeud, pointeur, valeur));
                return;
            }

            break;
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            auto expr_un = noeud->comme_expression_unaire();

            /* prise d'adresse */
            if (noeud->lexeme->genre == GenreLexeme::FOIS_UNAIRE) {
                genere_ri_pour_noeud(expr_un->operande);
                auto valeur = depile_valeur();
                if (expr_un->operande->type->genre == GenreType::REFERENCE) {
                    valeur = cree_charge_mem(noeud, valeur);
                }

                if (!expression_gauche) {
                    auto alloc = cree_allocation(noeud, expr_un->type, nullptr);
                    cree_stocke_mem(noeud, alloc, valeur);
                    valeur = alloc;
                }

                empile_valeur(valeur);
                return;
            }

            // @simplifie
            if (noeud->lexeme->genre == GenreLexeme::EXCLAMATION) {
                auto condition = expr_un->operande;
                auto type_condition = condition->type;
                auto valeur = static_cast<Atome *>(nullptr);

                switch (type_condition->genre) {
                    case GenreType::ENTIER_NATUREL:
                    case GenreType::ENTIER_RELATIF:
                    case GenreType::ENTIER_CONSTANT:
                    {
                        genere_ri_pour_expression_droite(condition, nullptr);
                        auto valeur1 = depile_valeur();
                        auto valeur2 = cree_constante_entiere(type_condition, 0);
                        valeur = cree_op_comparaison(
                            noeud, OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                        break;
                    }
                    case GenreType::BOOL:
                    {
                        genere_ri_pour_expression_droite(condition, nullptr);
                        auto valeur1 = depile_valeur();
                        auto valeur2 = cree_constante_booleenne(false);
                        valeur = cree_op_comparaison(
                            noeud, OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                        break;
                    }
                    case GenreType::FONCTION:
                    case GenreType::POINTEUR:
                    {
                        genere_ri_pour_expression_droite(condition, nullptr);
                        auto valeur1 = depile_valeur();
                        auto valeur2 = cree_constante_nulle(type_condition);
                        valeur = cree_op_comparaison(
                            noeud, OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                        break;
                    }
                    case GenreType::EINI:
                    {
                        genere_ri_pour_noeud(condition);
                        auto pointeur = depile_valeur();
                        auto pointeur_pointeur = cree_reference_membre(noeud, pointeur, 0);
                        auto valeur1 = cree_charge_mem(noeud, pointeur_pointeur);
                        auto valeur2 = cree_constante_nulle(valeur1->type);
                        valeur = cree_op_comparaison(
                            noeud, OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                        break;
                    }
                    case GenreType::CHAINE:
                    case GenreType::TABLEAU_DYNAMIQUE:
                    {
                        genere_ri_pour_noeud(condition);
                        auto pointeur = depile_valeur();
                        auto pointeur_taille = cree_reference_membre(noeud, pointeur, 1);
                        auto valeur1 = cree_charge_mem(noeud, pointeur_taille);
                        auto valeur2 = cree_z64(0);
                        valeur = cree_op_comparaison(
                            noeud, OperateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }

                empile_valeur(valeur);
                return;
            }

            genere_ri_pour_expression_droite(expr_un->operande, nullptr);
            auto valeur = depile_valeur();
            empile_valeur(cree_op_unaire(noeud, expr_un->type, expr_un->op->genre, valeur));
            break;
        }
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto inst = noeud->comme_retourne();

            auto valeur_ret = static_cast<Atome *>(nullptr);

            if (inst->expression) {
                genere_ri_pour_expression_droite(inst->expression, nullptr);
                valeur_ret = depile_valeur();
            }

            genere_ri_insts_differees(noeud->bloc_parent, nullptr);
            cree_retour(noeud, valeur_ret);
            break;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto inst_si = static_cast<NoeudSi *>(noeud);

            auto label_si_vrai = reserve_label(noeud);
            auto label_si_faux = reserve_label(noeud);

            if (noeud->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
                genere_ri_pour_condition(inst_si->condition, label_si_faux, label_si_vrai);
            }
            else {
                genere_ri_pour_condition(inst_si->condition, label_si_vrai, label_si_faux);
            }

            if (inst_si->bloc_si_faux) {
                auto label_apres_instruction = reserve_label(noeud);

                insere_label(label_si_vrai);
                genere_ri_pour_noeud(inst_si->bloc_si_vrai);

                auto di = fonction_courante->derniere_instruction();
                if (!di->est_branche_ou_retourne()) {
                    cree_branche(noeud, label_apres_instruction);
                }

                insere_label(label_si_faux);
                genere_ri_pour_noeud(inst_si->bloc_si_faux);
                insere_label(label_apres_instruction);
            }
            else {
                insere_label(label_si_vrai);
                genere_ri_pour_noeud(inst_si->bloc_si_vrai);
                insere_label(label_si_faux);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_BOUCLE:
        case GenreNoeud::INSTRUCTION_REPETE:
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            auto boucle = static_cast<NoeudBoucle *>(noeud);

            /* labels pour les différents blocs possible */
            auto label_boucle = reserve_label(noeud);
            auto label_pour_bloc_inc = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_sansarret = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_sinon = static_cast<InstructionLabel *>(nullptr);
            auto label_apres_boucle = reserve_label(noeud);

            /* labels pour les controles des boucles */
            auto label_pour_continue = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_arret = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_arret_implicite = static_cast<InstructionLabel *>(nullptr);

            if (boucle->bloc_inc) {
                label_pour_bloc_inc = reserve_label(boucle);
                label_pour_continue = label_pour_bloc_inc;
            }
            else {
                label_pour_continue = label_boucle;
            }

            if (boucle->bloc_sansarret) {
                label_pour_sansarret = reserve_label(boucle);
                label_pour_arret_implicite = label_pour_sansarret;
            }
            else {
                label_pour_arret_implicite = label_apres_boucle;
            }

            if (boucle->bloc_sinon) {
                label_pour_sinon = reserve_label(noeud);
                label_pour_arret = label_pour_sinon;
            }
            else {
                label_pour_arret = label_apres_boucle;
            }

            boucle->label_pour_arrete = label_pour_arret;
            boucle->label_pour_arrete_implicite = label_pour_arret_implicite;
            boucle->label_pour_continue = label_pour_continue;
            boucle->label_pour_reprends = label_boucle;

            if (boucle->bloc_pre) {
                genere_ri_pour_noeud(boucle->bloc_pre);
                cree_branche(noeud, label_boucle);
            }

            insere_label(label_boucle);
            genere_ri_pour_noeud(boucle->bloc);

            auto di = fonction_courante->derniere_instruction();

            if (boucle->bloc_inc) {
                insere_label(label_pour_bloc_inc);
                genere_ri_pour_noeud(boucle->bloc_inc);

                if (di->est_branche_ou_retourne()) {
                    cree_branche(noeud, label_boucle);
                }
            }

            if (!di->est_branche_ou_retourne()) {
                cree_branche(noeud, label_boucle);
            }

            if (boucle->bloc_sansarret) {
                insere_label(label_pour_sansarret);
                genere_ri_pour_noeud(boucle->bloc_sansarret);
                cree_branche(boucle, label_apres_boucle);
            }

            if (boucle->bloc_sinon) {
                insere_label(label_pour_sinon);
                genere_ri_pour_noeud(boucle->bloc_sinon);
            }

            insere_label(label_apres_boucle);

            break;
        }
        case GenreNoeud::INSTRUCTION_ARRETE:
        {
            auto inst = noeud->comme_arrete();
            auto boucle_controlee = inst->boucle_controlee->substitution ?
                                        inst->boucle_controlee->substitution :
                                        inst->boucle_controlee;

            if (inst->possede_drapeau(EST_IMPLICITE)) {
                auto label = boucle_controlee->comme_boucle()->label_pour_arrete_implicite;
                cree_branche(noeud, label);
            }
            else {
                auto label = boucle_controlee->comme_boucle()->label_pour_arrete;
                genere_ri_insts_differees(inst->bloc_parent, boucle_controlee->bloc_parent);
                cree_branche(noeud, label);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_CONTINUE:
        {
            auto inst = noeud->comme_continue();
            auto boucle_controlee = inst->boucle_controlee->substitution ?
                                        inst->boucle_controlee->substitution :
                                        inst->boucle_controlee;
            auto label = boucle_controlee->comme_boucle()->label_pour_continue;
            genere_ri_insts_differees(inst->bloc_parent, boucle_controlee->bloc_parent);
            cree_branche(noeud, label);
            break;
        }
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            auto inst = noeud->comme_reprends();
            auto boucle_controlee = inst->boucle_controlee->substitution ?
                                        inst->boucle_controlee->substitution :
                                        inst->boucle_controlee;
            auto label = boucle_controlee->comme_boucle()->label_pour_reprends;
            genere_ri_insts_differees(inst->bloc_parent, boucle_controlee->bloc_parent);
            cree_branche(noeud, label);
            break;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto inst = noeud->comme_comme();
            auto expr = inst->expression;

            if (expression_gauche &&
                inst->transformation.type == TypeTransformation::DEREFERENCE) {
                genere_ri_pour_noeud(expr);
                /* déréférence l'adresse du pointeur */
                empile_valeur(cree_charge_mem(noeud, depile_valeur()));
                break;
            }

            auto alloc = cree_allocation(noeud, inst->type, nullptr);
            genere_ri_transformee_pour_noeud(expr, alloc, inst->transformation);
            empile_valeur(alloc);
            break;
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            auto noeud_tableau = noeud->comme_args_variadiques();
            auto taille_tableau = noeud_tableau->expressions.taille();

            if (taille_tableau == 0) {
                auto type_tableau_dyn = m_espace->typeuse.type_tableau_dynamique(noeud->type);
                auto alloc = cree_allocation(noeud, type_tableau_dyn, nullptr);
                auto init = genere_initialisation_defaut_pour_type(type_tableau_dyn);
                cree_stocke_mem(noeud, alloc, init);
                empile_valeur(alloc);
                return;
            }

            auto type_tableau_fixe = m_espace->typeuse.type_tableau_fixe(noeud->type,
                                                                         taille_tableau);
            auto pointeur_tableau = cree_allocation(noeud, type_tableau_fixe, nullptr);

            auto index = 0ul;
            POUR (noeud_tableau->expressions) {
                auto index_tableau = cree_acces_index(noeud, pointeur_tableau, cree_z64(index++));
                genere_ri_pour_expression_droite(it, index_tableau);
            }

            auto valeur = converti_vers_tableau_dyn(
                noeud, pointeur_tableau, type_tableau_fixe, nullptr);
            empile_valeur(valeur);
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        {
            auto expr = noeud->comme_construction_structure();
            auto alloc = static_cast<Atome *>(nullptr);

            if (expr->type->genre == GenreType::UNION) {
                auto index_membre = 0u;
                auto valeur = static_cast<Atome *>(nullptr);
                auto type_union = expr->type->comme_union();

                POUR (expr->parametres_resolus) {
                    if (it != nullptr) {
                        genere_ri_pour_expression_droite(it, nullptr);
                        valeur = depile_valeur();

                        if (it->type != type_union->type_le_plus_grand) {
                            Atome *ptr = nullptr;

                            if (!valeur->est_instruction()) {
                                auto alloc_temp = cree_allocation(it, it->type, nullptr);
                                cree_stocke_mem(it, alloc_temp, valeur);
                                ptr = alloc_temp;
                            }
                            else {
                                ptr = valeur->comme_instruction()->comme_charge()->chargee;
                            }

                            valeur = cree_transtype(noeud,
                                                    m_espace->typeuse.type_pointeur_pour(
                                                        type_union->type_le_plus_grand, false),
                                                    ptr,
                                                    TypeTranstypage::BITS);
                            valeur = cree_charge_mem(noeud, valeur);
                        }

                        break;
                    }

                    index_membre += 1;
                }

                alloc = cree_allocation(noeud, type_union, nullptr);

                if (type_union->est_nonsure) {
                    cree_stocke_mem(noeud, alloc, valeur);
                }
                else {
                    auto ptr_valeur = cree_reference_membre(noeud, alloc, 0);
                    cree_stocke_mem(noeud, ptr_valeur, valeur);

                    auto ptr_index = cree_reference_membre(noeud, alloc, 1);
                    cree_stocke_mem(noeud, ptr_index, cree_z32(index_membre + 1));
                }
            }
            else {
                /* appelee peut être nulle pour les structures anonymes crées par la compilatrice
                 */
                if (expr->expression && expr->expression->ident == ID::PositionCodeSource) {
                    genere_ri_pour_position_code_source(noeud);
                    return;
                }

                auto type_struct = noeud->type->comme_structure();
                auto index_membre = 0;

                alloc = cree_allocation(noeud, type_struct, nullptr);

                POUR (expr->parametres_resolus) {
                    const auto &membre = type_struct->membres[index_membre];

                    if ((membre.drapeaux & TypeCompose::Membre::EST_CONSTANT) != 0) {
                        index_membre += 1;
                        continue;
                    }

                    auto valeur = static_cast<Atome *>(nullptr);

                    if (it != nullptr) {
                        genere_ri_pour_expression_droite(it, nullptr);
                        valeur = depile_valeur();
                    }
                    else {
                        valeur = genere_initialisation_defaut_pour_type(
                            type_struct->membres[index_membre].type);
                    }

                    // À FAIRE(tableau fixe)
                    if (valeur) {
                        auto ptr = cree_reference_membre(it, alloc, index_membre);
                        cree_stocke_mem(it, ptr, valeur);
                    }

                    index_membre += 1;
                }
            }

            empile_valeur(alloc);
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto expr = noeud->comme_construction_tableau();

            auto feuilles = expr->expression->comme_virgule();

            if (fonction_courante == nullptr) {
                auto type_tableau_fixe = expr->type->comme_tableau_fixe();
                kuri::tableau<AtomeConstante *> valeurs;
                valeurs.reserve(feuilles->expressions.taille());

                POUR (feuilles->expressions) {
                    genere_ri_pour_noeud(it);
                    auto valeur = depile_valeur();
                    valeurs.ajoute(static_cast<AtomeConstante *>(valeur));
                }

                auto tableau_constant = cree_constante_tableau_fixe(type_tableau_fixe,
                                                                    std::move(valeurs));
                empile_valeur(tableau_constant);
                return;
            }

            auto pointeur_tableau = cree_allocation(noeud, expr->type, nullptr);

            auto index = 0ul;
            POUR (feuilles->expressions) {
                genere_ri_pour_expression_droite(it, nullptr);
                auto valeur = depile_valeur();
                auto index_tableau = cree_acces_index(noeud, pointeur_tableau, cree_z64(index++));
                cree_stocke_mem(noeud, index_tableau, valeur);
            }

            empile_valeur(pointeur_tableau);
            break;
        }
        case GenreNoeud::EXPRESSION_INFO_DE:
        {
            auto inst = noeud->comme_info_de();
            auto enfant = inst->expression;
            auto valeur = cree_info_type(enfant->type);

            /* utilise une temporaire pour simplifier la compilation d'expressions du style :
             * info_de(z32).id */
            auto alloc = cree_allocation(noeud, valeur->type, nullptr);
            cree_stocke_mem(noeud, alloc, valeur);

            empile_valeur(alloc);
            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            // @simplifie
            auto type_fonction = noeud->type->comme_fonction();
            auto type_pointeur = type_fonction->types_entrees[1];
            auto type_arg = type_pointeur->comme_pointeur()->type_pointe;
            empile_valeur(m_espace->trouve_ou_insere_fonction_init(*this, type_arg));
            break;
        }
        case GenreNoeud::EXPRESSION_MEMOIRE:
        {
            auto inst_mem = noeud->comme_memoire();
            genere_ri_pour_noeud(inst_mem->expression);
            auto valeur = depile_valeur();

            if (!expression_gauche) {
                auto alloc = cree_allocation(noeud, inst_mem->type, nullptr);
                // déréférence la locale
                valeur = cree_charge_mem(noeud, valeur);
                // déréférence le pointeur
                valeur = cree_charge_mem(noeud, valeur);
                cree_stocke_mem(noeud, alloc, valeur);
                empile_valeur(alloc);
                return;
            }

            // mémoire(@expr) = ...
            if (inst_mem->expression->genre_valeur == GenreValeur::DROITE &&
                !inst_mem->expression->est_comme()) {
                empile_valeur(valeur);
                return;
            }

            empile_valeur(cree_charge_mem(noeud, valeur));
            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            genere_ri_pour_declaration_structure(noeud->comme_structure());
            break;
        }
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            auto noeud_pc = noeud->comme_pousse_contexte();
            genere_ri_pour_noeud(noeud_pc->expression);
            auto atome_nouveau_contexte = depile_valeur();
            auto atome_ancien_contexte = contexte;

            contexte = atome_nouveau_contexte;
            genere_ri_pour_noeud(noeud_pc->bloc);
            contexte = atome_ancien_contexte;

            break;
        }
        case GenreNoeud::EXPANSION_VARIADIQUE:
        {
            genere_ri_pour_expression_droite(noeud->comme_expansion_variadique()->expression,
                                             nullptr);
            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            genere_ri_pour_tente(noeud->comme_tente());
            break;
        }
    }
}

void ConstructriceRI::genere_ri_pour_fonction(NoeudDeclarationEnteteFonction *decl)
{
    fonction_courante = nullptr;
    nombre_labels = 0;
    taille_allouee = 0;
    this->m_pile.efface();

    auto atome_fonc = m_espace->trouve_ou_insere_fonction(*this, decl);

    if (decl->est_externe) {
        decl->drapeaux |= RI_FUT_GENEREE;
        return;
    }

    fonction_courante = atome_fonc;

    if (!decl->possede_drapeau(FORCE_NULCTX)) {
        contexte = atome_fonc->params_entrees[0];

        POUR ((*decl->corps->bloc->membres.verrou_lecture())) {
            if (it->ident == ID::contexte) {
                static_cast<NoeudDeclarationSymbole *>(it)->atome = contexte;
                break;
            }
        }
    }

    cree_label(decl);

    genere_ri_pour_noeud(decl->corps->bloc);

    if (decl->corps->aide_generation_code == REQUIERS_CODE_EXTRA_RETOUR) {
        cree_retour(decl, nullptr);
    }

    decl->drapeaux |= RI_FUT_GENEREE;
    decl->corps->drapeaux |= RI_FUT_GENEREE;

    fonction_courante = nullptr;
    contexte = nullptr;
    this->m_pile.efface();
}

void ConstructriceRI::genere_ri_pour_expression_droite(NoeudExpression *noeud, Atome *place)
{
    auto ancienne_expression_gauche = expression_gauche;
    expression_gauche = false;
    genere_ri_pour_noeud(noeud);
    auto atome = depile_valeur();
    expression_gauche = ancienne_expression_gauche;

    if (atome->est_chargeable) {
        atome = cree_charge_mem(noeud, atome);
    }

    if (place) {
        cree_stocke_mem(noeud, place, atome);
    }
    else {
        empile_valeur(atome);
    }
}

void ConstructriceRI::genere_ri_transformee_pour_noeud(NoeudExpression *noeud,
                                                       Atome *place,
                                                       TransformationType const &transformation)
{
    auto ancienne_expression_gauche = expression_gauche;
    expression_gauche = false;
    genere_ri_pour_noeud(noeud);
    auto valeur = depile_valeur();
    expression_gauche = ancienne_expression_gauche;

    assert_rappel(valeur, [&] {
        std::cerr << __func__ << ", valeur est nulle pour " << noeud->genre << '\n';
        erreur::imprime_site(*m_espace, noeud);
    });

    transforme_valeur(noeud, valeur, transformation, place);
}

void ConstructriceRI::transforme_valeur(NoeudExpression *noeud,
                                        Atome *valeur,
                                        TransformationType const &transformation,
                                        Atome *place)
{
    auto place_fut_utilisee = false;

    switch (transformation.type) {
        case TypeTransformation::IMPOSSIBLE:
        {
            break;
        }
        case TypeTransformation::CONVERTI_REFERENCE_VERS_TYPE_CIBLE:
        {
            assert_rappel(false, [&]() {
                std::cerr << "CONVERTI_REFERENCE_VERS_TYPE_CIBLE utilisée dans la RI !\n";
            });
            break;
        }
        case TypeTransformation::INUTILE:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            break;
        }
        case TypeTransformation::CONVERTI_ENTIER_CONSTANT:
        {
            // valeur est déjà une constante, change simplement le type
            if (valeur->genre_atome == Atome::Genre::CONSTANTE) {
                valeur->type = transformation.type_cible;
            }
            // nous avons une temporaire créée lors d'une opération binaire
            else {
                valeur = cree_charge_mem(noeud, valeur);

                TypeTranstypage type_transtypage;

                if (transformation.type_cible->taille_octet > 4) {
                    type_transtypage = AUGMENTE_NATUREL;
                }
                else {
                    type_transtypage = DIMINUE_NATUREL;
                }

                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, type_transtypage);
            }

            assert_rappel(valeur->type->genre != GenreType::ENTIER_CONSTANT, [=]() {
                std::cerr << "Type de la valeur : " << chaine_type(valeur->type) << "\n.";
            });
            break;
        }
        case TypeTransformation::CONSTRUIT_UNION:
        {
            auto type_union = transformation.type_cible->comme_union();

            if (!valeur->est_chargeable) {
                auto alloc_valeur = cree_allocation(noeud, noeud->type, nullptr);
                cree_stocke_mem(noeud, alloc_valeur, valeur);
                valeur = alloc_valeur;
            }

            auto alloc = cree_allocation(noeud, type_union, nullptr);

            valeur = cree_transtype(
                noeud,
                m_espace->typeuse.type_pointeur_pour(type_union->type_le_plus_grand, false),
                valeur,
                TypeTranstypage::BITS);
            valeur = cree_charge_mem(noeud, valeur);

            if (type_union->est_nonsure) {
                cree_stocke_mem(noeud, alloc, valeur);
            }
            else {
                auto acces_membre = cree_reference_membre(noeud, alloc, 0);
                cree_stocke_mem(noeud, acces_membre, valeur);

                acces_membre = cree_reference_membre(noeud, alloc, 1);
                auto index = cree_constante_entiere(
                    m_espace->typeuse[TypeBase::Z32],
                    static_cast<unsigned long>(transformation.index_membre + 1));
                cree_stocke_mem(noeud, acces_membre, index);
            }

            valeur = cree_charge_mem(noeud, alloc);
            break;
        }
        case TypeTransformation::EXTRAIT_UNION:
        {
            auto type_union = noeud->type->comme_union();

            if (!valeur->est_chargeable) {
                auto alloc = cree_allocation(noeud, valeur->type, nullptr);
                cree_stocke_mem(noeud, alloc, valeur);
                valeur = alloc;
            }

            if (!type_union->est_nonsure) {
                auto membre_actif = cree_reference_membre_et_charge(noeud, valeur, 1);

                auto label_si_vrai = reserve_label(noeud);
                auto label_si_faux = reserve_label(noeud);

                auto condition = cree_op_comparaison(
                    noeud,
                    OperateurBinaire::Genre::Comp_Inegal,
                    membre_actif,
                    cree_z32(static_cast<unsigned>(transformation.index_membre + 1)));

                cree_branche_condition(noeud, condition, label_si_vrai, label_si_faux);
                insere_label(label_si_vrai);
                // À FAIRE : nous pourrions avoir une erreur différente ici.
                auto params = kuri::tableau<Atome *, int>(1);
                params[0] = cree_charge_mem(noeud, contexte);
                cree_appel(noeud,
                           noeud->lexeme,
                           m_espace->trouve_ou_insere_fonction(
                               *this, m_espace->interface_kuri->decl_panique_membre_union),
                           std::move(params));
                insere_label(label_si_faux);

                valeur = cree_reference_membre(noeud, valeur, 0);
            }

            valeur = cree_transtype(
                noeud,
                m_espace->typeuse.type_pointeur_pour(transformation.type_cible, false),
                valeur,
                TypeTranstypage::BITS);
            valeur = cree_charge_mem(noeud, valeur);

            break;
        }
        case TypeTransformation::CONVERTI_VERS_PTR_RIEN:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            if (noeud->genre != GenreNoeud::EXPRESSION_LITTERALE_NUL) {
                valeur = cree_transtype(
                    noeud, m_espace->typeuse[TypeBase::PTR_RIEN], valeur, TypeTranstypage::BITS);
            }

            break;
        }
        case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            valeur = cree_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::DEFAUT);
            break;
        }
        case TypeTransformation::POINTEUR_VERS_ENTIER:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            valeur = cree_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::POINTEUR_VERS_ENTIER);
            break;
        }
        case TypeTransformation::ENTIER_VERS_POINTEUR:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            /* Augmente la taille du type ici pour éviter de le faire dans les coulisses.
             * Nous ne pouvons le faire via l'arbre syntaxique car les arbres des expressions
             * d'assigations ou de retours ne peuvent être modifiés. */
            auto type = valeur->type;
            if (type->taille_octet != 8) {
                if (type->est_entier_naturel()) {
                    valeur = cree_transtype(noeud,
                                            m_espace->typeuse[TypeBase::N64],
                                            valeur,
                                            TypeTranstypage::AUGMENTE_NATUREL);
                }
                else {
                    valeur = cree_transtype(noeud,
                                            m_espace->typeuse[TypeBase::Z64],
                                            valeur,
                                            TypeTranstypage::AUGMENTE_RELATIF);
                }
            }

            valeur = cree_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::ENTIER_VERS_POINTEUR);
            break;
        }
        case TypeTransformation::AUGMENTE_TAILLE_TYPE:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            if (noeud->type->genre == GenreType::REEL) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_REEL);
            }
            else if (noeud->type->genre == GenreType::ENTIER_NATUREL) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_NATUREL);
            }
            else if (noeud->type->genre == GenreType::ENTIER_RELATIF) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_RELATIF);
            }
            else if (noeud->type->genre == GenreType::BOOL) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_NATUREL);
            }
            else if (noeud->type->genre == GenreType::OCTET) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_NATUREL);
            }

            break;
        }
        case TypeTransformation::ENTIER_VERS_REEL:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }
            valeur = cree_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::ENTIER_VERS_REEL);
            break;
        }
        case TypeTransformation::REEL_VERS_ENTIER:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }
            valeur = cree_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::REEL_VERS_ENTIER);
            break;
        }
        case TypeTransformation::REDUIT_TAILLE_TYPE:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            if (noeud->type->genre == GenreType::REEL) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_REEL);
            }
            else if (noeud->type->genre == GenreType::ENTIER_NATUREL) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_NATUREL);
            }
            else if (noeud->type->genre == GenreType::ENTIER_RELATIF) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_RELATIF);
            }
            else if (noeud->type->genre == GenreType::BOOL) {
                valeur = cree_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_NATUREL);
            }

            break;
        }
        case TypeTransformation::CONSTRUIT_EINI:
        {
            auto alloc_eini = place;

            if (alloc_eini == nullptr) {
                auto type_eini = m_espace->typeuse[TypeBase::EINI];
                alloc_eini = cree_allocation(noeud, type_eini, nullptr);
            }

            /* copie le pointeur de la valeur vers le type eini */
            auto ptr_eini = cree_reference_membre(noeud, alloc_eini, 0);

            if (!valeur->est_chargeable) {
                auto alloc_tmp = cree_allocation(noeud, valeur->type, nullptr);
                cree_stocke_mem(noeud, alloc_tmp, valeur);
                valeur = alloc_tmp;
            }

            auto transtype = cree_transtype(
                noeud, m_espace->typeuse[TypeBase::PTR_RIEN], valeur, TypeTranstypage::BITS);
            cree_stocke_mem(noeud, ptr_eini, transtype);

            /* copie le pointeur vers les infos du type du eini */
            auto tpe_eini = cree_reference_membre(noeud, alloc_eini, 1);
            auto info_type = cree_info_type(noeud->type);
            auto ptr_info_type = cree_transtype_constant(
                m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                info_type);

            cree_stocke_mem(noeud, tpe_eini, ptr_info_type);

            if (place == nullptr) {
                valeur = cree_charge_mem(noeud, alloc_eini);
            }
            else {
                place_fut_utilisee = true;
            }

            break;
        }
        case TypeTransformation::EXTRAIT_EINI:
        {
            valeur = cree_reference_membre(noeud, valeur, 0);
            auto type_cible = m_espace->typeuse.type_pointeur_pour(transformation.type_cible,
                                                                   false);
            valeur = cree_transtype(noeud, type_cible, valeur, TypeTranstypage::BITS);
            valeur = cree_charge_mem(noeud, valeur);
            break;
        }
        case TypeTransformation::CONSTRUIT_TABL_OCTET:
        {
            auto valeur_pointeur = static_cast<Atome *>(nullptr);
            auto valeur_taille = static_cast<Atome *>(nullptr);

            auto type_cible = m_espace->typeuse[TypeBase::PTR_OCTET];

            switch (noeud->type->genre) {
                default:
                {
                    if (valeur->genre_atome == Atome::Genre::CONSTANTE) {
                        auto alloc = cree_allocation(noeud, noeud->type, nullptr);
                        cree_stocke_mem(noeud, alloc, valeur);
                        valeur = alloc;
                    }

                    valeur = cree_transtype(noeud, type_cible, valeur, TypeTranstypage::BITS);
                    valeur_pointeur = valeur;

                    if (noeud->type->genre == GenreType::ENTIER_CONSTANT) {
                        valeur_taille = cree_z64(4);
                    }
                    else {
                        valeur_taille = cree_z64(noeud->type->taille_octet);
                    }

                    break;
                }
                case GenreType::POINTEUR:
                {
                    auto type_pointe = noeud->type->comme_pointeur()->type_pointe;
                    valeur = cree_transtype(noeud, type_cible, valeur, TypeTranstypage::BITS);
                    valeur_pointeur = valeur;
                    auto taille_type = type_pointe->taille_octet;
                    valeur_taille = cree_z64(taille_type);
                    break;
                }
                case GenreType::CHAINE:
                {
                    valeur_pointeur = cree_reference_membre_et_charge(noeud, valeur, 0);
                    valeur_pointeur = cree_transtype(
                        noeud, type_cible, valeur_pointeur, TypeTranstypage::BITS);
                    valeur_taille = cree_reference_membre_et_charge(noeud, valeur, 1);
                    break;
                }
                case GenreType::TABLEAU_DYNAMIQUE:
                {
                    auto type_pointer = noeud->type->comme_tableau_dynamique()->type_pointe;

                    valeur_pointeur = cree_reference_membre_et_charge(noeud, valeur, 0);
                    valeur_pointeur = cree_transtype(
                        noeud, type_cible, valeur_pointeur, TypeTranstypage::BITS);
                    valeur_taille = cree_reference_membre_et_charge(noeud, valeur, 1);

                    auto taille_type = type_pointer->taille_octet;

                    valeur_taille = cree_op_binaire(noeud,
                                                    m_espace->typeuse[TypeBase::Z64],
                                                    OperateurBinaire::Genre::Multiplication,
                                                    valeur_taille,
                                                    cree_z64(taille_type));

                    break;
                }
                case GenreType::TABLEAU_FIXE:
                {
                    auto type_tabl = noeud->type->comme_tableau_fixe();
                    auto type_pointe = type_tabl->type_pointe;
                    auto taille_type = type_pointe->taille_octet;

                    valeur_pointeur = cree_acces_index(noeud, valeur, cree_z64(0ul));
                    valeur_pointeur = cree_transtype(
                        noeud, type_cible, valeur_pointeur, TypeTranstypage::BITS);
                    valeur_taille = cree_z64(static_cast<unsigned>(type_tabl->taille) *
                                             taille_type);

                    break;
                }
            }

            /* alloue de l'espace pour ce type */
            auto tabl_octet = cree_allocation(
                noeud, m_espace->typeuse[TypeBase::TABL_OCTET], nullptr);

            auto pointeur_tabl_octet = cree_reference_membre(noeud, tabl_octet, 0);
            cree_stocke_mem(noeud, pointeur_tabl_octet, valeur_pointeur);

            auto taille_tabl_octet = cree_reference_membre(noeud, tabl_octet, 1);
            cree_stocke_mem(noeud, taille_tabl_octet, valeur_taille);

            valeur = cree_charge_mem(noeud, tabl_octet);
            break;
        }
        case TypeTransformation::CONVERTI_TABLEAU:
        {
            if (fonction_courante == nullptr) {
                auto valeur_tableau_fixe = static_cast<AtomeConstante *>(valeur);
                empile_valeur(cree_tableau_global(valeur_tableau_fixe));
                return;
            }

            valeur = converti_vers_tableau_dyn(
                noeud, valeur, noeud->type->comme_tableau_fixe(), place);

            if (place == nullptr) {
                valeur = cree_charge_mem(noeud, valeur);
            }
            else {
                place_fut_utilisee = true;
            }

            break;
        }
        case TypeTransformation::FONCTION:
        {
            auto atome_fonction = m_espace->trouve_ou_insere_fonction(
                *this, const_cast<NoeudDeclarationEnteteFonction *>(transformation.fonction));

            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            auto args = kuri::tableau<Atome *, int>();
            args.ajoute(valeur);

            valeur = cree_appel(noeud, noeud->lexeme, atome_fonction, std::move(args));
            break;
        }
        case TypeTransformation::PREND_REFERENCE:
        {
            // RÀF : valeur doit déjà être un pointeur
            break;
        }
        case TypeTransformation::DEREFERENCE:
        {
            valeur = cree_charge_mem(noeud, valeur);
            valeur = cree_charge_mem(noeud, valeur);
            break;
        }
        case TypeTransformation::CONVERTI_VERS_BASE:
        {
            // À FAIRE : décalage dans la structure
            valeur = cree_charge_mem(noeud, valeur);
            valeur = cree_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::BITS);
            break;
        }
    }

    if (place && !place_fut_utilisee) {
        cree_stocke_mem(noeud, place, valeur);
    }

    empile_valeur(valeur);
}

void ConstructriceRI::genere_ri_pour_tente(NoeudInstructionTente *noeud)
{
    // À FAIRE(retours multiples)
    genere_ri_pour_expression_droite(noeud->expression_appelee, nullptr);
    auto valeur_expression = depile_valeur();

    struct DonneesGenerationCodeTente {
        Atome *acces_variable{};
        Atome *acces_erreur{};
        Atome *acces_erreur_pour_test{};

        Type *type_piege = nullptr;
        Type *type_variable = nullptr;
    };

    DonneesGenerationCodeTente gen_tente;

    if (noeud->expression_appelee->type->genre == GenreType::ERREUR) {
        gen_tente.type_piege = noeud->expression_appelee->type;
        gen_tente.type_variable = gen_tente.type_piege;
        gen_tente.acces_erreur = valeur_expression;
        gen_tente.acces_variable = gen_tente.acces_erreur;
        gen_tente.acces_erreur_pour_test = gen_tente.acces_erreur;

        auto label_si_vrai = reserve_label(noeud);
        auto label_si_faux = reserve_label(noeud);

        auto condition = cree_op_comparaison(noeud,
                                             OperateurBinaire::Genre::Comp_Inegal,
                                             gen_tente.acces_erreur_pour_test,
                                             cree_z32(0));

        cree_branche_condition(noeud, condition, label_si_vrai, label_si_faux);

        insere_label(label_si_vrai);
        if (noeud->expression_piegee == nullptr) {
            auto params = kuri::tableau<Atome *, int>(1);
            params[0] = cree_charge_mem(noeud, contexte);
            cree_appel(noeud,
                       noeud->lexeme,
                       m_espace->trouve_ou_insere_fonction(
                           *this, m_espace->interface_kuri->decl_panique_erreur),
                       std::move(params));
        }
        else {
            auto var_expr_piegee = cree_allocation(
                noeud, gen_tente.type_piege, noeud->expression_piegee->ident);
            auto decl_expr_piegee =
                noeud->expression_piegee->comme_reference_declaration()->declaration_referee;
            static_cast<NoeudDeclarationSymbole *>(decl_expr_piegee)->atome = var_expr_piegee;
            genere_ri_pour_noeud(noeud->bloc);
        }

        insere_label(label_si_faux);

        empile_valeur(valeur_expression);
        return;
    }
    else if (noeud->expression_appelee->type->genre == GenreType::UNION) {
        auto type_union = noeud->expression_appelee->type->comme_union();
        auto index_membre_erreur = 0;

        if (type_union->membres.taille() == 2) {
            if (type_union->membres[0].type->genre == GenreType::ERREUR) {
                gen_tente.type_piege = type_union->membres[0].type;
                gen_tente.type_variable = normalise_type(m_espace->typeuse,
                                                         type_union->membres[1].type);
            }
            else {
                gen_tente.type_piege = type_union->membres[1].type;
                gen_tente.type_variable = normalise_type(m_espace->typeuse,
                                                         type_union->membres[0].type);
                index_membre_erreur = 1;
            }
        }
        else {
            espace()->rapporte_erreur(
                noeud, "Utilisation de « tente » sur une union ayant plus de 2 membres !");
        }

        // test si membre actif est erreur
        auto label_si_vrai = reserve_label(noeud);
        auto label_si_faux = reserve_label(noeud);

        auto valeur_union = cree_allocation(noeud, noeud->expression_appelee->type, nullptr);
        cree_stocke_mem(noeud, valeur_union, valeur_expression);

        auto acces_membre_actif = cree_reference_membre_et_charge(noeud, valeur_union, 1);

        auto condition_membre_actif = cree_op_comparaison(
            noeud,
            OperateurBinaire::Genre::Comp_Egal,
            acces_membre_actif,
            cree_z32(static_cast<unsigned>(index_membre_erreur + 1)));

        cree_branche_condition(noeud, condition_membre_actif, label_si_vrai, label_si_faux);

        insere_label(label_si_vrai);
        if (noeud->expression_piegee == nullptr) {
            auto params = kuri::tableau<Atome *, int>(1);
            params[0] = cree_charge_mem(noeud, contexte);
            cree_appel(noeud,
                       noeud->lexeme,
                       m_espace->trouve_ou_insere_fonction(
                           *this, m_espace->interface_kuri->decl_panique_erreur),
                       std::move(params));
        }
        else {
            Instruction *membre_erreur = cree_reference_membre(noeud, valeur_union, 0);
            membre_erreur = cree_transtype(
                noeud,
                m_espace->typeuse.type_pointeur_pour(gen_tente.type_piege, false),
                membre_erreur,
                TypeTranstypage::BITS);
            membre_erreur->est_chargeable = true;
            auto decl_expr_piegee =
                noeud->expression_piegee->comme_reference_declaration()->declaration_referee;
            static_cast<NoeudDeclarationSymbole *>(decl_expr_piegee)->atome = membre_erreur;
            genere_ri_pour_noeud(noeud->bloc);
        }

        insere_label(label_si_faux);
        valeur_expression = cree_reference_membre(noeud, valeur_union, 0);
        valeur_expression = cree_transtype(
            noeud,
            m_espace->typeuse.type_pointeur_pour(gen_tente.type_variable, false),
            valeur_expression,
            TypeTranstypage::BITS);
        valeur_expression->est_chargeable = true;
    }

    empile_valeur(valeur_expression);
}

void ConstructriceRI::empile_valeur(Atome *valeur)
{
    m_pile.ajoute(valeur);
}

Atome *ConstructriceRI::depile_valeur()
{
    auto v = m_pile.back();
    m_pile.pop_back();
    return v;
}

void ConstructriceRI::genere_ri_pour_declaration_structure(NoeudStruct *noeud)
{
    auto type = noeud->type;

    if (type->genre == GenreType::UNION && type->comme_union()->deja_genere) {
        return;
    }
    else if (type->genre == GenreType::STRUCTURE && type->comme_structure()->deja_genere) {
        return;
    }

    SAUVEGARDE_ETAT(m_pile);
    SAUVEGARDE_ETAT(fonction_courante);
    SAUVEGARDE_ETAT(nombre_labels);
    SAUVEGARDE_ETAT(taille_allouee);

    auto fonction = m_espace->trouve_ou_insere_fonction_init(*this, type);
    fonction_courante = fonction;
    cree_label(noeud);

    if (type->genre == GenreType::UNION) {
        auto type_union = type->comme_union();
        auto index_membre = 0u;
        auto pointeur_union = cree_charge_mem(noeud, fonction->params_entrees[0]);

        // À FAIRE(union) : test proprement cette logique
        POUR (type_union->membres) {
            if (it.type != type_union->type_le_plus_grand) {
                index_membre += 1;
                continue;
            }

            auto valeur = static_cast<Atome *>(nullptr);

            if (it.expression_valeur_defaut) {
                genere_ri_pour_expression_droite(it.expression_valeur_defaut, nullptr);
                valeur = depile_valeur();
            }
            else {
                valeur = genere_initialisation_defaut_pour_type(
                    normalise_type(m_espace->typeuse, it.type));
            }

            // valeur peut être nulle pour les initialisations de tableaux fixes
            if (valeur) {
                if (type_union->est_nonsure) {
                    cree_stocke_mem(noeud, pointeur_union, valeur);
                }
                else {
                    auto pointeur = cree_reference_membre(noeud, pointeur_union, 0);
                    cree_stocke_mem(noeud, pointeur, valeur);

                    pointeur = cree_reference_membre(noeud, pointeur_union, 1);
                    cree_stocke_mem(noeud, pointeur, cree_z32(index_membre + 1));
                }
            }

            break;
        }

        type_union->deja_genere = true;
    }
    else if (type->genre == GenreType::STRUCTURE) {
        auto type_struct = type->comme_structure();
        auto index_membre = 0;
        auto pointeur_struct = cree_charge_mem(noeud, fonction->params_entrees[0]);

        POUR (type_struct->membres) {
            if (it.drapeaux == TypeCompose::Membre::EST_CONSTANT) {
                index_membre += 1;
                continue;
            }

            auto valeur = static_cast<Atome *>(nullptr);
            auto pointeur = cree_reference_membre(noeud, pointeur_struct, index_membre);

            if (it.expression_valeur_defaut) {
                genere_ri_pour_expression_droite(it.expression_valeur_defaut, nullptr);
                valeur = depile_valeur();
            }
            else {
                if (it.type->genre == GenreType::STRUCTURE || it.type->genre == GenreType::UNION) {
                    auto atome_fonction = m_espace->trouve_ou_insere_fonction_init(*this, it.type);

                    auto params_init = kuri::tableau<Atome *, int>(1);
                    params_init[0] = pointeur;

                    cree_appel(noeud, noeud->lexeme, atome_fonction, std::move(params_init));
                }
                else {
                    valeur = genere_initialisation_defaut_pour_type(it.type);
                }
            }

            // valeur peut être nulle pour les initialisations de tableaux fixes
            if (valeur) {
                cree_stocke_mem(noeud, pointeur, valeur);
            }

            index_membre += 1;
        }

        type_struct->deja_genere = true;
    }

    type->drapeaux |= RI_TYPE_FUT_GENEREE;

    cree_retour(noeud, nullptr);
}

void ConstructriceRI::genere_ri_pour_acces_membre(NoeudExpressionMembre *noeud)
{
    // À FAIRE(ri) : ceci ignore les espaces de noms.
    auto accede = noeud->accedee;
    auto type_accede = accede->type;

    genere_ri_pour_noeud(accede);
    auto pointeur_accede = depile_valeur();

    while (type_accede->genre == GenreType::POINTEUR ||
           type_accede->genre == GenreType::REFERENCE) {
        type_accede = type_dereference_pour(type_accede);
        pointeur_accede = cree_charge_mem(noeud, pointeur_accede);
    }

    if (pointeur_accede->genre_atome == Atome::Genre::CONSTANTE) {
        auto initialisateur = static_cast<AtomeValeurConstante *>(pointeur_accede);
        auto valeur = initialisateur->valeur.valeur_structure.pointeur[noeud->index_membre];
        empile_valeur(valeur);
        return;
    }

    empile_valeur(cree_reference_membre(noeud, pointeur_accede, noeud->index_membre));
}

void ConstructriceRI::genere_ri_pour_acces_membre_union(NoeudExpressionMembre *noeud)
{
    genere_ri_pour_noeud(noeud->accedee);
    auto ptr_union = depile_valeur();
    auto type = noeud->accedee->type;

    // À FAIRE(union) : doit déréférencer le pointeur
    while (type->genre == GenreType::POINTEUR || type->genre == GenreType::REFERENCE) {
        type = type_dereference_pour(type);
    }

    auto type_union = type->comme_union();
    auto index_membre = noeud->index_membre;
    auto type_membre = type_union->membres[index_membre].type;

    if (type_union->est_nonsure) {
        if (type_membre != type_union->type_le_plus_grand) {
            ptr_union = cree_transtype(noeud,
                                       m_espace->typeuse.type_pointeur_pour(type_membre, false),
                                       ptr_union,
                                       TypeTranstypage::BITS);
            ptr_union->est_chargeable = true;
        }

        empile_valeur(ptr_union);
        return;
    }

    if (expression_gauche) {
        // ajourne l'index du membre
        auto membre_actif = cree_reference_membre(noeud, ptr_union, 1);
        cree_stocke_mem(
            noeud, membre_actif, cree_z32(static_cast<unsigned>(noeud->index_membre + 1)));
    }
    else {
        // vérifie l'index du membre
        auto membre_actif = cree_reference_membre_et_charge(noeud, ptr_union, 1);

        auto label_si_vrai = reserve_label(noeud);
        auto label_si_faux = reserve_label(noeud);

        auto condition = cree_op_comparaison(
            noeud,
            OperateurBinaire::Genre::Comp_Inegal,
            membre_actif,
            cree_z32(static_cast<unsigned>(noeud->index_membre + 1)));

        cree_branche_condition(noeud, condition, label_si_vrai, label_si_faux);
        insere_label(label_si_vrai);
        auto params = kuri::tableau<Atome *, int>(1);
        params[0] = cree_charge_mem(noeud, contexte);
        cree_appel(noeud,
                   noeud->lexeme,
                   m_espace->trouve_ou_insere_fonction(
                       *this, m_espace->interface_kuri->decl_panique_membre_union),
                   std::move(params));
        insere_label(label_si_faux);
    }

    Instruction *pointeur_membre = cree_reference_membre(noeud, ptr_union, 0);

    if (type_membre != type_union->type_le_plus_grand) {
        pointeur_membre = cree_transtype(noeud,
                                         m_espace->typeuse.type_pointeur_pour(type_membre, false),
                                         pointeur_membre,
                                         TypeTranstypage::BITS);
        pointeur_membre->est_chargeable = true;
    }

    empile_valeur(pointeur_membre);
}

AtomeConstante *ConstructriceRI::genere_initialisation_defaut_pour_type(Type *type)
{
    switch (type->genre) {
        case GenreType::RIEN:
        case GenreType::POLYMORPHIQUE:
        {
            return nullptr;
        }
        case GenreType::BOOL:
        {
            return cree_constante_booleenne(false);
        }
        /* Les seules réféences pouvant être nulles sont celles générées par la compilatrice pour
         * les boucles pour. */
        case GenreType::REFERENCE:
        case GenreType::POINTEUR:
        case GenreType::FONCTION:
        {
            return cree_constante_nulle(type);
        }
        case GenreType::OCTET:
        case GenreType::ENTIER_NATUREL:
        case GenreType::ENTIER_RELATIF:
        case GenreType::TYPE_DE_DONNEES:
        {
            return cree_constante_entiere(type, 0);
        }
        case GenreType::ENTIER_CONSTANT:
        {
            return cree_constante_entiere(m_espace->typeuse[TypeBase::Z32], 0);
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 2) {
                return cree_constante_entiere(m_espace->typeuse[TypeBase::N16], 0);
            }

            return cree_constante_reelle(type, 0.0);
        }
        case GenreType::TABLEAU_FIXE:
        {
            // À FAIRE(ri) : initialisation défaut pour tableau fixe
            return nullptr;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_union();

            if (type_union->est_nonsure) {
                return genere_initialisation_defaut_pour_type(type_union->type_le_plus_grand);
            }

            auto valeurs = kuri::tableau<AtomeConstante *>();
            valeurs.reserve(2);

            valeurs.ajoute(genere_initialisation_defaut_pour_type(type_union->type_le_plus_grand));
            valeurs.ajoute(
                genere_initialisation_defaut_pour_type(m_espace->typeuse[TypeBase::Z32]));

            return cree_constante_structure(type, std::move(valeurs));
        }
        case GenreType::CHAINE:
        case GenreType::EINI:
        case GenreType::STRUCTURE:
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::VARIADIQUE:
        case GenreType::TUPLE:
        {
            auto type_compose = static_cast<TypeCompose *>(type);
            auto valeurs = kuri::tableau<AtomeConstante *>();
            valeurs.reserve(type_compose->membres.taille());

            POUR (type_compose->membres) {
                if (it.drapeaux & TypeCompose::Membre::EST_CONSTANT) {
                    continue;
                }

                auto valeur = genere_initialisation_defaut_pour_type(it.type);
                valeurs.ajoute(valeur);
            }

            return cree_constante_structure(type, std::move(valeurs));
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum *>(type);
            return cree_constante_entiere(type_enum, 0);
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_opaque();
            auto valeur = genere_initialisation_defaut_pour_type(type_opaque->type_opacifie);

            // À FAIRE(tableau fixe)
            if (valeur) {
                valeur->type = type_opaque;
            }

            return valeur;
        }
    }

    return nullptr;
}

// Logique tirée de « Basics of Compiler Design », Torben Ægidius Mogensen
void ConstructriceRI::genere_ri_pour_condition(NoeudExpression *condition,
                                               InstructionLabel *label_si_vrai,
                                               InstructionLabel *label_si_faux)
{
    auto genre_lexeme = condition->lexeme->genre;

    if (est_operateur_comparaison(genre_lexeme) ||
        condition->possede_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
        genere_ri_pour_expression_droite(condition, nullptr);
        auto valeur = depile_valeur();
        cree_branche_condition(condition, valeur, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexeme::ESP_ESP) {
        auto expr_bin = condition->comme_expression_binaire();
        auto cond1 = expr_bin->operande_gauche;
        auto cond2 = expr_bin->operande_droite;

        auto nouveau_label = reserve_label(condition);
        genere_ri_pour_condition(cond1, nouveau_label, label_si_faux);
        insere_label(nouveau_label);
        genere_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexeme::BARRE_BARRE) {
        auto expr_bin = condition->comme_expression_binaire();
        auto cond1 = expr_bin->operande_gauche;
        auto cond2 = expr_bin->operande_droite;

        auto nouveau_label = reserve_label(condition);
        genere_ri_pour_condition(cond1, label_si_vrai, nouveau_label);
        insere_label(nouveau_label);
        genere_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexeme::EXCLAMATION) {
        auto expr_unaire = condition->comme_expression_unaire();
        genere_ri_pour_condition(expr_unaire->operande, label_si_faux, label_si_vrai);
    }
    else if (genre_lexeme == GenreLexeme::VRAI) {
        cree_branche(condition, label_si_vrai);
    }
    else if (genre_lexeme == GenreLexeme::FAUX) {
        cree_branche(condition, label_si_faux);
    }
    else if (condition->genre == GenreNoeud::EXPRESSION_PARENTHESE) {
        auto expr_unaire = condition->comme_parenthese();
        genere_ri_pour_condition(expr_unaire->expression, label_si_vrai, label_si_faux);
    }
    else {
        auto type_condition = condition->type;
        auto valeur = static_cast<Atome *>(nullptr);

        switch (type_condition->genre) {
            case GenreType::ENTIER_NATUREL:
            case GenreType::ENTIER_RELATIF:
            case GenreType::ENTIER_CONSTANT:
            {
                genere_ri_pour_expression_droite(condition, nullptr);
                auto valeur1 = depile_valeur();
                auto valeur2 = cree_constante_entiere(type_condition, 0);
                valeur = cree_op_comparaison(
                    condition, OperateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
                break;
            }
            case GenreType::BOOL:
            {
                genere_ri_pour_expression_droite(condition, nullptr);
                valeur = depile_valeur();
                break;
            }
            case GenreType::FONCTION:
            case GenreType::POINTEUR:
            {
                genere_ri_pour_expression_droite(condition, nullptr);
                auto valeur1 = depile_valeur();
                auto valeur2 = cree_constante_nulle(type_condition);
                valeur = cree_op_comparaison(
                    condition, OperateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
                break;
            }
            case GenreType::EINI:
            {
                genere_ri_pour_noeud(condition);
                auto pointeur = depile_valeur();
                auto pointeur_pointeur = cree_reference_membre(condition, pointeur, 0);
                auto valeur1 = cree_charge_mem(condition, pointeur_pointeur);
                auto valeur2 = cree_constante_nulle(valeur1->type);
                valeur = cree_op_comparaison(
                    condition, OperateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
                break;
            }
            case GenreType::CHAINE:
            case GenreType::TABLEAU_DYNAMIQUE:
            {
                genere_ri_pour_noeud(condition);
                auto pointeur = depile_valeur();
                auto pointeur_taille = cree_reference_membre(condition, pointeur, 1);
                auto valeur1 = cree_charge_mem(condition, pointeur_taille);
                auto valeur2 = cree_z64(0);
                valeur = cree_op_comparaison(
                    condition, OperateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
                break;
            }
            default:
            {
                break;
            }
        }

        cree_branche_condition(condition, valeur, label_si_vrai, label_si_faux);
    }
}

void ConstructriceRI::genere_ri_pour_expression_logique(NoeudExpression *noeud, Atome *place)
{
    auto label_si_vrai = reserve_label(noeud);
    auto label_si_faux = reserve_label(noeud);
    auto label_apres_faux = reserve_label(noeud);

    if (place == nullptr) {
        place = cree_allocation(noeud, m_espace->typeuse[TypeBase::BOOL], nullptr);
    }

    genere_ri_pour_condition(noeud, label_si_vrai, label_si_faux);

    insere_label(label_si_vrai);
    cree_stocke_mem(noeud, place, cree_constante_booleenne(true));
    cree_branche(noeud, label_apres_faux);

    insere_label(label_si_faux);
    cree_stocke_mem(noeud, place, cree_constante_booleenne(false));

    insere_label(label_apres_faux);

    empile_valeur(place);
}

void ConstructriceRI::genere_ri_insts_differees(NoeudBloc *bloc, NoeudBloc *bloc_final)
{
#if 0
	if (compilatrice.donnees_fonction->est_coroutine) {
		constructrice << "__etat->__termine_coro = 1;\n";
		constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
		constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
		constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	}
#endif

    while (bloc != bloc_final) {
        for (auto i = bloc->instructions_differees.taille() - 1; i >= 0; --i) {
            auto instruction_differee = bloc->instructions_differees[i];
            genere_ri_pour_noeud(instruction_differee->expression);
        }

        bloc = bloc->bloc_parent;
    }
}

/* À tenir synchronisé avec l'énum dans info_type.kuri
 * Nous utilisons ceci lors de la génération du code des infos types car nous ne
 * générons pas de code (ou symboles) pour les énums, mais prenons directements
 * leurs valeurs.
 */
struct IDInfoType {
    static constexpr unsigned ENTIER = 0;
    static constexpr unsigned REEL = 1;
    static constexpr unsigned BOOLEEN = 2;
    static constexpr unsigned CHAINE = 3;
    static constexpr unsigned POINTEUR = 4;
    static constexpr unsigned STRUCTURE = 5;
    static constexpr unsigned FONCTION = 6;
    static constexpr unsigned TABLEAU = 7;
    static constexpr unsigned EINI = 8;
    static constexpr unsigned RIEN = 9;
    static constexpr unsigned ENUM = 10;
    static constexpr unsigned OCTET = 11;
    static constexpr unsigned TYPE_DE_DONNEES = 12;
    static constexpr unsigned UNION = 13;
    static constexpr unsigned OPAQUE = 14;
};

AtomeConstante *ConstructriceRI::cree_info_type(Type *type)
{
    if (type->atome_info_type != nullptr) {
        return type->atome_info_type;
    }

    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        case GenreType::TUPLE:
        {
            assert_rappel(false, []() { std::cerr << "Obtenu un type tuple ou polymophique\n"; });
            break;
        }
        case GenreType::BOOL:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::BOOLEEN, type->taille_octet);
            break;
        }
        case GenreType::OCTET:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::OCTET, type->taille_octet);
            break;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            type->atome_info_type = cree_info_type_entier(4, true);
            break;
        }
        case GenreType::ENTIER_NATUREL:
        {
            type->atome_info_type = cree_info_type_entier(type->taille_octet, false);
            break;
        }
        case GenreType::ENTIER_RELATIF:
        {
            type->atome_info_type = cree_info_type_entier(type->taille_octet, true);
            break;
        }
        case GenreType::REEL:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::REEL, type->taille_octet);
            break;
        }
        case GenreType::REFERENCE:
        case GenreType::POINTEUR:
        {
            /* { id, taille_en_octet type_pointé, est_référence } */

            auto type_pointeur = m_espace->typeuse.type_info_type_pointeur;

            auto valeur_id = cree_z32(IDInfoType::POINTEUR);
            auto valeur_taille_octet = cree_z32(type->taille_octet);

            auto type_deref = type_dereference_pour(type);
            auto valeur_type_pointe = cree_info_type(type_deref);
            valeur_type_pointe = cree_transtype_constant(
                m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                valeur_type_pointe);

            auto est_reference = cree_constante_booleenne(type->genre == GenreType::REFERENCE);

            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = valeur_id;
            valeurs[1] = valeur_taille_octet;
            valeurs[2] = valeur_type_pointe;
            valeurs[3] = est_reference;

            auto initialisateur = cree_constante_structure(type_pointeur, std::move(valeurs));
            type->atome_info_type = cree_globale(type_pointeur, initialisateur, false, true);
            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum *>(type);

            auto type_info_type_enum = m_espace->typeuse.type_info_type_enum;

            /* { id: e32, taille_en_octet, nom: chaine, valeurs: [], membres: [], est_drapeau: bool
             * } */

            auto valeur_id = cree_z32(IDInfoType::ENUM);
            auto valeur_taille_octet = cree_z32(type->taille_octet);
            auto est_drapeau = cree_constante_booleenne(type_enum->est_drapeau);

            auto struct_chaine = cree_chaine(type_enum->nom->nom);

            /* création des tableaux de valeurs et de noms */

            kuri::tableau<AtomeConstante *> valeurs_enum;
            valeurs_enum.reserve(type_enum->membres.taille());

            POUR (type_enum->membres) {
                if (it.drapeaux == TypeCompose::Membre::EST_IMPLICITE) {
                    continue;
                }

                auto valeur = cree_z32(static_cast<unsigned>(it.valeur));
                valeurs_enum.ajoute(valeur);
            }

            kuri::tableau<AtomeConstante *> noms_enum;
            noms_enum.reserve(type_enum->membres.taille());

            POUR (type_enum->membres) {
                if (it.drapeaux == TypeCompose::Membre::EST_IMPLICITE) {
                    continue;
                }

                auto chaine_nom = cree_chaine(it.nom->nom);
                noms_enum.ajoute(chaine_nom);
            }

            auto tableau_valeurs = cree_tableau_global(m_espace->typeuse[TypeBase::Z32],
                                                       std::move(valeurs_enum));
            auto tableau_noms = cree_tableau_global(m_espace->typeuse[TypeBase::CHAINE],
                                                    std::move(noms_enum));

            /* création de l'info type */

            auto valeurs = kuri::tableau<AtomeConstante *>(6);
            valeurs[0] = valeur_id;
            valeurs[1] = valeur_taille_octet;
            valeurs[2] = struct_chaine;
            valeurs[3] = tableau_valeurs;
            valeurs[4] = tableau_noms;
            valeurs[5] = est_drapeau;

            auto initialisateur = cree_constante_structure(type_info_type_enum,
                                                           std::move(valeurs));
            type->atome_info_type = cree_globale(type_info_type_enum, initialisateur, false, true);
            break;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_union();
            auto nom_union = kuri::chaine_statique();

            if (type_union->est_anonyme) {
                nom_union = "anonyme";
            }
            else {
                nom_union = type_union->decl->ident->nom;
            }

            // ------------------------------------
            // Commence par assigner une globale non-initialisée comme info type
            // pour éviter de recréer plusieurs fois le même info type.
            auto type_info_union = m_espace->typeuse.type_info_type_union;

            auto globale = cree_globale(type_info_union, nullptr, false, true);
            type->atome_info_type = globale;

            // ------------------------------------
            /* pour chaque membre cree une instance de InfoTypeMembreStructure */
            auto type_struct_membre = m_espace->typeuse.type_info_type_membre_structure;

            kuri::tableau<AtomeConstante *> valeurs_membres;

            POUR (type_union->membres) {
                /* { nom: chaine, info : *InfoType, décalage, drapeaux } */
                auto type_membre = it.type;

                auto info_type = cree_info_type(type_membre);
                info_type = cree_transtype_constant(
                    m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                    info_type);

                auto valeur_nom = cree_chaine(it.nom->nom);
                auto valeur_decalage = cree_z64(static_cast<uint64_t>(it.decalage));
                auto valeur_drapeaux = cree_z32(static_cast<unsigned>(it.drapeaux));

                auto valeurs = kuri::tableau<AtomeConstante *>(4);
                valeurs[0] = valeur_nom;
                valeurs[1] = info_type;
                valeurs[2] = valeur_decalage;
                valeurs[3] = valeur_drapeaux;

                auto initialisateur = cree_constante_structure(type_struct_membre,
                                                               std::move(valeurs));

                /* Création d'un InfoType globale. */
                auto globale_membre = cree_globale(
                    type_struct_membre, initialisateur, false, true);
                valeurs_membres.ajoute(globale_membre);
            }

            /* id : n32
             * taille_en_octet: z32
             * nom: chaine
             * membres : []InfoTypeMembreStructure
             * type_le_plus_grand : *InfoType
             * décalage_index : z64
             * est_sûre: bool
             */
            auto info_type_plus_grand = cree_info_type(type_union->type_le_plus_grand);
            info_type_plus_grand = cree_transtype_constant(
                m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                info_type_plus_grand);

            auto valeur_id = cree_z32(IDInfoType::UNION);
            auto valeur_taille_octet = cree_z32(type->taille_octet);
            auto valeur_nom = cree_chaine(nom_union);
            auto valeur_est_sure = cree_constante_booleenne(!type_union->est_nonsure);
            auto valeur_type_le_plus_grand = info_type_plus_grand;
            auto valeur_decalage_index = cree_z64(type_union->decalage_index);

            // Pour les références à des globales, nous devons avoir un type pointeur.
            auto type_membre = m_espace->typeuse.type_pointeur_pour(type_struct_membre, false);

            auto tableau_membre = cree_tableau_global(type_membre, std::move(valeurs_membres));

            auto valeurs = kuri::tableau<AtomeConstante *>(7);
            valeurs[0] = valeur_id;
            valeurs[1] = valeur_taille_octet;
            valeurs[2] = valeur_nom;
            valeurs[3] = tableau_membre;
            valeurs[4] = valeur_type_le_plus_grand;
            valeurs[5] = valeur_decalage_index;
            valeurs[6] = valeur_est_sure;

            globale->initialisateur = cree_constante_structure(type_info_union,
                                                               std::move(valeurs));

            break;
        }
        case GenreType::STRUCTURE:
        {
            auto type_struct = type->comme_structure();

            // ------------------------------------
            // Commence par assigner une globale non-initialisée comme info type
            // pour éviter de recréer plusieurs fois le même info type.
            auto type_info_struct = m_espace->typeuse.type_info_type_structure;

            auto globale = cree_globale(type_info_struct, nullptr, false, true);
            type->atome_info_type = globale;

            // ------------------------------------
            /* pour chaque membre cree une instance de InfoTypeMembreStructure */
            auto type_struct_membre = m_espace->typeuse.type_info_type_membre_structure;

            kuri::tableau<AtomeConstante *> valeurs_membres;

            POUR (type_struct->membres) {
                /* { nom: chaine, info : *InfoType, décalage, drapeaux } */
                auto type_membre = it.type;

                auto info_type = cree_info_type(type_membre);
                info_type = cree_transtype_constant(
                    m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                    info_type);

                auto valeur_nom = cree_chaine(it.nom->nom);
                auto valeur_decalage = cree_z64(static_cast<uint64_t>(it.decalage));
                auto valeur_drapeaux = cree_z32(static_cast<unsigned>(it.drapeaux));

                auto valeurs = kuri::tableau<AtomeConstante *>(4);
                valeurs[0] = valeur_nom;
                valeurs[1] = info_type;
                valeurs[2] = valeur_decalage;
                valeurs[3] = valeur_drapeaux;

                auto initialisateur = cree_constante_structure(type_struct_membre,
                                                               std::move(valeurs));

                /* Création d'un InfoType globale. */
                auto globale_membre = cree_globale(
                    type_struct_membre, initialisateur, false, true);
                valeurs_membres.ajoute(globale_membre);
            }

            /* { id : n32, taille_en_octet, nom: chaine, membres : []InfoTypeMembreStructure } */

            auto valeur_id = cree_z32(IDInfoType::STRUCTURE);
            auto valeur_taille_octet = cree_z32(type->taille_octet);
            auto valeur_nom = cree_chaine(type_struct->nom->nom);

            // Pour les références à des globales, nous devons avoir un type pointeur.
            auto type_membre = m_espace->typeuse.type_pointeur_pour(type_struct_membre, false);

            auto tableau_membre = cree_tableau_global(type_membre, std::move(valeurs_membres));

            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = valeur_id;
            valeurs[1] = valeur_taille_octet;
            valeurs[2] = valeur_nom;
            valeurs[3] = tableau_membre;

            globale->initialisateur = cree_constante_structure(type_info_struct,
                                                               std::move(valeurs));
            break;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::VARIADIQUE:
        {
            /* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */

            auto type_pointeur = m_espace->typeuse.type_info_type_tableau;

            auto valeur_id = cree_z32(IDInfoType::TABLEAU);
            auto valeur_taille_octet = cree_z32(type->taille_octet);

            auto type_deref = type_dereference_pour(type);

            auto valeur_type_pointe = cree_info_type(type_deref);
            valeur_type_pointe = cree_transtype_constant(
                m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                valeur_type_pointe);

            auto valeur_est_fixe = cree_constante_booleenne(false);
            auto valeur_taille_fixe = cree_z32(0);

            auto valeurs = kuri::tableau<AtomeConstante *>(5);
            valeurs[0] = valeur_id;
            valeurs[1] = valeur_taille_octet;
            valeurs[2] = valeur_type_pointe;
            valeurs[3] = valeur_est_fixe;
            valeurs[4] = valeur_taille_fixe;

            auto initialisateur = cree_constante_structure(type_pointeur, std::move(valeurs));

            type->atome_info_type = cree_globale(type_pointeur, initialisateur, false, true);
            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_tableau_fixe();
            /* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */

            auto type_pointeur = m_espace->typeuse.type_info_type_tableau;

            auto valeur_id = cree_z32(IDInfoType::TABLEAU);
            auto valeur_taille_octet = cree_z32(type->taille_octet);

            auto valeur_type_pointe = cree_info_type(type_tableau->type_pointe);
            valeur_type_pointe = cree_transtype_constant(
                m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                valeur_type_pointe);

            auto valeur_est_fixe = cree_constante_booleenne(true);
            auto valeur_taille_fixe = cree_z32(static_cast<unsigned>(type_tableau->taille));

            auto valeurs = kuri::tableau<AtomeConstante *>(5);
            valeurs[0] = valeur_id;
            valeurs[1] = valeur_taille_octet;
            valeurs[2] = valeur_type_pointe;
            valeurs[3] = valeur_est_fixe;
            valeurs[4] = valeur_taille_fixe;

            auto initialisateur = cree_constante_structure(type_pointeur, std::move(valeurs));

            type->atome_info_type = cree_globale(type_pointeur, initialisateur, false, true);
            break;
        }
        case GenreType::FONCTION:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::FONCTION,
                                                          type->taille_octet);
            break;
        }
        case GenreType::EINI:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::EINI, type->taille_octet);
            break;
        }
        case GenreType::RIEN:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::RIEN, type->taille_octet);
            break;
        }
        case GenreType::CHAINE:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::CHAINE, type->taille_octet);
            break;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::TYPE_DE_DONNEES,
                                                          type->taille_octet);
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_opaque();
            auto type_opacifie = type_opaque->type_opacifie;

            /* { id, taille_en_octet, nom, type_opacifié } */
            auto type_info_type_opaque = m_espace->typeuse.type_info_type_opaque;

            auto valeur_id = cree_z32(IDInfoType::OPAQUE);
            auto valeur_taille_octet = cree_z32(type_opacifie->taille_octet);
            auto valeur_nom = cree_chaine(type_opaque->ident->nom);
            auto valeur_type_opacifie = cree_info_type(type_opacifie);
            valeur_type_opacifie = cree_transtype_constant(
                m_espace->typeuse.type_pointeur_pour(m_espace->typeuse.type_info_type_, false),
                valeur_type_opacifie);

            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = valeur_id;
            valeurs[1] = valeur_taille_octet;
            valeurs[2] = valeur_nom;
            valeurs[3] = valeur_type_opacifie;

            auto initialisateur = cree_constante_structure(type_info_type_opaque,
                                                           std::move(valeurs));
            type->atome_info_type = cree_globale(
                type_info_type_opaque, initialisateur, false, true);
            break;
        }
    }

    return type->atome_info_type;
}

AtomeConstante *ConstructriceRI::cree_info_type_defaut(unsigned index, unsigned taille_octet)
{
    auto valeur_id = cree_z32(index);
    auto valeur_taille_octet = cree_z32(taille_octet);

    auto valeurs = kuri::tableau<AtomeConstante *>(2);
    valeurs[0] = valeur_id;
    valeurs[1] = valeur_taille_octet;

    auto initialisateur = cree_constante_structure(m_espace->typeuse.type_info_type_,
                                                   std::move(valeurs));

    return cree_globale(m_espace->typeuse.type_info_type_, initialisateur, false, true);
}

AtomeConstante *ConstructriceRI::cree_info_type_entier(unsigned taille_octet, bool est_relatif)
{
    auto valeur_id = cree_z32(IDInfoType::ENTIER);
    auto valeur_taille_octet = cree_z32(taille_octet);

    auto valeurs = kuri::tableau<AtomeConstante *>(3);
    valeurs[0] = valeur_id;
    valeurs[1] = valeur_taille_octet;
    valeurs[2] = cree_constante_booleenne(est_relatif);

    auto type_info_entier = m_espace->typeuse.type_info_type_entier;
    auto initialisateur = cree_constante_structure(type_info_entier, std::move(valeurs));

    return cree_globale(type_info_entier, initialisateur, false, true);
}

void ConstructriceRI::genere_ri_pour_position_code_source(NoeudExpression *noeud)
{
    if (m_noeud_pour_appel) {
        noeud = m_noeud_pour_appel;
    }

    auto type_position = m_espace->typeuse.type_position_code_source;

    auto alloc = cree_allocation(noeud, type_position, nullptr);

    // fichier
    auto const &fichier = m_espace->fichier(noeud->lexeme->fichier);
    // À FAIRE : sécurité, n'utilise pas le chemin, mais détermine une manière fiable
    // et robuste d'obtenir le fichier, utiliser simplement le nom n'est pas fiable
    // (d'autres fichiers du même nom dans le module)
    auto chaine_nom_fichier = cree_chaine(fichier->chemin());
    auto ptr_fichier = cree_reference_membre(noeud, alloc, 0);
    cree_stocke_mem(noeud, ptr_fichier, chaine_nom_fichier);

    // fonction
    auto nom_fonction = kuri::chaine_statique("");

    if (fonction_courante != nullptr) {
        nom_fonction = fonction_courante->nom;
    }

    auto chaine_fonction = cree_chaine(nom_fonction);
    auto ptr_fonction = cree_reference_membre(noeud, alloc, 1);
    cree_stocke_mem(noeud, ptr_fonction, chaine_fonction);

    // ligne
    auto pos = position_lexeme(*noeud->lexeme);
    auto ligne = cree_z32(static_cast<unsigned>(pos.numero_ligne));
    auto ptr_ligne = cree_reference_membre(noeud, alloc, 2);
    cree_stocke_mem(noeud, ptr_ligne, ligne);

    // colonne
    auto colonne = cree_z32(static_cast<unsigned>(pos.pos));
    auto ptr_colonne = cree_reference_membre(noeud, alloc, 3);
    cree_stocke_mem(noeud, ptr_colonne, colonne);

    empile_valeur(alloc);
}

Atome *ConstructriceRI::converti_vers_tableau_dyn(NoeudExpression *noeud,
                                                  Atome *pointeur_tableau_fixe,
                                                  TypeTableauFixe *type_tableau_fixe,
                                                  Atome *place)
{
    auto alloc_tableau_dyn = place;

    if (alloc_tableau_dyn == nullptr) {
        auto type_tableau_dyn = m_espace->typeuse.type_tableau_dynamique(
            type_tableau_fixe->type_pointe);
        alloc_tableau_dyn = cree_allocation(noeud, type_tableau_dyn, nullptr);
    }

    auto ptr_pointeur_donnees = cree_reference_membre(noeud, alloc_tableau_dyn, 0);
    auto premier_elem = cree_acces_index(noeud, pointeur_tableau_fixe, cree_z64(0ul));
    cree_stocke_mem(noeud, ptr_pointeur_donnees, premier_elem);

    auto ptr_taille = cree_reference_membre(noeud, alloc_tableau_dyn, 1);
    auto constante = cree_z64(unsigned(type_tableau_fixe->taille));
    cree_stocke_mem(noeud, ptr_taille, constante);

    return alloc_tableau_dyn;
}

AtomeConstante *ConstructriceRI::cree_chaine(kuri::chaine_statique chaine)
{
    auto table_chaines = m_espace->table_chaines.verrou_ecriture();
    auto trouve = false;
    auto valeur = table_chaines->trouve(chaine, trouve);

    if (trouve) {
        return valeur;
    }

    auto type_chaine = m_espace->typeuse.type_chaine;

    auto type_tableau = m_espace->typeuse.type_tableau_fixe(m_espace->typeuse[TypeBase::Z8],
                                                            static_cast<int>(chaine.taille()));
    auto tableau = cree_constante_tableau_donnees_constantes(
        type_tableau, const_cast<char *>(chaine.pointeur()), chaine.taille());

    auto globale_tableau = cree_globale(type_tableau, tableau, false, true);
    auto pointeur_chaine = cree_acces_index_constant(globale_tableau, cree_z64(0));
    auto taille_chaine = cree_z64(static_cast<unsigned long>(chaine.taille()));

    auto membres = kuri::tableau<AtomeConstante *>(2);
    membres[0] = pointeur_chaine;
    membres[1] = taille_chaine;

    auto constante_chaine = cree_constante_structure(type_chaine, std::move(membres));

    table_chaines->insere(chaine, constante_chaine);

    return constante_chaine;
}

AtomeFonction *ConstructriceRI::genere_ri_pour_fonction_principale()
{
    nombre_labels = 0;

    // déclare une fonction de type int(ContexteProgramme) appelée __principale
    auto types_entrees = dls::tablet<Type *, 6>(1);
    types_entrees[0] = m_espace->typeuse.type_contexte;

    auto type_sortie = m_espace->typeuse[TypeBase::Z32];

    auto type_fonction = m_espace->typeuse.type_fonction(types_entrees, type_sortie, false);

    auto alloc_contexte = cree_allocation(nullptr, m_espace->typeuse.type_contexte, ID::contexte);
    contexte = alloc_contexte;

    auto params = kuri::tableau<Atome *, int>(1);
    params[0] = alloc_contexte;

    auto fonction = m_espace->cree_fonction(nullptr, "__principale", std::move(params));
    fonction->type = type_fonction;
    fonction->sanstrace = true;
    fonction->nombre_utilisations = 1;

    /* Crée également un paramètre pour le retour, les coulisses en ayant besoin,
     * car nous y devons prédéclarer les valeurs de retours. */
    fonction->param_sortie = cree_allocation(
        nullptr, m_espace->typeuse[TypeBase::Z32], ID::valeur);

    fonction_courante = fonction;

    cree_label(nullptr);

    // ----------------------------------
    // initialise les variables globales
    auto constructeurs_globaux = m_espace->constructeurs_globaux.verrou_lecture();

    POUR (*constructeurs_globaux) {
        if (it.expression->est_non_initialisation()) {
            continue;
        }

        genere_ri_transformee_pour_noeud(it.expression, it.atome, it.transformation);
    }

    // ----------------------------------
    // appel notre fonction principale en passant le contexte et le tableau
    auto fonc_princ = m_espace->fonction_principale;

    auto params_principale = kuri::tableau<Atome *, int>(1);
    params_principale[0] = cree_charge_mem(nullptr, fonction->params_entrees[0]);

    static Lexeme lexeme_appel_principale = {
        "principale", {}, GenreLexeme::CHAINE_CARACTERE, 0, 0, 0};
    lexeme_appel_principale.ident = ID::principale;

    auto valeur_princ = cree_appel(
        nullptr, &lexeme_appel_principale, fonc_princ->atome, std::move(params_principale));

    // return
    cree_retour(nullptr, valeur_princ);

    return fonction;
}

void ConstructriceRI::genere_ri_pour_fonction_metaprogramme(
    NoeudDeclarationEnteteFonction *fonction)
{
    assert(fonction->est_metaprogramme);
    auto atome_fonc = m_espace->trouve_ou_insere_fonction(*this, fonction);

    fonction_courante = atome_fonc;
    taille_allouee = 0;

    auto decl_creation_contexte = m_espace->interface_kuri->decl_creation_contexte;

    auto atome_creation_contexte = m_espace->trouve_ou_insere_fonction(*this,
                                                                       decl_creation_contexte);

    atome_fonc->instructions.reserve(atome_creation_contexte->instructions.taille());

    for (auto i = 0; i < atome_creation_contexte->instructions.taille(); ++i) {
        auto it = atome_creation_contexte->instructions[i];
        atome_fonc->instructions.ajoute(it);

        if (it->genre == Instruction::Genre::ALLOCATION && it->ident == ID::contexte) {
            contexte = it;
        }
    }

    atome_fonc->decalage_appel_init_globale = atome_fonc->instructions.taille();

    genere_ri_pour_noeud(fonction->corps->bloc);

    fonction->drapeaux |= RI_FUT_GENEREE;

    fonction_courante = nullptr;
    contexte = nullptr;
}

void ConstructriceRI::genere_ri_pour_declaration_variable(NoeudDeclarationVariable *decl)
{
    if (decl->possede_drapeau(EST_CONSTANTE)) {
        return;
    }

    if (fonction_courante == nullptr) {
        POUR (decl->donnees_decl.plage()) {
            for (auto i = 0; i < it.variables.taille(); ++i) {
                auto var = it.variables[i];
                auto est_externe = dls::outils::possede_drapeau(decl->drapeaux, EST_EXTERNE);
                auto valeur = static_cast<AtomeConstante *>(nullptr);
                auto atome = m_espace->trouve_ou_insere_globale(decl);
                atome->est_externe = est_externe;
                atome->est_constante = false;
                atome->ident = var->ident;

                auto expression = it.expression;

                if (!expression) {
                    valeur = genere_initialisation_defaut_pour_type(var->type);
                }
                else {
                    if (expression->substitution) {
                        expression = expression->substitution;
                    }

                    if (expression->genre == GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU &&
                        it.transformations[i].type == TypeTransformation::INUTILE) {
                        genere_ri_pour_noeud(expression);
                        valeur = static_cast<AtomeConstante *>(depile_valeur());
                    }
                    else {
                        m_espace->constructeurs_globaux->ajoute(
                            {atome, expression, it.transformations[i]});
                    }
                }

                atome->initialisateur = valeur;
            }
        }

        decl->drapeaux |= RI_FUT_GENEREE;
        return;
    }

    auto alloc_pointeur = [this, decl](NoeudExpression *var) {
        Atome *pointeur = nullptr;

        // les allocations pour les variables employées sont créées lors de la génération de code
        // pour les blocs
        if (decl->drapeaux & EMPLOYE) {
            pointeur = decl->atome;
            assert_rappel(pointeur != nullptr, [=]() {
                std::cerr << "Aucune allocation pour « " << var->ident->nom << " »\n.";
            });
        }
        else {
            pointeur = cree_allocation(var, var->type, var->ident);
        }

        return pointeur;
    };

    POUR (decl->donnees_decl.plage()) {
        auto expression = it.expression;

        if (expression) {
            if (expression->genre != GenreNoeud::INSTRUCTION_NON_INITIALISATION) {
                auto ancienne_expression_gauche = expression_gauche;
                expression_gauche = false;
                genere_ri_pour_noeud(expression);
                expression_gauche = ancienne_expression_gauche;

                if (it.multiple_retour) {
                    auto valeur_tuple = depile_valeur();

                    for (auto i = 0; i < it.variables.taille(); ++i) {
                        auto var = it.variables[i];
                        auto &transformation = it.transformations[i];
                        auto pointeur = alloc_pointeur(var);
                        static_cast<NoeudDeclarationSymbole *>(
                            var->comme_reference_declaration()->declaration_referee)
                            ->atome = pointeur;

                        auto valeur = cree_reference_membre(expression, valeur_tuple, i);
                        transforme_valeur(expression, valeur, transformation, pointeur);
                        depile_valeur();
                    }
                }
                else {
                    auto valeur = depile_valeur();

                    for (auto i = 0; i < it.variables.taille(); ++i) {
                        auto var = it.variables[i];
                        auto &transformation = it.transformations[i];
                        auto pointeur = alloc_pointeur(var);
                        static_cast<NoeudDeclarationSymbole *>(
                            var->comme_reference_declaration()->declaration_referee)
                            ->atome = pointeur;

                        transforme_valeur(expression, valeur, transformation, pointeur);
                        depile_valeur();
                    }
                }
            }
            else {
                for (auto &var : it.variables.plage()) {
                    auto pointeur = alloc_pointeur(var);
                    static_cast<NoeudDeclarationSymbole *>(
                        var->comme_reference_declaration()->declaration_referee)
                        ->atome = pointeur;
                }
            }
        }
        else {
            for (auto &var : it.variables.plage()) {
                auto pointeur = alloc_pointeur(var);

                // À FAIRE: appel pour les opaques étant des structures
                auto type_var = var->type;
                if (type_var->est_opaque()) {
                    type_var = type_var->comme_opaque()->type_opacifie;
                }

                if (type_var->genre == GenreType::TABLEAU_FIXE) {
                    // À FAIRE(tableau fixe) : valeur défaut
                }
                else if (type_var->genre == GenreType::STRUCTURE ||
                         type_var->genre == GenreType::UNION) {
                    auto atome_fonction = m_espace->trouve_ou_insere_fonction_init(*this,
                                                                                   var->type);

                    auto params_init = kuri::tableau<Atome *, int>(1);
                    params_init[0] = pointeur;

                    cree_appel(var, var->lexeme, atome_fonction, std::move(params_init));
                }
                else {
                    auto valeur = genere_initialisation_defaut_pour_type(type_var);
                    cree_stocke_mem(var, pointeur, valeur);
                }

                static_cast<NoeudDeclarationSymbole *>(
                    var->comme_reference_declaration()->declaration_referee)
                    ->atome = pointeur;
            }
        }
    }
}

void ConstructriceRI::rassemble_statistiques(Statistiques &stats)
{
    auto &stats_ri = stats.stats_ri;

#define AJOUTE_ENTREE(Tableau)                                                                    \
    stats_ri.fusionne_entree({#Tableau, Tableau.taille(), Tableau.memoire_utilisee()});

    AJOUTE_ENTREE(atomes_constante)
    AJOUTE_ENTREE(insts_allocation)
    AJOUTE_ENTREE(insts_branche)
    AJOUTE_ENTREE(insts_branche_condition)
    AJOUTE_ENTREE(insts_charge_memoire)
    AJOUTE_ENTREE(insts_label)
    AJOUTE_ENTREE(insts_opbinaire)
    AJOUTE_ENTREE(insts_opunaire)
    AJOUTE_ENTREE(insts_stocke_memoire)
    AJOUTE_ENTREE(insts_retour)
    AJOUTE_ENTREE(insts_accede_index)
    AJOUTE_ENTREE(insts_accede_membre)
    AJOUTE_ENTREE(insts_transtype)
    AJOUTE_ENTREE(transtypes_constants)
    AJOUTE_ENTREE(op_binaires_constants)
    AJOUTE_ENTREE(op_unaires_constants)
    AJOUTE_ENTREE(accede_index_constants)

#undef AJOUTE_ENTREE

    auto memoire = insts_appel.memoire_utilisee();

    pour_chaque_element(insts_appel,
                        [&](InstructionAppel const &it) { memoire += it.args.taille_memoire(); });

    stats_ri.fusionne_entree({"insts_appel", insts_appel.taille(), memoire});
}
