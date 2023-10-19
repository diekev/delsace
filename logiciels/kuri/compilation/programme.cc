/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "programme.hh"

#include <iostream>

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "representation_intermediaire/instructions.hh"

#include "compilatrice.hh"
#include "coulisse.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "ipa.hh"
#include "typage.hh"
#include "unite_compilation.hh"

/* ------------------------------------------------------------------------- */
/** \name Programme.
 * \{ */

#include "plateforme/windows.h"

Programme *Programme::cree(EspaceDeTravail *espace)
{
    Programme *resultat = memoire::loge<Programme>("Programme");
    resultat->m_espace = espace;
    return resultat;
}

Programme *Programme::cree_pour_espace(EspaceDeTravail *espace)
{
    auto resultat = Programme::cree(espace);
    resultat->m_coulisse = Coulisse::cree_pour_options(espace->options);
    return resultat;
}

Programme *Programme::cree_pour_metaprogramme(EspaceDeTravail *espace,
                                              MetaProgramme *metaprogramme)
{
    Programme *resultat = Programme::cree(espace);
    resultat->m_pour_metaprogramme = metaprogramme;
    resultat->m_coulisse = Coulisse::cree_pour_metaprogramme();
    return resultat;
}

void Programme::ajoute_fonction(NoeudDeclarationEnteteFonction *fonction)
{
    if (possede(fonction)) {
        return;
    }
    m_fonctions.ajoute(fonction);
    m_fonctions_utilisees.insère(fonction);
    ajoute_fichier(m_espace->compilatrice().fichier(fonction->lexeme->fichier));
    elements_sont_sales[FONCTIONS][POUR_TYPAGE] = true;
    elements_sont_sales[FONCTIONS][POUR_RI] = true;
    if (fonction->possede_drapeau(DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insère(fonction);
    }
    if (pour_métaprogramme()) {
        if (fonction->possede_drapeau(DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
            if (fonction->ident == ID::compilatrice_commence_interception) {
                pour_métaprogramme()->comportement |=
                    ComportementMétaprogramme::COMMENCE_INTERCEPTION;
                return;
            }
            if (fonction->ident == ID::compilatrice_termine_interception) {
                pour_métaprogramme()->comportement |=
                    ComportementMétaprogramme::TERMINE_INTERCEPTION;
                return;
            }
            if (fonction->ident == ID::ajoute_chaine_au_module ||
                fonction->ident == ID::ajoute_chaine_à_la_compilation ||
                fonction->ident == ID::ajoute_fichier_à_la_compilation) {
                pour_métaprogramme()->comportement |= ComportementMétaprogramme::AJOUTE_CODE;
                return;
            }
        }
    }
}

void Programme::ajoute_globale(NoeudDeclarationVariable *globale)
{
    if (possede(globale)) {
        return;
    }
    m_globales.ajoute(globale);
    m_globales_utilisees.insère(globale);
    ajoute_fichier(m_espace->compilatrice().fichier(globale->lexeme->fichier));
    elements_sont_sales[GLOBALES][POUR_TYPAGE] = true;
    elements_sont_sales[GLOBALES][POUR_RI] = true;
    if (globale->possede_drapeau(DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insère(globale);
    }
}

void Programme::ajoute_type(Type *type, RaisonAjoutType raison, NoeudExpression *noeud)
{
    if (possede(type)) {
        return;
    }
    m_types.ajoute(type);
    m_types_utilises.insère(type);
    elements_sont_sales[TYPES][POUR_TYPAGE] = true;
    elements_sont_sales[TYPES][POUR_RI] = true;

    if (type->fonction_init) {
        ajoute_fonction(type->fonction_init);
    }

#if 1
    static_cast<void>(raison);
    static_cast<void>(noeud);
#else
    if (!m_pour_metaprogramme) {
        if (raison == RaisonAjoutType::DÉPENDANCE_DIRECTE) {
            std::cerr << "Dépendence   directe de " << nom_humainement_lisible(noeud) << " : "
                      << chaine_type(type) << '\n';
        }
        else {
            std::cerr << "Dépendence indirecte de " << nom_humainement_lisible(noeud) << " : "
                      << chaine_type(type) << '\n';
        }
    }
#endif

    auto decl = decl_pour_type(type);
    if (decl && decl->possede_drapeau(DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insère(decl);
    }
}

bool Programme::typages_termines(DiagnostiqueÉtatCompilation &diagnostique) const
{
    if (elements_sont_sales[FONCTIONS][POUR_TYPAGE]) {
        POUR (m_fonctions) {
            if (!it->possede_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                diagnostique.déclaration_à_valider = it;
                return false;
            }

            if (!it->possede_drapeau(DrapeauxNoeudFonction::EST_EXTERNE) &&
                !it->corps->possede_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                diagnostique.déclaration_à_valider = it;
                return false;
            }
        }
        elements_sont_sales[FONCTIONS][POUR_TYPAGE] = false;
    }

    if (elements_sont_sales[GLOBALES][POUR_TYPAGE]) {
        POUR (m_globales) {
            if (!it->possede_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                diagnostique.déclaration_à_valider = it;
                return false;
            }
        }
        elements_sont_sales[GLOBALES][POUR_TYPAGE] = false;
    }

    if (elements_sont_sales[TYPES][POUR_TYPAGE]) {
        POUR (m_types) {
            if ((it->drapeaux & TYPE_FUT_VALIDE) == 0) {
                diagnostique.type_à_valider = it;
                return false;
            }
        }
        elements_sont_sales[TYPES][POUR_TYPAGE] = false;
    }

    diagnostique.toutes_les_déclarations_à_typer_le_sont = true;
    return true;
}

bool Programme::ri_generees(DiagnostiqueÉtatCompilation &diagnostique) const
{
    if (!typages_termines(diagnostique)) {
        return false;
    }

    using dls::outils::est_element;

    if (elements_sont_sales[FONCTIONS][POUR_RI]) {
        POUR (m_fonctions) {
            if (!it->possede_drapeau(DrapeauxNoeud::RI_FUT_GENEREE) &&
                !est_element(it->ident,
                             ID::init_execution_kuri,
                             ID::fini_execution_kuri,
                             ID::init_globales_kuri)) {
                assert_rappel(it->unite, [&]() {
                    std::cerr << "Aucune unité pour de compilation pour :\n";
                    erreur::imprime_site(*m_espace, it);
                });
                diagnostique.ri_déclaration_à_générer = it;
                return false;
            }
        }
        elements_sont_sales[FONCTIONS][POUR_RI] = false;
    }

    if (elements_sont_sales[GLOBALES][POUR_RI]) {
        POUR (m_globales) {
            if (!it->possede_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
                diagnostique.ri_déclaration_à_générer = it;
                return false;
            }
        }
        elements_sont_sales[GLOBALES][POUR_RI] = false;
    }

    if (elements_sont_sales[TYPES][POUR_RI]) {
        POUR (m_types) {
            if (!requiers_fonction_initialisation(it)) {
                continue;
            }

            if (requiers_création_fonction_initialisation(it)) {
                diagnostique.fonction_initialisation_type_à_créer = it;
                return false;
            }

            if (it->fonction_init &&
                !it->fonction_init->possede_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
                diagnostique.ri_type_à_générer = it;
                return false;
            }
        }
        elements_sont_sales[TYPES][POUR_RI] = false;
    }

    diagnostique.toutes_les_ri_sont_generees = true;
    return true;
}

bool Programme::ri_generees() const
{
    auto diagnostic = diagnostique_compilation();
    return diagnostic.toutes_les_ri_sont_generees;
}

DiagnostiqueÉtatCompilation Programme::diagnostique_compilation() const
{
    DiagnostiqueÉtatCompilation diagnostique{};
    verifie_etat_compilation_fichier(diagnostique);
    ri_generees(diagnostique);
    return diagnostique;
}

ÉtatCompilation Programme::ajourne_etat_compilation()
{
    auto diagnostic = diagnostique_compilation();

    // À FAIRE(gestion) : ceci n'est que pour les métaprogrammes
    m_etat_compilation.essaie_d_aller_à(PhaseCompilation::PARSAGE_EN_COURS);
    m_etat_compilation.essaie_d_aller_à(PhaseCompilation::PARSAGE_TERMINE);

    if (diagnostic.toutes_les_déclarations_à_typer_le_sont) {
        m_etat_compilation.essaie_d_aller_à(PhaseCompilation::TYPAGE_TERMINE);
    }

    if (diagnostic.toutes_les_ri_sont_generees) {
        m_etat_compilation.essaie_d_aller_à(PhaseCompilation::GENERATION_CODE_TERMINEE);
    }

    return m_etat_compilation;
}

void Programme::change_de_phase(PhaseCompilation phase)
{
    m_etat_compilation.essaie_d_aller_à(phase);
}

int64_t Programme::memoire_utilisee() const
{
    auto memoire = int64_t(0);
    memoire += fonctions().taille_memoire();
    memoire += types().taille_memoire();
    memoire += globales().taille_memoire();
    memoire += m_fonctions_utilisees.taille_memoire();
    memoire += m_types_utilises.taille_memoire();
    memoire += m_globales_utilisees.taille_memoire();
    memoire += taille_de(Coulisse);
    memoire += m_fichiers.taille_memoire();
    return memoire;
}

void Programme::rassemble_statistiques(Statistiques & /*stats*/)
{
}

kuri::ensemble<Module *> Programme::modules_utilises() const
{
    kuri::ensemble<Module *> modules;
    POUR (m_fichiers) {
        modules.insère(it->module);
    }
    return modules;
}

void Programme::ajourne_pour_nouvelles_options_espace()
{
    /* Recrée la coulisse. */
    Coulisse::detruit(m_coulisse);
    m_coulisse = Coulisse::cree_pour_options(espace()->options);

    auto index = 0;
    POUR (m_fonctions) {
        /* Supprime le point d'entrée. */
        if (it == espace()->fonction_point_d_entree &&
            espace()->options.resultat != ResultatCompilation::EXECUTABLE) {
            std::swap(m_fonctions[index], m_fonctions[m_fonctions.taille() - 1]);
            m_fonctions.redimensionne(m_fonctions.taille() - 1);
            break;
        }

        index += 1;
    }
}

kuri::ensemble<NoeudDeclaration *> &Programme::dépendances_manquantes()
{
    return m_dépendances_manquantes;
}

void Programme::imprime_diagnostique(std::ostream &os, bool ignore_doublon)
{
    auto diag = diagnostique_compilation();

    if (!m_dernier_diagnostique.has_value() ||
        (diag != m_dernier_diagnostique.value() || !ignore_doublon)) {
        os << "==========================================================\n";
        os << "Diagnostique pour programme de " << m_espace->nom << '\n';
        ::imprime_diagnostique(diag, os);
    }

    m_dernier_diagnostique = diag;
}

void Programme::verifie_etat_compilation_fichier(DiagnostiqueÉtatCompilation &diagnostique) const
{
    diagnostique.tous_les_fichiers_sont_chargés = true;
    diagnostique.tous_les_fichiers_sont_lexés = true;
    diagnostique.tous_les_fichiers_sont_parsés = true;

    if (!m_fichiers_sont_sales) {
        return;
    }

    POUR (m_fichiers) {
        if (!it->fut_chargé) {
            diagnostique.tous_les_fichiers_sont_chargés = false;
        }

        if (!it->fut_lexé) {
            diagnostique.tous_les_fichiers_sont_lexés = false;
        }

        if (!it->fut_parsé) {
            diagnostique.tous_les_fichiers_sont_parsés = false;
        }
    }

    m_fichiers_sont_sales = false;
}

void Programme::ajoute_fichier(Fichier *fichier)
{
    if (m_fichiers_utilises.possède(fichier)) {
        return;
    }

    m_fichiers.ajoute(fichier);
    m_fichiers_utilises.insère(fichier);
    m_fichiers_sont_sales = true;
}

void Programme::ajoute_racine(NoeudDeclarationEnteteFonction *racine)
{
    if (pour_métaprogramme()) {
        /* Pour les métaprogrammes, nous n'ajoutons que les racines pour la création de
         * l'exécutable. */
        if (racine->ident == ID::cree_contexte) {
            ajoute_fonction(racine);
        }
    }
    else {
        if (racine->ident != ID::cree_contexte) {
            ajoute_fonction(racine);
        }
    }
}

void imprime_contenu_programme(const Programme &programme, uint32_t quoi, std::ostream &os)
{
    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_TYPES) != 0) {
        os << "Types dans le programme...\n";
        POUR (programme.types()) {
            os << "-- " << chaine_type(it) << '\n';
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_FONCTIONS) != 0) {
        os << "Fonctions dans le programme...\n";
        POUR (programme.fonctions()) {
            os << "-- " << it->ident->nom << '\n';
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_GLOBALES) != 0) {
        os << "Globales dans le programme...\n";
        POUR (programme.globales()) {
            os << "-- " << it->ident->nom << '\n';
        }
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Diagnostique état compilation.
 * \{ */

static void imprime_détails_déclaration_à_valider(std::ostream &os, NoeudDeclaration *déclaration)
{
    if (!déclaration->est_entete_fonction()) {
        os << "-- validation non performée pour déclaration "
           << nom_humainement_lisible(déclaration) << '\n';
        return;
    }

    if (!déclaration->possede_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        os << "-- validation non performée pour l'entête " << nom_humainement_lisible(déclaration)
           << '\n';
        return;
    }

    auto corps = déclaration->comme_entete_fonction();
    if (corps->possede_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        /* NOTE : ceci peut-être un faux positif car un thread différent peut mettre en place le
         * drapeau... */
        os << "-- erreur : le corps et l'entête de " << nom_humainement_lisible(déclaration)
           << " sont marqués comme validés, mais le diagnostique considère le corps comme non "
              "validé\n";
        return;
    }

    auto unité_corps = corps->unite;
    if (!unité_corps) {
        os << "-- validation non performée car aucune unité pour le corps de "
           << nom_humainement_lisible(déclaration) << "\n";
        return;
    }

    os << "-- validation non performée pour le corps de " << nom_humainement_lisible(déclaration)
       << "\n";
}

static void imprime_détails_ri_à_générée(std::ostream &os, NoeudDeclaration *déclaration)
{
    os << "-- RI non générée pour ";

    if (déclaration->est_entete_fonction()) {
        os << "la fonction";
    }
    else {
        os << "la déclaration de";
    }

    os << " " << nom_humainement_lisible(déclaration) << '\n';

    if (déclaration->est_entete_fonction()) {
        auto entête = déclaration->comme_entete_fonction();
        os << "-- état de l'unité de l'entête :\n";
        imprime_état_unité(os, entête->unite);
        if (entête->corps->unite) {
            os << "-- état de l'unité du corps :\n";
            imprime_état_unité(os, entête->corps->unite);
        }
    }
    else if (déclaration->unite) {
        os << "-- état de l'unité :\n";
        imprime_état_unité(os, déclaration->unite);
    }
}

void imprime_diagnostique(const DiagnostiqueÉtatCompilation &diagnostique, std::ostream &os)
{
    if (!diagnostique.toutes_les_déclarations_à_typer_le_sont) {
        if (diagnostique.type_à_valider) {
            os << "-- validation non performée pour le type : "
               << chaine_type(diagnostique.type_à_valider) << '\n';
        }
        if (diagnostique.déclaration_à_valider) {
            imprime_détails_déclaration_à_valider(os, diagnostique.déclaration_à_valider);
        }
        return;
    }

    if (diagnostique.fonction_initialisation_type_à_créer) {
        os << "-- fonction d'initialisation non-créée pour le type : "
           << chaine_type(diagnostique.ri_type_à_générer) << '\n';
    }
    if (diagnostique.ri_type_à_générer) {
        os << "-- RI non générée pour la fonction d'initialisation du type : "
           << chaine_type(diagnostique.ri_type_à_générer) << '\n';
    }
    if (diagnostique.ri_déclaration_à_générer) {
        imprime_détails_ri_à_générée(os, diagnostique.ri_déclaration_à_générer);
    }
}

bool operator==(DiagnostiqueÉtatCompilation const &diag1, DiagnostiqueÉtatCompilation const &diag2)
{
#define COMPARE_MEMBRE(x)                                                                         \
    if ((diag1.x) != (diag2.x)) {                                                                 \
        return false;                                                                             \
    }

    COMPARE_MEMBRE(tous_les_fichiers_sont_chargés)
    COMPARE_MEMBRE(tous_les_fichiers_sont_lexés)
    COMPARE_MEMBRE(tous_les_fichiers_sont_parsés)
    COMPARE_MEMBRE(toutes_les_déclarations_à_typer_le_sont)
    COMPARE_MEMBRE(toutes_les_ri_sont_generees)
    COMPARE_MEMBRE(type_à_valider)
    COMPARE_MEMBRE(déclaration_à_valider)
    COMPARE_MEMBRE(ri_type_à_générer)
    COMPARE_MEMBRE(fonction_initialisation_type_à_créer)
    COMPARE_MEMBRE(ri_déclaration_à_générer)

#undef COMPARE_MEMBRE
    return true;
}

bool operator!=(DiagnostiqueÉtatCompilation const &diag1, DiagnostiqueÉtatCompilation const &diag2)
{
    return !(diag1 == diag2);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name VisiteuseType.
 * \{ */

struct VisiteuseType {
    kuri::ensemble<Type *> visites{};

    void visite_type(Type *type, std::function<void(Type *)> rappel)
    {
        if (!type) {
            return;
        }

        if (visites.possède(type)) {
            return;
        }

        visites.insère(type);
        rappel(type);

        if (type->fonction_init) {
            visite_type(type->fonction_init->type, rappel);
        }

        switch (type->genre) {
            case GenreType::EINI:
            {
                break;
            }
            case GenreType::CHAINE:
            {
                break;
            }
            case GenreType::RIEN:
            case GenreType::BOOL:
            case GenreType::OCTET:
            case GenreType::ENTIER_CONSTANT:
            case GenreType::ENTIER_NATUREL:
            case GenreType::ENTIER_RELATIF:
            case GenreType::REEL:
            {
                break;
            }
            case GenreType::REFERENCE:
            {
                auto reference = type->comme_type_reference();
                visite_type(reference->type_pointe, rappel);
                break;
            }
            case GenreType::POINTEUR:
            {
                auto pointeur = type->comme_type_pointeur();
                visite_type(pointeur->type_pointe, rappel);
                break;
            }
            case GenreType::UNION:
            {
                auto type_union = type->comme_type_union();
                POUR (type_union->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            case GenreType::STRUCTURE:
            {
                auto type_structure = type->comme_type_structure();
                POUR (type_structure->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            case GenreType::TABLEAU_DYNAMIQUE:
            {
                auto tableau = type->comme_type_tableau_dynamique();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreType::TABLEAU_FIXE:
            {
                auto tableau = type->comme_type_tableau_fixe();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreType::VARIADIQUE:
            {
                auto variadique = type->comme_type_variadique();
                visite_type(variadique->type_pointe, rappel);
                break;
            }
            case GenreType::FONCTION:
            {
                auto fonction = type->comme_type_fonction();
                POUR (fonction->types_entrees) {
                    visite_type(it, rappel);
                }
                visite_type(fonction->type_sortie, rappel);
                break;
            }
            case GenreType::ENUM:
            case GenreType::ERREUR:
            {
                auto type_enum = static_cast<TypeEnum *>(type);
                visite_type(type_enum->type_sous_jacent, rappel);
                break;
            }
            case GenreType::TYPE_DE_DONNEES:
            {
                break;
            }
            case GenreType::POLYMORPHIQUE:
            {
                break;
            }
            case GenreType::OPAQUE:
            {
                break;
            }
            case GenreType::TUPLE:
            {
                break;
            }
        }
    }
};

#if 0
static void visite_type(Type *type, std::function<void(Type *)> rappel)
{
    VisiteuseType visiteuse{};
    visiteuse.visite_type(type, rappel);
}
#endif

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Représentation intermédiaire Programme.
 * \{ */

void imprime_contenu_programme(const ProgrammeRepreInter &programme,
                               uint32_t quoi,
                               std::ostream &os)
{
    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_TYPES) != 0) {
        os << "Types dans le programme...\n";
        POUR (programme.types) {
            os << "-- " << chaine_type(it) << '\n';
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_FONCTIONS) != 0) {
        os << "Fonctions dans le programme...\n";
        POUR (programme.fonctions) {
            if (it->decl && it->decl->ident) {
                os << "-- " << it->decl->ident->nom << ' ' << chaine_type(it->type) << '\n';
            }
            else {
                os << "-- anonyme de type " << chaine_type(it->type) << '\n';
            }
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_GLOBALES) != 0) {
        os << "Globales dans le programme...\n";
        POUR (programme.globales) {
            if (it->ident) {
                os << "-- " << it->ident->nom << '\n';
            }
            else {
                os << "-- anonyme de type " << chaine_type(it->type) << '\n';
            }
        }
    }
}

/* La seule raison d'existence pour cette fonction est de rassembler les globales pour les chaines
 * et InfoType. */
static void rassemble_globales_supplementaires(ProgrammeRepreInter &repr_inter,
                                               Atome *atome,
                                               VisiteuseAtome &visiteuse,
                                               kuri::ensemble<AtomeGlobale *> &globales_utilisees)
{
    visiteuse.visite_atome(atome, [&](Atome *atome_local) {
        if (atome_local->genre_atome == Atome::Genre::GLOBALE) {
            if (globales_utilisees.possède(static_cast<AtomeGlobale *>(atome_local))) {
                return;
            }

            repr_inter.globales.ajoute(static_cast<AtomeGlobale *>(atome_local));
            globales_utilisees.insère(static_cast<AtomeGlobale *>(atome_local));
        }
    });
}

static void rassemble_globales_supplementaires(ProgrammeRepreInter &repr_inter,
                                               AtomeFonction *fonction,
                                               VisiteuseAtome &visiteuse,
                                               kuri::ensemble<AtomeGlobale *> &globales_utilisees)
{
    POUR (fonction->instructions) {
        rassemble_globales_supplementaires(repr_inter, it, visiteuse, globales_utilisees);
    }
}

static void rassemble_globales_supplementaires(ProgrammeRepreInter &repr_inter)
{
    auto globales_utilisees = crée_ensemble(repr_inter.globales);
    VisiteuseAtome visiteuse{};

    /* Prend en compte les globales pouvant être ajoutées via l'initialisation des tableaux fixes
     * devant être convertis. */
    /* Itération avec un index car l'insertion de nouvelles globales invaliderait les
     * itérateurs. */
    auto const nombre_de_globales = repr_inter.globales.taille();
    for (auto i = 0; i < nombre_de_globales; i++) {
        auto it = repr_inter.globales[i];

        if (!it->initialisateur) {
            continue;
        }

        rassemble_globales_supplementaires(
            repr_inter, it->initialisateur, visiteuse, globales_utilisees);
    }

    POUR (repr_inter.fonctions) {
        visiteuse.reinitialise();
        rassemble_globales_supplementaires(repr_inter, it, visiteuse, globales_utilisees);
    }
}

static void rassemble_types_supplementaires(ProgrammeRepreInter &repr_inter)
{
    /* Ajoute les types de toutes les globales et toutes les fonctions, dans le cas où nous en
     * aurions ajoutées (qui ne sont pas dans le programme initiale). */
    auto type_utilises = crée_ensemble(repr_inter.types);

    VisiteuseType visiteuse{};
    auto ajoute_type_si_necessaire = [&](Type const *type_racine) {
        visiteuse.visite_type(const_cast<Type *>(type_racine), [&](Type *type) {
            if (type_utilises.possède(type)) {
                return;
            }

            type_utilises.insère(type);
            repr_inter.types.ajoute(type);
        });
    };

    POUR (repr_inter.fonctions) {
        ajoute_type_si_necessaire(it->type);
        for (auto &inst : it->instructions) {
            ajoute_type_si_necessaire(inst->type);
        }
    }

    POUR (repr_inter.globales) {
        ajoute_type_si_necessaire(it->type);
    }
}

ProgrammeRepreInter représentation_intermédiaire_programme(Programme const &programme)
{
    auto resultat = ProgrammeRepreInter{};

    resultat.fonctions.reserve(programme.fonctions().taille() + programme.types().taille());
    resultat.globales.reserve(programme.globales().taille());

    /* Nous pouvons directement copier les types. */
    resultat.types = programme.types();

    /* Extrait les atomes pour les fonctions. */
    POUR (programme.fonctions()) {
        assert_rappel(it->possede_drapeau(DrapeauxNoeud::RI_FUT_GENEREE), [&]() {
            std::cerr << "La RI ne fut pas généré pour:\n";
            erreur::imprime_site(*programme.espace(), it);
        });
        assert_rappel(it->atome, [&]() {
            std::cerr << "Aucun atome pour:\n";
            erreur::imprime_site(*programme.espace(), it);
        });
        resultat.fonctions.ajoute(static_cast<AtomeFonction *>(it->atome));
    }

    /* Extrait les atomes pour les globales. */
    POUR (programme.globales()) {
        assert_rappel(it->possede_drapeau(DrapeauxNoeud::RI_FUT_GENEREE), [&]() {
            std::cerr << "La RI ne fut pas généré pour:\n";
            erreur::imprime_site(*programme.espace(), it);
        });
        assert_rappel(it->atome, [&]() {
            std::cerr << "Aucun atome pour:\n";
            erreur::imprime_site(*programme.espace(), it);
            std::cerr << "Taille données decl  : " << it->donnees_decl.taille() << '\n';
            std::cerr << "Possède substitution : " << (it->substitution != nullptr) << '\n';
        });
        resultat.globales.ajoute(static_cast<AtomeGlobale *>(it->atome));
    }

    /* Traverse les instructions des fonctions, et rassemble les globales pour les chaines, les
     * tableaux, et les infos-types. */
    rassemble_globales_supplementaires(resultat);

    rassemble_types_supplementaires(resultat);

    return resultat;
}

/* Cette fonction n'existe que parce que la principale peut ajouter des globales pour les
 * constructeurs globaux... */
void ProgrammeRepreInter::ajoute_fonction(AtomeFonction *fonction)
{
    fonctions.ajoute(fonction);
    ajourne_globales_pour_fonction(fonction);
}

void ProgrammeRepreInter::ajourne_globales_pour_fonction(AtomeFonction *fonction)
{
    auto globales_utilisees = crée_ensemble(this->globales);
    VisiteuseAtome visiteuse{};
    rassemble_globales_supplementaires(*this, fonction, visiteuse, globales_utilisees);
    /* Les types ont peut-être changé. */
    rassemble_types_supplementaires(*this);
}

/** \} */
