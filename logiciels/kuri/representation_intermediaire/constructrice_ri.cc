/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "constructrice_ri.hh"

#include "biblinternes/outils/assert.hh"
#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/infos_types.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/portee.hh"

#include "statistiques/statistiques.hh"

#include "structures/enchaineuse.hh"
#include "structures/rassembleuse.hh"

#include "utilitaires/log.hh"

#include "analyse.hh"
#include "impression.hh"
#include "optimisations.hh"

/* À FAIRE : (représentation intermédiaire, non-urgent)
 * - copie les tableaux fixes quand nous les assignations (a = b -> copie_mem(a, b))
 */

/* ************************************************************************** */

static NoeudDéclarationEntêteFonction *est_appel_fonction_avec_drapeau(
    NoeudExpression *expression, DrapeauxNoeudFonction drapeau)
{
    if (expression->est_entête_fonction()) {
        auto entete = expression->comme_entête_fonction();
        if (entete->possède_drapeau(drapeau)) {
            return entete;
        }
        return nullptr;
    }

    if (expression->est_référence_déclaration()) {
        return est_appel_fonction_avec_drapeau(
            expression->comme_référence_déclaration()->déclaration_référée, drapeau);
    }

    return nullptr;
}

/* Retourne la déclaration de la fontion si l'expression d'appel est un appel vers une telle
 * fontion. */
static NoeudDéclarationEntêteFonction *est_appel_fonction_initialisation(
    NoeudExpression *expression)
{
    return est_appel_fonction_avec_drapeau(expression,
                                           DrapeauxNoeudFonction::EST_INITIALISATION_TYPE);
}

static NoeudDéclarationEntêteFonction *est_appel_fonction_intrinsèque(NoeudExpression *expression)
{
    return est_appel_fonction_avec_drapeau(expression, DrapeauxNoeudFonction::EST_INTRINSÈQUE);
}

static bool est_référence_compatible_pointeur(Type const *type_dest, Type const *type_source)
{
    if (!type_source->est_type_référence()) {
        return false;
    }

    if (type_source->comme_type_référence()->type_pointé !=
        type_dest->comme_type_pointeur()->type_pointé) {
        return false;
    }

    return true;
}

static bool type_dest_et_type_source_sont_compatibles(Type const *type_dest,
                                                      Type const *type_source)
{
    auto type_élément_dest = type_déréférencé_pour(type_dest);
    if (type_élément_dest == type_source) {
        return true;
    }

    /* L'initialisation des r16 utilise un n16. */
    if (type_source == TypeBase::N16 && type_élément_dest == TypeBase::R16) {
        return true;
    }

    /* Nous avons différents types de données selon le type connu lors de la compilation. */
    if (type_élément_dest->est_type_type_de_données() && type_source->est_type_type_de_données()) {
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
    if (!gauche->est_type_énum()) {
        return false;
    }

    auto type_énum = gauche->comme_type_énum();
    return type_énum->type_sous_jacent == droite;
}

static bool sont_types_compatibles_pour_opérateur_binaire(Type const *gauche, Type const *droite)
{
    if (gauche == droite) {
        return true;
    }
    if (droite->est_type_type_de_données() && gauche->est_type_type_de_données()) {
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
        if (!type_variadique->type_pointé) {
            return true;
        }
        return expression == type_variadique->type_tranche;
    }
    if (expression->est_type_type_de_données() && paramètre->est_type_type_de_données()) {
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
    return fonctions.ajoute_élément(nullptr, nom_fonction);
}

AtomeFonction *RegistreSymboliqueRI::trouve_ou_insère_fonction(
    NoeudDéclarationEntêteFonction *decl)
{
    std::unique_lock lock(mutex_atomes_fonctions);

    if (decl->atome) {
        return decl->atome->comme_fonction();
    }

    auto params = kuri::tableau<InstructionAllocation *, int>();
    params.redimensionne(decl->params.taille());

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = decl->parametre_entree(i)->comme_déclaration_variable();
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
            it->comme_déclaration_variable()->atome = m_constructrice->crée_référence_membre(
                it, atome_param_sortie, index_it, true);
        }
    }

    auto atome_fonc = fonctions.ajoute_élément(
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
    return globales.ajoute_élément(&ident,
                                   m_typeuse.type_pointeur_pour(const_cast<Type *>(type), false),
                                   initialisateur,
                                   est_externe,
                                   est_constante);
}

AtomeGlobale *RegistreSymboliqueRI::trouve_globale(NoeudDéclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);
    auto decl_var = decl->comme_déclaration_variable();
    return decl_var->atome->comme_globale();
}

AtomeGlobale *RegistreSymboliqueRI::trouve_ou_insère_globale(NoeudDéclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);

    auto decl_var = decl->comme_déclaration_variable();

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

    auto mémoire_paramètres = int64_t(0);
    auto gaspillage_paramètres = int64_t(0);
    auto mémoire_instructions = int64_t(0);
    auto gaspillage_instructions = int64_t(0);
    auto mémoire_code_binaire = int64_t(0);
    pour_chaque_élément(fonctions, [&](AtomeFonction const &it) {
        mémoire_paramètres += it.params_entrée.taille_mémoire();
        gaspillage_paramètres += it.params_entrée.gaspillage_mémoire();
        mémoire_instructions += it.instructions.taille_mémoire();
        gaspillage_instructions += it.instructions.gaspillage_mémoire();

        if (it.données_exécution) {
            mémoire_code_binaire += it.données_exécution->mémoire_utilisée();
            mémoire_code_binaire += taille_de(DonnéesExécutionFonction);
        }
    });

    stats_ri.fusionne_entrée({"paramètres_fonctions", fonctions.taille(), mémoire_paramètres});
    stats_ri.fusionne_entrée({"instructions_fonctions", fonctions.taille(), mémoire_instructions});
    stats_ri.fusionne_entrée({"fonctions", fonctions.taille(), fonctions.mémoire_utilisée()});
    stats_ri.fusionne_entrée({"globales", globales.taille(), globales.mémoire_utilisée()});

    m_constructrice->rassemble_statistiques(stats);

    stats.ajoute_mémoire_utilisée("Code Binaire", mémoire_code_binaire);

    auto &stats_gaspillage = stats.stats_gaspillage;
    stats_gaspillage.fusionne_entrée({"Instructions RI", 1, gaspillage_instructions});
    stats_gaspillage.fusionne_entrée({"Paramètres Fonctions RI", 1, gaspillage_paramètres});
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name ConstructriceRI
 * \{ */

void ConstructriceRI::définis_fonction_courante(AtomeFonction *fonction_courante)
{
    m_charges.efface();
    m_nombre_labels = 0;
    m_fonction_courante = fonction_courante;
}

AtomeFonction *ConstructriceRI::crée_fonction(kuri::chaine_statique nom_fonction)
{
    return m_registre.crée_fonction(nom_fonction);
}

AtomeFonction *ConstructriceRI::trouve_ou_insère_fonction(NoeudDéclarationEntêteFonction *decl)
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

AtomeGlobale *ConstructriceRI::trouve_globale(NoeudDéclaration *decl)
{
    return m_registre.trouve_globale(decl);
}

AtomeGlobale *ConstructriceRI::trouve_ou_insère_globale(NoeudDéclaration *decl)
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
    auto inst = m_alloc.ajoute_élément(site_, type_pointeur, ident);

    /* Nous utilisons pour l'instant crée_allocation pour les paramètres des
     * fonctions, et la fonction_courante est nulle lors de cette opération.
     */
    if (m_fonction_courante && !crée_seulement) {
        insère(inst);
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
    POUR_TABLEAU_PAGE (m_constante_entière) {
        if (it.type == type && it.valeur == valeur) {
            return &it;
        }
    }
#endif
    return m_constante_entière.ajoute_élément(type, valeur);
}

AtomeConstanteType *ConstructriceRI::crée_constante_type(Type const *pointeur_type)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (m_constante_type) {
        if (it.type_de_données == pointeur_type) {
            return &it;
        }
    }
#endif
    return m_constante_type.ajoute_élément(m_typeuse.type_type_de_donnees_, pointeur_type);
}

AtomeConstanteTailleDe *ConstructriceRI::crée_constante_taille_de(Type const *pointeur_type)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (m_taille_de) {
        if (it.type_de_données == pointeur_type) {
            return &it;
        }
    }
#endif
    return m_taille_de.ajoute_élément(TypeBase::N32, pointeur_type);
}

AtomeIndexTableType *ConstructriceRI::crée_index_table_type(const Type *pointeur_type)
{
    return m_index_table_type.ajoute_élément(TypeBase::N32, pointeur_type);
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
    return m_constante_réelle.ajoute_élément(type, valeur);
}

AtomeConstanteStructure *ConstructriceRI::crée_constante_structure(
    Type const *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    /* À FAIRE : attend sur la validation des types avant de générer la RI. */
#if 0
    assert_rappel(valeurs.taille() == type->comme_type_composé()->nombre_de_membres_réels, [&]() {
        dbg() << "Le type est " << chaine_type(type) << "\nLe nombre de membres est "
              << type->comme_type_composé()->nombre_de_membres_réels
              << ".\nLe nombre de valeurs est " << valeurs.taille() << ".";
    });
#endif
    return m_constante_structure.ajoute_élément(type, std::move(valeurs));
}

AtomeConstanteTableauFixe *ConstructriceRI::crée_constante_tableau_fixe(
    Type const *type, kuri::tableau<AtomeConstante *> &&valeurs)
{
    return m_constante_tableau.ajoute_élément(type, std::move(valeurs));
}

AtomeConstanteDonnéesConstantes *ConstructriceRI::crée_constante_tableau_données_constantes(
    Type const *type, kuri::tableau<char> &&données_constantes)
{
    return m_données_constantes.ajoute_élément(type, std::move(données_constantes));
}

AtomeConstanteDonnéesConstantes *ConstructriceRI::crée_constante_tableau_données_constantes(
    Type const *type, char *pointeur, int64_t taille)
{
    return m_données_constantes.ajoute_élément(type, pointeur, taille);
}

AtomeInitialisationTableau *ConstructriceRI::crée_initialisation_tableau(
    const Type *type, const AtomeConstante *valeur)
{
    return m_initialisation_tableau.ajoute_élément(type, valeur);
}

AtomeNonInitialisation *ConstructriceRI::crée_non_initialisation()
{
    return m_non_initialisation.ajoute_élément();
}

AtomeConstante *ConstructriceRI::crée_tranche_globale(IdentifiantCode &ident,
                                                      Type const *type,
                                                      kuri::tableau<AtomeConstante *> &&valeurs)
{
    auto taille_tableau = static_cast<int>(valeurs.taille());

    if (taille_tableau == 0) {
        auto type_tranche = m_typeuse.crée_type_tranche(const_cast<Type *>(type));
        return crée_initialisation_défaut_pour_type(type_tranche);
    }

    auto type_tableau = m_typeuse.type_tableau_fixe(const_cast<Type *>(type), taille_tableau);
    auto tableau_fixe = crée_constante_tableau_fixe(type_tableau, std::move(valeurs));

    return crée_tranche_globale(ident, tableau_fixe);
}

AtomeConstante *ConstructriceRI::crée_tranche_globale(IdentifiantCode &ident,
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
    auto type_tranche = m_typeuse.crée_type_tranche(type_tableau_fixe->type_pointé);

    if (est_globale_pour_tableau_données_constantes(globale_tableau_fixe)) {
        if (type_tableau_fixe->type_pointé != TypeBase::Z8) {
            /* Nous devons transtypé vers le type pointeur idoine. */
            auto type_cible = m_typeuse.type_pointeur_pour(type_tableau_fixe->type_pointé);
            ptr_premier_élément = crée_transtype_constant(type_cible, ptr_premier_élément);
        }
    }

    auto membres = kuri::tableau<AtomeConstante *>(2);
    membres[0] = ptr_premier_élément;
    membres[1] = valeur_taille;

    return crée_constante_structure(type_tranche, std::move(membres));
}

AtomeConstanteBooléenne *ConstructriceRI::crée_constante_booléenne(bool valeur)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (m_constante_booléenne) {
        if (it.valeur == valeur) {
            return &it;
        }
    }
#endif
    return m_constante_booléenne.ajoute_élément(TypeBase::BOOL, valeur);
}

AtomeConstanteCaractère *ConstructriceRI::crée_constante_caractère(Type const *type,
                                                                   uint64_t valeur)
{
    return m_constante_caractère.ajoute_élément(type, valeur);
}

AtomeConstanteNulle *ConstructriceRI::crée_constante_nulle(Type const *type)
{
#ifdef DEDUPLIQUE_CONSTANTE
    POUR_TABLEAU_PAGE (m_constante_nulle) {
        if (it.type == type) {
            return &it;
        }
    }
#endif
    return m_constante_nulle.ajoute_élément(type);
}

InstructionBranche *ConstructriceRI::crée_branche(NoeudExpression const *site_,
                                                  InstructionLabel *label,
                                                  bool crée_seulement)
{
    auto inst = m_branche.ajoute_élément(site_, label);

    label->drapeaux |= DrapeauxAtome::EST_UTILISÉ;

    if (!crée_seulement) {
        insère(inst);
    }

    return inst;
}

InstructionBrancheCondition *ConstructriceRI::crée_branche_condition(
    NoeudExpression const *site_,
    Atome *valeur,
    InstructionLabel *label_si_vrai,
    InstructionLabel *label_si_faux)
{
    auto inst = m_branche_cond.ajoute_élément(site_, valeur, label_si_vrai, label_si_faux);

    label_si_vrai->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
    label_si_faux->drapeaux |= DrapeauxAtome::EST_UTILISÉ;

    insère(inst);
    return inst;
}

InstructionLabel *ConstructriceRI::crée_label(NoeudExpression const *site_)
{
    auto inst = m_label.ajoute_élément(site_, m_nombre_labels++);
    insère_label(inst);
    return inst;
}

InstructionLabel *ConstructriceRI::réserve_label(NoeudExpression const *site_)
{
    return m_label.ajoute_élément(site_, m_nombre_labels++);
}

/* Pour dédupliquer les chargements de mémoire, afin de réduire la mémoire utilisée. */
#define DEDUPLIQUE_CHARGES

void ConstructriceRI::insère_label(InstructionLabel *label)
{
    /* La génération de code pour les conditions (#si, #saufsi) et les boucles peut ajouter des
     * labels redondants (par exemple les labels pour après la condition ou la boucle) quand ces
     * instructions sont conséquentes. Afin d'éviter d'avoir des labels définissant des blocs
     * vides, nous ajoutons des branches implicites. */
    if (!m_fonction_courante->instructions.est_vide()) {
        auto di = m_fonction_courante->dernière_instruction();
        /* Nous pourrions avoir `if (di->est_label())` pour détecter des labels de blocs vides,
         * mais la génération de code pour par exemple les conditions d'une instructions `si` sans
         * `sinon` ne met pas de branche à la fin de `si.bloc_si_vrai`. Donc ceci permet de
         * détecter également ces cas. */
        if (!di->est_terminatrice()) {
            crée_branche(label->site, label);
        }
    }

#ifdef DEDUPLIQUE_CHARGES
    /* Invalide tous les chargements car nous pouvons réordonner les blocs. */
    m_charges.efface();
#endif

    insère(label);
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
    auto inst = m_retour.ajoute_élément(site_, valeur);
    insère(inst);
    return inst;
}

InstructionStockeMem *ConstructriceRI::crée_stocke_mem(NoeudExpression const *site_,
                                                       Atome *ou,
                                                       Atome *valeur,
                                                       bool crée_seulement)
{
    assert_rappel(ou->type->est_type_pointeur() || ou->type->est_type_référence(), [&]() {
        dbg() << "Le type n'est pas un pointeur : " << chaine_type(ou->type) << '\n'
              << imprime_site(site_);
    });

    assert_rappel(type_dest_et_type_source_sont_compatibles(ou->type, valeur->type), [&]() {
        auto type_élément_dest = type_déréférencé_pour(ou->type);
        dbg() << "\tType élément destination : " << chaine_type(type_élément_dest) << " ("
              << type_élément_dest << ") "
              << ", type source : " << chaine_type(valeur->type) << " (" << valeur->type << ")\n"
              << imprime_site(site_);
    });

    auto inst = m_stocke_mem.ajoute_élément(site_, ou, valeur);

    if (!crée_seulement) {
        insère(inst);
    }

#ifdef DEDUPLIQUE_CHARGES
    invalide_charge(ou);
#endif

    return inst;
}

InstructionChargeMem *ConstructriceRI::crée_charge_mem(NoeudExpression const *site_,
                                                       Atome *ou,
                                                       bool crée_seulement)
{
    /* nous chargeons depuis une adresse en mémoire, donc nous devons avoir un pointeur */
    assert_rappel(ou->type->est_type_pointeur() || ou->type->est_type_référence(), [&]() {
        dbg() << "Le type est '" << chaine_type(ou->type) << "'\n" << imprime_site(site_);
    });

    assert_rappel(
        ou->genre_atome == Atome::Genre::INSTRUCTION || ou->genre_atome == Atome::Genre::GLOBALE,
        [=]() {
            dbg() << "Le genre de l'atome est : " << static_cast<int>(ou->genre_atome) << ".";
        });

    auto type = type_déréférencé_pour(ou->type);

#ifdef DEDUPLIQUE_CHARGES
    auto inst_existante = donne_charge(ou);
    if (inst_existante) {
        return inst_existante;
    }
#endif

    auto inst = m_charge.ajoute_élément(site_, type, ou);

    if (!crée_seulement) {
        insère(inst);
    }

    m_charges.ajoute(inst);
    return inst;
}

InstructionAppel *ConstructriceRI::crée_appel(NoeudExpression const *site_, Atome *appelé)
{
    auto inst = m_appel.ajoute_élément(site_, appelé);
    insère(inst);
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
        type_fonction = appelé->type->comme_type_pointeur()->type_pointé->comme_type_fonction();
    }

    POUR_INDEX (type_fonction->types_entrées) {
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

    auto inst = m_appel.ajoute_élément(site_, appelé, std::move(args));
    insère(inst);
    return inst;
}

Atome *ConstructriceRI::crée_op_unaire(NoeudExpression const *site_,
                                       Type const *type,
                                       OpérateurUnaire::Genre op,
                                       Atome *valeur)
{
    switch (op) {
        case OpérateurUnaire::Genre::Négation:
        {
            if (valeur->est_constante_réelle()) {
                auto const constante_réelle = valeur->comme_constante_réelle();
                auto const valeur_réelle = -constante_réelle->valeur;
                return crée_constante_nombre_réel(type, valeur_réelle);
            }

            if (valeur->est_constante_entière()) {
                auto const constante_entière = valeur->comme_constante_entière();
                auto const valeur_entière = -int64_t(constante_entière->valeur);
                return crée_constante_nombre_entier(type, uint64_t(valeur_entière));
            }

            break;
        }
        case OpérateurUnaire::Genre::Négation_Binaire:
        {
            if (valeur->est_constante_entière()) {
                auto const constante_entière = valeur->comme_constante_entière();
                auto const valeur_entière = ~constante_entière->valeur;
                return crée_constante_nombre_entier(type, valeur_entière);
            }

            break;
        }
        case OpérateurUnaire::Genre::Invalide:
        {
            break;
        }
        case OpérateurUnaire::Genre::Positif:
        {
            return valeur;
        }
    }

    auto inst = m_op_unaire.ajoute_élément(site_, type, op, valeur);
    insère(inst);
    return inst;
}

Atome *ConstructriceRI::crée_op_binaire(NoeudExpression const *site_,
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

    if (valeur_gauche->est_constante() && !valeur_droite->est_constante()) {
        if (op == OpérateurBinaire::Genre::Soustraction &&
            est_constante_entière_zéro(valeur_gauche)) {
            return crée_op_unaire(site_, type, OpérateurUnaire::Genre::Négation, valeur_droite);
        }

        if (peut_permuter_opérandes(op)) {
            op = donne_opérateur_pour_permutation_opérandes(op);
            auto tmp = valeur_gauche;
            valeur_gauche = valeur_droite;
            valeur_droite = tmp;
        }
    }

    auto inst_tmp = InstructionOpBinaire(site_, type, op, valeur_gauche, valeur_droite);

    if (est_opérateur_binaire_constant(&inst_tmp)) {
        if (auto constante = évalue_opérateur_binaire(&inst_tmp, *this)) {
            return constante;
        }
    }
    else if (auto remplacement = peut_remplacer_instruction_binaire_par_opérande(&inst_tmp)) {
        return remplacement;
    }

    auto inst = m_op_binaire.ajoute_élément(site_, type, op, valeur_gauche, valeur_droite);
    insère(inst);
    return inst;
}

Atome *ConstructriceRI::crée_op_comparaison(NoeudExpression const *site_,
                                            OpérateurBinaire::Genre op,
                                            Atome *valeur_gauche,
                                            Atome *valeur_droite)
{
    return crée_op_binaire(site_, TypeBase::BOOL, op, valeur_gauche, valeur_droite);
}

InstructionAccèdeIndex *ConstructriceRI::crée_accès_index(NoeudExpression const *site_,
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
        type_élément = type_pointeur->type_pointé;
    }

    assert_rappel(
        dls::outils::est_element(
            type_élément->genre, GenreNoeud::POINTEUR, GenreNoeud::TABLEAU_FIXE) ||
            (type_élément->est_type_opaque() &&
             dls::outils::est_element(type_élément->comme_type_opaque()->type_opacifié->genre,
                                      GenreNoeud::POINTEUR,
                                      GenreNoeud::TABLEAU_FIXE)),
        [=]() { dbg() << "Type accédé : '" << chaine_type(accédé->type) << "'"; });

    auto type = m_typeuse.type_pointeur_pour(type_déréférencé_pour(type_élément), false);

    auto inst = m_acces_index.ajoute_élément(site_, type, accédé, index);
    insère(inst);
    return inst;
}

InstructionAccèdeMembre *ConstructriceRI::crée_référence_membre(
    NoeudExpression const *site_, Type const *type, Atome *accédé, int index, bool crée_seulement)
{

    auto inst = m_acces_membre.ajoute_élément(site_, type, accédé, index);
    if (!crée_seulement) {
        insère(inst);
    }
    return inst;
}

InstructionAccèdeMembre *ConstructriceRI::crée_référence_membre(NoeudExpression const *site_,
                                                                Atome *accédé,
                                                                int index,
                                                                bool crée_seulement)
{
    assert_rappel(accédé->type->est_type_pointeur() || accédé->type->est_type_référence(),
                  [=]() { dbg() << "Type accédé : '" << chaine_type(accédé->type) << "'"; });

    auto type_élément = type_déréférencé_pour(accédé->type);
    if (type_élément->est_type_opaque()) {
        type_élément = type_élément->comme_type_opaque()->type_opacifié;
    }

    assert_rappel(type_élément->est_type_composé(), [&]() {
        dbg() << "Type accédé : '" << chaine_type(type_élément) << "'\n" << imprime_site(site_);
    });

    auto type_composé = type_élément->comme_type_composé();
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

union ValeurConstanteEntière {
    uint8_t nat8;
    uint16_t nat16;
    uint32_t nat32;
    uint64_t nat64;

    int8_t rel8;
    int16_t rel16;
    int32_t rel32;
    int64_t rel64;
};

template <typename T>
T donne_valeur_typée(ValeurConstanteEntière valeur);

template <typename T>
void définis_valeur_typée(ValeurConstanteEntière &valeur, T v);

#define DEFINIS_ACCESSEURS(type, nom_membre)                                                      \
    template <>                                                                                   \
    type donne_valeur_typée(ValeurConstanteEntière valeur)                                        \
    {                                                                                             \
        return valeur.nom_membre;                                                                 \
    }                                                                                             \
    template <>                                                                                   \
    void définis_valeur_typée(ValeurConstanteEntière &valeur, type v)                             \
    {                                                                                             \
        valeur.nom_membre = v;                                                                    \
    }

DEFINIS_ACCESSEURS(uint8_t, nat8)
DEFINIS_ACCESSEURS(uint16_t, nat16)
DEFINIS_ACCESSEURS(uint32_t, nat32)
DEFINIS_ACCESSEURS(uint64_t, nat64)
DEFINIS_ACCESSEURS(int8_t, rel8)
DEFINIS_ACCESSEURS(int16_t, rel16)
DEFINIS_ACCESSEURS(int32_t, rel32)
DEFINIS_ACCESSEURS(int64_t, rel64)

#undef DEFINIS_ACCESSEURS

template <typename TypeSource, typename TypeCible>
ValeurConstanteEntière applique_transtype(ValeurConstanteEntière valeur)
{
    auto valeur_source = donne_valeur_typée<TypeSource>(valeur);
    définis_valeur_typée(valeur, TypeCible(valeur_source));
    return valeur;
}

ValeurConstanteEntière applique_transtype(Type const *type_source,
                                          Type const *type_cible,
                                          TypeTranstypage transtypage,
                                          ValeurConstanteEntière valeur)
{
    switch (transtypage) {
        case TypeTranstypage::AUGMENTE_NATUREL:
        {
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<uint8_t, uint16_t>(valeur);
                }
            }
            if (type_cible->taille_octet == 4) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<uint8_t, uint32_t>(valeur);
                }
                if (type_source->taille_octet == 2) {
                    return applique_transtype<uint16_t, uint32_t>(valeur);
                }
            }
            if (type_source->taille_octet == 1) {
                return applique_transtype<uint8_t, uint64_t>(valeur);
            }
            if (type_source->taille_octet == 2) {
                return applique_transtype<uint16_t, uint64_t>(valeur);
            }
            if (type_source->taille_octet == 4) {
                return applique_transtype<uint32_t, uint64_t>(valeur);
            }
            break;
        }
        case TypeTranstypage::AUGMENTE_NATUREL_VERS_RELATIF:
        {
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<uint8_t, int16_t>(valeur);
                }
            }
            if (type_cible->taille_octet == 4) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<uint8_t, int32_t>(valeur);
                }
                if (type_source->taille_octet == 2) {
                    return applique_transtype<uint16_t, int32_t>(valeur);
                }
            }
            if (type_source->taille_octet == 1) {
                return applique_transtype<uint8_t, int64_t>(valeur);
            }
            if (type_source->taille_octet == 2) {
                return applique_transtype<uint16_t, int64_t>(valeur);
            }
            if (type_source->taille_octet == 4) {
                return applique_transtype<uint32_t, int64_t>(valeur);
            }
            break;
        }
        case TypeTranstypage::AUGMENTE_RELATIF:
        {
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<int8_t, int16_t>(valeur);
                }
            }
            if (type_cible->taille_octet == 4) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<int8_t, int32_t>(valeur);
                }
                if (type_source->taille_octet == 2) {
                    return applique_transtype<int16_t, int32_t>(valeur);
                }
            }
            if (type_source->taille_octet == 1) {
                return applique_transtype<int8_t, int64_t>(valeur);
            }
            if (type_source->taille_octet == 2) {
                return applique_transtype<int16_t, int64_t>(valeur);
            }
            if (type_source->taille_octet == 4) {
                return applique_transtype<int32_t, int64_t>(valeur);
            }
            break;
        }
        case TypeTranstypage::AUGMENTE_RELATIF_VERS_NATUREL:
        {
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<int8_t, uint16_t>(valeur);
                }
            }
            if (type_cible->taille_octet == 4) {
                if (type_source->taille_octet == 1) {
                    return applique_transtype<int8_t, uint32_t>(valeur);
                }
                if (type_source->taille_octet == 2) {
                    return applique_transtype<int16_t, uint32_t>(valeur);
                }
            }
            if (type_source->taille_octet == 1) {
                return applique_transtype<int8_t, uint64_t>(valeur);
            }
            if (type_source->taille_octet == 2) {
                return applique_transtype<int16_t, uint64_t>(valeur);
            }
            if (type_source->taille_octet == 4) {
                return applique_transtype<int32_t, uint64_t>(valeur);
            }
            break;
        }
        case TypeTranstypage::DIMINUE_NATUREL:
        {
            if (type_cible->taille_octet == 1) {
                if (type_source->taille_octet == 2) {
                    return applique_transtype<uint16_t, uint8_t>(valeur);
                }
                if (type_source->taille_octet == 4) {
                    return applique_transtype<uint32_t, uint8_t>(valeur);
                }
                return applique_transtype<uint64_t, uint8_t>(valeur);
            }
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 4) {
                    return applique_transtype<uint32_t, uint16_t>(valeur);
                }
                return applique_transtype<uint64_t, uint16_t>(valeur);
            }
            return applique_transtype<uint64_t, uint32_t>(valeur);
        }
        case TypeTranstypage::DIMINUE_RELATIF:
        {
            if (type_cible->taille_octet == 1) {
                if (type_source->taille_octet == 2) {
                    return applique_transtype<int16_t, int8_t>(valeur);
                }
                if (type_source->taille_octet == 4) {
                    return applique_transtype<int32_t, int8_t>(valeur);
                }
                return applique_transtype<int64_t, int8_t>(valeur);
            }
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 4) {
                    return applique_transtype<int32_t, int16_t>(valeur);
                }
                return applique_transtype<int64_t, int16_t>(valeur);
            }
            return applique_transtype<int64_t, int32_t>(valeur);
        }
        case TypeTranstypage::DIMINUE_NATUREL_VERS_RELATIF:
        {
            if (type_cible->taille_octet == 1) {
                if (type_source->taille_octet == 2) {
                    return applique_transtype<uint16_t, int8_t>(valeur);
                }
                if (type_source->taille_octet == 4) {
                    return applique_transtype<uint32_t, int8_t>(valeur);
                }
                return applique_transtype<uint64_t, int8_t>(valeur);
            }
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 4) {
                    return applique_transtype<uint32_t, int16_t>(valeur);
                }
                return applique_transtype<uint64_t, int16_t>(valeur);
            }
            return applique_transtype<uint64_t, int32_t>(valeur);
        }
        case TypeTranstypage::DIMINUE_RELATIF_VERS_NATUREL:
        {
            if (type_cible->taille_octet == 1) {
                if (type_source->taille_octet == 2) {
                    return applique_transtype<int16_t, uint8_t>(valeur);
                }
                if (type_source->taille_octet == 4) {
                    return applique_transtype<int32_t, uint8_t>(valeur);
                }
                return applique_transtype<int64_t, uint8_t>(valeur);
            }
            if (type_cible->taille_octet == 2) {
                if (type_source->taille_octet == 4) {
                    return applique_transtype<int32_t, uint16_t>(valeur);
                }
                return applique_transtype<int64_t, uint16_t>(valeur);
            }
            return applique_transtype<int64_t, uint32_t>(valeur);
        }
        case TypeTranstypage::AUGMENTE_REEL:
        case TypeTranstypage::DIMINUE_REEL:
        case TypeTranstypage::POINTEUR_VERS_ENTIER:
        case TypeTranstypage::ENTIER_VERS_POINTEUR:
        case TypeTranstypage::REEL_VERS_ENTIER_RELATIF:
        case TypeTranstypage::REEL_VERS_ENTIER_NATUREL:
        case TypeTranstypage::ENTIER_RELATIF_VERS_REEL:
        case TypeTranstypage::ENTIER_NATUREL_VERS_REEL:
        {
            break;
        }
        case TypeTranstypage::BITS:
        {
            break;
        }
    }
    return valeur;
}

static ValeurConstanteEntière donne_valeur_constante(AtomeConstanteEntière const *atome)
{
    auto ancienne_valeur = atome->valeur;
    auto résultat = ValeurConstanteEntière{};

    if (atome->type->est_type_entier_naturel()) {
        if (atome->type->taille_octet == 1) {
            résultat.nat8 = uint8_t(ancienne_valeur);
            return résultat;
        }

        if (atome->type->taille_octet == 2) {
            résultat.nat16 = uint16_t(ancienne_valeur);
            return résultat;
        }

        if (atome->type->taille_octet == 4) {
            résultat.nat32 = uint32_t(ancienne_valeur);
            return résultat;
        }

        résultat.nat64 = ancienne_valeur;
        return résultat;
    }

    if (atome->type->taille_octet == 1) {
        résultat.rel8 = int8_t(ancienne_valeur);
        return résultat;
    }

    if (atome->type->taille_octet == 2) {
        résultat.rel16 = int16_t(ancienne_valeur);
        return résultat;
    }

    if (atome->type->taille_octet == 4) {
        résultat.rel32 = int32_t(ancienne_valeur);
        return résultat;
    }

    résultat.rel64 = int64_t(ancienne_valeur);
    return résultat;
}

static uint64_t donne_valeur_pour_constante(Type const *type, ValeurConstanteEntière valeur)
{
    if (type->est_type_entier_naturel()) {
        if (type->taille_octet == 1) {
            return uint64_t(valeur.nat8);
        }
        if (type->taille_octet == 2) {
            return uint64_t(valeur.nat16);
        }
        if (type->taille_octet == 4) {
            return uint64_t(valeur.nat32);
        }
        return valeur.nat64;
    }
    if (type->taille_octet == 1) {
        return uint64_t(valeur.rel8);
    }
    if (type->taille_octet == 2) {
        return uint64_t(valeur.rel16);
    }
    if (type->taille_octet == 4) {
        return uint64_t(valeur.rel32);
    }
    return uint64_t(valeur.rel64);
}

Atome *ConstructriceRI::crée_transtype(NoeudExpression const *site_,
                                       Type const *type,
                                       Atome *valeur,
                                       TypeTranstypage op)
{
    if (valeur->est_constante_nulle()) {
        return crée_constante_nulle(type);
    }

    if (valeur->est_constante_entière()) {
        if (op == TypeTranstypage::BITS) {
            if (type->est_type_opaque() || type->est_type_énum() || type->est_type_octet()) {
                return crée_constante_nombre_entier(type,
                                                    valeur->comme_constante_entière()->valeur);
            }
        }

        if (type->est_type_bool()) {
            return crée_constante_booléenne(valeur->comme_constante_entière()->valeur != 0);
        }

        if (type->est_type_énum()) {
            return crée_constante_nombre_entier(type, valeur->comme_constante_entière()->valeur);
        }

        if (est_type_entier(type)) {
            auto valeur_entière = valeur->comme_constante_entière();
            auto ancienne_valeur = donne_valeur_constante(valeur_entière);
            auto nouvelle_valeur = applique_transtype(
                valeur_entière->type, type, op, ancienne_valeur);
            return crée_constante_nombre_entier(
                type, donne_valeur_pour_constante(type, nouvelle_valeur));
        }
    }

    if (valeur->est_constante_réelle() && type->est_type_réel()) {
        auto valeur_réelle = valeur->comme_constante_réelle();
        return crée_constante_nombre_réel(type, valeur_réelle->valeur);
    }

    if (valeur->est_constante_caractère() && est_type_entier(type)) {
        return crée_constante_nombre_entier(type, valeur->comme_constante_caractère()->valeur);
    }

    // dbg() << __func__ << ", type : " << chaine_type(type) << ", valeur " <<
    // chaine_type(valeur->type);
    auto inst = m_transtype.ajoute_élément(site_, type, valeur, op);
    insère(inst);
    return inst;
}

TranstypeConstant *ConstructriceRI::crée_transtype_constant(Type const *type,
                                                            AtomeConstante *valeur)
{
    return m_transtype_constant.ajoute_élément(type, valeur);
}

AccèdeIndexConstant *ConstructriceRI::crée_accès_index_constant(AtomeConstante *accédé,
                                                                int64_t index)
{
    assert_rappel(accédé->type->est_type_pointeur(),
                  [=]() { dbg() << "Type accédé : '" << chaine_type(accédé->type) << "'"; });
    auto type_pointeur = accédé->type->comme_type_pointeur();
    assert_rappel(
        dls::outils::est_element(
            type_pointeur->type_pointé->genre, GenreNoeud::POINTEUR, GenreNoeud::TABLEAU_FIXE),
        [=]() { dbg() << "Type accédé : '" << chaine_type(type_pointeur->type_pointé) << "'"; });

    auto type = m_typeuse.type_pointeur_pour(type_déréférencé_pour(type_pointeur->type_pointé),
                                             false);

    return m_accès_index_constant.ajoute_élément(type, accédé, index);
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
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::FONCTION:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            return crée_constante_nulle(type);
        }
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            return crée_constante_nombre_entier(type, 0);
        }
        case GenreNoeud::ENTIER_CONSTANT:
        {
            return crée_constante_nombre_entier(TypeBase::Z32, 0);
        }
        case GenreNoeud::RÉEL:
        {
            if (type->taille_octet == 2) {
                return crée_constante_nombre_entier(TypeBase::N16, 0);
            }

            return crée_constante_nombre_réel(type, 0.0);
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();
            auto valeur = crée_initialisation_défaut_pour_type(type_tableau->type_pointé);
            return crée_initialisation_tableau(type_tableau, valeur);
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type->comme_type_union();

            if (type_union->est_nonsure) {
                return crée_initialisation_défaut_pour_type(type_union->type_le_plus_grand);
            }

            auto valeurs = kuri::tableau<AtomeConstante *>();
            valeurs.réserve(2);

            valeurs.ajoute(crée_initialisation_défaut_pour_type(type_union->type_le_plus_grand));
            valeurs.ajoute(crée_initialisation_défaut_pour_type(TypeBase::Z32));

            return crée_constante_structure(type, std::move(valeurs));
        }
        case GenreNoeud::CHAINE:
        case GenreNoeud::EINI:
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TYPE_TRANCHE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::TUPLE:
        {
            auto type_composé = type->comme_type_composé();
            auto valeurs = kuri::tableau<AtomeConstante *>();
            valeurs.réserve(type_composé->membres.taille());

            POUR (type_composé->donne_membres_pour_code_machine()) {
                auto valeur = crée_initialisation_défaut_pour_type(it.type);
                valeurs.ajoute(valeur);
            }

            return crée_constante_structure(type, std::move(valeurs));
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        {
            auto type_enum = static_cast<TypeEnum const *>(type);
            return crée_constante_nombre_entier(type_enum, 0);
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            auto valeur = crée_initialisation_défaut_pour_type(type_opaque->type_opacifié);
            valeur->type = type_opaque;
            return valeur;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            return nullptr;
        }
    }

    return nullptr;
}

InstructionInatteignable *ConstructriceRI::crée_inatteignable(const NoeudExpression *site,
                                                              bool crée_seulement)
{
    auto résultat = m_inatteignable.ajoute_élément(site);
    if (!crée_seulement) {
        insère(résultat);
    }
    return résultat;
}

InstructionSélection *ConstructriceRI::crée_sélection(NoeudExpression const *site,
                                                      bool crée_seulement)
{
    auto résultat = m_sélection.ajoute_élément(site);
    if (!crée_seulement) {
        insère(résultat);
    }
    return résultat;
}

void ConstructriceRI::insère(Instruction *inst)
{
    m_fonction_courante->instructions.ajoute(inst);
}

InstructionChargeMem *ConstructriceRI::donne_charge(Atome *source)
{
    POUR (m_charges) {
        if (it && it->chargée == source) {
            return it;
        }
    }

    return nullptr;
}

void ConstructriceRI::invalide_charge(Atome *source)
{
    POUR (m_charges) {
        if (it && it->chargée == source) {
            it = nullptr;
        }
    }
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

    auto &stats_gaspillage = stats.stats_gaspillage;
#define ENUMERE_GENRE_ATOME_EX(genre, classe, ident)                                              \
    if (m_##ident.taille() != 0) {                                                                \
        stats_ri.fusionne_entrée({ #ident, m_##ident.taille(), m_##ident.mémoire_utilisée() });   \
        stats_gaspillage.fusionne_entrée({ #ident, 1, m_##ident.gaspillage_mémoire() });          \
    }

    ENUMERE_GENRE_ATOME(ENUMERE_GENRE_ATOME_EX)
    ENUMERE_GENRE_INSTRUCTION(ENUMERE_GENRE_ATOME_EX)
#undef ENUMERE_GENRE_ATOME_EX

    auto mémoire_args_appel = 0;
    auto gaspillage_args_appel = 0;

    pour_chaque_élément(m_appel, [&](InstructionAppel const &it) {
        mémoire_args_appel += it.args.taille_mémoire();
        gaspillage_args_appel += it.args.gaspillage_mémoire();
    });

    stats_ri.fusionne_entrée({"arguments_insts_appel", m_appel.taille(), mémoire_args_appel});

    stats_gaspillage.fusionne_entrée({"Arguments Appel RI", 1, gaspillage_args_appel});
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
    EspaceDeTravail *espace, NoeudDéclarationEntêteFonction *fonction)
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
        auto graphe = compilatrice.graphe_dépendance.verrou_ecriture();
        graphe->prépare_visite();

        POUR (globales_avec_déclarations) {
            auto noeud = it->decl->noeud_dépendance;
            assert(noeud);

            if (rassembleuse_atomes.possède(it)) {
                continue;
            }

            graphe->traverse(noeud, [&](NoeudDépendance const *relation) {
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

    std::rotate(fonction_pour->instructions.begin() + fonction_pour->décalage_appel_init_globale,
                fonction_pour->instructions.end() - 1,
                fonction_pour->instructions.end());

    définis_fonction_courante(nullptr);

    return fonction;
}

void CompilatriceRI::définis_fonction_courante(AtomeFonction *fonction_courante)
{
    m_fonction_courante = fonction_courante;
    m_label_après_controle = nullptr;
    m_pile.efface();
    m_constructrice.définis_fonction_courante(fonction_courante);
}

AtomeConstante *CompilatriceRI::crée_tranche_globale(IdentifiantCode &ident,
                                                     const Type *type,
                                                     kuri::tableau<AtomeConstante *> &&valeurs)
{
    return m_constructrice.crée_tranche_globale(ident, type, std::move(valeurs));
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
    else if (type->est_type_énum()) {
        auto type_enum = static_cast<TypeEnum const *>(type);
        auto type_sousjacent = type_enum->type_sous_jacent;
        auto &typeuse = m_compilatrice.typeuse;
        auto type_ptr_ptr_rien = typeuse.type_pointeur_pour(type_sousjacent);
        argument = m_constructrice.crée_transtype(
            site_, type_ptr_ptr_rien, argument, TypeTranstypage::BITS);
    }
    else if (argument->type->comme_type_pointeur()->type_pointé->est_type_énum()) {
        auto type_enum = static_cast<TypeEnum const *>(
            argument->type->comme_type_pointeur()->type_pointé);
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

        if (noeud_pour->corps_opérateur_pour) {
            auto directive = noeud_pour->corps_opérateur_pour->corps_boucle;
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
        case GenreNoeud::COMMENTAIRE:
        case GenreNoeud::DÉCLARATION_BIBLIOTHÈQUE:
        case GenreNoeud::DIRECTIVE_DÉPENDANCE_BIBLIOTHÈQUE:
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::DÉCLARATION_OPAQUE:
        case GenreNoeud::EXPRESSION_PLAGE:
        case GenreNoeud::EXPRESSION_VIRGULE:
        case GenreNoeud::INSTRUCTION_CHARGE:
        case GenreNoeud::INSTRUCTION_EMPL:
        case GenreNoeud::INSTRUCTION_IMPORTE:
        case GenreNoeud::INSTRUCTION_NON_INITIALISATION:
        case GenreNoeud::DÉCLARATION_MODULE:
        case GenreNoeud::EXPRESSION_PAIRE_DISCRIMINATION:
        case GenreNoeud::OCTET:
        case GenreNoeud::RÉEL:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_RELATIF:
        case GenreNoeud::ENTIER_NATUREL:
        case GenreNoeud::CHAINE:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::EINI:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::BOOL:
        case GenreNoeud::RIEN:
        case GenreNoeud::TABLEAU_FIXE:
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        case GenreNoeud::TYPE_TRANCHE:
        case GenreNoeud::FONCTION:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::TYPE_DE_DONNÉES:
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::TUPLE:
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_FIXE:
        case GenreNoeud::EXPRESSION_TYPE_TABLEAU_DYNAMIQUE:
        case GenreNoeud::EXPRESSION_TYPE_TRANCHE:
        case GenreNoeud::EXPRESSION_TYPE_FONCTION:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        case GenreNoeud::DIRECTIVE_FONCTION:
        {
            break;
        }
        case GenreNoeud::DÉCLARATION_CONSTANTE:
        {
            assert_rappel(!noeud->possède_drapeau(DrapeauxNoeud::EST_GLOBALE), [&]() {
                dbg() << "Obtenu une constante dans la RI.\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        /* Les déclarations de structures doivent passer par les fonctions d'initialisation. */
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        case GenreNoeud::DÉCLARATION_UNION:
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
        case GenreNoeud::DIRECTIVE_PRÉ_EXÉCUTABLE:
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
        case GenreNoeud::DIRECTIVE_EXÉCUTE:
        {
            auto directive = noeud->comme_exécute();
            assert_rappel(directive->ident == ID::assert_, [&]() {
                dbg() << "Erreur interne : une directive ne fut pas simplifié !\n"
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
        case GenreNoeud::EXPRESSION_PARENTHÈSE:
        case GenreNoeud::EXPRESSION_TYPE_DE:
        case GenreNoeud::INSTRUCTION_DISCR:
        case GenreNoeud::INSTRUCTION_DISCR_ÉNUM:
        case GenreNoeud::INSTRUCTION_DISCR_UNION:
        case GenreNoeud::INSTRUCTION_POUR:
        case GenreNoeud::INSTRUCTION_RETIENS:
        case GenreNoeud::OPÉRATEUR_COMPARAISON_CHAINÉE:
        case GenreNoeud::DIRECTIVE_CORPS_BOUCLE:
        case GenreNoeud::DIRECTIVE_INTROSPECTION:
        case GenreNoeud::DÉCLARATION_OPÉRATEUR_POUR:
        case GenreNoeud::EXPRESSION_ASSIGNATION_LOGIQUE:
        case GenreNoeud::INSTRUCTION_TENTE:
        {
            assert_rappel(false, [&]() {
                dbg() << "Erreur interne : un noeud ne fut pas simplifié !\n"
                      << "Le noeud est de genre : " << noeud->genre << '\n'
                      << erreur::imprime_site(*m_espace, noeud);
            });
            break;
        }
        case GenreNoeud::DÉCLARATION_ENTÊTE_FONCTION:
        {
            auto decl = noeud->comme_entête_fonction();
            génère_ri_pour_fonction(decl);
            if (!decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
                assert(decl->corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));
            }
            break;
        }
        case GenreNoeud::DÉCLARATION_CORPS_FONCTION:
        {
            auto corps = noeud->comme_corps_fonction();
            assert(corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE));
            génère_ri_pour_fonction(corps->entête);
            break;
        }
        case GenreNoeud::INSTRUCTION_DIFFÈRE:
        {
            m_instructions_diffères.ajoute(noeud);
            break;
        }
        case GenreNoeud::INSTRUCTION_COMPOSÉE:
        {
            auto noeud_bloc = noeud->comme_bloc();

            if (!m_est_dans_diffère) {
                /* Utilise le bloc parent comme sentinelle. */
                m_instructions_diffères.ajoute(noeud_bloc->bloc_parent);
            }

            auto dernière_expression = NoeudExpression::nul();
            if (!noeud_bloc->expressions->est_vide()) {
                dernière_expression = noeud_bloc->expressions->dernier_élément();
            }

            POUR (*noeud_bloc->expressions.verrou_lecture()) {
                if (it->est_entête_fonction()) {
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
                    it->est_arrête()) {
                    if (it != dernière_expression) {
                        m_constructrice.crée_label(it);
                    }
                }
            }

            auto derniere_instruction = m_fonction_courante->dernière_instruction();

            if (derniere_instruction->genre != GenreInstruction::RETOUR) {
                /* Génère le code pour toutes les instructions différées de ce bloc. */
                génère_ri_insts_différées(noeud_bloc->bloc_parent);
            }

            if (!m_est_dans_diffère) {
                /* Dépile jusqu'à la sentinelle. */
                while (m_instructions_diffères.dernier_élément() != noeud_bloc->bloc_parent) {
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
            args.réserve(expr_appel->paramètres_résolus.taille());

            /* Nous pouvons avoir des initialisations de type ici qui furent créés lors de la
             * création de fonctions d'initialisations d'autres types. */
            auto fonction_init = est_appel_fonction_initialisation(expr_appel->expression);
            if (fonction_init) {
                auto argument = expr_appel->paramètres_résolus[0];
                génère_ri_pour_expression_droite(argument, nullptr);
                auto valeur = depile_valeur();
                auto type = fonction_init->type_initialisé();
                crée_appel_fonction_init_type(expr_appel, type, valeur);
                return;
            }

            auto const est_intrinsèque = est_appel_fonction_intrinsèque(expr_appel->expression);

            POUR (expr_appel->paramètres_résolus) {
                génère_ri_pour_expression_droite(it, nullptr);
                auto valeur = depile_valeur();

                /* Crée une temporaire pour simplifier l'enlignage, car nous devrons
                 * remplacer les locales par les expressions passées, et il est plus
                 * simple de remplacer une allocation par une autre. Par contre si nous avons une
                 * instruction de chargement (indiquant une allocation, ou référence d'une chose
                 * ultimement allouée), ce n'est pas la peine de créer une temporaire. */
                if (est_intrinsèque ||
                    (valeur->est_instruction() && valeur->comme_instruction()->est_charge())) {
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

            if (atome_fonc->est_fonction()) {
                auto atome_fonction = atome_fonc->comme_fonction();
                if (atome_fonction->decl &&
                    atome_fonction->decl->possède_drapeau(DrapeauxNoeudFonction::EST_SANSRETOUR)) {
                    m_constructrice.crée_inatteignable(noeud);
                }
            }

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
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_DÉCLARATION:
        {
            auto expr_ref = noeud->comme_référence_déclaration();
            auto decl_ref = expr_ref->déclaration_référée;

            assert_rappel(decl_ref, [&]() {
                dbg() << erreur::imprime_site(*m_espace, noeud) << '\n'
                      << "La référence à la déclaration est nulle " << noeud->ident->nom << " ("
                      << chaine_type(noeud->type) << ")\n";
            });

            if (decl_ref->est_entête_fonction()) {
                auto atome_fonc = m_constructrice.trouve_ou_insère_fonction(
                    decl_ref->comme_entête_fonction());
                empile_valeur(atome_fonc);
                return;
            }

            if (decl_ref->possède_drapeau(DrapeauxNoeud::EST_GLOBALE)) {
                empile_valeur(m_constructrice.trouve_ou_insère_globale(decl_ref));
                return;
            }

            auto locale = decl_ref->comme_déclaration_symbole()->atome;
            assert_rappel(locale, [&]() {
                Enchaineuse enchaineuse;

                if (m_fonction_courante) {
                    enchaineuse << "Dans la compilation de la fonction : "
                                << nom_humainement_lisible(m_fonction_courante->decl) << " :\n";
                }

                enchaineuse << erreur::imprime_site(*m_espace, noeud) << '\n'
                            << "Aucune locale trouvée pour "
                            << (noeud->ident ? noeud->ident->nom : "<anonyme>") << " ("
                            << chaine_type(noeud->type) << ")\n"
                            << "\nLa locale fut déclarée ici :\n"
                            << erreur::imprime_site(*m_espace, decl_ref);

                dbg() << enchaineuse.chaine();
            });
            empile_valeur(locale);
            break;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE:
        {
            génère_ri_pour_accès_membre(noeud->comme_référence_membre());
            break;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_MEMBRE_UNION:
        {
            génère_ri_pour_accès_membre_union(noeud->comme_référence_membre_union());
            break;
        }
        case GenreNoeud::EXPRESSION_RÉFÉRENCE_TYPE:
        {
            auto type_de_donnees = noeud->type->comme_type_type_de_données();

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
            compile_locale(expr_ass->assignée, expr_ass->expression, {});
            break;
        }
        case GenreNoeud::EXPRESSION_ASSIGNATION_MULTIPLE:
        {
            auto expr_ass = noeud->comme_assignation_multiple();
            génère_ri_pour_assignation_variable(expr_ass->données_exprs);
            break;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE:
        {
            génère_ri_pour_déclaration_variable(noeud->comme_déclaration_variable());
            break;
        }
        case GenreNoeud::DÉCLARATION_VARIABLE_MULTIPLE:
        {
            compile_déclaration_variable_multiple(noeud->comme_déclaration_variable_multiple());
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_RÉEL:
        {
            empile_valeur(m_constructrice.crée_constante_nombre_réel(
                noeud->type, noeud->comme_littérale_réel()->valeur));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NOMBRE_ENTIER:
        {
            if (noeud->type->est_type_réel()) {
                empile_valeur(m_constructrice.crée_constante_nombre_réel(
                    noeud->type, static_cast<double>(noeud->comme_littérale_entier()->valeur)));
            }
            else {
                empile_valeur(m_constructrice.crée_constante_nombre_entier(
                    noeud->type, noeud->comme_littérale_entier()->valeur));
            }

            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_CHAINE:
        {
            auto lit_chaine = noeud->comme_littérale_chaine();
            auto chaine = compilatrice().gérante_chaine->chaine_pour_adresse(lit_chaine->valeur);

            assert_rappel(
                (noeud->lexème->chaine.taille() != 0 && chaine.taille() != 0) ||
                    (noeud->lexème->chaine.taille() == 0 && chaine.taille() == 0) ||
                    noeud->possède_drapeau(DrapeauxNoeud::LEXÈME_EST_RÉUTILISÉ_POUR_SUBSTITUTION),
                [&]() {
                    dbg() << erreur::imprime_site(*espace(), noeud) << '\n'
                          << "La chaine n'est pas de la bonne taille !\n"
                          << "Le lexème a une chaine taille de " << noeud->lexème->chaine.taille()
                          << " alors que la chaine littérale a une taille de " << chaine.taille()
                          << '\n'
                          << "L'index de la chaine est de " << lit_chaine->valeur << '\n'
                          << "La valeur de la chaine du lexème est \"" << noeud->lexème->chaine
                          << "\"\n"
                          << "La valeur de la chaine littérale est \"" << chaine << "\"";
                });

            if (m_fonction_courante == nullptr) {
                auto constante = crée_constante_pour_chaine(chaine);
                empile_valeur(constante);
                return;
            }

            auto globale = crée_globale_pour_chaine(chaine);
            auto valeur = m_constructrice.crée_charge_mem(noeud, globale);
            crée_temporaire_ou_mets_dans_place(noeud, valeur, place);
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_BOOLÉEN:
        {
            empile_valeur(
                m_constructrice.crée_constante_booléenne(noeud->comme_littérale_bool()->valeur));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_CARACTÈRE:
        {
            // À FAIRE : caractères Unicode
            auto caractere = static_cast<unsigned char>(
                noeud->comme_littérale_caractère()->valeur);
            empile_valeur(m_constructrice.crée_constante_caractère(noeud->type, caractere));
            break;
        }
        case GenreNoeud::EXPRESSION_LITTÉRALE_NUL:
        {
            empile_valeur(m_constructrice.crée_constante_nulle(noeud->type));
            break;
        }
        case GenreNoeud::OPÉRATEUR_BINAIRE:
        {
            auto expr_bin = noeud->comme_expression_binaire();

            génère_ri_pour_expression_droite(expr_bin->opérande_gauche, nullptr);
            auto valeur_gauche = depile_valeur();
            génère_ri_pour_expression_droite(expr_bin->opérande_droite, nullptr);
            auto valeur_droite = depile_valeur();
            auto résultat = m_constructrice.crée_op_binaire(
                noeud, noeud->type, expr_bin->op->genre, valeur_gauche, valeur_droite);

            if (résultat->est_constante()) {
                if (place) {
                    m_constructrice.crée_stocke_mem(noeud, place, résultat);
                    place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
                }
                else {
                    empile_valeur(résultat);
                }
                return;
            }

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
            auto type_gauche = expr_bin->opérande_gauche->type;

            if (type_gauche->est_type_opaque()) {
                type_gauche = type_gauche->comme_type_opaque()->type_opacifié;
            }

            génère_ri_pour_noeud(expr_bin->opérande_gauche);
            auto pointeur = depile_valeur();
            génère_ri_pour_expression_droite(expr_bin->opérande_droite, nullptr);
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

                    if (condition->est_constante_booléenne() &&
                        condition->comme_constante_booléenne()->valeur == false) {
                        m_constructrice.crée_branche(noeud, label2);
                    }
                    else {
                        m_constructrice.crée_branche_condition(noeud, condition, label1, label2);

                        m_constructrice.insère_label(label1);

                        auto params = kuri::tableau<Atome *, int>(2);
                        params[0] = acces_taille;
                        params[1] = valeur_;
                        m_constructrice.crée_appel(noeud, fonction, std::move(params));
                        m_constructrice.crée_inatteignable(noeud);
                    }

                    m_constructrice.insère_label(label2);

                    condition = m_constructrice.crée_op_comparaison(
                        noeud, OpérateurBinaire::Genre::Comp_Sup_Egal, valeur_, acces_taille);
                    m_constructrice.crée_branche_condition(noeud, condition, label3, label4);

                    m_constructrice.insère_label(label3);

                    auto params = kuri::tableau<Atome *, int>(2);
                    params[0] = acces_taille;
                    params[1] = valeur_;
                    m_constructrice.crée_appel(noeud, fonction, std::move(params));
                    m_constructrice.crée_inatteignable(noeud);

                    m_constructrice.insère_label(label4);
                };

            if (type_gauche->est_type_tableau_fixe()) {
                if (noeud->aide_génération_code != IGNORE_VERIFICATION) {
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

            if (type_gauche->est_type_tableau_dynamique() || type_gauche->est_type_variadique() ||
                type_gauche->est_type_tranche()) {
                if (noeud->aide_génération_code != IGNORE_VERIFICATION) {
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
                if (noeud->aide_génération_code != IGNORE_VERIFICATION) {
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
        case GenreNoeud::OPÉRATEUR_UNAIRE:
        {
            auto expr_un = noeud->comme_expression_unaire();
            génère_ri_pour_expression_droite(expr_un->opérande, nullptr);
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
            if (prise_adresse->opérande->type->est_type_référence()) {
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            }

            if (!expression_gauche) {
                valeur = crée_temporaire(noeud, valeur);
            }

            empile_valeur(valeur);
            return;
        }
        case GenreNoeud::EXPRESSION_PRISE_RÉFÉRENCE:
        {
            assert_rappel(false, [&]() {
                dbg() << "Prise de référence dans la RI :\n"
                      << erreur::imprime_site(*m_espace, noeud);
            });
            return;
        }
        case GenreNoeud::EXPRESSION_NÉGATION_LOGIQUE:
        {
            // @simplifie
            auto négation = noeud->comme_négation_logique();
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
                case GenreNoeud::TYPE_TRANCHE:
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
        case GenreNoeud::INSTRUCTION_RETOUR_MULTIPLE:
        {
            auto inst = noeud->comme_retourne();

            auto valeur_ret = static_cast<Atome *>(nullptr);

            if (inst->expression) {
                génère_ri_pour_expression_droite(inst->expression, nullptr);
                valeur_ret = depile_valeur();

                if (inst->genre == GenreNoeud::INSTRUCTION_RETOUR) {
                    /* Création manuelle d'une assignation dans le paramètre de retour.
                     * Nous ne pouvons le faire lors de la simplification sans créer de blocs
                     * inutiles, et nous avons besoin de cette assignation pour que la détection de
                     * retour de pointeurs locales fonctionne (elle se base sur la destination des
                     * stockages). */
                    auto param_sortie = m_fonction_courante->decl->param_sortie;
                    if (!param_sortie->atome) {
                        génère_ri_pour_noeud(param_sortie);
                    }
                    auto atome_valeur_retour = param_sortie->atome;
                    m_constructrice.crée_stocke_mem(inst, atome_valeur_retour, valeur_ret);
                    valeur_ret = m_constructrice.crée_charge_mem(inst, atome_valeur_retour);
                }
            }

            auto bloc_final = NoeudBloc::nul();
            if (m_fonction_courante->decl) {
                bloc_final = m_fonction_courante->decl->bloc_constantes;
            }

            génère_ri_insts_différées(bloc_final);

            if (m_fonction_courante->decl->possède_drapeau(
                    DrapeauxNoeudFonction::EST_SANSRETOUR)) {
                if (!m_fonction_courante->instructions.est_vide() &&
                    !m_fonction_courante->instructions.dernier_élément()->est_inatteignable()) {
                    /* Crée une instruction inatteignable sauf s'il y a déjà une (qui fut peut-être
                     * ajoutée par un appel). */
                    m_constructrice.crée_inatteignable(noeud);
                }
            }
            else {
                m_constructrice.crée_retour(noeud, valeur_ret);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_SAUFSI:
        case GenreNoeud::INSTRUCTION_SI:
        {
            auto inst_si = noeud->comme_si();

            auto label_si_vrai = m_constructrice.réserve_label(noeud);
            auto label_si_faux = m_constructrice.réserve_label(noeud);

            if (inst_si->condition->est_déclaration_variable()) {
                génère_ri_pour_noeud(inst_si->condition);
                auto atome = inst_si->condition->comme_déclaration_variable()->atome;
                assert(atome);

                auto valeur = m_constructrice.crée_charge_mem(inst_si->condition, atome);
                if (noeud->est_saufsi()) {
                    m_constructrice.crée_branche_condition(
                        inst_si->condition, valeur, label_si_faux, label_si_vrai);
                }
                else {
                    m_constructrice.crée_branche_condition(
                        inst_si->condition, valeur, label_si_vrai, label_si_faux);
                }
            }
            else if (noeud->genre == GenreNoeud::INSTRUCTION_SAUFSI) {
                génère_ri_pour_condition(inst_si->condition, label_si_faux, label_si_vrai);
            }
            else {
                génère_ri_pour_condition(inst_si->condition, label_si_vrai, label_si_faux);
            }

            if (inst_si->bloc_si_faux) {
                auto label_apres_instruction = m_constructrice.réserve_label(noeud);

                m_constructrice.insère_label(label_si_vrai);
                génère_ri_pour_noeud(inst_si->bloc_si_vrai);

                auto di = m_fonction_courante->dernière_instruction();
                if (!di->est_terminatrice()) {
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
        case GenreNoeud::INSTRUCTION_RÉPÈTE:
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

            if (boucle->bloc_sansarrêt) {
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

            boucle->label_pour_arrête = label_pour_arret;
            boucle->label_pour_arrête_implicite = label_pour_arret_implicite;
            boucle->label_pour_continue = label_pour_continue;
            boucle->label_pour_reprends = label_boucle;

            m_constructrice.insère_label(label_boucle);
            génère_ri_pour_noeud(boucle->bloc);

            auto di = m_fonction_courante->dernière_instruction();

            if (boucle->bloc_inc) {
                m_constructrice.insère_label(label_pour_bloc_inc);
                génère_ri_pour_noeud(boucle->bloc_inc);

                if (di->est_terminatrice()) {
                    m_constructrice.crée_branche(noeud, label_boucle);
                }
            }

            if (!di->est_terminatrice()) {
                m_constructrice.crée_branche(noeud, label_boucle);
            }

            if (boucle->bloc_sansarrêt) {
                m_constructrice.insère_label(label_pour_sansarret);
                génère_ri_pour_noeud(boucle->bloc_sansarrêt);
                di = m_fonction_courante->dernière_instruction();
                if (!di->est_terminatrice()) {
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
        case GenreNoeud::INSTRUCTION_ARRÊTE:
        {
            auto inst = noeud->comme_arrête();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlée);

            if (inst->possède_drapeau(DrapeauxNoeud::EST_IMPLICITE)) {
                auto label = boucle_controlee->comme_boucle()->label_pour_arrête_implicite;
                m_constructrice.crée_branche(noeud, label);
            }
            else {
                auto label = boucle_controlee->comme_boucle()->label_pour_arrête;
                génère_ri_insts_différées(boucle_controlee->bloc_parent);
                m_constructrice.crée_branche(noeud, label);
            }

            break;
        }
        case GenreNoeud::INSTRUCTION_CONTINUE:
        {
            auto inst = noeud->comme_continue();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlée);
            auto label = boucle_controlee->comme_boucle()->label_pour_continue;
            génère_ri_insts_différées(boucle_controlee->bloc_parent);
            m_constructrice.crée_branche(noeud, label);
            break;
        }
        case GenreNoeud::INSTRUCTION_REPRENDS:
        {
            auto inst = noeud->comme_reprends();
            auto boucle_controlee = boucle_controlée_effective(inst->boucle_controlée);
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
                auto type_tableau_dyn = m_compilatrice.typeuse.crée_type_tranche(noeud->type);
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
                auto type_tableau_dyn = m_compilatrice.typeuse.crée_type_tranche(noeud->type);
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

            auto valeur = convertis_vers_tranche(
                noeud, pointeur_tableau, type_tableau_fixe, nullptr);
            empile_valeur(valeur);
            break;
        }
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU:
        case GenreNoeud::EXPRESSION_CONSTRUCTION_TABLEAU_TYPÉ:
        {
            auto expr = noeud->comme_construction_tableau();
            génère_ri_pour_construction_tableau(expr, place);
            break;
        }
        case GenreNoeud::EXPRESSION_INFO_DE:
        {
            auto inst = noeud->comme_info_de();
            auto enfant = inst->expression;
            auto type = enfant->type;
            type = type->comme_type_type_de_données()->type_connu;
            auto valeur = crée_info_type(type, noeud);

            /* utilise une temporaire pour simplifier la compilation d'expressions du style :
             * info_de(z32).id */
            crée_temporaire_ou_mets_dans_place(noeud, valeur, place);
            break;
        }
        case GenreNoeud::EXPRESSION_INIT_DE:
        {
            auto type_fonction = noeud->type->comme_type_fonction();
            auto type_pointeur = type_fonction->types_entrées[0];
            auto type_arg = type_pointeur->comme_type_pointeur()->type_pointé;
            assert_rappel(type_arg->fonction_init, [&]() {
                dbg() << "Aucune fonction init pour " << chaine_type(type_arg);
                dbg() << erreur::imprime_site(*espace(), noeud);
            });
            empile_valeur(m_constructrice.trouve_ou_insère_fonction(type_arg->fonction_init));
            break;
        }
        case GenreNoeud::EXPRESSION_TAILLE_DE:
        {
            auto expr = noeud->comme_taille_de();
            auto expr_type = expr->expression;
            auto type = expr_type->type;
            type = type->comme_type_type_de_données()->type_connu;
            auto constante = m_constructrice.crée_constante_taille_de(type);
            empile_valeur(constante);
            break;
        }
        case GenreNoeud::EXPRESSION_MÉMOIRE:
        {
            auto inst_mem = noeud->comme_mémoire();
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
        case GenreNoeud::EXPRESSION_SÉLECTION:
        {
            auto sélection = noeud->comme_sélection();
            génère_ri_pour_expression_droite(sélection->condition, nullptr);
            auto valeur_condition = depile_valeur();
            génère_ri_pour_expression_droite(sélection->si_vrai, nullptr);
            auto valeur_si_vrai = depile_valeur();
            génère_ri_pour_expression_droite(sélection->si_faux, nullptr);
            auto valeur_si_faux = depile_valeur();

            auto inst = m_constructrice.crée_sélection(noeud, false);
            inst->type = noeud->type;
            inst->condition = valeur_condition;
            inst->si_vrai = valeur_si_vrai;
            inst->si_faux = valeur_si_faux;

            empile_valeur(inst);
            break;
        }
    }
}

void CompilatriceRI::génère_ri_pour_fonction(NoeudDéclarationEntêteFonction *decl)
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

static TypeTranstypage donne_type_transtypage_pour_défaut(Type const *src,
                                                          Type const *dst,
                                                          NoeudExpression const *noeud,
                                                          EspaceDeTravail const &espace)
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

    if (src->est_type_énum()) {
        auto type_énum = src->comme_type_énum();
        src = type_énum->type_sous_jacent;
    }

    if (dst->est_type_énum()) {
        auto type_énum = dst->comme_type_énum();
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

    if (src->est_type_octet()) {
        if (dst->est_type_entier_relatif()) {
            if (src->taille_octet < dst->taille_octet) {
                return TypeTranstypage::AUGMENTE_RELATIF;
            }

            return TypeTranstypage::BITS;
        }

        if (dst->est_type_entier_naturel()) {
            if (src->taille_octet < dst->taille_octet) {
                return TypeTranstypage::AUGMENTE_RELATIF_VERS_NATUREL;
            }

            return TypeTranstypage::BITS;
        }
    }

    assert_rappel(false, [&]() {
        dbg() << "Type transtypage défaut non-géré : " << chaine_type(src_originale) << " -> "
              << chaine_type(dst_originale) << "\n"
              << erreur::imprime_site(espace, noeud);
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
                                           NoeudDéclarationEntêteFonction *fonction) {
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

                if (transformation.type_cible->est_type_réel()) {
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
                m_constructrice.crée_inatteignable(noeud);
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

            if (noeud->genre != GenreNoeud::EXPRESSION_LITTÉRALE_NUL) {
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
            auto type_transtypage = donne_type_transtypage_pour_défaut(
                type_valeur, type_cible, noeud, *espace());

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

            if (noeud->type->est_type_réel()) {
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

            if (noeud->type->est_type_réel()) {
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
            Atome *info_type = crée_info_type(noeud->type, noeud);

            auto pointeur_type_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                m_compilatrice.typeuse.type_info_type_, false, false);
            if (info_type->type != pointeur_type_info_type) {
                info_type = m_constructrice.crée_transtype(
                    noeud, pointeur_type_info_type, info_type, TypeTranstypage::BITS);
            }

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
                    auto type_pointe = noeud->type->comme_type_pointeur()->type_pointé;
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
                    auto type_pointer = noeud->type->comme_type_tableau_dynamique()->type_pointé;

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
                case GenreNoeud::TYPE_TRANCHE:
                {
                    auto type_élément = noeud->type->comme_type_tranche()->type_élément;

                    valeur_pointeur = m_constructrice.crée_reference_membre_et_charge(
                        noeud, valeur, 0);
                    valeur_pointeur = m_constructrice.crée_transtype(
                        noeud, type_cible, valeur_pointeur, TypeTranstypage::BITS);
                    valeur_taille = m_constructrice.crée_reference_membre_et_charge(
                        noeud, valeur, 1);

                    auto taille_type = type_élément->taille_octet;

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
                    auto type_pointe = type_tabl->type_pointé;
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
        case TypeTransformation::CONVERTI_TABLEAU_FIXE_VERS_TRANCHE:
        {
            if (m_fonction_courante == nullptr) {
                auto valeur_tableau_fixe = static_cast<AtomeConstante *>(valeur);
                auto ident = m_compilatrice.donne_identifiant_pour_globale("tableau_convertis");
                empile_valeur(m_constructrice.crée_tranche_globale(*ident, valeur_tableau_fixe));
                return;
            }

            valeur = convertis_vers_tranche(
                noeud, valeur, noeud->type->comme_type_tableau_fixe(), place);

            if (place == nullptr) {
                valeur = m_constructrice.crée_charge_mem(noeud, valeur);
            }
            else {
                place->drapeaux |= DrapeauxAtome::EST_UTILISÉ;
            }

            break;
        }
        case TypeTransformation::CONVERTI_TABLEAU_DYNAMIQUE_VERS_TRANCHE:
        {
            if (m_fonction_courante == nullptr) {
                assert_rappel(false, []() {
                    dbg() << "La conversion de tableaux dynamiques globaux en tranches n'est pas "
                             "implémentée.";
                });
                return;
            }

            valeur = convertis_vers_tranche(
                noeud, valeur, noeud->type->comme_type_tableau_dynamique(), place);

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
    auto accede = noeud->accédée;
    auto type_accede = accede->type;

    génère_ri_pour_noeud(accede);
    auto pointeur_accede = depile_valeur();

    while (type_accede->est_type_pointeur() || type_accede->est_type_référence()) {
        type_accede = type_déréférencé_pour(type_accede);
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
    génère_ri_pour_noeud(noeud->accédée);
    auto ptr_union = depile_valeur();
    auto type = noeud->accédée->type;

    while (type->est_type_pointeur() || type->est_type_référence()) {
        type = type_déréférencé_pour(type);
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
        m_constructrice.crée_inatteignable(noeud);
        m_constructrice.insère_label(label_si_faux);
    }

    Atome *pointeur_membre = m_constructrice.crée_référence_membre(noeud, ptr_union, 0);

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
    auto genre_lexeme = condition->lexème->genre;

    if (est_opérateur_comparaison(genre_lexeme) ||
        condition->possède_drapeau(DrapeauxNoeud::ACCES_EST_ENUM_DRAPEAU)) {
        génère_ri_pour_expression_droite(condition, nullptr);
        auto valeur = depile_valeur();
        m_constructrice.crée_branche_condition(condition, valeur, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexème::ESP_ESP) {
        auto expr_bin = condition->comme_expression_logique();
        auto cond1 = expr_bin->opérande_gauche;
        auto cond2 = expr_bin->opérande_droite;

        auto nouveau_label = m_constructrice.réserve_label(condition);
        génère_ri_pour_condition(cond1, nouveau_label, label_si_faux);
        m_constructrice.insère_label(nouveau_label);
        génère_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexème::BARRE_BARRE) {
        auto expr_bin = condition->comme_expression_logique();
        auto cond1 = expr_bin->opérande_gauche;
        auto cond2 = expr_bin->opérande_droite;

        auto nouveau_label = m_constructrice.réserve_label(condition);
        génère_ri_pour_condition(cond1, label_si_vrai, nouveau_label);
        m_constructrice.insère_label(nouveau_label);
        génère_ri_pour_condition(cond2, label_si_vrai, label_si_faux);
    }
    else if (genre_lexeme == GenreLexème::EXCLAMATION) {
        auto expr_unaire = condition->comme_négation_logique();
        génère_ri_pour_condition(expr_unaire->opérande, label_si_faux, label_si_vrai);
    }
    else if (genre_lexeme == GenreLexème::VRAI) {
        m_constructrice.crée_branche(condition, label_si_vrai);
    }
    else if (genre_lexeme == GenreLexème::FAUX) {
        m_constructrice.crée_branche(condition, label_si_faux);
    }
    else if (condition->genre == GenreNoeud::EXPRESSION_PARENTHÈSE) {
        auto expr_unaire = condition->comme_parenthèse();
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
        type_condition = type_condition->comme_type_opaque()->type_opacifié;
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
        case GenreNoeud::TYPE_TRANCHE:
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
        auto instruction_differee = expr->comme_diffère();
        génère_ri_pour_noeud(instruction_differee->expression);
    }

    m_est_dans_diffère = false;
}

AtomeConstante *CompilatriceRI::crée_tableau_annotations_pour_info_membre(
    kuri::tableau<Annotation, int> const &annotations)
{
    if (annotations.est_vide() && m_globale_annotations_vides) {
        return m_globale_annotations_vides;
    }

    kuri::tableau<AtomeConstante *> valeurs_annotations;
    valeurs_annotations.réserve(annotations.taille());

    auto type_annotation = m_compilatrice.typeuse.type_annotation;
    auto type_pointeur_annotation = m_compilatrice.typeuse.type_pointeur_pour(type_annotation);

    POUR (annotations) {
        auto annotation_existante = m_registre_annotations.trouve_globale_pour_annotation(it);
        if (annotation_existante) {
            valeurs_annotations.ajoute(annotation_existante);
            continue;
        }

        kuri::tableau<AtomeConstante *> valeurs(2);
        valeurs[0] = crée_constante_pour_chaine(it.nom);
        valeurs[1] = crée_constante_pour_chaine(it.valeur);

        auto ident = m_compilatrice.donne_identifiant_pour_globale("annotations");

        auto initialisateur = m_constructrice.crée_constante_structure(type_annotation,
                                                                       std::move(valeurs));
        auto valeur = m_constructrice.crée_globale(
            *ident, type_annotation, initialisateur, false, true);

        valeurs_annotations.ajoute(valeur);
        m_registre_annotations.ajoute_annotation(it, valeur);
    }

    auto ident = m_compilatrice.donne_identifiant_pour_globale("tableau_annotations");
    auto résultat = m_constructrice.crée_tranche_globale(
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
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::ADRESSE_FONCTION, type);
            break;
        }
        case GenreNoeud::BOOL:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::BOOLÉEN, type);
            break;
        }
        case GenreNoeud::OCTET:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::OCTET, type);
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
        case GenreNoeud::RÉEL:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::RÉEL, type);
            break;
        }
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::POINTEUR:
        {
            auto type_deref = type_déréférencé_pour(type);

            /* { membres basiques, type_pointé, est_référence } */
            auto valeurs = kuri::tableau<AtomeConstante *>(3);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::POINTEUR, type);
            if (type_deref) {
                valeurs[1] = crée_info_type_avec_transtype(type_deref, site);
            }
            else {
                auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                    m_compilatrice.typeuse.type_info_type_, false);
                valeurs[1] = m_constructrice.crée_constante_nulle(type_pointeur_info_type);
            }
            valeurs[2] = m_constructrice.crée_constante_booléenne(type->est_type_référence());

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_pointeur, std::move(valeurs));
            break;
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
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
            noms_enum.réserve(type_enum->membres.taille());

            POUR (type_enum->membres) {
                if (it.drapeaux == MembreTypeComposé::EST_IMPLICITE) {
                    continue;
                }

                auto chaine_nom = crée_constante_pour_chaine(it.nom->nom);
                noms_enum.ajoute(chaine_nom);
            }

            auto ident_valeurs = m_compilatrice.donne_identifiant_pour_globale("valeurs_énums");
            auto ident_noms = m_compilatrice.donne_identifiant_pour_globale("noms_valeus_énums");
            auto tableau_valeurs = m_constructrice.crée_tranche_globale(*ident_valeurs, tableau);
            auto tableau_noms = m_constructrice.crée_tranche_globale(
                *ident_noms, TypeBase::CHAINE, std::move(noms_enum));

            /* création de l'info type */

            /* { membres basiques, nom, valeurs, membres, est_drapeau } */
            auto valeurs = kuri::tableau<AtomeConstante *>(6);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::ÉNUM, type);
            valeurs[1] = crée_constante_pour_chaine(
                donne_nom_hiérarchique(const_cast<TypeEnum *>(type_enum)));
            valeurs[2] = tableau_valeurs;
            valeurs[3] = tableau_noms;
            valeurs[4] = m_constructrice.crée_constante_booléenne(
                type_enum->est_type_enum_drapeau());
            valeurs[5] = crée_info_type(type_enum->type_sous_jacent, site);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_enum, std::move(valeurs));
            break;
        }
        case GenreNoeud::DÉCLARATION_UNION:
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
            valeurs_membres.réserve(type_union->membres.taille());

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
            AtomeConstante *info_type_plus_grand = nullptr;
            if (type_union->est_polymorphe) {
                auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                    m_compilatrice.typeuse.type_info_type_, false);
                info_type_plus_grand = m_constructrice.crée_constante_nulle(
                    type_pointeur_info_type);
            }
            else {
                info_type_plus_grand = crée_info_type_avec_transtype(
                    type_union->type_le_plus_grand, site);
            }

            // Pour les références à des globales, nous devons avoir un type pointeur.
            auto type_membre = m_compilatrice.typeuse.type_pointeur_pour(type_struct_membre,
                                                                         false);

            auto ident_valeurs = m_compilatrice.donne_identifiant_pour_globale(
                "membres_info_type");
            auto tableau_membre = m_constructrice.crée_tranche_globale(
                *ident_valeurs, type_membre, std::move(valeurs_membres));

            auto type_pointeur_info_type_union = m_compilatrice.typeuse.type_pointeur_pour(
                type_info_union, false);

            AtomeConstante *info_type_polymorphe_de_base = nullptr;
            if (type_union->polymorphe_de_base) {
                info_type_polymorphe_de_base = crée_info_type(type_union->polymorphe_de_base,
                                                              site);
            }
            else {
                info_type_polymorphe_de_base = m_constructrice.crée_constante_nulle(
                    type_pointeur_info_type_union);
            }

            auto valeurs = kuri::tableau<AtomeConstante *>(9);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::UNION, type);
            valeurs[1] = crée_constante_pour_chaine(
                donne_nom_hiérarchique(const_cast<TypeUnion *>(type_union)));
            valeurs[2] = tableau_membre;
            valeurs[3] = info_type_plus_grand;
            valeurs[4] = m_constructrice.crée_z64(type_union->décalage_index);
            valeurs[5] = m_constructrice.crée_constante_booléenne(!type_union->est_nonsure);
            valeurs[6] = m_constructrice.crée_constante_booléenne(type_union->est_polymorphe);
            valeurs[7] = crée_tableau_annotations_pour_info_membre(type_union->annotations);
            valeurs[8] = info_type_polymorphe_de_base;

            globale->initialisateur = m_constructrice.crée_constante_structure(type_info_union,
                                                                               std::move(valeurs));

            break;
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
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
            auto tableau_membre = m_constructrice.crée_tranche_globale(
                *ident_valeurs, type_membre, std::move(valeurs_membres));

            auto tableau_structs_employées = donne_tableau_pour_structs_employées(type_struct,
                                                                                  site);

            auto type_pointeur_info_type_struct = m_compilatrice.typeuse.type_pointeur_pour(
                type_info_struct, false);

            AtomeConstante *info_type_polymorphe_de_base = nullptr;
            if (type_struct->polymorphe_de_base) {
                info_type_polymorphe_de_base = crée_info_type(type_struct->polymorphe_de_base,
                                                              site);
            }
            else {
                info_type_polymorphe_de_base = m_constructrice.crée_constante_nulle(
                    type_pointeur_info_type_struct);
            }

            /* { membres basiques, nom, membres } */
            auto valeurs = kuri::tableau<AtomeConstante *>(7);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::STRUCTURE, type);
            valeurs[1] = crée_constante_pour_chaine(
                donne_nom_hiérarchique(const_cast<TypeStructure *>(type_struct)));
            valeurs[2] = tableau_membre;
            valeurs[3] = tableau_structs_employées;
            valeurs[4] = crée_tableau_annotations_pour_info_membre(type_struct->annotations);
            valeurs[5] = m_constructrice.crée_constante_booléenne(type_struct->est_polymorphe);
            valeurs[6] = info_type_polymorphe_de_base;

            globale->initialisateur = m_constructrice.crée_constante_structure(type_info_struct,
                                                                               std::move(valeurs));
            break;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto type_deref = type_déréférencé_pour(type);
            auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                m_compilatrice.typeuse.type_info_type_, false);

            /* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */
            auto valeurs = kuri::tableau<AtomeConstante *>(2);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::TABLEAU, type);
            valeurs[1] = type_deref ?
                             crée_info_type_avec_transtype(type_deref, site) :
                             m_constructrice.crée_constante_nulle(type_pointeur_info_type);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_tableau, std::move(valeurs));
            break;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto type_deref = type_déréférencé_pour(type);
            auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                m_compilatrice.typeuse.type_info_type_, false);

            /* { id, taille_en_octet, type_pointé, est_tableau_fixe, taille_fixe } */
            auto valeurs = kuri::tableau<AtomeConstante *>(2);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::TRANCHE, type);
            valeurs[1] = type_deref ?
                             crée_info_type_avec_transtype(type_deref, site) :
                             m_constructrice.crée_constante_nulle(type_pointeur_info_type);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_tranche, std::move(valeurs));
            break;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();

            /* { membres basiques, type_pointé, taille_fixe } */
            auto valeurs = kuri::tableau<AtomeConstante *>(3);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::TABLEAU_FIXE, type);
            valeurs[1] = crée_info_type_avec_transtype(type_tableau->type_pointé, site);
            valeurs[2] = m_constructrice.crée_z32(static_cast<unsigned>(type_tableau->taille));

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_tableau_fixe, std::move(valeurs));
            break;
        }
        case GenreNoeud::FONCTION:
        {
            auto type_fonction = type->comme_type_fonction();

            auto tableau_types_entrée = donne_tableau_pour_types_entrées(type_fonction, site);
            auto tableau_types_sortie = donne_tableau_pour_type_sortie(type_fonction, site);

            auto valeurs = kuri::tableau<AtomeConstante *>(4);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::FONCTION, type);
            valeurs[1] = tableau_types_entrée;
            valeurs[2] = tableau_types_sortie;
            valeurs[3] = m_constructrice.crée_constante_booléenne(false);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_fonction, std::move(valeurs));
            break;
        }
        case GenreNoeud::EINI:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::EINI, type);
            break;
        }
        case GenreNoeud::RIEN:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::RIEN, type);
            break;
        }
        case GenreNoeud::CHAINE:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::CHAINE, type);
            break;
        }
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            type->atome_info_type = crée_info_type_défaut(GenreInfoType::TYPE_DE_DONNÉES, type);
            break;
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            auto type_opacifie = type_opaque->type_opacifié;

            /* { membres basiques, nom, type_opacifié } */
            auto valeurs = kuri::tableau<AtomeConstante *>(3);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::OPAQUE, type_opaque);
            valeurs[1] = crée_constante_pour_chaine(
                donne_nom_hiérarchique(const_cast<TypeOpaque *>(type_opaque)));
            valeurs[2] = crée_info_type_avec_transtype(type_opacifie, site);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_opaque, std::move(valeurs));
            break;
        }
        case GenreNoeud::VARIADIQUE:
        {
            auto type_variadique = type->comme_type_variadique();
            auto type_élément = type_variadique->type_pointé;
            auto type_pointeur_info_type = m_compilatrice.typeuse.type_pointeur_pour(
                m_compilatrice.typeuse.type_info_type_, false);

            /* { base, type_élément } */
            auto valeurs = kuri::tableau<AtomeConstante *>(2);
            valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::VARIADIQUE, type);
            valeurs[1] = type_élément ?
                             crée_info_type_avec_transtype(type_élément, site) :
                             m_constructrice.crée_constante_nulle(type_pointeur_info_type);

            type->atome_info_type = crée_globale_info_type(
                m_compilatrice.typeuse.type_info_type_variadique, std::move(valeurs));
            break;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    // À FAIRE : il nous faut toutes les informations du type pour pouvoir générer les informations
    assert_rappel(type->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE), [type]() {
        dbg() << "Info type pour " << chaine_type(type) << " est incomplet";
    });

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

AtomeConstante *CompilatriceRI::crée_constante_info_type_pour_base(GenreInfoType index,
                                                                   Type const *pour_type)
{
    auto membres = kuri::tableau<AtomeConstante *>(3);
    remplis_membres_de_bases_info_type(membres, index, pour_type);
    return m_constructrice.crée_constante_structure(m_compilatrice.typeuse.type_info_type_,
                                                    std::move(membres));
}

void CompilatriceRI::remplis_membres_de_bases_info_type(kuri::tableau<AtomeConstante *> &valeurs,
                                                        GenreInfoType index,
                                                        Type const *pour_type)
{
    assert(valeurs.taille() == 3);
    valeurs[0] = m_constructrice.crée_z32(uint32_t(index));
    /* Puisque nous pouvons générer du code pour des architectures avec adressages en 32 ou 64
     * bits, et puisque nous pouvons exécuter sur une machine avec une architecture différente de
     * la cible de compilation, nous générons les constantes pour les taille_de lors de l'émission
     * du code machine. */
    valeurs[1] = m_constructrice.crée_constante_taille_de(pour_type);
    // L'index dans la table des types sera mis en place lors de la génération du code machine.
    valeurs[2] = m_constructrice.crée_index_table_type(pour_type);
}

AtomeGlobale *CompilatriceRI::crée_info_type_défaut(GenreInfoType index, Type const *pour_type)
{
    auto valeurs = kuri::tableau<AtomeConstante *>(3);
    remplis_membres_de_bases_info_type(valeurs, index, pour_type);
    return crée_globale_info_type(m_compilatrice.typeuse.type_info_type_, std::move(valeurs));
}

AtomeGlobale *CompilatriceRI::crée_info_type_entier(Type const *pour_type, bool est_relatif)
{
    auto valeurs = kuri::tableau<AtomeConstante *>(2);
    valeurs[0] = crée_constante_info_type_pour_base(GenreInfoType::ENTIER, pour_type);
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
    valeurs[0] = crée_constante_pour_chaine(membre.nom->nom);
    valeurs[1] = crée_info_type_avec_transtype(membre.type, site);
    valeurs[2] = m_constructrice.crée_z64(static_cast<uint64_t>(membre.decalage));
    valeurs[3] = m_constructrice.crée_z32(static_cast<unsigned>(membre.drapeaux));

    if (membre.decl) {
        if (membre.decl->est_déclaration_variable()) {
            valeurs[4] = crée_tableau_annotations_pour_info_membre(
                membre.decl->comme_déclaration_variable()->annotations);
        }
        else if (membre.decl->est_déclaration_constante()) {
            valeurs[4] = crée_tableau_annotations_pour_info_membre(
                membre.decl->comme_déclaration_constante()->annotations);
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

    kuri::tablet<AtomeConstante *, 6> valeurs_structs_employees;
    valeurs_structs_employees.réserve(type_structure->types_employés.taille());

    POUR (type_structure->types_employés) {
        valeurs_structs_employees.ajoute(crée_info_type(it->type, site));
    }

    auto tableau_existant = m_trie_structs_employées.trouve_valeur_ou_noeud_insertion(
        valeurs_structs_employees);

    if (std::holds_alternative<AtomeConstante *>(tableau_existant)) {
        auto résultat = std::get<AtomeConstante *>(tableau_existant);

        if (type_structure->types_employés.taille() == 0) {
            m_tableau_structs_employées_vide = résultat;
        }

        return résultat;
    }

    using TypeNoeudInsertion = kuri::trie<AtomeConstante *, AtomeConstante *>::Noeud;
    auto noeud_insertion = std::get<TypeNoeudInsertion *>(tableau_existant);

    auto type_info_struct = m_compilatrice.typeuse.type_info_type_structure;

    auto ident_structs_employées = m_compilatrice.donne_identifiant_pour_globale(
        "structs_employées");
    auto type_pointeur_info_struct = m_compilatrice.typeuse.type_pointeur_pour(type_info_struct,
                                                                               false);

    auto tableau_valeurs = kuri::tableau<AtomeConstante *>();
    kuri::copie_tablet_tableau(valeurs_structs_employees, tableau_valeurs);

    auto résultat = m_constructrice.crée_tranche_globale(
        *ident_structs_employées, type_pointeur_info_struct, std::move(tableau_valeurs));

    if (type_structure->types_employés.taille() == 0) {
        m_tableau_structs_employées_vide = résultat;
    }

    noeud_insertion->données = résultat;

    return résultat;
}

AtomeConstante *CompilatriceRI::donne_tableau_pour_types_entrées(const TypeFonction *type_fonction,
                                                                 const NoeudExpression *site)
{
    if (type_fonction->types_entrées.est_vide() && m_tableau_types_entrées_vide) {
        return m_tableau_types_entrées_vide;
    }

    kuri::tablet<Type *, 6> types_entrées;
    types_entrées.réserve(type_fonction->types_entrées.taille());
    POUR (type_fonction->types_entrées) {
        types_entrées.ajoute(it);
    }

    auto tableau_existant = m_trie_types_entrée_sortie.trouve_valeur_ou_noeud_insertion(
        types_entrées);

    if (std::holds_alternative<AtomeConstante *>(tableau_existant)) {
        auto résultat = std::get<AtomeConstante *>(tableau_existant);

        if (type_fonction->types_entrées.taille() == 0) {
            m_tableau_types_entrées_vide = résultat;
        }

        return résultat;
    }

    kuri::tableau<AtomeConstante *> tableau_types_entrée;
    tableau_types_entrée.réserve(types_entrées.taille());
    POUR (types_entrées) {
        tableau_types_entrée.ajoute(crée_info_type_avec_transtype(it, site));
    }

    auto type_élément = m_compilatrice.typeuse.type_pointeur_pour(
        m_compilatrice.typeuse.type_info_type_, false);

    auto ident = m_compilatrice.donne_identifiant_pour_globale("types_entrée");
    auto résultat = m_constructrice.crée_tranche_globale(
        *ident, type_élément, std::move(tableau_types_entrée));

    if (type_fonction->types_entrées.est_vide()) {
        m_tableau_types_entrées_vide = résultat;
    }

    using TypeNoeudInsertion = kuri::trie<Type *, AtomeConstante *>::Noeud;
    auto noeud_insertion = std::get<TypeNoeudInsertion *>(tableau_existant);
    noeud_insertion->données = résultat;

    return résultat;
}

AtomeConstante *CompilatriceRI::donne_tableau_pour_type_sortie(const TypeFonction *type_fonction,
                                                               const NoeudExpression *site)
{
    auto const type_sortie = type_fonction->type_sortie;
    if (type_sortie->est_type_rien() && m_tableau_types_sorties_rien) {
        return m_tableau_types_sorties_rien;
    }

    kuri::tablet<Type *, 6> types_sortie;
    if (type_sortie->est_type_tuple()) {
        auto tuple = type_sortie->comme_type_tuple();

        types_sortie.réserve(tuple->membres.taille());
        POUR (tuple->membres) {
            types_sortie.ajoute(it.type);
        }
    }
    else {
        types_sortie.réserve(1);
        types_sortie.ajoute(type_fonction->type_sortie);
    }

    auto tableau_existant = m_trie_types_entrée_sortie.trouve_valeur_ou_noeud_insertion(
        types_sortie);

    if (std::holds_alternative<AtomeConstante *>(tableau_existant)) {
        auto résultat = std::get<AtomeConstante *>(tableau_existant);

        if (type_sortie->est_type_rien()) {
            m_tableau_types_sorties_rien = résultat;
        }

        return résultat;
    }

    kuri::tableau<AtomeConstante *> tableau_types_sortie;
    tableau_types_sortie.réserve(types_sortie.taille());
    POUR (types_sortie) {
        tableau_types_sortie.ajoute(crée_info_type_avec_transtype(it, site));
    }

    auto ident = m_compilatrice.donne_identifiant_pour_globale("types_sortie");
    auto type_élément = m_compilatrice.typeuse.type_pointeur_pour(
        m_compilatrice.typeuse.type_info_type_, false);
    auto résultat = m_constructrice.crée_tranche_globale(
        *ident, type_élément, std::move(tableau_types_sortie));

    if (type_sortie->est_type_rien()) {
        m_tableau_types_sorties_rien = résultat;
    }

    return résultat;
}

Atome *CompilatriceRI::convertis_vers_tranche(NoeudExpression const *noeud,
                                              Atome *pointeur_tableau_fixe,
                                              TypeTableauFixe const *type_tableau_fixe,
                                              Atome *place)
{
    auto alloc_tranche = place;

    if (alloc_tranche == nullptr) {
        auto type_tranche = m_compilatrice.typeuse.crée_type_tranche(
            type_tableau_fixe->type_pointé);
        alloc_tranche = m_constructrice.crée_allocation(noeud, type_tranche, nullptr);
    }

    auto ptr_pointeur_donnees = m_constructrice.crée_référence_membre(noeud, alloc_tranche, 0);
    auto premier_elem = m_constructrice.crée_accès_index(
        noeud, pointeur_tableau_fixe, m_constructrice.crée_z64(0ul));
    m_constructrice.crée_stocke_mem(noeud, ptr_pointeur_donnees, premier_elem);

    auto ptr_taille = m_constructrice.crée_référence_membre(noeud, alloc_tranche, 1);
    auto constante = m_constructrice.crée_z64(unsigned(type_tableau_fixe->taille));
    m_constructrice.crée_stocke_mem(noeud, ptr_taille, constante);

    return alloc_tranche;
}

Atome *CompilatriceRI::convertis_vers_tranche(NoeudExpression const *noeud,
                                              Atome *pointeur_tableau,
                                              TypeTableauDynamique const *type_tableau_fixe,
                                              Atome *place)
{
    auto alloc_tranche = place;

    if (alloc_tranche == nullptr) {
        auto type_tranche = m_compilatrice.typeuse.crée_type_tranche(
            type_tableau_fixe->type_pointé);
        alloc_tranche = m_constructrice.crée_allocation(noeud, type_tranche, nullptr);
    }

    auto ptr_pointeur_donnees = m_constructrice.crée_référence_membre(noeud, alloc_tranche, 0);
    auto pointeur_éléments = m_constructrice.crée_reference_membre_et_charge(
        noeud, pointeur_tableau, 0);
    m_constructrice.crée_stocke_mem(noeud, ptr_pointeur_donnees, pointeur_éléments);

    auto ptr_taille = m_constructrice.crée_référence_membre(noeud, alloc_tranche, 1);
    auto taille_tableau = m_constructrice.crée_reference_membre_et_charge(
        noeud, pointeur_tableau, 1);
    m_constructrice.crée_stocke_mem(noeud, ptr_taille, taille_tableau);

    return alloc_tranche;
}

AtomeConstante *CompilatriceRI::crée_constante_pour_chaine(kuri::chaine_statique chaine)
{
    auto table_chaines = m_compilatrice.registre_chaines_ri.verrou_ecriture();
    auto valeur = table_chaines->donne_constante_pour_chaine(chaine);

    if (valeur) {
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

    table_chaines->insère_constante_pour_chaine(chaine, constante_chaine);

    return constante_chaine;
}

AtomeGlobale *CompilatriceRI::crée_globale_pour_chaine(kuri::chaine_statique chaine)
{
    auto constante = crée_constante_pour_chaine(chaine);

    auto table_chaines = m_compilatrice.registre_chaines_ri.verrou_ecriture();
    auto valeur = table_chaines->donne_globale_pour_chaine(constante);

    if (valeur) {
        return valeur;
    }

    auto ident = m_compilatrice.donne_identifiant_pour_globale("constante_chaine");
    auto résultat = m_constructrice.crée_globale(*ident, TypeBase::CHAINE, constante, false, true);
    table_chaines->insère_globale_pour_chaine(constante, résultat);
    return résultat;
}

void CompilatriceRI::génère_ri_pour_initialisation_globales(
    AtomeFonction *fonction_init, kuri::tableau_statique<AtomeGlobale *> globales)
{
    définis_fonction_courante(fonction_init);

    /* Sauvegarde le retour. */
    auto di = fonction_init->instructions.dernier_élément();
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
    NoeudDéclarationEntêteFonction *fonction)
{
    assert(fonction->possède_drapeau(DrapeauxNoeudFonction::EST_MÉTAPROGRAMME));
    auto atome_fonc = m_constructrice.trouve_ou_insère_fonction(fonction);

    définis_fonction_courante(atome_fonc);

    auto decl_creation_contexte = m_compilatrice.interface_kuri->decl_creation_contexte;

    auto atome_creation_contexte = m_constructrice.trouve_ou_insère_fonction(
        decl_creation_contexte);

    atome_fonc->instructions.réserve(atome_creation_contexte->instructions.taille());

    for (auto i = 0; i < atome_creation_contexte->instructions.taille(); ++i) {
        auto it = atome_creation_contexte->instructions[i];
        atome_fonc->instructions.ajoute(it);
    }

    atome_fonc->décalage_appel_init_globale = atome_fonc->instructions.taille();

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

        auto const type_pointe = type_déréférencé_pour(expression->type);

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

void CompilatriceRI::génère_ri_pour_déclaration_variable(NoeudDéclarationVariable *decl)
{
    if (m_fonction_courante == nullptr) {
        génère_ri_pour_variable_globale(decl);
        return;
    }

    génère_ri_pour_variable_locale(decl);
}

void CompilatriceRI::génère_ri_pour_variable_globale(NoeudDéclarationVariable *decl)
{
    auto transformation = TransformationType{};
    auto expression = decl->expression;
    if (expression && expression->substitution) {
        expression = expression->substitution;
    }

    /* Supprime les transtypages pour pouvoir compiler les expressions comme des constantes
     * globales si possible, et uniquement transtyper la valeur dans la fonction d'initialisation
     * des globales au besoin. */
    if (expression && expression->est_comme()) {
        auto transtypage = expression->comme_comme();
        transformation = transtypage->transformation;
        expression = transtypage->expression;
    }

    compile_globale(decl, expression, transformation);
    decl->atome->drapeaux |= DrapeauxAtome::RI_FUT_GÉNÉRÉE;
    decl->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
}

void CompilatriceRI::génère_ri_pour_variable_locale(NoeudDéclarationVariable *decl)
{
    decl->atome = m_constructrice.crée_allocation(decl, decl->type, decl->ident);
    compile_locale(decl, decl->expression, {});
}

void CompilatriceRI::compile_déclaration_variable_multiple(NoeudDéclarationVariableMultiple *decl)
{
    if (m_fonction_courante == nullptr) {
        compile_déclaration_globale_multiple(decl);
        return;
    }

    compile_déclaration_locale_multiple(decl);
}

void CompilatriceRI::compile_déclaration_globale_multiple(NoeudDéclarationVariableMultiple *decl)
{
    POUR (decl->données_decl.plage()) {
        for (auto i = 0; i < it.variables.taille(); ++i) {
            auto var = it.variables[i];
            auto globale = var->comme_référence_déclaration()
                               ->déclaration_référée->comme_déclaration_variable();
            compile_globale(var->comme_référence_déclaration()->déclaration_référée,
                            it.expression,
                            it.transformations[i]);
            globale->atome->drapeaux |= DrapeauxAtome::RI_FUT_GÉNÉRÉE;
            globale->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
        }
    }

    decl->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
}

void CompilatriceRI::compile_déclaration_locale_multiple(NoeudDéclarationVariableMultiple *decl)
{
    /* Crée d'abord les allocations. */
    POUR (decl->données_decl.plage()) {
        POUR_NOMME (var, it.variables.plage()) {
            auto pointeur = m_constructrice.crée_allocation(var, var->type, var->ident);
            auto decl_var = var->comme_référence_déclaration()
                                ->déclaration_référée->comme_déclaration_symbole();
            decl_var->atome = pointeur;
        }
    }

    /* Génère le code pour les expressions. */
    génère_ri_pour_assignation_variable(decl->données_decl);
}

void CompilatriceRI::compile_globale(NoeudDéclaration *decl,
                                     NoeudExpression *expression,
                                     TransformationType const &transformation)
{
    auto valeur = static_cast<AtomeConstante *>(nullptr);
    auto atome = m_constructrice.trouve_ou_insère_globale(decl);

    if (atome->est_externe) {
        /* Les globales externes n'ont pas d'expressions, nous ne devrions pas les
         * initialiser nous-même. */
        return;
    }

    if (expression && expression->substitution) {
        expression = expression->substitution;
    }

    auto const méthode_construction = détermine_méthode_construction_globale(expression,
                                                                             transformation);

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
            auto ident = m_compilatrice.donne_identifiant_pour_globale("données_initilisateur");
            auto globale_tableau = m_constructrice.crée_globale(
                *ident, expression->type, nullptr, false, false);

            /* La construction du tableau devra se faire via la fonction
             * d'initialisation des globales. */
            m_compilatrice.constructeurs_globaux->ajoute({globale_tableau, expression, {}});

            valeur = m_constructrice.crée_initialisation_tableau_global(globale_tableau,
                                                                        type_tableau_fixe);
            break;
        }
        case MéthodeConstructionGlobale::NORMALE:
        {
            m_compilatrice.constructeurs_globaux->ajoute({atome, expression, transformation});
            break;
        }
        case MéthodeConstructionGlobale::PAR_VALEUR_DEFAUT:
        {
            valeur = m_constructrice.crée_initialisation_défaut_pour_type(decl->type);
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

void CompilatriceRI::compile_locale(NoeudExpression *variable,
                                    NoeudExpression *expression,
                                    TransformationType const &transformation)
{
    if (!expression) {
        auto pointeur = donne_atome_pour_locale(variable);
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

    auto pointeur = donne_atome_pour_locale(variable);
    /* Désactive le drapeau pour les assignations répétées aux mêmes valeurs.
     */
    pointeur->drapeaux &= ~DrapeauxAtome::EST_UTILISÉ;

    assert_rappel(expression->type, [&]() { dbg() << "Aucun type pour " << expression->genre; });

    if (transformation.type == TypeTransformation::INUTILE) {
        génère_ri_pour_expression_droite(expression, pointeur);
        return;
    }

    génère_ri_transformee_pour_noeud(expression, pointeur, transformation);
    return;
}

Atome *CompilatriceRI::donne_atome_pour_locale(NoeudExpression *expression)
{
    if (expression->est_référence_déclaration()) {
        auto référence = expression->comme_référence_déclaration();
        auto decl_var = référence->déclaration_référée->comme_déclaration_symbole();
        if (decl_var->atome) {
            return decl_var->atome;
        }
    }

    if (expression->est_déclaration_variable()) {
        auto decl_var = expression->comme_déclaration_variable();
        assert(decl_var->atome);
        return decl_var->atome;
    }

    génère_ri_pour_noeud(expression);
    return depile_valeur();
}

void CompilatriceRI::génère_ri_pour_assignation_variable(
    kuri::tableau_compresse<DonneesAssignations, int> const &données_exprs)
{
    if (données_exprs.taille() == 1 && données_exprs[0].variables.taille() == 1) {
        /* Cas simple : a = b ou a := b */
        auto expression = données_exprs[0].expression;
        auto variable = données_exprs[0].variables[0];
        auto transformation = données_exprs[0].transformations[0];
        compile_locale(variable, expression, transformation);
        return;
    }

    /* Cas complexe a, b = fonction() */

    POUR (données_exprs.plage()) {
        auto expression = it.expression;

        if (!expression) {
            /* Cas pour les déclarations. */
            POUR_NOMME (var, it.variables.plage()) {
                auto pointeur = donne_atome_pour_locale(var);
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
                auto pointeur = donne_atome_pour_locale(var);
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
                auto pointeur = donne_atome_pour_locale(var);
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
    if (!est_type_entier(type_tableau->type_pointé) &&
        !type_tableau->type_pointé->est_type_réel()) {
        return false;
    }

    POUR (expr->expression->comme_virgule()->expressions) {
        if (!it->est_littérale_entier() && !it->est_littérale_réel()) {
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
        auto valeur_constante = it->comme_littérale_entier()->valeur;
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
        if (it->est_littérale_entier()) {
            auto valeur_constante = it->comme_littérale_entier()->valeur;
            *données_constantes++ = static_cast<T>(valeur_constante);
        }
        else {
            auto valeur_constante = it->comme_littérale_réel()->valeur;
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
            auto type_élément = type_tableau_fixe->type_pointé;
            kuri::tableau<char> données_constantes(feuilles->expressions.taille() *
                                                   int(type_élément->taille_octet));

            if (est_type_entier(type_élément)) {
                remplis_données_constantes_entières(
                    &données_constantes[0], type_élément, feuilles);
            }
            else {
                assert(type_élément->est_type_réel());
                remplis_données_constantes_réelles(&données_constantes[0], type_élément, feuilles);
            }

            auto tableau = m_constructrice.crée_constante_tableau_données_constantes(
                type_tableau_fixe, std::move(données_constantes));

            empile_valeur(tableau);
            return;
        }

        kuri::tableau<AtomeConstante *> valeurs;
        valeurs.réserve(feuilles->expressions.taille());

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
    auto fichier = m_compilatrice.fichier(decl->lexème->fichier);
    auto nom_fonction = decl->ident ? crée_constante_pour_chaine(decl->ident->nom) :
                                      crée_constante_pour_chaine("???");
    auto nom_fichier = crée_constante_pour_chaine(fichier->nom());

    kuri::tableau<AtomeConstante *> valeurs(3);
    valeurs[0] = nom_fonction;
    valeurs[1] = nom_fichier;
    valeurs[2] = m_constructrice.crée_transtype_constant(TypeBase::ADRESSE_FONCTION,
                                                         pour_fonction);

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
        auto const lexeme = pour_appel->site->lexème;
        auto const fichier = m_compilatrice.fichier(pour_appel->site->lexème->fichier);
        auto const texte_ligne = fichier->tampon()[lexeme->ligne];

        valeurs[0] = m_constructrice.crée_z32(uint64_t(lexeme->ligne));
        valeurs[1] = m_constructrice.crée_z32(uint64_t(lexeme->colonne));
        valeurs[2] = crée_constante_pour_chaine(
            kuri::chaine_statique(texte_ligne.begin(), texte_ligne.taille()));
    }
    else {
        valeurs[0] = m_constructrice.crée_z32(0);
        valeurs[1] = m_constructrice.crée_z32(0);
        valeurs[2] = crée_constante_pour_chaine("???");
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
    auto type_trace_appel = m_compilatrice.typeuse.type_trace_appel->comme_type_composé();
    auto contexte_fil_principale = m_constructrice.trouve_globale(
        m_compilatrice.globale_contexte_programme);
    auto type_contexte_fil_principal = m_compilatrice.typeuse.type_contexte->comme_type_composé();
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

    auto mémoire = int64_t(0);
    mémoire += m_pile.taille_mémoire();
    mémoire += m_instructions_diffères.taille_mémoire();
    mémoire += m_registre_annotations.mémoire_utilisée();
    mémoire += m_trie_structs_employées.mémoire_utilisée();
    mémoire += m_trie_types_entrée_sortie.mémoire_utilisée();

    stats.ajoute_mémoire_utilisée("RI", mémoire);
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

int64_t RegistreAnnotations::mémoire_utilisée() const
{
    auto résultat = m_table.taille_mémoire();

    m_table.pour_chaque_élément([&](kuri::tableau<PaireValeurGlobale, int> const &tableau) {
        résultat += tableau.taille_mémoire();
    });

    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name RegistreChainesRI
 * \{ */

AtomeConstante *RegistreChainesRI::donne_constante_pour_chaine(kuri::chaine_statique chaine)
{
    return table_constantes.valeur_ou(chaine, nullptr);
}

void RegistreChainesRI::insère_constante_pour_chaine(kuri::chaine_statique chaine,
                                                     AtomeConstante *constante)
{
    table_constantes.insère(chaine, constante);
}

AtomeGlobale *RegistreChainesRI::donne_globale_pour_chaine(AtomeConstante *chaine)
{
    return table_globales.valeur_ou(chaine, nullptr);
}

void RegistreChainesRI::insère_globale_pour_chaine(AtomeConstante *chaine, AtomeGlobale *globale)
{
    table_globales.insère(chaine, globale);
}

int64_t RegistreChainesRI::mémoire_utilisée() const
{
    int64_t résultat(0);
    résultat += table_constantes.taille_mémoire();
    résultat += table_globales.taille_mémoire();
    return résultat;
}

/** \} */
