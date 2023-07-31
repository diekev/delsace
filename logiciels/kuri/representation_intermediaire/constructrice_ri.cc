/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

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
#include "portee.hh"
#include "statistiques/statistiques.hh"

#include "analyse.hh"
#include "impression.hh"
#include "optimisations.hh"

#include "structures/enchaineuse.hh"

/* À FAIRE : (représentation intermédiaire, non-urgent)
 * - copie les tableaux fixes quand nous les assignations (a = b -> copie_mem(a, b))
 */

/* ************************************************************************** */

/* Retourne la déclaration de la fontion si l'expression d'appel est un appel vers une telle
 * fontion. */
static NoeudDeclarationEnteteFonction *est_appel_fonction_initialisation(
    NoeudExpression *expression)
{
    if (expression->est_entete_fonction()) {
        auto entete = expression->comme_entete_fonction();
        if (entete->est_initialisation_type) {
            return entete;
        }
        return nullptr;
    }

    if (expression->est_reference_declaration()) {
        return est_appel_fonction_initialisation(
            expression->comme_reference_declaration()->declaration_referee);
    }

    return nullptr;
}

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

AtomeFonction *ConstructriceRI::genere_fonction_init_globales_et_appel(
    EspaceDeTravail *espace,
    const kuri::tableau<AtomeGlobale *> &globales,
    AtomeFonction *fonction_pour)
{
    m_espace = espace;
    return genere_fonction_init_globales_et_appel(globales, fonction_pour);
}

AtomeConstante *ConstructriceRI::cree_constante_entiere(Type const *type, uint64_t valeur)
{
    return atomes_constante.ajoute_element(type, valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_type(Type const *pointeur_type)
{
    return atomes_constante.ajoute_element(m_compilatrice.typeuse.type_type_de_donnees_,
                                           pointeur_type);
}

AtomeConstante *ConstructriceRI::cree_constante_taille_de(Type const *pointeur_type)
{
    auto taille_de = atomes_constante.ajoute_element(m_compilatrice.typeuse[TypeBase::N32],
                                                     pointeur_type);
    taille_de->valeur.genre = AtomeValeurConstante::Valeur::Genre::TAILLE_DE;
    return taille_de;
}

AtomeConstante *ConstructriceRI::cree_z32(uint64_t valeur)
{
    return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z32], valeur);
}

AtomeConstante *ConstructriceRI::cree_z64(uint64_t valeur)
{
    return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z64], valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_reelle(Type const *type, double valeur)
{
    return atomes_constante.ajoute_element(type, valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_structure(
    Type const *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    return atomes_constante.ajoute_element(type, std::move(valeurs));
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_fixe(
    Type const *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto atome = atomes_constante.ajoute_element(type, std::move(valeurs));
    atome->valeur.genre = AtomeValeurConstante::Valeur::Genre::TABLEAU_FIXE;
    return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_donnees_constantes(
    Type const *type, kuri::tableau<char> &&donnees_constantes)
{
    return atomes_constante.ajoute_element(type, std::move(donnees_constantes));
}

AtomeConstante *ConstructriceRI::cree_constante_tableau_donnees_constantes(Type const *type,
                                                                           char *pointeur,
                                                                           int64_t taille)
{
    return atomes_constante.ajoute_element(type, pointeur, taille);
}

AtomeGlobale *ConstructriceRI::cree_globale(Type const *type,
                                            AtomeConstante *initialisateur,
                                            bool est_externe,
                                            bool est_constante)
{
    return m_compilatrice.cree_globale(type, initialisateur, est_externe, est_constante);
}

AtomeConstante *ConstructriceRI::cree_tableau_global(Type const *type,
                                                     kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto taille_tableau = static_cast<int>(valeurs.taille());

    if (taille_tableau == 0) {
        auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(
            const_cast<Type *>(type));
        return genere_initialisation_defaut_pour_type(type_tableau_dyn);
    }

    auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(const_cast<Type *>(type),
                                                                 taille_tableau);
    auto tableau_fixe = cree_constante_tableau_fixe(type_tableau, std::move(valeurs));

    return cree_tableau_global(tableau_fixe);
}

AtomeConstante *ConstructriceRI::cree_tableau_global(AtomeConstante *tableau_fixe)
{
    auto type_tableau_fixe = tableau_fixe->type->comme_tableau_fixe();
    auto globale_tableau_fixe = cree_globale(type_tableau_fixe, tableau_fixe, false, true);
    return cree_initialisation_tableau_global(globale_tableau_fixe, type_tableau_fixe);
}

AtomeConstante *ConstructriceRI::cree_initialisation_tableau_global(
    AtomeGlobale *globale_tableau_fixe, TypeTableauFixe const *type_tableau_fixe)
{
    auto ptr_premier_element = cree_acces_index_constant(globale_tableau_fixe, cree_z64(0));
    auto valeur_taille = cree_z64(static_cast<unsigned>(type_tableau_fixe->taille));
    auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(
        type_tableau_fixe->type_pointe);

    auto membres = kuri::tableau<AtomeConstante *>(3);
    membres[0] = ptr_premier_element;
    membres[1] = valeur_taille;
    membres[2] = valeur_taille;

    return cree_constante_structure(type_tableau_dyn, std::move(membres));
}

AtomeConstante *ConstructriceRI::cree_constante_booleenne(bool valeur)
{
    return atomes_constante.ajoute_element(m_compilatrice.typeuse[TypeBase::BOOL], valeur);
}

AtomeConstante *ConstructriceRI::cree_constante_caractere(Type const *type, uint64_t valeur)
{
    auto atome = atomes_constante.ajoute_element(type, valeur);
    atome->valeur.genre = AtomeValeurConstante::Valeur::Genre::CARACTERE;
    return atome;
}

AtomeConstante *ConstructriceRI::cree_constante_nulle(Type const *type)
{
    return atomes_constante.ajoute_element(type);
}

InstructionBranche *ConstructriceRI::cree_branche(NoeudExpression *site_,
                                                  InstructionLabel *label,
                                                  bool cree_seulement)
{
    auto inst = insts_branche.ajoute_element(site_, label);

    label->nombre_utilisations += 1;

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

    label_si_vrai->nombre_utilisations += 1;
    label_si_faux->nombre_utilisations += 1;

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
    /* La génération de code pour les conditions (#si, #saufsi) et les boucles peut ajouter des
     * labels redondants (par exemple les labels pour après la condition ou la boucle) quand ces
     * instructions sont conséquentes. Afin d'éviter d'avoir des labels définissant des blocs
     * vides, nous ajoutons des branches implicites. */
    if (!fonction_courante->instructions.est_vide()) {
        auto di = fonction_courante->derniere_instruction();
        /* Nous pourrions avoir `if (di->est_label())` pour détecter des labels de blocs vides,
         * mais la génération de code pour par exemple les conditions d'une instructions `si` sans
         * `sinon` ne met pas de branche à la fin de `si.bloc_si_vrai`. Donc ceci permet de
         * détecter également ces cas. */
        if (!di->est_branche_ou_retourne()) {
            cree_branche(label->site, label);
        }
    }

    fonction_courante->instructions.ajoute(label);
}

void ConstructriceRI::insere_label_si_utilise(InstructionLabel *label)
{
    if (label->nombre_utilisations == 0) {
        return;
    }

    insere_label(label);
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

    auto types_entrees = kuri::tablet<Type *, 6>(0);
    auto type_sortie = m_compilatrice.typeuse[TypeBase::RIEN];

    auto fonction = m_compilatrice.cree_fonction(nullptr, nom_fontion);
    fonction->type = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie, false);

    this->fonction_courante = fonction;
    this->m_pile.efface();

    auto constructeurs = m_compilatrice.constructeurs_globaux.verrou_lecture();
    auto trouve_constructeur_pour =
        [&constructeurs](
            AtomeGlobale *globale) -> const Compilatrice::DonneesConstructeurGlobale * {
        for (auto &constructeur : *constructeurs) {
            if (globale == constructeur.atome) {
                return &constructeur;
            }
        }
        return nullptr;
    };

    POUR (globales) {
        if (it->est_info_type_de) {
            // À FAIRE : ignore également les globales utilisées uniquement dans celles-ci.
            continue;
        }
        auto constructeur = trouve_constructeur_pour(it);
        if (constructeur) {
            genere_ri_transformee_pour_noeud(
                constructeur->expression, nullptr, constructeur->transformation);
            auto valeur = depile_valeur();
            if (!valeur) {
                continue;
            }
            cree_stocke_mem(nullptr, it, valeur);
        }
        // À FAIRE : it->ident est utilisé car les globales générées par la compilatrice font
        // crasher l'exécution dans la MV. Sans doute, les expressions constantes n'ont pas de
        // place allouée sur la pile, donc l'assignation dépile de la mémoire appartenant à
        // quelqu'un d'autres. Il faudra également avoir un bon système pour garantir un site à
        // imprimer.
        else if (it->initialisateur && it->ident) {
            cree_stocke_mem(nullptr, it, it->initialisateur);
        }
    }

    cree_retour(nullptr, nullptr);

    // crée l'appel de cette fonction et ajoute là au début de la fonction_pour

    this->fonction_courante = fonction_pour;
    cree_appel(nullptr, fonction);

    std::rotate(fonction_pour->instructions.begin() + fonction_pour->decalage_appel_init_globale,
                fonction_pour->instructions.end() - 1,
                fonction_pour->instructions.end());

    this->fonction_courante = nullptr;

    return fonction;
}

InstructionAllocation *ConstructriceRI::cree_allocation(NoeudExpression *site_,
                                                        Type const *type,
                                                        IdentifiantCode *ident,
                                                        bool cree_seulement)
{
    /* le résultat d'une instruction d'allocation est l'adresse de la variable. */
    auto type_pointeur = m_compilatrice.typeuse.type_pointeur_pour(const_cast<Type *>(type),
                                                                   false);
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

static bool est_reference_compatible_pointeur(Type const *type_dest, Type const *type_source)
{
    if (!type_source->est_reference()) {
        return false;
    }

    if (type_source->comme_reference()->type_pointe != type_dest->comme_pointeur()->type_pointe) {
        return false;
    }

    return true;
}

static bool est_type_opacifie(Type const *type_dest, Type const *type_source)
{
    return type_dest->est_opaque() && type_dest->comme_opaque()->type_opacifie == type_source;
}

static bool type_dest_et_type_source_sont_compatibles(Type const *type_dest,
                                                      Type const *type_source)
{
    auto type_élément_dest = type_dereference_pour(type_dest);
    if (type_élément_dest == type_source) {
        return true;
    }

    /* Nous avons différents types de données selon le type connu lors de la compilation. */
    if (type_élément_dest->est_type_de_donnees() && type_source->est_type_de_donnees()) {
        return true;
    }

    if (est_type_opacifie(type_élément_dest, type_source)) {
        return true;
    }

    /* À FAIRE : supprime les entiers constants. */
    if (type_source->est_entier_constant() && est_type_entier(type_élément_dest)) {
        return true;
    }

    /* Certaines références sont converties en pointeur, nous devons vérifier ce cas. Les erreurs
     * de sémantiques devraient déjà avoir été attrappées lors de la validation sémantique.
     * À FAIRE : supprimer les références de la RI, ou les garder totalement. */
    if (est_reference_compatible_pointeur(type_élément_dest, type_source)) {
        return true;
    }

    /* Comme pour au-dessus, dans certains cas une fonction espère une référence mais la valeur est
     * un pointeur. */
    if (type_source->est_pointeur()) {
        if (est_reference_compatible_pointeur(type_source, type_élément_dest)) {
            return true;
        }
    }

    return false;
}

InstructionStockeMem *ConstructriceRI::cree_stocke_mem(NoeudExpression *site_,
                                                       Atome *ou,
                                                       Atome *valeur,
                                                       bool cree_seulement)
{
    assert_rappel(ou->type->est_pointeur() || ou->type->est_reference(), [&]() {
        std::cerr << "Le type n'est pas un pointeur : " << chaine_type(ou->type) << '\n';
        erreur::imprime_site(*m_espace, site_);
    });

    assert_rappel(type_dest_et_type_source_sont_compatibles(ou->type, valeur->type), [&]() {
        auto type_élément_dest = type_dereference_pour(ou->type);
        std::cerr << "\tType élément destination : " << chaine_type(type_élément_dest) << " ("
                  << type_élément_dest << ") "
                  << ", type source : " << chaine_type(valeur->type) << " (" << valeur->type
                  << ") " << '\n';

        erreur::imprime_site(*m_espace, site_);
    });

    auto type = valeur->type;
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

    assert_rappel(
        ou->genre_atome == Atome::Genre::INSTRUCTION || ou->genre_atome == Atome::Genre::GLOBALE,
        [=]() {
            std::cerr << "Le genre de l'atome est : " << static_cast<int>(ou->genre_atome)
                      << ".\n";
        });

    auto type = type_dereference_pour(ou->type);
    auto inst = insts_charge_memoire.ajoute_element(site_, type, ou);

    if (!cree_seulement) {
        fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

InstructionAppel *ConstructriceRI::cree_appel(NoeudExpression *site_, Atome *appele)
{
    // incrémente le nombre d'utilisation au cas où nous appelerions une fonction
    // lorsque que nous enlignons une fonction, son nombre d'utilisations sera décrémentée,
    // et si à 0, nous pourrons ignorer la génération de code final pour celle-ci
    appele->nombre_utilisations += 1;
    auto inst = insts_appel.ajoute_element(site_, appele);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionAppel *ConstructriceRI::cree_appel(NoeudExpression *site_,
                                              Atome *appele,
                                              kuri::tableau<Atome *, int> &&args)
{
    // voir commentaire plus haut
    appele->nombre_utilisations += 1;
    auto inst = insts_appel.ajoute_element(site_, appele, std::move(args));
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

void ConstructriceRI::cree_appel_fonction_init_type(NoeudExpression *site_,
                                                    Type const *type,
                                                    Atome *argument)
{
    auto fonc_init = type->fonction_init;
    auto atome_fonc_init = m_compilatrice.trouve_ou_insere_fonction(*this, fonc_init);
    auto params = kuri::tableau<Atome *, int>(1);

    /* Les fonctions d'initialisation sont partagées entre certains types donc nous devons
     * transtyper vers le type approprié. */
    if (type->est_pointeur() || type->est_fonction()) {
        auto &typeuse = m_compilatrice.typeuse;
        auto type_ptr_ptr_rien = typeuse.type_pointeur_pour(typeuse[TypeBase::PTR_RIEN]);
        argument = cree_transtype(site_, type_ptr_ptr_rien, argument, TypeTranstypage::BITS);
    }

    params[0] = argument;
    cree_appel(site_, atome_fonc_init, std::move(params));
}

InstructionOpUnaire *ConstructriceRI::cree_op_unaire(NoeudExpression *site_,
                                                     Type const *type,
                                                     OperateurUnaire::Genre op,
                                                     Atome *valeur)
{
    auto inst = insts_opunaire.ajoute_element(site_, type, op, valeur);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionOpBinaire *ConstructriceRI::cree_op_binaire(NoeudExpression *site_,
                                                       Type const *type,
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
        site_, m_compilatrice.typeuse[TypeBase::BOOL], op, valeur_gauche, valeur_droite);
}

InstructionAccedeIndex *ConstructriceRI::cree_acces_index(NoeudExpression *site_,
                                                          Atome *accede,
                                                          Atome *index)
{
    auto type_pointe = static_cast<Type const *>(nullptr);
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

    auto type = m_compilatrice.typeuse.type_pointeur_pour(type_dereference_pour(type_pointe),
                                                          false);

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

    auto type_pointe = type_dereference_pour(accede->type);
    if (type_pointe->est_opaque()) {
        type_pointe = type_pointe->comme_opaque()->type_opacifie;
    }

    assert_rappel(est_type_compose(type_pointe), [&]() {
        std::cerr << "Type accédé : '" << chaine_type(type_pointe) << "'\n";
        erreur::imprime_site(*espace(), site_);
    });

    auto type_compose = static_cast<TypeCompose *>(type_pointe);
    if (type_compose->est_union()) {
        type_compose = type_compose->comme_union()->type_structure;
    }

    auto type = type_compose->membres[index].type;
    assert_rappel(
        (type_compose->membres[index].drapeaux & TypeCompose::Membre::PROVIENT_D_UN_EMPOI) == 0,
        [&]() {
            std::cerr << chaine_type(type_compose) << '\n';
            erreur::imprime_site(*espace(), site_);
            imprime_arbre(site_, std::cerr, 0);
        });

    /* nous retournons un pointeur vers le membre */
    type = m_compilatrice.typeuse.type_pointeur_pour(type, false);

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
                                                      Type const *type,
                                                      Atome *valeur,
                                                      TypeTranstypage op)
{
    // std::cerr << __func__ << ", type : " << chaine_type(type) << ", valeur " <<
    // chaine_type(valeur->type) << '\n';
    auto inst = insts_transtype.ajoute_element(site_, type, valeur, op);
    fonction_courante->instructions.ajoute(inst);
    return inst;
}

TranstypeConstant *ConstructriceRI::cree_transtype_constant(Type const *type,
                                                            AtomeConstante *valeur)
{
    return transtypes_constants.ajoute_element(type, valeur);
}

OpUnaireConstant *ConstructriceRI::cree_op_unaire_constant(Type const *type,
                                                           OperateurUnaire::Genre op,
                                                           AtomeConstante *valeur)
{
    return op_unaires_constants.ajoute_element(type, op, valeur);
}

OpBinaireConstant *ConstructriceRI::cree_op_binaire_constant(Type const *type,
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
        m_compilatrice.typeuse[TypeBase::BOOL], op, valeur_gauche, valeur_droite);
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

    auto type = m_compilatrice.typeuse.type_pointeur_pour(
        type_dereference_pour(type_pointeur->type_pointe), false);

    return accede_index_constants.ajoute_element(type, accede, index);
}

/* Retourne la boucle controlée effective de la boucle controlée passé en paramètre. Ceci prend en
 * compte les boucles remplacées par les opérateurs « pour ». */
static NoeudExpression *boucle_controlée_effective(NoeudExpression *boucle_controlée)
{
    if (boucle_controlée->est_pour()) {
        auto noeud_pour = boucle_controlée->comme_pour();

        if (noeud_pour->corps_operateur_pour) {
            /* Nous devons retourner la première boucle parent de #corps_boucle. */
            POUR (noeud_pour->corps_operateur_pour->arbre_aplatis) {
                if (!it->est_directive_corps_boucle()) {
                    continue;
                }

                auto boucle_parent = bloc_est_dans_boucle(it->bloc_parent, nullptr);
                assert(boucle_parent);
                return boucle_controlée_effective(boucle_parent);
            }
        }
    }

    if (boucle_controlée->substitution) {
        return boucle_controlée->substitution;
    }

    return boucle_controlée;
}

void ConstructriceRI::genere_ri_pour_noeud(NoeudExpression *noeud)
{
    if (noeud->substitution) {
        genere_ri_pour_noeud(noeud->substitution);
        return;
    }

    switch (noeud->genre) {
        case GenreNoeud::DECLARATION_BIBLIOTHEQUE:
        case GenreNoeud::DIRECTIVE_DEPENDANCE_BIBLIOTHEQUE:
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::DECLARATION_OPAQUE:
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
        /* Les déclarations de structures doivent passer par les fonctions d'initialisation. */
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            /* Il est possible d'avoir des déclarations de structures dans les fonctions, donc il
             * est possible d'en avoir une ici. */
            assert_rappel(fonction_courante, [&]() {
                std::cerr
                    << "Erreur interne : une déclaration de structure fut passée à la RI !\n";
                erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        /* Ceux-ci doivent être ajoutés aux fonctions d'initialisation/finition de
         * l'environnement d'exécution */
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        {
            assert_rappel(false, [&]() {
                std::cerr
                    << "Erreur interne : une directive #ajoute_fini se retrouve dans la RI !\n";
                erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DIRECTIVE_AJOUTE_INIT:
        {
            assert_rappel(false, [&]() {
                std::cerr
                    << "Erreur interne : une directive #ajoute_init se retrouve dans la RI !\n";
                erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DIRECTIVE_PRE_EXECUTABLE:
        {
            assert_rappel(false, [&]() {
                std::cerr
                    << "Erreur interne : une directive #pré_exécutable se retrouve dans la RI !\n";
                erreur::imprime_site(*m_espace, noeud);
            });
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
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Erreur interne : une expression de construction de structure ne fut "
                             "pas simplifiée !\n";
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
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Erreur interne : une instruction pousse_contexte ne fut pas "
                             "simplifiée !\n";
                erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::EXPRESSION_PARENTHESE:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ENUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        case GenreNoeud::INSTRUCTION_POUR:
        case GenreNoeud::INSTRUCTION_RETIENS:
        case GenreNoeud::OPERATEUR_COMPARAISON_CHAINEE:
        case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Erreur interne : un noeud ne fut pas simplifié !\n";
                std::cerr << "Le noeud est de genre : " << noeud->genre << '\n';
                erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DECLARATION_OPERATEUR_POUR:
        {
            assert_rappel(false, [&]() {
                std::cerr << "Erreur interne : un opérateur « pour » ne fut pas simplifié !\n";
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

            /* Nous pouvons avoir des initialisations de type ici qui furent créés lors de la
             * création de fonctions d'initialisations d'autres types. */
            auto fonction_init = est_appel_fonction_initialisation(expr_appel->expression);
            if (fonction_init) {
                auto argument = expr_appel->parametres_resolus[0];
                genere_ri_pour_expression_droite(argument, nullptr);
                auto valeur = depile_valeur();
                auto type = fonction_init->type_initialisé();
                cree_appel_fonction_init_type(expr_appel, type, valeur);
                return;
            }

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

            auto valeur = cree_appel(expr_appel, atome_fonc, std::move(args));
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
                auto atome_fonc = m_compilatrice.trouve_ou_insere_fonction(
                    *this, decl_ref->comme_entete_fonction());
                // voir commentaire dans cree_appel
                atome_fonc->nombre_utilisations += 1;
                empile_valeur(atome_fonc);
                return;
            }

            if (decl_ref->possede_drapeau(EST_GLOBALE)) {
                empile_valeur(m_compilatrice.trouve_ou_insere_globale(decl_ref));
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

            auto alloc = cree_allocation(noeud, m_compilatrice.typeuse.type_chaine, nullptr);
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

                    auto params = kuri::tableau<Atome *, int>(2);
                    params[0] = acces_taille;
                    params[1] = valeur_;
                    cree_appel(noeud, fonction, std::move(params));

                    insere_label(label2);

                    condition = cree_op_comparaison(
                        noeud, OperateurBinaire::Genre::Comp_Sup_Egal, valeur_, acces_taille);
                    cree_branche_condition(noeud, condition, label3, label4);

                    insere_label(label3);

                    params = kuri::tableau<Atome *, int>(2);
                    params[0] = acces_taille;
                    params[1] = valeur_;
                    cree_appel(noeud, fonction, std::move(params));

                    insere_label(label4);
                };

            if (type_gauche->genre == GenreType::TABLEAU_FIXE) {
                if (noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto type_tableau_fixe = type_gauche->comme_tableau_fixe();
                    auto acces_taille = cree_z64(static_cast<unsigned>(type_tableau_fixe->taille));
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_compilatrice.trouve_ou_insere_fonction(
                            *this, m_compilatrice.interface_kuri->decl_panique_tableau));
                }
                empile_valeur(cree_acces_index(noeud, pointeur, valeur));
                return;
            }

            if (type_gauche->genre == GenreType::TABLEAU_DYNAMIQUE ||
                type_gauche->genre == GenreType::VARIADIQUE) {
                if (noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto acces_taille = cree_reference_membre_et_charge(noeud, pointeur, 1);
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_compilatrice.trouve_ou_insere_fonction(
                            *this, m_compilatrice.interface_kuri->decl_panique_tableau));
                }
                pointeur = cree_reference_membre(noeud, pointeur, 0);
                empile_valeur(cree_acces_index(noeud, pointeur, valeur));
                return;
            }

            if (type_gauche->genre == GenreType::CHAINE) {
                if (noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto acces_taille = cree_reference_membre_et_charge(noeud, pointeur, 1);
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_compilatrice.trouve_ou_insere_fonction(
                            *this, m_compilatrice.interface_kuri->decl_panique_chaine));
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

            auto bloc_final = NoeudBloc::nul();
            if (fonction_courante->decl) {
                bloc_final = fonction_courante->decl->bloc_constantes;
            }

            genere_ri_insts_differees(noeud->bloc_parent, bloc_final);
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
                insere_label_si_utilise(label_apres_instruction);
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
                di = fonction_courante->derniere_instruction();
                if (!di->est_branche_ou_retourne()) {
                    cree_branche(boucle, label_apres_boucle);
                }
            }

            if (boucle->bloc_sinon) {
                insere_label(label_pour_sinon);
                genere_ri_pour_noeud(boucle->bloc_sinon);
            }

            insere_label_si_utilise(label_apres_boucle);

            break;
        }
        case GenreNoeud::INSTRUCTION_ARRETE:
        {
            auto inst = noeud->comme_arrete();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlee);

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
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlee);
            auto label = boucle_controlee->comme_boucle()->label_pour_continue;
            genere_ri_insts_differees(inst->bloc_parent, boucle_controlee->bloc_parent);
            cree_branche(noeud, label);
            break;
        }
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            auto inst = noeud->comme_reprends();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlee);
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
                auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(noeud->type);
                auto alloc = cree_allocation(noeud, type_tableau_dyn, nullptr);
                auto init = genere_initialisation_defaut_pour_type(type_tableau_dyn);
                cree_stocke_mem(noeud, alloc, init);
                empile_valeur(alloc);
                return;
            }

            auto type_tableau_fixe = m_compilatrice.typeuse.type_tableau_fixe(noeud->type,
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
            auto valeur = cree_info_type(enfant->type, noeud);

            /* utilise une temporaire pour simplifier la compilation d'expressions du style :
             * info_de(z32).id */
            auto alloc = cree_allocation(noeud, valeur->type, nullptr);
            cree_stocke_mem(noeud, alloc, valeur);

            empile_valeur(alloc);
            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto type_fonction = noeud->type->comme_fonction();
            auto type_pointeur = type_fonction->types_entrees[0];
            auto type_arg = type_pointeur->comme_pointeur()->type_pointe;
            empile_valeur(
                m_compilatrice.trouve_ou_insere_fonction(*this, type_arg->fonction_init));
            break;
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr = noeud->comme_taille_de();
            auto expr_type = expr->expression;
            auto constante = cree_constante_taille_de(expr_type->type);
            empile_valeur(constante);
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

            // mémoire(*expr) = ...
            if (inst_mem->expression->genre_valeur == GenreValeur::DROITE &&
                !inst_mem->expression->est_comme()) {
                empile_valeur(valeur);
                return;
            }

            empile_valeur(cree_charge_mem(noeud, valeur));
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

    auto atome_fonc = m_compilatrice.trouve_ou_insere_fonction(*this, decl);

    if (decl->est_externe) {
        decl->drapeaux |= RI_FUT_GENEREE;
        atome_fonc->ri_generee = true;
        return;
    }

    fonction_courante = atome_fonc;

    cree_label(decl);

    genere_ri_pour_noeud(decl->corps->bloc);

    decl->drapeaux |= RI_FUT_GENEREE;
    decl->corps->drapeaux |= RI_FUT_GENEREE;
    fonction_courante->ri_generee = true;

    if (decl->possede_drapeau(DEBOGUE)) {
        imprime_fonction(atome_fonc, std::cerr);
    }

    fonction_courante = nullptr;
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
        case TypeTransformation::PREND_REFERENCE_ET_CONVERTIS_VERS_BASE:
        {
            assert_rappel(false, [&]() {
                std::cerr << "PREND_REFERENCE_ET_CONVERTIS_VERS_BASE utilisée dans la RI !\n";
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

            if (type_union->est_nonsure) {
                valeur = cree_charge_mem(noeud, valeur);

                /* Transtype l'union vers le type cible pour garantir une sûreté de type et éviter
                 * les problèmes de surécriture si la valeur est transtypée mais pas du même genre
                 * que le type le plus grand de l'union. */
                auto dest = cree_transtype(noeud,
                                           m_compilatrice.typeuse.type_pointeur_pour(
                                               const_cast<Type *>(valeur->type), false),
                                           alloc,
                                           TypeTranstypage::BITS);

                cree_stocke_mem(noeud, dest, valeur);
            }
            else {
                /* Pour les unions, nous transtypons le membre vers le type cible afin d'éviter les
                 * problème de surécriture de mémoire dans le cas où le type du membre est plus
                 * grand que le type de la valeur. */
                valeur = cree_charge_mem(noeud, valeur);

                auto acces_membre = cree_reference_membre(noeud, alloc, 0);
                auto membre_transtype = cree_transtype(
                    noeud,
                    m_compilatrice.typeuse.type_pointeur_pour(const_cast<Type *>(valeur->type),
                                                              false),
                    acces_membre,
                    TypeTranstypage::BITS);
                cree_stocke_mem(noeud, membre_transtype, valeur);

                acces_membre = cree_reference_membre(noeud, alloc, 1);
                auto index = cree_constante_entiere(
                    m_compilatrice.typeuse[TypeBase::Z32],
                    static_cast<uint64_t>(transformation.index_membre + 1));
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
                cree_appel(noeud,
                           m_compilatrice.trouve_ou_insere_fonction(
                               *this, m_compilatrice.interface_kuri->decl_panique_membre_union));
                insere_label(label_si_faux);

                valeur = cree_reference_membre(noeud, valeur, 0);
            }

            valeur = cree_transtype(noeud,
                                    m_compilatrice.typeuse.type_pointeur_pour(
                                        const_cast<Type *>(transformation.type_cible), false),
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
                valeur = cree_transtype(noeud,
                                        m_compilatrice.typeuse[TypeBase::PTR_RIEN],
                                        valeur,
                                        TypeTranstypage::BITS);
            }

            break;
        }
        case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
        {
            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            auto type_valeur = valeur->type;
            auto type_cible = transformation.type_cible;
            auto type_transtypage = TypeTranstypage::DEFAUT;

            // À FAIRE(transtypage) : tous les cas
            if (type_cible->est_entier_naturel()) {
                if (type_valeur->est_enum()) {
                    type_valeur = type_valeur->comme_enum()->type_donnees;

                    if (type_valeur->taille_octet < type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::AUGMENTE_RELATIF;
                    }
                    else if (type_valeur->taille_octet > type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::DIMINUE_RELATIF;
                    }
                }
                else if (type_valeur->est_erreur()) {
                    type_valeur = type_valeur->comme_erreur()->type_donnees;

                    if (type_valeur->taille_octet < type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::AUGMENTE_RELATIF;
                    }
                    else if (type_valeur->taille_octet > type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::DIMINUE_RELATIF;
                    }
                }
            }
            else if (type_cible->est_entier_relatif()) {
                if (type_valeur->est_enum()) {
                    type_valeur = type_valeur->comme_enum()->type_donnees;

                    if (type_valeur->taille_octet < type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::AUGMENTE_RELATIF;
                    }
                    else if (type_valeur->taille_octet > type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::DIMINUE_RELATIF;
                    }
                }
                else if (type_valeur->est_erreur()) {
                    type_valeur = type_valeur->comme_erreur()->type_donnees;

                    if (type_valeur->taille_octet < type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::AUGMENTE_RELATIF;
                    }
                    else if (type_valeur->taille_octet > type_cible->taille_octet) {
                        type_transtypage = TypeTranstypage::DIMINUE_RELATIF;
                    }
                }
            }

            valeur = cree_transtype(noeud, type_cible, valeur, type_transtypage);
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
                                            m_compilatrice.typeuse[TypeBase::N64],
                                            valeur,
                                            TypeTranstypage::AUGMENTE_NATUREL);
                }
                else {
                    valeur = cree_transtype(noeud,
                                            m_compilatrice.typeuse[TypeBase::Z64],
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
                auto type_eini = m_compilatrice.typeuse[TypeBase::EINI];
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
                noeud, m_compilatrice.typeuse[TypeBase::PTR_RIEN], valeur, TypeTranstypage::BITS);
            cree_stocke_mem(noeud, ptr_eini, transtype);

            /* copie le pointeur vers les infos du type du eini */
            auto tpe_eini = cree_reference_membre(noeud, alloc_eini, 1);
            auto info_type = cree_info_type_avec_transtype(noeud->type, noeud);
            cree_stocke_mem(noeud, tpe_eini, info_type);

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
            auto type_cible = m_compilatrice.typeuse.type_pointeur_pour(
                const_cast<Type *>(transformation.type_cible), false);
            valeur = cree_transtype(noeud, type_cible, valeur, TypeTranstypage::BITS);
            valeur = cree_charge_mem(noeud, valeur);
            break;
        }
        case TypeTransformation::CONSTRUIT_TABL_OCTET:
        {
            auto valeur_pointeur = static_cast<Atome *>(nullptr);
            auto valeur_taille = static_cast<Atome *>(nullptr);

            auto type_cible = m_compilatrice.typeuse[TypeBase::PTR_OCTET];

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
                                                    m_compilatrice.typeuse[TypeBase::Z64],
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
                noeud, m_compilatrice.typeuse[TypeBase::TABL_OCTET], nullptr);

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
            auto atome_fonction = m_compilatrice.trouve_ou_insere_fonction(
                *this, const_cast<NoeudDeclarationEnteteFonction *>(transformation.fonction));

            if (valeur->est_chargeable) {
                valeur = cree_charge_mem(noeud, valeur);
            }

            auto args = kuri::tableau<Atome *, int>();
            args.ajoute(valeur);

            valeur = cree_appel(noeud, atome_fonction, std::move(args));
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
        Atome *acces_erreur{};
        Atome *acces_erreur_pour_test{};

        Type const *type_piege = nullptr;
        Type const *type_variable = nullptr;
    };

    if (noeud->expression_appelee->type->genre == GenreType::ERREUR) {

        DonneesGenerationCodeTente gen_tente;
        gen_tente.type_piege = noeud->expression_appelee->type;
        gen_tente.acces_erreur = valeur_expression;
        gen_tente.acces_erreur_pour_test = gen_tente.acces_erreur;

        auto label_si_vrai = reserve_label(noeud);
        auto label_si_faux = reserve_label(noeud);

        auto condition = cree_op_comparaison(
            noeud,
            OperateurBinaire::Genre::Comp_Inegal,
            gen_tente.acces_erreur_pour_test,
            cree_constante_entiere(noeud->expression_appelee->type, 0));

        cree_branche_condition(noeud, condition, label_si_vrai, label_si_faux);

        insere_label(label_si_vrai);
        if (noeud->expression_piegee == nullptr) {
            cree_appel(noeud,
                       m_compilatrice.trouve_ou_insere_fonction(
                           *this, m_compilatrice.interface_kuri->decl_panique_erreur));
        }
        else {
            auto var_expr_piegee = cree_allocation(
                noeud, gen_tente.type_piege, noeud->expression_piegee->ident);
            auto decl_expr_piegee =
                noeud->expression_piegee->comme_reference_declaration()->declaration_referee;
            static_cast<NoeudDeclarationSymbole *>(decl_expr_piegee)->atome = var_expr_piegee;

            cree_stocke_mem(noeud->expression_piegee, var_expr_piegee, valeur_expression);

            genere_ri_pour_noeud(noeud->bloc);
        }

        insere_label(label_si_faux);

        empile_valeur(valeur_expression);
        return;
    }
    else if (noeud->expression_appelee->type->genre == GenreType::UNION) {
        DonneesGenerationCodeTente gen_tente;
        auto type_union = noeud->expression_appelee->type->comme_union();
        auto index_membre_erreur = 0;

        if (type_union->membres.taille() == 2) {
            if (type_union->membres[0].type->genre == GenreType::ERREUR) {
                gen_tente.type_piege = type_union->membres[0].type;
                gen_tente.type_variable = type_union->membres[1].type;
            }
            else {
                gen_tente.type_piege = type_union->membres[1].type;
                gen_tente.type_variable = type_union->membres[0].type;
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
            cree_appel(noeud,
                       m_compilatrice.trouve_ou_insere_fonction(
                           *this, m_compilatrice.interface_kuri->decl_panique_erreur));
        }
        else {
            Instruction *membre_erreur = cree_reference_membre(noeud, valeur_union, 0);
            membre_erreur = cree_transtype(noeud,
                                           m_compilatrice.typeuse.type_pointeur_pour(
                                               const_cast<Type *>(gen_tente.type_piege), false),
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
        valeur_expression = cree_transtype(noeud,
                                           m_compilatrice.typeuse.type_pointeur_pour(
                                               const_cast<Type *>(gen_tente.type_variable), false),
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
        ptr_union = cree_transtype(noeud,
                                   m_compilatrice.typeuse.type_pointeur_pour(type_membre, false),
                                   ptr_union,
                                   TypeTranstypage::BITS);
        ptr_union->est_chargeable = true;
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
        cree_appel(noeud,
                   m_compilatrice.trouve_ou_insere_fonction(
                       *this, m_compilatrice.interface_kuri->decl_panique_membre_union));
        insere_label(label_si_faux);
    }

    Instruction *pointeur_membre = cree_reference_membre(noeud, ptr_union, 0);

    if (type_membre != type_union->type_le_plus_grand) {
        pointeur_membre = cree_transtype(
            noeud,
            m_compilatrice.typeuse.type_pointeur_pour(type_membre, false),
            pointeur_membre,
            TypeTranstypage::BITS);
        pointeur_membre->est_chargeable = true;
    }

    empile_valeur(pointeur_membre);
}

AtomeConstante *ConstructriceRI::genere_initialisation_defaut_pour_type(Type const *type)
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
            return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::Z32], 0);
        }
        case GenreType::REEL:
        {
            if (type->taille_octet == 2) {
                return cree_constante_entiere(m_compilatrice.typeuse[TypeBase::N16], 0);
            }

            return cree_constante_reelle(type, 0.0);
        }
        case GenreType::TABLEAU_FIXE:
        {
            // À FAIRE(tableau fixe) : initialisation défaut
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
                genere_initialisation_defaut_pour_type(m_compilatrice.typeuse[TypeBase::Z32]));

            return cree_constante_structure(type, std::move(valeurs));
        }
        case GenreType::CHAINE:
        case GenreType::EINI:
        case GenreType::STRUCTURE:
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::VARIADIQUE:
        case GenreType::TUPLE:
        {
            auto type_compose = static_cast<TypeCompose const *>(type);
            auto valeurs = kuri::tableau<AtomeConstante *>();
            valeurs.reserve(type_compose->membres.taille());

            POUR (type_compose->membres) {
                if (it.ne_doit_pas_être_dans_code_machine()) {
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
            auto type_enum = static_cast<TypeEnum const *>(type);
            return cree_constante_entiere(type_enum, 0);
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_opaque();
            auto valeur = genere_initialisation_defaut_pour_type(type_opaque->type_opacifie);

            // À FAIRE(tableau fixe) : initialisation défaut
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
        genere_ri_pour_condition_implicite(condition, label_si_vrai, label_si_faux);
    }
}

void ConstructriceRI::genere_ri_pour_condition_implicite(NoeudExpression *condition,
                                                         InstructionLabel *label_si_vrai,
                                                         InstructionLabel *label_si_faux)
{
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
            assert_rappel(false, [&]() {
                std::cerr << "Type non géré pour la génération d'une condition d'une branche : "
                          << chaine_type(type_condition) << '\n';
            });
            break;
        }
    }

    cree_branche_condition(condition, valeur, label_si_vrai, label_si_faux);
}

void ConstructriceRI::genere_ri_pour_expression_logique(NoeudExpression *noeud, Atome *place)
{
    auto label_si_vrai = reserve_label(noeud);
    auto label_si_faux = reserve_label(noeud);
    auto label_apres_faux = reserve_label(noeud);

    if (place == nullptr) {
        place = cree_allocation(noeud, m_compilatrice.typeuse[TypeBase::BOOL], nullptr);
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

void ConstructriceRI::genere_ri_insts_differees(NoeudBloc *bloc, const NoeudBloc *bloc_final)
{
#if 0
	if (compilatrice.donnees_fonction->est_coroutine) {
		constructrice << "__etat->__termine_coro = 1;\n";
		constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
		constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
		constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	}
#endif

    /* À FAIRE : la hiérarchie de blocs des #corps_texte n'a pas le bloc de la fonction... */
    while (bloc && bloc != bloc_final) {
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

AtomeConstante *ConstructriceRI::cree_tableau_annotations_pour_info_membre(
    kuri::tableau<Annotation, int> const &annotations)
{
    kuri::tableau<AtomeConstante *> valeurs_annotations;
    valeurs_annotations.reserve(annotations.taille());

    auto type_annotation = m_compilatrice.typeuse.type_annotation;
    auto type_pointeur_annotation = m_compilatrice.typeuse.type_pointeur_pour(type_annotation);

    POUR (annotations) {
        kuri::tableau<AtomeConstante *> valeurs(2);
        valeurs[0] = cree_chaine(it.nom);
        valeurs[1] = cree_chaine(it.valeur);
        auto valeur = cree_globale_info_type(type_annotation, std::move(valeurs));
        valeurs_annotations.ajoute(valeur);
    }

    return cree_tableau_global(type_pointeur_annotation, std::move(valeurs_annotations));
}

AtomeConstante *ConstructriceRI::cree_info_type(Type const *type, NoeudExpression *site)
{
    if (type->atome_info_type != nullptr) {
        return type->atome_info_type;
    }

#if 0
    // assert((type->drapeaux & TYPE_FUT_VALIDE) != 0);
    if ((type->drapeaux & TYPE_FUT_VALIDE) == 0) {
        espace()
            ->rapporte_erreur(site, "Type non validé lors de la création d'un info type !\n")
            .ajoute_message("Note : le type est : ", chaine_type(type));
    }
#endif

    switch (type->genre) {
        case GenreType::POLYMORPHIQUE:
        case GenreType::TUPLE:
        {
            assert_rappel(false, []() { std::cerr << "Obtenu un type tuple ou polymophique\n"; });
            break;
        }
        case GenreType::BOOL:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::BOOLEEN, type);
            break;
        }
        case GenreType::OCTET:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::OCTET, type);
            break;
        }
        case GenreType::ENTIER_CONSTANT:
        {
            auto &typeuse = m_compilatrice.typeuse;
            auto type_z32 = typeuse[TypeBase::Z32];
            if (type_z32->atome_info_type) {
                type->atome_info_type = type_z32->atome_info_type;
            }
            else {
                type->atome_info_type = cree_info_type_entier(type_z32, true);
                type_z32->atome_info_type = type->atome_info_type;
            }
            break;
        }
        case GenreType::ENTIER_NATUREL:
        {
            type->atome_info_type = cree_info_type_entier(type, false);
            break;
        }
        case GenreType::ENTIER_RELATIF:
        {
            auto &typeuse = m_compilatrice.typeuse;
            auto type_z32 = typeuse[TypeBase::Z32];

            if (type != type_z32) {
                type->atome_info_type = cree_info_type_entier(type, true);
            }
            else {
                auto type_entier_constant = typeuse[TypeBase::ENTIER_CONSTANT];
                if (type_entier_constant->atome_info_type) {
                    type->atome_info_type = type_entier_constant->atome_info_type;
                }
                else {
                    type->atome_info_type = cree_info_type_entier(type, true);
                    type_entier_constant->atome_info_type = type->atome_info_type;
                }
            }
            break;
        }
        case GenreType::REEL:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::REEL, type);
            break;
        }
        case GenreType::REFERENCE:
        case GenreType::POINTEUR:
        {
            auto type_deref = type_dereference_pour(type);

            /* { membres basiques, type_pointé, est_référence } */
            auto valeurs = kuri::tableau<AtomeConstante *>(3);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::POINTEUR, type);
            valeurs[1] = cree_info_type_avec_transtype(type_deref, site);
            valeurs[2] = cree_constante_booleenne(type->genre == GenreType::REFERENCE);

            type->atome_info_type = cree_globale_info_type(
                m_compilatrice.typeuse.type_info_type_pointeur, std::move(valeurs));
            break;
        }
        case GenreType::ENUM:
        case GenreType::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);

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

            auto tableau_valeurs = cree_tableau_global(m_compilatrice.typeuse[TypeBase::Z32],
                                                       std::move(valeurs_enum));
            auto tableau_noms = cree_tableau_global(m_compilatrice.typeuse[TypeBase::CHAINE],
                                                    std::move(noms_enum));

            /* création de l'info type */

            /* { membres basiques, nom, valeurs, membres, est_drapeau } */
            auto valeurs = kuri::tableau<AtomeConstante *>(6);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::ENUM, type);
            valeurs[1] = cree_chaine(const_cast<TypeEnum *>(type_enum)->nom_hierarchique());
            valeurs[2] = tableau_valeurs;
            valeurs[3] = tableau_noms;
            valeurs[4] = cree_constante_booleenne(type_enum->est_drapeau);
            valeurs[5] = cree_info_type(type_enum->type_donnees, site);

            type->atome_info_type = cree_globale_info_type(
                m_compilatrice.typeuse.type_info_type_enum, std::move(valeurs));
            break;
        }
        case GenreType::UNION:
        {
            auto type_union = type->comme_union();

            // ------------------------------------
            // Commence par assigner une globale non-initialisée comme info type
            // pour éviter de recréer plusieurs fois le même info type.
            auto type_info_union = m_compilatrice.typeuse.type_info_type_union;

            auto globale = cree_globale(type_info_union, nullptr, false, true);
            type->atome_info_type = globale;

            // ------------------------------------
            /* pour chaque membre cree une instance de InfoTypeMembreStructure */
            auto type_struct_membre = m_compilatrice.typeuse.type_info_type_membre_structure;

            kuri::tableau<AtomeConstante *> valeurs_membres;
            valeurs_membres.reserve(type_union->membres.taille());

            POUR (type_union->membres) {
                /* { nom: chaine, info : *InfoType, décalage, drapeaux } */
                auto valeurs = kuri::tableau<AtomeConstante *>(5);
                valeurs[0] = cree_chaine(it.nom->nom);
                valeurs[1] = cree_info_type_avec_transtype(it.type, site);
                valeurs[2] = cree_z64(static_cast<uint64_t>(it.decalage));
                valeurs[3] = cree_z32(static_cast<unsigned>(it.drapeaux));

                if (it.decl) {
                    valeurs[4] = cree_tableau_annotations_pour_info_membre(it.decl->annotations);
                }
                else {
                    valeurs[4] = cree_tableau_annotations_pour_info_membre({});
                }

                /* Création d'un InfoType globale. */
                auto globale_membre = cree_globale_info_type(type_struct_membre,
                                                             std::move(valeurs));
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
            auto info_type_plus_grand = cree_info_type_avec_transtype(
                type_union->type_le_plus_grand, site);

            // Pour les références à des globales, nous devons avoir un type pointeur.
            auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(type_struct_membre,
                                                                         false);

            auto tableau_membre = cree_tableau_global(type_membre, std::move(valeurs_membres));

            auto valeurs = kuri::tableau<AtomeConstante *>(7);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::UNION, type);
            valeurs[1] = cree_chaine(const_cast<TypeUnion *>(type_union)->nom_hierarchique());
            valeurs[2] = tableau_membre;
            valeurs[3] = info_type_plus_grand;
            valeurs[4] = cree_z64(type_union->decalage_index);
            valeurs[5] = cree_constante_booleenne(!type_union->est_nonsure);
            if (type_union->decl) {
                valeurs[6] = cree_tableau_annotations_pour_info_membre(
                    type_union->decl->annotations);
            }
            else {
                valeurs[6] = cree_tableau_annotations_pour_info_membre({});
            }

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
            auto type_info_struct = m_compilatrice.typeuse.type_info_type_structure;

            auto globale = cree_globale(type_info_struct, nullptr, false, true);
            type->atome_info_type = globale;

            // ------------------------------------
            /* pour chaque membre cree une instance de InfoTypeMembreStructure */
            auto type_struct_membre = m_compilatrice.typeuse.type_info_type_membre_structure;

            kuri::tableau<AtomeConstante *> valeurs_membres;

            POUR (type_struct->membres) {
                if (it.nom == ID::chaine_vide) {
                    continue;
                }

                if (it.possède_drapeau(TypeCompose::Membre::PROVIENT_D_UN_EMPOI)) {
                    continue;
                }

                /* { nom: chaine, info : *InfoType, décalage, drapeaux } */
                auto valeurs = kuri::tableau<AtomeConstante *>(5);
                valeurs[0] = cree_chaine(it.nom->nom);
                valeurs[1] = cree_info_type_avec_transtype(it.type, site);
                valeurs[2] = cree_z64(static_cast<uint64_t>(it.decalage));
                valeurs[3] = cree_z32(static_cast<unsigned>(it.drapeaux));

                if (it.decl) {
                    valeurs[4] = cree_tableau_annotations_pour_info_membre(it.decl->annotations);
                }
                else {
                    valeurs[4] = cree_tableau_annotations_pour_info_membre({});
                }

                /* Création d'un InfoType globale. */
                auto globale_membre = cree_globale_info_type(type_struct_membre,
                                                             std::move(valeurs));
                valeurs_membres.ajoute(globale_membre);
            }

            // Pour les références à des globales, nous devons avoir un type pointeur.
            auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(type_struct_membre,
                                                                         false);

            auto tableau_membre = cree_tableau_global(type_membre, std::move(valeurs_membres));

            kuri::tableau<AtomeConstante *> valeurs_structs_employees;
            valeurs_structs_employees.reserve(type_struct->types_employes.taille());
            POUR (type_struct->types_employes) {
                valeurs_structs_employees.ajoute(cree_info_type(it, site));
            }

            auto type_pointeur_info_struct = m_compilatrice.typeuse.type_pointeur_pour(
                type_info_struct, false);
            auto tableau_structs_employees = cree_tableau_global(
                type_pointeur_info_struct, std::move(valeurs_structs_employees));

            /* { membres basiques, nom, membres } */
            auto valeurs = kuri::tableau<AtomeConstante *>(5);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::STRUCTURE, type);
            valeurs[1] = cree_chaine(const_cast<TypeStructure *>(type_struct)->nom_hierarchique());
            valeurs[2] = tableau_membre;
            valeurs[3] = tableau_structs_employees;
            if (type_struct->decl) {
                valeurs[4] = cree_tableau_annotations_pour_info_membre(
                    type_struct->decl->annotations);
            }
            else {
                valeurs[4] = cree_tableau_annotations_pour_info_membre({});
            }

            globale->initialisateur = cree_constante_structure(type_info_struct,
                                                               std::move(valeurs));
            break;
        }
        case GenreType::TABLEAU_DYNAMIQUE:
        case GenreType::VARIADIQUE:
        {
            auto type_deref = type_dereference_pour(type);

            /* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */
            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::TABLEAU, type);
            valeurs[1] = cree_info_type_avec_transtype(type_deref, site);
            valeurs[2] = cree_constante_booleenne(false);
            valeurs[3] = cree_z32(0);

            type->atome_info_type = cree_globale_info_type(
                m_compilatrice.typeuse.type_info_type_tableau, std::move(valeurs));
            break;
        }
        case GenreType::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_tableau_fixe();

            /* { membres basiques, type_pointé, est_tableau_fixe, taille_fixe } */
            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::TABLEAU, type);
            valeurs[1] = cree_info_type_avec_transtype(type_tableau->type_pointe, site);
            valeurs[2] = cree_constante_booleenne(true);
            valeurs[3] = cree_z32(static_cast<unsigned>(type_tableau->taille));

            type->atome_info_type = cree_globale_info_type(
                m_compilatrice.typeuse.type_info_type_tableau, std::move(valeurs));
            break;
        }
        case GenreType::FONCTION:
        {
            auto type_fonction = type->comme_fonction();

            kuri::tableau<AtomeConstante *> types_entree;
            types_entree.reserve(type_fonction->types_entrees.taille());
            POUR (type_fonction->types_entrees) {
                types_entree.ajoute(cree_info_type_avec_transtype(it, site));
            }

            kuri::tableau<AtomeConstante *> types_sortie;
            auto type_sortie = type_fonction->type_sortie;
            if (type_sortie->est_tuple()) {
                auto tuple = type_sortie->comme_tuple();

                types_sortie.reserve(tuple->membres.taille());
                POUR (tuple->membres) {
                    types_sortie.ajoute(cree_info_type_avec_transtype(it.type, site));
                }
            }
            else {
                types_sortie.reserve(1);
                types_sortie.ajoute(
                    cree_info_type_avec_transtype(type_fonction->type_sortie, site));
            }

            auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(
                m_compilatrice.typeuse.type_info_type_, false);
            auto tableau_types_entree = cree_tableau_global(type_membre, std::move(types_entree));
            auto tableau_types_sortie = cree_tableau_global(type_membre, std::move(types_sortie));

            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::FONCTION, type);
            valeurs[1] = tableau_types_entree;
            valeurs[2] = tableau_types_sortie;
            valeurs[3] = cree_constante_booleenne(false);

            type->atome_info_type = cree_globale_info_type(
                m_compilatrice.typeuse.type_info_type_fonction, std::move(valeurs));
            break;
        }
        case GenreType::EINI:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::EINI, type);
            break;
        }
        case GenreType::RIEN:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::RIEN, type);
            break;
        }
        case GenreType::CHAINE:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::CHAINE, type);
            break;
        }
        case GenreType::TYPE_DE_DONNEES:
        {
            type->atome_info_type = cree_info_type_defaut(IDInfoType::TYPE_DE_DONNEES, type);
            break;
        }
        case GenreType::OPAQUE:
        {
            auto type_opaque = type->comme_opaque();
            auto type_opacifie = type_opaque->type_opacifie;

            /* { membres basiques, nom, type_opacifié } */
            auto valeurs = kuri::tableau<AtomeConstante *>(3);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::OPAQUE, type_opaque);
            valeurs[1] = cree_chaine(const_cast<TypeOpaque *>(type_opaque)->nom_hierarchique());
            valeurs[2] = cree_info_type_avec_transtype(type_opacifie, site);

            type->atome_info_type = cree_globale_info_type(
                m_compilatrice.typeuse.type_info_type_opaque, std::move(valeurs));
            break;
        }
    }

    // À FAIRE : il nous faut toutes les informations du type pour pouvoir générer les informations
    assert_rappel((type->drapeaux & TYPE_FUT_VALIDE) != 0, [type]() {
        std::cerr << "Info type pour " << chaine_type(type) << " est incomplet\n";
    });

    static_cast<AtomeGlobale *>(type->atome_info_type)->est_info_type_de = type;

    return type->atome_info_type;
}

AtomeConstante *ConstructriceRI::transtype_base_info_type(AtomeConstante *info_type)
{
    auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
        m_compilatrice.typeuse.type_info_type_, false);

    if (info_type->type == type_pointeur_info_type) {
        return info_type;
    }

    return cree_transtype_constant(type_pointeur_info_type, info_type);
}

void ConstructriceRI::genere_ri_pour_initialisation_globales(
    EspaceDeTravail *espace,
    AtomeFonction *fonction_init,
    const kuri::tableau<AtomeGlobale *> &globales)
{
    m_espace = espace;
    genere_ri_pour_initialisation_globales(fonction_init, globales);
}

AtomeConstante *ConstructriceRI::crée_constante_info_type_pour_base(uint32_t index,
                                                                    Type const *pour_type)
{
    auto membres = kuri::tableau<AtomeConstante *>(3);
    remplis_membres_de_bases_info_type(membres, index, pour_type);
    return cree_constante_structure(m_compilatrice.typeuse.type_info_type_, std::move(membres));
}

void ConstructriceRI::remplis_membres_de_bases_info_type(kuri::tableau<AtomeConstante *> &valeurs,
                                                         uint32_t index,
                                                         Type const *pour_type)
{
    assert(valeurs.taille() == 3);
    valeurs[0] = cree_z32(index);
    /* Puisque nous pouvons générer du code pour des architectures avec adressages en 32 ou 64
     * bits, et puisque nous pouvons exécuter sur une machine avec une architecture différente de
     * la cible de compilation, nous générons les constantes pour les taille_de lors de l'émission
     * du code machine. */
    valeurs[1] = cree_constante_taille_de(pour_type);
    // L'index dans la table des types sera mis en place lors de la génération du code machine.
    valeurs[2] = cree_z32(0);
}

AtomeConstante *ConstructriceRI::cree_info_type_defaut(unsigned index, Type const *pour_type)
{
    auto valeurs = kuri::tableau<AtomeConstante *>(3);
    remplis_membres_de_bases_info_type(valeurs, index, pour_type);
    return cree_globale_info_type(m_compilatrice.typeuse.type_info_type_, std::move(valeurs));
}

AtomeConstante *ConstructriceRI::cree_info_type_entier(Type const *pour_type, bool est_relatif)
{
    auto valeurs = kuri::tableau<AtomeConstante *>(2);
    valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::ENTIER, pour_type);
    valeurs[1] = cree_constante_booleenne(est_relatif);
    return cree_globale_info_type(m_compilatrice.typeuse.type_info_type_entier,
                                  std::move(valeurs));
}

AtomeConstante *ConstructriceRI::cree_info_type_avec_transtype(Type const *type,
                                                               NoeudExpression *site)
{
    auto info_type = cree_info_type(type, site);
    return transtype_base_info_type(info_type);
}

AtomeConstante *ConstructriceRI::cree_globale_info_type(Type const *type_info_type,
                                                        kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto initialisateur = cree_constante_structure(type_info_type, std::move(valeurs));
    return cree_globale(type_info_type, initialisateur, false, true);
}

Atome *ConstructriceRI::converti_vers_tableau_dyn(NoeudExpression *noeud,
                                                  Atome *pointeur_tableau_fixe,
                                                  TypeTableauFixe *type_tableau_fixe,
                                                  Atome *place)
{
    auto alloc_tableau_dyn = place;

    if (alloc_tableau_dyn == nullptr) {
        auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(
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
    auto table_chaines = m_compilatrice.table_chaines.verrou_ecriture();
    auto trouve = false;
    auto valeur = table_chaines->trouve(chaine, trouve);

    if (trouve) {
        return valeur;
    }

    auto type_chaine = m_compilatrice.typeuse.type_chaine;

    AtomeConstante *constante_chaine;

    if (chaine.taille() == 0) {
        constante_chaine = genere_initialisation_defaut_pour_type(type_chaine);
    }
    else {
        auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(
            m_compilatrice.typeuse[TypeBase::Z8], static_cast<int>(chaine.taille()));
        auto tableau = cree_constante_tableau_donnees_constantes(
            type_tableau, const_cast<char *>(chaine.pointeur()), chaine.taille());

        auto globale_tableau = cree_globale(type_tableau, tableau, false, true);
        auto pointeur_chaine = cree_acces_index_constant(globale_tableau, cree_z64(0));
        auto taille_chaine = cree_z64(static_cast<uint64_t>(chaine.taille()));

        auto membres = kuri::tableau<AtomeConstante *>(2);
        membres[0] = pointeur_chaine;
        membres[1] = taille_chaine;

        constante_chaine = cree_constante_structure(type_chaine, std::move(membres));
    }

    table_chaines->insere(chaine, constante_chaine);

    return constante_chaine;
}

void ConstructriceRI::genere_ri_pour_initialisation_globales(
    AtomeFonction *fonction_init, kuri::tableau<AtomeGlobale *> const &globales)
{
    nombre_labels = 0;
    fonction_courante = fonction_init;

    /* Sauvegarde le retour. */
    auto di = fonction_init->instructions.derniere();
    fonction_init->instructions.supprime_dernier();

    auto constructeurs_globaux = m_compilatrice.constructeurs_globaux.verrou_lecture();

    POUR (*constructeurs_globaux) {
        bool globale_trouvee = false;
        for (auto &globale : globales) {
            if (it.atome == globale) {
                globale_trouvee = true;
                break;
            }
        }

        if (!globale_trouvee) {
            continue;
        }

        if (it.expression->est_non_initialisation()) {
            continue;
        }

        genere_ri_transformee_pour_noeud(it.expression, it.atome, it.transformation);
    }

    /* Restaure le retour. */
    fonction_init->instructions.ajoute(di);

    fonction_courante = nullptr;
}

void ConstructriceRI::genere_ri_pour_fonction_metaprogramme(
    NoeudDeclarationEnteteFonction *fonction)
{
    assert(fonction->est_metaprogramme);
    auto atome_fonc = m_compilatrice.trouve_ou_insere_fonction(*this, fonction);

    fonction_courante = atome_fonc;
    taille_allouee = 0;

    auto decl_creation_contexte = m_compilatrice.interface_kuri->decl_creation_contexte;

    auto atome_creation_contexte = m_compilatrice.trouve_ou_insere_fonction(
        *this, decl_creation_contexte);

    atome_fonc->instructions.reserve(atome_creation_contexte->instructions.taille());

    for (auto i = 0; i < atome_creation_contexte->instructions.taille(); ++i) {
        auto it = atome_creation_contexte->instructions[i];
        atome_fonc->instructions.ajoute(it);
    }

    atome_fonc->decalage_appel_init_globale = atome_fonc->instructions.taille();

    genere_ri_pour_noeud(fonction->corps->bloc);

    fonction->drapeaux |= RI_FUT_GENEREE;

    fonction_courante = nullptr;
}

enum class TypeConstructionGlobale {
    /* L'expression est un tableau fixe que nous pouvons simplement construire. */
    TABLEAU_CONSTANT,
    /* L'expression est un tableau fixe que nous devons convertir vers un tableau
     * dynamique. */
    TABLEAU_FIXE_A_CONVERTIR,
    /* L'expression peut-être construite via un simple constructeur. */
    NORMALE,
    /* L'expression est nulle, la valeur défaut du type devra être utilisée. */
    PAR_VALEUR_DEFAUT,
    /* L'expression est une expression de non-initialisation. */
    SANS_INITIALISATION,
};

static TypeConstructionGlobale type_construction_globale(NoeudExpression const *expression,
                                                         TransformationType const &transformation)
{
    if (!expression) {
        return TypeConstructionGlobale::PAR_VALEUR_DEFAUT;
    }

    if (expression->est_non_initialisation()) {
        return TypeConstructionGlobale::SANS_INITIALISATION;
    }

    if (expression->est_construction_tableau()) {
        if (transformation.type != TypeTransformation::INUTILE) {
            return TypeConstructionGlobale::TABLEAU_FIXE_A_CONVERTIR;
        }

        auto const type_pointe = type_dereference_pour(expression->type);

        /* À FAIRE : permet la génération de code pour les tableaux globaux de structures dans le
         * contexte global. Ceci nécessitera d'avoir une deuxième version de la génération de code
         * pour les structures avec des instructions constantes. */
        if (type_pointe->est_structure() || type_pointe->est_union()) {
            return TypeConstructionGlobale::NORMALE;
        }

        return TypeConstructionGlobale::TABLEAU_CONSTANT;
    }

    return TypeConstructionGlobale::NORMALE;
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
                auto atome = m_compilatrice.trouve_ou_insere_globale(decl);
                atome->est_externe = est_externe;
                atome->est_constante = false;
                atome->ident = var->ident;

                auto expression = it.expression;
                if (expression && expression->substitution) {
                    expression = expression->substitution;
                }

                auto const type_construction = type_construction_globale(expression,
                                                                         it.transformations[i]);

                switch (type_construction) {
                    case TypeConstructionGlobale::TABLEAU_CONSTANT:
                    {
                        genere_ri_pour_noeud(expression);
                        valeur = static_cast<AtomeConstante *>(depile_valeur());
                        break;
                    }
                    case TypeConstructionGlobale::TABLEAU_FIXE_A_CONVERTIR:
                    {
                        auto type_tableau_fixe = expression->type->comme_tableau_fixe();

                        /* Crée une globale pour le tableau fixe, et utilise celle-ci afin
                         * d'initialiser le tableau dynamique. */
                        auto globale_tableau = cree_globale(
                            expression->type, nullptr, false, false);

                        /* La construction du tableau deva se faire via la fonction
                         * d'initialisation des globales. */
                        m_compilatrice.constructeurs_globaux->ajoute(
                            {globale_tableau, expression, {}});

                        valeur = cree_initialisation_tableau_global(globale_tableau,
                                                                    type_tableau_fixe);
                        break;
                    }
                    case TypeConstructionGlobale::NORMALE:
                    {
                        m_compilatrice.constructeurs_globaux->ajoute(
                            {atome, expression, it.transformations[i]});
                        break;
                    }
                    case TypeConstructionGlobale::PAR_VALEUR_DEFAUT:
                    {
                        valeur = genere_initialisation_defaut_pour_type(var->type);
                        break;
                    }
                    case TypeConstructionGlobale::SANS_INITIALISATION:
                    {
                        /* Rien à faire. */
                        break;
                    }
                }

                atome->ri_generee = true;
                atome->initialisateur = valeur;
            }
        }

        decl->atome->ri_generee = true;
        decl->drapeaux |= RI_FUT_GENEREE;
        return;
    }

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
                        auto pointeur = cree_allocation(var, var->type, var->ident);
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
                        auto pointeur = cree_allocation(var, var->type, var->ident);
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
                    auto pointeur = cree_allocation(var, var->type, var->ident);
                    static_cast<NoeudDeclarationSymbole *>(
                        var->comme_reference_declaration()->declaration_referee)
                        ->atome = pointeur;
                }
            }
        }
        else {
            for (auto &var : it.variables.plage()) {
                auto pointeur = cree_allocation(var, var->type, var->ident);
                auto type_var = var->type;
                cree_appel_fonction_init_type(var, type_var, pointeur);

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
