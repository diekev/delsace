/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "compilatrice.hh"

#include <iostream>
#include <stdarg.h>

#include "arbre_syntaxique/noeud_code.hh"

#include "statistiques/statistiques.hh"

#include "parsage/lexeuse.hh"

#include "structures/chemin_systeme.hh"

#include "broyage.hh"
#include "environnement.hh"
#include "erreur.h"
#include "espace_de_travail.hh"
#include "gestionnaire_code.hh"
#include "intrinseques.hh"
#include "ipa.hh"
#include "portee.hh"
#include "programme.hh"
#include "validation_expression_appel.hh"
#include "validation_semantique.hh"

#include "utilitaires/log.hh"

/* ************************************************************************** */

int64_t GestionnaireChainesAjoutées::ajoute(kuri::chaine chaine)
{
    int64_t décalage = nombre_total_de_lignes;

    POUR (chaine) {
        nombre_total_de_lignes += (it == '\n');
    }

    /* Nous ajoutons une ligne car toutes les chaines sont suffixées d'une ligne. */
    nombre_total_de_lignes += 1;
    m_chaines.ajoute(chaine);
    return décalage;
}

int GestionnaireChainesAjoutées::nombre_de_chaines() const
{
    return m_chaines.taille();
}

void GestionnaireChainesAjoutées::imprime_dans(std::ostream &os)
{
    auto d = hui_systeme();

    os << "Fichier créé le " << d.jour << "/" << d.mois << "/" << d.annee << " à " << d.heure
       << ':' << d.minute << ':' << d.seconde << "\n\n";

    POUR (m_chaines) {
        os << it;
        os << "\n";
    }
}

int64_t GestionnaireChainesAjoutées::mémoire_utilisée() const
{
    int64_t résultat = m_chaines.taille_mémoire();

    POUR (m_chaines) {
        résultat += it.taille();
    }

    return résultat;
}

/* ************************************************************************** */

Compilatrice::Compilatrice(kuri::chaine chemin_racine_kuri, ArgumentsCompilatrice arguments_)
    : ordonnanceuse(this), messagère(this),
      gestionnaire_code(memoire::loge<GestionnaireCode>("GestionnaireCode", this)),
      gestionnaire_bibliothèques(GestionnaireBibliothèques(*this)), arguments(arguments_),
      racine_kuri(chemin_racine_kuri), typeuse(this->opérateurs),
      registre_ri(memoire::loge<RegistreSymboliqueRI>("RegistreSymboliqueRI", typeuse))
{
    racine_modules_kuri = racine_kuri / "modules";

    initialise_identifiants_intrinsèques(*table_identifiants.verrou_ecriture());
    initialise_identifiants_ipa(*table_identifiants.verrou_ecriture());

    auto ops = opérateurs.verrou_ecriture();
    enregistre_opérateurs_basiques(typeuse, *ops);

    auto options_espace_défaut = OptionsDeCompilation{};
    options_espace_défaut.utilise_trace_appel = !arguments.sans_traces_d_appel;
    options_espace_défaut.coulisse = arguments.coulisse;

    espace_de_travail_defaut = demarre_un_espace_de_travail(options_espace_défaut, "Espace 1");

    /* Charge le module Kuri. */
    if (arguments.importe_kuri) {
        module_kuri = importe_module(espace_de_travail_defaut, "Kuri", nullptr);
    }

    broyeuse = memoire::loge<Broyeuse>("Broyeuse");

    m_date_début_compilation = hui_systeme();
}

Compilatrice::~Compilatrice()
{
    POUR ((*espaces_de_travail.verrou_ecriture())) {
        memoire::deloge("EspaceDeTravail", it);
    }

    POUR (m_états_libres) {
        memoire::deloge("ÉtatRésolutionAppel", it);
    }

    POUR (m_sémanticiennes) {
        memoire::deloge("Sémanticienne", it);
    }

    POUR (m_convertisseuses_noeud_code) {
        memoire::deloge("ConvertisseuseNoeudCode", it);
    }

    memoire::deloge("Broyeuse", broyeuse);
    memoire::deloge("RegistreSymboliqueRI", registre_ri);
    memoire::deloge("GestionnaireCode", gestionnaire_code);
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace,
                                     kuri::chaine_statique nom,
                                     NoeudExpression const *site)
{
    auto chemin = kuri::chemin_systeme(nom);

    if (!kuri::chemin_systeme::existe(chemin)) {
        /* essaie dans la racine kuri */
        chemin = racine_modules_kuri / chemin;

        if (!kuri::chemin_systeme::existe(chemin)) {
            espace
                ->rapporte_erreur(site, "Impossible de trouver le dossier correspondant au module")
                .ajoute_message("Le chemin testé fut : ", chemin);

            return nullptr;
        }
    }

    if (!kuri::chemin_systeme::est_dossier(chemin)) {
        espace->rapporte_erreur(
            site, "Le nom du module ne pointe pas vers un dossier", erreur::Genre::MODULE_INCONNU);
        return nullptr;
    }

    /* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
    auto chemin_absolu = kuri::chemin_systeme::canonique_absolu(chemin);
    auto nom_dossier = chemin_absolu.nom_fichier();

    // @concurrence critique
    auto module = this->trouve_ou_crée_module(
        table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier), chemin_absolu);

    if (module->importé) {
        return module;
    }

    module->importé = true;

    messagère->ajoute_message_module_ouvert(espace, module);

#if 1
    auto fichiers = kuri::chemin_systeme::fichiers_du_dossier(chemin_absolu);

    POUR (fichiers) {
        auto résultat = this->trouve_ou_crée_fichier(
            module, it.nom_fichier_sans_extension(), it, importe_kuri);

        if (std::holds_alternative<FichierNeuf>(résultat)) {
            auto fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
            gestionnaire_code->requiers_chargement(espace, fichier);
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

    auto résultat = this->trouve_ou_crée_fichier(
        module, "module", chemin_fichier_module, importe_kuri);
    if (résultat.est<FichierNeuf>()) {
        gestionnaire_code->requiers_chargement(espace, résultat.résultat<FichierNeuf>().fichier);
    }
#endif

    if (module->nom() == ID::Kuri) {
        auto résultat = this->trouve_ou_crée_fichier(
            module, "constantes", "constantes.kuri", false);

        if (std::holds_alternative<FichierNeuf>(résultat)) {
            auto donnees_fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
            if (!donnees_fichier->fut_chargé) {
                const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
                donnees_fichier->charge_tampon(lng::tampon_source(source));
            }

            gestionnaire_code->requiers_lexage(espace, donnees_fichier);
        }
    }

    messagère->ajoute_message_module_fermé(espace, module);

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

    auto résultat = this->trouve_ou_crée_fichier(module, nom, opt_chemin.value(), importe_kuri);

    if (std::holds_alternative<FichierNeuf>(résultat)) {
        auto fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
        gestionnaire_code->requiers_chargement(espace, fichier);
    }
}

/* ************************************************************************** */

int64_t Compilatrice::memoire_utilisee() const
{
    auto résultat = taille_de(Compilatrice);

    résultat += ordonnanceuse->mémoire_utilisée();
    résultat += table_identifiants->memoire_utilisee();

    résultat += gérante_chaine->memoire_utilisee();

    POUR ((*espaces_de_travail.verrou_lecture())) {
        résultat += it->memoire_utilisee();
    }

    résultat += messagère->mémoire_utilisée();

    résultat += sys_module->mémoire_utilisée();

    auto metaprogrammes_ = métaprogrammes.verrou_lecture();
    POUR_TABLEAU_PAGE ((*metaprogrammes_)) {
        résultat += it.programme->mémoire_utilisée();
        résultat += it.données_constantes.taille_mémoire();
        résultat += it.données_globales.taille_mémoire();
        résultat += it.cibles_appels.taille_mémoire();
    }

    résultat += metaprogrammes_->mémoire_utilisée();

    résultat += chaines_ajoutées_à_la_compilation->mémoire_utilisée();

    résultat += m_tableaux_lexèmes.taille_mémoire();
    POUR (m_tableaux_lexèmes) {
        résultat += it.taille_mémoire();
    }

    résultat += m_tableaux_code_fonctions.taille_mémoire();
    POUR (m_tableaux_code_fonctions) {
        résultat += it.taille_mémoire();
    }

    résultat += m_états_libres.taille_mémoire();
    POUR (m_états_libres) {
        résultat += taille_de(ÉtatRésolutionAppel);
        résultat += it->args.taille_mémoire();
    }

    résultat += broyeuse->mémoire_utilisée();
    résultat += constructeurs_globaux->taille_mémoire();
    résultat += registre_chaines_ri->mémoire_utilisée();

    résultat += m_sémanticiennes.taille_mémoire() +
                taille_de(Sémanticienne) * m_sémanticiennes.taille();

    résultat += m_convertisseuses_noeud_code.taille_mémoire() +
                taille_de(ConvertisseuseNoeudCode) * m_convertisseuses_noeud_code.taille();

    return résultat;
}

void Compilatrice::rassemble_statistiques(Statistiques &stats) const
{
    stats.ajoute_mémoire_utilisée("Compilatrice", memoire_utilisee());

    POUR ((*espaces_de_travail.verrou_lecture())) {
        it->rassemble_statistiques(stats);
    }

    stats.nombre_identifiants = table_identifiants->taille();

    gestionnaire_code->rassemble_statistiques(stats);

    sys_module->rassemble_stats(stats);

    opérateurs->rassemble_statistiques(stats);
    graphe_dépendance->rassemble_statistiques(stats);
    gestionnaire_bibliothèques->rassemble_statistiques(stats);
    typeuse.rassemble_statistiques(stats);

    auto metaprogrammes_ = métaprogrammes.verrou_lecture();
    POUR_TABLEAU_PAGE ((*metaprogrammes_)) {
        it.programme->rassemble_statistiques(stats);
    }

    données_constantes_exécutions.rassemble_statistiques(stats);
    registre_ri->rassemble_statistiques(stats);

    POUR (m_sémanticiennes) {
        stats.temps_typage -= it->donne_temps_chargement();
        it->rassemble_statistiques(stats);

#ifdef STATISTIQUES_DETAILLEES
        it->donne_stats_typage().imprime_stats();
#endif
    }

    POUR (m_convertisseuses_noeud_code) {
        stats.ajoute_mémoire_utilisée("Compilatrice", it->mémoire_utilisée());
    }
}

void Compilatrice::rapporte_erreur(EspaceDeTravail const *espace,
                                   kuri::chaine_statique message,
                                   erreur::Genre genre)
{
    if (espace) {
        // Toutes les erreurs ne transitent pas forcément par EspaceDeTravail
        // (comme les erreurs de syntaxage ou de lexage).
        espace->possède_erreur = true;
    }

    if (!espace || !espace->options.continue_si_erreur) {
        m_possède_erreur = true;
        m_code_erreur = genre;
    }

    dbg() << message;
}

bool Compilatrice::possède_erreur(const EspaceDeTravail *espace) const
{
    return espace->possède_erreur;
}

/* ************************************************************************** */

EspaceDeTravail *Compilatrice::demarre_un_espace_de_travail(OptionsDeCompilation const &options,
                                                            kuri::chaine_statique nom)
{
    auto espace = memoire::loge<EspaceDeTravail>("EspaceDeTravail", *this, options, nom);
    espaces_de_travail->ajoute(espace);
    gestionnaire_code->espace_créé(espace);
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

    return {gérante_chaine, table_identifiants, rappel_erreur};
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

    auto decalage = chaines_ajoutées_à_la_compilation->ajoute(
        kuri::chaine(c.pointeur(), c.taille()));

    /* Les fichiers sont comparés selon leurs chemins, donc il nous faut un chemin unique pour
     * chaque nouvelle chaine. */
    auto nom_fichier = enchaine("chaine_ajoutée",
                                chaines_ajoutées_à_la_compilation->nombre_de_chaines());
    auto chemin_fichier = enchaine(".", nom_fichier);
    auto résultat = this->trouve_ou_crée_fichier(
        module, nom_fichier, chemin_fichier, importe_kuri);

    assert(std::holds_alternative<FichierNeuf>(résultat));

    auto fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
    fichier->source = SourceFichier::CHAINE_AJOUTÉE;
    fichier->décalage_fichier = decalage;
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
    auto messagère_ = messagère.verrou_ecriture();
    if (!messagère_->possède_message()) {
        return nullptr;
    }
    return messagère_->defile();
}

EspaceDeTravail *Compilatrice::espace_defaut_compilation()
{
    return espace_de_travail_defaut;
}

static kuri::tableau<kuri::Lexème> converti_tableau_lexemes(
    kuri::tableau<Lexème, int> const &lexemes)
{
    auto résultat = kuri::tableau<kuri::Lexème>(lexemes.taille());
    auto index_résultat = 0;
    POUR (lexemes) {
        résultat[index_résultat++] = {static_cast<int>(it.genre), it.chaine};
    }
    return résultat;
}

kuri::tableau_statique<kuri::Lexème> Compilatrice::lexe_fichier(EspaceDeTravail *espace,
                                                                kuri::chaine_statique chemin_donne,
                                                                NoeudExpression const *site)
{
    auto opt_chemin = determine_chemin_absolu(espace, chemin_donne, site);
    if (!opt_chemin.has_value()) {
        return {nullptr, 0};
    }

    auto chemin_absolu = opt_chemin.value();

    auto module = this->module(ID::chaine_vide);

    auto résultat = this->trouve_ou_crée_fichier(
        module, chemin_absolu.nom_fichier_sans_extension(), chemin_absolu, importe_kuri);

    if (std::holds_alternative<FichierExistant>(résultat)) {
        auto fichier = static_cast<Fichier *>(std::get<FichierExistant>(résultat));
        auto tableau = converti_tableau_lexemes(fichier->lexèmes);
        m_tableaux_lexèmes.ajoute(tableau);
        return m_tableaux_lexèmes.dernier_élément();
    }

    auto fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
    auto tampon = charge_contenu_fichier({chemin_absolu.pointeur(), chemin_absolu.taille()});
    fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

    auto lexeuse = Lexeuse(
        contexte_lexage(espace), fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
    lexeuse.performe_lexage();

    auto tableau = converti_tableau_lexemes(fichier->lexèmes);
    m_tableaux_lexèmes.ajoute(tableau);
    return m_tableaux_lexèmes.dernier_élément();
}

kuri::tableau_statique<NoeudCodeEntêteFonction *> Compilatrice::fonctions_parsees(
    EspaceDeTravail *espace)
{
    auto convertisseuse = donne_convertisseuse_noeud_code_disponible();
    auto entetes = gestionnaire_code->fonctions_parsées();
    auto résultat = kuri::tableau<NoeudCodeEntêteFonction *>();
    résultat.réserve(entetes.taille());
    POUR (entetes) {
        if (it->est_opérateur || it->est_coroutine ||
            it->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
            continue;
        }
        auto code_entete = convertisseuse->convertis_noeud_syntaxique(espace, it);
        résultat.ajoute(code_entete->comme_entête_fonction());
    }
    m_tableaux_code_fonctions.ajoute(résultat);
    dépose_convertisseuse(convertisseuse);
    return m_tableaux_code_fonctions.dernier_élément();
}

Module *Compilatrice::trouve_ou_crée_module(IdentifiantCode *nom_module,
                                            kuri::chaine_statique chemin)
{
    auto module = sys_module->trouve_ou_crée_module(nom_module, chemin);

    /* Initialise les chemins des bibliothèques internes au module. */
    if (module->chemin_bibliothèque_32bits.taille() == 0) {
        module->chemin_bibliothèque_32bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliothèque(
                                                 ArchitectureCible::X86);
        module->chemin_bibliothèque_64bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliothèque(
                                                 ArchitectureCible::X64);
    }

    return module;
}

Module *Compilatrice::module(const IdentifiantCode *nom_module) const
{
    return sys_module->module(nom_module);
}

RésultatFichier Compilatrice::trouve_ou_crée_fichier(Module *module,
                                                     kuri::chaine_statique nom_fichier,
                                                     kuri::chaine_statique chemin,
                                                     bool importe_kuri_)
{
    auto résultat_fichier = sys_module->trouve_ou_crée_fichier(module, nom_fichier, chemin);

    if (std::holds_alternative<FichierNeuf>(résultat_fichier)) {
        auto fichier_neuf = static_cast<Fichier *>(std::get<FichierNeuf>(résultat_fichier));
        if (importe_kuri_ && module->nom() != ID::Kuri) {
            assert(module_kuri);
            fichier_neuf->modules_importés.insère({module_kuri, true});
        }
    }

    return résultat_fichier;
}

MetaProgramme *Compilatrice::metaprogramme_pour_fonction(
    NoeudDéclarationEntêteFonction const *entete)
{
    POUR_TABLEAU_PAGE ((*métaprogrammes.verrou_ecriture())) {
        if (it.fonction == entete) {
            return &it;
        }
    }

    return nullptr;
}

Fichier *Compilatrice::crée_fichier_pour_metaprogramme(MetaProgramme *metaprogramme_)
{
    auto fichier_racine = this->fichier(metaprogramme_->corps_texte->lexème->fichier);
    auto module = fichier_racine->module;
    auto nom_fichier = enchaine(metaprogramme_);
    auto résultat_fichier = this->trouve_ou_crée_fichier(module, nom_fichier, nom_fichier, false);
    assert(std::holds_alternative<FichierNeuf>(résultat_fichier));
    auto résultat = static_cast<Fichier *>(std::get<FichierNeuf>(résultat_fichier));
    résultat->métaprogramme_corps_texte = metaprogramme_;
    résultat->source = SourceFichier::CHAINE_AJOUTÉE;
    metaprogramme_->fichier = résultat;
    /* Hérite des modules importés par le fichier où se trouve le métaprogramme afin de pouvoir
     * également accéder aux symboles de ces modules. */
    résultat->modules_importés = fichier_racine->modules_importés;
    return résultat;
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

MetaProgramme *Compilatrice::crée_metaprogramme(EspaceDeTravail *espace)
{
    auto résultat = métaprogrammes->ajoute_élément();
    résultat->programme = Programme::crée_pour_metaprogramme(espace, résultat);
    return résultat;
}

/* ************************************************************************** */

ÉtatRésolutionAppel *Compilatrice::crée_ou_donne_état_résolution_appel()
{
    if (!m_états_libres.est_vide()) {
        auto résultat = m_états_libres.dernier_élément();
        résultat->réinitialise();
        m_états_libres.supprime_dernier();
        return résultat;
    }

    return memoire::loge<ÉtatRésolutionAppel>("ÉtatRésolutionAppel");
}

void Compilatrice::libère_état_résolution_appel(ÉtatRésolutionAppel *&état)
{
    m_états_libres.ajoute(état);
    état = nullptr;
}

int Compilatrice::donne_nombre_occurences_chaine(kuri::chaine_statique chn)
{
    auto trouvé = false;
    auto n = m_nombre_occurences_chaines.trouve(chn, trouvé);
    if (trouvé) {
        m_nombre_occurences_chaines.trouve_ref(chn) += 1;
        return n;
    }

    m_nombre_occurences_chaines.insère(chn, 1);
    return 0;
}

IdentifiantCode *Compilatrice::donne_identifiant_pour_globale(kuri::chaine_statique nom_de_base)
{
    auto occurences = donne_nombre_occurences_chaine(nom_de_base);
    if (occurences == 0) {
        return table_identifiants->identifiant_pour_nouvelle_chaine(nom_de_base);
    }

    auto nom = enchaine(nom_de_base, '_', occurences);
    return table_identifiants->identifiant_pour_nouvelle_chaine(nom);
}

IdentifiantCode *Compilatrice::donne_nom_défaut_valeur_retour(int index)
{
    std::unique_lock verrouille(m_mutex_noms_valeurs_retours_défaut);

    if (index >= m_noms_valeurs_retours_défaut.taille()) {
        auto ident = table_identifiants->identifiant_pour_nouvelle_chaine(
            enchaine("__ret", index));
        m_noms_valeurs_retours_défaut.ajoute(ident);
    }

    return m_noms_valeurs_retours_défaut[index];
}

Sémanticienne *Compilatrice::donne_sémanticienne_disponible(Tacheronne &tacheronne)
{
    std::unique_lock l(m_mutex_sémanticiennes);

    Sémanticienne *résultat;

    if (!m_sémanticiennes.est_vide()) {
        résultat = m_sémanticiennes.dernier_élément();
        m_sémanticiennes.supprime_dernier();
    }
    else {
        résultat = memoire::loge<Sémanticienne>("Sémanticienne", *this);
    }

    résultat->réinitialise();
    résultat->définis_tacheronne(tacheronne);

    return résultat;
}

void Compilatrice::dépose_sémanticienne(Sémanticienne *sémanticienne)
{
    std::unique_lock l(m_mutex_sémanticiennes);
    m_sémanticiennes.ajoute(sémanticienne);
}

/* ************************************************************************** */

ConvertisseuseNoeudCode *Compilatrice::donne_convertisseuse_noeud_code_disponible()
{
    std::unique_lock l(m_mutex_convertisseuses_noeud_code);

    ConvertisseuseNoeudCode *résultat;

    if (!m_convertisseuses_noeud_code.est_vide()) {
        résultat = m_convertisseuses_noeud_code.dernier_élément();
        m_convertisseuses_noeud_code.supprime_dernier();
    }
    else {
        résultat = memoire::loge<ConvertisseuseNoeudCode>("ConvertisseuseNoeudCode");
    }

    return résultat;
}

void Compilatrice::dépose_convertisseuse(ConvertisseuseNoeudCode *convertisseuse)
{
    std::unique_lock l(m_mutex_convertisseuses_noeud_code);
    m_convertisseuses_noeud_code.ajoute(convertisseuse);
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
