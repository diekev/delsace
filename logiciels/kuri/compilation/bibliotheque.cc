/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "bibliotheque.hh"

#ifndef _MSC_VER
#    include <dlfcn.h>
#    include <fcntl.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/lexeuse.hh"

#include "r16/r16_c.h"

#include "statistiques/statistiques.hh"

#include "structures/chemin_systeme.hh"
#include "structures/file.hh"
#include "structures/rassembleuse.hh"

#include "compilatrice.hh"
#include "environnement.hh"
#include "espace_de_travail.hh"

#include "utilitaires/garde_portee.hh"
#include "utilitaires/log.hh"

#ifdef _MSC_VER
#    define NOMINMAX
#    include <libloaderapi.h>
#    include <windows.h>

#    include <array>
#    include <codecvt>
#    include <stack>

/* Référence : https://smacdo.com/code-examples/windows-getlasterror-message/ */
static std::string donne_message_pour_erreur(DWORD code_erreur)
{
    LPSTR tampon_message = nullptr;
    const DWORD drapeaux_format = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                                  FORMAT_MESSAGE_IGNORE_INSERTS;

    /* Utilisons FormatMessageA pour recevoir une chaine UTF-8 (FormatMessage en retourne une
     * UTF-16).
     * https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessagea */
    const auto longueur_message = ::FormatMessageA(drapeaux_format,
                                                   nullptr,
                                                   code_erreur,
                                                   0,
                                                   reinterpret_cast<LPSTR>(&tampon_message),
                                                   0,
                                                   nullptr);

    /* Retourne une erreur générique si nous n'avons pas pu en générer une. */
    if (longueur_message == 0) {
        std::array<char, 50> tampon_temp{};

        const auto longueur_erreur = snprintf(tampon_temp.data(),
                                              tampon_temp.size(),
                                              "::FormatMessage échoua avec l'erreur 0x%x",
                                              static_cast<unsigned int>(::GetLastError()));

        return std::string{tampon_temp.data(),
                           std::min(tampon_temp.size(), size_t(longueur_erreur))};
    }

    std::string message_erreur{tampon_message, longueur_message};
    ::LocalFree(tampon_message);

    while (!message_erreur.empty() &&
           (message_erreur.back() == '\n' || message_erreur.back() == '\r')) {
        message_erreur.erase(message_erreur.end() - 1);
    }

    return message_erreur;
}

static std::string dlerror()
{
    auto error_code = GetLastError();
    return donne_message_pour_erreur(error_code);
}

static void *dlsym(void *handle, LPCSTR name)
{
    return GetProcAddress((HMODULE)handle, name);
}

#    define RTLD_LAZY 0

static void *dlopen(const char *filepath, int /*flags*/)
{
    return LoadLibraryExA(filepath, nullptr, 0);
}

static int dlclose(void *handle)
{
    return FreeLibrary((HMODULE)handle);
}
#endif

/* ************************************************************************** */

/* Redéfini certaines fonctions afin de pouvoir controler leurs comportements.
 * Par exemple, pour les fonctions d'allocations nous voudrions pouvoir libérer
 * la mémoire de notre coté, ou encore vérifier qu'il n'y a pas de fuite de
 * mémoire dans les métaprogrammes.
 */
void *notre_malloc(size_t n)
{
    return malloc(n);
}

void *notre_realloc(void *ptr, size_t taille)
{
    return realloc(ptr, taille);
}

void notre_free(void *ptr)
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

/* ------------------------------------------------------------------------- */
/** \name Utilitaires.
 * \{ */

static int plateforme_pour_options(OptionsDeCompilation const &options)
{
    if (options.architecture == ArchitectureCible::X86) {
        return PLATEFORME_32_BIT;
    }

    return PLATEFORME_64_BIT;
}

static int type_informations(const OptionsDeCompilation &options)
{
    if (options.compilation_pour == CompilationPour::DÉBOGAGE) {
        if (options.utilise_asan) {
            return POUR_DÉBOGAGE_ASAN;
        }

        return POUR_DÉBOGAGE;
    }

    if (options.compilation_pour == CompilationPour::PROFILAGE) {
        return POUR_PROFILAGE;
    }

    return POUR_PRODUCTION;
}

#ifdef _MSC_VER
static kuri::tablet<kuri::chaine_statique, 16> divise_chaine_par(kuri::chaine_statique chn,
                                                                 char separateur)
{
    kuri::tablet<kuri::chaine_statique, 16> resultat;

    auto taille = 0;
    auto pointeur = chn.pointeur();
    auto debut = chn.pointeur();

    for (auto i = 0; i < chn.taille(); i++) {
        if (pointeur[i] == separateur) {
            auto sous_chaine = kuri::chaine_statique(debut, taille);
            taille = 0;
            debut = &pointeur[i + 1];
            resultat.ajoute(sous_chaine);
            continue;
        }

        taille += 1;
    }

    if (taille != 0) {
        auto sous_chaine = kuri::chaine_statique(debut, taille);
        resultat.ajoute(sous_chaine);
    }

    return resultat;
}

static void ajoute_chemins_depuis_env(const char *variable,
                                      kuri::tablet<kuri::chemin_systeme, 16> &chemins,
                                      kuri::ensemblon<kuri::chaine_statique, 16> &chemins_connus)
{
    auto valeurs = getenv(variable);
    if (!valeurs) {
        return;
    }

    auto chaines = divise_chaine_par(valeurs, ';');
    POUR (chaines) {
        if (chemins_connus.possède(it)) {
            continue;
        }

        chemins.ajoute(it);
        chemins_connus.insère(it);
    }
}
#endif

static kuri::tablet<kuri::chemin_systeme, 16> chemins_systeme_pour(ArchitectureCible architecture)
{
    kuri::tablet<kuri::chemin_systeme, 16> résultat;

    /* Pour les tables r16. */
    résultat.ajoute(chemin_de_base_pour_bibliothèque_r16(architecture));

#ifdef _MSC_VER
    kuri::ensemblon<kuri::chaine_statique, 16> chemins_connus;
    chemins_connus.insère(résultat[0]);

    if (architecture == ArchitectureCible::X64) {
        ajoute_chemins_depuis_env("LIB", résultat, chemins_connus);
        ajoute_chemins_depuis_env("LIBPATH", résultat, chemins_connus);
    }
    // A FAIRE : version 32-bit
#else
    if (architecture == ArchitectureCible::X64) {
        résultat.ajoute("/lib/x86_64-linux-gnu/");
        résultat.ajoute("/usr/lib/x86_64-linux-gnu/");
    }
    else {
        résultat.ajoute("/lib/i386-linux-gnu/");
        résultat.ajoute("/usr/lib/i386-linux-gnu/");
    }
#endif

    return résultat;
}

static kuri::tablet<kuri::chemin_systeme, 16> chemins_syteme_x86_64{};
static kuri::tablet<kuri::chemin_systeme, 16> chemins_syteme_i386{};

static void initialise_chemins_systeme()
{
    chemins_syteme_x86_64 = chemins_systeme_pour(ArchitectureCible::X64);
    chemins_syteme_i386 = chemins_systeme_pour(ArchitectureCible::X86);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Symbole
 * \{ */

bool Symbole::charge(EspaceDeTravail *espace,
                     NoeudExpression const *site,
                     RaisonRechercheSymbole raison)
{
    switch (raison) {
        case RaisonRechercheSymbole::EXÉCUTION_MÉTAPROGRAMME:
        {
            /* Si nous avons une adresse pour l'exécution, il est inutile d'essayer de charger le
             * symbole. NOTE : puisque adresse_exécution est une union, nous pouvons tester
             * n'importe quel rubrique. */
            if (adresse_exécution.fonction || état_recherche == ÉtatRechercheSymbole::TROUVÉ) {
                return true;
            }
            break;
        }
        case RaisonRechercheSymbole::LIAISON_PROGRAMME_FINAL:
        {
            if (état_recherche == ÉtatRechercheSymbole::TROUVÉ) {
                return true;
            }
            break;
        }
    }

    if (bibliothèque->état_recherche != ÉtatRechercheBibliothèque::TROUVÉE) {
        if (!bibliothèque->charge(espace)) {
            return false;
        }
    }

    auto ptr_symbole = bibliothèque->bib(nom);
    if (ptr_symbole == nullptr) {
        espace->rapporte_erreur(site, "Impossible de trouver un symbole !")
            .ajoute_message("La bibliothèque « ",
                            bibliothèque->ident->nom,
                            " » ne possède pas le symbole « ",
                            nom,
                            " » !\n");
        état_recherche = ÉtatRechercheSymbole::INTROUVÉ;
        return false;
    }

    if (type == TypeSymbole::FONCTION) {
        this->adresse_liaison.fonction = reinterpret_cast<Symbole::type_adresse_fonction>(
            ptr_symbole);
    }
    else {
        this->adresse_liaison.objet = ptr_symbole;
    }
    état_recherche = ÉtatRechercheSymbole::TROUVÉ;
    return true;
}

void Symbole::définis_adresse_pour_exécution(type_adresse_fonction adresse)
{
    assert(type == TypeSymbole::FONCTION);
    adresse_exécution.fonction = adresse;
}

void Symbole::définis_adresse_pour_exécution(type_adresse_objet adresse)
{
    assert(type == TypeSymbole::VARIABLE_GLOBALE);
    adresse_exécution.objet = adresse;
}

Symbole::type_adresse_fonction Symbole::donne_adresse_fonction_pour_exécution()
{
    assert(type == TypeSymbole::FONCTION);
    if (type != TypeSymbole::FONCTION) {
        return nullptr;
    }

    if (adresse_exécution.fonction) {
        return adresse_exécution.fonction;
    }
    return adresse_liaison.fonction;
}

Symbole::type_adresse_objet Symbole::donne_adresse_objet_pour_exécution()
{
    assert(type == TypeSymbole::VARIABLE_GLOBALE);
    if (type != TypeSymbole::VARIABLE_GLOBALE) {
        return nullptr;
    }

    if (adresse_exécution.objet) {
        return adresse_exécution.objet;
    }
    return adresse_liaison.objet;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name IndexBibliothèque
 * \{ */

IndexBibliothèque IndexBibliothèque::crée_pour_exécution()
{
    IndexBibliothèque résultat;
    résultat.plateforme = PLATEFORME_64_BIT;
    résultat.type_liaison = DYNAMIQUE;
    résultat.type_compilation = POUR_PRODUCTION;
    return résultat;
}

IndexBibliothèque IndexBibliothèque::crée_pour_options(OptionsDeCompilation const &options,
                                                       int type_liaison)
{
    IndexBibliothèque résultat;
    résultat.plateforme = plateforme_pour_options(options);
    résultat.type_liaison = type_liaison;
    résultat.type_compilation = type_informations(options);
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name CheminsBibliothèque
 * \{ */

kuri::chaine_statique CheminsBibliothèque::donne_chemin(IndexBibliothèque const index) const
{
    auto const &chemins = m_chemins[index.plateforme][index.type_liaison];

    /* Utilise le chemins pour production par défaut. */
    if (!chemins[index.type_compilation]) {
        return chemins[POUR_PRODUCTION];
    }

    return chemins[index.type_compilation];
}

IndexBibliothèque CheminsBibliothèque::rafine_index(IndexBibliothèque const index) const
{
    auto résultat = index;
    auto const &chemins = m_chemins[index.plateforme][index.type_liaison];
    /* Utilise le chemins pour production par défaut. */
    if (!chemins[index.type_compilation]) {
        résultat.type_compilation = POUR_PRODUCTION;
    }
    return résultat;
}

void CheminsBibliothèque::définis_chemins(
    int plateforme,
    const kuri::chemin_systeme nouveaux_chemins[NUM_TYPES_BIBLIOTHÈQUE]
                                               [NUM_TYPES_INFORMATION_BIBLIOTHÈQUE])
{
    for (int i = 0; i < NUM_TYPES_BIBLIOTHÈQUE; i++) {
        for (int j = 0; j < NUM_TYPES_INFORMATION_BIBLIOTHÈQUE; j++) {
            m_chemins[plateforme][i][j] = nouveaux_chemins[i][j];
        }
    }
}

int64_t CheminsBibliothèque::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    for (int i = 0; i < NUM_TYPES_PLATEFORME; i++) {
        for (int j = 0; j < NUM_TYPES_BIBLIOTHÈQUE; j++) {
            for (int k = 0; k < NUM_TYPES_INFORMATION_BIBLIOTHÈQUE; k++) {
                résultat += m_chemins[i][j][k].taille();
            }
        }
    }
    return résultat;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name BibliothèqueExécutable
 * Représente une bibliothèque partagée (.so sur Linux, .dll sur Windows) qui
 * sera utilisée lors de l'exécution dans la machine virtuelle.
 * \{ */

BibliothèqueExécutable::BibliothèqueExécutable(kuri::chaine_statique chemin)
{
    auto std_chemin = vers_std_path(chemin);

#ifdef _MSC_VER
    std::wstring w_string = std_chemin.c_str();
    using convert_type = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convert_type, wchar_t> converter;
    std::string c_string = converter.to_bytes(w_string);
    const char *filepath = c_string.c_str();
#else
    const char *filepath = std_chemin.c_str();
#endif

    m_handle = dlopen(filepath, RTLD_LAZY);
}

BibliothèqueExécutable::~BibliothèqueExécutable() noexcept
{
    if (m_handle) {
        dlclose(m_handle);
    }
}

BibliothèqueExécutable::BibliothèqueExécutable(BibliothèqueExécutable &&autre) noexcept
{
    std::swap(m_handle, autre.m_handle);
}

BibliothèqueExécutable &BibliothèqueExécutable::operator=(BibliothèqueExécutable &&autre) noexcept
{
    std::swap(m_handle, autre.m_handle);
    return *this;
}

BibliothèqueExécutable::operator bool() const noexcept
{
    return m_handle != nullptr;
}

void *BibliothèqueExécutable::operator()(kuri::chaine_statique symbol)
{
    /* Vide les erreurs. */
    dlerror();

    auto symbol_c = std::string(symbol.pointeur(), size_t(symbol.taille()));
    const auto sym = dlsym(m_handle, symbol_c.c_str());
    if (sym == nullptr) {
        dbg() << dlerror();
    }

    return sym;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Bibliothèque
 * \{ */

Symbole *Bibliothèque::crée_symbole(kuri::chaine_statique nom_symbole, TypeSymbole type)
{
    POUR_TABLEAU_PAGE (symboles) {
        if (it.nom == nom_symbole) {
            return &it;
        }
    }

    auto symbole = symboles.ajoute_élément(type);
    symbole->nom = nom_symbole;
    symbole->bibliothèque = this;
    return symbole;
}

bool Bibliothèque::charge(EspaceDeTravail *espace)
{
    if (état_recherche == ÉtatRechercheBibliothèque::TROUVÉE) {
        return true;
    }

    auto chemin_dynamique = chemins.donne_chemin(IndexBibliothèque::crée_pour_exécution());

    if (chemin_dynamique == "") {
        espace
            ->rapporte_erreur(
                site, "Impossible de charger une bibliothèque dynamique pour l'exécution du code")
            .ajoute_message("La bibliothèque « ", nom, " » n'a pas de version dynamique !\n");
        return false;
    }

    this->bib = BibliothèqueExécutable(chemin_dynamique);
    if (!this->bib) {
        espace->rapporte_erreur(
            site, enchaine("Impossible de charger la bibliothèque « ", nom, " » !\n"));
        état_recherche = ÉtatRechercheBibliothèque::INTROUVÉE;
        return false;
    }

    état_recherche = ÉtatRechercheBibliothèque::TROUVÉE;
    return true;
}

int64_t Bibliothèque::mémoire_utilisée() const
{
    auto résultat = symboles.mémoire_utilisée();
    POUR_TABLEAU_PAGE (symboles) {
        résultat += it.nom.taille();
    }
    résultat += chemins.mémoire_utilisée();
    for (int k = 0; k < NUM_TYPES_INFORMATION_BIBLIOTHÈQUE; k++) {
        résultat += noms[k].taille();
    }
    résultat += dépendances.taille_mémoire();
    return résultat;
}

void Bibliothèque::ajoute_dépendance(Bibliothèque *dépendance)
{
    dépendances.ajoute(dépendance);
    dépendance->prépendances.ajoute(this);
}

kuri::chaine_statique Bibliothèque::chemin_de_base(const OptionsDeCompilation &options) const
{
    return chemins_de_base[plateforme_pour_options(options)];
}

kuri::chaine_statique Bibliothèque::chemin_statique(const OptionsDeCompilation &options) const
{
    return chemins.donne_chemin(IndexBibliothèque::crée_pour_options(options, STATIQUE));
}

kuri::chaine_statique Bibliothèque::chemin_dynamique(const OptionsDeCompilation &options) const
{
    return chemins.donne_chemin(IndexBibliothèque::crée_pour_options(options, DYNAMIQUE));
}

kuri::chaine_statique Bibliothèque::nom_pour_liaison(const OptionsDeCompilation &options) const
{
    auto index = IndexBibliothèque::crée_pour_options(options, DYNAMIQUE);
    index = chemins.rafine_index(index);
    return noms[index.type_compilation];
}

bool Bibliothèque::peut_lier_statiquement() const
{
    /* libc et libm sont la même bibliothèque, et ne peuvent pas être liées statiquement. */
    if (nom == "c" || nom == "m") {
        return false;
    }
    return chemins.donne_chemin(IndexBibliothèque{PLATEFORME_64_BIT, STATIQUE, POUR_PRODUCTION}) !=
           kuri::chaine_statique("");
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name BibliothèquesUtilisées
 * Représentation des bibliothèques utilisées par un programme.
 * \{ */

BibliothèquesUtilisées::BibliothèquesUtilisées() = default;

BibliothèquesUtilisées::BibliothèquesUtilisées(kuri::ensemble<Bibliothèque *> const &ensemble)
    : m_ensemble(ensemble)
{
    m_bibliothèques = m_ensemble.donne_tableau();
    trie_bibliothèques();
}

kuri::tableau_statique<Bibliothèque *> BibliothèquesUtilisées::donne_tableau() const
{
    return m_bibliothèques;
}

int64_t BibliothèquesUtilisées::mémoire_utilisée() const
{
    auto résultat = int64_t(0);
    résultat += m_bibliothèques.taille_mémoire();
    résultat += m_ensemble.taille_mémoire();
    return résultat;
}

void BibliothèquesUtilisées::efface()
{
    m_bibliothèques.efface();
    m_ensemble.efface();
}

bool BibliothèquesUtilisées::peut_lier_statiquement(Bibliothèque const *bibliothèque) const
{
    /* Cette vérification est pour la racine. La vérification plus bas sur les prépendances n'est
     * pas vraiment redondante car nous devons exclure les prépendances qui ne sont pas incluses
     * dans le programme. */
    if (!m_ensemble.possède(const_cast<Bibliothèque *>(bibliothèque))) {
        return false;
    }

    if (!bibliothèque->peut_lier_statiquement()) {
        return false;
    }

    /* Une bibliothèque ne peut être liée statiquement que si ses prépendances peuvent également
     * l'être. */
    POUR (bibliothèque->prépendances.plage()) {
        /* Ignore les prépendances ne faisant pas partie du programme. */
        if (!m_ensemble.possède(const_cast<Bibliothèque *>(it))) {
            continue;
        }

        if (!peut_lier_statiquement(it)) {
            return false;
        }
    }

    return true;
}

void BibliothèquesUtilisées::trie_bibliothèques()
{
    kuri::file<Bibliothèque *> bibliothèques_à_traiter;
    kuri::rassembleuse<Bibliothèque *> bibliothèques_triées;

    POUR (m_bibliothèques) {
        bibliothèques_à_traiter.enfile(it);
    }

    auto toutes_les_prépendances_furent_ajoutées = [&](Bibliothèque *bibliothèque) {
        if (bibliothèque->prépendances.taille() == 0) {
            return true;
        }

        POUR (bibliothèque->prépendances.plage()) {
            if (!m_ensemble.possède(it)) {
                continue;
            }

            if (!bibliothèques_triées.possède(it)) {
                return false;
            }
        }

        return true;
    };

    while (!bibliothèques_à_traiter.est_vide()) {
        auto bibliothèque = bibliothèques_à_traiter.defile();

        if (toutes_les_prépendances_furent_ajoutées(bibliothèque)) {
            bibliothèques_triées.insère(bibliothèque);
            continue;
        }

        /* Recommence plus tard. */
        bibliothèques_à_traiter.enfile(bibliothèque);
    }

    m_bibliothèques = bibliothèques_triées.donne_copie_éléments();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name GestionnaireBibliothèques
 * \{ */

static kuri::chaine_statique nom_bibliothèque_c()
{
#ifdef _MSC_VER
    // "ucrt" est pour la version dynamique, "libucrt" est pour la version statique
    return "ucrt";
#else
    return "c";
#endif
}

GestionnaireBibliothèques::GestionnaireBibliothèques(Compilatrice &compilatrice_)
    : compilatrice(compilatrice_)
{
    initialise_chemins_systeme();
}

bool GestionnaireBibliothèques::initialise_bibliothèques_pour_exécution(Compilatrice &compilatrice)
{
    auto table_idents = compilatrice.table_identifiants.verrou_ecriture();
    auto gestionnaire = compilatrice.gestionnaire_bibliothèques.verrou_ecriture();
    auto espace = compilatrice.espace_défaut_compilation();

    /* La bibliothèque C. */
    auto libc = gestionnaire->crée_bibliothèque(*espace, nullptr, ID::libc, nom_bibliothèque_c());

    auto malloc_ = libc->crée_symbole("malloc", TypeSymbole::FONCTION);
    malloc_->définis_adresse_pour_exécution(
        reinterpret_cast<Symbole::type_adresse_fonction>(notre_malloc));

    auto realloc_ = libc->crée_symbole("realloc", TypeSymbole::FONCTION);
    realloc_->définis_adresse_pour_exécution(
        reinterpret_cast<Symbole::type_adresse_fonction>(notre_realloc));

    auto free_ = libc->crée_symbole("free", TypeSymbole::FONCTION);
    free_->définis_adresse_pour_exécution(
        reinterpret_cast<Symbole::type_adresse_fonction>(notre_free));

    /* La bibliothèque r16. */
    auto bibr16 = gestionnaire->crée_bibliothèque(
        *espace, nullptr, table_idents->identifiant_pour_chaine("libr16"), "r16");

    bibr16->crée_symbole("DLS_vers_r32", TypeSymbole::FONCTION)
        ->définis_adresse_pour_exécution(
            reinterpret_cast<Symbole::type_adresse_fonction>(vers_r32));
    bibr16->crée_symbole("DLS_depuis_r32", TypeSymbole::FONCTION)
        ->définis_adresse_pour_exécution(
            reinterpret_cast<Symbole::type_adresse_fonction>(depuis_r32));
    bibr16->crée_symbole("DLS_vers_r64", TypeSymbole::FONCTION)
        ->définis_adresse_pour_exécution(
            reinterpret_cast<Symbole::type_adresse_fonction>(vers_r64));
    bibr16->crée_symbole("DLS_depuis_r64", TypeSymbole::FONCTION)
        ->définis_adresse_pour_exécution(
            reinterpret_cast<Symbole::type_adresse_fonction>(depuis_r64));

#ifndef _MSC_VER
    /* La bibliothèque pthread. */
    gestionnaire->crée_bibliothèque(
        *espace, nullptr, table_idents->identifiant_pour_chaine("libpthread"), "pthread");
#endif

    return !compilatrice.possède_erreur();
}

Bibliothèque *GestionnaireBibliothèques::trouve_bibliothèque(IdentifiantCode *ident)
{
    POUR_TABLEAU_PAGE (bibliothèques) {
        if (it.ident == ident) {
            return &it;
        }
    }

    return nullptr;
}

Bibliothèque *GestionnaireBibliothèques::trouve_ou_crée_bibliothèque(EspaceDeTravail &espace,
                                                                     IdentifiantCode *ident)
{
    return crée_bibliothèque(espace, nullptr, ident, "");
}

Bibliothèque *GestionnaireBibliothèques::crée_bibliothèque(EspaceDeTravail &espace,
                                                           NoeudExpression *site)
{
    return crée_bibliothèque(espace, site, site->ident, "");
}

Bibliothèque *GestionnaireBibliothèques::crée_bibliothèque(EspaceDeTravail &espace,
                                                           NoeudExpression *site,
                                                           IdentifiantCode *ident,
                                                           kuri::chaine_statique nom)
{
    auto bibliothèque = trouve_bibliothèque(ident);

    if (bibliothèque) {
        if (nom != "" &&
            bibliothèque->état_recherche == ÉtatRechercheBibliothèque::NON_RECHERCHÉE) {
            bibliothèque->site = site;
            bibliothèque->ident = ident;
            bibliothèque->nom = nom;
            résoud_chemins_bibliothèque(espace, site, bibliothèque);
        }

        return bibliothèque;
    }

    bibliothèque = bibliothèques.ajoute_élément();

    if (nom != "") {
        bibliothèque->nom = nom;
        résoud_chemins_bibliothèque(espace, site, bibliothèque);
    }

    bibliothèque->site = site;
    bibliothèque->ident = ident;
    return bibliothèque;
}

#ifndef _MSC_VER
static bool est_fichier_elf(unsigned char tampon[4])
{
    /* .ELF */
    return tampon[0] == 0x7f && tampon[1] == 0x45 && tampon[2] == 0x4c && tampon[3] == 0x46;
}

static int64_t taille_fichier(int fd)
{
    auto debut = lseek(fd, 0, SEEK_SET);
    auto fin = lseek(fd, 0, SEEK_END);

    /* Remettons nous au début. */
    lseek(fd, 0, SEEK_SET);

    if (debut == -1 || fin == -1) {
        return -1;
    }

    return fin - debut;
}

static kuri::chaine_statique explication_errno_ouverture_fichier()
{
    auto err = *__errno_location();
    switch (err) {
        case EACCES:
        {
            return "permission non accordée";
        }
        case EDQUOT:
        {
            return "espace disque insuffisant";
        }  // O_CREAT
        case EEXIST:
        {
            return "fichier existe déjà";
        }  // O_CREAT & O_EXCL
        case EFAULT:
        {
            return "mauvais espace d'adressage";
        }
        case EFBIG:
        {
            return "fichier trop grand";
        }
        case EINTR:
        {
            return "interrompu par signal";
        }
        case EINVAL:
        {
            return "valeur invalide";
        }  // dépend du drapeau
        case EISDIR:
        {
            return "ouverture impossible car dossier";
        }  // Si O_WRONLY | O_RDWR ou O_TMPFILE and one of O_WRONLY or O_RDWR
        case ELOOP:
        {
            return "trop de liens symboliques";
        }  // dépend du drapeau
        case EMFILE:
        {
            return "limite ouverture de fichiers atteinte";
        }
        case ENAMETOOLONG:
        {
            return "nom trop grand";
        }
        case ENFILE:
        {
            return "limite ouverture de fichiers atteinte";
        }
        case ENODEV:
        {
            return "matériel inconnu";
        }
        case ENOENT:
        {
            return "fichier inexistant";
        }  // dépend du drapeau
        case ENOMEM:
        {
            return "mémoire insuffisante";
        }  // dépend du type de fichier
        case ENOSPC:
        {
            return "espace disque insuffisant";
        }
        case ENOTDIR:
        {
            return "pas dans un dossier";
        }
        case ENXIO:
        {
            return "entrée sortie spéciale";
        }  // dépend du type de fichier
        case EOPNOTSUPP:
        {
            return "fichier temporaire non supporté";
        }
        case EOVERFLOW:
        {
            return "fichier trop grand";
        }
        case EPERM:
        {
            return "permission non accordée";
        }  // dépend du drapeau
        case EROFS:
        {
            return "fichier est en lecture seule";
        }  // dépend du drapeau
        case ETXTBSY:
        {
            return "image en lecture";
        }  // dépend du drapeau
        case EWOULDBLOCK:
        {
            return "ouverture peut bloquer";
        }  // dépend du drapeau
    }

    return "raison inconnue";
}

/* Vérifie si le chemin dynamique pointe vers une bibliothèque partagée, ou vers un script LD.
 * La plupart des bibliothèques standarde de C (comme libc.so, libm.so, ou libpthread.so, sont
 * en fait des scripts LD).
 * Si nous pointons vers un script LD, nous parsons le script pour déterminer le chemin exacte.
 * Voir https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_3.html pour la référence
 * des options des scripts LD. */
static kuri::chemin_systeme resoud_chemin_dynamique_si_script_ld(
    EspaceDeTravail &espace, NoeudExpression *site, kuri::chemin_systeme const &chemin_dynamique)
{
    /* Ouvre le fichier pour voir si nous avons un fichier elf, qui doit commencer par .ELF (ou
     * 0x7f 0x45 0x4c 0x46). */

    /* Garantie l'existence d'un caractère nul à la fin du nom. */
    assert(chemin_dynamique.taille() < 1023);
    char tampon_chaine_c[1024];
    memcpy(tampon_chaine_c,
           chemin_dynamique.pointeur(),
           static_cast<size_t>(chemin_dynamique.taille()));
    tampon_chaine_c[chemin_dynamique.taille()] = '\0';

    int fd = open(tampon_chaine_c, O_RDONLY);
    SUR_SORTIE_PORTEE {
        close(fd);
    };

    if (fd == -1) {
        auto explication = explication_errno_ouverture_fichier();
        espace.rapporte_erreur(site, "Impossible d'ouvrir le fichier alors que le chemin existe !")
            .ajoute_message("Note : ", explication);
        return chemin_dynamique;
    }

    unsigned char tampon[4];
    if (read(fd, tampon, 4) != 4) {
        espace.rapporte_erreur(site, "Impossible de lire l'entête du fichier !");
        return chemin_dynamique;
    }

    if (est_fichier_elf(tampon)) {
        return chemin_dynamique;
    }

    /* Nous n'avons pas de fichier ELF, parsons le comme un script LD. */

    auto taille = taille_fichier(fd);
    if (taille == -1) {
        espace.rapporte_erreur(site, "Impossible de déterminer la taille du fichier !");
        return chemin_dynamique;
    }

    auto chaine = kuri::chaine();
    chaine.redimensionne(taille);
    auto taille_lue = read(fd, &chaine[0], static_cast<size_t>(taille));
    if (taille_lue != taille) {
        espace.rapporte_erreur(site, "Nous n'avons pas lu la quantité de données désirée !")
            .ajoute_message("La taille désirée est de ", taille, "\n")
            .ajoute_message("La taille lue est de ", taille_lue, "\n");
        return chemin_dynamique;
    }

    /* Cherche le mot-clé GROUP, nous ne pouvons pas utiliser de Lexeuse, car les chaines n'ont pas
     * de guillemets. */
    auto pos = chaine.trouve("GROUP");
    if (pos == -1) {
        espace.rapporte_erreur(site,
                               "Impossible de trouver le mot-clé GROUP dans le script LD. !");
        return chemin_dynamique;
    }

    /* Prend le premier chemin disponible.
     * À FAIRE(bibliothèques) : considère tous les chemins du GROUP
     * À FAIRE(bibliothèques) : considère la directive AS_NEEDED. */
    auto pos_premier_slash = chaine.trouve('/', pos);
    if (pos_premier_slash == -1) {
        espace.rapporte_erreur(
            site, "Impossible de trouver le premier slash après GROUP dans le script LD. !");
        return chemin_dynamique;
    }

    /* Les chemins sont délimités par des espace. */
    auto pos_premiere_espace = chaine.trouve(' ', pos_premier_slash + 1);
    if (pos_premiere_espace == -1) {
        espace.rapporte_erreur(
            site,
            "Impossible de trouver le premier espace après le premier slash dans le script LD. !");
        return chemin_dynamique;
    }

    auto chemin_potentiel = chaine.sous_chaine(pos_premier_slash, pos_premiere_espace);
    return chemin_potentiel;
}
#endif

struct ResultatRechercheBibliothèque {
    kuri::chaine_statique chemin_de_base = "";
    kuri::chemin_systeme chemins[NUM_TYPES_BIBLIOTHÈQUE][NUM_TYPES_INFORMATION_BIBLIOTHÈQUE];
};

static std::optional<ResultatRechercheBibliothèque> recherche_bibliothèque(
    EspaceDeTravail &espace,
    NoeudExpression *site,
    kuri::tablet<kuri::chaine_statique, 4> const &dossiers,
    kuri::chaine const noms[NUM_TYPES_BIBLIOTHÈQUE][NUM_TYPES_INFORMATION_BIBLIOTHÈQUE])
{
    auto résultat = ResultatRechercheBibliothèque();

    bool chemin_trouve[NUM_TYPES_BIBLIOTHÈQUE][NUM_TYPES_INFORMATION_BIBLIOTHÈQUE];

    POUR (dossiers) {
        résultat.chemin_de_base = "";
        for (int i = 0; i < NUM_TYPES_BIBLIOTHÈQUE; i++) {
            for (int j = 0; j < NUM_TYPES_INFORMATION_BIBLIOTHÈQUE; j++) {
                chemin_trouve[i][j] = false;
                résultat.chemins[i][j] = "";
            }
        }

        for (int i = 0; i < NUM_TYPES_BIBLIOTHÈQUE; i++) {
            for (int j = 0; j < NUM_TYPES_INFORMATION_BIBLIOTHÈQUE; j++) {
                if (chemin_trouve[i][j]) {
                    continue;
                }

                auto chemin_test = kuri::chemin_systeme(it) / noms[i][j];
                if (!kuri::chemin_systeme::existe(chemin_test)) {
                    continue;
                }

                chemin_trouve[i][j] = true;
                résultat.chemins[i][j] = chemin_test;
            }
        }
        /* Les bibliothèques doivent être dans le même dossier. */
        if (chemin_trouve[STATIQUE][POUR_PRODUCTION] ||
            chemin_trouve[DYNAMIQUE][POUR_PRODUCTION]) {
            résultat.chemin_de_base = it;
            break;
        }
    }

#ifndef _MSC_VER
    if (chemin_trouve[DYNAMIQUE][POUR_PRODUCTION]) {
        résultat.chemins[DYNAMIQUE][POUR_PRODUCTION] = resoud_chemin_dynamique_si_script_ld(
            espace, site, résultat.chemins[DYNAMIQUE][POUR_PRODUCTION]);
    }
#endif

    if (chemin_trouve[STATIQUE][POUR_PRODUCTION] || chemin_trouve[DYNAMIQUE][POUR_PRODUCTION]) {
        return résultat;
    }

    return {};
}

static void garantie_chemins_bibliothèques(Module *module)
{

    if (module->chemin_bibliothèque_32bits.taille() == 0) {
        module->chemin_bibliothèque_32bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliothèque(
                                                 ArchitectureCible::X86);
        module->chemin_bibliothèque_64bits = module->chemin() /
                                             suffixe_chemin_module_pour_bibliothèque(
                                                 ArchitectureCible::X64);
    }
}

static kuri::tablet<kuri::chaine_statique, 4> dossiers_recherche_32_bits(
    EspaceDeTravail const *espace, NoeudExpression *site)
{
    kuri::tablet<kuri::chaine_statique, 4> dossiers;
    if (site) {
        const auto fichier = espace->fichier(site->lexème->fichier);
        auto module = fichier->module;
        garantie_chemins_bibliothèques(module);
        dossiers.ajoute(module->chemin_bibliothèque_32bits);
    }

    POUR (chemins_syteme_i386) {
        dossiers.ajoute(it);
    }

    return dossiers;
}

static kuri::tablet<kuri::chaine_statique, 4> dossiers_recherche_64_bits(
    EspaceDeTravail const *espace, NoeudExpression *site)
{
    kuri::tablet<kuri::chaine_statique, 4> dossiers;
    if (site) {
        const auto fichier = espace->fichier(site->lexème->fichier);
        const auto module = fichier->module;
        garantie_chemins_bibliothèques(module);
        dossiers.ajoute(module->chemin_bibliothèque_64bits);
    }

    POUR (chemins_syteme_x86_64) {
        dossiers.ajoute(it);
    }

    return dossiers;
}

static kuri::tablet<kuri::chaine_statique, 4> dossiers_recherche_plateforme(
    EspaceDeTravail const *espace, NoeudExpression *site, int plateforme)
{
    if (plateforme == PLATEFORME_32_BIT) {
        return dossiers_recherche_32_bits(espace, site);
    }
    return dossiers_recherche_64_bits(espace, site);
}

static void copie_chemins(ResultatRechercheBibliothèque const &résultat,
                          Bibliothèque *bibliothèque,
                          int plateforme)
{
    bibliothèque->chemins.définis_chemins(plateforme, résultat.chemins);
    bibliothèque->chemins_de_base[plateforme] = résultat.chemin_de_base;
}

static void rapporte_erreur_bibliothèque_introuvable(
    EspaceDeTravail &espace,
    const NoeudExpression *site,
    const Bibliothèque *bibliothèque,
    int plateforme,
    const kuri::chaine noms[2][4],
    const kuri::tablet<kuri::chaine_statique, 4> &dossiers)
{
    auto e = espace.rapporte_erreur(site,
                                    "Impossible de résoudre le chemin vers une bibliothèque");
    e.ajoute_message("La bibliothèque en question est « ", bibliothèque->nom, " »\n\n");
    e.ajoute_message("La plateforme cible de l'espace est : ",
                     (plateforme == PLATEFORME_32_BIT) ? "32-bit" : "64-bit",
                     "\n");
    e.ajoute_message("Les chemins testés furent :\n");
    POUR (dossiers) {
        e.ajoute_message("    ", it, "/", noms[STATIQUE][POUR_PRODUCTION], "\n");
        e.ajoute_message("    ", it, "/", noms[DYNAMIQUE][POUR_PRODUCTION], "\n");
    }
}

void GestionnaireBibliothèques::résoud_chemins_bibliothèque(EspaceDeTravail &espace,
                                                            NoeudExpression *site,
                                                            Bibliothèque *bibliothèque)
{
    auto const plateforme_requise = plateforme_pour_options(espace.options);
    // regarde soit dans le module courant, soit dans le chemin système
    // chemin_système : /lib/x86_64-linux-gnu/ pour 64-bit
    //                  /lib/i386-linux-gnu/ pour 32-bit

    // essaye de déterminer le chemin
    // pour un fichier statique :
    // /chemin/de/base/libnom.a
    // pour un fichier dynamique :
    // /chemin/de/base/libnom.so
    kuri::chaine noms[NUM_TYPES_BIBLIOTHÈQUE][NUM_TYPES_INFORMATION_BIBLIOTHÈQUE];

    bibliothèque->noms[POUR_PRODUCTION] = bibliothèque->nom;
    bibliothèque->noms[POUR_PROFILAGE] = enchaine(bibliothèque->nom, "_profile");
    bibliothèque->noms[POUR_DÉBOGAGE] = enchaine(bibliothèque->nom, "_debogage");
    bibliothèque->noms[POUR_DÉBOGAGE_ASAN] = enchaine(bibliothèque->nom, "_asan");

    for (int i = 0; i < NUM_TYPES_INFORMATION_BIBLIOTHÈQUE; i++) {
        noms[STATIQUE][i] = nom_bibliothèque_statique_pour(bibliothèque->noms[i], true);
        noms[DYNAMIQUE][i] = nom_bibliothèque_dynamique_pour(bibliothèque->noms[i], true);
    }

    const int plateformes[2] = {
        PLATEFORME_32_BIT,
        PLATEFORME_64_BIT,
    };

    POUR (plateformes) {
        auto dossiers = dossiers_recherche_plateforme(&espace, site, it);
        auto résultat = recherche_bibliothèque(espace, site, dossiers, noms);

        if (résultat.has_value()) {
            copie_chemins(résultat.value(), bibliothèque, it);
            continue;
        }

        if (plateforme_requise == it) {
            rapporte_erreur_bibliothèque_introuvable(
                espace, site, bibliothèque, plateforme_requise, noms, dossiers);
            return;
        }
    }
}

int64_t GestionnaireBibliothèques::mémoire_utilisée() const
{
    auto memoire = bibliothèques.mémoire_utilisée();
    POUR_TABLEAU_PAGE (bibliothèques) {
        memoire += it.mémoire_utilisée();
    }
    return memoire;
}

void GestionnaireBibliothèques::rassemble_statistiques(Statistiques &stats) const
{
    stats.ajoute_mémoire_utilisée("Bibliothèques", mémoire_utilisée());
}

/** \} */
