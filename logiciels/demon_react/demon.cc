#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <sys/inotify.h>
#include <thread>
#include <unistd.h>

template <typename... Ts>
static std::string imprime_commande_systeme(Ts &&... ts)
{
    std::stringstream enchaineuse;
    ((enchaineuse << ts), ...);
    return enchaineuse.str();
}

template <typename... Ts>
static std::string enchaine(Ts &&... ts)
{
    std::stringstream enchaineuse;
    ((enchaineuse << ts), ...);
    return enchaineuse.str();
}

static std::string chaine_depuis_path(const std::filesystem::path &chemin)
{
    return chemin.c_str();
}

static std::filesystem::path path_depuis_chaine(const std::string &c)
{
    return c;
}

static bool execute_commande_systeme(std::string_view commande)
{
    std::cout << "Exécution de la commande : " << commande << '\n';
    return system(&commande[0]) == 0;
}

static std::atomic_bool compilation_terminee{false};

struct DonneesCommande {
    std::string_view fichier_entree{};
    std::string_view fichier_sortie{};
};

static void lance_application(DonneesCommande *donnees)
{
    compilation_terminee = false;

    // browserify FICHIER_ENTREE -t babelify --outfile FICHIER_SORTIE
    // en mode production :
    // NODE_ENV=production browserify -g envify -e FICHIER_ENTREE -t babelify --outfile
    // FICHIER_SORTIE
    auto commande_kuri = enchaine("browserify ",
                                  donnees->fichier_entree,
                                  " -t babelify ",
                                  " --outfile ",
                                  donnees->fichier_sortie);

    if (!execute_commande_systeme(commande_kuri)) {
        compilation_terminee = true;
        std::cerr << "Erreur lors de la compilation !\n";
        return;
    }

    compilation_terminee = true;
}

static bool lis_tout(int fd)
{
    // std::cerr << __func__ << '\n';

    auto octets_totaux = 0ll;

    while (true) {
        char *tampon[1024];
        auto octets_lus = read(fd, tampon, 1024);
        // std::cerr << "octets_lus : " << octets_lus << '\n';

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
    std::map<int, std::string> table_chemin_fichiers{};
    std::map<std::string, int> table_desc_fichiers{};

  public:
    void ajoute(int fd, const std::filesystem::path &chemin)
    {
        if (possede(fd)) {
            return;
        }

        // std::cerr << "Ajout du fichier " << chemin << '\n';

        table_chemin_fichiers.insert({fd, chaine_depuis_path(chemin)});
        table_desc_fichiers.insert({chaine_depuis_path(chemin), fd});
    }

    bool possede(const std::filesystem::path &chemin) const
    {
        return table_desc_fichiers.find(chaine_depuis_path(chemin)) != table_desc_fichiers.end();
    }

    bool possede(int fd) const
    {
        return table_chemin_fichiers.find(fd) != table_chemin_fichiers.end();
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

static bool doit_guetter_fichier(std::filesystem::path const &chemin)
{
    if (!std::filesystem::is_regular_file(chemin)) {
        return false;
    }

    auto extension = chemin.extension();

    if (extension != ".js" && extension != ".jsx") {
        return false;
    }

    return true;
}

static void rafraichis_liste_fichiers_utilises(int fd,
                                               Guetteuse &guetteuse,
                                               std::filesystem::path const &dossier_parent)
{
    while (!compilation_terminee) {
        continue;
    }

    for (auto &iter : std::filesystem::recursive_directory_iterator(dossier_parent)) {
        auto chemin = iter.path();

        if (!doit_guetter_fichier(chemin)) {
            continue;
        }

        auto chemin_absolu = std::filesystem::absolute(chemin);

        const auto fd_guetteuse = inotify_add_watch(fd, chemin_absolu.c_str(), IN_MODIFY);
        if (fd_guetteuse == -1) {
            perror("inotify_add_watch");
        }
        else {
            guetteuse.ajoute(fd_guetteuse, chemin);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::cerr << "Utilisation: " << argv[0] << " FICHIER_ENTREE FICHIER_SORTIE;\n";
        return 1;
    }

    auto fichier_entree = std::string_view(argv[1]);
    auto fichier_sortie = std::string_view(argv[2]);
    auto chemin_fichier_entree = std::filesystem::path(fichier_entree);
    auto dossier_parent = chemin_fichier_entree.parent_path();

    auto donnees_commande_kuri = DonneesCommande{fichier_entree, fichier_sortie};

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

    while (true) {
        rafraichis_liste_fichiers_utilises(fd, guetteuse, dossier_parent);

        const auto succes = lis_tout(fd);

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
