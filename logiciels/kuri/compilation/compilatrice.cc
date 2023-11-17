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

int64_t GestionnaireChainesAjoutees::mémoire_utilisée() const
{
    int64_t résultat = m_chaines.taille_memoire();

    POUR (m_chaines) {
        résultat += it.taille();
    }

    return résultat;
}

/* ************************************************************************** */

Compilatrice::Compilatrice(kuri::chaine chemin_racine_kuri, ArgumentsCompilatrice arguments_)
    : ordonnanceuse(this), messagere(this), gestionnaire_code(this),
      gestionnaire_bibliotheques(GestionnaireBibliotheques(*this)), arguments(arguments_),
      racine_kuri(chemin_racine_kuri), typeuse(graphe_dependance, this->operateurs),
      registre_ri(memoire::loge<RegistreSymboliqueRI>("RegistreSymboliqueRI", typeuse))
{
    initialise_identifiants_intrinsèques(*table_identifiants.verrou_ecriture());
    initialise_identifiants_ipa(*table_identifiants.verrou_ecriture());

    auto ops = operateurs.verrou_ecriture();
    enregistre_opérateurs_basiques(typeuse, *ops);

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

    POUR (m_états_libres) {
        memoire::deloge("EtatResolutionAppel", it);
    }

    memoire::deloge("Broyeuse", broyeuse);
    memoire::deloge("RegistreSymboliqueRI", registre_ri);
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace,
                                     kuri::chaine_statique nom,
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
    auto module = this->trouve_ou_crée_module(
        table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier), chemin_absolu);

    if (module->importé) {
        return module;
    }

    module->importé = true;

    messagere->ajoute_message_module_ouvert(espace, module);

#if 1
    auto fichiers = kuri::chemin_systeme::fichiers_du_dossier(chemin_absolu);

    POUR (fichiers) {
        auto resultat = this->trouve_ou_crée_fichier(
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

    auto resultat = this->trouve_ou_crée_fichier(
        module, "module", chemin_fichier_module, importe_kuri);
    if (resultat.est<FichierNeuf>()) {
        gestionnaire_code->requiers_chargement(espace, resultat.resultat<FichierNeuf>().fichier);
    }
#endif

    if (module->nom() == ID::Kuri) {
        auto resultat = this->trouve_ou_crée_fichier(
            module, "constantes", "constantes.kuri", false);

        if (resultat.est<FichierNeuf>()) {
            auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier;
            if (!donnees_fichier->fut_chargé) {
                const char *source = "SYS_EXP :: SystèmeExploitation.LINUX\n";
                donnees_fichier->charge_tampon(lng::tampon_source(source));
            }

            gestionnaire_code->requiers_lexage(espace, resultat.resultat<FichierNeuf>().fichier);
        }
    }

    messagere->ajoute_message_module_fermé(espace, module);

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

    auto resultat = this->trouve_ou_crée_fichier(module, nom, opt_chemin.value(), importe_kuri);

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

    memoire += messagere->mémoire_utilisée();

    memoire += sys_module->mémoire_utilisée();

    auto metaprogrammes_ = metaprogrammes.verrou_lecture();
    POUR_TABLEAU_PAGE ((*metaprogrammes_)) {
        memoire += it.programme->memoire_utilisee();
    }

    memoire += chaines_ajoutées_à_la_compilation->mémoire_utilisée();

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

    données_constantes_exécutions.rassemble_statistiques(stats);
    registre_ri->rassemble_statistiques(stats);
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

    std::cerr << message << '\n';
}

bool Compilatrice::possède_erreur(const EspaceDeTravail *espace) const
{
    return espace->possède_erreur;
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

    auto decalage = chaines_ajoutées_à_la_compilation->ajoute(
        kuri::chaine(c.pointeur(), c.taille()));

    /* Les fichiers sont comparés selon leurs chemins, donc il nous faut un chemin unique pour
     * chaque nouvelle chaine. */
    auto nom_fichier = enchaine("chaine_ajoutée",
                                chaines_ajoutées_à_la_compilation->nombre_de_chaines());
    auto chemin_fichier = enchaine(".", nom_fichier);
    auto resultat = this->trouve_ou_crée_fichier(
        module, nom_fichier, chemin_fichier, importe_kuri);

    assert(resultat.est<FichierNeuf>());

    auto fichier = resultat.resultat<FichierNeuf>().fichier;
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
    auto messagere_ = messagere.verrou_ecriture();
    if (!messagere_->possède_message()) {
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
    auto index_résultat = 0;
    POUR (lexemes) {
        resultat[index_résultat++] = {static_cast<int>(it.genre), it.chaine};
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

    auto resultat = this->trouve_ou_crée_fichier(
        module, chemin_absolu.nom_fichier_sans_extension(), chemin_absolu, importe_kuri);

    if (resultat.est<FichierExistant>()) {
        auto donnees_fichier = resultat.resultat<FichierExistant>().fichier;
        auto tableau = converti_tableau_lexemes(donnees_fichier->lexèmes);
        m_tableaux_lexemes.ajoute(tableau);
        return m_tableaux_lexemes.dernière();
    }

    auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier;
    auto tampon = charge_contenu_fichier({chemin_absolu.pointeur(), chemin_absolu.taille()});
    donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

    auto lexeuse = Lexeuse(
        contexte_lexage(espace), donnees_fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
    lexeuse.performe_lexage();

    auto tableau = converti_tableau_lexemes(donnees_fichier->lexèmes);
    m_tableaux_lexemes.ajoute(tableau);
    return m_tableaux_lexemes.dernière();
}

kuri::tableau_statique<NoeudCodeEnteteFonction *> Compilatrice::fonctions_parsees(
    EspaceDeTravail *espace)
{
    auto entetes = gestionnaire_code->fonctions_parsees();
    auto resultat = kuri::tableau<NoeudCodeEnteteFonction *>();
    resultat.reserve(entetes.taille());
    POUR (entetes) {
        if (it->est_operateur || it->est_coroutine ||
            it->possède_drapeau(DrapeauxNoeudFonction::EST_POLYMORPHIQUE)) {
            continue;
        }
        auto code_entete = convertisseuse_noeud_code.convertis_noeud_syntaxique(espace, it);
        resultat.ajoute(code_entete->comme_entete_fonction());
    }
    m_tableaux_code_fonctions.ajoute(resultat);
    return m_tableaux_code_fonctions.dernière();
}

Module *Compilatrice::trouve_ou_crée_module(IdentifiantCode *nom_module,
                                            kuri::chaine_statique chemin)
{
    auto module = sys_module->trouve_ou_crée_module(nom_module, chemin);

    /* Initialise les chemins des bibliothèques internes au module. */
    if (module->chemin_bibliothèque_32bits.taille() == 0) {
        module->chemin_bibliothèque_32bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliotheque(
                                                 ArchitectureCible::X86);
        module->chemin_bibliothèque_64bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliotheque(
                                                 ArchitectureCible::X64);
    }

    return module;
}

Module *Compilatrice::module(const IdentifiantCode *nom_module) const
{
    return sys_module->module(nom_module);
}

ResultatFichier Compilatrice::trouve_ou_crée_fichier(Module *module,
                                                     kuri::chaine_statique nom_fichier,
                                                     kuri::chaine_statique chemin,
                                                     bool importe_kuri_)
{
    auto resultat_fichier = sys_module->trouve_ou_crée_fichier(module, nom_fichier, chemin);

    if (resultat_fichier.est<FichierNeuf>()) {
        auto fichier_neuf = resultat_fichier.resultat<FichierNeuf>().fichier;
        if (importe_kuri_ && module->nom() != ID::Kuri) {
            assert(module_kuri);
            fichier_neuf->modules_importés.insere(module_kuri);
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

Fichier *Compilatrice::crée_fichier_pour_metaprogramme(MetaProgramme *metaprogramme_)
{
    auto fichier_racine = this->fichier(metaprogramme_->corps_texte->lexeme->fichier);
    auto module = fichier_racine->module;
    auto nom_fichier = enchaine(metaprogramme_);
    auto resultat_fichier = this->trouve_ou_crée_fichier(module, nom_fichier, nom_fichier, false);
    assert(resultat_fichier.est<FichierNeuf>());
    auto resultat = resultat_fichier.resultat<FichierNeuf>().fichier;
    resultat->métaprogramme_corps_texte = metaprogramme_;
    resultat->source = SourceFichier::CHAINE_AJOUTÉE;
    metaprogramme_->fichier = resultat;
    /* Hérite des modules importés par le fichier où se trouve le métaprogramme afin de pouvoir
     * également accéder aux symboles de ces modules. */
    resultat->modules_importés = fichier_racine->modules_importés;
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

MetaProgramme *Compilatrice::crée_metaprogramme(EspaceDeTravail *espace)
{
    auto resultat = metaprogrammes->ajoute_element();
    resultat->programme = Programme::crée_pour_metaprogramme(espace, resultat);
    return resultat;
}

/* ************************************************************************** */

EtatResolutionAppel *Compilatrice::crée_ou_donne_état_résolution_appel()
{
    if (!m_états_libres.est_vide()) {
        auto résultat = m_états_libres.dernière();
        résultat->réinitialise();
        m_états_libres.supprime_dernier();
        return résultat;
    }

    return memoire::loge<EtatResolutionAppel>("EtatResolutionAppel");
}

void Compilatrice::libère_état_résolution_appel(EtatResolutionAppel *&état)
{
    m_états_libres.ajoute(état);
    état = nullptr;
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
