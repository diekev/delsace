/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "compilatrice.hh"

#include <iostream>
#include <stdarg.h>

#include "biblinternes/flux/outils.h"
#include "biblinternes/outils/sauvegardeuse_etat.hh"

#include "statistiques/statistiques.hh"

#include "parsage/lexeuse.hh"

#include "structures/chemin_systeme.hh"
#include "structures/date.hh"

#include "broyage.hh"
#include "environnement.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "gestionnaire_code.hh"
#include "intrinseques.hh"
#include "ipa.hh"
#include "portee.hh"
#include "programme.hh"

/* ************************************************************************** */

int64_t GestionnaireChainesAjoutees::ajoute(kuri::chaine chaine)
{
    int64_t decalage = nombre_total_de_lignes;

    POUR (chaine) {
        nombre_total_de_lignes += (it == '\n');
    }

    /* Nous ajoutons une ligne car toutes les chaines sont suffixées d'une ligne. */
    nombre_total_de_lignes += 1;
    m_chaines.ajoute(chaine);
    return decalage;
}

int GestionnaireChainesAjoutees::nombre_de_chaines() const
{
    return m_chaines.taille();
}

void GestionnaireChainesAjoutees::imprime_dans(std::ostream &os)
{
    auto d = hui_systeme();

    os << "Fichier créé le " << d.jour << "/" << d.mois << "/" << d.annee << " à " << d.heure
       << ':' << d.minute << ':' << d.seconde << "\n\n";

    POUR (m_chaines) {
        os << it;
        os << "\n";
    }
}

/* ************************************************************************** */

Compilatrice::Compilatrice(kuri::chaine chemin_racine_kuri, ArgumentsCompilatrice arguments_)
    : ordonnanceuse(this), messagere(this),
      gestionnaire_code(memoire::loge<GestionnaireCode>("GestionnaireCode", this)),
      gestionnaire_bibliotheques(GestionnaireBibliotheques(*this)), arguments(arguments_),
      racine_kuri(chemin_racine_kuri), typeuse(this->operateurs)
{
    initialise_identifiants_intrinsèques(*table_identifiants.verrou_ecriture());
    initialise_identifiants_ipa(*table_identifiants.verrou_ecriture());

    auto ops = operateurs.verrou_ecriture();
    enregistre_operateurs_basiques(typeuse, *ops);

    espace_de_travail_defaut = demarre_un_espace_de_travail({}, "Espace 1");

    /* Charge le module Kuri. */
    module_kuri = importe_module(espace_de_travail_defaut, "Kuri", nullptr);

    broyeuse = memoire::loge<Broyeuse>("Broyeuse");
}

Compilatrice::~Compilatrice()
{
    POUR ((*espaces_de_travail.verrou_ecriture())) {
        memoire::deloge("EspaceDeTravail", it);
    }

    memoire::deloge("Broyeuse", broyeuse);
    memoire::deloge("GestionnaireCode", gestionnaire_code);
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace,
                                     const kuri::chaine &nom,
                                     NoeudExpression const *site)
{
    auto chemin = kuri::chemin_systeme(nom);

    if (!kuri::chemin_systeme::existe(chemin)) {
        /* essaie dans la racine kuri */
        chemin = kuri::chemin_systeme(racine_kuri) / "modules" / chemin;

        if (!kuri::chemin_systeme::existe(chemin)) {
            espace
                ->rapporte_erreur(site, "Impossible de trouver le dossier correspondant au module")
                .ajoute_message("Le chemin testé fut : ", chemin);

            return nullptr;
        }
    }

    if (!kuri::chemin_systeme::est_dossier(chemin)) {
        erreur::lance_erreur("Le nom du module ne pointe pas vers un dossier",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);

        return nullptr;
    }

    /* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
    auto chemin_absolu = kuri::chemin_systeme::canonique_absolu(chemin);
    auto nom_dossier = chemin_absolu.nom_fichier();

    // @concurrence critique
    auto module = this->trouve_ou_cree_module(
        table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier), chemin_absolu);

    if (module->importe) {
        return module;
    }

    module->importe = true;

    messagere->ajoute_message_module_ouvert(espace, module);

#if 1
    auto fichiers = kuri::chemin_systeme::fichiers_du_dossier(chemin_absolu);

    POUR (fichiers) {
        auto resultat = this->trouve_ou_cree_fichier(
            module, it.nom_fichier_sans_extension(), it, importe_kuri);

        if (resultat.est<FichierNeuf>()) {
            gestionnaire_code->requiers_chargement(espace,
                                                   resultat.resultat<FichierNeuf>().fichier);
        }
    }
#else
    auto chemin_fichier_module = chemin_absolu / "module.kuri";

    if (!kuri::chemin_systeme::existe(chemin_fichier_module)) {
        espace->rapporte_erreur(site,
                                enchaine("Aucun fichier « module.kuri » trouvé pour le module « ",
                                         module->nom()->nom,
                                         " »."));
        return nullptr;
    }

    auto resultat = this->trouve_ou_cree_fichier(
        module, "module", chemin_fichier_module, importe_kuri);
    if (resultat.est<FichierNeuf>()) {
        gestionnaire_code->requiers_chargement(espace, resultat.resultat<FichierNeuf>().fichier);
    }
#endif

    if (module->nom() == ID::Kuri) {
        auto resultat = this->trouve_ou_cree_fichier(
            module, "constantes", "constantes.kuri", false);

        if (resultat.est<FichierNeuf>()) {
            auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier;
            if (!donnees_fichier->fut_charge) {
                const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
                donnees_fichier->charge_tampon(lng::tampon_source(source));
            }

            gestionnaire_code->requiers_lexage(espace, resultat.resultat<FichierNeuf>().fichier);
        }
    }

    messagere->ajoute_message_module_ferme(espace, module);

    return module;
}

/* ************************************************************************** */

static std::optional<kuri::chemin_systeme> determine_chemin_absolu(EspaceDeTravail *espace,
                                                                   kuri::chaine_statique chemin,
                                                                   NoeudExpression const *site)
{
    if (!kuri::chemin_systeme::existe(chemin)) {
        espace->rapporte_erreur(site, "Impossible de trouver le fichier")
            .ajoute_message("Le chemin testé fut : ", chemin);
        return {};
    }

    if (!kuri::chemin_systeme::est_fichier_regulier(chemin)) {
        espace->rapporte_erreur(site, "Le chemin ne pointe pas vers un fichier régulier")
            .ajoute_message("Le chemin testé fut : ", chemin);
        return {};
    }

    return kuri::chemin_systeme::absolu(chemin);
}

void Compilatrice::ajoute_fichier_a_la_compilation(EspaceDeTravail *espace,
                                                   kuri::chaine_statique nom,
                                                   Module *module,
                                                   NoeudExpression const *site)
{
    auto chemin = dls::chaine(kuri::chaine(module->chemin())) + dls::chaine(kuri::chaine(nom));

    if (chemin.trouve(".kuri") == dls::chaine::npos) {
        chemin += ".kuri";
    }

    auto opt_chemin = determine_chemin_absolu(espace, chemin.c_str(), site);
    if (!opt_chemin.has_value()) {
        return;
    }

    auto resultat = this->trouve_ou_cree_fichier(module, nom, opt_chemin.value(), importe_kuri);

    if (resultat.est<FichierNeuf>()) {
        gestionnaire_code->requiers_chargement(espace, resultat.resultat<FichierNeuf>().fichier);
    }
}

/* ************************************************************************** */

int64_t Compilatrice::memoire_utilisee() const
{
    auto memoire = taille_de(Compilatrice);

    memoire += ordonnanceuse->memoire_utilisee();
    memoire += table_identifiants->memoire_utilisee();

    memoire += gerante_chaine->memoire_utilisee();

    POUR ((*espaces_de_travail.verrou_lecture())) {
        memoire += it->memoire_utilisee();
    }

    memoire += messagere->memoire_utilisee();

    memoire += sys_module->memoire_utilisee();

    auto metaprogrammes_ = metaprogrammes.verrou_lecture();
    POUR_TABLEAU_PAGE ((*metaprogrammes_)) {
        memoire += it.programme->memoire_utilisee();
    }

    return memoire;
}

void Compilatrice::rassemble_statistiques(Statistiques &stats) const
{
    stats.memoire_compilatrice = memoire_utilisee();

    POUR ((*espaces_de_travail.verrou_lecture())) {
        it->rassemble_statistiques(stats);
    }

    stats.nombre_identifiants = table_identifiants->taille();

    sys_module->rassemble_stats(stats);

    operateurs->rassemble_statistiques(stats);
    graphe_dependance->rassemble_statistiques(stats);
    gestionnaire_bibliotheques->rassemble_statistiques(stats);
    typeuse.rassemble_statistiques(stats);

    auto metaprogrammes_ = metaprogrammes.verrou_lecture();
    POUR_TABLEAU_PAGE ((*metaprogrammes_)) {
        it.programme->rassemble_statistiques(stats);
    }

    auto &stats_ri = stats.stats_ri;

    auto memoire_fonctions = fonctions.memoire_utilisee();
    memoire_fonctions += fonctions.memoire_utilisee();
    pour_chaque_element(fonctions, [&](AtomeFonction const &it) {
        memoire_fonctions += it.params_entrees.taille_memoire();
        memoire_fonctions += it.instructions.taille_memoire();

        if (it.données_exécution) {
            memoire_fonctions += it.données_exécution->mémoire_utilisée();
            memoire_fonctions += taille_de(DonnéesExécutionFonction);
        }
    });

    données_constantes_exécutions.rassemble_statistiques(stats);

    stats_ri.fusionne_entree({"fonctions", fonctions.taille(), memoire_fonctions});
    stats_ri.fusionne_entree({"globales", globales.taille(), globales.memoire_utilisee()});
}

void Compilatrice::rapporte_erreur(EspaceDeTravail const *espace,
                                   kuri::chaine_statique message,
                                   erreur::Genre genre)
{
    if (espace) {
        // Toutes les erreurs ne transitent pas forcément par EspaceDeTravail
        // (comme les erreurs de syntaxage ou de lexage).
        espace->possede_erreur = true;
    }

    if (!espace || !espace->options.continue_si_erreur) {
        m_possede_erreur = true;
        m_code_erreur = genre;
    }

    std::cerr << message << '\n';
}

bool Compilatrice::possede_erreur(const EspaceDeTravail *espace) const
{
    return espace->possede_erreur;
}

/* ************************************************************************** */

EspaceDeTravail *Compilatrice::demarre_un_espace_de_travail(OptionsDeCompilation const &options,
                                                            const kuri::chaine &nom)
{
    auto espace = memoire::loge<EspaceDeTravail>("EspaceDeTravail", *this, options, nom);
    espaces_de_travail->ajoute(espace);
    gestionnaire_code->espace_cree(espace);
    return espace;
}

ContexteLexage Compilatrice::contexte_lexage(EspaceDeTravail *espace)
{
    auto rappel_erreur = [this, espace](SiteSource site, kuri::chaine message) {
        if (espace) {
            espace->rapporte_erreur(site, message, erreur::Genre::LEXAGE);
        }
        else {
            this->rapporte_erreur(espace, message, erreur::Genre::LEXAGE);
        }
    };

    return {gerante_chaine, table_identifiants, rappel_erreur};
}

// -----------------------------------------------------------------------------
// Implémentation des fonctions d'interface afin d'éviter les erreurs, toutes les
// fonctions ne sont pas implémentées dans la Compilatrice, d'autres appelent
// directement les fonctions se trouvant sur EspaceDeTravail, ou enlignent la
// logique dans la MachineVirtuelle.

OptionsDeCompilation *Compilatrice::options_compilation()
{
    return &espace_de_travail_defaut->options;
}

void Compilatrice::ajourne_options_compilation(OptionsDeCompilation *options)
{
    /* À FAIRE : il faut ajourner la coulisse selon l'espace, et peut-être arrêter la compilation
     * du code. */
    espace_de_travail_defaut->options = *options;
    gestionnaire_code->ajourne_espace_pour_nouvelles_options(espace_de_travail_defaut);
}

void Compilatrice::ajoute_chaine_compilation(EspaceDeTravail *espace,
                                             NoeudExpression const *site,
                                             kuri::chaine_statique c)
{
    ajoute_chaine_au_module(espace, site, module_racine_compilation, c);
}

void Compilatrice::ajoute_chaine_au_module(EspaceDeTravail *espace,
                                           NoeudExpression const *site,
                                           Module *module,
                                           kuri::chaine_statique c)
{
    auto chaine = dls::chaine(c.pointeur(), c.taille());

    auto decalage = chaines_ajoutees_a_la_compilation->ajoute(
        kuri::chaine(c.pointeur(), c.taille()));

    /* Les fichiers sont comparés selon leurs chemins, donc il nous faut un chemin unique pour
     * chaque nouvelle chaine. */
    auto nom_fichier = enchaine("chaine_ajoutée",
                                chaines_ajoutees_a_la_compilation->nombre_de_chaines());
    auto chemin_fichier = enchaine(".", nom_fichier);
    auto resultat = this->trouve_ou_cree_fichier(
        module, nom_fichier, chemin_fichier, importe_kuri);

    assert(resultat.est<FichierNeuf>());

    auto fichier = resultat.resultat<FichierNeuf>().fichier;
    fichier->source = SourceFichier::CHAINE_AJOUTEE;
    fichier->decalage_fichier = decalage;
    fichier->site = site;
    fichier->charge_tampon(lng::tampon_source(std::move(chaine)));
    gestionnaire_code->requiers_lexage(espace, fichier);
}

void Compilatrice::ajoute_fichier_compilation(EspaceDeTravail *espace,
                                              kuri::chaine_statique c,
                                              const NoeudExpression *site)
{
    ajoute_fichier_a_la_compilation(espace, c, module_racine_compilation, site);
}

Message const *Compilatrice::attend_message()
{
    auto messagere_ = messagere.verrou_ecriture();
    if (!messagere_->possede_message()) {
        return nullptr;
    }
    return messagere_->defile();
}

EspaceDeTravail *Compilatrice::espace_defaut_compilation()
{
    return espace_de_travail_defaut;
}

static kuri::tableau<kuri::Lexeme> converti_tableau_lexemes(
    kuri::tableau<Lexeme, int> const &lexemes)
{
    auto resultat = kuri::tableau<kuri::Lexeme>(lexemes.taille());
    POUR (lexemes) {
        resultat.ajoute({static_cast<int>(it.genre), it.chaine});
    }
    return resultat;
}

kuri::tableau_statique<kuri::Lexeme> Compilatrice::lexe_fichier(EspaceDeTravail *espace,
                                                                kuri::chaine_statique chemin_donne,
                                                                NoeudExpression const *site)
{
    auto opt_chemin = determine_chemin_absolu(espace, chemin_donne, site);
    if (!opt_chemin.has_value()) {
        return {nullptr, 0};
    }

    auto chemin_absolu = opt_chemin.value();

    auto module = this->module(ID::chaine_vide);

    auto resultat = this->trouve_ou_cree_fichier(
        module, chemin_absolu.nom_fichier_sans_extension(), chemin_absolu, importe_kuri);

    if (resultat.est<FichierExistant>()) {
        auto donnees_fichier = resultat.resultat<FichierExistant>().fichier;
        auto tableau = converti_tableau_lexemes(donnees_fichier->lexemes);
        m_tableaux_lexemes.ajoute(tableau);
        return m_tableaux_lexemes.derniere();
    }

    auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier;
    auto tampon = charge_contenu_fichier({chemin_absolu.pointeur(), chemin_absolu.taille()});
    donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

    auto lexeuse = Lexeuse(
        contexte_lexage(espace), donnees_fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
    lexeuse.performe_lexage();

    auto tableau = converti_tableau_lexemes(donnees_fichier->lexemes);
    m_tableaux_lexemes.ajoute(tableau);
    return m_tableaux_lexemes.derniere();
}

kuri::tableau_statique<NoeudCodeEnteteFonction *> Compilatrice::fonctions_parsees(
    EspaceDeTravail *espace)
{
    auto entetes = gestionnaire_code->fonctions_parsees();
    auto resultat = kuri::tableau<NoeudCodeEnteteFonction *>();
    resultat.reserve(entetes.taille());
    POUR (entetes) {
        if (it->est_operateur || it->est_coroutine ||
            it->possede_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
            continue;
        }
        auto code_entete = convertisseuse_noeud_code.convertis_noeud_syntaxique(espace, it);
        resultat.ajoute(code_entete->comme_entete_fonction());
    }
    m_tableaux_code_fonctions.ajoute(resultat);
    return m_tableaux_code_fonctions.derniere();
}

Module *Compilatrice::trouve_ou_cree_module(IdentifiantCode *nom_module,
                                            kuri::chaine_statique chemin)
{
    auto module = sys_module->trouve_ou_cree_module(nom_module, chemin);

    /* Initialise les chemins des bibliothèques internes au module. */
    if (module->chemin_bibliotheque_32bits.taille() == 0) {
        module->chemin_bibliotheque_32bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliotheque(
                                                 ArchitectureCible::X86);
        module->chemin_bibliotheque_64bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliotheque(
                                                 ArchitectureCible::X64);
    }

    return module;
}

Module *Compilatrice::module(const IdentifiantCode *nom_module) const
{
    return sys_module->module(nom_module);
}

ResultatFichier Compilatrice::trouve_ou_cree_fichier(Module *module,
                                                     kuri::chaine_statique nom_fichier,
                                                     kuri::chaine_statique chemin,
                                                     bool importe_kuri_)
{
    auto resultat_fichier = sys_module->trouve_ou_cree_fichier(module, nom_fichier, chemin);

    if (resultat_fichier.est<FichierNeuf>()) {
        auto fichier_neuf = resultat_fichier.resultat<FichierNeuf>().fichier;
        if (importe_kuri_ && module->nom() != ID::Kuri) {
            assert(module_kuri);
            fichier_neuf->modules_importes.insere(module_kuri);
        }
    }

    return resultat_fichier;
}

MetaProgramme *Compilatrice::metaprogramme_pour_fonction(
    NoeudDeclarationEnteteFonction const *entete)
{
    POUR_TABLEAU_PAGE ((*metaprogrammes.verrou_ecriture())) {
        if (it.fonction == entete) {
            return &it;
        }
    }

    return nullptr;
}

Fichier *Compilatrice::cree_fichier_pour_metaprogramme(MetaProgramme *metaprogramme_)
{
    auto fichier_racine = this->fichier(metaprogramme_->corps_texte->lexeme->fichier);
    auto module = fichier_racine->module;
    auto nom_fichier = enchaine(metaprogramme_);
    auto resultat_fichier = this->trouve_ou_cree_fichier(module, nom_fichier, nom_fichier, false);
    assert(resultat_fichier.est<FichierNeuf>());
    auto resultat = resultat_fichier.resultat<FichierNeuf>().fichier;
    resultat->metaprogramme_corps_texte = metaprogramme_;
    resultat->source = SourceFichier::CHAINE_AJOUTEE;
    metaprogramme_->fichier = resultat;
    /* Hérite des modules importés par le fichier où se trouve le métaprogramme afin de pouvoir
     * également accéder aux symboles de ces modules. */
    resultat->modules_importes = fichier_racine->modules_importes;
    return resultat;
}

const Fichier *Compilatrice::fichier(int64_t index) const
{
    return sys_module->fichier(index);
}

Fichier *Compilatrice::fichier(int64_t index)
{
    return sys_module->fichier(index);
}

Fichier *Compilatrice::fichier(kuri::chaine_statique chemin) const
{
    return sys_module->fichier(chemin);
}

AtomeFonction *Compilatrice::cree_fonction(const Lexeme *lexeme, const kuri::chaine &nom_fichier)
{
    std::unique_lock lock(mutex_atomes_fonctions);
    /* Le broyage est en soi inutile mais nous permet d'avoir une chaine_statique. */;
    auto atome_fonc = fonctions.ajoute_element(lexeme, broyeuse->broye_nom_simple(nom_fichier));
    return atome_fonc;
}

/* Il existe des dépendances cycliques entre les fonctions qui nous empêche de
 * générer le code linéairement. Cette fonction nous sers soit à trouver le
 * pointeur vers l'atome d'une fonction si nous l'avons déjà généré, soit de le
 * créer en préparation de la génération de la RI de son corps.
 */
AtomeFonction *Compilatrice::trouve_ou_insere_fonction(ConstructriceRI &constructrice,
                                                       NoeudDeclarationEnteteFonction *decl)
{
    std::unique_lock lock(mutex_atomes_fonctions);

    if (decl->atome) {
        return static_cast<AtomeFonction *>(decl->atome);
    }

    SAUVEGARDE_ETAT(constructrice.fonction_courante);

    auto params = kuri::tableau<Atome *, int>();
    params.reserve(decl->params.taille());

    for (auto i = 0; i < decl->params.taille(); ++i) {
        auto param = decl->parametre_entree(i);
        auto atome = constructrice.cree_allocation(param, param->type, param->ident);
        param->atome = atome;
        params.ajoute(atome);
    }

    /* Pour les sorties multiples, les valeurs de sorties sont des accès de
     * membres du tuple, ainsi nous n'avons pas à compliquer la génération de
     * code ou sa simplification.
     */

    auto param_sortie = decl->param_sortie;
    auto atome_param_sortie = constructrice.cree_allocation(
        param_sortie, param_sortie->type, param_sortie->ident);
    param_sortie->atome = atome_param_sortie;

    if (decl->params_sorties.taille() > 1) {
        POUR_INDEX (decl->params_sorties) {
            it->comme_declaration_variable()->atome = constructrice.cree_reference_membre(
                it, atome_param_sortie, index_it, true);
        }
    }

    auto atome_fonc = fonctions.ajoute_element(
        decl->lexeme, decl->nom_broye(constructrice.espace(), *broyeuse), std::move(params));
    atome_fonc->type = decl->type;
    atome_fonc->est_externe = decl->possede_drapeau(DrapeauxNoeudFonction::EST_EXTERNE);
    atome_fonc->sanstrace = decl->possede_drapeau(DrapeauxNoeudFonction::FORCE_SANSTRACE);
    atome_fonc->decl = decl;
    atome_fonc->param_sortie = atome_param_sortie;
    atome_fonc->enligne = decl->possede_drapeau(DrapeauxNoeudFonction::FORCE_ENLIGNE);

    decl->atome = atome_fonc;

    return atome_fonc;
}

AtomeGlobale *Compilatrice::cree_globale(Type const *type,
                                         AtomeConstante *initialisateur,
                                         bool est_externe,
                                         bool est_constante)
{
    return globales.ajoute_element(typeuse.type_pointeur_pour(const_cast<Type *>(type), false),
                                   initialisateur,
                                   est_externe,
                                   est_constante);
}

AtomeGlobale *Compilatrice::trouve_globale(NoeudDeclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);
    auto decl_var = decl->comme_declaration_variable();
    return static_cast<AtomeGlobale *>(decl_var->atome);
}

AtomeGlobale *Compilatrice::trouve_ou_insere_globale(NoeudDeclaration *decl)
{
    std::unique_lock lock(mutex_atomes_globales);

    auto decl_var = decl->comme_declaration_variable();

    if (decl_var->atome == nullptr) {
        decl_var->atome = cree_globale(decl->type, nullptr, false, false);
    }

    return static_cast<AtomeGlobale *>(decl_var->atome);
}

MetaProgramme *Compilatrice::cree_metaprogramme(EspaceDeTravail *espace)
{
    auto resultat = metaprogrammes->ajoute_element();
    resultat->programme = Programme::cree_pour_metaprogramme(espace, resultat);
    return resultat;
}

/* ************************************************************************** */

// fonction pour tester les appels de fonctions variadiques externe dans la machine virtuelle
int fonction_test_variadique_externe(int sentinel, ...)
{
    va_list ap;
    va_start(ap, sentinel);

    int i = 0;
    for (;; ++i) {
        int t = va_arg(ap, int);

        if (t == sentinel) {
            break;
        }
    }

    va_end(ap);

    return i;
}
