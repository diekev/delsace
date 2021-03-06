/*
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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "espace_de_travail.hh"

#include <fstream>

#include "biblinternes/outils/sauvegardeuse_etat.hh"

#include "representation_intermediaire/constructrice_ri.hh"
#include "representation_intermediaire/impression.hh"
#include "representation_intermediaire/instructions.hh"

#include "arbre_syntaxique/noeud_expression.hh"
#include "coulisse.hh"
#include "parsage/identifiant.hh"
#include "statistiques/statistiques.hh"

/* ************************************************************************** */

/* Redéfini certaines fonction afin de pouvoir controler leurs comportements.
 * Par exemple, pour les fonctions d'allocations nous voudrions pouvoir libérer
 * la mémoire de notre coté, ou encore vérifier qu'il n'y ait pas de fuite de
 * mémoire dans les métaprogrammes.
 */
static void *notre_malloc(size_t n)
{
    return malloc(n);
}

static void *notre_realloc(void *ptr, size_t taille)
{
    return realloc(ptr, taille);
}

static void notre_free(void *ptr)
{
    free(ptr);
}

/* ************************************************************************** */

EspaceDeTravail::EspaceDeTravail(Compilatrice &compilatrice, OptionsDeCompilation opts)
    : options(opts), typeuse(graphe_dependance, this->operateurs), m_compilatrice(compilatrice)
{
    auto ops = operateurs.verrou_ecriture();
    enregistre_operateurs_basiques(*this, *ops);
    coulisse = Coulisse::cree_pour_options(options);

    gestionnaire_bibliotheques->ajoute_bibliotheque("/lib/x86_64-linux-gnu/libc.so.6");
    gestionnaire_bibliotheques->ajoute_bibliotheque("/tmp/r16_tables_x64.so");

    gestionnaire_bibliotheques->ajoute_fonction_pour_symbole(
        ID::malloc_, reinterpret_cast<GestionnaireBibliotheques::type_fonction>(notre_malloc));
    gestionnaire_bibliotheques->ajoute_fonction_pour_symbole(
        ID::realloc_, reinterpret_cast<GestionnaireBibliotheques::type_fonction>(notre_realloc));
    gestionnaire_bibliotheques->ajoute_fonction_pour_symbole(
        ID::free_, reinterpret_cast<GestionnaireBibliotheques::type_fonction>(notre_free));
}

EspaceDeTravail::~EspaceDeTravail()
{
    Coulisse::detruit(coulisse);
}

Module *EspaceDeTravail::trouve_ou_cree_module(dls::outils::Synchrone<SystemeModule> &sys_module,
                                               IdentifiantCode *nom_module,
                                               kuri::chaine_statique chemin)
{
    auto donnees_module = sys_module->trouve_ou_cree_module(nom_module, chemin);

    auto modules_ = modules.verrou_ecriture();

    POUR_TABLEAU_PAGE ((*modules_)) {
        if (it.donnees_constantes == donnees_module) {
            return &it;
        }
    }

    return modules_->ajoute_element(donnees_module);
}

Module *EspaceDeTravail::module(const IdentifiantCode *nom_module) const
{
    auto modules_ = modules.verrou_lecture();
    POUR_TABLEAU_PAGE ((*modules_)) {
        if (it.nom() == nom_module) {
            return const_cast<Module *>(&it);
        }
    }

    return nullptr;
}

ResultatFichier EspaceDeTravail::trouve_ou_cree_fichier(
    dls::outils::Synchrone<SystemeModule> &sys_module,
    Module *module,
    kuri::chaine_statique nom_fichier,
    kuri::chaine_statique chemin,
    bool importe_kuri)
{
    auto donnees_fichier = sys_module->trouve_ou_cree_fichier(nom_fichier, chemin);

    auto fichiers_ = fichiers.verrou_ecriture();

    /* fait de la place la table */
    table_fichiers.redimensionne(donnees_fichier->id + 1, nullptr);

    if (table_fichiers[donnees_fichier->id] != nullptr) {
        return FichierExistant(*table_fichiers[donnees_fichier->id]);
    }

    auto fichier = fichiers_->ajoute_element(donnees_fichier);

    if (importe_kuri && module->nom() != ID::Kuri) {
        assert(module_kuri);
        fichier->modules_importes.insere(module_kuri);
    }

    fichier->module = module;
    module->fichiers.ajoute(fichier);

    table_fichiers[donnees_fichier->id] = fichier;

    return FichierNeuf(*fichier);
}

Fichier *EspaceDeTravail::fichier(long index) const
{
    auto fichiers_ = fichiers.verrou_lecture();
    return table_fichiers[index];
}

Fichier *EspaceDeTravail::fichier(const dls::vue_chaine_compacte &chemin) const
{
    auto fichiers_ = fichiers.verrou_lecture();

    POUR_TABLEAU_PAGE ((*fichiers_)) {
        if (dls::vue_chaine_compacte(it.chemin()) == chemin) {
            return const_cast<Fichier *>(&it);
        }
    }

    return nullptr;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme,
                                              const kuri::chaine &nom_fichier)
{
    std::unique_lock lock(mutex_atomes_fonctions);
    auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fichier);
    return atome_fonc;
}

AtomeFonction *EspaceDeTravail::cree_fonction(const Lexeme *lexeme,
                                              const kuri::chaine &nom_fonction,
                                              kuri::tableau<Atome *, int> &&params)
{
    std::unique_lock lock(mutex_atomes_fonctions);
    auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fonction, std::move(params));
    return atome_fonc;
}

/* Il existe des dépendances cycliques entre les fonctions qui nous empêche de
 * générer le code linéairement. Cette fonction nous sers soit à trouver le
 * pointeur vers l'atome d'une fonction si nous l'avons déjà généré, soit de le
 * créer en préparation de la génération de la RI de son corps.
 */
AtomeFonction *EspaceDeTravail::trouve_ou_insere_fonction(ConstructriceRI &constructrice,
                                                          NoeudDeclarationEnteteFonction *decl)
{
    std::unique_lock lock(mutex_atomes_fonctions);

    if (decl->atome) {
        return static_cast<AtomeFonction *>(decl->atome);
    }

    SAUVEGARDE_ETAT(constructrice.fonction_courante);

    auto params = kuri::tableau<Atome *, int>();
    params.reserve(decl->params.taille());

    if (!decl->est_externe && !decl->possede_drapeau(FORCE_NULCTX)) {
        auto atome = constructrice.cree_allocation(decl, typeuse.type_contexte, ID::contexte);
        params.ajoute(atome);
    }

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = decl->parametre_entree(i);
        auto atome = constructrice.cree_allocation(decl, param->type, param->ident);
        param->atome = atome;
        params.ajoute(atome);
    }

    /* Pour les sorties multiples, les valeurs de sorties sont des accès de
     * membres du tuple, ainsi nous n'avons pas à compliquer la génération de
     * code ou sa simplification.
     */

    auto param_sortie = decl->param_sortie;
    auto atome_param_sortie = constructrice.cree_allocation(
        decl, param_sortie->type, param_sortie->ident);
    param_sortie->atome = atome_param_sortie;

    if (decl->params_sorties.taille() > 1) {
        auto index_membre = 0;
        POUR (decl->params_sorties) {
            it->comme_declaration_variable()->atome = constructrice.cree_reference_membre(
                it, atome_param_sortie, index_membre++, true);
        }
    }

    auto atome_fonc = fonctions.ajoute_element(
        decl->lexeme, decl->nom_broye(constructrice.espace()), std::move(params));
    atome_fonc->type = normalise_type(typeuse, decl->type);
    atome_fonc->est_externe = decl->est_externe;
    atome_fonc->sanstrace = decl->possede_drapeau(FORCE_SANSTRACE);
    atome_fonc->decl = decl;
    atome_fonc->param_sortie = atome_param_sortie;
    atome_fonc->enligne = decl->possede_drapeau(FORCE_ENLIGNE);

    decl->atome = atome_fonc;

    return atome_fonc;
}

AtomeFonction *EspaceDeTravail::trouve_ou_insere_fonction_init(ConstructriceRI &constructrice,
                                                               Type *type)
{
    std::unique_lock lock(mutex_atomes_fonctions);

    if (type->fonction_init) {
        return type->fonction_init;
    }

    auto nom_fonction = enchaine("initialise_", type);

    SAUVEGARDE_ETAT(constructrice.fonction_courante);

    auto types_entrees = dls::tablet<Type *, 6>(1);
    types_entrees[0] = typeuse.type_pointeur_pour(normalise_type(typeuse, type), false);

    auto type_sortie = typeuse[TypeBase::RIEN];

    auto params = kuri::tableau<Atome *, int>(1);
    params[0] = constructrice.cree_allocation(nullptr, types_entrees[0], ID::pointeur);

    auto param_sortie = constructrice.cree_allocation(nullptr, typeuse[TypeBase::RIEN], nullptr);

    auto atome_fonc = fonctions.ajoute_element(nullptr, nom_fonction, std::move(params));
    atome_fonc->type = typeuse.type_fonction(types_entrees, type_sortie, false);
    atome_fonc->param_sortie = param_sortie;
    atome_fonc->enligne = true;
    atome_fonc->sanstrace = true;

    type->fonction_init = atome_fonc;

    return atome_fonc;
}

AtomeGlobale *EspaceDeTravail::cree_globale(Type *type,
                                            AtomeConstante *initialisateur,
                                            bool est_externe,
                                            bool est_constante)
{
    return globales.ajoute_element(
        typeuse.type_pointeur_pour(type, false), initialisateur, est_externe, est_constante);
}

AtomeGlobale *EspaceDeTravail::trouve_globale(NoeudDeclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);
    auto decl_var = decl->comme_declaration_variable();
    return static_cast<AtomeGlobale *>(decl_var->atome);
}

AtomeGlobale *EspaceDeTravail::trouve_ou_insere_globale(NoeudDeclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);

    auto decl_var = decl->comme_declaration_variable();

    if (decl_var->atome == nullptr) {
        decl_var->atome = cree_globale(decl->type, nullptr, false, false);
    }

    return static_cast<AtomeGlobale *>(decl_var->atome);
}

long EspaceDeTravail::memoire_utilisee() const
{
    auto memoire = 0l;

    memoire += modules->memoire_utilisee();
    memoire += fichiers->memoire_utilisee();

    auto modules_ = modules.verrou_lecture();
    POUR_TABLEAU_PAGE ((*modules_)) {
        memoire += it.fichiers.taille() * taille_de(Fichier *);
    }

    auto fichiers_ = fichiers.verrou_lecture();
    POUR_TABLEAU_PAGE ((*fichiers_)) {
        // les autres membres sont gérés dans rassemble_statistiques()
        if (!it.modules_importes.est_stocke_dans_classe()) {
            memoire += it.modules_importes.taille() * taille_de(dls::vue_chaine_compacte);
        }
    }

    return memoire;
}

void EspaceDeTravail::rassemble_statistiques(Statistiques &stats) const
{
    operateurs->rassemble_statistiques(stats);
    graphe_dependance->rassemble_statistiques(stats);
    gestionnaire_bibliotheques->rassemble_statistiques(stats);
    typeuse.rassemble_statistiques(stats);

    auto &stats_fichiers = stats.stats_fichiers;
    auto fichiers_ = fichiers.verrou_lecture();
    POUR_TABLEAU_PAGE ((*fichiers_)) {
        auto entree = EntreeFichier();
        entree.nom = it.nom();
        entree.temps_parsage = it.temps_analyse;

        stats_fichiers.fusionne_entree(entree);
    }

    auto &stats_ri = stats.stats_ri;

    auto memoire_fonctions = fonctions.memoire_utilisee();
    memoire_fonctions += fonctions.memoire_utilisee();
    pour_chaque_element(fonctions, [&](AtomeFonction const &it) {
        memoire_fonctions += it.params_entrees.taille_memoire();
        memoire_fonctions += it.chunk.capacite;
        memoire_fonctions += it.chunk.locales.taille_memoire();
        memoire_fonctions += it.chunk.decalages_labels.taille_memoire();
    });

    stats_ri.fusionne_entree({"fonctions", fonctions.taille(), memoire_fonctions});
    stats_ri.fusionne_entree({"globales", globales.taille(), globales.memoire_utilisee()});
}

MetaProgramme *EspaceDeTravail::cree_metaprogramme()
{
    return metaprogrammes->ajoute_element();
}

void EspaceDeTravail::tache_chargement_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
    nombre_taches_chargement += 1;
}

void EspaceDeTravail::tache_lexage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
    nombre_taches_lexage += 1;
}

void EspaceDeTravail::tache_parsage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::PARSAGE_EN_COURS);
    nombre_taches_parsage += 1;
}

void EspaceDeTravail::tache_typage_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    if (phase > PhaseCompilation::PARSAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::PARSAGE_TERMINE);
    }

    nombre_taches_typage += 1;
}

void EspaceDeTravail::tache_ri_ajoutee(dls::outils::Synchrone<Messagere> &messagere)
{
    if (phase > PhaseCompilation::TYPAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::TYPAGE_TERMINE);
    }

    nombre_taches_ri += 1;
}

void EspaceDeTravail::tache_optimisation_ajoutee(dls::outils::Synchrone<Messagere> & /*messagere*/)
{
    nombre_taches_optimisation += 1;
}

void EspaceDeTravail::tache_execution_ajoutee(dls::outils::Synchrone<Messagere> & /*messagere*/)
{
    nombre_taches_execution += 1;
}

void EspaceDeTravail::tache_chargement_terminee(dls::outils::Synchrone<Messagere> &messagere,
                                                Fichier *fichier)
{
    messagere->ajoute_message_fichier_ferme(this, fichier->chemin());

    /* Une fois que nous avons fini de charger un fichier, il faut le lexer. */
    tache_lexage_ajoutee(messagere);

    nombre_taches_chargement -= 1;
    assert(nombre_taches_chargement >= 0);
}

void EspaceDeTravail::tache_lexage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    /* Une fois que nous lexer quelque chose, il faut le parser. */
    tache_parsage_ajoutee(messagere);
    nombre_taches_lexage -= 1;
    assert(nombre_taches_lexage >= 0);
}

void EspaceDeTravail::tache_parsage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    nombre_taches_parsage -= 1;
    assert(nombre_taches_parsage >= 0);

    if (parsage_termine()) {
        change_de_phase(messagere, PhaseCompilation::PARSAGE_TERMINE);
    }
}

void EspaceDeTravail::tache_typage_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    nombre_taches_typage -= 1;
    assert(nombre_taches_typage >= 0);

    if (nombre_taches_typage == 0 && phase == PhaseCompilation::PARSAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::TYPAGE_TERMINE);
    }
}

void EspaceDeTravail::tache_ri_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    nombre_taches_ri -= 1;
    assert(nombre_taches_ri >= 0);

    if (optimisations) {
        tache_optimisation_ajoutee(messagere);
    }

    if (nombre_taches_ri == 0 && nombre_taches_optimisation == 0 &&
        phase == PhaseCompilation::TYPAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::GENERATION_CODE_TERMINEE);
    }
}

void EspaceDeTravail::tache_optimisation_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    nombre_taches_optimisation -= 1;
    assert(nombre_taches_optimisation >= 0);

    if (nombre_taches_ri == 0 && nombre_taches_optimisation == 0 &&
        phase == PhaseCompilation::TYPAGE_TERMINE) {
        change_de_phase(messagere, PhaseCompilation::GENERATION_CODE_TERMINEE);
    }
}

void EspaceDeTravail::tache_execution_terminee(dls::outils::Synchrone<Messagere> & /*messagere*/)
{
    nombre_taches_execution -= 1;
    assert(nombre_taches_execution >= 0);
}

void EspaceDeTravail::tache_generation_objet_terminee(dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::APRES_GENERATION_OBJET);
}

void EspaceDeTravail::tache_liaison_executable_terminee(
    dls::outils::Synchrone<Messagere> &messagere)
{
    change_de_phase(messagere, PhaseCompilation::APRES_LIAISON_EXECUTABLE);
}

bool EspaceDeTravail::peut_generer_code_final() const
{
    if (phase != PhaseCompilation::GENERATION_CODE_TERMINEE) {
        return false;
    }

    if (nombre_taches_execution == 0) {
        return true;
    }

    if (nombre_taches_execution == 1 && metaprogramme) {
        return true;
    }

    return false;
}

bool EspaceDeTravail::parsage_termine() const
{
    return nombre_taches_chargement == 0 && nombre_taches_lexage == 0 &&
           nombre_taches_parsage == 0;
}

void EspaceDeTravail::imprime_compte_taches(std::ostream &os) const
{
    os << "nombre_taches_chargement : " << nombre_taches_chargement << '\n';
    os << "nombre_taches_lexage : " << nombre_taches_lexage << '\n';
    os << "nombre_taches_parsage : " << nombre_taches_parsage << '\n';
    os << "nombre_taches_typage : " << nombre_taches_typage << '\n';
    os << "nombre_taches_ri : " << nombre_taches_ri << '\n';
    os << "nombre_taches_execution : " << nombre_taches_execution << '\n';
    os << "nombre_taches_optimisation : " << nombre_taches_optimisation << '\n';
}

void EspaceDeTravail::change_de_phase(dls::outils::Synchrone<Messagere> &messagere,
                                      PhaseCompilation nouvelle_phase)
{
    phase = nouvelle_phase;
    messagere->ajoute_message_phase_compilation(this);
}

PhaseCompilation EspaceDeTravail::phase_courante() const
{
    return phase;
}

void EspaceDeTravail::rapporte_avertissement(NoeudExpression *site,
                                             kuri::chaine_statique message) const
{
    std::cerr << genere_entete_erreur(this, site, erreur::Genre::AVERTISSEMENT, message);
}

void EspaceDeTravail::rapporte_avertissement(kuri::chaine const &chemin_fichier,
                                             int ligne,
                                             kuri::chaine const &message) const
{
    const Fichier *f = this->fichier({chemin_fichier.pointeur(), chemin_fichier.taille()});
    std::cerr << genere_entete_erreur(this, f, ligne, erreur::Genre::AVERTISSEMENT, message);
}

Erreur EspaceDeTravail::rapporte_erreur(NoeudExpression const *site,
                                        kuri::chaine_statique message,
                                        erreur::Genre genre) const
{
    possede_erreur = true;

    if (!site) {
        return rapporte_erreur_sans_site(message, genre);
    }

    return ::rapporte_erreur(this, site, message, genre);
}

Erreur EspaceDeTravail::rapporte_erreur(kuri::chaine const &fichier,
                                        int ligne,
                                        kuri::chaine const &message) const
{
    possede_erreur = true;
    return ::rapporte_erreur(this, fichier, ligne, message);
}

Erreur EspaceDeTravail::rapporte_erreur_sans_site(const kuri::chaine &message,
                                                  erreur::Genre genre) const
{
    possede_erreur = true;
    return ::rapporte_erreur_sans_site(this, message, genre);
}

void EspaceDeTravail::imprime_programme() const
{
    std::ofstream os;
    os.open("/tmp/ri_programme.kr");

    POUR_TABLEAU_PAGE (fonctions) {
        imprime_fonction(&it, os);
    }
}
