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

#include "bibliotheque.hh"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "arbre_syntaxique/noeud_expression.hh"

#include "parsage/lexeuse.hh"

#include "statistiques/statistiques.hh"

#include "compilatrice.hh"
#include "espace_de_travail.hh"

/* garde_portee.h doit être inclus après les lexèmes car DIFFERE y est définis comme un macro. */
#include "biblinternes/outils/garde_portee.h"

static bool fichier_existe(kuri::chaine const &chemin)
{
    const auto std_string = std::string(chemin.pointeur(), static_cast<size_t>(chemin.taille()));
    const auto std_path = std::filesystem::path(std_string);

    if (std::filesystem::exists(std_path)) {
        return true;
    }

    return false;
}

bool Symbole::charge(EspaceDeTravail *espace, NoeudExpression const *site)
{
    // À FAIRE(bibliothèque) : surécris n'est que pour les l'exécution dans la MV
    if (etat_recherche == EtatRechercheSymbole::TROUVE ||
        etat_recherche == EtatRechercheSymbole::SURECRIS) {
        return true;
    }

    if (bibliotheque->etat_recherche != EtatRechercheBibliotheque::TROUVEE) {
        if (!bibliotheque->charge(espace)) {
            return false;
        }
    }

    try {
        auto ptr_symbole = bibliotheque->bib(dls::chaine(nom.pointeur(), nom.taille()));
        this->ptr_fonction = reinterpret_cast<Symbole::type_fonction>(ptr_symbole.ptr());
        etat_recherche = EtatRechercheSymbole::TROUVE;
    }
    catch (...) {
        espace->rapporte_erreur(site, "Impossible de trouver un symbole !")
            .ajoute_message("La bibliothèque « ",
                            bibliotheque->ident->nom,
                            " » ne possède pas le symbole « ",
                            nom,
                            " » !\n");
        etat_recherche = EtatRechercheSymbole::INTROUVE;
        return false;
    }

    return true;
}

Symbole *Bibliotheque::cree_symbole(kuri::chaine_statique nom_symbole)
{
    POUR_TABLEAU_PAGE (symboles) {
        if (it.nom == nom_symbole) {
            return &it;
        }
    }

    auto symbole = symboles.ajoute_element();
    symbole->nom = nom_symbole;
    symbole->bibliotheque = this;
    return symbole;
}

bool Bibliotheque::charge(EspaceDeTravail *espace)
{
    if (etat_recherche == EtatRechercheBibliotheque::TROUVEE) {
        return true;
    }

    kuri::chaine_statique chemin_dynamique =
        chemins[PLATEFORME_64_BIT][DYNAMIQUE][POUR_PRODUCTION];

    if (chemin_dynamique == "") {
        espace
            ->rapporte_erreur(
                site, "Impossible de charger une bibliothèque dynamique pour l'exécution du code")
            .ajoute_message("La bibliothèque « ", nom, " » n'a pas de version dynamique !\n");
        return false;
    }

    try {
        this->bib = dls::systeme_fichier::shared_library(
            dls::chaine(chemin_dynamique.pointeur(), chemin_dynamique.taille()).c_str());
        etat_recherche = EtatRechercheBibliotheque::TROUVEE;
    }
    catch (std::filesystem::filesystem_error const &e) {
        espace
            ->rapporte_erreur(site,
                              enchaine("Impossible de charger la bibliothèque « ", nom, " » !\n"))
            .ajoute_message("Message d'erreur : ", e.what());
        etat_recherche = EtatRechercheBibliotheque::INTROUVEE;
        return false;
    }
    catch (...) {
        espace->rapporte_erreur(
            site, enchaine("Impossible de charger la bibliothèque « ", nom, " » !\n"));
        etat_recherche = EtatRechercheBibliotheque::INTROUVEE;
        return false;
    }

    return true;
}

long Bibliotheque::memoire_utilisee() const
{
    auto memoire = symboles.memoire_utilisee();
    POUR_TABLEAU_PAGE (symboles) {
        memoire += it.nom.taille();
    }
    for (int i = 0; i < NUM_TYPES_PLATEFORME; i++) {
        for (int j = 0; j < NUM_TYPES_BIBLIOTHEQUE; j++) {
            for (int k = 0; k < NUM_TYPES_INFORMATION_BIBLIOTHEQUE; k++) {
                memoire += chemins[i][j][k].taille();
            }
        }
    }
    for (int k = 0; k < NUM_TYPES_INFORMATION_BIBLIOTHEQUE; k++) {
        memoire += noms[k].taille();
    }
    memoire += dependances.taille_memoire();
    return memoire;
}

static int plateforme_pour_options(OptionsDeCompilation const &options)
{
    if (options.architecture == ArchitectureCible::X86) {
        return PLATEFORME_32_BIT;
    }

    return PLATEFORME_64_BIT;
}

static int type_informations(kuri::chaine const *chemins, const OptionsDeCompilation &options)
{
    if (options.compilation_pour == CompilationPour::DEBOGAGE) {
        if (options.utilise_asan && chemins[POUR_DEBOGAGE_ASAN]) {
            return POUR_DEBOGAGE_ASAN;
        }

        if (chemins[POUR_DEBOGAGE]) {
            return POUR_DEBOGAGE;
        }
    }

    if (options.compilation_pour == CompilationPour::PROFILAGE) {
        if (chemins[POUR_PROFILAGE]) {
            return POUR_PROFILAGE;
        }
    }

    return POUR_PRODUCTION;
}

static kuri::chaine_statique selectionne_chemin_pour_options(kuri::chaine const *chemins,
                                                             const OptionsDeCompilation &options)
{
    return chemins[type_informations(chemins, options)];
}

kuri::chaine_statique Bibliotheque::chemin_de_base(const OptionsDeCompilation &options) const
{
    return chemins_de_base[plateforme_pour_options(options)];
}

kuri::chaine_statique Bibliotheque::chemin_statique(const OptionsDeCompilation &options) const
{
    auto const plateforme = plateforme_pour_options(options);
    auto chemins_statiques = chemins[plateforme][STATIQUE];
    return selectionne_chemin_pour_options(chemins_statiques, options);
}

kuri::chaine_statique Bibliotheque::chemin_dynamique(const OptionsDeCompilation &options) const
{
    auto const plateforme = plateforme_pour_options(options);
    auto chemins_dynamiques = chemins[plateforme][DYNAMIQUE];
    return selectionne_chemin_pour_options(chemins_dynamiques, options);
}

kuri::chaine_statique Bibliotheque::nom_pour_liaison(const OptionsDeCompilation &options) const
{
    // À FAIRE : statique vs dynamique
    auto const plateforme = plateforme_pour_options(options);
    auto chemins_dynamiques = chemins[plateforme][DYNAMIQUE];
    return noms[type_informations(chemins_dynamiques, options)];
}

Bibliotheque *GestionnaireBibliotheques::trouve_bibliotheque(IdentifiantCode *ident)
{
    POUR_TABLEAU_PAGE (bibliotheques) {
        if (it.ident == ident) {
            return &it;
        }
    }

    return nullptr;
}

Bibliotheque *GestionnaireBibliotheques::trouve_ou_cree_bibliotheque(EspaceDeTravail &espace,
                                                                     IdentifiantCode *ident)
{
    return cree_bibliotheque(espace, nullptr, ident, "");
}

Bibliotheque *GestionnaireBibliotheques::cree_bibliotheque(EspaceDeTravail &espace,
                                                           NoeudExpression *site)
{
    return cree_bibliotheque(espace, site, site->ident, "");
}

Bibliotheque *GestionnaireBibliotheques::cree_bibliotheque(EspaceDeTravail &espace,
                                                           NoeudExpression *site,
                                                           IdentifiantCode *ident,
                                                           kuri::chaine_statique nom)
{
    auto bibliotheque = trouve_bibliotheque(ident);

    if (bibliotheque) {
        if (nom != "" &&
            bibliotheque->etat_recherche == EtatRechercheBibliotheque::NON_RECHERCHEE) {
            bibliotheque->site = site;
            bibliotheque->ident = ident;
            bibliotheque->nom = nom;
            resoud_chemins_bibliotheque(espace, site, bibliotheque);
        }

        return bibliotheque;
    }

    bibliotheque = bibliotheques.ajoute_element();

    if (nom != "") {
        bibliotheque->nom = nom;
        resoud_chemins_bibliotheque(espace, site, bibliotheque);
    }

    bibliotheque->site = site;
    bibliotheque->ident = ident;
    return bibliotheque;
}

static bool est_fichier_elf(unsigned char tampon[4])
{
    /* .ELF */
    return tampon[0] == 0x7f && tampon[1] == 0x45 && tampon[2] == 0x4c && tampon[3] == 0x46;
}

static long taille_fichier(int fd)
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
static kuri::chaine resoud_chemin_dynamique_si_script_ld(EspaceDeTravail &espace,
                                                         NoeudExpression *site,
                                                         kuri::chaine const &chemin_dynamique)
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
    DIFFERE {
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

    auto chaine = dls::chaine();
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

    auto chemin_potentiel = chaine.sous_chaine(pos_premier_slash,
                                               pos_premiere_espace - pos_premier_slash);
    return kuri::chaine(chemin_potentiel.c_str(), chemin_potentiel.taille());
}

struct ResultatRechercheBibliotheque {
    kuri::chaine_statique chemin_de_base;
    kuri::chaine chemins[NUM_TYPES_BIBLIOTHEQUE][NUM_TYPES_INFORMATION_BIBLIOTHEQUE];
};

static std::optional<ResultatRechercheBibliotheque> recherche_bibliotheque(
    EspaceDeTravail &espace,
    NoeudExpression *site,
    kuri::tablet<kuri::chaine_statique, 4> const &dossiers,
    kuri::chaine const noms[NUM_TYPES_BIBLIOTHEQUE][NUM_TYPES_INFORMATION_BIBLIOTHEQUE])
{
    auto resultat = ResultatRechercheBibliotheque();

    bool chemin_trouve[NUM_TYPES_BIBLIOTHEQUE][NUM_TYPES_INFORMATION_BIBLIOTHEQUE];

    POUR (dossiers) {

        resultat.chemin_de_base = "";
        for (int i = 0; i < NUM_TYPES_BIBLIOTHEQUE; i++) {
            for (int j = 0; j < NUM_TYPES_INFORMATION_BIBLIOTHEQUE; j++) {
                chemin_trouve[i][j] = false;
                resultat.chemins[i][j] = "";
            }
        }

        for (int i = 0; i < NUM_TYPES_BIBLIOTHEQUE; i++) {
            for (int j = 0; j < NUM_TYPES_INFORMATION_BIBLIOTHEQUE; j++) {
                if (chemin_trouve[i][j]) {
                    continue;
                }

                auto chemin_test = enchaine(it, noms[i][j]);
                if (!fichier_existe(chemin_test)) {
                    continue;
                }

                chemin_trouve[i][j] = true;
                resultat.chemins[i][j] = chemin_test;
            }
        }
        /* Les bibliothèques doivent être dans le même dossier. */
        if (chemin_trouve[STATIQUE][POUR_PRODUCTION] ||
            chemin_trouve[DYNAMIQUE][POUR_PRODUCTION]) {
            resultat.chemin_de_base = it;
            break;
        }
    }

    if (chemin_trouve[DYNAMIQUE][POUR_PRODUCTION]) {
        resultat.chemins[DYNAMIQUE][POUR_PRODUCTION] = resoud_chemin_dynamique_si_script_ld(
            espace, site, resultat.chemins[DYNAMIQUE][POUR_PRODUCTION]);
    }

    if (chemin_trouve[STATIQUE][POUR_PRODUCTION] || chemin_trouve[DYNAMIQUE][POUR_PRODUCTION]) {
        return resultat;
    }

    return {};
}

static kuri::tablet<kuri::chaine_statique, 4> dossiers_recherche_32_bits(
    Compilatrice const &compilatrice, NoeudExpression *site)
{
    kuri::tablet<kuri::chaine_statique, 4> dossiers;
    if (site) {
        const auto fichier = compilatrice.fichier(site->lexeme->fichier);
        const auto module = fichier->module;
        dossiers.ajoute(module->chemin_bibliotheque_32bits);
    }

    dossiers.ajoute("/lib/i386-linux-gnu/");
    dossiers.ajoute("/usr/lib/i386-linux-gnu/");
    // pour les tables r16...
    dossiers.ajoute("/tmp/lib/i386-linux-gnu/");
    return dossiers;
}

static kuri::tablet<kuri::chaine_statique, 4> dossiers_recherche_64_bits(
    Compilatrice const &compilatrice, NoeudExpression *site)
{
    kuri::tablet<kuri::chaine_statique, 4> dossiers;
    if (site) {
        const auto fichier = compilatrice.fichier(site->lexeme->fichier);
        const auto module = fichier->module;
        dossiers.ajoute(module->chemin_bibliotheque_64bits);
    }

    dossiers.ajoute("/lib/x86_64-linux-gnu/");
    dossiers.ajoute("/usr/lib/x86_64-linux-gnu/");
    // pour les tables r16...
    dossiers.ajoute("/tmp/lib/x86_64-linux-gnu/");
    return dossiers;
}

static void copie_chemins(ResultatRechercheBibliotheque const &resultat,
                          Bibliotheque *bibliotheque,
                          int plateforme)
{
    for (int i = 0; i < NUM_TYPES_BIBLIOTHEQUE; i++) {
        for (int j = 0; j < NUM_TYPES_INFORMATION_BIBLIOTHEQUE; j++) {
            bibliotheque->chemins[plateforme][i][j] = resultat.chemins[i][j];
        }
    }

    bibliotheque->chemins_de_base[plateforme] = resultat.chemin_de_base;
}

void GestionnaireBibliotheques::resoud_chemins_bibliotheque(EspaceDeTravail &espace,
                                                            NoeudExpression *site,
                                                            Bibliotheque *bibliotheque)
{
    // regarde soit dans le module courant, soit dans le chemin système
    // chemin_système : /lib/x86_64-linux-gnu/ pour 64-bit
    //                  /lib/i386-linux-gnu/ pour 32-bit

    // essaye de déterminer le chemin
    // pour un fichier statique :
    // /chemin/de/base/libnom.a
    // pour un fichier dynamique :
    // /chemin/de/base/libnom.so
    kuri::chaine noms[NUM_TYPES_BIBLIOTHEQUE][NUM_TYPES_INFORMATION_BIBLIOTHEQUE];

    bibliotheque->noms[POUR_PRODUCTION] = bibliotheque->nom;
    bibliotheque->noms[POUR_PROFILAGE] = enchaine(bibliotheque->nom, "_profile");
    bibliotheque->noms[POUR_DEBOGAGE] = enchaine(bibliotheque->nom, "_debogage");
    bibliotheque->noms[POUR_DEBOGAGE_ASAN] = enchaine(bibliotheque->nom, "_asan");

    for (int i = 0; i < NUM_TYPES_INFORMATION_BIBLIOTHEQUE; i++) {
        noms[STATIQUE][i] = enchaine("lib", bibliotheque->noms[i], ".a");
        noms[DYNAMIQUE][i] = enchaine("lib", bibliotheque->noms[i], ".so");
    }

    /* Commence par les versions 64-bit. */
    auto dossiers = dossiers_recherche_64_bits(compilatrice, site);
    auto resultat = recherche_bibliotheque(espace, site, dossiers, noms);

    if (!resultat.has_value()) {
        auto e = espace.rapporte_erreur(site,
                                        "Impossible de résoudre le chemin vers une bibliothèque");
        e.ajoute_message("La bibliothèque en question est « ", bibliotheque->nom, " »\n\n");
        e.ajoute_message("Les chemins testés furent :\n");
        POUR (dossiers) {
            e.ajoute_message("    ", it, "/", noms[STATIQUE][POUR_PRODUCTION], "\n");
            e.ajoute_message("    ", it, "/", noms[DYNAMIQUE][POUR_PRODUCTION], "\n");
        }

        return;
    }

    copie_chemins(resultat.value(), bibliotheque, PLATEFORME_64_BIT);

    /* Versions 32-bit. */
    dossiers = dossiers_recherche_32_bits(compilatrice, site);
    resultat = recherche_bibliotheque(espace, site, dossiers, noms);

    if (resultat.has_value()) {
        /* Pas d'erreur si non trouvés pour le moment. */
        copie_chemins(resultat.value(), bibliotheque, PLATEFORME_32_BIT);
    }
}

long GestionnaireBibliotheques::memoire_utilisee() const
{
    auto memoire = bibliotheques.memoire_utilisee();
    POUR_TABLEAU_PAGE (bibliotheques) {
        memoire += it.memoire_utilisee();
    }
    return memoire;
}

void GestionnaireBibliotheques::rassemble_statistiques(Statistiques &stats) const
{
    stats.memoire_bibliotheques += memoire_utilisee();
}
