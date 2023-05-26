/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "programme.hh"

#include "biblinternes/outils/conditions.h"

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/identifiant.hh"

#include "representation_intermediaire/instructions.hh"

#include "compilatrice.hh"
#include "coulisse.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "typage.hh"

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
    m_fonctions_utilisees.insere(fonction);
    ajoute_fichier(m_espace->compilatrice().fichier(fonction->lexeme->fichier));
    elements_sont_sales[FONCTIONS][POUR_TYPAGE] = true;
    elements_sont_sales[FONCTIONS][POUR_RI] = true;
    if (fonction->possede_drapeau(DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insere(fonction);
    }
}

void Programme::ajoute_globale(NoeudDeclarationVariable *globale)
{
    if (possede(globale)) {
        return;
    }
    m_globales.ajoute(globale);
    m_globales_utilisees.insere(globale);
    ajoute_fichier(m_espace->compilatrice().fichier(globale->lexeme->fichier));
    elements_sont_sales[GLOBALES][POUR_TYPAGE] = true;
    elements_sont_sales[GLOBALES][POUR_RI] = true;
    if (globale->possede_drapeau(DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insere(globale);
    }
}

void Programme::ajoute_type(Type *type, RaisonAjoutType raison, NoeudExpression *noeud)
{
    if (possede(type)) {
        return;
    }
    m_types.ajoute(type);
    m_types_utilises.insere(type);
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
    if (decl && decl->possede_drapeau(DÉPENDANCES_FURENT_RÉSOLUES)) {
        m_dépendances_manquantes.insere(decl);
    }
}

bool Programme::typages_termines(DiagnostiqueEtatCompilation &diagnostique) const
{
    if (elements_sont_sales[FONCTIONS][POUR_TYPAGE]) {
        POUR (m_fonctions) {
            if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                diagnostique.declaration_a_valider = it;
                return false;
            }

            if (!it->est_externe && !it->corps->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                diagnostique.declaration_a_valider = it;
                return false;
            }
        }
        elements_sont_sales[FONCTIONS][POUR_TYPAGE] = false;
    }

    if (elements_sont_sales[GLOBALES][POUR_TYPAGE]) {
        POUR (m_globales) {
            if (!it->possede_drapeau(DECLARATION_FUT_VALIDEE)) {
                diagnostique.declaration_a_valider = it;
                return false;
            }
        }
        elements_sont_sales[GLOBALES][POUR_TYPAGE] = false;
    }

    if (elements_sont_sales[TYPES][POUR_TYPAGE]) {
        POUR (m_types) {
            if ((it->drapeaux & TYPE_FUT_VALIDE) == 0) {
                diagnostique.type_a_valider = it;
                return false;
            }
        }
        elements_sont_sales[TYPES][POUR_TYPAGE] = false;
    }

    diagnostique.toutes_les_declarations_a_typer_le_sont = true;
    return true;
}

bool Programme::ri_generees(DiagnostiqueEtatCompilation &diagnostique) const
{
    if (!typages_termines(diagnostique)) {
        return false;
    }

    using dls::outils::est_element;

    if (elements_sont_sales[FONCTIONS][POUR_RI]) {
        POUR (m_fonctions) {
            if (!it->possede_drapeau(RI_FUT_GENEREE) && !est_element(it->ident,
                                                                     ID::init_execution_kuri,
                                                                     ID::fini_execution_kuri,
                                                                     ID::init_globales_kuri)) {
                assert_rappel(it->unite, [&]() {
                    std::cerr << "Aucune unité pour de compilation pour :\n";
                    erreur::imprime_site(*m_espace, it);
                });
                diagnostique.ri_declaration_a_generer = it;
                return false;
            }
        }
        elements_sont_sales[FONCTIONS][POUR_RI] = false;
    }

    if (elements_sont_sales[GLOBALES][POUR_RI]) {
        POUR (m_globales) {
            if (!it->possede_drapeau(RI_FUT_GENEREE)) {
                diagnostique.ri_declaration_a_generer = it;
                return false;
            }
        }
        elements_sont_sales[GLOBALES][POUR_RI] = false;
    }

    if (elements_sont_sales[TYPES][POUR_RI]) {
        POUR (m_types) {
            if (!it->requiers_fonction_initialisation()) {
                continue;
            }

            if (it->requiers_création_fonction_initialisation()) {
                diagnostique.fonction_initialisation_type_a_creer = it;
                return false;
            }

            if (it->fonction_init && !it->fonction_init->possede_drapeau(RI_FUT_GENEREE)) {
                diagnostique.ri_type_a_generer = it;
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
    auto diagnostic = diagnositique_compilation();
    return diagnostic.toutes_les_ri_sont_generees;
}

DiagnostiqueEtatCompilation Programme::diagnositique_compilation() const
{
    DiagnostiqueEtatCompilation diagnositique{};
    verifie_etat_compilation_fichier(diagnositique);
    ri_generees(diagnositique);
    return diagnositique;
}

EtatCompilation Programme::ajourne_etat_compilation()
{
    auto diagnostic = diagnositique_compilation();

    // À FAIRE(gestion) : ceci n'est que pour les métaprogrammes
    m_etat_compilation.essaie_d_aller_a(PhaseCompilation::PARSAGE_EN_COURS);
    m_etat_compilation.essaie_d_aller_a(PhaseCompilation::PARSAGE_TERMINE);

    if (diagnostic.toutes_les_declarations_a_typer_le_sont) {
        m_etat_compilation.essaie_d_aller_a(PhaseCompilation::TYPAGE_TERMINE);
    }

    if (diagnostic.toutes_les_ri_sont_generees) {
        m_etat_compilation.essaie_d_aller_a(PhaseCompilation::GENERATION_CODE_TERMINEE);
    }

    return m_etat_compilation;
}

void Programme::change_de_phase(PhaseCompilation phase)
{
    m_etat_compilation.essaie_d_aller_a(phase);
}

int64_t Programme::memoire_utilisee() const
{
    auto memoire = 0l;
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
        modules.insere(it->module);
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

void Programme::verifie_etat_compilation_fichier(DiagnostiqueEtatCompilation &diagnostique) const
{
    diagnostique.tous_les_fichiers_sont_charges = true;
    diagnostique.tous_les_fichiers_sont_lexes = true;
    diagnostique.tous_les_fichiers_sont_parses = true;

    if (!m_fichiers_sont_sales) {
        return;
    }

    POUR (m_fichiers) {
        if (!it->fut_charge) {
            diagnostique.tous_les_fichiers_sont_charges = false;
        }

        if (!it->fut_lexe) {
            diagnostique.tous_les_fichiers_sont_lexes = false;
        }

        if (!it->fut_parse) {
            diagnostique.tous_les_fichiers_sont_parses = false;
        }
    }

    m_fichiers_sont_sales = false;
}

void Programme::ajoute_fichier(Fichier *fichier)
{
    if (m_fichiers_utilises.possede(fichier)) {
        return;
    }

    m_fichiers.ajoute(fichier);
    m_fichiers_utilises.insere(fichier);
    m_fichiers_sont_sales = true;
}

void Programme::ajoute_racine(NoeudDeclarationEnteteFonction *racine)
{
    if (pour_metaprogramme()) {
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

// --------------------------------------------------------
// RepresentationIntermediaireProgramme

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

template <typename T>
static kuri::ensemble<T> cree_ensemble(const kuri::tableau<T> &tableau)
{
    kuri::ensemble<T> resultat;

    POUR (tableau) {
        resultat.insere(it);
    }

    return resultat;
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
            if (globales_utilisees.possede(static_cast<AtomeGlobale *>(atome_local))) {
                return;
            }

            repr_inter.globales.ajoute(static_cast<AtomeGlobale *>(atome_local));
            globales_utilisees.insere(static_cast<AtomeGlobale *>(atome_local));
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
    auto globales_utilisees = cree_ensemble(repr_inter.globales);
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

struct VisiteuseType {
    kuri::ensemble<Type *> visites{};

    void visite_type(Type *type, std::function<void(Type *)> rappel)
    {
        if (!type) {
            return;
        }

        if (visites.possede(type)) {
            return;
        }

        visites.insere(type);
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
                auto reference = type->comme_reference();
                visite_type(reference->type_pointe, rappel);
                break;
            }
            case GenreType::POINTEUR:
            {
                auto pointeur = type->comme_pointeur();
                visite_type(pointeur->type_pointe, rappel);
                break;
            }
            case GenreType::UNION:
            {
                auto type_union = type->comme_union();
                POUR (type_union->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            case GenreType::STRUCTURE:
            {
                auto type_structure = type->comme_structure();
                POUR (type_structure->membres) {
                    visite_type(it.type, rappel);
                }
                break;
            }
            case GenreType::TABLEAU_DYNAMIQUE:
            {
                auto tableau = type->comme_tableau_dynamique();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreType::TABLEAU_FIXE:
            {
                auto tableau = type->comme_tableau_fixe();
                visite_type(tableau->type_pointe, rappel);
                break;
            }
            case GenreType::VARIADIQUE:
            {
                auto variadique = type->comme_variadique();
                visite_type(variadique->type_pointe, rappel);
                break;
            }
            case GenreType::FONCTION:
            {
                auto fonction = type->comme_fonction();
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
                visite_type(type_enum->type_donnees, rappel);
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

static void rassemble_types_supplementaires(ProgrammeRepreInter &repr_inter)
{
    /* Ajoute les types de toutes les globales et toutes les fonctions, dans le cas où nous en
     * aurions ajoutées (qui ne sont pas dans le programme initiale). */
    auto type_utilises = cree_ensemble(repr_inter.types);

    VisiteuseType visiteuse{};
    auto ajoute_type_si_necessaire = [&](Type const *type_racine) {
        visiteuse.visite_type(const_cast<Type *>(type_racine), [&](Type *type) {
            if (type_utilises.possede(type)) {
                return;
            }

            type_utilises.insere(type);
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

ProgrammeRepreInter representation_intermediaire_programme(Programme const &programme)
{
    auto resultat = ProgrammeRepreInter{};

    resultat.fonctions.reserve(programme.fonctions().taille() + programme.types().taille());
    resultat.globales.reserve(programme.globales().taille());

    /* Nous pouvons directement copier les types. */
    resultat.types = programme.types();

    /* Extrait les atomes pour les fonctions. */
    POUR (programme.fonctions()) {
        assert_rappel(it->possede_drapeau(RI_FUT_GENEREE), [&]() {
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
        assert_rappel(it->possede_drapeau(RI_FUT_GENEREE), [&]() {
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

void imprime_diagnostique(const DiagnostiqueEtatCompilation &diagnositic)
{
    if (!diagnositic.toutes_les_declarations_a_typer_le_sont) {
        if (diagnositic.type_a_valider) {
            std::cerr << "-- validation non performée pour le type : "
                      << chaine_type(diagnositic.type_a_valider) << '\n';
        }
        if (diagnositic.declaration_a_valider) {
            std::cerr << "-- validation non performée pour déclaration "
                      << nom_humainement_lisible(diagnositic.declaration_a_valider) << '\n';
        }
        return;
    }

    if (diagnositic.fonction_initialisation_type_a_creer) {
        std::cerr << "-- fonction d'initialisation non-créée pour le type : "
                  << chaine_type(diagnositic.ri_type_a_generer) << '\n';
    }
    if (diagnositic.ri_type_a_generer) {
        std::cerr << "-- RI non générée pour la fonction d'initialisation du type : "
                  << chaine_type(diagnositic.ri_type_a_generer) << '\n';
    }
    if (diagnositic.ri_declaration_a_generer) {
        std::cerr << "-- RI non générée pour déclaration "
                  << diagnositic.ri_declaration_a_generer->lexeme->chaine << '\n';
    }
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
    auto globales_utilisees = cree_ensemble(this->globales);
    VisiteuseAtome visiteuse{};
    rassemble_globales_supplementaires(*this, fonction, visiteuse, globales_utilisees);
    /* Les types ont peut-être changé. */
    rassemble_types_supplementaires(*this);
}
