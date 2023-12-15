/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "constructrice_ri.hh"

#include <iostream>

#include "biblinternes/chrono/chronometrage.hh"
#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/sauvegardeuse_etat.hh"

#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/portee.hh"
#include "utilitaires/log.hh"

#include "parsage/outils_lexemes.hh"

#include "statistiques/statistiques.hh"

#include "structures/enchaineuse.hh"
#include "structures/rassembleuse.hh"

#include "analyse.hh"
#include "impression.hh"
#include "optimisations.hh"

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
        if (entete->possède_drapeau(DrapeauxNoeudFonction::EST_INITIALISATION_TYPE)) {
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

static bool est_référence_compatible_pointeur(Type const *type_dest, Type const *type_source)
{
    if (!type_source->est_type_reference()) {
        return false;
    }

    if (type_source->comme_type_reference()->type_pointe !=
        type_dest->comme_type_pointeur()->type_pointe) {
        return false;
    }

    return true;
}

static bool type_dest_et_type_source_sont_compatibles(Type const *type_dest,
                                                      Type const *type_source)
{
    auto type_élément_dest = type_dereference_pour(type_dest);
    if (type_élément_dest == type_source) {
        return true;
    }

    /* Nous avons différents types de données selon le type connu lors de la compilation. */
    if (type_élément_dest->est_type_type_de_donnees() && type_source->est_type_type_de_donnees()) {
        return true;
    }

    if (est_type_opacifié(type_élément_dest, type_source)) {
        return true;
    }

    /* À FAIRE : supprime les entiers constants. */
    if (type_source->est_type_entier_constant() && est_type_entier(type_élément_dest)) {
        return true;
    }

    /* Certaines références sont converties en pointeur, nous devons vérifier ce cas. Les erreurs
     * de sémantiques devraient déjà avoir été attrappées lors de la validation sémantique.
     * À FAIRE : supprimer les références de la RI, ou les garder totalement. */
    if (est_référence_compatible_pointeur(type_élément_dest, type_source)) {
        return true;
    }

    /* Comme pour au-dessus, dans certains cas une fonction espère une référence mais la valeur est
     * un pointeur. */
    if (type_source->est_type_pointeur()) {
        if (est_référence_compatible_pointeur(type_source, type_élément_dest)) {
            return true;
        }
    }

    return false;
}

#ifndef NDEBUG
static bool est_type_sous_jacent_énum(Type const *gauche, Type const *droite)
{
    if (!gauche->est_type_enum()) {
        return false;
    }

    auto type_énum = gauche->comme_type_enum();
    return type_énum->type_sous_jacent == droite;
}

static bool sont_types_compatibles_pour_opérateur_binaire(Type const *gauche, Type const *droite)
{
    if (gauche == droite) {
        return true;
    }
    if (droite->est_type_type_de_donnees() && gauche->est_type_type_de_donnees()) {
        return true;
    }
    if (est_type_entier(droite) && gauche->est_type_entier_constant()) {
        return true;
    }
    if (est_type_entier(gauche) && droite->est_type_entier_constant()) {
        return true;
    }
    if (est_type_opacifié(gauche, droite)) {
        return true;
    }
    /* Par exemple des boucles-pour sur énum.nombre_éléments. */
    if (est_type_sous_jacent_énum(gauche, droite) || est_type_sous_jacent_énum(droite, gauche)) {
        return true;
    }
    return false;
}

static bool sont_types_compatibles_pour_param_appel(Type const *paramètre, Type const *expression)
{
    if (paramètre == expression) {
        return true;
    }
    if (paramètre->est_type_variadique()) {
        auto type_variadique = paramètre->comme_type_variadique();
        if (!type_variadique->type_pointe) {
            return true;
        }
        return expression == type_variadique->type_tableau_dynamique;
    }
    if (expression->est_type_type_de_donnees() && paramètre->est_type_type_de_donnees()) {
        return true;
    }
    if (est_type_entier(expression) && paramètre->est_type_entier_constant()) {
        return true;
    }
    if (est_type_entier(paramètre) && expression->est_type_entier_constant()) {
        return true;
    }
    if (est_référence_compatible_pointeur(paramètre, expression)) {
        return true;
    }
    return false;
}
#endif

/* ------------------------------------------------------------------------- */
/** \name RegistreSymboliqueRI
 * \{ */

RegistreSymboliqueRI::RegistreSymboliqueRI(Typeuse &typeuse)
    : broyeuse(memoire::loge<Broyeuse>("Broyeuse")), m_typeuse(typeuse),
      m_constructrice(memoire::loge<ConstructriceRI>("ConstructriceRI", m_typeuse, *this))
{
}

RegistreSymboliqueRI::~RegistreSymboliqueRI()
{
    memoire::deloge("Broyeuse", broyeuse);
    memoire::deloge("ConstructriceRI", m_constructrice);
}

AtomeFonction *RegistreSymboliqueRI::crée_fonction(kuri::chaine_statique nom_fonction)
{
    std::unique_lock lock(mutex_atomes_fonctions);
    return fonctions.ajoute_element(nullptr, nom_fonction);
}

AtomeFonction *RegistreSymboliqueRI::trouve_ou_insère_fonction(
    NoeudDeclarationEnteteFonction *decl)
{
    std::unique_lock lock(mutex_atomes_fonctions);

    if (decl->atome) {
        return decl->atome->comme_fonction();
    }

    auto params = kuri::tableau<InstructionAllocation *, int>();
    params.redimensionne(decl->params.taille());

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = decl->parametre_entree(i)->comme_declaration_variable();
        auto atome = m_constructrice->crée_allocation(param, param->type, param->ident);
        param->atome = atome;
        params[i] = atome;
    }

    /* Pour les sorties multiples, les valeurs de sorties sont des accès de
     * membres du tuple, ainsi nous n'avons pas à compliquer la génération de
     * code ou sa simplification.
     */

    auto param_sortie = decl->param_sortie;
    auto atome_param_sortie = m_constructrice->crée_allocation(
        param_sortie, param_sortie->type, param_sortie->ident);
    param_sortie->atome = atome_param_sortie;

    if (decl->params_sorties.taille() > 1) {
        POUR_INDEX (decl->params_sorties) {
            it->comme_declaration_variable()->atome = m_constructrice->crée_référence_membre(
                it, atome_param_sortie, index_it, true);
        }
    }

    auto atome_fonc = fonctions.ajoute_element(
        decl, decl->donne_nom_broyé(*broyeuse), std::move(params), atome_param_sortie);
    atome_fonc->type = decl->type;
    atome_fonc->est_externe = decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE);
    atome_fonc->sanstrace = decl->possède_drapeau(DrapeauxNoeudFonction::FORCE_SANSTRACE);
    atome_fonc->enligne = decl->possède_drapeau(DrapeauxNoeudFonction::FORCE_ENLIGNE);

    decl->atome = atome_fonc;

    return atome_fonc;
}

AtomeGlobale *RegistreSymboliqueRI::crée_globale(IdentifiantCode &ident,
                                                 Type const *type,
                                                 AtomeConstante *initialisateur,
                                                 bool est_externe,
                                                 bool est_constante)
{
    return globales.ajoute_element(&ident,
                                   m_typeuse.type_pointeur_pour(const_cast<Type *>(type), false),
                                   initialisateur,
                                   est_externe,
                                   est_constante);
}

AtomeGlobale *RegistreSymboliqueRI::trouve_globale(NoeudDeclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);
    auto decl_var = decl->comme_declaration_variable();
    return decl_var->atome->comme_globale();
}

AtomeGlobale *RegistreSymboliqueRI::trouve_ou_insère_globale(NoeudDeclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);

    auto decl_var = decl->comme_declaration_variable();

    if (decl_var->atome == nullptr) {
        auto est_externe = decl->possède_drapeau(DrapeauxNoeud::EST_EXTERNE);
        auto globale = crée_globale(*decl_var->ident, decl->type, nullptr, est_externe, false);
        globale->decl = decl_var;
        decl_var->atome = globale;
    }

    return decl_var->atome->comme_globale();
}

void RegistreSymboliqueRI::rassemble_statistiques(Statistiques &stats) const
{
    auto &stats_ri = stats.stats_ri;

    auto mémoire_fonctions = fonctions.memoire_utilisee();
    mémoire_fonctions += fonctions.memoire_utilisee();
    pour_chaque_element(fonctions, [&](AtomeFonction const &it) {
        mémoire_fonctions += it.params_entrees.taille_memoire();
        mémoire_fonctions += it.instructions.taille_memoire();

        if (it.données_exécution) {
            stats.mémoire_code_binaire += it.données_exécution->mémoire_utilisée();
            stats.mémoire_code_binaire += taille_de(DonnéesExécutionFonction);
        }
    });

    stats_ri.fusionne_entrée({"fonctions", fonctions.taille(), mémoire_fonctions});
    stats_ri.fusionne_entrée({"globales", globales.taille(), globales.memoire_utilisee()});

    m_constructrice->rassemble_statistiques(stats);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ConstructriceRI
 * \{ */

void ConstructriceRI::définis_fonction_courante(AtomeFonction *fonction_courante)
{
    m_nombre_labels = 0;
    m_fonction_courante = fonction_courante;
}

AtomeFonction *ConstructriceRI::crée_fonction(kuri::chaine_statique nom_fonction)
{
    return m_registre.crée_fonction(nom_fonction);
}

AtomeFonction *ConstructriceRI::trouve_ou_insère_fonction(NoeudDeclarationEnteteFonction *decl)
{
    return m_registre.trouve_ou_insère_fonction(decl);
}

AtomeGlobale *ConstructriceRI::crée_globale(IdentifiantCode &ident,
                                            Type const *type,
                                            AtomeConstante *initialisateur,
                                            bool est_externe,
                                            bool est_constante)
{
    return m_registre.crée_globale(ident, type, initialisateur, est_externe, est_constante);
}

AtomeGlobale *ConstructriceRI::trouve_globale(NoeudDeclaration *decl)
{
    return m_registre.trouve_globale(decl);
}

AtomeGlobale *ConstructriceRI::trouve_ou_insère_globale(NoeudDeclaration *decl)
{
    return m_registre.trouve_ou_insère_globale(decl);
}

InstructionAllocation *ConstructriceRI::crée_allocation(NoeudExpression const *site_,
                                                        Type const *type,
                                                        IdentifiantCode *ident,
                                                        bool crée_seulement)
{
    /* le résultat d'une instruction d'allocation est l'adresse de la variable. */
    auto type_pointeur = m_typeuse.type_pointeur_pour(const_cast<Type *>(type), false);
    auto inst = insts_allocation.ajoute_element(site_, type_pointeur, ident);

    /* Nous utilisons pour l'instant crée_allocation pour les paramètres des
     * fonctions, et la fonction_courante est nulle lors de cette opération.
     */
    if (m_fonction_courante && !crée_seulement) {
        m_fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

/* Nous ne pouvons dédupliquer les constantes car la mise à jour des index dans la table de types
 * les modifie... */
#undef DEDUPLIQUE_CONSTANTE

AtomeConstanteEntière *ConstructriceRI::crée_constante_nombre_entier(Type const *type,
                                                                     uint64_t valeur)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (constantes_entières) {
        if (it.type == type && it.valeur == valeur) {
            return &it;
        }
    }
#endif
    return constantes_entières.ajoute_element(type, valeur);
}

AtomeConstanteType *ConstructriceRI::crée_constante_type(Type const *pointeur_type)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (constantes_types) {
        if (it.type_de_données == pointeur_type) {
            return &it;
        }
    }
#endif
    return constantes_types.ajoute_element(m_typeuse.type_type_de_donnees_, pointeur_type);
}

AtomeConstanteTailleDe *ConstructriceRI::crée_constante_taille_de(Type const *pointeur_type)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (constantes_taille_de) {
        if (it.type_de_données == pointeur_type) {
            return &it;
        }
    }
#endif
    return constantes_taille_de.ajoute_element(TypeBase::N32, pointeur_type);
}

AtomeIndexTableType *ConstructriceRI::crée_index_table_type(const Type *pointeur_type)
{
    return constantes_index_table_type.ajoute_element(TypeBase::N32, pointeur_type);
}

AtomeConstante *ConstructriceRI::crée_z32(uint64_t valeur)
{
    return crée_constante_nombre_entier(TypeBase::Z32, valeur);
}

AtomeConstante *ConstructriceRI::crée_z64(uint64_t valeur)
{
    return crée_constante_nombre_entier(TypeBase::Z64, valeur);
}

AtomeConstanteRéelle *ConstructriceRI::crée_constante_nombre_réel(Type const *type, double valeur)
{
    return constantes_réelles.ajoute_element(type, valeur);
}

AtomeConstanteStructure *ConstructriceRI::crée_constante_structure(
    Type const *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    return constantes_structures.ajoute_element(type, std::move(valeurs));
}

AtomeConstanteTableauFixe *ConstructriceRI::crée_constante_tableau_fixe(
    Type const *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    return constantes_tableaux.ajoute_element(type, std::move(valeurs));
}

AtomeConstanteDonnéesConstantes *ConstructriceRI::crée_constante_tableau_données_constantes(
    Type const *type, kuri::tableau<char> &&données_constantes)
{
    return constantes_données_constantes.ajoute_element(type, std::move(données_constantes));
}

AtomeConstanteDonnéesConstantes *ConstructriceRI::crée_constante_tableau_données_constantes(
    Type const *type, char *pointeur, int64_t taille)
{
    return constantes_données_constantes.ajoute_element(type, pointeur, taille);
}

AtomeInitialisationTableau *ConstructriceRI::crée_initialisation_tableau(
    const Type *type, const AtomeConstante *valeur)
{
    return initialisations_tableau.ajoute_element(type, valeur);
}

AtomeNonInitialisation *ConstructriceRI::crée_non_initialisation()
{
    return non_initialisations.ajoute_element();
}

AtomeConstante *ConstructriceRI::crée_tableau_global(IdentifiantCode &ident,
                                                     Type const *type,
                                                     kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto taille_tableau = static_cast<int>(valeurs.taille());

    if (taille_tableau == 0) {
        auto type_tableau_dyn = m_typeuse.type_tableau_dynamique(const_cast<Type *>(type));
        return crée_initialisation_défaut_pour_type(type_tableau_dyn);
    }

    auto type_tableau = m_typeuse.type_tableau_fixe(const_cast<Type *>(type), taille_tableau);
    auto tableau_fixe = crée_constante_tableau_fixe(type_tableau, std::move(valeurs));

    return crée_tableau_global(ident, tableau_fixe);
}

AtomeConstante *ConstructriceRI::crée_tableau_global(IdentifiantCode &ident,
                                                     AtomeConstante *tableau_fixe)
{
    auto type_tableau_fixe = tableau_fixe->type->comme_type_tableau_fixe();
    auto globale_tableau_fixe = crée_globale(ident, type_tableau_fixe, tableau_fixe, false, true);
    return crée_initialisation_tableau_global(globale_tableau_fixe, type_tableau_fixe);
}

AtomeConstante *ConstructriceRI::crée_initialisation_tableau_global(
    AtomeGlobale *globale_tableau_fixe, TypeTableauFixe const *type_tableau_fixe)
{
    AtomeConstante *ptr_premier_élément = crée_accès_index_constant(globale_tableau_fixe, 0);
    auto valeur_taille = crée_z64(static_cast<unsigned>(type_tableau_fixe->taille));
    auto type_tableau_dyn = m_typeuse.type_tableau_dynamique(type_tableau_fixe->type_pointe);

    if (est_globale_pour_tableau_données_constantes(globale_tableau_fixe)) {
        if (type_tableau_fixe->type_pointe != TypeBase::Z8) {
            /* Nous devons transtypé vers le type pointeur idoine. */
            auto type_cible = m_typeuse.type_pointeur_pour(type_tableau_fixe->type_pointe);
            ptr_premier_élément = crée_transtype_constant(type_cible, ptr_premier_élément);
        }
    }

    auto membres = kuri::tableau<AtomeConstante *>(3);
    membres[0] = ptr_premier_élément;
    membres[1] = valeur_taille;
    membres[2] = valeur_taille;

    return crée_constante_structure(type_tableau_dyn, std::move(membres));
}

AtomeConstanteBooléenne *ConstructriceRI::crée_constante_booléenne(bool valeur)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (constantes_booléennes) {
        if (it.valeur == valeur) {
            return &it;
        }
    }
#endif
    return constantes_booléennes.ajoute_element(TypeBase::BOOL, valeur);
}

AtomeConstanteCaractère *ConstructriceRI::crée_constante_caractère(Type const *type,
                                                                   uint64_t valeur)
{
    return constantes_caractères.ajoute_element(type, valeur);
}

AtomeConstanteNulle *ConstructriceRI::crée_constante_nulle(Type const *type)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (constantes_nulles) {
        if (it.type == type) {
            return &it;
        }
    }
#endif
    return constantes_nulles.ajoute_element(type);
}

InstructionBranche *ConstructriceRI::crée_branche(NoeudExpression const *site_,
                                                  InstructionLabel *label,
                                                  bool crée_seulement)
{
    auto inst = insts_branche.ajoute_element(site_, label);

    label->drapeaux |= DrapeauxAtome::EST_UTILISÉ;

    if (!crée_seulement) {
        m_fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

InstructionBrancheCondition *ConstructriceRI::crée_branche_condition(
    NoeudExpression const *site_,
    Atome *valeur,
    InstructionLabel *label_si_vrai,
    InstructionLabel *label_si_faux)
{
    auto inst = insts_branche_condition.ajoute_element(
        site_, valeur, label_si_vrai, label_si_faux);

    label_si_vrai->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
    label_si_faux->drapeaux |= DrapeauxAtome::EST_UTILISÉ;

    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionLabel *ConstructriceRI::crée_label(NoeudExpression const *site_)
{
    auto inst = insts_label.ajoute_element(site_, m_nombre_labels++);
    insère_label(inst);
    return inst;
}

InstructionLabel *ConstructriceRI::réserve_label(NoeudExpression const *site_)
{
    return insts_label.ajoute_element(site_, m_nombre_labels++);
}

void ConstructriceRI::insère_label(InstructionLabel *label)
{
    /* La génération de code pour les conditions (#si, #saufsi) et les boucles peut ajouter des
     * labels redondants (par exemple les labels pour après la condition ou la boucle) quand ces
     * instructions sont conséquentes. Afin d'éviter d'avoir des labels définissant des blocs
     * vides, nous ajoutons des branches implicites. */
    if (!m_fonction_courante->instructions.est_vide()) {
        auto di = m_fonction_courante->derniere_instruction();
        /* Nous pourrions avoir `if (di->est_label())` pour détecter des labels de blocs vides,
         * mais la génération de code pour par exemple les conditions d'une instructions `si` sans
         * `sinon` ne met pas de branche à la fin de `si.bloc_si_vrai`. Donc ceci permet de
         * détecter également ces cas. */
        if (!di->est_branche_ou_retourne()) {
            crée_branche(label->site, label);
        }
    }

    m_fonction_courante->instructions.ajoute(label);
}

void ConstructriceRI::insère_label_si_utilisé(InstructionLabel *label)
{
    if (!label->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
        return;
    }

    insère_label(label);
}

InstructionRetour *ConstructriceRI::crée_retour(NoeudExpression const *site_, Atome *valeur)
{
    auto inst = insts_retour.ajoute_element(site_, valeur);
    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionStockeMem *ConstructriceRI::crée_stocke_mem(NoeudExpression const *site_,
                                                       Atome *ou,
                                                       Atome *valeur,
                                                       bool crée_seulement)
{
    assert_rappel(ou->type->est_type_pointeur() || ou->type->est_type_reference(), [&]() {
        dbg() << "Le type n'est pas un pointeur : " << chaine_type(ou->type) << '\n'
              << imprime_site(site_);
    });

    assert_rappel(type_dest_et_type_source_sont_compatibles(ou->type, valeur->type), [&]() {
        auto type_élément_dest = type_dereference_pour(ou->type);
        dbg() << "\tType élément destination : " << chaine_type(type_élément_dest) << " ("
              << type_élément_dest << ") "
              << ", type source : " << chaine_type(valeur->type) << " (" << valeur->type << ")\n"
              << imprime_site(site_);
    });

    auto type = valeur->type;
    auto inst = insts_stocke_memoire.ajoute_element(site_, type, ou, valeur);

    if (!crée_seulement) {
        m_fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

InstructionChargeMem *ConstructriceRI::crée_charge_mem(NoeudExpression const *site_,
                                                       Atome *ou,
                                                       bool crée_seulement)
{
    /* nous chargeons depuis une adresse en mémoire, donc nous devons avoir un pointeur */
    assert_rappel(ou->type->est_type_pointeur() || ou->type->est_type_reference(), [&]() {
        dbg() << "Le type est '" << chaine_type(ou->type) << "'\n" << imprime_site(site_);
    });

    assert_rappel(
        ou->genre_atome == Atome::Genre::INSTRUCTION || ou->genre_atome == Atome::Genre::GLOBALE,
        [=]() {
            dbg() << "Le genre de l'atome est : " << static_cast<int>(ou->genre_atome) << ".";
        });

    auto type = type_dereference_pour(ou->type);
    auto inst = insts_charge_memoire.ajoute_element(site_, type, ou);

    if (!crée_seulement) {
        m_fonction_courante->instructions.ajoute(inst);
    }

    return inst;
}

InstructionAppel *ConstructriceRI::crée_appel(NoeudExpression const *site_, Atome *appelé)
{
    auto inst = insts_appel.ajoute_element(site_, appelé);
    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionAppel *ConstructriceRI::crée_appel(NoeudExpression const *site_,
                                              Atome *appelé,
                                              kuri::tableau<Atome *, int> &&args)
{
#ifndef NDEBUG
    TypeFonction const *type_fonction;
    if (appelé->type->est_type_fonction()) {
        type_fonction = appelé->type->comme_type_fonction();
    }
    else {
        type_fonction = appelé->type->comme_type_pointeur()->type_pointe->comme_type_fonction();
    }

    POUR_INDEX (type_fonction->types_entrees) {
        assert_rappel(sont_types_compatibles_pour_param_appel(it, args[index_it]->type), [&]() {
            dbg() << "Espéré " << chaine_type(it);
            dbg() << "Obtenu " << chaine_type(args[index_it]->type);
            dbg() << imprime_site(site_);
            if (appelé->est_fonction()) {
                auto fonction_appelé = appelé->comme_fonction();
                if (fonction_appelé->decl) {
                    dbg() << "Dans l'appel de " << nom_humainement_lisible(fonction_appelé->decl);
                }
            }
        });
    }
#endif

    auto inst = insts_appel.ajoute_element(site_, appelé, std::move(args));
    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionOpUnaire *ConstructriceRI::crée_op_unaire(NoeudExpression const *site_,
                                                     Type const *type,
                                                     OpérateurUnaire::Genre op,
                                                     Atome *valeur)
{
    auto inst = insts_opunaire.ajoute_element(site_, type, op, valeur);
    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionOpBinaire *ConstructriceRI::crée_op_binaire(NoeudExpression const *site_,
                                                       Type const *type,
                                                       OpérateurBinaire::Genre op,
                                                       Atome *valeur_gauche,
                                                       Atome *valeur_droite)
{
    assert_rappel(
        sont_types_compatibles_pour_opérateur_binaire(valeur_gauche->type, valeur_droite->type),
        [&]() {
            dbg() << imprime_site(site_);
            dbg() << "Type à gauche " << chaine_type(valeur_gauche->type);
            dbg() << "Type à droite " << chaine_type(valeur_droite->type);
        });
    auto inst = insts_opbinaire.ajoute_element(site_, type, op, valeur_gauche, valeur_droite);
    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionOpBinaire *ConstructriceRI::crée_op_comparaison(NoeudExpression const *site_,
                                                           OpérateurBinaire::Genre op,
                                                           Atome *valeur_gauche,
                                                           Atome *valeur_droite)
{
    return crée_op_binaire(site_, TypeBase::BOOL, op, valeur_gauche, valeur_droite);
}

InstructionAccedeIndex *ConstructriceRI::crée_accès_index(NoeudExpression const *site_,
                                                          Atome *accédé,
                                                          Atome *index)
{
    auto type_élément = static_cast<Type const *>(nullptr);
    if (accédé->est_constante_tableau() || accédé->est_données_constantes()) {
        type_élément = accédé->type;
    }
    else {
        assert_rappel(accédé->type->est_type_pointeur(),
                      [=]() { dbg() << "Type accédé : '" << chaine_type(accédé->type) << "'"; });
        auto type_pointeur = accédé->type->comme_type_pointeur();
        type_élément = type_pointeur->type_pointe;
    }

    assert_rappel(
        dls::outils::est_element(
            type_élément->genre, GenreNoeud::POINTEUR, GenreNoeud::TABLEAU_FIXE) ||
            (type_élément->est_type_opaque() &&
             dls::outils::est_element(type_élément->comme_type_opaque()->type_opacifie->genre,
                                      GenreNoeud::POINTEUR,
                                      GenreNoeud::TABLEAU_FIXE)),
        [=]() { dbg() << "Type accédé : '" << chaine_type(accédé->type) << "'"; });

    auto type = m_typeuse.type_pointeur_pour(type_dereference_pour(type_élément), false);

    auto inst = insts_accede_index.ajoute_element(site_, type, accédé, index);
    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

InstructionAccedeMembre *ConstructriceRI::crée_référence_membre(
    NoeudExpression const *site_, Type const *type, Atome *accédé, int index, bool crée_seulement)
{

    auto inst = insts_accede_membre.ajoute_element(site_, type, accédé, index);
    if (!crée_seulement) {
        m_fonction_courante->instructions.ajoute(inst);
    }
    return inst;
}

InstructionAccedeMembre *ConstructriceRI::crée_référence_membre(NoeudExpression const *site_,
                                                                Atome *accédé,
                                                                int index,
                                                                bool crée_seulement)
{
    assert_rappel(accédé->type->est_type_pointeur() || accédé->type->est_type_reference(),
                  [=]() { dbg() << "Type accédé : '" << chaine_type(accédé->type) << "'"; });

    auto type_élément = type_dereference_pour(accédé->type);
    if (type_élément->est_type_opaque()) {
        type_élément = type_élément->comme_type_opaque()->type_opacifie;
    }

    assert_rappel(type_élément->est_type_compose(), [&]() {
        dbg() << "Type accédé : '" << chaine_type(type_élément) << "'\n" << imprime_site(site_);
    });

    auto type_composé = static_cast<TypeCompose *>(type_élément);
    if (type_composé->est_type_union()) {
        type_composé = type_composé->comme_type_union()->type_structure;
    }

    auto type = type_composé->membres[index].type;
    assert_rappel(
        (type_composé->membres[index].drapeaux & MembreTypeComposé::PROVIENT_D_UN_EMPOI) == 0,
        [&]() {
            dbg() << chaine_type(type_composé) << '\n'
                  << imprime_site(site_) << '\n'
                  << imprime_arbre(site_, 0);
        });

    /* nous retournons un pointeur vers le membre */
    type = m_typeuse.type_pointeur_pour(type, false);
    return crée_référence_membre(site_, type, accédé, index, crée_seulement);
}

Instruction *ConstructriceRI::crée_reference_membre_et_charge(NoeudExpression const *site_,
                                                              Atome *accédé,
                                                              int index)
{
    auto inst = crée_référence_membre(site_, accédé, index);
    return crée_charge_mem(site_, inst);
}

InstructionTranstype *ConstructriceRI::crée_transtype(NoeudExpression const *site_,
                                                      Type const *type,
                                                      Atome *valeur,
                                                      TypeTranstypage op)
{
    // dbg() << __func__ << ", type : " << chaine_type(type) << ", valeur " <<
    // chaine_type(valeur->type);
    auto inst = insts_transtype.ajoute_element(site_, type, valeur, op);
    m_fonction_courante->instructions.ajoute(inst);
    return inst;
}

TranstypeConstant *ConstructriceRI::crée_transtype_constant(Type const *type,
                                                            AtomeConstante *valeur)
{
    return transtypes_constants.ajoute_element(type, valeur);
}

AccedeIndexConstant *ConstructriceRI::crée_accès_index_constant(AtomeConstante *accédé,
                                                                int64_t index)
{
    assert_rappel(accédé->type->est_type_pointeur(),
                  [=]() { dbg() << "Type accédé : '" << chaine_type(accédé->type) << "'"; });
    auto type_pointeur = accédé->type->comme_type_pointeur();
    assert_rappel(
        dls::outils::est_element(
            type_pointeur->type_pointe->genre, GenreNoeud::POINTEUR, GenreNoeud::TABLEAU_FIXE),
        [=]() { dbg() << "Type accédé : '" << chaine_type(type_pointeur->type_pointe) << "'"; });

    auto type = m_typeuse.type_pointeur_pour(type_dereference_pour(type_pointeur->type_pointe),
                                             false);

    return accede_index_constants.ajoute_element(type, accédé, index);
}

AtomeConstante *ConstructriceRI::crée_initialisation_défaut_pour_type(Type const *type)
{
    switch (type->genre) {
        case GenreNoeud::RIEN:
        case GenreNoeud::POLYMORPHIQUE:
        {
            return nullptr;
        }
        case GenreNoeud::BOOL:
        {
            return crée_constante_booléenne(false);
        }
        /* Les seules réféences pouvant être nulles sont celles générées par la compilatrice pour
         * les boucles pour. */
        case GenreNoeud::REFERENCE:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::FONCTION:
        {
            return crée_constante_nulle(type);
        }
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::TYPE_DE_DONNEES:
        {
            return crée_constante_nombre_entier(type, 0);
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            return crée_constante_nombre_entier(TypeBase::Z32, 0);
        }
        case GenreNoeud::REEL:
        {
            if (type->taille_octet == 2) {
                return crée_constante_nombre_entier(TypeBase::N16, 0);
            }

            return crée_constante_nombre_réel(type, 0.0);
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();
            auto valeur = crée_initialisation_défaut_pour_type(type_tableau->type_pointe);
            return crée_initialisation_tableau(type_tableau, valeur);
        }
        case GenreNoeud::DECLARATION_UNION:
        {
            auto type_union = type->comme_type_union();

            if (type_union->est_nonsure) {
                return crée_initialisation_défaut_pour_type(type_union->type_le_plus_grand);
            }

            auto valeurs = kuri::tableau<AtomeConstante *>();
            valeurs.reserve(2);

            valeurs.ajoute(crée_initialisation_défaut_pour_type(type_union->type_le_plus_grand));
            valeurs.ajoute(crée_initialisation_défaut_pour_type(TypeBase::Z32));

            return crée_constante_structure(type, std::move(valeurs));
        }
        case GenreNoeud::CHAINE:
        case GenreNoeud::EINI:
        case GenreNoeud::DECLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::TUPLE:
        {
            auto type_composé = static_cast<TypeCompose const *>(type);
            auto valeurs = kuri::tableau<AtomeConstante *>();
            valeurs.reserve(type_composé->membres.taille());

            POUR (type_composé->donne_membres_pour_code_machine()) {
                auto valeur = crée_initialisation_défaut_pour_type(it.type);
                valeurs.ajoute(valeur);
            }

            return crée_constante_structure(type, std::move(valeurs));
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);
            return crée_constante_nombre_entier(type_enum, 0);
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            auto valeur = crée_initialisation_défaut_pour_type(type_opaque->type_opacifie);
            valeur->type = type_opaque;
            return valeur;
        }
        default:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud géré pour type : " << type->genre; });
            return nullptr;
        }
    }

    return nullptr;
}

kuri::chaine ConstructriceRI::imprime_site(NoeudExpression const *site) const
{
    if (!m_fonction_courante || !m_fonction_courante->decl) {
        return "aucun site";
    }
    auto const unité = m_fonction_courante->decl->unité;
    if (!unité) {
        return "aucun site";
    }
    auto espace = unité->espace;
    return erreur::imprime_site(*espace, site);
}

void ConstructriceRI::rassemble_statistiques(Statistiques &stats)
{
    auto &stats_ri = stats.stats_ri;

#define AJOUTE_ENTREE(Tableau)                                                                    \
    stats_ri.fusionne_entrée({#Tableau, Tableau.taille(), Tableau.memoire_utilisee()});

    AJOUTE_ENTREE(constantes_entières)
    AJOUTE_ENTREE(constantes_réelles)
    AJOUTE_ENTREE(constantes_booléennes)
    AJOUTE_ENTREE(constantes_nulles)
    AJOUTE_ENTREE(constantes_caractères)
    AJOUTE_ENTREE(constantes_structures)
    AJOUTE_ENTREE(constantes_tableaux)
    AJOUTE_ENTREE(constantes_données_constantes)
    AJOUTE_ENTREE(constantes_types)
    AJOUTE_ENTREE(constantes_index_table_type)
    AJOUTE_ENTREE(constantes_taille_de)
    AJOUTE_ENTREE(initialisations_tableau)
    AJOUTE_ENTREE(non_initialisations)
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
    AJOUTE_ENTREE(accede_index_constants)

#undef AJOUTE_ENTREE

    auto mémoire_utilisée = insts_appel.memoire_utilisee();

    pour_chaque_element(insts_appel, [&](InstructionAppel const &it) {
        mémoire_utilisée += it.args.taille_memoire();
    });

    stats_ri.fusionne_entrée({"insts_appel", insts_appel.taille(), mémoire_utilisée});
}

/** \} */

/* ************************************************************************** */

CompilatriceRI::CompilatriceRI(Compilatrice &compilatrice)
    : m_compilatrice(compilatrice),
      m_constructrice(compilatrice.typeuse, *compilatrice.registre_ri)
{
}

CompilatriceRI::~CompilatriceRI()
{
}

void CompilatriceRI::génère_ri_pour_noeud(EspaceDeTravail *espace, NoeudExpression *noeud)
{
    m_espace = espace;
    génère_ri_pour_noeud(noeud);
}

void CompilatriceRI::génère_ri_pour_fonction_métaprogramme(
    EspaceDeTravail *espace, NoeudDeclarationEnteteFonction *fonction)
{
    m_espace = espace;
    génère_ri_pour_fonction_métaprogramme(fonction);
}

AtomeFonction *CompilatriceRI::genere_fonction_init_globales_et_appel(
    EspaceDeTravail *espace,
    kuri::tableau_statique<AtomeGlobale *> globales,
    AtomeFonction *fonction_pour)
{
    m_espace = espace;
    return genere_fonction_init_globales_et_appel(globales, fonction_pour);
}

static kuri::tableau<AtomeGlobale *> donne_globales_à_initialiser(
    kuri::tableau_statique<AtomeGlobale *> globales, Compilatrice &compilatrice)
{
    /* Commence par rassembler les globales qui furent déclarées dans le code. */
    kuri::tableau<AtomeGlobale *> globales_avec_déclarations;
    POUR (globales) {
        if (!it->decl) {
            continue;
        }

        globales_avec_déclarations.ajoute(it);
    }

    /* Tri ces globales selon leurs dépendances entre elles (les globales dépendant d'autres
     * doivent être initialisées après). */
    kuri::rassembleuse<AtomeGlobale *> rassembleuse_atomes;
    {
        auto graphe = compilatrice.graphe_dependance.verrou_ecriture();
        graphe->prepare_visite();

        POUR (globales_avec_déclarations) {
            auto noeud = it->decl->noeud_dependance;
            assert(noeud);

            if (rassembleuse_atomes.possède(it)) {
                continue;
            }

            graphe->traverse(noeud, [&](NoeudDependance const *relation) {
                if (noeud == relation) {
                    return;
                }
                if (relation->est_globale()) {
                    auto globale = relation->globale();
                    assert(globale->atome);
                    rassembleuse_atomes.insère(globale->atome->comme_globale());
                }
            });

            rassembleuse_atomes.insère(it);
        }
    }

    kuri::tableau<AtomeGlobale *> globales_triées = rassembleuse_atomes.donne_copie_éléments();
    rassembleuse_atomes.réinitialise();

    /* Le résultat est composé des globales et des globales dans leurs initialisateurs. */
    POUR (globales_triées) {
        if (rassembleuse_atomes.possède(it)) {
            continue;
        }

        /* Visite les initialisateurs d'abord. */
        if (it->initialisateur) {
            visite_atome(it->initialisateur, [&](Atome *atome_visité) {
                if (!atome_visité->est_globale()) {
                    return;
                }

                auto globale_visitée = atome_visité->comme_globale();
                rassembleuse_atomes.insère(globale_visitée);
            });
        }

        rassembleuse_atomes.insère(it);
    }

    return rassembleuse_atomes.donne_copie_éléments();
}

AtomeFonction *CompilatriceRI::genere_fonction_init_globales_et_appel(
    kuri::tableau_statique<AtomeGlobale *> globales, AtomeFonction *fonction_pour)
{
    auto nom_fonction = enchaine("init_globale", fonction_pour);
    auto ident_nom = m_compilatrice.table_identifiants->identifiant_pour_nouvelle_chaine(
        nom_fonction);

    auto types_entrees = kuri::tablet<Type *, 6>(0);
    auto type_sortie = TypeBase::RIEN;

    auto fonction = m_constructrice.crée_fonction(ident_nom->nom);
    fonction->type = m_compilatrice.typeuse.type_fonction(types_entrees, type_sortie, false);
    fonction->param_sortie = m_constructrice.crée_allocation(nullptr, type_sortie, nullptr, true);

    définis_fonction_courante(fonction);
    /* Génère d'abord le retour car generi_ri_pour_initialisation_globales le requiers. */
    m_constructrice.crée_retour(nullptr, nullptr);

    génère_ri_pour_initialisation_globales(fonction, globales);

    // crée l'appel de cette fonction et ajoute là au début de la fonction_pour

    définis_fonction_courante(fonction_pour);

    m_constructrice.crée_appel(nullptr, fonction);

    std::rotate(fonction_pour->instructions.begin() + fonction_pour->decalage_appel_init_globale,
                fonction_pour->instructions.end() - 1,
                fonction_pour->instructions.end());

    définis_fonction_courante(nullptr);

    return fonction;
}

void CompilatriceRI::définis_fonction_courante(AtomeFonction *fonction_courante)
{
    m_fonction_courante = fonction_courante;
    m_pile.efface();
    m_constructrice.définis_fonction_courante(fonction_courante);
}

AtomeConstante *CompilatriceRI::crée_tableau_global(IdentifiantCode &ident,
                                                    const Type *type,
                                                    kuri::tableau<AtomeConstante *> &&valeurs)
{
    return m_constructrice.crée_tableau_global(ident, type, std::move(valeurs));
}

Atome *CompilatriceRI::crée_charge_mem_si_chargeable(NoeudExpression const *site_, Atome *source)
{
    if (source->possède_drapeau(DrapeauxAtome::EST_CHARGEABLE)) {
        return m_constructrice.crée_charge_mem(site_, source);
    }
    return source;
}

Atome *CompilatriceRI::crée_temporaire_si_non_chargeable(NoeudExpression const *site_,
                                                         Atome *source)
{
    if (source->possède_drapeau(DrapeauxAtome::EST_CHARGEABLE)) {
        return source;
    }

    return crée_temporaire(site_, source);
}

InstructionAllocation *CompilatriceRI::crée_temporaire(NoeudExpression const *site_, Atome *source)
{
    auto résultat = m_constructrice.crée_allocation(site_, source->type, nullptr);
    m_constructrice.crée_stocke_mem(site_, résultat, source);
    return résultat;
}

void CompilatriceRI::crée_temporaire_ou_mets_dans_place(NoeudExpression const *site_,
                                                        Atome *source,
                                                        Atome *place)
{
    if (place) {
        m_constructrice.crée_stocke_mem(site_, place, source);
        place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
        return;
    }

    auto alloc = crée_temporaire(site_, source);
    empile_valeur(alloc);
}

void CompilatriceRI::crée_appel_fonction_init_type(NoeudExpression const *site_,
                                                   Type const *type,
                                                   Atome *argument)
{
    auto fonc_init = type->fonction_init;
    auto atome_fonc_init = m_constructrice.trouve_ou_insère_fonction(fonc_init);
    auto params = kuri::tableau<Atome *, int>(1);

    /* Les fonctions d'initialisation sont partagées entre certains types donc nous devons
     * transtyper vers le type approprié. */
    if (type->est_type_pointeur() || type->est_type_fonction()) {
        auto &typeuse = m_compilatrice.typeuse;
        auto type_ptr_ptr_rien = typeuse.type_pointeur_pour(TypeBase::PTR_RIEN);
        argument = m_constructrice.crée_transtype(
            site_, type_ptr_ptr_rien, argument, TypeTranstypage::BITS);
    }
    else if (type->est_type_enum()) {
        auto type_enum = static_cast<TypeEnum const *>(type);
        auto type_sousjacent = type_enum->type_sous_jacent;
        auto &typeuse = m_compilatrice.typeuse;
        auto type_ptr_ptr_rien = typeuse.type_pointeur_pour(type_sousjacent);
        argument = m_constructrice.crée_transtype(
            site_, type_ptr_ptr_rien, argument, TypeTranstypage::BITS);
    }
    else if (argument->type->comme_type_pointeur()->type_pointe->est_type_enum()) {
        auto type_enum = static_cast<TypeEnum const *>(
            argument->type->comme_type_pointeur()->type_pointe);
        auto type_sousjacent = type_enum->type_sous_jacent;
        auto &typeuse = m_compilatrice.typeuse;
        auto type_ptr_ptr_rien = typeuse.type_pointeur_pour(type_sousjacent);
        argument = m_constructrice.crée_transtype(
            site_, type_ptr_ptr_rien, argument, TypeTranstypage::BITS);
    }

    params[0] = argument;
    m_constructrice.crée_appel(site_, atome_fonc_init, std::move(params));
}

/* Retourne la boucle controlée effective de la boucle controlée passé en paramètre. Ceci prend en
 * compte les boucles remplacées par les opérateurs « pour ». */
static NoeudExpression *boucle_controlée_effective(NoeudExpression *boucle_controlée)
{
    if (boucle_controlée->est_pour()) {
        auto noeud_pour = boucle_controlée->comme_pour();

        if (noeud_pour->corps_operateur_pour) {
            auto directive = noeud_pour->corps_operateur_pour->corps_boucle;
            assert(directive);

            /* Nous devons retourner la première boucle parent de #corps_boucle. */
            auto boucle_parent = bloc_est_dans_boucle(directive->bloc_parent, nullptr);
            assert(boucle_parent);
            return boucle_controlée_effective(boucle_parent);
        }
    }

    if (boucle_controlée->substitution) {
        return boucle_controlée->substitution;
    }

    return boucle_controlée;
}

void CompilatriceRI::génère_ri_pour_noeud(NoeudExpression *noeud, Atome *place)
{
    if (noeud->substitution) {
        génère_ri_pour_noeud(noeud->substitution, place);
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
        case GenreNoeud::OCTET:
        case GenreNoeud::REEL:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::CHAINE:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::REFERENCE:
        case GenreNoeud::EINI:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::BOOL:
        case GenreNoeud::RIEN:
        case GenreNoeud::TABLEAU_FIXE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::FONCTION:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::TYPE_DE_DONNEES:
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::TUPLE:
        {
            break;
        }
        case GenreNoeud::DECLARATION_CONSTANTE:
        {
            assert_rappel(!noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE), [&]() {
                dbg() << "Obtenu une constante dans la RI.\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        /* Les déclarations de structures doivent passer par les fonctions d'initialisation. */
        case GenreNoeud::DECLARATION_STRUCTURE:
        case GenreNoeud::DECLARATION_UNION:
        {
            /* Il est possible d'avoir des déclarations de structures dans les fonctions, donc il
             * est possible d'en avoir une ici. */
            assert_rappel(m_fonction_courante, [&]() {
                dbg() << "Erreur interne : une déclaration de structure fut passée à la RI !\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        /* Ceux-ci doivent être ajoutés aux fonctions d'initialisation/finition de
         * l'environnement d'exécution */
        case GenreNoeud::DIRECTIVE_AJOUTE_FINI:
        case GenreNoeud::DIRECTIVE_AJOUTE_INIT:
        case GenreNoeud::DIRECTIVE_PRE_EXECUTABLE:
        {
            assert_rappel(false, [&]() {
                dbg() << "Erreur interne : une " << noeud->genre << " se retrouve dans la RI !\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::INSTRUCTION_SI_STATIQUE:
        case GenreNoeud::INSTRUCTION_SAUFSI_STATIQUE:
        {
            auto inst = noeud->comme_si_statique();
            assert_rappel(!inst->condition_est_vraie || !inst->bloc_si_faux, [&]() {
                dbg() << "Erreur interne : une directive " << noeud->genre
                      << " ne fut pas simplifiée !\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DIRECTIVE_EXECUTE:
        {
            auto directive = noeud->comme_execute();
            assert_rappel(directive->ident == ID::assert_, [&]() {
                dbg() << "Erreur interne : un directive ne fut pas simplifié !\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DIRECTIVE_CUISINE:
        case GenreNoeud::EXPRESSION_CONSTRUCTION_STRUCTURE:
        case GenreNoeud::INSTRUCTION_POUSSE_CONTEXTE:
        {
            assert_rappel(false, [&]() {
                dbg() << "Erreur interne : une " << noeud->genre << " ne fut pas simplifiée !\n"
                      << erreur::imprime_site(*m_espace, noeud);
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
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        case GenreNoeud::DECLARATION_OPERATEUR_POUR:
        {
            assert_rappel(false, [&]() {
                dbg() << "Erreur interne : un noeud ne fut pas simplifié !\n"
                      << "Le noeud est de genre : " << noeud->genre << '\n'
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DECLARATION_ENTETE_FONCTION:
        {
            auto decl = noeud->comme_entete_fonction();
            génère_ri_pour_fonction(decl);
            if (!decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
                assert(decl->corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));
            }
            break;
        }
        case GenreNoeud::DECLARATION_CORPS_FONCTION:
        {
            auto corps = noeud->comme_corps_fonction();
            assert(corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));
            génère_ri_pour_fonction(corps->entete);
            break;
        }
        case GenreNoeud::INSTRUCTION_DIFFERE:
        {
            m_instructions_diffères.ajoute(noeud);
            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSEE:
        {
            auto noeud_bloc = noeud->comme_bloc();

            if (!m_est_dans_diffère) {
                /* Utilise le bloc parent comme sentinelle. */
                m_instructions_diffères.ajoute(noeud_bloc->bloc_parent);
            }

            auto dernière_expression = NoeudExpression::nul();
            if (!noeud_bloc->expressions->est_vide()) {
                dernière_expression = noeud_bloc->expressions->dernière();
            }

            POUR (*noeud_bloc->expressions.verrou_lecture()) {
                if (it->est_entete_fonction()) {
                    continue;
                }
                if (m_label_après_controle) {
                    m_constructrice.insère_label(m_label_après_controle);
                    m_label_après_controle = nullptr;
                }

                génère_ri_pour_noeud(it);

                /* Insère un label si nous avons des expressions après une de ces
                 * instructions. Ce sera du code mort, mais la RI doit être bien
                 * formée et n'avoir qu'une seule branche ou un seul retour
                 * bloc. */
                if (it->est_continue() || it->est_retourne() || it->est_reprends() ||
                    it->est_arrete()) {
                    if (it != dernière_expression) {
                        m_constructrice.crée_label(it);
                    }
                }
            }

            auto derniere_instruction = m_fonction_courante->derniere_instruction();

            if (derniere_instruction->genre != GenreInstruction::RETOUR) {
                /* Génère le code pour toutes les instructions différées de ce bloc. */
                génère_ri_insts_différées(noeud_bloc->bloc_parent);
            }

            if (!m_est_dans_diffère) {
                /* Dépile jusqu'à la sentinelle. */
                while (m_instructions_diffères.dernière() != noeud_bloc->bloc_parent) {
                    m_instructions_diffères.supprime_dernier();
                }
                m_instructions_diffères.supprime_dernier();
            }

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
                génère_ri_pour_expression_droite(argument, nullptr);
                auto valeur = depile_valeur();
                auto type = fonction_init->type_initialisé();
                crée_appel_fonction_init_type(expr_appel, type, valeur);
                return;
            }

            POUR (expr_appel->parametres_resolus) {
                génère_ri_pour_expression_droite(it, nullptr);
                auto valeur = depile_valeur();

                /* Crée une temporaire pour simplifier l'enlignage, car nous devrons
                 * remplacer les locales par les expressions passées, et il est plus
                 * simple de remplacer une allocation par une autre. Par contre si nous avons une
                 * instruction de chargement (indiquant une allocation, ou référence d'une chose
                 * ultimement allouée), ce n'est pas la peine de créer une temporaire. */
                if (valeur->est_instruction() && valeur->comme_instruction()->est_charge()) {
                    args.ajoute(valeur);
                    continue;
                }

                auto alloc = crée_temporaire(it, valeur);
                args.ajoute(m_constructrice.crée_charge_mem(it, alloc));
            }

            génère_ri_pour_expression_droite(expr_appel->expression, nullptr);
            auto atome_fonc = depile_valeur();

            assert_rappel(atome_fonc && atome_fonc->type != nullptr, [&] {
                if (atome_fonc == nullptr) {
                    dbg() << "L'atome est nul !\n" << erreur::imprime_site(*m_espace, expr_appel);
                }
                else if (atome_fonc->type == nullptr) {
                    dbg() << "Le type de l'atome est nul !\n"
                          << erreur::imprime_site(*m_espace, expr_appel);
                }
            });

            assert_rappel(atome_fonc->type->est_type_fonction(), [&] {
                dbg() << "L'atome n'est pas de type fonction mais de type "
                      << chaine_type(atome_fonc->type) << " !\n"
                      << erreur::imprime_site(*espace(), expr_appel) << '\n'
                      << erreur::imprime_site(*espace(), expr_appel->expression) << '\n'
                      << ((!expr_appel->expression->substitution) ?
                              "L'appelée n'a pas de substitution !" :
                              "");
            });

            auto type_fonction = atome_fonc->type->comme_type_fonction();
            Atome *adresse_retour = nullptr;

            if (!type_fonction->type_sortie->est_type_rien()) {
                adresse_retour = place;
                if (!adresse_retour) {
                    adresse_retour = m_constructrice.crée_allocation(
                        expr_appel, type_fonction->type_sortie, nullptr);
                }
            }

            auto valeur = m_constructrice.crée_appel(expr_appel, atome_fonc, std::move(args));

            if (adresse_retour) {
                m_constructrice.crée_stocke_mem(noeud, adresse_retour, valeur);
                if (adresse_retour != place) {
                    empile_valeur(adresse_retour);
                }
                else {
                    place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
                }
                return;
            }

            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_DECLARATION:
        {
            auto expr_ref = noeud->comme_reference_declaration();
            auto decl_ref = expr_ref->declaration_referee;

            assert_rappel(decl_ref, [&]() {
                dbg() << erreur::imprime_site(*m_espace, noeud) << '\n'
                      << "La référence à la déclaration est nulle " << noeud->ident->nom << " ("
                      << chaine_type(noeud->type) << ")\n";
            });

            if (decl_ref->est_entete_fonction()) {
                auto atome_fonc = m_constructrice.trouve_ou_insère_fonction(
                    decl_ref->comme_entete_fonction());
                empile_valeur(atome_fonc);
                return;
            }

            if (decl_ref->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
                empile_valeur(m_constructrice.trouve_ou_insère_globale(decl_ref));
                return;
            }

            auto locale = decl_ref->comme_declaration_symbole()->atome;
            assert_rappel(locale, [&]() {
                dbg() << erreur::imprime_site(*m_espace, noeud) << '\n'
                      << "Aucune locale trouvée pour " << noeud->ident->nom << " ("
                      << chaine_type(noeud->type) << ")\n"
                      << "\nLa locale fut déclarée ici :\n"
                      << erreur::imprime_site(*m_espace, decl_ref);
            });
            empile_valeur(locale);
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE:
        {
            génère_ri_pour_accès_membre(noeud->comme_reference_membre());
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_MEMBRE_UNION:
        {
            génère_ri_pour_accès_membre_union(noeud->comme_reference_membre_union());
            break;
        }
        case GenreNoeud::EXPRESSION_REFERENCE_TYPE:
        {
            auto type_de_donnees = noeud->type->comme_type_type_de_donnees();

            if (type_de_donnees->type_connu) {
                empile_valeur(m_constructrice.crée_constante_type(type_de_donnees->type_connu));
                return;
            }

            empile_valeur(m_constructrice.crée_constante_type(type_de_donnees));
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_VARIABLE:
        {
            auto expr_ass = noeud->comme_assignation_variable();
            génère_ri_pour_assignation_variable(expr_ass->donnees_exprs);
            break;
        }
        case GenreNoeud::DECLARATION_VARIABLE:
        {
            génère_ri_pour_déclaration_variable(noeud->comme_declaration_variable());
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_REEL:
        {
            empile_valeur(m_constructrice.crée_constante_nombre_réel(
                noeud->type, noeud->comme_litterale_reel()->valeur));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NOMBRE_ENTIER:
        {
            if (noeud->type->est_type_reel()) {
                empile_valeur(m_constructrice.crée_constante_nombre_réel(
                    noeud->type, static_cast<double>(noeud->comme_litterale_entier()->valeur)));
            }
            else {
                empile_valeur(m_constructrice.crée_constante_nombre_entier(
                    noeud->type, noeud->comme_litterale_entier()->valeur));
            }

            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CHAINE:
        {
            auto lit_chaine = noeud->comme_litterale_chaine();
            auto chaine = compilatrice().gerante_chaine->chaine_pour_adresse(lit_chaine->valeur);
            auto constante = crée_chaine(chaine);

            assert_rappel(
                (noeud->lexeme->chaine.taille() != 0 && chaine.taille() != 0) ||
                    (noeud->lexeme->chaine.taille() == 0 && chaine.taille() == 0) ||
                    noeud->possède_drapeau(DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION),
                [&]() {
                    dbg() << erreur::imprime_site(*espace(), noeud) << '\n'
                          << "La chaine n'est pas de la bonne taille !\n"
                          << "Le lexème a une chaine taille de " << noeud->lexeme->chaine.taille()
                          << " alors que la chaine littérale a une taille de " << chaine.taille()
                          << '\n'
                          << "L'index de la chaine est de " << lit_chaine->valeur << '\n'
                          << "La valeur de la chaine du lexème est \"" << noeud->lexeme->chaine
                          << "\"\n"
                          << "La valeur de la chaine littérale est \"" << chaine << "\"";
                });

            if (m_fonction_courante == nullptr) {
                empile_valeur(constante);
                return;
            }

            crée_temporaire_ou_mets_dans_place(noeud, constante, place);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_BOOLEEN:
        {
            empile_valeur(
                m_constructrice.crée_constante_booléenne(noeud->comme_litterale_bool()->valeur));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_CARACTERE:
        {
            // À FAIRE : caractères Unicode
            auto caractere = static_cast<unsigned char>(
                noeud->comme_litterale_caractere()->valeur);
            empile_valeur(m_constructrice.crée_constante_caractère(noeud->type, caractere));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTERALE_NUL:
        {
            empile_valeur(m_constructrice.crée_constante_nulle(noeud->type));
            break;
        }
        case GenreNoeud::OPERATEUR_BINAIRE:
        {
            auto expr_bin = noeud->comme_expression_binaire();

            génère_ri_pour_expression_droite(expr_bin->operande_gauche, nullptr);
            auto valeur_gauche = depile_valeur();
            génère_ri_pour_expression_droite(expr_bin->operande_droite, nullptr);
            auto valeur_droite = depile_valeur();
            auto résultat = m_constructrice.crée_op_binaire(
                noeud, noeud->type, expr_bin->op->genre, valeur_gauche, valeur_droite);

            crée_temporaire_ou_mets_dans_place(noeud, résultat, place);
            break;
        }
        case GenreNoeud::EXPRESSION_LOGIQUE:
        {
            génère_ri_pour_expression_logique(noeud->comme_expression_logique(), place);
            break;
        }
        case GenreNoeud::EXPRESSION_INDEXAGE:
        {
            auto expr_bin = noeud->comme_indexage();
            auto type_gauche = expr_bin->operande_gauche->type;

            if (type_gauche->est_type_opaque()) {
                type_gauche = type_gauche->comme_type_opaque()->type_opacifie;
            }

            génère_ri_pour_noeud(expr_bin->operande_gauche);
            auto pointeur = depile_valeur();
            génère_ri_pour_expression_droite(expr_bin->operande_droite, nullptr);
            auto valeur = depile_valeur();

            if (type_gauche->est_type_pointeur()) {
                empile_valeur(m_constructrice.crée_accès_index(noeud, pointeur, valeur));
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
                    auto label1 = m_constructrice.réserve_label(noeud);
                    auto label2 = m_constructrice.réserve_label(noeud);
                    auto label3 = m_constructrice.réserve_label(noeud);
                    auto label4 = m_constructrice.réserve_label(noeud);

                    auto condition = m_constructrice.crée_op_comparaison(
                        noeud,
                        OpérateurBinaire::Genre::Comp_Inf,
                        valeur_,
                        m_constructrice.crée_z64(0));
                    m_constructrice.crée_branche_condition(noeud, condition, label1, label2);

                    m_constructrice.insère_label(label1);

                    auto params = kuri::tableau<Atome *, int>(2);
                    params[0] = acces_taille;
                    params[1] = valeur_;
                    m_constructrice.crée_appel(noeud, fonction, std::move(params));

                    m_constructrice.insère_label(label2);

                    condition = m_constructrice.crée_op_comparaison(
                        noeud, OpérateurBinaire::Genre::Comp_Sup_Egal, valeur_, acces_taille);
                    m_constructrice.crée_branche_condition(noeud, condition, label3, label4);

                    m_constructrice.insère_label(label3);

                    params = kuri::tableau<Atome *, int>(2);
                    params[0] = acces_taille;
                    params[1] = valeur_;
                    m_constructrice.crée_appel(noeud, fonction, std::move(params));

                    m_constructrice.insère_label(label4);
                };

            if (type_gauche->est_type_tableau_fixe()) {
                if (noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto type_tableau_fixe = type_gauche->comme_type_tableau_fixe();
                    auto acces_taille = m_constructrice.crée_z64(
                        static_cast<unsigned>(type_tableau_fixe->taille));
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_constructrice.trouve_ou_insère_fonction(
                            m_compilatrice.interface_kuri->decl_panique_tableau));
                }
                empile_valeur(m_constructrice.crée_accès_index(noeud, pointeur, valeur));
                return;
            }

            if (type_gauche->est_type_tableau_dynamique() || type_gauche->est_type_variadique()) {
                if (noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto acces_taille = m_constructrice.crée_reference_membre_et_charge(
                        noeud, pointeur, 1);
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_constructrice.trouve_ou_insère_fonction(
                            m_compilatrice.interface_kuri->decl_panique_tableau));
                }
                pointeur = m_constructrice.crée_référence_membre(noeud, pointeur, 0);
                empile_valeur(m_constructrice.crée_accès_index(noeud, pointeur, valeur));
                return;
            }

            if (type_gauche->est_type_chaine()) {
                if (noeud->aide_generation_code != IGNORE_VERIFICATION) {
                    auto acces_taille = m_constructrice.crée_reference_membre_et_charge(
                        noeud, pointeur, 1);
                    genere_protection_limites(
                        acces_taille,
                        valeur,
                        m_constructrice.trouve_ou_insère_fonction(
                            m_compilatrice.interface_kuri->decl_panique_chaine));
                }
                pointeur = m_constructrice.crée_référence_membre(noeud, pointeur, 0);
                empile_valeur(m_constructrice.crée_accès_index(noeud, pointeur, valeur));
                return;
            }

            break;
        }
        case GenreNoeud::OPERATEUR_UNAIRE:
        {
            auto expr_un = noeud->comme_expression_unaire();
            génère_ri_pour_expression_droite(expr_un->operande, nullptr);
            auto valeur = depile_valeur();
            empile_valeur(
                m_constructrice.crée_op_unaire(noeud, expr_un->type, expr_un->op->genre, valeur));
            break;
        }
        case GenreNoeud::EXPRESSION_PRISE_ADRESSE:
        {
            auto prise_adresse = noeud->comme_prise_adresse();
            génère_ri_pour_noeud(prise_adresse->opérande);
            auto valeur = depile_valeur();
            if (prise_adresse->opérande->type->est_type_reference()) {
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            }

            if (!expression_gauche) {
                valeur = crée_temporaire(noeud, valeur);
            }

            empile_valeur(valeur);
            return;
        }
        case GenreNoeud::EXPRESSION_PRISE_REFERENCE:
        {
            assert_rappel(false, [&]() {
                dbg() << "Prise de référence dans la RI :\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            return;
        }
        case GenreNoeud::EXPRESSION_NEGATION_LOGIQUE:
        {
            // @simplifie
            auto négation = noeud->comme_negation_logique();
            auto condition = négation->opérande;
            auto type_condition = condition->type;
            auto valeur = static_cast<Atome *>(nullptr);

            /* Peut être implémenté via x = 1 ^ (valeur == 0). */
            switch (type_condition->genre) {
                case GenreNoeud::ENTIER_NATUREL:
                case GenreNoeud::ENTIER_RELATIF:
                case GenreNoeud::ENTIER_CONSTANT:
                {
                    génère_ri_pour_expression_droite(condition, nullptr);
                    auto valeur1 = depile_valeur();
                    auto valeur2 = m_constructrice.crée_constante_nombre_entier(type_condition, 0);
                    valeur = m_constructrice.crée_op_comparaison(
                        noeud, OpérateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                    break;
                }
                case GenreNoeud::BOOL:
                {
                    génère_ri_pour_expression_droite(condition, nullptr);
                    auto valeur1 = depile_valeur();
                    auto valeur2 = m_constructrice.crée_constante_booléenne(false);
                    valeur = m_constructrice.crée_op_comparaison(
                        noeud, OpérateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                    break;
                }
                case GenreNoeud::FONCTION:
                case GenreNoeud::POINTEUR:
                {
                    génère_ri_pour_expression_droite(condition, nullptr);
                    auto valeur1 = depile_valeur();
                    auto valeur2 = m_constructrice.crée_constante_nulle(type_condition);
                    valeur = m_constructrice.crée_op_comparaison(
                        noeud, OpérateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                    break;
                }
                case GenreNoeud::EINI:
                {
                    génère_ri_pour_noeud(condition);
                    auto pointeur = depile_valeur();
                    auto pointeur_pointeur = m_constructrice.crée_référence_membre(
                        noeud, pointeur, 0);
                    auto valeur1 = m_constructrice.crée_charge_mem(noeud, pointeur_pointeur);
                    auto valeur2 = m_constructrice.crée_constante_nulle(valeur1->type);
                    valeur = m_constructrice.crée_op_comparaison(
                        noeud, OpérateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
                    break;
                }
                case GenreNoeud::CHAINE:
                case GenreNoeud::TABLEAU_DYNAMIQUE:
                {
                    génère_ri_pour_noeud(condition);
                    auto pointeur = depile_valeur();
                    auto pointeur_taille = m_constructrice.crée_référence_membre(
                        noeud, pointeur, 1);
                    auto valeur1 = m_constructrice.crée_charge_mem(noeud, pointeur_taille);
                    auto valeur2 = m_constructrice.crée_z64(0);
                    valeur = m_constructrice.crée_op_comparaison(
                        noeud, OpérateurBinaire::Genre::Comp_Egal, valeur1, valeur2);
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
        case GenreNoeud::INSTRUCTION_RETOUR:
        {
            auto inst = noeud->comme_retourne();

            auto valeur_ret = static_cast<Atome *>(nullptr);

            if (inst->expression) {
                génère_ri_pour_expression_droite(inst->expression, nullptr);
                valeur_ret = depile_valeur();
            }

            auto bloc_final = NoeudBloc::nul();
            if (m_fonction_courante->decl) {
                bloc_final = m_fonction_courante->decl->bloc_constantes;
            }

            génère_ri_insts_différées(bloc_final);
            m_constructrice.crée_retour(noeud, valeur_ret);
            break;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto inst_si = noeud->comme_si();

            auto label_si_vrai = m_constructrice.réserve_label(noeud);
            auto label_si_faux = m_constructrice.réserve_label(noeud);

            if (noeud->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
                génère_ri_pour_condition(inst_si->condition, label_si_faux, label_si_vrai);
            }
            else {
                génère_ri_pour_condition(inst_si->condition, label_si_vrai, label_si_faux);
            }

            if (inst_si->bloc_si_faux) {
                auto label_apres_instruction = m_constructrice.réserve_label(noeud);

                m_constructrice.insère_label(label_si_vrai);
                génère_ri_pour_noeud(inst_si->bloc_si_vrai);

                auto di = m_fonction_courante->derniere_instruction();
                if (!di->est_branche_ou_retourne()) {
                    m_constructrice.crée_branche(noeud, label_apres_instruction);
                }

                m_constructrice.insère_label(label_si_faux);
                génère_ri_pour_noeud(inst_si->bloc_si_faux);
                m_constructrice.insère_label_si_utilisé(label_apres_instruction);

                if (!label_apres_instruction->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
                    m_label_après_controle = label_apres_instruction;
                }
            }
            else {
                m_constructrice.insère_label(label_si_vrai);
                génère_ri_pour_noeud(inst_si->bloc_si_vrai);
                m_constructrice.insère_label(label_si_faux);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_BOUCLE:
        case GenreNoeud::INSTRUCTION_REPETE:
        case GenreNoeud::INSTRUCTION_TANTQUE:
        {
            auto boucle = noeud->comme_boucle();

            /* labels pour les différents blocs possible */
            auto label_boucle = m_constructrice.réserve_label(noeud);
            auto label_pour_bloc_inc = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_sansarret = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_sinon = static_cast<InstructionLabel *>(nullptr);
            auto label_apres_boucle = m_constructrice.réserve_label(noeud);

            /* labels pour les controles des boucles */
            auto label_pour_continue = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_arret = static_cast<InstructionLabel *>(nullptr);
            auto label_pour_arret_implicite = static_cast<InstructionLabel *>(nullptr);

            if (boucle->bloc_inc) {
                label_pour_bloc_inc = m_constructrice.réserve_label(boucle);
                label_pour_continue = label_pour_bloc_inc;
            }
            else {
                label_pour_continue = label_boucle;
            }

            if (boucle->bloc_sansarret) {
                label_pour_sansarret = m_constructrice.réserve_label(boucle);
                label_pour_arret_implicite = label_pour_sansarret;
            }
            else {
                label_pour_arret_implicite = label_apres_boucle;
            }

            if (boucle->bloc_sinon) {
                label_pour_sinon = m_constructrice.réserve_label(noeud);
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
                génère_ri_pour_noeud(boucle->bloc_pre);
                m_constructrice.crée_branche(noeud, label_boucle);
            }

            m_constructrice.insère_label(label_boucle);
            génère_ri_pour_noeud(boucle->bloc);

            auto di = m_fonction_courante->derniere_instruction();

            if (boucle->bloc_inc) {
                m_constructrice.insère_label(label_pour_bloc_inc);
                génère_ri_pour_noeud(boucle->bloc_inc);

                if (di->est_branche_ou_retourne()) {
                    m_constructrice.crée_branche(noeud, label_boucle);
                }
            }

            if (!di->est_branche_ou_retourne()) {
                m_constructrice.crée_branche(noeud, label_boucle);
            }

            if (boucle->bloc_sansarret) {
                m_constructrice.insère_label(label_pour_sansarret);
                génère_ri_pour_noeud(boucle->bloc_sansarret);
                di = m_fonction_courante->derniere_instruction();
                if (!di->est_branche_ou_retourne()) {
                    m_constructrice.crée_branche(boucle, label_apres_boucle);
                }
            }

            if (boucle->bloc_sinon) {
                m_constructrice.insère_label(label_pour_sinon);
                génère_ri_pour_noeud(boucle->bloc_sinon);
            }

            m_constructrice.insère_label_si_utilisé(label_apres_boucle);

            if (!label_apres_boucle->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
                m_label_après_controle = label_apres_boucle;
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_ARRETE:
        {
            auto inst = noeud->comme_arrete();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlee);

            if (inst->possède_drapeau(DrapeauxNoeud::EST_IMPLICITE)) {
                auto label = boucle_controlee->comme_boucle()->label_pour_arrete_implicite;
                m_constructrice.crée_branche(noeud, label);
            }
            else {
                auto label = boucle_controlee->comme_boucle()->label_pour_arrete;
                génère_ri_insts_différées(boucle_controlee->bloc_parent);
                m_constructrice.crée_branche(noeud, label);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_CONTINUE:
        {
            auto inst = noeud->comme_continue();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlee);
            auto label = boucle_controlee->comme_boucle()->label_pour_continue;
            génère_ri_insts_différées(boucle_controlee->bloc_parent);
            m_constructrice.crée_branche(noeud, label);
            break;
        }
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            auto inst = noeud->comme_reprends();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlee);
            auto label = boucle_controlee->comme_boucle()->label_pour_reprends;
            génère_ri_insts_différées(boucle_controlee->bloc_parent);
            m_constructrice.crée_branche(noeud, label);
            break;
        }
        case GenreNoeud::EXPRESSION_COMME:
        {
            auto inst = noeud->comme_comme();
            auto expr = inst->expression;

            if (expression_gauche &&
                inst->transformation.type == TypeTransformation::DEREFERENCE) {
                génère_ri_pour_noeud(expr);
                /* déréférence l'adresse du pointeur */
                empile_valeur(m_constructrice.crée_charge_mem(noeud, depile_valeur()));
                break;
            }

            auto alloc = place;
            if (!alloc) {
                alloc = m_constructrice.crée_allocation(noeud, inst->type, nullptr);
            }

            génère_ri_transformee_pour_noeud(expr, alloc, inst->transformation);
            if (alloc != place) {
                empile_valeur(alloc);
            }
            else {
                place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
            }
            break;
        }
        case GenreNoeud::EXPRESSION_TABLEAU_ARGS_VARIADIQUES:
        {
            auto noeud_tableau = noeud->comme_args_variadiques();
            auto taille_tableau = noeud_tableau->expressions.taille();

            if (taille_tableau == 0) {
                auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(noeud->type);
                auto init = m_constructrice.crée_initialisation_défaut_pour_type(type_tableau_dyn);
                auto alloc = crée_temporaire(noeud, init);
                empile_valeur(alloc);
                return;
            }

            if (taille_tableau == 1) {
                /* Crée une temporaire scalaire. */
                auto tmp = m_constructrice.crée_allocation(noeud, noeud->type, nullptr);
                auto expression = noeud_tableau->expressions[0];
                génère_ri_pour_expression_droite(expression, tmp);

                /* Crée un tableau dynamique. */
                auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(noeud->type);
                auto alloc_tableau_dyn = m_constructrice.crée_allocation(
                    noeud, type_tableau_dyn, nullptr);

                /* Le pointeur du tableau doit être vers la temporaire. */
                auto ptr_pointeur_donnees = m_constructrice.crée_référence_membre(
                    noeud, alloc_tableau_dyn, 0);
                m_constructrice.crée_stocke_mem(noeud, ptr_pointeur_donnees, tmp);

                auto ptr_taille = m_constructrice.crée_référence_membre(
                    noeud, alloc_tableau_dyn, 1);
                m_constructrice.crée_stocke_mem(noeud, ptr_taille, m_constructrice.crée_z64(1));
                empile_valeur(alloc_tableau_dyn);
                return;
            }

            auto type_tableau_fixe = m_compilatrice.typeuse.type_tableau_fixe(noeud->type,
                                                                              taille_tableau);
            auto pointeur_tableau = m_constructrice.crée_allocation(
                noeud, type_tableau_fixe, nullptr);

            auto index = 0ul;
            POUR (noeud_tableau->expressions) {
                auto index_tableau = m_constructrice.crée_accès_index(
                    noeud, pointeur_tableau, m_constructrice.crée_z64(index++));
                génère_ri_pour_expression_droite(it, index_tableau);
            }

            auto valeur = converti_vers_tableau_dyn(
                noeud, pointeur_tableau, type_tableau_fixe, nullptr);
            empile_valeur(valeur);
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        {
            auto expr = noeud->comme_construction_tableau();
            génère_ri_pour_construction_tableau(expr, place);
            break;
        }
        case GenreNoeud::EXPRESSION_INFO_DE:
        {
            auto inst = noeud->comme_info_de();
            auto enfant = inst->expression;
            auto valeur = crée_info_type(enfant->type, noeud);

            /* utilise une temporaire pour simplifier la compilation d'expressions du style :
             * info_de(z32).id */
            crée_temporaire_ou_mets_dans_place(noeud, valeur, place);
            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto type_fonction = noeud->type->comme_type_fonction();
            auto type_pointeur = type_fonction->types_entrees[0];
            auto type_arg = type_pointeur->comme_type_pointeur()->type_pointe;
            empile_valeur(m_constructrice.trouve_ou_insère_fonction(type_arg->fonction_init));
            break;
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr = noeud->comme_taille_de();
            auto expr_type = expr->expression;
            auto constante = m_constructrice.crée_constante_taille_de(expr_type->type);
            empile_valeur(constante);
            break;
        }
        case GenreNoeud::EXPRESSION_MEMOIRE:
        {
            auto inst_mem = noeud->comme_memoire();
            génère_ri_pour_noeud(inst_mem->expression);
            auto valeur = depile_valeur();

            if (!expression_gauche) {
                // déréférence la locale
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);
                // déréférence le pointeur
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);
                crée_temporaire_ou_mets_dans_place(noeud, valeur, place);
                return;
            }

            // mémoire(*expr) = ...
            if (inst_mem->expression->genre_valeur == GenreValeur::DROITE &&
                !inst_mem->expression->est_comme()) {
                empile_valeur(valeur);
                return;
            }

            empile_valeur(m_constructrice.crée_charge_mem(noeud, valeur));
            break;
        }
        case GenreNoeud::EXPANSION_VARIADIQUE:
        {
            génère_ri_pour_expression_droite(noeud->comme_expansion_variadique()->expression,
                                             place);
            break;
        }
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            génère_ri_pour_tente(noeud->comme_tente());
            break;
        }
    }
}

void CompilatriceRI::génère_ri_pour_fonction(NoeudDeclarationEnteteFonction *decl)
{
    auto atome_fonc = m_constructrice.trouve_ou_insère_fonction(decl);

    if (decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        decl->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
        atome_fonc->drapeaux |= DrapeauxAtome::RI_FUT_GÉNÉRÉE;
        return;
    }

    définis_fonction_courante(atome_fonc);

    m_constructrice.crée_label(decl);

    génère_ri_pour_noeud(decl->corps->bloc);

    decl->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
    decl->corps->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
    m_fonction_courante->drapeaux |= DrapeauxAtome::RI_FUT_GÉNÉRÉE;

    if (decl->possède_drapeau(DrapeauxNoeudFonction::CLICHÉ_RI_FUT_REQUIS)) {
        dbg() << imprime_fonction(atome_fonc);
    }

    m_fonction_courante->instructions.rétrécis_capacité_sur_taille();

    assert_rappel(m_pile.taille() == 0, [&]() {
        dbg() << __func__ << " : " << m_pile.taille() << ", " << m_fonction_courante->nom;
        dbg() << imprime_fonction(m_fonction_courante);
        POUR (m_pile) {
            if (it->est_instruction()) {
                dbg() << "-- " << imprime_instruction(it->comme_instruction());
            }
            else {
                dbg() << "-- " << it->genre_atome;
            }
        }
    });

    définis_fonction_courante(nullptr);
}

void CompilatriceRI::génère_ri_pour_expression_droite(NoeudExpression const *noeud, Atome *place)
{
    auto ancienne_expression_gauche = expression_gauche;
    expression_gauche = false;

    assert(!place || !place->possède_drapeau(DrapeauxAtome::EST_UTILISÉ));

    génère_ri_pour_noeud(const_cast<NoeudExpression *>(noeud), place);
    expression_gauche = ancienne_expression_gauche;

    if (place) {
        if (!place->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
            auto atome = depile_valeur();
            atome = crée_charge_mem_si_chargeable(noeud, atome);
            m_constructrice.crée_stocke_mem(noeud, place, atome);
        }
    }
    else {
        auto atome = depile_valeur();
        atome = crée_charge_mem_si_chargeable(noeud, atome);
        empile_valeur(atome);
    }
}

void CompilatriceRI::génère_ri_transformee_pour_noeud(NoeudExpression const *noeud,
                                                      Atome *place,
                                                      TransformationType const &transformation)
{
    auto ancienne_expression_gauche = expression_gauche;
    expression_gauche = false;
    génère_ri_pour_noeud(const_cast<NoeudExpression *>(noeud));
    auto valeur = depile_valeur();
    expression_gauche = ancienne_expression_gauche;

    assert_rappel(valeur, [&] {
        dbg() << __func__ << ", valeur est nulle pour " << noeud->genre << '\n'
              << erreur::imprime_site(*m_espace, noeud);
    });

    transforme_valeur(noeud, valeur, transformation, place);
}

static TypeTranstypage donne_type_transtypage_pour_défaut(Type const *src, Type const *dst)
{
#ifndef NDEBUG
    auto const src_originale = src;
    auto const dst_originale = dst;
#endif

    if (src->est_type_entier_constant()) {
        /* Entier constant vers énum. */
        src = TypeBase::Z32;
    }

    if (src->taille_octet == dst->taille_octet) {
        /* Cas simple : les types ont les mêmes tailles, transtype la représentation binaire. */
        return TypeTranstypage::BITS;
    }

    if (src->est_type_enum()) {
        auto type_énum = src->comme_type_enum();
        src = type_énum->type_sous_jacent;
    }

    if (dst->est_type_enum()) {
        auto type_énum = dst->comme_type_enum();
        dst = type_énum->type_sous_jacent;
    }

    if (src->est_type_entier_naturel()) {
        if (dst->est_type_entier_naturel()) {
            if (src->taille_octet < dst->taille_octet) {
                return TypeTranstypage::AUGMENTE_NATUREL;
            }

            return TypeTranstypage::DIMINUE_NATUREL;
        }

        if (dst->est_type_entier_relatif()) {
            if (src->taille_octet < dst->taille_octet) {
                return TypeTranstypage::AUGMENTE_NATUREL_VERS_RELATIF;
            }

            return TypeTranstypage::DIMINUE_NATUREL_VERS_RELATIF;
        }

        if (dst->est_type_bool()) {
            return TypeTranstypage::DIMINUE_NATUREL;
        }
    }

    if (src->est_type_entier_relatif()) {
        if (dst->est_type_entier_relatif()) {
            if (src->taille_octet < dst->taille_octet) {
                return TypeTranstypage::AUGMENTE_RELATIF;
            }

            return TypeTranstypage::DIMINUE_RELATIF;
        }

        if (dst->est_type_entier_naturel()) {
            if (src->taille_octet < dst->taille_octet) {
                return TypeTranstypage::AUGMENTE_RELATIF_VERS_NATUREL;
            }

            return TypeTranstypage::DIMINUE_RELATIF_VERS_NATUREL;
        }

        if (dst->est_type_bool()) {
            return TypeTranstypage::DIMINUE_RELATIF;
        }
    }

    assert_rappel(false, [&]() {
        dbg() << "Type transtypage défaut non-géré : " << chaine_type(src_originale) << " -> "
              << chaine_type(dst_originale);
    });

    return TypeTranstypage::BITS;
}

void CompilatriceRI::transforme_valeur(NoeudExpression const *noeud,
                                       Atome *valeur,
                                       TransformationType const &transformation,
                                       Atome *place)
{
    auto transforme_avec_fonction = [this](NoeudExpression const *noeud_,
                                           Atome *valeur_,
                                           NoeudDeclarationEnteteFonction *fonction) {
        auto atome_fonction = m_constructrice.trouve_ou_insère_fonction(fonction);

        valeur_ = crée_charge_mem_si_chargeable(noeud_, valeur_);

        auto args = kuri::tableau<Atome *, int>();
        args.ajoute(valeur_);

        return m_constructrice.crée_appel(noeud_, atome_fonction, std::move(args));
    };

    assert(!place || !place->possède_drapeau(DrapeauxAtome::EST_UTILISÉ));

    switch (transformation.type) {
        case TypeTransformation::IMPOSSIBLE:
        {
            break;
        }
        case TypeTransformation::PREND_REFERENCE_ET_CONVERTIS_VERS_BASE:
        {
            assert_rappel(false, [&]() {
                dbg() << "PREND_REFERENCE_ET_CONVERTIS_VERS_BASE utilisée dans la RI !";
            });
            break;
        }
        case TypeTransformation::INUTILE:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);
            break;
        }
        case TypeTransformation::CONVERTI_ENTIER_CONSTANT:
        {
            // valeur est déjà une constante, change simplement le type
            if (est_valeur_constante(valeur)) {
                auto constante_entière = valeur->comme_constante_entière();
                auto valeur_entière = constante_entière->valeur;

                if (transformation.type_cible->est_type_reel()) {
                    auto valeur_réelle = static_cast<double>(valeur_entière);
                    valeur = m_constructrice.crée_constante_nombre_réel(transformation.type_cible,
                                                                        valeur_réelle);
                }
                else {
                    valeur = m_constructrice.crée_constante_nombre_entier(
                        transformation.type_cible, valeur_entière);
                }
            }
            // nous avons une temporaire créée lors d'une opération binaire
            else {
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);

                TypeTranstypage type_transtypage;

                if (transformation.type_cible->taille_octet > 4) {
                    type_transtypage = AUGMENTE_NATUREL;
                }
                else {
                    type_transtypage = DIMINUE_NATUREL;
                }

                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, type_transtypage);
            }

            assert_rappel(!valeur->type->est_type_entier_constant(), [=]() {
                dbg() << "Type de la valeur : " << chaine_type(valeur->type) << ".";
            });
            break;
        }
        case TypeTransformation::CONSTRUIT_UNION:
        {
            auto type_union = transformation.type_cible->comme_type_union();

            valeur = crée_temporaire_si_non_chargeable(noeud, valeur);

            auto alloc = place;
            if (!alloc) {
                alloc = m_constructrice.crée_allocation(noeud, type_union, nullptr);
            }

            if (type_union->est_nonsure) {
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);

                /* Transtype l'union vers le type cible pour garantir une sûreté de type et éviter
                 * les problèmes de surécriture si la valeur est transtypée mais pas du même genre
                 * que le type le plus grand de l'union. */
                auto dest = m_constructrice.crée_transtype(
                    noeud,
                    m_compilatrice.typeuse.type_pointeur_pour(const_cast<Type *>(valeur->type),
                                                              false),
                    alloc,
                    TypeTranstypage::BITS);

                m_constructrice.crée_stocke_mem(noeud, dest, valeur);
            }
            else {
                /* Pour les unions, nous transtypons le membre vers le type cible afin d'éviter les
                 * problème de surécriture de mémoire dans le cas où le type du membre est plus
                 * grand que le type de la valeur. */
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);

                auto acces_membre = m_constructrice.crée_référence_membre(noeud, alloc, 0);
                auto membre_transtype = m_constructrice.crée_transtype(
                    noeud,
                    m_compilatrice.typeuse.type_pointeur_pour(const_cast<Type *>(valeur->type),
                                                              false),
                    acces_membre,
                    TypeTranstypage::BITS);
                m_constructrice.crée_stocke_mem(noeud, membre_transtype, valeur);

                acces_membre = m_constructrice.crée_référence_membre(noeud, alloc, 1);
                auto index = m_constructrice.crée_constante_nombre_entier(
                    TypeBase::Z32, static_cast<uint64_t>(transformation.index_membre + 1));
                m_constructrice.crée_stocke_mem(noeud, acces_membre, index);
            }

            if (place == nullptr) {
                valeur = m_constructrice.crée_charge_mem(noeud, alloc);
            }
            else {
                place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
            }
            break;
        }
        case TypeTransformation::EXTRAIT_UNION:
        {
            auto type_union = noeud->type->comme_type_union();

            valeur = crée_temporaire_si_non_chargeable(noeud, valeur);

            if (!type_union->est_nonsure) {
                auto membre_actif = m_constructrice.crée_reference_membre_et_charge(
                    noeud, valeur, 1);

                auto label_si_vrai = m_constructrice.réserve_label(noeud);
                auto label_si_faux = m_constructrice.réserve_label(noeud);

                auto condition = m_constructrice.crée_op_comparaison(
                    noeud,
                    OpérateurBinaire::Genre::Comp_Inegal,
                    membre_actif,
                    m_constructrice.crée_z32(
                        static_cast<unsigned>(transformation.index_membre + 1)));

                m_constructrice.crée_branche_condition(
                    noeud, condition, label_si_vrai, label_si_faux);
                m_constructrice.insère_label(label_si_vrai);
                // À FAIRE : nous pourrions avoir une erreur différente ici.
                m_constructrice.crée_appel(
                    noeud,
                    m_constructrice.trouve_ou_insère_fonction(
                        m_compilatrice.interface_kuri->decl_panique_membre_union));
                m_constructrice.insère_label(label_si_faux);

                valeur = m_constructrice.crée_référence_membre(noeud, valeur, 0);
            }

            valeur = m_constructrice.crée_transtype(
                noeud,
                m_compilatrice.typeuse.type_pointeur_pour(
                    const_cast<Type *>(transformation.type_cible), false),
                valeur,
                TypeTranstypage::BITS);
            valeur = m_constructrice.crée_charge_mem(noeud, valeur);

            break;
        }
        case TypeTransformation::CONVERTI_VERS_PTR_RIEN:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);

            if (noeud->genre != GenreNoeud::EXPRESSION_LITTERALE_NUL) {
                valeur = m_constructrice.crée_transtype(
                    noeud, TypeBase::PTR_RIEN, valeur, TypeTranstypage::BITS);
            }

            break;
        }
        case TypeTransformation::CONVERTI_VERS_TYPE_CIBLE:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);

            auto type_valeur = valeur->type;
            auto type_cible = transformation.type_cible;
            auto type_transtypage = donne_type_transtypage_pour_défaut(type_valeur, type_cible);

            valeur = m_constructrice.crée_transtype(noeud, type_cible, valeur, type_transtypage);
            break;
        }
        case TypeTransformation::POINTEUR_VERS_ENTIER:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);
            valeur = m_constructrice.crée_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::POINTEUR_VERS_ENTIER);
            break;
        }
        case TypeTransformation::ENTIER_VERS_POINTEUR:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);

            /* Augmente la taille du type ici pour éviter de le faire dans les coulisses.
             * Nous ne pouvons le faire via l'arbre syntaxique car les arbres des expressions
             * d'assigations ou de retours ne peuvent être modifiés. */
            auto type = valeur->type;
            if (type->taille_octet != 8) {
                if (type->est_type_entier_naturel()) {
                    valeur = m_constructrice.crée_transtype(
                        noeud, TypeBase::N64, valeur, TypeTranstypage::AUGMENTE_NATUREL);
                }
                else {
                    valeur = m_constructrice.crée_transtype(
                        noeud, TypeBase::Z64, valeur, TypeTranstypage::AUGMENTE_RELATIF);
                }
            }

            valeur = m_constructrice.crée_transtype(
                noeud, transformation.type_cible, valeur, TypeTranstypage::ENTIER_VERS_POINTEUR);
            break;
        }
        case TypeTransformation::AUGMENTE_TAILLE_TYPE:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);

            if (noeud->type->est_type_reel()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_REEL);
            }
            else if (noeud->type->est_type_entier_naturel()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_NATUREL);
            }
            else if (noeud->type->est_type_entier_relatif()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_RELATIF);
            }
            else if (noeud->type->est_type_bool()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_NATUREL);
            }
            else if (noeud->type->est_type_octet()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::AUGMENTE_NATUREL);
            }

            break;
        }
        case TypeTransformation::ENTIER_VERS_REEL:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);

            auto type_transtypage = valeur->type->est_type_entier_naturel() ?
                                        TypeTranstypage::ENTIER_NATUREL_VERS_REEL :
                                        TypeTranstypage::ENTIER_RELATIF_VERS_REEL;

            valeur = m_constructrice.crée_transtype(
                noeud, transformation.type_cible, valeur, type_transtypage);
            break;
        }
        case TypeTransformation::REEL_VERS_ENTIER:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);

            auto type_transtypage = transformation.type_cible->est_type_entier_naturel() ?
                                        TypeTranstypage::REEL_VERS_ENTIER_NATUREL :
                                        TypeTranstypage::REEL_VERS_ENTIER_RELATIF;

            valeur = m_constructrice.crée_transtype(
                noeud, transformation.type_cible, valeur, type_transtypage);
            break;
        }
        case TypeTransformation::REDUIT_TAILLE_TYPE:
        {
            valeur = crée_charge_mem_si_chargeable(noeud, valeur);

            if (noeud->type->est_type_reel()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_REEL);
            }
            else if (noeud->type->est_type_entier_naturel()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_NATUREL);
            }
            else if (noeud->type->est_type_entier_relatif()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_RELATIF);
            }
            else if (noeud->type->est_type_bool()) {
                valeur = m_constructrice.crée_transtype(
                    noeud, transformation.type_cible, valeur, TypeTranstypage::DIMINUE_NATUREL);
            }

            break;
        }
        case TypeTransformation::CONSTRUIT_EINI:
        {
            auto alloc_eini = place;

            if (alloc_eini == nullptr) {
                auto type_eini = TypeBase::EINI;
                alloc_eini = m_constructrice.crée_allocation(noeud, type_eini, nullptr);
            }

            /* copie le pointeur de la valeur vers le type eini */
            auto ptr_eini = m_constructrice.crée_référence_membre(noeud, alloc_eini, 0);

            if (valeur->type->est_type_entier_constant()) {
                valeur->type = TypeBase::Z32;
            }

            valeur = crée_temporaire_si_non_chargeable(noeud, valeur);

            auto transtype = m_constructrice.crée_transtype(
                noeud, TypeBase::PTR_RIEN, valeur, TypeTranstypage::BITS);
            m_constructrice.crée_stocke_mem(noeud, ptr_eini, transtype);

            /* copie le pointeur vers les infos du type du eini */
            auto tpe_eini = m_constructrice.crée_référence_membre(noeud, alloc_eini, 1);
            auto info_type = crée_info_type_avec_transtype(noeud->type, noeud);
            m_constructrice.crée_stocke_mem(noeud, tpe_eini, info_type);

            if (place == nullptr) {
                valeur = m_constructrice.crée_charge_mem(noeud, alloc_eini);
            }
            else {
                place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
            }

            break;
        }
        case TypeTransformation::EXTRAIT_EINI:
        {
            valeur = m_constructrice.crée_référence_membre(noeud, valeur, 0);

            /* eini.pointeur est une adresse vers une adresse, donc nous avons deux niveaux de
             * pointeur. */
            auto type_cible = m_compilatrice.typeuse.type_pointeur_pour(
                const_cast<Type *>(transformation.type_cible), false);
            type_cible = m_compilatrice.typeuse.type_pointeur_pour(type_cible, false);

            valeur = m_constructrice.crée_transtype(
                noeud, type_cible, valeur, TypeTranstypage::BITS);

            /* Déréférence eini.pointeur pour obtenir l'adresse de la valeur. */
            valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            /* Charge la valeur depuis son adresse. */
            valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            break;
        }
        case TypeTransformation::CONSTRUIT_TABL_OCTET:
        {
            auto valeur_pointeur = static_cast<Atome *>(nullptr);
            auto valeur_taille = static_cast<Atome *>(nullptr);

            auto type_cible = TypeBase::PTR_OCTET;

            switch (noeud->type->genre) {
                default:
                {
                    if (valeur->est_constante()) {
                        valeur = crée_temporaire(noeud, valeur);
                    }

                    valeur = m_constructrice.crée_transtype(
                        noeud, type_cible, valeur, TypeTranstypage::BITS);
                    valeur_pointeur = valeur;

                    if (noeud->type->est_type_entier_constant()) {
                        valeur_taille = m_constructrice.crée_z64(4);
                    }
                    else {
                        valeur_taille = m_constructrice.crée_z64(noeud->type->taille_octet);
                    }

                    break;
                }
                case GenreNoeud::POINTEUR:
                {
                    auto type_pointe = noeud->type->comme_type_pointeur()->type_pointe;
                    valeur = m_constructrice.crée_transtype(
                        noeud, type_cible, valeur, TypeTranstypage::BITS);
                    valeur_pointeur = valeur;
                    auto taille_type = type_pointe->taille_octet;
                    valeur_taille = m_constructrice.crée_z64(taille_type);
                    break;
                }
                case GenreNoeud::CHAINE:
                {
                    valeur_pointeur = m_constructrice.crée_reference_membre_et_charge(
                        noeud, valeur, 0);
                    valeur_pointeur = m_constructrice.crée_transtype(
                        noeud, type_cible, valeur_pointeur, TypeTranstypage::BITS);
                    valeur_taille = m_constructrice.crée_reference_membre_et_charge(
                        noeud, valeur, 1);
                    break;
                }
                case GenreNoeud::TABLEAU_DYNAMIQUE:
                {
                    auto type_pointer = noeud->type->comme_type_tableau_dynamique()->type_pointe;

                    valeur_pointeur = m_constructrice.crée_reference_membre_et_charge(
                        noeud, valeur, 0);
                    valeur_pointeur = m_constructrice.crée_transtype(
                        noeud, type_cible, valeur_pointeur, TypeTranstypage::BITS);
                    valeur_taille = m_constructrice.crée_reference_membre_et_charge(
                        noeud, valeur, 1);

                    auto taille_type = type_pointer->taille_octet;

                    valeur_taille = m_constructrice.crée_op_binaire(
                        noeud,
                        TypeBase::Z64,
                        OpérateurBinaire::Genre::Multiplication,
                        valeur_taille,
                        m_constructrice.crée_z64(taille_type));

                    break;
                }
                case GenreNoeud::TABLEAU_FIXE:
                {
                    auto type_tabl = noeud->type->comme_type_tableau_fixe();
                    auto type_pointe = type_tabl->type_pointe;
                    auto taille_type = type_pointe->taille_octet;

                    valeur_pointeur = m_constructrice.crée_accès_index(
                        noeud, valeur, m_constructrice.crée_z64(0ul));
                    valeur_pointeur = m_constructrice.crée_transtype(
                        noeud, type_cible, valeur_pointeur, TypeTranstypage::BITS);
                    valeur_taille = m_constructrice.crée_z64(
                        static_cast<unsigned>(type_tabl->taille) * taille_type);

                    break;
                }
            }

            auto tabl_octet = place;
            if (!tabl_octet) {
                tabl_octet = m_constructrice.crée_allocation(noeud, TypeBase::TABL_OCTET, nullptr);
            }

            auto pointeur_tabl_octet = m_constructrice.crée_référence_membre(noeud, tabl_octet, 0);
            m_constructrice.crée_stocke_mem(noeud, pointeur_tabl_octet, valeur_pointeur);

            auto taille_tabl_octet = m_constructrice.crée_référence_membre(noeud, tabl_octet, 1);
            m_constructrice.crée_stocke_mem(noeud, taille_tabl_octet, valeur_taille);

            if (!place) {
                valeur = m_constructrice.crée_charge_mem(noeud, tabl_octet);
            }
            else {
                place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
            }

            break;
        }
        case TypeTransformation::CONVERTI_TABLEAU:
        {
            if (m_fonction_courante == nullptr) {
                auto valeur_tableau_fixe = static_cast<AtomeConstante *>(valeur);
                auto ident = m_compilatrice.donne_identifiant_pour_globale("tableau_convertis");
                empile_valeur(m_constructrice.crée_tableau_global(*ident, valeur_tableau_fixe));
                return;
            }

            valeur = converti_vers_tableau_dyn(
                noeud, valeur, noeud->type->comme_type_tableau_fixe(), place);

            if (place == nullptr) {
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            }
            else {
                place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
            }

            break;
        }
        case TypeTransformation::R16_VERS_R32:
        {
            valeur = transforme_avec_fonction(
                noeud, valeur, m_compilatrice.interface_kuri->decl_dls_vers_r32);
            break;
        }
        case TypeTransformation::R16_VERS_R64:
        {
            valeur = transforme_avec_fonction(
                noeud, valeur, m_compilatrice.interface_kuri->decl_dls_vers_r64);
            break;
        }
        case TypeTransformation::R32_VERS_R16:
        {
            valeur = transforme_avec_fonction(
                noeud, valeur, m_compilatrice.interface_kuri->decl_dls_depuis_r32);
            break;
        }
        case TypeTransformation::R64_VERS_R16:
        {
            valeur = transforme_avec_fonction(
                noeud, valeur, m_compilatrice.interface_kuri->decl_dls_depuis_r64);
            break;
        }
        case TypeTransformation::PREND_REFERENCE:
        {
            // RÀF : valeur doit déjà être un pointeur
            break;
        }
        case TypeTransformation::DEREFERENCE:
        {
            valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            break;
        }
        case TypeTransformation::CONVERTI_VERS_BASE:
        {
            valeur = crée_transtype_entre_base_et_dérivé(
                noeud, valeur, transformation, OpérateurBinaire::Genre::Addition);
            break;
        }
        case TypeTransformation::CONVERTI_VERS_DÉRIVÉ:
        {
            valeur = crée_transtype_entre_base_et_dérivé(
                noeud, valeur, transformation, OpérateurBinaire::Genre::Soustraction);
            break;
        }
    }

    if (place) {
        if (!place->possède_drapeau(DrapeauxAtome::EST_UTILISÉ)) {
            m_constructrice.crée_stocke_mem(noeud, place, valeur);
        }
    }
    else {
        empile_valeur(valeur);
    }
}

Atome *CompilatriceRI::crée_transtype_entre_base_et_dérivé(
    NoeudExpression const *noeud,
    Atome *valeur,
    TransformationType const &transformation,
    OpérateurBinaire::Genre op)
{
    valeur = m_constructrice.crée_charge_mem(noeud, valeur);

    auto const décalage_type_base = transformation.décalage_type_base;
    if (décalage_type_base != 0) {
        auto const type_z64 = TypeBase::Z64;
        auto const type_ptr_octet = TypeBase::PTR_OCTET;

        /* Convertis en entier. */
        valeur = m_constructrice.crée_transtype(
            noeud, type_z64, valeur, TypeTranstypage::POINTEUR_VERS_ENTIER);

        /* Ajoute ou soustrait le décalage selon l'opération. */
        auto valeur_décalage = m_constructrice.crée_z64(uint64_t(décalage_type_base));
        valeur = m_constructrice.crée_op_binaire(noeud, type_z64, op, valeur, valeur_décalage);

        /* Convertis vers un pointeur. */
        valeur = m_constructrice.crée_transtype(
            noeud, type_ptr_octet, valeur, TypeTranstypage::ENTIER_VERS_POINTEUR);
    }

    return m_constructrice.crée_transtype(
        noeud, transformation.type_cible, valeur, TypeTranstypage::BITS);
}

void CompilatriceRI::génère_ri_pour_tente(NoeudInstructionTente const *noeud)
{
    // À FAIRE(retours multiples)
    génère_ri_pour_expression_droite(noeud->expression_appelee, nullptr);
    auto valeur_expression = depile_valeur();

    struct DonneesGenerationCodeTente {
        Atome *acces_erreur{};
        Atome *acces_erreur_pour_test{};

        Type const *type_piege = nullptr;
        Type const *type_variable = nullptr;
    };

    if (noeud->expression_appelee->type->est_type_erreur()) {

        DonneesGenerationCodeTente gen_tente;
        gen_tente.type_piege = noeud->expression_appelee->type;
        gen_tente.acces_erreur = valeur_expression;
        gen_tente.acces_erreur_pour_test = gen_tente.acces_erreur;

        auto label_si_vrai = m_constructrice.réserve_label(noeud);
        auto label_si_faux = m_constructrice.réserve_label(noeud);

        auto condition = m_constructrice.crée_op_comparaison(
            noeud,
            OpérateurBinaire::Genre::Comp_Inegal,
            gen_tente.acces_erreur_pour_test,
            m_constructrice.crée_constante_nombre_entier(noeud->expression_appelee->type, 0));

        m_constructrice.crée_branche_condition(noeud, condition, label_si_vrai, label_si_faux);

        m_constructrice.insère_label(label_si_vrai);
        if (noeud->expression_piegee == nullptr) {
            m_constructrice.crée_appel(noeud,
                                       m_constructrice.trouve_ou_insère_fonction(
                                           m_compilatrice.interface_kuri->decl_panique_erreur));
        }
        else {
            auto var_expr_piegee = m_constructrice.crée_allocation(
                noeud, gen_tente.type_piege, noeud->expression_piegee->ident);
            auto decl_expr_piegee =
                noeud->expression_piegee->comme_reference_declaration()->declaration_referee;
            static_cast<NoeudDeclarationSymbole *>(decl_expr_piegee)->atome = var_expr_piegee;

            m_constructrice.crée_stocke_mem(
                noeud->expression_piegee, var_expr_piegee, valeur_expression);

            génère_ri_pour_noeud(noeud->bloc);
        }

        m_constructrice.insère_label(label_si_faux);

        if (noeud->possède_drapeau(DrapeauxNoeud::DROITE_ASSIGNATION)) {
            empile_valeur(valeur_expression);
        }
        return;
    }
    else if (noeud->expression_appelee->type->est_type_union()) {
        DonneesGenerationCodeTente gen_tente;
        auto type_union = noeud->expression_appelee->type->comme_type_union();
        auto index_membre_erreur = 0;

        if (type_union->membres.taille() == 2) {
            if (type_union->membres[0].type->est_type_erreur()) {
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
        auto label_si_vrai = m_constructrice.réserve_label(noeud);
        auto label_si_faux = m_constructrice.réserve_label(noeud);

        auto valeur_union = crée_temporaire(noeud, valeur_expression);
        auto acces_membre_actif = m_constructrice.crée_reference_membre_et_charge(
            noeud, valeur_union, 1);

        auto condition_membre_actif = m_constructrice.crée_op_comparaison(
            noeud,
            OpérateurBinaire::Genre::Comp_Egal,
            acces_membre_actif,
            m_constructrice.crée_z32(static_cast<unsigned>(index_membre_erreur + 1)));

        m_constructrice.crée_branche_condition(
            noeud, condition_membre_actif, label_si_vrai, label_si_faux);

        m_constructrice.insère_label(label_si_vrai);
        if (noeud->expression_piegee == nullptr) {
            m_constructrice.crée_appel(noeud,
                                       m_constructrice.trouve_ou_insère_fonction(
                                           m_compilatrice.interface_kuri->decl_panique_erreur));
        }
        else {
            Instruction *membre_erreur = m_constructrice.crée_référence_membre(
                noeud, valeur_union, 0);
            membre_erreur = m_constructrice.crée_transtype(
                noeud,
                m_compilatrice.typeuse.type_pointeur_pour(const_cast<Type *>(gen_tente.type_piege),
                                                          false),
                membre_erreur,
                TypeTranstypage::BITS);
            auto decl_expr_piegee =
                noeud->expression_piegee->comme_reference_declaration()->declaration_referee;
            static_cast<NoeudDeclarationSymbole *>(decl_expr_piegee)->atome = membre_erreur;
            génère_ri_pour_noeud(noeud->bloc);
        }

        m_constructrice.insère_label(label_si_faux);

        if (gen_tente.type_variable->est_type_rien()) {
            return;
        }

        valeur_expression = m_constructrice.crée_référence_membre(noeud, valeur_union, 0);
        valeur_expression = m_constructrice.crée_transtype(
            noeud,
            m_compilatrice.typeuse.type_pointeur_pour(const_cast<Type *>(gen_tente.type_variable),
                                                      false),
            valeur_expression,
            TypeTranstypage::BITS);
    }

    empile_valeur(valeur_expression);
}

void CompilatriceRI::empile_valeur(Atome *valeur)
{
    m_pile.ajoute(valeur);
}

Atome *CompilatriceRI::depile_valeur()
{
    auto v = m_pile.back();
    m_pile.pop_back();
    return v;
}

void CompilatriceRI::génère_ri_pour_accès_membre(NoeudExpressionMembre const *noeud)
{
    auto accede = noeud->accedee;
    auto type_accede = accede->type;

    génère_ri_pour_noeud(accede);
    auto pointeur_accede = depile_valeur();

    while (type_accede->est_type_pointeur() || type_accede->est_type_reference()) {
        type_accede = type_dereference_pour(type_accede);
        pointeur_accede = m_constructrice.crée_charge_mem(noeud, pointeur_accede);
    }

    if (pointeur_accede->est_constante_structure()) {
        auto initialisateur = pointeur_accede->comme_constante_structure();
        auto valeur = initialisateur->donne_atomes_membres()[noeud->index_membre];
        empile_valeur(valeur);
        return;
    }

    empile_valeur(
        m_constructrice.crée_référence_membre(noeud, pointeur_accede, noeud->index_membre));
}

void CompilatriceRI::génère_ri_pour_accès_membre_union(NoeudExpressionMembre const *noeud)
{
    génère_ri_pour_noeud(noeud->accedee);
    auto ptr_union = depile_valeur();
    auto type = noeud->accedee->type;

    while (type->est_type_pointeur() || type->est_type_reference()) {
        type = type_dereference_pour(type);
        ptr_union = m_constructrice.crée_charge_mem(noeud, ptr_union);
    }

    auto type_union = type->comme_type_union();
    auto index_membre = noeud->index_membre;
    auto type_membre = type_union->membres[index_membre].type;

    if (type_union->est_nonsure) {
        ptr_union = m_constructrice.crée_transtype(
            noeud,
            m_compilatrice.typeuse.type_pointeur_pour(type_membre, false),
            ptr_union,
            TypeTranstypage::BITS);
        empile_valeur(ptr_union);
        return;
    }

    if (expression_gauche) {
        // ajourne l'index du membre
        auto membre_actif = m_constructrice.crée_référence_membre(noeud, ptr_union, 1);
        m_constructrice.crée_stocke_mem(
            noeud,
            membre_actif,
            m_constructrice.crée_z32(static_cast<unsigned>(noeud->index_membre + 1)));
    }
    else {
        // vérifie l'index du membre
        auto membre_actif = m_constructrice.crée_reference_membre_et_charge(noeud, ptr_union, 1);

        auto label_si_vrai = m_constructrice.réserve_label(noeud);
        auto label_si_faux = m_constructrice.réserve_label(noeud);

        auto condition = m_constructrice.crée_op_comparaison(
            noeud,
            OpérateurBinaire::Genre::Comp_Inegal,
            membre_actif,
            m_constructrice.crée_z32(static_cast<unsigned>(noeud->index_membre + 1)));

        m_constructrice.crée_branche_condition(noeud, condition, label_si_vrai, label_si_faux);
        m_constructrice.insère_label(label_si_vrai);
        m_constructrice.crée_appel(noeud,
                                   m_constructrice.trouve_ou_insère_fonction(
                                       m_compilatrice.interface_kuri->decl_panique_membre_union));
        m_constructrice.insère_label(label_si_faux);
    }

    Instruction *pointeur_membre = m_constructrice.crée_référence_membre(noeud, ptr_union, 0);

    if (type_membre != type_union->type_le_plus_grand) {
        pointeur_membre = m_constructrice.crée_transtype(
            noeud,
            m_compilatrice.typeuse.type_pointeur_pour(type_membre, false),
            pointeur_membre,
            TypeTranstypage::BITS);
    }

    empile_valeur(pointeur_membre);
}

// Logique tirée de « Basics of Compiler Design », Torben Ægidius Mogensen
void CompilatriceRI::génère_ri_pour_condition(NoeudExpression const *condition,
                                              InstructionLabel *label_si_vrai,
                                              InstructionLabel *label_si_faux)
{
    auto genre_lexeme = condition->lexeme->genre;

    if (est_opérateur_comparaison(genre_lexeme) ||
        condition->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
        génère_ri_pour_expression_droite(condition, nullptr);
        auto valeur = depile_valeur();
        m_constructrice.crée_branche_condition(condition, valeur, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexeme::ESP_ESP) {
        auto expr_bin = condition->comme_expression_logique();
        auto cond1 = expr_bin->opérande_gauche;
        auto cond2 = expr_bin->opérande_droite;

        auto nouveau_label = m_constructrice.réserve_label(condition);
        génère_ri_pour_condition(cond1, nouveau_label, label_si_faux);
        m_constructrice.insère_label(nouveau_label);
        génère_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexeme::BARRE_BARRE) {
        auto expr_bin = condition->comme_expression_logique();
        auto cond1 = expr_bin->opérande_gauche;
        auto cond2 = expr_bin->opérande_droite;

        auto nouveau_label = m_constructrice.réserve_label(condition);
        génère_ri_pour_condition(cond1, label_si_vrai, nouveau_label);
        m_constructrice.insère_label(nouveau_label);
        génère_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexeme::EXCLAMATION) {
        auto expr_unaire = condition->comme_negation_logique();
        génère_ri_pour_condition(expr_unaire->opérande, label_si_faux, label_si_vrai);
    }
    else if (genre_lexeme == GenreLexeme::VRAI) {
        m_constructrice.crée_branche(condition, label_si_vrai);
    }
    else if (genre_lexeme == GenreLexeme::FAUX) {
        m_constructrice.crée_branche(condition, label_si_faux);
    }
    else if (condition->genre == GenreNoeud::EXPRESSION_PARENTHESE) {
        auto expr_unaire = condition->comme_parenthese();
        génère_ri_pour_condition(expr_unaire->expression, label_si_vrai, label_si_faux);
    }
    else {
        génère_ri_pour_condition_implicite(condition, label_si_vrai, label_si_faux);
    }
}

void CompilatriceRI::génère_ri_pour_condition_implicite(NoeudExpression const *condition,
                                                        InstructionLabel *label_si_vrai,
                                                        InstructionLabel *label_si_faux)
{
    auto type_condition = condition->type;
    auto valeur = static_cast<Atome *>(nullptr);

    if (type_condition->est_type_opaque()) {
        type_condition = type_condition->comme_type_opaque()->type_opacifie;
    }

    switch (type_condition->genre) {
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::ENTIER_CONSTANT:
        {
            génère_ri_pour_expression_droite(condition, nullptr);
            auto valeur1 = depile_valeur();
            auto valeur2 = m_constructrice.crée_constante_nombre_entier(type_condition, 0);
            valeur = m_constructrice.crée_op_comparaison(
                condition, OpérateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
            break;
        }
        case GenreNoeud::BOOL:
        {
            génère_ri_pour_expression_droite(condition, nullptr);
            valeur = depile_valeur();
            break;
        }
        case GenreNoeud::FONCTION:
        case GenreNoeud::POINTEUR:
        {
            génère_ri_pour_expression_droite(condition, nullptr);
            auto valeur1 = depile_valeur();
            auto valeur2 = m_constructrice.crée_constante_nulle(type_condition);
            valeur = m_constructrice.crée_op_comparaison(
                condition, OpérateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
            break;
        }
        case GenreNoeud::EINI:
        {
            génère_ri_pour_noeud(const_cast<NoeudExpression *>(condition));
            auto pointeur = depile_valeur();
            auto pointeur_pointeur = m_constructrice.crée_référence_membre(condition, pointeur, 0);
            auto valeur1 = m_constructrice.crée_charge_mem(condition, pointeur_pointeur);
            auto valeur2 = m_constructrice.crée_constante_nulle(valeur1->type);
            valeur = m_constructrice.crée_op_comparaison(
                condition, OpérateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
            break;
        }
        case GenreNoeud::CHAINE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            génère_ri_pour_noeud(const_cast<NoeudExpression *>(condition));
            auto pointeur = depile_valeur();
            auto pointeur_taille = m_constructrice.crée_référence_membre(condition, pointeur, 1);
            auto valeur1 = m_constructrice.crée_charge_mem(condition, pointeur_taille);
            auto valeur2 = m_constructrice.crée_z64(0);
            valeur = m_constructrice.crée_op_comparaison(
                condition, OpérateurBinaire::Genre::Comp_Inegal, valeur1, valeur2);
            break;
        }
        default:
        {
            assert_rappel(false, [&]() {
                dbg() << "Type non géré pour la génération d'une condition d'une branche : "
                      << chaine_type(type_condition);
            });
            break;
        }
    }

    m_constructrice.crée_branche_condition(condition, valeur, label_si_vrai, label_si_faux);
}

void CompilatriceRI::génère_ri_pour_expression_logique(NoeudExpressionLogique const *noeud,
                                                       Atome *place)
{
    auto label_si_vrai = m_constructrice.réserve_label(noeud);
    auto label_si_faux = m_constructrice.réserve_label(noeud);
    auto label_apres_faux = m_constructrice.réserve_label(noeud);

    auto destination = place;
    if (destination == nullptr) {
        destination = m_constructrice.crée_allocation(noeud, TypeBase::BOOL, nullptr);
    }

    génère_ri_pour_condition(noeud, label_si_vrai, label_si_faux);

    m_constructrice.insère_label(label_si_vrai);
    m_constructrice.crée_stocke_mem(
        noeud, destination, m_constructrice.crée_constante_booléenne(true));
    m_constructrice.crée_branche(noeud, label_apres_faux);

    m_constructrice.insère_label(label_si_faux);
    m_constructrice.crée_stocke_mem(
        noeud, destination, m_constructrice.crée_constante_booléenne(false));

    m_constructrice.insère_label(label_apres_faux);

    if (destination != place) {
        empile_valeur(destination);
    }
    else {
        place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
    }
}

void CompilatriceRI::génère_ri_insts_différées(NoeudBloc const *bloc_final)
{
#if 0
	if (compilatrice.donnees_fonction->est_coroutine) {
		constructrice << "__etat->__termine_coro = 1;\n";
		constructrice << "pthread_mutex_lock(&__etat->mutex_boucle);\n";
		constructrice << "pthread_cond_signal(&__etat->cond_boucle);\n";
		constructrice << "pthread_mutex_unlock(&__etat->mutex_boucle);\n";
	}
#endif

    if (m_est_dans_diffère) {
        return;
    }

    m_est_dans_diffère = true;

    for (auto i = m_instructions_diffères.taille() - 1; i >= 0; i--) {
        auto expr = m_instructions_diffères[i];
        if (expr == bloc_final) {
            break;
        }

        if (expr->est_bloc()) {
            continue;
        }
        auto instruction_differee = expr->comme_differe();
        génère_ri_pour_noeud(instruction_differee->expression);
    }

    m_est_dans_diffère = false;
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
    static constexpr unsigned VARIADIQUE = 15;
};

AtomeConstante *CompilatriceRI::crée_tableau_annotations_pour_info_membre(
    kuri::tableau<Annotation, int> const &annotations)
{
    if (annotations.est_vide() && m_globale_annotations_vides) {
        return m_globale_annotations_vides;
    }

    kuri::tableau<AtomeConstante *> valeurs_annotations;
    valeurs_annotations.reserve(annotations.taille());

    auto type_annotation = m_compilatrice.typeuse.type_annotation;
    auto type_pointeur_annotation = m_compilatrice.typeuse.type_pointeur_pour(type_annotation);

    POUR (annotations) {
        auto annotation_existante = m_registre_annotations.trouve_globale_pour_annotation(it);
        if (annotation_existante) {
            valeurs_annotations.ajoute(annotation_existante);
            continue;
        }

        kuri::tableau<AtomeConstante *> valeurs(2);
        valeurs[0] = crée_chaine(it.nom);
        valeurs[1] = crée_chaine(it.valeur);

        auto ident = m_compilatrice.donne_identifiant_pour_globale("annotations");

        auto initialisateur = m_constructrice.crée_constante_structure(type_annotation,
                                                                       std::move(valeurs));
        auto valeur = m_constructrice.crée_globale(
            *ident, type_annotation, initialisateur, false, true);

        valeurs_annotations.ajoute(valeur);
        m_registre_annotations.ajoute_annotation(it, valeur);
    }

    auto ident = m_compilatrice.donne_identifiant_pour_globale("tableau_annotations");
    auto résultat = m_constructrice.crée_tableau_global(
        *ident, type_pointeur_annotation, std::move(valeurs_annotations));
    if (annotations.est_vide()) {
        m_globale_annotations_vides = résultat;
    }
    return résultat;
}

AtomeGlobale *CompilatriceRI::crée_info_type(Type const *type, NoeudExpression const *site)
{
    if (type->atome_info_type != nullptr) {
        return type->atome_info_type;
    }

    switch (type->genre) {
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::TUPLE:
        {
            assert_rappel(false, []() { dbg() << "Obtenu un type tuple ou polymophique"; });
            break;
        }
        case GenreNoeud::BOOL:
        {
            type->atome_info_type = crée_info_type_défaut(IDInfoType::BOOLEEN, type);
            break;
        }
        case GenreNoeud::OCTET:
        {
            type->atome_info_type = crée_info_type_défaut(IDInfoType::OCTET, type);
            break;
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            auto type_z32 = TypeBase::Z32;
            if (type_z32->atome_info_type) {
                type->atome_info_type = type_z32->atome_info_type;
            }
            else {
                type->atome_info_type = crée_info_type_entier(type_z32, true);
                type_z32->atome_info_type = type->atome_info_type;
            }
            break;
        }
        case GenreNoeud::ENTIER_NATUREL:
        {
            type->atome_info_type = crée_info_type_entier(type, false);
            break;
        }
        case GenreNoeud::ENTIER_RELATIF:
        {
            auto type_z32 = TypeBase::Z32;

            if (type != type_z32) {
                type->atome_info_type = crée_info_type_entier(type, true);
            }
            else {
                auto type_entier_constant = TypeBase::ENTIER_CONSTANT;
                if (type_entier_constant->atome_info_type) {
                    type->atome_info_type = type_entier_constant->atome_info_type;
                }
                else {
                    type->atome_info_type = crée_info_type_entier(type, true);
                    type_entier_constant->atome_info_type = type->atome_info_type;
                }
            }
            break;
        }
        case GenreNoeud::REEL:
        {
            type->atome_info_type = crée_info_type_défaut(IDInfoType::REEL, type);
            break;
        }
        case GenreNoeud::REFERENCE:
        case GenreNoeud::POINTEUR:
        {
            auto type_deref = type_dereference_pour(type);

            /* { membres basiques, type_pointé, est_référence } */
            auto valeurs = kuri::tableau<AtomeConstante *>(3);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::POINTEUR, type);
            if (type_deref) {
                valeurs[1] = crée_info_type_avec_transtype(type_deref, site);
            }
            else {
                auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                    m_compilatrice.typeuse.type_info_type_, false);
                valeurs[1] = m_constructrice.crée_constante_nulle(type_pointeur_info_type);
            }
            valeurs[2] = m_constructrice.crée_constante_booléenne(type->est_type_reference());

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_pointeur, std::move(valeurs));
            break;
        }
        case GenreNoeud::DECLARATION_ENUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);

            /* Les valeurs sont convertis en un tableau de données constantes. */
            int nombre_de_membres_non_implicite = 0;
            POUR (type_enum->membres) {
                if (it.drapeaux == MembreTypeComposé::EST_IMPLICITE) {
                    continue;
                }

                nombre_de_membres_non_implicite += 1;
            }

            kuri::tableau<char> tampon_valeurs_énum(nombre_de_membres_non_implicite * 4);
            auto pointeur_tampon = reinterpret_cast<int *>(&tampon_valeurs_énum[0]);

            POUR (type_enum->membres) {
                if (it.drapeaux == MembreTypeComposé::EST_IMPLICITE) {
                    continue;
                }

                *pointeur_tampon++ = it.valeur;
            }

            auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(
                TypeBase::Z32, nombre_de_membres_non_implicite);
            auto tableau = m_constructrice.crée_constante_tableau_données_constantes(
                type_tableau, std::move(tampon_valeurs_énum));

            kuri::tableau<AtomeConstante *> noms_enum;
            noms_enum.reserve(type_enum->membres.taille());

            POUR (type_enum->membres) {
                if (it.drapeaux == MembreTypeComposé::EST_IMPLICITE) {
                    continue;
                }

                auto chaine_nom = crée_chaine(it.nom->nom);
                noms_enum.ajoute(chaine_nom);
            }

            auto ident_valeurs = m_compilatrice.donne_identifiant_pour_globale("valeurs_énums");
            auto ident_noms = m_compilatrice.donne_identifiant_pour_globale("noms_valeus_énums");
            auto tableau_valeurs = m_constructrice.crée_tableau_global(*ident_valeurs, tableau);
            auto tableau_noms = m_constructrice.crée_tableau_global(
                *ident_noms, TypeBase::CHAINE, std::move(noms_enum));

            /* création de l'info type */

            /* { membres basiques, nom, valeurs, membres, est_drapeau } */
            auto valeurs = kuri::tableau<AtomeConstante *>(6);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::ENUM, type);
            valeurs[1] = crée_chaine(donne_nom_hiérarchique(const_cast<TypeEnum *>(type_enum)));
            valeurs[2] = tableau_valeurs;
            valeurs[3] = tableau_noms;
            valeurs[4] = m_constructrice.crée_constante_booléenne(
                type_enum->est_type_enum_drapeau());
            valeurs[5] = crée_info_type(type_enum->type_sous_jacent, site);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_enum, std::move(valeurs));
            break;
        }
        case GenreNoeud::DECLARATION_UNION:
        {
            auto type_union = type->comme_type_union();

            // ------------------------------------
            // Commence par assigner une globale non-initialisée comme info type
            // pour éviter de recréer plusieurs fois le même info type.
            auto type_info_union = m_compilatrice.typeuse.type_info_type_union;

            auto ident = m_compilatrice.donne_identifiant_pour_globale("info_type");
            auto globale = m_constructrice.crée_globale(
                *ident, type_info_union, nullptr, false, true);
            type->atome_info_type = globale;

            // ------------------------------------
            /* pour chaque membre cree une instance de InfoTypeMembreStructure */
            auto type_struct_membre = m_compilatrice.typeuse.type_info_type_membre_structure;

            kuri::tableau<AtomeConstante *> valeurs_membres;
            valeurs_membres.reserve(type_union->membres.taille());

            POUR (type_union->membres) {
                /* { nom: chaine, info : *InfoType, décalage, drapeaux } */
                auto valeurs = kuri::tableau<AtomeConstante *>(5);
                auto globale_membre = crée_info_type_membre_structure(it, site);
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
            auto info_type_plus_grand = crée_info_type_avec_transtype(
                type_union->type_le_plus_grand, site);

            // Pour les références à des globales, nous devons avoir un type pointeur.
            auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(type_struct_membre,
                                                                         false);

            auto ident_valeurs = m_compilatrice.donne_identifiant_pour_globale(
                "membres_info_type");
            auto tableau_membre = m_constructrice.crée_tableau_global(
                *ident_valeurs, type_membre, std::move(valeurs_membres));

            auto valeurs = kuri::tableau<AtomeConstante *>(7);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::UNION, type);
            valeurs[1] = crée_chaine(donne_nom_hiérarchique(const_cast<TypeUnion *>(type_union)));
            valeurs[2] = tableau_membre;
            valeurs[3] = info_type_plus_grand;
            valeurs[4] = m_constructrice.crée_z64(type_union->decalage_index);
            valeurs[5] = m_constructrice.crée_constante_booléenne(!type_union->est_nonsure);
            valeurs[6] = crée_tableau_annotations_pour_info_membre(type_union->annotations);

            globale->initialisateur = m_constructrice.crée_constante_structure(type_info_union,
                                                                               std::move(valeurs));

            break;
        }
        case GenreNoeud::DECLARATION_STRUCTURE:
        {
            auto type_struct = type->comme_type_structure();

            // ------------------------------------
            // Commence par assigner une globale non-initialisée comme info type
            // pour éviter de recréer plusieurs fois le même info type.
            auto type_info_struct = m_compilatrice.typeuse.type_info_type_structure;

            auto ident = m_compilatrice.donne_identifiant_pour_globale("info_type");
            auto globale = m_constructrice.crée_globale(
                *ident, type_info_struct, nullptr, false, true);
            type->atome_info_type = globale;

            // ------------------------------------
            /* pour chaque membre cree une instance de InfoTypeMembreStructure */
            auto type_struct_membre = m_compilatrice.typeuse.type_info_type_membre_structure;

            kuri::tableau<AtomeConstante *> valeurs_membres;

            POUR (type_struct->membres) {
                if (it.nom == ID::chaine_vide) {
                    continue;
                }

                if (it.possède_drapeau(MembreTypeComposé::PROVIENT_D_UN_EMPOI)) {
                    continue;
                }

                auto globale_membre = crée_info_type_membre_structure(it, site);
                valeurs_membres.ajoute(globale_membre);
            }

            // Pour les références à des globales, nous devons avoir un type pointeur.
            auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(type_struct_membre,
                                                                         false);

            auto ident_valeurs = m_compilatrice.donne_identifiant_pour_globale(
                "membres_info_type");
            auto tableau_membre = m_constructrice.crée_tableau_global(
                *ident_valeurs, type_membre, std::move(valeurs_membres));

            auto tableau_structs_employées = donne_tableau_pour_structs_employées(type_struct,
                                                                                  site);

            /* { membres basiques, nom, membres } */
            auto valeurs = kuri::tableau<AtomeConstante *>(5);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::STRUCTURE, type);
            valeurs[1] = crée_chaine(
                donne_nom_hiérarchique(const_cast<TypeStructure *>(type_struct)));
            valeurs[2] = tableau_membre;
            valeurs[3] = tableau_structs_employées;
            valeurs[4] = crée_tableau_annotations_pour_info_membre(type_struct->annotations);

            globale->initialisateur = m_constructrice.crée_constante_structure(type_info_struct,
                                                                               std::move(valeurs));
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto type_deref = type_dereference_pour(type);
            auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                m_compilatrice.typeuse.type_info_type_, false);

            /* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */
            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::TABLEAU, type);
            valeurs[1] = type_deref ?
                             crée_info_type_avec_transtype(type_deref, site) :
                             m_constructrice.crée_constante_nulle(type_pointeur_info_type);
            ;
            valeurs[2] = m_constructrice.crée_constante_booléenne(false);
            valeurs[3] = m_constructrice.crée_z32(0);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_tableau, std::move(valeurs));
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();

            /* { membres basiques, type_pointé, est_tableau_fixe, taille_fixe } */
            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::TABLEAU, type);
            valeurs[1] = crée_info_type_avec_transtype(type_tableau->type_pointe, site);
            valeurs[2] = m_constructrice.crée_constante_booléenne(true);
            valeurs[3] = m_constructrice.crée_z32(static_cast<unsigned>(type_tableau->taille));

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_tableau, std::move(valeurs));
            break;
        }
        case GenreNoeud::FONCTION:
        {
            auto type_fonction = type->comme_type_fonction();

            auto tableau_types_entrée = donne_tableau_pour_types_entrées(type_fonction, site);
            auto tableau_types_sortie = donne_tableau_pour_type_sortie(type_fonction, site);

            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::FONCTION, type);
            valeurs[1] = tableau_types_entrée;
            valeurs[2] = tableau_types_sortie;
            valeurs[3] = m_constructrice.crée_constante_booléenne(false);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_fonction, std::move(valeurs));
            break;
        }
        case GenreNoeud::EINI:
        {
            type->atome_info_type = crée_info_type_défaut(IDInfoType::EINI, type);
            break;
        }
        case GenreNoeud::RIEN:
        {
            type->atome_info_type = crée_info_type_défaut(IDInfoType::RIEN, type);
            break;
        }
        case GenreNoeud::CHAINE:
        {
            type->atome_info_type = crée_info_type_défaut(IDInfoType::CHAINE, type);
            break;
        }
        case GenreNoeud::TYPE_DE_DONNEES:
        {
            type->atome_info_type = crée_info_type_défaut(IDInfoType::TYPE_DE_DONNEES, type);
            break;
        }
        case GenreNoeud::DECLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            auto type_opacifie = type_opaque->type_opacifie;

            /* { membres basiques, nom, type_opacifié } */
            auto valeurs = kuri::tableau<AtomeConstante *>(3);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::OPAQUE, type_opaque);
            valeurs[1] = crée_chaine(
                donne_nom_hiérarchique(const_cast<TypeOpaque *>(type_opaque)));
            valeurs[2] = crée_info_type_avec_transtype(type_opacifie, site);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_opaque, std::move(valeurs));
            break;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_variadique = type->comme_type_variadique();
            auto type_élément = type_variadique->type_pointe;
            auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                m_compilatrice.typeuse.type_info_type_, false);

            /* { base, type_élément } */
            auto valeurs = kuri::tableau<AtomeConstante *>(2);
            valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::VARIADIQUE, type);
            valeurs[1] = type_élément ?
                             crée_info_type_avec_transtype(type_élément, site) :
                             m_constructrice.crée_constante_nulle(type_pointeur_info_type);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_variadique, std::move(valeurs));
            break;
        }
        default:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud géré pour type : " << type->genre; });
            break;
        }
    }

    // À FAIRE : il nous faut toutes les informations du type pour pouvoir générer les informations
    // assert_rappel(type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE), [type]() {
    //     dbg() << "Info type pour " << chaine_type(type) << " est incomplet";
    // });

    type->atome_info_type->est_info_type_de = type;

    return type->atome_info_type;
}

AtomeConstante *CompilatriceRI::transtype_base_info_type(AtomeConstante *info_type)
{
    auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
        m_compilatrice.typeuse.type_info_type_, false);

    if (info_type->type == type_pointeur_info_type) {
        return info_type;
    }

    return m_constructrice.crée_transtype_constant(type_pointeur_info_type, info_type);
}

void CompilatriceRI::génère_ri_pour_initialisation_globales(
    EspaceDeTravail *espace,
    AtomeFonction *fonction_init,
    kuri::tableau_statique<AtomeGlobale *> globales)
{
    m_espace = espace;
    génère_ri_pour_initialisation_globales(fonction_init, globales);
}

AtomeConstante *CompilatriceRI::crée_constante_info_type_pour_base(uint32_t index,
                                                                   Type const *pour_type)
{
    auto membres = kuri::tableau<AtomeConstante *>(3);
    remplis_membres_de_bases_info_type(membres, index, pour_type);
    return m_constructrice.crée_constante_structure(m_compilatrice.typeuse.type_info_type_,
                                                    std::move(membres));
}

void CompilatriceRI::remplis_membres_de_bases_info_type(kuri::tableau<AtomeConstante *> &valeurs,
                                                        uint32_t index,
                                                        Type const *pour_type)
{
    assert(valeurs.taille() == 3);
    valeurs[0] = m_constructrice.crée_z32(index);
    /* Puisque nous pouvons générer du code pour des architectures avec adressages en 32 ou 64
     * bits, et puisque nous pouvons exécuter sur une machine avec une architecture différente de
     * la cible de compilation, nous générons les constantes pour les taille_de lors de l'émission
     * du code machine. */
    valeurs[1] = m_constructrice.crée_constante_taille_de(pour_type);
    // L'index dans la table des types sera mis en place lors de la génération du code machine.
    valeurs[2] = m_constructrice.crée_index_table_type(pour_type);
}

AtomeGlobale *CompilatriceRI::crée_info_type_défaut(unsigned index, Type const *pour_type)
{
    auto valeurs = kuri::tableau<AtomeConstante *>(3);
    remplis_membres_de_bases_info_type(valeurs, index, pour_type);
    return crée_globale_info_type(m_compilatrice.typeuse.type_info_type_, std::move(valeurs));
}

AtomeGlobale *CompilatriceRI::crée_info_type_entier(Type const *pour_type, bool est_relatif)
{
    auto valeurs = kuri::tableau<AtomeConstante *>(2);
    valeurs[0] = crée_constante_info_type_pour_base(IDInfoType::ENTIER, pour_type);
    valeurs[1] = m_constructrice.crée_constante_booléenne(est_relatif);
    return crée_globale_info_type(m_compilatrice.typeuse.type_info_type_entier,
                                  std::move(valeurs));
}

AtomeConstante *CompilatriceRI::crée_info_type_avec_transtype(Type const *type,
                                                              NoeudExpression const *site)
{
    auto info_type = crée_info_type(type, site);
    return transtype_base_info_type(info_type);
}

AtomeGlobale *CompilatriceRI::crée_globale_info_type(Type const *type_info_type,
                                                     kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto ident = m_compilatrice.donne_identifiant_pour_globale("info_type");
    auto initialisateur = m_constructrice.crée_constante_structure(type_info_type,
                                                                   std::move(valeurs));
    return m_constructrice.crée_globale(*ident, type_info_type, initialisateur, false, true);
}

AtomeGlobale *CompilatriceRI::crée_info_type_membre_structure(const MembreTypeComposé &membre,
                                                              NoeudExpression const *site)
{
    auto type_struct_membre = m_compilatrice.typeuse.type_info_type_membre_structure;

    /* { nom: chaine, info : *InfoType, décalage, drapeaux } */
    auto valeurs = kuri::tableau<AtomeConstante *>(5);
    valeurs[0] = crée_chaine(membre.nom->nom);
    valeurs[1] = crée_info_type_avec_transtype(membre.type, site);
    valeurs[2] = m_constructrice.crée_z64(static_cast<uint64_t>(membre.decalage));
    valeurs[3] = m_constructrice.crée_z32(static_cast<unsigned>(membre.drapeaux));

    if (membre.decl) {
        if (membre.decl->est_declaration_variable()) {
            valeurs[4] = crée_tableau_annotations_pour_info_membre(
                membre.decl->comme_declaration_variable()->annotations);
        }
        else if (membre.decl->est_declaration_constante()) {
            valeurs[4] = crée_tableau_annotations_pour_info_membre(
                membre.decl->comme_declaration_constante()->annotations);
        }
        else {
            valeurs[4] = crée_tableau_annotations_pour_info_membre({});
        }
    }
    else {
        valeurs[4] = crée_tableau_annotations_pour_info_membre({});
    }

    return crée_globale_info_type(type_struct_membre, std::move(valeurs));
}

AtomeConstante *CompilatriceRI::donne_tableau_pour_structs_employées(
    const TypeStructure *type_structure, NoeudExpression const *site)
{
    if (type_structure->types_employés.taille() == 0 && m_tableau_structs_employées_vide) {
        return m_tableau_structs_employées_vide;
    }

    kuri::tableau<AtomeConstante *> valeurs_structs_employees;
    valeurs_structs_employees.reserve(type_structure->types_employés.taille());

    POUR (type_structure->types_employés) {
        valeurs_structs_employees.ajoute(crée_info_type(it->type, site));
    }

    /* À FAIRE : déduplique les autres tableaux. */

    auto type_info_struct = m_compilatrice.typeuse.type_info_type_structure;

    auto ident_structs_employées = m_compilatrice.donne_identifiant_pour_globale(
        "structs_employées");
    auto type_pointeur_info_struct = m_compilatrice.typeuse.type_pointeur_pour(type_info_struct,
                                                                               false);
    auto résultat = m_constructrice.crée_tableau_global(
        *ident_structs_employées, type_pointeur_info_struct, std::move(valeurs_structs_employees));

    if (type_structure->types_employés.taille() == 0) {
        m_tableau_structs_employées_vide = résultat;
    }

    return résultat;
}

AtomeConstante *CompilatriceRI::donne_tableau_pour_types_entrées(const TypeFonction *type_fonction,
                                                                 const NoeudExpression *site)
{
    if (type_fonction->types_entrees.est_vide() && m_tableau_types_entrées_vide) {
        return m_tableau_types_entrées_vide;
    }

    kuri::tableau<AtomeConstante *> types_entree;
    types_entree.reserve(type_fonction->types_entrees.taille());
    POUR (type_fonction->types_entrees) {
        types_entree.ajoute(crée_info_type_avec_transtype(it, site));
    }

    /* À FAIRE : déduplique les autres tableaux. */

    auto type_élément = m_compilatrice.typeuse.type_pointeur_pour(
        m_compilatrice.typeuse.type_info_type_, false);

    auto ident = m_compilatrice.donne_identifiant_pour_globale("types_entrée");
    auto résultat = m_constructrice.crée_tableau_global(
        *ident, type_élément, std::move(types_entree));

    if (type_fonction->types_entrees.est_vide()) {
        m_tableau_types_entrées_vide = résultat;
    }

    return résultat;
}

AtomeConstante *CompilatriceRI::donne_tableau_pour_type_sortie(const TypeFonction *type_fonction,
                                                               const NoeudExpression *site)
{
    auto const type_sortie = type_fonction->type_sortie;
    if (type_sortie->est_type_rien() && m_tableau_types_sorties_rien) {
        return m_tableau_types_sorties_rien;
    }

    kuri::tableau<AtomeConstante *> types_sortie;
    if (type_sortie->est_type_tuple()) {
        auto tuple = type_sortie->comme_type_tuple();

        types_sortie.reserve(tuple->membres.taille());
        POUR (tuple->membres) {
            types_sortie.ajoute(crée_info_type_avec_transtype(it.type, site));
        }
    }
    else {
        types_sortie.reserve(1);
        types_sortie.ajoute(crée_info_type_avec_transtype(type_fonction->type_sortie, site));
    }

    /* À FAIRE : déduplique les autres tableaux. */

    auto ident = m_compilatrice.donne_identifiant_pour_globale("types_sortie");
    auto type_élément = m_compilatrice.typeuse.type_pointeur_pour(
        m_compilatrice.typeuse.type_info_type_, false);
    auto résultat = m_constructrice.crée_tableau_global(
        *ident, type_élément, std::move(types_sortie));

    if (type_sortie->est_type_rien()) {
        m_tableau_types_sorties_rien = résultat;
    }

    return résultat;
}

Atome *CompilatriceRI::converti_vers_tableau_dyn(NoeudExpression const *noeud,
                                                 Atome *pointeur_tableau_fixe,
                                                 TypeTableauFixe const *type_tableau_fixe,
                                                 Atome *place)
{
    auto alloc_tableau_dyn = place;

    if (alloc_tableau_dyn == nullptr) {
        auto type_tableau_dyn = m_compilatrice.typeuse.type_tableau_dynamique(
            type_tableau_fixe->type_pointe);
        alloc_tableau_dyn = m_constructrice.crée_allocation(noeud, type_tableau_dyn, nullptr);
    }

    auto ptr_pointeur_donnees = m_constructrice.crée_référence_membre(noeud, alloc_tableau_dyn, 0);
    auto premier_elem = m_constructrice.crée_accès_index(
        noeud, pointeur_tableau_fixe, m_constructrice.crée_z64(0ul));
    m_constructrice.crée_stocke_mem(noeud, ptr_pointeur_donnees, premier_elem);

    auto ptr_taille = m_constructrice.crée_référence_membre(noeud, alloc_tableau_dyn, 1);
    auto constante = m_constructrice.crée_z64(unsigned(type_tableau_fixe->taille));
    m_constructrice.crée_stocke_mem(noeud, ptr_taille, constante);

    return alloc_tableau_dyn;
}

AtomeConstante *CompilatriceRI::crée_chaine(kuri::chaine_statique chaine)
{
    auto table_chaines = m_compilatrice.table_chaines.verrou_ecriture();
    auto trouve = false;
    auto valeur = table_chaines->trouve(chaine, trouve);

    if (trouve) {
        return valeur;
    }

    auto type_chaine = TypeBase::CHAINE;

    AtomeConstante *constante_chaine;

    if (chaine.taille() == 0) {
        constante_chaine = m_constructrice.crée_initialisation_défaut_pour_type(type_chaine);
    }
    else {
        auto type_tableau = m_compilatrice.typeuse.type_tableau_fixe(
            TypeBase::Z8, static_cast<int>(chaine.taille()));
        auto tableau = m_constructrice.crée_constante_tableau_données_constantes(
            type_tableau, const_cast<char *>(chaine.pointeur()), chaine.taille());

        auto ident = m_compilatrice.donne_identifiant_pour_globale("texte_chaine");
        auto globale_tableau = m_constructrice.crée_globale(
            *ident, type_tableau, tableau, false, true);
        auto pointeur_chaine = m_constructrice.crée_accès_index_constant(globale_tableau, 0);
        auto taille_chaine = m_constructrice.crée_z64(static_cast<uint64_t>(chaine.taille()));

        auto membres = kuri::tableau<AtomeConstante *>(2);
        membres[0] = pointeur_chaine;
        membres[1] = taille_chaine;

        constante_chaine = m_constructrice.crée_constante_structure(type_chaine,
                                                                    std::move(membres));
    }

    table_chaines->insère(chaine, constante_chaine);

    return constante_chaine;
}

void CompilatriceRI::génère_ri_pour_initialisation_globales(
    AtomeFonction *fonction_init, kuri::tableau_statique<AtomeGlobale *> globales)
{
    définis_fonction_courante(fonction_init);

    /* Sauvegarde le retour. */
    auto di = fonction_init->instructions.dernière();
    fonction_init->instructions.supprime_dernier();

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

    auto globales_à_initialiser = donne_globales_à_initialiser(globales, m_compilatrice);
    POUR (globales_à_initialiser) {
        auto constructeur = trouve_constructeur_pour(it);
        if (!constructeur) {
            continue;
        }

        génère_ri_transformee_pour_noeud(
            constructeur->expression, nullptr, constructeur->transformation);

        auto valeur = depile_valeur();
        if (!valeur) {
            continue;
        }
        m_constructrice.crée_stocke_mem(nullptr, it, valeur);
    }

    /* Restaure le retour. */
    fonction_init->instructions.ajoute(di);

    définis_fonction_courante(nullptr);
}

void CompilatriceRI::génère_ri_pour_fonction_métaprogramme(
    NoeudDeclarationEnteteFonction *fonction)
{
    assert(fonction->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME));
    auto atome_fonc = m_constructrice.trouve_ou_insère_fonction(fonction);

    définis_fonction_courante(atome_fonc);

    auto decl_creation_contexte = m_compilatrice.interface_kuri->decl_creation_contexte;

    auto atome_creation_contexte = m_constructrice.trouve_ou_insère_fonction(
        decl_creation_contexte);

    atome_fonc->instructions.reserve(atome_creation_contexte->instructions.taille());

    for (auto i = 0; i < atome_creation_contexte->instructions.taille(); ++i) {
        auto it = atome_creation_contexte->instructions[i];
        atome_fonc->instructions.ajoute(it);
    }

    atome_fonc->decalage_appel_init_globale = atome_fonc->instructions.taille();

    génère_ri_pour_noeud(fonction->corps->bloc);

    fonction->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;

    définis_fonction_courante(nullptr);
}

enum class MéthodeConstructionGlobale {
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

static MéthodeConstructionGlobale détermine_méthode_construction_globale(
    NoeudExpression const *expression, TransformationType const &transformation)
{
    if (!expression) {
        return MéthodeConstructionGlobale::PAR_VALEUR_DEFAUT;
    }

    if (expression->est_non_initialisation()) {
        return MéthodeConstructionGlobale::SANS_INITIALISATION;
    }

    if (expression->est_construction_tableau()) {
        if (transformation.type != TypeTransformation::INUTILE) {
            return MéthodeConstructionGlobale::TABLEAU_FIXE_A_CONVERTIR;
        }

        auto const type_pointe = type_dereference_pour(expression->type);

        if (!peut_être_utilisée_pour_initialisation_constante_globale(expression)) {
            return MéthodeConstructionGlobale::NORMALE;
        }

        /* À FAIRE : permet la génération de code pour les tableaux globaux de structures dans le
         * contexte global. Ceci nécessitera d'avoir une deuxième version de la génération de code
         * pour les structures avec des instructions constantes. */
        if (type_pointe->est_type_structure() || type_pointe->est_type_union()) {
            return MéthodeConstructionGlobale::NORMALE;
        }

        return MéthodeConstructionGlobale::TABLEAU_CONSTANT;
    }

    return MéthodeConstructionGlobale::NORMALE;
}

void CompilatriceRI::génère_ri_pour_déclaration_variable(NoeudDeclarationVariable *decl)
{
    if (m_fonction_courante == nullptr) {
        génère_ri_pour_variable_globale(decl);
        return;
    }

    génère_ri_pour_variable_locale(decl);
}

void CompilatriceRI::génère_ri_pour_variable_globale(NoeudDeclarationVariable *decl)
{
    POUR (decl->donnees_decl.plage()) {
        for (auto i = 0; i < it.variables.taille(); ++i) {
            auto var = it.variables[i];
            auto valeur = static_cast<AtomeConstante *>(nullptr);
            auto atome = m_constructrice.trouve_ou_insère_globale(decl);

            auto expression = it.expression;
            if (expression && expression->substitution) {
                expression = expression->substitution;
            }

            auto const méthode_construction = détermine_méthode_construction_globale(
                expression, it.transformations[i]);

            switch (méthode_construction) {
                case MéthodeConstructionGlobale::TABLEAU_CONSTANT:
                {
                    génère_ri_pour_noeud(expression);
                    valeur = static_cast<AtomeConstante *>(depile_valeur());
                    break;
                }
                case MéthodeConstructionGlobale::TABLEAU_FIXE_A_CONVERTIR:
                {
                    auto type_tableau_fixe = expression->type->comme_type_tableau_fixe();

                    /* Crée une globale pour le tableau fixe, et utilise celle-ci afin
                     * d'initialiser le tableau dynamique. */
                    auto ident = m_compilatrice.donne_identifiant_pour_globale(
                        "données_initilisateur");
                    auto globale_tableau = m_constructrice.crée_globale(
                        *ident, expression->type, nullptr, false, false);

                    /* La construction du tableau devra se faire via la fonction
                     * d'initialisation des globales. */
                    m_compilatrice.constructeurs_globaux->ajoute(
                        {globale_tableau, expression, {}});

                    valeur = m_constructrice.crée_initialisation_tableau_global(globale_tableau,
                                                                                type_tableau_fixe);
                    break;
                }
                case MéthodeConstructionGlobale::NORMALE:
                {
                    m_compilatrice.constructeurs_globaux->ajoute(
                        {atome, expression, it.transformations[i]});
                    break;
                }
                case MéthodeConstructionGlobale::PAR_VALEUR_DEFAUT:
                {
                    valeur = m_constructrice.crée_initialisation_défaut_pour_type(var->type);
                    break;
                }
                case MéthodeConstructionGlobale::SANS_INITIALISATION:
                {
                    /* Rien à faire. */
                    break;
                }
            }

            atome->drapeaux |= DrapeauxAtome::RI_FUT_GÉNÉRÉE;
            atome->initialisateur = valeur;
        }
    }

    decl->atome->drapeaux |= DrapeauxAtome::RI_FUT_GÉNÉRÉE;
    decl->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
}

void CompilatriceRI::génère_ri_pour_variable_locale(NoeudDeclarationVariable const *decl)
{
    /* Crée d'abord les allocations. */
    POUR (decl->donnees_decl.plage()) {
        POUR_NOMME (var, it.variables.plage()) {
            auto pointeur = m_constructrice.crée_allocation(var, var->type, var->ident);
            auto decl_var = var->comme_reference_declaration()
                                ->declaration_referee->comme_declaration_symbole();
            decl_var->atome = pointeur;
        }
    }

    /* Génère le code pour les expressions. */
    génère_ri_pour_assignation_variable(decl->donnees_decl);
}

void CompilatriceRI::génère_ri_pour_assignation_variable(
    kuri::tableau_compresse<DonneesAssignations, int> const &données_exprs)
{
    auto donne_atome_pour_var = [&](NoeudExpression *var) {
        if (var->est_reference_declaration()) {
            auto référence = var->comme_reference_declaration();
            auto decl_var = référence->declaration_referee->comme_declaration_symbole();
            if (decl_var->atome) {
                return decl_var->atome;
            }
        }

        génère_ri_pour_noeud(var);
        return depile_valeur();
    };

    if (données_exprs.taille() == 1 && données_exprs[0].variables.taille() == 1) {
        /* Cas simple : a = b ou a := b */

        auto expression = données_exprs[0].expression;
        auto variable = données_exprs[0].variables[0];
        auto transformation = données_exprs[0].transformations[0];

        if (!expression) {
            auto pointeur = donne_atome_pour_var(variable);
            auto type_var = variable->type;
            if (est_type_fondamental(type_var)) {
                m_constructrice.crée_stocke_mem(
                    variable,
                    pointeur,
                    m_constructrice.crée_initialisation_défaut_pour_type(type_var));
            }
            else {
                crée_appel_fonction_init_type(variable, type_var, pointeur);
            }
            return;
        }

        if (expression->est_non_initialisation()) {
            return;
        }

        auto pointeur = donne_atome_pour_var(variable);
        /* Désactive le drapeau pour les assignations répétées aux mêmes valeurs.
         */
        pointeur->drapeaux &= ~DrapeauxAtome::EST_UTILISÉ;

        assert_rappel(expression->type,
                      [&]() { dbg() << "Aucun type pour " << expression->genre; });

        if (transformation.type == TypeTransformation::INUTILE) {
            génère_ri_pour_expression_droite(expression, pointeur);
            return;
        }

        génère_ri_transformee_pour_noeud(expression, pointeur, transformation);
        return;
    }

    /* Cas complexe a, b = fonction() */

    POUR (données_exprs.plage()) {
        auto expression = it.expression;

        if (!expression) {
            /* Cas pour les déclarations. */
            POUR_NOMME (var, it.variables.plage()) {
                auto pointeur = donne_atome_pour_var(var);
                auto type_var = var->type;
                if (est_type_fondamental(type_var)) {
                    m_constructrice.crée_stocke_mem(
                        var,
                        pointeur,
                        m_constructrice.crée_initialisation_défaut_pour_type(type_var));
                }
                else {
                    crée_appel_fonction_init_type(var, type_var, pointeur);
                }
            }

            continue;
        }

        if (expression->est_non_initialisation()) {
            /* Cas pour les déclarations. */
            continue;
        }

        auto ancienne_expression_gauche = expression_gauche;
        expression_gauche = false;
        génère_ri_pour_noeud(expression);
        expression_gauche = ancienne_expression_gauche;

        if (it.multiple_retour) {
            auto valeur_tuple = depile_valeur();

            for (auto i = 0; i < it.variables.taille(); ++i) {
                auto var = it.variables[i];
                auto pointeur = donne_atome_pour_var(var);
                auto &transformation = it.transformations[i];
                /* Désactive le drapeau pour les assignations répétées aux mêmes valeurs.
                 */
                pointeur->drapeaux &= ~DrapeauxAtome::EST_UTILISÉ;

                auto valeur = m_constructrice.crée_référence_membre(expression, valeur_tuple, i);
                transforme_valeur(expression, valeur, transformation, pointeur);
            }
        }
        else {
            auto valeur = depile_valeur();

            for (auto i = 0; i < it.variables.taille(); ++i) {
                auto var = it.variables[i];
                auto pointeur = donne_atome_pour_var(var);
                auto &transformation = it.transformations[i];
                /* Désactive le drapeau pour les assignations répétées aux mêmes valeurs.
                 */
                pointeur->drapeaux &= ~DrapeauxAtome::EST_UTILISÉ;

                transforme_valeur(expression, valeur, transformation, pointeur);
            }
        }
    }
}

static bool peut_être_compilé_en_données_constantes(NoeudExpressionConstructionTableau const *expr)
{
    auto const type_tableau = expr->type->comme_type_tableau_fixe();
    if (!est_type_entier(type_tableau->type_pointe) &&
        !type_tableau->type_pointe->est_type_reel()) {
        return false;
    }

    POUR (expr->expression->comme_virgule()->expressions) {
        if (!it->est_litterale_entier() && !it->est_litterale_reel()) {
            return false;
        }
    }

    return true;
}

template <typename T>
static void remplis_données_constantes_entières(T *données_constantes,
                                                NoeudExpressionVirgule const *expressions)
{
    POUR (expressions->expressions) {
        auto valeur_constante = it->comme_litterale_entier()->valeur;
        *données_constantes++ = static_cast<T>(valeur_constante);
    }
}

static void remplis_données_constantes_entières(char *données_constantes,
                                                Type const *type,
                                                NoeudExpressionVirgule const *expressions)
{
    if (type->taille_octet == 1) {
        remplis_données_constantes_entières(données_constantes, expressions);
    }
    else if (type->taille_octet == 2) {
        remplis_données_constantes_entières(reinterpret_cast<int16_t *>(données_constantes),
                                            expressions);
    }
    else if (type->taille_octet == 4 || type->est_type_entier_constant()) {
        remplis_données_constantes_entières(reinterpret_cast<int32_t *>(données_constantes),
                                            expressions);
    }
    else if (type->taille_octet == 8) {
        remplis_données_constantes_entières(reinterpret_cast<int64_t *>(données_constantes),
                                            expressions);
    }
}

template <typename T>
static void remplis_données_constantes_réelles(T *données_constantes,
                                               NoeudExpressionVirgule const *expressions)
{
    POUR (expressions->expressions) {
        if (it->est_litterale_entier()) {
            auto valeur_constante = it->comme_litterale_entier()->valeur;
            *données_constantes++ = static_cast<T>(valeur_constante);
        }
        else {
            auto valeur_constante = it->comme_litterale_reel()->valeur;
            *données_constantes++ = static_cast<T>(valeur_constante);
        }
    }
}

static void remplis_données_constantes_réelles(char *données_constantes,
                                               Type const *type,
                                               NoeudExpressionVirgule const *expressions)
{
    if (type->taille_octet == 2) {
        /* À FAIRE(r16) */
        remplis_données_constantes_réelles(reinterpret_cast<int16_t *>(données_constantes),
                                           expressions);
    }
    else if (type->taille_octet == 4) {
        remplis_données_constantes_réelles(reinterpret_cast<float *>(données_constantes),
                                           expressions);
    }
    else if (type->taille_octet == 8) {
        remplis_données_constantes_réelles(reinterpret_cast<double *>(données_constantes),
                                           expressions);
    }
}

void CompilatriceRI::génère_ri_pour_construction_tableau(
    NoeudExpressionConstructionTableau const *expr, Atome *place)
{
    auto feuilles = expr->expression->comme_virgule();

    if (m_fonction_courante == nullptr) {
        auto type_tableau_fixe = expr->type->comme_type_tableau_fixe();

        if (peut_être_compilé_en_données_constantes(expr)) {
            auto type_élément = type_tableau_fixe->type_pointe;
            kuri::tableau<char> données_constantes(feuilles->expressions.taille() *
                                                   int(type_élément->taille_octet));

            if (est_type_entier(type_élément)) {
                remplis_données_constantes_entières(
                    &données_constantes[0], type_élément, feuilles);
            }
            else {
                assert(type_élément->est_type_reel());
                remplis_données_constantes_réelles(&données_constantes[0], type_élément, feuilles);
            }

            auto tableau = m_constructrice.crée_constante_tableau_données_constantes(
                type_tableau_fixe, std::move(données_constantes));

            empile_valeur(tableau);
            return;
        }

        kuri::tableau<AtomeConstante *> valeurs;
        valeurs.reserve(feuilles->expressions.taille());

        POUR (feuilles->expressions) {
            génère_ri_pour_noeud(it);
            auto valeur = depile_valeur();
            valeurs.ajoute(static_cast<AtomeConstante *>(valeur));
        }

        auto tableau_constant = m_constructrice.crée_constante_tableau_fixe(type_tableau_fixe,
                                                                            std::move(valeurs));
        empile_valeur(tableau_constant);
        return;
    }

    auto pointeur_tableau = place;
    if (!pointeur_tableau) {
        pointeur_tableau = m_constructrice.crée_allocation(expr, expr->type, nullptr);
    }

    auto index = 0ul;
    POUR (feuilles->expressions) {
        auto index_tableau = m_constructrice.crée_accès_index(
            expr, pointeur_tableau, m_constructrice.crée_z64(index++));
        génère_ri_pour_expression_droite(it, index_tableau);
    }

    if (pointeur_tableau != place) {
        empile_valeur(pointeur_tableau);
    }
    else {
        place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
    }
}

AtomeGlobale *CompilatriceRI::crée_info_fonction_pour_trace_appel(AtomeFonction *pour_fonction)
{
    /* @concurrence critique. */
    if (pour_fonction->info_trace_appel) {
        return pour_fonction->info_trace_appel;
    }

    auto type_info_fonction_trace_appel = m_compilatrice.typeuse.type_info_fonction_trace_appel;
    auto decl = pour_fonction->decl;
    auto fichier = m_compilatrice.fichier(decl->lexeme->fichier);
    auto nom_fonction = decl->ident ? crée_chaine(decl->ident->nom) : crée_chaine("???");
    auto nom_fichier = crée_chaine(fichier->nom());

    kuri::tableau<AtomeConstante *> valeurs(3);
    valeurs[0] = nom_fonction;
    valeurs[1] = nom_fichier;
    valeurs[2] = m_constructrice.crée_transtype_constant(TypeBase::PTR_RIEN, pour_fonction);

    auto initialisateur = m_constructrice.crée_constante_structure(type_info_fonction_trace_appel,
                                                                   std::move(valeurs));

    auto ident = m_compilatrice.donne_identifiant_pour_globale("info_fonction_trace_appel");
    pour_fonction->info_trace_appel = m_constructrice.crée_globale(
        *ident, type_info_fonction_trace_appel, initialisateur, false, true);

    crée_trace_appel(pour_fonction);

    return pour_fonction->info_trace_appel;
}

AtomeGlobale *CompilatriceRI::crée_info_appel_pour_trace_appel(InstructionAppel *pour_appel)
{
    /* @concurrence critique. */
    if (pour_appel->info_trace_appel) {
        return pour_appel->info_trace_appel;
    }

    auto type_info_appel_trace_appel = m_compilatrice.typeuse.type_info_appel_trace_appel;

    /* Ligne colonne texte. */
    kuri::tableau<AtomeConstante *> valeurs(3);
    if (pour_appel->site) {
        auto const lexeme = pour_appel->site->lexeme;
        auto const fichier = m_compilatrice.fichier(pour_appel->site->lexeme->fichier);
        auto const texte_ligne = fichier->tampon()[lexeme->ligne];

        valeurs[0] = m_constructrice.crée_z32(uint64_t(lexeme->ligne));
        valeurs[1] = m_constructrice.crée_z32(uint64_t(lexeme->colonne));
        valeurs[2] = crée_chaine(kuri::chaine_statique(texte_ligne.begin(), texte_ligne.taille()));
    }
    else {
        valeurs[0] = m_constructrice.crée_z32(0);
        valeurs[1] = m_constructrice.crée_z32(0);
        valeurs[2] = crée_chaine("???");
    }

    auto initialisateur = m_constructrice.crée_constante_structure(type_info_appel_trace_appel,
                                                                   std::move(valeurs));

    auto ident = m_compilatrice.donne_identifiant_pour_globale("info_appel_trace_appel");
    pour_appel->info_trace_appel = m_constructrice.crée_globale(
        *ident, type_info_appel_trace_appel, initialisateur, false, true);

    return pour_appel->info_trace_appel;
}

void CompilatriceRI::crée_trace_appel(AtomeFonction *fonction)
{
    auto type_trace_appel = m_compilatrice.typeuse.type_trace_appel->comme_type_compose();
    auto contexte_fil_principale = m_constructrice.trouve_globale(
        m_compilatrice.globale_contexte_programme);
    auto type_contexte_fil_principal = m_compilatrice.typeuse.type_contexte->comme_type_compose();
    auto index_trace_appel_contexte =
        donne_membre_pour_nom(type_contexte_fil_principal, ID::trace_appel)->index_membre;

    auto anciennes_instructions = fonction->instructions;
    fonction->instructions.efface();

    définis_fonction_courante(fonction);

    /* Insère le premier label. */
    assert(anciennes_instructions[0]->est_label());
    fonction->instructions.ajoute(anciennes_instructions[0]);

    auto const index_précédente =
        donne_membre_pour_nom(type_trace_appel, ID::précédente)->index_membre;
    auto const index_info_fonction =
        donne_membre_pour_nom(type_trace_appel, ID::info_fonction)->index_membre;
    auto const index_info_appel =
        donne_membre_pour_nom(type_trace_appel, ID::info_appel)->index_membre;
    auto const index_profondeur =
        donne_membre_pour_nom(type_trace_appel, ID::profondeur)->index_membre;

    /* Crée la variable pour la trace d'appel. */
    auto trace_appel = m_constructrice.crée_allocation(nullptr, type_trace_appel, nullptr);

    /* trace.info_fonction = *info_fonction */
    auto ref_info_fonction = m_constructrice.crée_référence_membre(
        nullptr, trace_appel, index_info_fonction);
    m_constructrice.crée_stocke_mem(nullptr, ref_info_fonction, fonction->info_trace_appel);

    /* trace.précédente = __contexte_fil_principal.trace_appel */
    auto trace_appel_contexte = m_constructrice.crée_référence_membre(
        nullptr, contexte_fil_principale, index_trace_appel_contexte);
    auto ref_trace_précédente = m_constructrice.crée_référence_membre(
        nullptr, trace_appel, index_précédente);
    m_constructrice.crée_stocke_mem(
        nullptr,
        ref_trace_précédente,
        m_constructrice.crée_charge_mem(nullptr, trace_appel_contexte));

    /* trace.profondeur = trace.précédente.profondeur + 1 */
    auto ref_trace_précédente2 = m_constructrice.crée_référence_membre(
        nullptr, trace_appel, index_précédente);
    auto charge_ref_trace_précédente2 = m_constructrice.crée_charge_mem(nullptr,
                                                                        ref_trace_précédente2);
    auto ref_profondeur2 = m_constructrice.crée_référence_membre(
        nullptr, charge_ref_trace_précédente2, index_profondeur);
    auto charge_profondeur2 = m_constructrice.crée_charge_mem(nullptr, ref_profondeur2);
    auto incrémentation = m_constructrice.crée_op_binaire(nullptr,
                                                          TypeBase::Z32,
                                                          OpérateurBinaire::Genre::Addition,
                                                          charge_profondeur2,
                                                          m_constructrice.crée_z32(1));

    auto ref_profondeur = m_constructrice.crée_référence_membre(
        nullptr, trace_appel, index_profondeur);
    m_constructrice.crée_stocke_mem(nullptr, ref_profondeur, incrémentation);

    /* Copie les instructions et crée les modifications de la trace d'appel. */

    for (auto i = 1; i < anciennes_instructions.taille(); i++) {
        auto inst = anciennes_instructions[i];

        if (inst->est_appel()) {
            /* Définis trace appel. */
            /* trace_appel.info_appel = *info_appel */
            auto ref_info_appel = m_constructrice.crée_référence_membre(
                nullptr, trace_appel, index_info_appel);
            m_constructrice.crée_stocke_mem(
                nullptr, ref_info_appel, inst->comme_appel()->info_trace_appel);

            /* __contexte_fil_principal.trace_appel = *ma_trace */
            auto ref_trace_appel_contexte = m_constructrice.crée_référence_membre(
                nullptr, contexte_fil_principale, index_trace_appel_contexte);
            m_constructrice.crée_stocke_mem(nullptr, ref_trace_appel_contexte, trace_appel);
        }

        fonction->instructions.ajoute(inst);

        if (inst->est_appel()) {
            /* Restaure trace appel. */
            /* __contexte_fil_principal.trace_appel = ma_trace.précédente */
            auto ref_trace_appel_contexte = m_constructrice.crée_référence_membre(
                nullptr, contexte_fil_principale, index_trace_appel_contexte);
            auto ref_trace_précédente3 = m_constructrice.crée_référence_membre(
                nullptr, trace_appel, index_précédente);
            auto charge_ref_trace_précédente3 = m_constructrice.crée_charge_mem(
                nullptr, ref_trace_précédente3);
            m_constructrice.crée_stocke_mem(
                nullptr, ref_trace_appel_contexte, charge_ref_trace_précédente3);
        }
    }

    définis_fonction_courante(nullptr);
}

void CompilatriceRI::rassemble_statistiques(Statistiques &stats)
{
    m_constructrice.rassemble_statistiques(stats);
}

/* ------------------------------------------------------------------------- */
/** \name RegistreAnnotations.
 * \{ */

AtomeGlobale *RegistreAnnotations::trouve_globale_pour_annotation(
    const Annotation &annotation) const
{
    auto trouvée = false;
    auto const &paires = m_table.trouve(annotation.nom, trouvée);
    if (!trouvée) {
        return nullptr;
    }

    POUR (paires) {
        if (it.valeur == annotation.valeur) {
            return it.globale;
        }
    }

    return nullptr;
}

void RegistreAnnotations::ajoute_annotation(const Annotation &annotation, AtomeGlobale *globale)
{
    auto paire = PaireValeurGlobale{annotation.valeur, globale};

    auto ptr = m_table.trouve_pointeur(annotation.nom);
    if (ptr) {
        ptr->ajoute(paire);
        return;
    }

    auto tableau = kuri::tableau<PaireValeurGlobale, int>();
    tableau.ajoute(paire);
    m_table.insère(annotation.nom, tableau);
}

/** \} */
