#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <limits>

int distance_levenshtein(const std::string &chaine1, const std::string &chaine2)
{
    const size_t longueur_chaine1 = chaine1.size();
    const size_t longueur_chaine2 = chaine1.size();

    /* d est tableau de (longueur_chaine1 + 1) rangées et
     * (longueur_chaine2 + 1) colonnes */
    std::vector<int> d((longueur_chaine1 + 1) * (longueur_chaine2 + 1));
    const size_t decalage = (longueur_chaine2 + 1);

    for (size_t i = 0; i < longueur_chaine1; ++i) {
        d[i * decalage] = i;
    }

    for (size_t j = 0; j < longueur_chaine2; ++j) {
        d[j] = j;
    }

    for (size_t i = 1; i <= longueur_chaine1; ++i) {
        for (size_t j = 1; j <= longueur_chaine2; ++j) {
            const auto cout = (chaine1[i - 1] != chaine2[j - 1]);

            const auto effacement   = d[(i - 1) * decalage + j] + 1;
            const auto insertion    = d[(i    ) * decalage + j - 1] + 1;
            const auto substitution = d[(i - 1) * decalage + j - 1] + cout;

            d[i * decalage + j] = std::min(effacement, std::min(insertion, substitution));
        }
    }

    for (size_t i = 0; i < longueur_chaine1; ++i) {
        for (size_t j = 0; j < longueur_chaine2; ++j) {
            std::cerr << d[i * decalage + j] << ' ';
        }

        std::cerr << '\n';
    }

    return d[longueur_chaine1 * decalage + longueur_chaine2];
}

int distance_levenshtein2(const std::string &chaine1, const std::string &chaine2)
{
    const size_t m = chaine1.size();
    const size_t n = chaine1.size();

    std::vector<int> v0(n + 1);
    std::vector<int> v1(n + 1);

    for (size_t i = 0; i < n + 1; ++i) {
        v0[i] = i;
    }

    for (size_t i = 0; i < m; ++i) {
        v1[0] = i + 1;

        for (size_t j = 0; j < n; ++j) {
            auto cout_suppression  = v0[j + 1] + 1;
            auto cout_insertion    = v1[j] + 1;
            auto cout_substitution = (chaine1[i] == chaine2[j]) ? v0[j] : v0[j] + 1;

            v1[j + 1] = std::min(cout_suppression, std::min(cout_insertion, cout_substitution));
        }

        std::swap(v0, v1);
    }

    return v0[n];
}

int main()
{
    std::set<std::string> membres;
    membres.insert("nom");
    membres.insert("prénom");
    membres.insert("âge");
    membres.insert("sexe");

    const std::string &recherche = "age";

    if (membres.find(recherche) == membres.end()) {
        std::cerr << "Impossible de trouver '" << recherche << "'\n";
        auto dist_min = std::numeric_limits<int>::max();
        std::string candidat;

        for (const auto &m : membres) {
            auto dist = distance_levenshtein2(m, recherche);

            if (dist < dist_min && dist != m.size()) {
                dist_min = dist;
                candidat = m;
            }
        }

        std::cerr << "Candidat : '" << candidat << "'\n";
    }

    //std::cerr << "Levenstein \n" << distance_levenshtein2("âge", "prénom") << '\n';
}
