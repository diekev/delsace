/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include <fstream>
#include <iostream>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"
#include "structures/table_hachage.hh"

#include "structures/chemin_systeme.hh"

template <typename... Ts>
static kuri::chaine imprime_commande_systeme(Ts &&...ts)
{
    Enchaineuse enchaineuse;
    ((enchaineuse << ts) << ...);
    enchaineuse << '\0';
    return enchaineuse.chaine();
}

static bool exécute_commande_systeme(kuri::chaine_statique commande)
{
    return system(commande.pointeur()) == 0;
}

static std::atomic_bool compilation_terminee{false};

struct DonneesCommandeKuri {
    kuri::chaine_statique fichier{};
    kuri::chaine_statique nom_fichier_fichier_utilises{};
};

static void lance_application(DonneesCommandeKuri *donnees)
{
    compilation_terminee = false;

    auto commande_kuri = enchaine("kuri ",
                                  donnees->fichier,
                                  " --emets_fichiers_utilises ",
                                  donnees->nom_fichier_fichier_utilises,
                                  '\0');

    if (!exécute_commande_systeme(commande_kuri)) {
        compilation_terminee = true;
        std::cerr << "Erreur lors de la compilation !\n";
        return;
    }

    compilation_terminee = true;

    commande_kuri = enchaine("./a.out", '\0');

    if (!exécute_commande_systeme(commande_kuri)) {
        std::cerr << "Erreur lors du lancement de l'exécutable (peut avoir été interrompu) !\n";
        return;
    }
}

static bool valide_fichier_kuri(kuri::chemin_systeme const &chemin, bool verbeux)
{
    if (!kuri::chemin_systeme::existe(chemin)) {
        if (verbeux) {
            std::cerr << "Erreur : fichier " << chemin << " inexistant !\n";
        }
        return false;
    }

    if (!kuri::chemin_systeme::est_fichier_regulier(chemin)) {
        if (verbeux) {
            std::cerr << "Erreur : le chemin ne pointe pas vers un fichier régulier !\n";
        }
        return false;
    }

    const auto &extension = chemin.extension();

    if (extension != ".kuri") {
        if (verbeux) {
            std::cerr << "Erreur : le fichier n'est pas un fichier .kuri !\n";
        }
        return false;
    }

    return true;
}

static bool lis_tout(int fd)
{
    std::cerr << __func__ << '\n';

    auto octets_totaux = 0ll;

    while (true) {
        char *tampon[1024];
        auto octets_lus = read(fd, tampon, 1024);
        std::cerr << "octets_lus : " << octets_lus << '\n';

        if (octets_lus == -1) {
            perror("read");
            return false;
        }

        if (octets_lus == 0) {
            break;
        }

        if (octets_lus < 1024) {
            break;
        }

        octets_totaux += octets_lus;
    }

    // auto nombre_d_evenements = octets_totaux / sizeof(inotify_event);
    // std::cerr << "Nombre d'évènements : " << nombre_d_evenements << '\n';

    return true;
}

class Guetteuse {
    kuri::table_hachage<int, kuri::chaine> table_chemin_fichiers{"Chemins fichiers"};
    kuri::table_hachage<kuri::chaine, int> table_desc_fichiers{"Descripteurs fichiers"};

  public:
    void ajoute(int fd, const kuri::chemin_systeme &chemin)
    {
        if (possède(fd)) {
            return;
        }

        // std::cerr << "Ajout du fichier " << chemin << '\n';

        table_chemin_fichiers.insère(fd, kuri::chaine(chemin));
        table_desc_fichiers.insère(kuri::chaine(chemin), fd);
    }

    bool possède(const kuri::chemin_systeme &chemin)
    {
        return table_desc_fichiers.possède(kuri::chaine(chemin));
    }

    bool possède(int fd)
    {
        return table_chemin_fichiers.possède(fd);
    }
};

static void ajoute_a_guetteuse(int fd, Guetteuse &guetteuse, kuri::chemin_systeme const &chemin)
{
    const auto fd_guetteuse = inotify_add_watch(fd, vers_std_path(chemin).c_str(), IN_MODIFY);

    if (fd_guetteuse == -1) {
        perror("inotify_add_watch");
    }
    else {
        guetteuse.ajoute(fd_guetteuse, chemin);
    }
}

static void initialise_liste_fichiers(int fd, Guetteuse &guetteuse)
{
    auto chemin = kuri::chemin_systeme::chemin_courant();
    auto fichiers = kuri::chemin_systeme::fichiers_du_dossier(chemin);

    POUR (fichiers) {
        ajoute_a_guetteuse(fd, guetteuse, it);
    }
}

static void rafraichis_liste_fichiers_utilises(int fd,
                                               Guetteuse &guetteuse,
                                               kuri::chemin_systeme const &chemin_fichier)
{
    while (!compilation_terminee) {
        continue;
    }

    if (!kuri::chemin_systeme::existe(chemin_fichier)) {
        std::cerr << "Erreur : le fichier de fichiers utilisés n'existe pas !\n";
        exit(1);
        return;
    }

    std::ifstream ffu(vers_std_path(chemin_fichier));

    std::string ligne;
    while (std::getline(ffu, ligne)) {
        auto chemin = kuri::chemin_systeme(ligne.c_str());

        if (!valide_fichier_kuri(chemin, false)) {
            continue;
        }

        if (guetteuse.possède(chemin)) {
            continue;
        }

        ajoute_a_guetteuse(fd, guetteuse, chemin);
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Utilisation: " << argv[0] << " fichier;\n";
        return 1;
    }

    if (!valide_fichier_kuri(kuri::chaine_statique(argv[1]), true)) {
        return 1;
    }

    auto fichier_racine_compilation = kuri::chaine_statique(argv[1]);
    /* Utilise un pointeur pour rendre le fichier unique. */
    auto nom_base_fichier_fichiers_utilises = enchaine(&fichier_racine_compilation);
    auto nom_fichier_fichier_utilises = kuri::chemin_systeme::chemin_temporaire(
        nom_base_fichier_fichiers_utilises);

    auto donnees_commande_kuri = DonneesCommandeKuri{fichier_racine_compilation,
                                                     nom_fichier_fichier_utilises};

    auto crée_exetron = [&]() {
        return new std::thread(lance_application, &donnees_commande_kuri);
    };

    auto exetron = crée_exetron();

    // std::cerr << "Initialisation de inotify...\n";
    const auto fd = inotify_init();

    if (fd == -1) {
        // std::cerr << "Échec lors de l'initialisation de inotify\n";
        return 1;
    }

    auto guetteuse = Guetteuse{};

    // std::cerr << "Fd " << fd << '\n';

    while (true) {
        rafraichis_liste_fichiers_utilises(fd, guetteuse, nom_fichier_fichier_utilises);

        const auto succes = lis_tout(fd);

        exécute_commande_systeme("pkill a.out");
        exetron->join();
        delete exetron;

        if (!succes) {
            break;
        }

        exetron = crée_exetron();
    }

    close(fd);
    return 0;
}
