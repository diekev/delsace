/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "compilatrice.hh"

#include <stdarg.h>

#include "biblinternes/flux/outils.h"
#include "biblinternes/nombre_decimaux/r16_c.h"
#include "biblinternes/outils/sauvegardeuse_etat.hh"

#include "statistiques/statistiques.hh"

#include "parsage/lexeuse.hh"

#include "structures/date.hh"

#include "erreur.h"
#include "espace_de_travail.hh"
#include "ipa.hh"
#include "portee.hh"
#include "programme.hh"

/* ************************************************************************** */

/* Redéfini certaines fonctions afin de pouvoir controler leurs comportements.
 * Par exemple, pour les fonctions d'allocations nous voudrions pouvoir libérer
 * la mémoire de notre coté, ou encore vérifier qu'il n'y a pas de fuite de
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

static float vers_r32(uint16_t f)
{
    return DLS_vers_r32(f);
}

static uint16_t depuis_r32(float f)
{
    return DLS_depuis_r32(f);
}

static double vers_r64(uint16_t f)
{
    return DLS_vers_r64(f);
}

static uint16_t depuis_r64(double f)
{
    return DLS_depuis_r64(f);
}

/* ************************************************************************** */

long GestionnaireChainesAjoutees::ajoute(kuri::chaine chaine)
{
    long decalage = nombre_total_de_lignes;

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

Compilatrice::Compilatrice(kuri::chaine chemin_racine_kuri)
    : ordonnanceuse(this), messagere(this), gestionnaire_code(this),
      gestionnaire_bibliotheques(GestionnaireBibliotheques(*this)),
      racine_kuri(chemin_racine_kuri), typeuse(graphe_dependance, this->operateurs)
{
    initialise_identifiants_ipa(*table_identifiants.verrou_ecriture());

    auto ops = operateurs.verrou_ecriture();
    enregistre_operateurs_basiques(typeuse, *ops);

    espace_de_travail_defaut = demarre_un_espace_de_travail({}, "Espace 1");

    /* Charge le module Kuri. */
    module_kuri = importe_module(espace_de_travail_defaut, "Kuri", nullptr);

    auto table_idents = table_identifiants.verrou_ecriture();

    /* La bibliothèque C. */
    auto libc = gestionnaire_bibliotheques->cree_bibliotheque(
        *espace_de_travail_defaut, nullptr, table_idents->identifiant_pour_chaine("libc"), "c");

    auto malloc_ = libc->cree_symbole("malloc");
    malloc_->surecris_pointeur(reinterpret_cast<Symbole::type_fonction>(notre_malloc));

    auto realloc_ = libc->cree_symbole("realloc");
    realloc_->surecris_pointeur(reinterpret_cast<Symbole::type_fonction>(notre_realloc));

    auto free_ = libc->cree_symbole("free");
    free_->surecris_pointeur(reinterpret_cast<Symbole::type_fonction>(notre_free));

    /* La bibliothèque r16. */
    auto bibr16 = gestionnaire_bibliotheques->cree_bibliotheque(
        *espace_de_travail_defaut,
        nullptr,
        table_idents->identifiant_pour_chaine("libr16"),
        "r16");

    bibr16->cree_symbole("DLS_vers_r32")
        ->surecris_pointeur(reinterpret_cast<Symbole::type_fonction>(vers_r32));
    bibr16->cree_symbole("DLS_depuis_r32")
        ->surecris_pointeur(reinterpret_cast<Symbole::type_fonction>(depuis_r32));
    bibr16->cree_symbole("DLS_vers_r64")
        ->surecris_pointeur(reinterpret_cast<Symbole::type_fonction>(vers_r64));
    bibr16->cree_symbole("DLS_depuis_r64")
        ->surecris_pointeur(reinterpret_cast<Symbole::type_fonction>(depuis_r64));

    /* La bibliothèque pthread. */
    gestionnaire_bibliotheques->cree_bibliotheque(
        *espace_de_travail_defaut,
        nullptr,
        table_idents->identifiant_pour_chaine("libpthread"),
        "pthread");
}

Compilatrice::~Compilatrice()
{
    POUR ((*espaces_de_travail.verrou_ecriture())) {
        memoire::deloge("EspaceDeTravail", it);
    }
}

Module *Compilatrice::importe_module(EspaceDeTravail *espace,
                                     const kuri::chaine &nom,
                                     NoeudExpression const *site)
{
    auto chemin = dls::chaine(nom.pointeur(), nom.taille());

    if (!std::filesystem::exists(chemin.c_str())) {
        /* essaie dans la racine kuri */
        chemin = dls::chaine(racine_kuri.pointeur(), racine_kuri.taille()) + "/modules/" + chemin;

        if (!std::filesystem::exists(chemin.c_str())) {
            erreur::lance_erreur("Impossible de trouver le dossier correspondant au module",
                                 *espace,
                                 site,
                                 erreur::Genre::MODULE_INCONNU);

            return nullptr;
        }
    }

    if (!std::filesystem::is_directory(chemin.c_str())) {
        erreur::lance_erreur("Le nom du module ne pointe pas vers un dossier",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);

        return nullptr;
    }

    /* trouve le chemin absolu du module (cannonique pour supprimer les "../../" */
    auto chemin_absolu = std::filesystem::canonical(std::filesystem::absolute(chemin.c_str()));
    auto nom_dossier = chemin_absolu.filename();

    // @concurrence critique
    auto module = this->trouve_ou_cree_module(
        table_identifiants->identifiant_pour_nouvelle_chaine(nom_dossier.c_str()),
        chemin_absolu.c_str());

    if (module->importe) {
        return module;
    }

    module->importe = true;

    messagere->ajoute_message_module_ouvert(espace, module);

    for (auto const &entree : std::filesystem::directory_iterator(chemin_absolu)) {
        auto chemin_entree = entree.path();

        if (!std::filesystem::is_regular_file(chemin_entree)) {
            continue;
        }

        if (chemin_entree.extension() != ".kuri") {
            continue;
        }

        auto resultat = this->trouve_ou_cree_fichier(
            module, chemin_entree.stem().c_str(), chemin_entree.c_str(), importe_kuri);

        if (resultat.est<FichierNeuf>()) {
            gestionnaire_code->requiers_chargement(espace,
                                                   resultat.resultat<FichierNeuf>().fichier);
        }
    }

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

void Compilatrice::ajoute_fichier_a_la_compilation(EspaceDeTravail *espace,
                                                   const kuri::chaine &nom,
                                                   Module *module,
                                                   NoeudExpression const *site)
{
    auto chemin = dls::chaine(module->chemin()) + dls::chaine(nom) + ".kuri";

    if (!std::filesystem::exists(chemin.c_str())) {
        erreur::lance_erreur("Impossible de trouver le fichier correspondant au module",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    if (!std::filesystem::is_regular_file(chemin.c_str())) {
        erreur::lance_erreur("Le nom du fichier ne pointe pas vers un fichier régulier",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    /* trouve le chemin absolu du fichier */
    auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

    auto resultat = this->trouve_ou_cree_fichier(module, nom, chemin_absolu.c_str(), importe_kuri);

    if (resultat.est<FichierNeuf>()) {
        gestionnaire_code->requiers_chargement(espace, resultat.resultat<FichierNeuf>().fichier);
    }
}

/* ************************************************************************** */

long Compilatrice::memoire_utilisee() const
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
        memoire_fonctions += it.chunk.capacite;
        memoire_fonctions += it.chunk.locales.taille_memoire();
        memoire_fonctions += it.chunk.decalages_labels.taille_memoire();
    });

    donnees_constantes_executions.rassemble_statistiques(stats);

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

bool Compilatrice::globale_contexte_programme_est_disponible()
{
    if (globale_contexte_programme == nullptr) {
        std::unique_lock verrouille(mutex_globale_contexte_programme);

        if (globale_contexte_programme == nullptr) {
            if (module_kuri == nullptr || module_kuri->bloc == nullptr) {
                return false;
            }

            auto decl = trouve_dans_bloc(module_kuri->bloc, ID::__contexte_fil_principal);

            if (!decl) {
                return false;
            }

            globale_contexte_programme = decl->comme_declaration_variable();
        }
    }

    return globale_contexte_programme != nullptr;
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

void Compilatrice::ajoute_chaine_compilation(EspaceDeTravail *espace, kuri::chaine_statique c)
{
    ajoute_chaine_au_module(espace, module_racine_compilation, c);
}

void Compilatrice::ajoute_chaine_au_module(EspaceDeTravail *espace,
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
    fichier->charge_tampon(lng::tampon_source(std::move(chaine)));
    gestionnaire_code->requiers_lexage(espace, fichier);
}

void Compilatrice::ajoute_fichier_compilation(EspaceDeTravail *espace, kuri::chaine_statique c)
{
    auto vue = dls::chaine(c.pointeur(), c.taille());
    auto chemin = std::filesystem::current_path() / vue.c_str();

    if (!std::filesystem::exists(chemin)) {
        espace->rapporte_erreur_sans_site(enchaine("Le fichier ", chemin, " n'existe pas !"));
        return;
    }

    ajoute_fichier_a_la_compilation(espace, chemin.stem().c_str(), module_racine_compilation, {});
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

static kuri::tableau_statique<kuri::Lexeme> converti_tableau_lexemes(
    kuri::tableau<Lexeme, int> const &lexemes)
{
    auto resultat = kuri::tableau_statique<kuri::Lexeme>::cree(lexemes.taille());
    POUR (lexemes) {
        resultat.ajoute({static_cast<int>(it.genre), it.chaine});
    }
    return resultat;
}

kuri::tableau_statique<kuri::Lexeme> Compilatrice::lexe_fichier(EspaceDeTravail *espace,
                                                                kuri::chaine_statique chemin_donne,
                                                                NoeudExpression const *site)
{
    auto chemin = dls::chaine(chemin_donne.pointeur(), chemin_donne.taille());

    if (!std::filesystem::exists(chemin.c_str())) {
        erreur::lance_erreur("Impossible de trouver le fichier correspondant au chemin",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    if (!std::filesystem::is_regular_file(chemin.c_str())) {
        erreur::lance_erreur("Le nom du fichier ne pointe pas vers un fichier régulier",
                             *espace,
                             site,
                             erreur::Genre::MODULE_INCONNU);
    }

    auto chemin_absolu = std::filesystem::absolute(chemin.c_str());

    auto module = this->module(ID::chaine_vide);

    auto resultat = this->trouve_ou_cree_fichier(
        module, chemin_absolu.stem().c_str(), chemin_absolu.c_str(), importe_kuri);

    if (resultat.est<FichierExistant>()) {
        auto donnees_fichier = resultat.resultat<FichierExistant>().fichier;
        return converti_tableau_lexemes(donnees_fichier->lexemes);
    }

    auto donnees_fichier = resultat.resultat<FichierNeuf>().fichier;
    auto tampon = charge_contenu_fichier(chemin);
    donnees_fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

    auto lexeuse = Lexeuse(
        contexte_lexage(espace), donnees_fichier, INCLUS_COMMENTAIRES | INCLUS_CARACTERES_BLANC);
    lexeuse.performe_lexage();

    return converti_tableau_lexemes(donnees_fichier->lexemes);
}

kuri::tableau_statique<NoeudCodeEnteteFonction *> Compilatrice::fonctions_parsees(
    EspaceDeTravail *espace)
{
    auto entetes = gestionnaire_code->fonctions_parsees();
    auto resultat = kuri::tableau_statique<NoeudCodeEnteteFonction *>::cree(entetes.taille());
    POUR (entetes) {
        if (it->est_operateur || it->est_coroutine || it->est_polymorphe) {
            continue;
        }
        auto code_entete = convertisseuse_noeud_code.convertis_noeud_syntaxique(espace, it);
        resultat.ajoute(code_entete->comme_entete_fonction());
    }
    return resultat;
}

Module *Compilatrice::trouve_ou_cree_module(IdentifiantCode *nom_module,
                                            kuri::chaine_statique chemin)
{
    return sys_module->trouve_ou_cree_module(nom_module, chemin);
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
    metaprogramme_->fichier = resultat;
    return resultat;
}

const Fichier *Compilatrice::fichier(long index) const
{
    return sys_module->fichier(index);
}

Fichier *Compilatrice::fichier(long index)
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
    auto atome_fonc = fonctions.ajoute_element(lexeme, nom_fichier);
    return atome_fonc;
}

AtomeFonction *Compilatrice::cree_fonction(const Lexeme *lexeme,
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

AtomeGlobale *Compilatrice::cree_globale(Type *type,
                                         AtomeConstante *initialisateur,
                                         bool est_externe,
                                         bool est_constante)
{
    return globales.ajoute_element(
        typeuse.type_pointeur_pour(type, false), initialisateur, est_externe, est_constante);
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
