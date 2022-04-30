#include <filesystem>
#include <fstream>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"
#include "structures/table_hachage.hh"

template <typename... Ts>
static kuri::chaine imprime_commande_systeme(Ts &&...ts)
{
    Enchaineuse enchaineuse;
    ((enchaineuse << ts) << ...);
    enchaineuse << '\0';
    return enchaineuse.chaine();
}

static kuri::chaine chaine_depuis_path(const std::filesystem::path &chemin)
{
    return kuri::chaine(chemin.c_str());
}

static std::filesystem::path path_depuis_chaine(const kuri::chaine &c)
{
    return {std::string(c.pointeur(), static_cast<size_t>(c.taille()))};
}

static bool execute_commande_systeme(kuri::chaine_statique commande)
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

    if (!execute_commande_systeme(commande_kuri)) {
        compilation_terminee = true;
        std::cerr << "Erreur lors de la compilation !\n";
        return;
    }

    compilation_terminee = true;

    commande_kuri = enchaine("./a.out", '\0');

    if (!execute_commande_systeme(commande_kuri)) {
        std::cerr << "Erreur lors du lancement de l'exécutable (peut avoir été interrompu) !\n";
        return;
    }
}

static bool valide_fichier_kuri(std::filesystem::path chemin, bool verbeux)
{
    if (!std::filesystem::exists(chemin)) {
        if (verbeux) {
            std::cerr << "Erreur : fichier " << chemin << " inexistant !\n";
        }
        return false;
    }

    if (!std::filesystem::is_regular_file(chemin)) {
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
    void ajoute(int fd, const std::filesystem::path &chemin)
    {
        if (possede(fd)) {
            return;
        }

        // std::cerr << "Ajout du fichier " << chemin << '\n';

        table_chemin_fichiers.insere(fd, chaine_depuis_path(chemin));
        table_desc_fichiers.insere(chaine_depuis_path(chemin), fd);
    }

    bool possede(const std::filesystem::path &chemin)
    {
        return table_desc_fichiers.possede(chaine_depuis_path(chemin));
    }

    bool possede(int fd)
    {
        return table_chemin_fichiers.possede(fd);
    }
};

static void initialise_liste_fichiers(int fd, Guetteuse &guetteuse)
{
    auto chemin = std::filesystem::current_path();

    for (auto entry : std::filesystem::directory_iterator(chemin)) {
        const auto &chemin_entry = entry.path();

        if (!std::filesystem::is_regular_file(chemin_entry)) {
            continue;
        }

        const auto &extension = chemin_entry.extension();

        if (extension != ".kuri") {
            continue;
        }

        const auto fd_guetteuse = inotify_add_watch(fd, chemin_entry.c_str(), IN_MODIFY);

        if (fd_guetteuse == -1) {
            perror("inotify_add_watch");
        }
        else {
            guetteuse.ajoute(fd_guetteuse, chemin_entry);
        }
    }
}

static void rafraichis_liste_fichiers_utilises(int fd,
                                               Guetteuse &guetteuse,
                                               kuri::chaine const &chemin_fichier)
{
    while (!compilation_terminee) {
        continue;
    }

    if (!std::filesystem::exists(path_depuis_chaine(chemin_fichier))) {
        std::cerr << "Erreur : le fichier de fichiers utilisés n'existe pas !\n";
        exit(1);
        return;
    }

    std::ifstream ffu(path_depuis_chaine(chemin_fichier));

    std::string ligne;
    while (std::getline(ffu, ligne)) {
        auto chemin = std::filesystem::path(ligne);

        if (!valide_fichier_kuri(chemin, false)) {
            continue;
        }

        if (guetteuse.possede(chemin)) {
            continue;
        }

        const auto fd_guetteuse = inotify_add_watch(fd, chemin.c_str(), IN_MODIFY);

        if (fd_guetteuse == -1) {
            // perror("inotify_add_watch");
        }
        else {
            guetteuse.ajoute(fd_guetteuse, chemin);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::cerr << "Utilisation: " << argv[0] << " fichier;\n";
        return 1;
    }

    if (!valide_fichier_kuri(argv[1], true)) {
        return 1;
    }

    auto fichier_racine_compilation = kuri::chaine_statique(argv[1]);
    auto nom_fichier_fichier_utilises = enchaine("/tmp/", &fichier_racine_compilation);

    auto donnees_commande_kuri = DonneesCommandeKuri{fichier_racine_compilation,
                                                     nom_fichier_fichier_utilises};

    auto cree_exetron = [&]() {
        return new std::thread(lance_application, &donnees_commande_kuri);
    };

    auto exetron = cree_exetron();

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

        execute_commande_systeme("pkill a.out");
        exetron->join();
        delete exetron;

        if (!succes) {
            break;
        }

        exetron = cree_exetron();
    }

    close(fd);
    return 0;
}
