/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "programme.hh"

#include <iostream>

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "representation_intermediaire/instructions.hh"

#include "utilitaires/algorithmes.hh"

#include "structures/pile.hh"

#include "compilatrice.hh"
#include "coulisse.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "ipa.hh"
#include "typage.hh"
#include "unite_compilation.hh"
#include "utilitaires/log.hh"

/* ------------------------------------------------------------------------- */
/** \name Programme.
 * \{ */

Programme *Programme::cree(EspaceDeTravail *espace)
{
    Programme *résultat = memoire::loge<Programme>("Programme");
    résultat->m_espace = espace;
    return résultat;
}

Programme *Programme::crée_pour_espace(EspaceDeTravail *espace)
{
    auto résultat = Programme::cree(espace);
    résultat->m_coulisse = Coulisse::crée_pour_options(espace->options);
    return résultat;
}

Programme *Programme::crée_pour_metaprogramme(EspaceDeTravail *espace,
                                              MetaProgramme *metaprogramme)
{
    Programme *résultat = Programme::cree(espace);
    résultat->m_pour_metaprogramme = metaprogramme;
    résultat->m_coulisse = Coulisse::crée_pour_metaprogramme();
    return résultat;
}

Programme::~Programme()
{
    Coulisse::détruit(m_coulisse);
}

void Programme::ajoute_fonction(NoeudDeclarationEnteteFonction *fonction)
{
    if (possède(fonction)) {
        return;
    }
    m_fonctions.ajoute(fonction);
    m_fonctions_utilisees.insère(fonction);
    ajoute_fichier(m_espace->compilatrice().fichier(fonction->lexeme->fichier));
    elements_sont_sales[FONCTIONS][POUR_TYPAGE] = true;
    elements_sont_sales[FONCTIONS][POUR_RI] = true;
    if (fonction->possède_drapeau(DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insère(fonction);
    }
    if (pour_métaprogramme()) {
        if (fonction->possède_drapeau(DrapeauxNoeudFonction::EST_IPA_COMPILATRICE)) {
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
    if (possède(globale)) {
        return;
    }
    m_globales.ajoute(globale);
    m_globales_utilisees.insère(globale);
    ajoute_fichier(m_espace->compilatrice().fichier(globale->lexeme->fichier));
    elements_sont_sales[GLOBALES][POUR_TYPAGE] = true;
    elements_sont_sales[GLOBALES][POUR_RI] = true;
    if (globale->possède_drapeau(DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insère(globale);
    }
}

void Programme::ajoute_type(Type *type, RaisonAjoutType raison, NoeudExpression *noeud)
{
    if (possède(type)) {
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
            dbg() << "Dépendence   directe de " << nom_humainement_lisible(noeud) << " : "
                  << chaine_type(type);
        }
        else {
            dbg() << "Dépendence indirecte de " << nom_humainement_lisible(noeud) << " : "
                  << chaine_type(type);
        }
    }
#endif

    if (type->possède_drapeau(DrapeauxNoeud::DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insère(type);
    }
}

bool Programme::typages_termines(DiagnostiqueÉtatCompilation &diagnostique) const
{
    if (elements_sont_sales[FONCTIONS][POUR_TYPAGE]) {
        POUR (m_fonctions) {
            if (!it->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                diagnostique.déclaration_à_valider = it;
                return false;
            }

            if (!it->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE) &&
                !it->corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                diagnostique.déclaration_à_valider = it;
                return false;
            }
        }
        elements_sont_sales[FONCTIONS][POUR_TYPAGE] = false;
    }

    if (elements_sont_sales[GLOBALES][POUR_TYPAGE]) {
        POUR (m_globales) {
            if (!it->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
                diagnostique.déclaration_à_valider = it;
                return false;
            }
        }
        elements_sont_sales[GLOBALES][POUR_TYPAGE] = false;
    }

    if (elements_sont_sales[TYPES][POUR_TYPAGE]) {
        POUR (m_types) {
            if (!it->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
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
            if (!it->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE) &&
                !est_element(it->ident,
                             ID::init_execution_kuri,
                             ID::fini_execution_kuri,
                             ID::init_globales_kuri)) {
                assert_rappel(it->unité, [&]() {
                    dbg() << "Aucune unité pour de compilation pour :\n"
                          << erreur::imprime_site(*m_espace, it);
                });
                diagnostique.ri_déclaration_à_générer = it;
                return false;
            }
        }
        elements_sont_sales[FONCTIONS][POUR_RI] = false;
    }

    if (elements_sont_sales[GLOBALES][POUR_RI]) {
        POUR (m_globales) {
            if (!it->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
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
                !it->fonction_init->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE)) {
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
    Coulisse::détruit(m_coulisse);
    m_coulisse = Coulisse::crée_pour_options(espace()->options);

    auto index = 0;
    POUR (m_fonctions) {
        /* Supprime le point d'entrée. */
        if (it == espace()->fonction_point_d_entree &&
            espace()->options.résultat != ResultatCompilation::EXECUTABLE) {
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
        if (racine->ident == ID::crée_contexte) {
            ajoute_fonction(racine);
        }
    }
    else {
        if (racine->ident != ID::crée_contexte) {
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

    if (!déclaration->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        os << "-- validation non performée pour l'entête " << nom_humainement_lisible(déclaration)
           << '\n';
        return;
    }

    auto corps = déclaration->comme_entete_fonction();
    if (corps->possède_drapeau(DrapeauxNoeud::DECLARATION_FUT_VALIDEE)) {
        /* NOTE : ceci peut-être un faux positif car un thread différent peut mettre en place le
         * drapeau... */
        os << "-- erreur : le corps et l'entête de " << nom_humainement_lisible(déclaration)
           << " sont marqués comme validés, mais le diagnostique considère le corps comme non "
              "validé\n";
        return;
    }

    auto unité_corps = corps->unité;
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
        imprime_état_unité(os, entête->unité);
        if (entête->corps->unité) {
            os << "-- état de l'unité du corps :\n";
            imprime_état_unité(os, entête->corps->unité);
        }
    }
    else {
        auto adresse_unité = donne_adresse_unité(déclaration);
        if (*adresse_unité) {
            os << "-- état de l'unité :\n";
            imprime_état_unité(os, *adresse_unité);
        }
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
    bool visite_types_fonctions_init = true;

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

        if (visite_types_fonctions_init && type->fonction_init) {
            visite_type(type->fonction_init->type, rappel);
        }

        switch (type->genre) {
            case GenreNoeud::EINI:
            {
                break;
            }
            case GenreNoeud::CHAINE:
            {
                break;
            }
            case GenreNoeud::RIEN:
            case GenreNoeud::BOOL:
            case GenreNoeud::OCTET:
            case GenreNoeud::ENTIER_CONSTANT:
            case GenreNoeud::ENTIER_NATUREL:
            case GenreNoeud::ENTIER_RELATIF:
            case GenreNoeud::REEL:
            {
                break;
            }
            case GenreNoeud::REFERENCE:
            {
                auto reference = type->comme_type_reference();
                visite_type(reference->type_pointe, rappel);
                break;
            }
            case GenreNoeud::POINTEUR:
            {
                auto pointeur = type->comme_type_pointeur();
                visite_type(pointeur->type_pointe, rappel);
                break;
            }
            case GenreNoeud::DECLARATION_UNION:
            {
                auto type_union = type->comme_type_union();
                POUR (type_union->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            case GenreNoeud::DECLARATION_STRUCTURE:
            {
                auto type_structure = type->comme_type_structure();
                POUR (type_structure->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            case GenreNoeud::TABLEAU_DYNAMIQUE:
            {
                auto tableau = type->comme_type_tableau_dynamique();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreNoeud::TABLEAU_FIXE:
            {
                auto tableau = type->comme_type_tableau_fixe();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreNoeud::VARIADIQUE:
            {
                auto variadique = type->comme_type_variadique();
                visite_type(variadique->type_pointe, rappel);
                break;
            }
            case GenreNoeud::FONCTION:
            {
                auto fonction = type->comme_type_fonction();
                POUR (fonction->types_entrees) {
                    visite_type(it, rappel);
                }
                visite_type(fonction->type_sortie, rappel);
                break;
            }
            case GenreNoeud::DECLARATION_ENUM:
            case GenreNoeud::ERREUR:
            case GenreNoeud::ENUM_DRAPEAU:
            {
                auto type_enum = static_cast<TypeEnum *>(type);
                visite_type(type_enum->type_sous_jacent, rappel);
                break;
            }
            case GenreNoeud::TYPE_DE_DONNEES:
            {
                break;
            }
            case GenreNoeud::POLYMORPHIQUE:
            {
                break;
            }
            case GenreNoeud::DECLARATION_OPAQUE:
            {
                auto type_opaque = type->comme_type_opaque();
                visite_type(type_opaque->type_opacifie, rappel);
                break;
            }
            case GenreNoeud::TUPLE:
            {
                auto type_tuple = type->comme_type_tuple();
                POUR (type_tuple->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            CAS_POUR_NOEUDS_HORS_TYPES:
            {
                assert_rappel(false,
                              [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
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
static bool est_type_pointeur_tuple(Type const *type)
{
    if (!type->est_type_pointeur()) {
        return false;
    }

    auto type_pointeur = type->comme_type_pointeur();
    if (!type_pointeur->type_pointe) {
        return false;
    }

    return type_pointeur->type_pointe->est_type_tuple();
}

/* Détecte les types tuples et les types associés à leurs initialisation pour pouvoir les exclure
 * de la table des types. */
static bool est_type_tuple_ou_fonction_init_tuple(Type const *type)
{
    if (type->est_type_tuple()) {
        return true;
    }

    if (type->est_type_fonction()) {
        auto const type_fonction = type->comme_type_fonction();
        if (type_fonction->types_entrees.taille() != 1) {
            return false;
        }

        auto type_entrée = type_fonction->types_entrees[0];
        if (!est_type_pointeur_tuple(type_entrée)) {
            return false;
        }

        return type_fonction->type_sortie->est_type_rien();
    }

    if (est_type_pointeur_tuple(type)) {
        return true;
    }

    return false;
}

/* ------------------------------------------------------------------------- */
/** \name ConstructriceProgrammeFormeRI
 *
 * La ConstructriceProgrammeFormeRI s'occupe de créer un ProgrammeRepreInter
 * depuis un Programme.
 *
 * Elle rassemble toutes les fonctions et globales ainsi que tous les types du
 * Programme.
 *
 * Elle crée également le corps de la fonction d'initialisation des globales,
 * les trace d'appels au sein des fonctions, les infos-types manquants pour les
 * types, et la table des types.
 * \{ */

struct ConstructriceProgrammeFormeRI {
  private:
    ProgrammeRepreInter m_résultat{};

    EspaceDeTravail &m_espace;
    CompilatriceRI &m_compilatrice_ri;
    Programme const &m_programme;

    kuri::ensemble<AtomeGlobale *> m_globales_utilisées{};
    kuri::ensemble<Type *> m_types_utilisés{};

    kuri::tableau<AtomeFonction *> m_fonctions_racines{};

    VisiteuseAtome m_visiteuse_atome{};

  public:
    ConstructriceProgrammeFormeRI(EspaceDeTravail &espace,
                                  CompilatriceRI &compilatrice_ri,
                                  Programme const &programme)
        : m_espace(espace), m_compilatrice_ri(compilatrice_ri), m_programme(programme)
    {
    }

    std::optional<ProgrammeRepreInter> construit_représentation_intermédiaire_programme();

  private:
    void ajoute_fonction(AtomeFonction *fonction);

    void ajoute_dépendances_fonction(AtomeFonction *fonction);

    void ajoute_globale(AtomeGlobale *globale, bool visite_globale);

    void ajoute_type(Type *type, bool visite_type);

    void génère_ri_fonction_init_globales(AtomeFonction *fonction);

    void génère_traces_d_appel();

    void génère_table_des_types();

    void tri_fonctions_et_globales();

    void partitionne_globales_info_types();

    /* Traverse les fonctions racines et supprime du résultat les fonctions qui ne sont pas
     * utilisées. Ces fonctions sont principalement les fonctions d'initialisation des types
     * inutilisées. */
    void supprime_fonctions_inutilisées();

    /* Traverse les fonctions et globales et supprime du résultat les types qui ne sont pas
     * utilisés (soit comme paramètre ou comme comme variable). Ces types sont principalement les
     * types des fonctions. */
    void supprime_types_inutilisés();
};

std::optional<ProgrammeRepreInter> ConstructriceProgrammeFormeRI::
    construit_représentation_intermédiaire_programme()
{
    m_résultat.fonctions.reserve(m_programme.fonctions().taille() + m_programme.types().taille());
    m_résultat.globales.reserve(m_programme.globales().taille());

    /* Nous pouvons directement copier les types. */
    m_résultat.types = m_programme.types();

    auto nombre_fonctions_racines = 0;
    auto decl_init_globales = static_cast<AtomeFonction *>(nullptr);
    auto decl_principale = static_cast<AtomeFonction *>(nullptr);

    if (m_programme.pour_métaprogramme()) {
        m_fonctions_racines.ajoute(
            m_programme.pour_métaprogramme()->fonction->atome->comme_fonction());
    }

    /* Extrait les atomes pour les fonctions. */
    POUR (m_programme.fonctions()) {
        assert_rappel(it->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE), [&]() {
            dbg() << "La RI ne fut pas généré pour:\n"
                  << erreur::imprime_site(*m_programme.espace(), it);
        });
        assert_rappel(it->atome, [&]() {
            dbg() << "Aucun atome pour:\n" << erreur::imprime_site(*m_programme.espace(), it);
        });

        auto atome_fonction = it->atome->comme_fonction();

        if (it->ident == ID::__principale) {
            /* Cette fonction est symbolique et ne doit pas être dans le code généré. */
            continue;
        }

        ajoute_fonction(atome_fonction);

        if (it->possède_drapeau(DrapeauxNoeudFonction::EST_RACINE)) {
            ++nombre_fonctions_racines;
            m_fonctions_racines.ajoute(atome_fonction);
        }

        if (it->ident == ID::init_globales_kuri) {
            decl_init_globales = atome_fonction;
            m_fonctions_racines.ajoute(atome_fonction);
        }
        else if (it->ident == ID::principale) {
            decl_principale = atome_fonction;
            m_fonctions_racines.ajoute(atome_fonction);
        }
        else if (it->ident == ID::__point_d_entree_systeme) {
            m_fonctions_racines.ajoute(atome_fonction);
        }
        else if (it->ident == ID::__init_contexte_kuri) {
            m_fonctions_racines.ajoute(atome_fonction);
        }
        else if (it->ident == ID::__point_d_entree_dynamique) {
            m_fonctions_racines.ajoute(atome_fonction);
        }
        else if (it->ident == ID::__point_de_sortie_dynamique) {
            m_fonctions_racines.ajoute(atome_fonction);
        }
    }

    /* Extrait les atomes pour les globales. */
    POUR (m_programme.globales()) {
        assert_rappel(it->possède_drapeau(DrapeauxNoeud::RI_FUT_GENEREE), [&]() {
            dbg() << "La RI ne fut pas généré pour:\n"
                  << erreur::imprime_site(*m_programme.espace(), it);
        });
        assert_rappel(it->atome, [&]() {
            dbg() << "Aucun atome pour:\n"
                  << erreur::imprime_site(*m_programme.espace(), it) << '\n'
                  << "Taille données decl  : " << it->donnees_decl.taille() << '\n'
                  << "Possède substitution : " << (it->substitution != nullptr);
        });
        ajoute_globale(it->atome->comme_globale(), true);
    }

    if (m_programme.pour_métaprogramme()) {
        auto métaprogramme = m_programme.pour_métaprogramme();
        auto fonction = métaprogramme->fonction->atome->comme_fonction();

        if (!fonction) {
            m_espace.rapporte_erreur(métaprogramme->fonction,
                                     "Impossible de trouver la fonction pour le métaprogramme");
            return {};
        }

        if (!m_résultat.globales.est_vide()) {
            partitionne_globales_info_types();

            auto fonc_init = m_compilatrice_ri.genere_fonction_init_globales_et_appel(
                &m_espace, m_résultat.globales, fonction);

            if (!fonc_init) {
                return {};
            }

            ajoute_fonction(fonc_init);
        }

        /* Les métaprogrammes gèrent différemment les cas suivants, donc retournons directement. */
        return m_résultat;
    }

    if (decl_init_globales) {
        génère_ri_fonction_init_globales(decl_init_globales);
    }

    if (m_espace.options.utilise_trace_appel) {
        génère_traces_d_appel();
    }

    supprime_fonctions_inutilisées();
    supprime_types_inutilisés();

    génère_table_des_types();

    switch (m_espace.options.résultat) {
        case ResultatCompilation::RIEN:
        {
            break;
        }
        case ResultatCompilation::EXECUTABLE:
        {
            if (decl_principale == nullptr) {
                assert(m_espace.fonction_principale == nullptr);
                erreur::fonction_principale_manquante(m_espace);
                return {};
            }
            break;
        }
        case ResultatCompilation::FICHIER_OBJET:
        case ResultatCompilation::BIBLIOTHEQUE_STATIQUE:
        case ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE:
        {
            if (nombre_fonctions_racines == 0) {
                m_espace.rapporte_erreur_sans_site(
                    "Aucune fonction racine trouvée pour générer le code !\n");
                return {};
            }
            break;
        }
    }

    tri_fonctions_et_globales();

    return m_résultat;
}

void ConstructriceProgrammeFormeRI::ajoute_fonction(AtomeFonction *fonction)
{
    m_résultat.fonctions.ajoute(fonction);
    ajoute_dépendances_fonction(fonction);
}

void ConstructriceProgrammeFormeRI::ajoute_dépendances_fonction(AtomeFonction *fonction)
{
    m_visiteuse_atome.reinitialise();
    POUR (fonction->instructions) {
        m_visiteuse_atome.visite_atome(it, [&](Atome *atome_local) {
            if (atome_local->genre_atome == Atome::Genre::GLOBALE) {
                /* Ne visitons pas la sous-globale puisque nous la visitons ici. */
                ajoute_globale(atome_local->comme_globale(), false);
            }
        });
    }

    ajoute_type(const_cast<Type *>(fonction->type), true);

    POUR (fonction->instructions) {
        if (!it->type) {
            continue;
        }
        ajoute_type(const_cast<Type *>(it->type), true);
    }
}

void ConstructriceProgrammeFormeRI::ajoute_globale(AtomeGlobale *globale, bool visite_globale)
{
    if (m_globales_utilisées.possède(globale)) {
        return;
    }

    m_résultat.ajoute_globale(globale);
    m_globales_utilisées.insère(globale);

    /* Ces types ne sont pas utiles pour le code machine. */
    if (!est_globale_pour_tableau_données_constantes(globale)) {
        ajoute_type(const_cast<Type *>(globale->type), true);
    }

    if (!visite_globale) {
        return;
    }

    m_visiteuse_atome.reinitialise();
    m_visiteuse_atome.visite_atome(globale, [&](Atome *atome_local) {
        if (atome_local->genre_atome == Atome::Genre::GLOBALE) {
            /* Ne visitons pas la sous-globale puisque nous la visitons ici. */
            ajoute_globale(atome_local->comme_globale(), false);
        }
    });
}

void ConstructriceProgrammeFormeRI::ajoute_type(Type *type, bool visite_type)
{
    if (m_types_utilisés.possède(type)) {
        return;
    }

    m_résultat.types.ajoute(type);
    m_types_utilisés.insère(type);

    if (type->atome_info_type) {
        ajoute_globale(type->atome_info_type, true);
    }

    if (!visite_type) {
        return;
    }

    VisiteuseType visiteuse{};
    visiteuse.visite_type(type, [&](Type *type_enfant) { ajoute_type(type_enfant, false); });
}

void ConstructriceProgrammeFormeRI::génère_ri_fonction_init_globales(AtomeFonction *fonction)
{
    m_compilatrice_ri.génère_ri_pour_initialisation_globales(
        &m_espace, fonction, m_résultat.globales);

    /* Il faut ajourner les globales, car les globales référencées par les initialisations ne
     * sont peut-être pas encore dans la liste. */
    ajoute_dépendances_fonction(fonction);
}

void ConstructriceProgrammeFormeRI::génère_traces_d_appel()
{
    POUR (m_résultat.fonctions) {
        if (it->sanstrace || it->est_externe) {
            continue;
        }

        bool possède_appels = false;
        POUR_NOMME (inst, it->instructions) {
            if (!inst->est_appel()) {
                continue;
            }
            possède_appels = true;
            auto info_trace = m_compilatrice_ri.crée_info_appel_pour_trace_appel(
                inst->comme_appel());
            ajoute_globale(info_trace, true);
        }

        /* Ne créons pas de globale si la fonction n'appelle rien. */
        if (!possède_appels) {
            continue;
        }

        auto info_trace_appel = m_compilatrice_ri.crée_info_fonction_pour_trace_appel(it);
        ajoute_globale(info_trace_appel, true);
    }
}

void ConstructriceProgrammeFormeRI::génère_table_des_types()
{
    AtomeGlobale *atome_table_des_types = nullptr;
    POUR (m_résultat.globales) {
        if (it->ident == ID::__table_des_types) {
            atome_table_des_types = it;
            break;
        }
    }

    auto index_type = 0u;
    POUR (m_résultat.types) {
        if (est_type_tuple_ou_fonction_init_tuple(it)) {
            /* Ignore les tuples, nous ne devrions pas avoir de variables de ce type (aucune
             * variable de type type_de_données(tuple) n'est possible). */
            continue;
        }

        it->index_dans_table_types = index_type++;

        if (it->atome_info_type) {
            continue;
        }

        if (!atome_table_des_types) {
            /* La table n'est pas requise, ignorons-le. */
            continue;
        }

        /* Si la table des types est requise, créons un InfoType. */
        auto info_type = m_compilatrice_ri.crée_info_type(it, nullptr);
        ajoute_globale(info_type, true);
    }

    if (!atome_table_des_types) {
        return;
    }

    kuri::tableau<AtomeConstante *> table_des_types;
    table_des_types.reserve(index_type);

    POUR (m_résultat.types) {
        if (est_type_tuple_ou_fonction_init_tuple(it)) {
            continue;
        }

        if (!it->atome_info_type) {
            /* L'inclusion des globales des infos-type peut avoir ajouter des types qui n'auront
             * pas d'infos-type. Nous pouvons les ignorer car ce sont sans doute des types
             * auxiliaires (p.e. les types pour les tableaux de données constantes). */
            continue;
        }

        table_des_types.ajoute(m_compilatrice_ri.transtype_base_info_type(it->atome_info_type));
    }

    auto &typeuse = m_espace.compilatrice().typeuse;
    auto type_pointeur_info_type = typeuse.type_pointeur_pour(typeuse.type_info_type_);
    auto ident = m_espace.compilatrice().donne_identifiant_pour_globale("données_table_des_types");
    atome_table_des_types->initialisateur = m_compilatrice_ri.crée_tableau_global(
        *ident, type_pointeur_info_type, std::move(table_des_types));

    auto initialisateur = atome_table_des_types->initialisateur->comme_constante_structure();
    auto membres_init = initialisateur->donne_atomes_membres();
    auto atome_accès = membres_init[0]->comme_accès_index_constant();
    m_résultat.globales.ajoute(atome_accès->accede->comme_globale());

    auto type_tableau_fixe = typeuse.type_tableau_fixe(type_pointeur_info_type,
                                                       static_cast<int>(index_type));
    m_résultat.types.ajoute(type_tableau_fixe);
}

void ConstructriceProgrammeFormeRI::tri_fonctions_et_globales()
{
    /* Triage des globales :
     * - globales externes
     * - globales constantes (p.e. infos types, trace d'appels, etc.)
     * - globales internes non-constantes
     */
    auto partition_globales = partition_stable(m_résultat.globales,
                                               [](auto &globale) { return globale->est_externe; });
    m_résultat.définis_partition(partition_globales.vrai, ProgrammeRepreInter::GLOBALES_EXTERNES);
    m_résultat.définis_partition(partition_globales.faux, ProgrammeRepreInter::GLOBALES_INTERNES);

    partition_globales = partition_stable(partition_globales.faux,
                                          [](auto &globale) { return globale->est_constante; });

    /* Rassemble les globales selon leurs types. */
    kuri::tri_stable(partition_globales.vrai, [](auto &globale1, auto &globale2) {
        auto type1 = globale1->donne_type_alloué();
        auto type2 = globale2->donne_type_alloué();
        return type1 > type2;
    });

    m_résultat.définis_partition(partition_globales.vrai,
                                 ProgrammeRepreInter::GLOBALES_CONSTANTES);
    m_résultat.définis_partition(partition_globales.faux, ProgrammeRepreInter::GLOBALES_MUTABLES);

    /* Triage des fonctions :
     * - fonctions externes
     * - fonctions internes enlignées
     * - fonctions internes horslignées
     */
    auto partition_fonctions = partition_stable(
        m_résultat.fonctions, [](auto &fonction) { return fonction->est_externe; });
    m_résultat.définis_partition(partition_fonctions.vrai,
                                 ProgrammeRepreInter::FONCTIONS_EXTERNES);
    m_résultat.définis_partition(partition_fonctions.faux,
                                 ProgrammeRepreInter::FONCTIONS_INTERNES);

    tri_stable(partition_fonctions.vrai, [](auto &fonction1, auto &fonction2) {
        auto bib1 = fonction1->decl->données_externes->symbole->bibliotheque;
        auto bib2 = fonction2->decl->données_externes->symbole->bibliotheque;
        return bib1->nom < bib2->nom;
    });

    partition_fonctions = partition_stable(partition_fonctions.faux,
                                           [](auto &fonction) { return fonction->enligne; });
    m_résultat.définis_partition(partition_fonctions.vrai,
                                 ProgrammeRepreInter::FONCTIONS_ENLIGNÉES);
    m_résultat.définis_partition(partition_fonctions.faux,
                                 ProgrammeRepreInter::FONCTIONS_HORSLIGNÉES);
}

void ConstructriceProgrammeFormeRI::partitionne_globales_info_types()
{
    kuri::tableau<AtomeGlobale *> globales_infos_types;
    POUR (m_résultat.globales) {
        if (it->est_info_type_de) {
            globales_infos_types.ajoute(it);
        }
    }

    m_visiteuse_atome.reinitialise();
    kuri::ensemble<AtomeGlobale *> globales_utilisées_par_infos;
    POUR (globales_infos_types) {
        globales_utilisées_par_infos.insère(it);
        m_visiteuse_atome.visite_atome(it, [&](Atome *atome_visité) {
            if (!atome_visité->est_globale()) {
                return;
            }

            if (est_globale_pour_tableau_données_constantes(atome_visité->comme_globale())) {
                return;
            }

            globales_utilisées_par_infos.insère(atome_visité->comme_globale());
        });
    }

    auto partition = partition_stable(m_résultat.globales, [&](auto &globale) {
        return !globales_utilisées_par_infos.possède(globale);
    });

    m_résultat.définis_partition(partition.vrai, ProgrammeRepreInter::GLOBALES_NON_INFO_TYPES);
    m_résultat.définis_partition(partition.faux, ProgrammeRepreInter::GLOBALES_INFO_TYPES);
}

void ConstructriceProgrammeFormeRI::supprime_fonctions_inutilisées()
{
    kuri::pile<AtomeFonction *> fonction_à_visiter;
    POUR (m_fonctions_racines) {
        fonction_à_visiter.empile(it);
    }

    m_visiteuse_atome.reinitialise();
    POUR (m_résultat.globales) {
        m_visiteuse_atome.visite_atome(it, [&](Atome *atome) {
            if (atome->est_fonction()) {
                fonction_à_visiter.empile(atome->comme_fonction());
            }
        });
    }

    kuri::tableau<AtomeFonction *> fonctions_utilisées;
    kuri::ensemble<AtomeFonction *> fonctions_visitées;
    m_visiteuse_atome.reinitialise();
    while (!fonction_à_visiter.est_vide()) {
        auto fonction = fonction_à_visiter.depile();
        if (fonctions_visitées.possède(fonction)) {
            continue;
        }

        fonctions_visitées.insère(fonction);
        fonctions_utilisées.ajoute(fonction);
        m_visiteuse_atome.reinitialise();

        POUR (fonction->instructions) {
            m_visiteuse_atome.visite_atome(it, [&](Atome *atome) {
                if (atome->est_fonction()) {
                    fonction_à_visiter.empile(atome->comme_fonction());
                }
            });
        }
    }

    auto part = kuri::partition_stable(m_résultat.fonctions, [&](auto &fonction1) {
        return fonctions_visitées.possède(fonction1);
    });

    m_résultat.fonctions.redimensionne(part.vrai.taille());

#if 0
    pour_chaque_element(m_espace.compilatrice().sys_module->modules, [&](Module const &module) {
        if (module.nom()->nom != "Coeur") {
            return;
        }

        POUR (*module.bloc->membres.verrou_lecture()) {
            if (!it->est_entete_fonction()) {
                continue;
            }

            auto fonction = it->comme_entete_fonction();
            if (fonction->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
                if (!fonction->monomorphisations || fonction->monomorphisations->taille() == 0) {
                    dbg() << "Fonction inutilisée : " << nom_humainement_lisible(fonction);
                }
                continue;
            }

            if (!fonction->atome) {
                dbg() << "Fonction sans atome : " << nom_humainement_lisible(fonction);
                continue;
            }

            if (fonctions_visitées.possède(fonction->atome->comme_fonction())) {
                continue;
            }

            dbg() << "Fonction inutilisée : " << nom_humainement_lisible(fonction);
        }
    });
#endif
}

void ConstructriceProgrammeFormeRI::supprime_types_inutilisés()
{
    kuri::ensemble<Type const *> types_utilisés;
    POUR (m_résultat.globales) {
        types_utilisés.insère(it->donne_type_alloué());
    }

    POUR (m_résultat.fonctions) {
        if (it->est_externe) {
            POUR_NOMME (param, it->params_entrees) {
                types_utilisés.insère(param->donne_type_alloué());
            }

            types_utilisés.insère(it->param_sortie->donne_type_alloué());
        }

        POUR_NOMME (inst, it->instructions) {
            if (inst->est_stocke_mem()) {
                types_utilisés.insère(inst->comme_stocke_mem()->valeur->type);
            }
            else if (inst->est_charge()) {
                types_utilisés.insère(inst->comme_charge()->chargee->type);
            }
            else if (inst->est_alloc()) {
                types_utilisés.insère(inst->comme_alloc()->donne_type_alloué());
            }
            else if (inst->est_transtype()) {
                types_utilisés.insère(inst->type);
            }
        }
    }

    kuri::pile<Type const *> types_à_visiter;
    types_utilisés.pour_chaque_element([&](Type const *type) { types_à_visiter.empile(type); });

    VisiteuseType visiteuse{};
    visiteuse.visite_types_fonctions_init = false;
    while (!types_à_visiter.est_vide()) {
        auto type = types_à_visiter.depile();
        visiteuse.visite_type(const_cast<Type *>(type), [&](Type *tv) {
            types_à_visiter.empile(tv);
            types_utilisés.insère(tv);
        });
    }

    auto part_type = kuri::partition_stable(
        m_résultat.types, [&](auto &type) { return types_utilisés.possède(type); });
    m_résultat.types.redimensionne(part_type.vrai.taille());
}

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
        POUR (programme.donne_types()) {
            os << "-- " << chaine_type(it) << '\n';
        }
    }

    if (quoi == IMPRIME_TOUT || (quoi & IMPRIME_FONCTIONS) != 0) {
        os << "Fonctions dans le programme...\n";
        POUR (programme.donne_fonctions()) {
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
        POUR (programme.donne_globales()) {
            if (it->ident) {
                os << "-- " << it->ident->nom << '\n';
            }
            else {
                os << "-- anonyme de type " << chaine_type(it->type) << '\n';
            }
        }
    }
}

std::optional<ProgrammeRepreInter> représentation_intermédiaire_programme(
    EspaceDeTravail &espace, CompilatriceRI &compilatrice_ri, Programme const &programme)
{
    auto constructrice = ConstructriceProgrammeFormeRI(espace, compilatrice_ri, programme);
    return constructrice.construit_représentation_intermédiaire_programme();
}

/* Cette fonction n'existe que parce que la principale peut ajouter des globales pour les
 * constructeurs globaux... */
void ProgrammeRepreInter::ajoute_globale(AtomeGlobale *globale)
{
    if (est_globale_pour_tableau_données_constantes(globale)) {
        auto tableau_constant = globale->initialisateur->comme_données_constantes();
        m_données_constantes.tableaux_constants.ajoute({globale, tableau_constant, 0});
        return;
    }

    globales.ajoute(globale);
}

kuri::tableau_statique<AtomeGlobale *> ProgrammeRepreInter::donne_globales() const
{
    return globales;
}

kuri::tableau_statique<AtomeGlobale *> ProgrammeRepreInter::donne_globales_internes() const
{
    auto données = partitions_globales[GLOBALES_INTERNES];
    return {const_cast<AtomeGlobale **>(globales.begin()) + données.first, données.second};
}

kuri::tableau_statique<AtomeGlobale *> ProgrammeRepreInter::donne_globales_info_types() const
{
    auto données = partitions_globales[GLOBALES_INFO_TYPES];
    return {const_cast<AtomeGlobale **>(globales.begin()) + données.first, données.second};
}

kuri::tableau_statique<AtomeGlobale *> ProgrammeRepreInter::donne_globales_non_info_types() const
{
    auto données = partitions_globales[GLOBALES_NON_INFO_TYPES];
    return {const_cast<AtomeGlobale **>(globales.begin()) + données.first, données.second};
}

kuri::tableau_statique<AtomeFonction *> ProgrammeRepreInter::donne_fonctions() const
{
    return fonctions;
}

kuri::tableau_statique<AtomeFonction *> ProgrammeRepreInter::donne_fonctions_enlignées() const
{
    auto données = partitions_fonctions[FONCTIONS_ENLIGNÉES];
    return {const_cast<AtomeFonction **>(fonctions.begin()) + données.first, données.second};
}

kuri::tableau_statique<AtomeFonction *> ProgrammeRepreInter::donne_fonctions_horslignées() const
{
    auto données = partitions_fonctions[FONCTIONS_HORSLIGNÉES];
    return {const_cast<AtomeFonction **>(fonctions.begin()) + données.first, données.second};
}

kuri::tableau_statique<Type *> ProgrammeRepreInter::donne_types() const
{
    return types;
}

static Type const *donne_type_élément(AtomeConstanteDonnéesConstantes const *tableau)
{
    return tableau->type->comme_type_tableau_fixe()->type_pointe;
}

std::optional<const DonnéesConstantes *> ProgrammeRepreInter::donne_données_constantes() const
{
    if (m_données_constantes.tableaux_constants.est_vide()) {
        return {};
    }

    if (m_données_constantes_construites) {
        return &m_données_constantes;
    }

    /* Trie les tableaux selon le type de données, en mettant les tableaux des types les plus
     * grands en premier. */
    std::sort(m_données_constantes.tableaux_constants.begin(),
              m_données_constantes.tableaux_constants.end(),
              [](auto &tableau1, auto &tableau2) {
                  auto type_élément_tableau1 = donne_type_élément(tableau1.tableau);
                  auto type_élément_tableau2 = donne_type_élément(tableau2.tableau);
                  return type_élément_tableau1->taille_octet > type_élément_tableau2->taille_octet;
              });

    /* Calcul de la taille finale ainsi que du rembourrage pour chaque tableau. */
    auto alignement_désiré = 1u;
    POUR (m_données_constantes.tableaux_constants) {
        auto décalage = m_données_constantes.taille_données_tableaux_constants;
        auto type_élément = donne_type_élément(it.tableau);
        auto rembourrage = décalage % type_élément->alignement;
        alignement_désiré = std::max(alignement_désiré, type_élément->alignement);
        décalage += rembourrage;

        it.décalage_dans_données_constantes = décalage;
        it.rembourrage = rembourrage;

        auto taille_tableau = it.tableau->donne_données().taille();
        m_données_constantes.taille_données_tableaux_constants += taille_tableau + rembourrage;
    }

    m_données_constantes.alignement_désiré = alignement_désiré;

    m_données_constantes_construites = true;
    return &m_données_constantes;
}

static void rassemble_bibliothèques_utilisées(kuri::tableau<Bibliotheque *> &bibliothèques,
                                              kuri::ensemble<Bibliotheque *> &utilisées,
                                              Bibliotheque *bibliothèque)
{
    if (utilisées.possède(bibliothèque)) {
        return;
    }

    bibliothèques.ajoute(bibliothèque);
    utilisées.insère(bibliothèque);

    POUR (bibliothèque->dependances.plage()) {
        rassemble_bibliothèques_utilisées(bibliothèques, utilisées, it);
    }
}

kuri::tableau<Bibliotheque *> ProgrammeRepreInter::donne_bibliothèques_utilisées() const
{
    kuri::tableau<Bibliotheque *> résultat;
    kuri::ensemble<Bibliotheque *> bibliothèques_utilisées;
    POUR (fonctions) {
        if (it->decl && it->decl->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE) &&
            it->decl->données_externes && it->decl->données_externes->symbole) {
            rassemble_bibliothèques_utilisées(résultat,
                                              bibliothèques_utilisées,
                                              it->decl->données_externes->symbole->bibliotheque);
        }
    }
    POUR (globales) {
        if (!it->decl) {
            continue;
        }
        if (!it->decl->données_externes) {
            continue;
        }
        rassemble_bibliothèques_utilisées(
            résultat, bibliothèques_utilisées, it->decl->données_externes->symbole->bibliotheque);
    }
    return résultat;
}

/** \} */
