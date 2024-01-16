/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "import_objet.h"

#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

#include "biblinternes/structures/tableau.hh"

#include "outils.hh"

static kuri::tableau<std::string> morcelle(std::string const &texte, char const delimiteur)
{
    kuri::tableau<std::string> morceaux;
    std::string morceau;
    std::stringstream ss(texte.c_str());

    while (std::getline(ss, morceau, delimiteur)) {
        if (morceau.empty()) {
            continue;
        }

        morceaux.ajoute(morceau);
    }

    return morceaux;
}

namespace geo {

struct GroupePrimitives {
    std::string nom = "";
    kuri::tableau<int> index{};
};

struct DonneesLectureOBJ {
    kuri::tableau<math::vec3f> points{};
    kuri::tableau<math::vec3f> normaux{};
    kuri::tableau<math::vec3f> parametres_point{};
    kuri::tableau<math::vec3f> coordonnees_texture_point{};

    kuri::tableau<kuri::tableau<int>> polygones{};
    kuri::tableau<kuri::tableau<int>> normaux_polygones{};
    kuri::tableau<kuri::tableau<int>> uv_polygones{};

    std::list<GroupePrimitives> groupes{};
    std::list<GroupePrimitives *> groupes_courant{};

    GroupePrimitives *cree_groupe_si_non_existant(const std::string &nom)
    {
        for (auto &groupe : groupes) {
            if (groupe.nom == nom) {
                return &groupe;
            }
        }

        GroupePrimitives groupe;
        groupe.nom = nom;
        groupes.push_back(groupe);
        return &groupes.back();
    }
};

static void lis_normal_sommet(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    math::vec3f n;
    is >> n.x >> n.y >> n.z;
    donnees.normaux.ajoute(n);
}

static void lis_parametres_sommet(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    math::vec3f n;
    is >> n.x >> n.y >> n.z;
    donnees.parametres_point.ajoute(n);
}

static void lis_coord_texture_sommet(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    math::vec3f n;
    is >> n.x >> n.y >> n.z;
    donnees.coordonnees_texture_point.ajoute(n);
}

static void lis_sommet(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    math::vec3f n;
    is >> n.x >> n.y >> n.z;
    donnees.points.ajoute(n);
}

static void lis_polygone(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    std::string info_poly;
    kuri::tableau<int> index_polygones;
    kuri::tableau<int> index_normaux;
    kuri::tableau<int> index_coords_uv;

    while (is >> info_poly) {
        auto morceaux = morcelle(info_poly, '/');

        switch (morceaux.taille()) {
            case 1:
            {
                index_polygones.ajoute(std::stoi(morceaux[0].c_str()) - 1);
                break;
            }
            case 2:
            {
                index_polygones.ajoute(std::stoi(morceaux[0].c_str()) - 1);
                index_coords_uv.ajoute(std::stoi(morceaux[1].c_str()) - 1);
                break;
            }
            case 3:
            {
                index_polygones.ajoute(std::stoi(morceaux[0].c_str()) - 1);

                if (!morceaux[1].empty()) {
                    index_coords_uv.ajoute(std::stoi(morceaux[1].c_str()) - 1);
                }

                index_normaux.ajoute(std::stoi(morceaux[2].c_str()) - 1);
                break;
            }
        }
    }

    int64_t index_polygone = donnees.polygones.taille();

    donnees.polygones.ajoute(index_polygones);

    if (!index_normaux.est_vide()) {
        donnees.normaux_polygones.ajoute(index_normaux);
    }

    if (!index_coords_uv.est_vide()) {
        donnees.uv_polygones.ajoute(index_coords_uv);
    }

    for (auto groupe : donnees.groupes_courant) {
        groupe->index.ajoute(static_cast<int>(index_polygone));
    }
}

static void lis_objet(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    std::string nom_objet;
    is >> nom_objet;

    // maillage.ajoute_objet(nom_objet);
    // À FAIRE : non supporté pour le moment
}

static void lis_ligne(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    std::string info_poly;
    kuri::tableau<int> index;

    while (is >> info_poly) {
        index.ajoute(std::stoi(info_poly.c_str()) - 1);
    }

    // maillage.ajoute_ligne(index.donnees(), static_cast<size_t>(index.taille()));
    // À FAIRE : non supporté pour le moment
}

static void lis_groupes_geometries(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    std::string groupe;
    kuri::tableau<std::string> groupes;

    while (is >> groupe) {
        groupes.ajoute(groupe);
    }

    donnees.groupes_courant.clear();

    for (auto &groupe_ : groupes) {
        donnees.groupes_courant.push_back(donnees.cree_groupe_si_non_existant(groupe_));
    }
}

static void lis_groupe_nuancage(DonneesLectureOBJ &donnees, std::istringstream &is)
{
    std::string groupe;
    is >> groupe;

    if (groupe == "off") {
        return;
    }

    // int index = std::stoi(groupe.c_str());

    // maillage.groupe_nuancage(index);
    // À FAIRE : non supporté pour le moment
}

struct ParsatTableauIndexSurPolygone {
    bool valide = true;
    bool donnees_variantes_sur_polygone = false;
    int nombre_de_sommets = 0;
    int index_max = 0;
};

static ParsatTableauIndexSurPolygone parse_tableau_index(
    const kuri::tableau<kuri::tableau<int>> &valeurs)
{
    auto résultat = ParsatTableauIndexSurPolygone();

    for (const auto &tableau : valeurs) {
        if (tableau.est_vide()) {
            résultat.valide = false;
            return résultat;
        }

        résultat.nombre_de_sommets += static_cast<int>(tableau.taille());

        int premier_index = tableau[0];
        for (const auto &index : tableau) {
            résultat.index_max = std::max(résultat.index_max, index);

            if (premier_index != index) {
                résultat.donnees_variantes_sur_polygone = true;
            }
        }
    }

    return résultat;
}

void charge_fichier_OBJ(Maillage &maillage, std::string const &chemin)
{
    std::ifstream ifs;
    ifs.open(chemin.c_str());

    if (!ifs.is_open()) {
        return;
    }

    DonneesLectureOBJ donnees;

    std::string ligne;
    std::string entete;

    while (std::getline(ifs, ligne)) {
        std::istringstream is(ligne);
        is >> entete;

        if (entete == "vn") {
            lis_normal_sommet(donnees, is);
        }
        else if (entete == "vt") {
            lis_coord_texture_sommet(donnees, is);
        }
        else if (entete == "vp") {
            lis_parametres_sommet(donnees, is);
        }
        else if (entete == "v") {
            lis_sommet(donnees, is);
        }
        else if (ligne[0] == 'f') {
            lis_polygone(donnees, is);
        }
        else if (ligne[0] == 'l') {
            lis_ligne(donnees, is);
        }
        else if (entete == "o") {
            lis_objet(donnees, is);
        }
        else if (entete == "#") {
            continue;
        }
        else if (entete == "g") {
            lis_groupes_geometries(donnees, is);
        }
        else if (entete == "s") {
            lis_groupe_nuancage(donnees, is);
        }
        else if (entete == "mttlib") {
            /* À FAIRE */
        }
        else if (entete == "newmtl") {
            /* À FAIRE */
        }
        else if (entete == "usemtl") {
            /* À FAIRE */
        }
    }

    maillage.reserveNombreDePoints(donnees.points.taille());
    maillage.ajoutePoints(reinterpret_cast<float *>(donnees.points.données()),
                          donnees.points.taille());

    auto parsat_polygones = parse_tableau_index(donnees.polygones);
    auto parsat_normaux_polygones = parse_tableau_index(donnees.normaux_polygones);
    // auto parsat_uv_polygones = parse_tableau_index(donnees.uv_polygones);

    if (parsat_polygones.valide && parsat_polygones.donnees_variantes_sur_polygone &&
        !donnees.polygones.est_vide()) {
        if (parsat_polygones.index_max + 1 != donnees.points.taille()) {
            return;
        }

        maillage.reserveNombreDePolygones(donnees.polygones.taille());
        for (const auto &polygone : donnees.polygones) {
            maillage.ajouteUnPolygone(polygone.données(), static_cast<int>(polygone.taille()));
        }

        /* Exporte les groupes. */
        for (auto &groupe : donnees.groupes) {
            auto groupe_polygone = maillage.creeUnGroupeDePolygones(groupe.nom);

            if (groupe_polygone) {
                for (auto &index_poly : groupe.index) {
                    maillage.ajouteAuGroupe(groupe_polygone, index_poly);
                }
            }
        }
    }

    if (!donnees.normaux.est_vide()) {
        if (parsat_normaux_polygones.valide &&
            parsat_normaux_polygones.nombre_de_sommets == parsat_polygones.nombre_de_sommets &&
            donnees.normaux.taille() == parsat_normaux_polygones.index_max + 1) {
            auto attr = maillage.ajouteAttributSommetsPolygone<VEC3>("N");
            if (attr) {
                attr.copie(donnees.normaux);
            }
        }
        else if (parsat_normaux_polygones.valide &&
                 !parsat_normaux_polygones.donnees_variantes_sur_polygone &&
                 donnees.normaux.taille() == donnees.polygones.taille()) {
            auto attr = maillage.ajouteAttributPolygone<VEC3>("N");
            if (attr) {
                attr.copie(donnees.normaux);
            }
        }
        else if (donnees.normaux.taille() == donnees.points.taille()) {
            auto attr = maillage.ajouteAttributPoint<VEC3>("N");
            if (attr) {
                attr.copie(donnees.normaux);
            }
        }
        else {
            // À FAIRE : rapporte erreur
        }
    }
}

/* ************************************************************************** */

static bool est_entete_ascii(uint8_t *entete)
{
    return entete[0] == 's' && entete[1] == 'o' && entete[2] == 'l' && entete[3] == 'i' &&
           entete[4] == 'd';
}

static void charge_STL_ascii(Maillage &maillage, std::ifstream &fichier)
{
    /* lis la première ligne, doit être 'solid nom' */
    std::string tampon;
    std::getline(fichier, tampon);

    std::string mot;

    kuri::tableau<math::vec3f> tous_les_normaux;
    kuri::tableau<math::vec3f> tous_les_points;
    kuri::tableau<math::vec3i> tous_les_triangles;

    math::vec3i triangle(0, 1, 2);

    while (std::getline(fichier, tampon)) {
        std::istringstream is(tampon);
        is >> mot;

        if (mot == "facet") {
            is >> mot;

            if (mot != "normal") {
                std::cerr << "Attendu le mot 'normal' après 'facet' !\n";
                break;
            }

            math::vec3f n;
            is >> n.x >> n.y >> n.z;
            tous_les_normaux.ajoute(n);
        }
        else if (mot == "outer") {
            is >> mot;

            if (mot != "loop") {
                std::cerr << "Attendu le mot 'loop' après 'outer' !\n";
                break;
            }
        }
        else if (mot == "vertex") {
            math::vec3f v;
            is >> v.x >> v.y >> v.z;
            tous_les_points.ajoute(v);
        }
        else if (mot == "endloop") {
            /* R-À-F */
        }
        else if (mot == "endfacet") {
            tous_les_triangles.ajoute(triangle);

            for (int i = 0; i < 3; ++i) {
                triangle[static_cast<size_t>(i)] += 3;
            }
        }
        else if (mot == "endsolid") {
            /* R-À-F */
        }
        else {
            std::cerr << "Mot clé '" << mot << "' inattendu !\n";
            break;
        }
    }

    maillage.ajoutePoints(reinterpret_cast<float *>(tous_les_points.données()),
                          tous_les_points.taille());

    kuri::tableau<int> sommets_par_polygones(tous_les_triangles.taille());
    std::fill(sommets_par_polygones.debut(), sommets_par_polygones.fin(), 3);

    maillage.ajouteListePolygones(reinterpret_cast<int *>(tous_les_triangles.données()),
                                  sommets_par_polygones.données(),
                                  sommets_par_polygones.taille());

    if (tous_les_normaux.taille() == tous_les_triangles.taille()) {
        auto attr_normaux = maillage.ajouteAttributPolygone<VEC3>("N");
        attr_normaux.copie(tous_les_normaux);
    }
}

void ecris_fichier_OBJ(Maillage const &maillage, std::string const &chemin)
{
    std::ofstream fichier(chemin);

    if (!fichier.is_open()) {
        return;
    }

    for (int64_t i = 0; i < maillage.nombreDePoints(); ++i) {
        auto point = maillage.pointPourIndex(i);
        fichier << "v " << point.x << " " << point.y << " " << point.z << "\n";
    }

    kuri::tableau<int> temp_access_index_sommet;
    for (int64_t i = 0; i < maillage.nombreDePolygones(); ++i) {
        const int64_t nombre_sommets = maillage.nombreDeSommetsPolygone(i);

        if (nombre_sommets == 0) {
            continue;
        }

        temp_access_index_sommet.redimensionne(nombre_sommets);
        maillage.indexPointsSommetsPolygone(i, temp_access_index_sommet.données());

        fichier << "f";
        for (int64_t j = 0; j < nombre_sommets; ++j) {
            fichier << " " << (temp_access_index_sommet[j] + 1);
        }
        fichier << "\n";
    }
}

static void charge_STL_binaire(Maillage &maillage, std::ifstream &fichier)
{
    /* lis l'en-tête */
    uint8_t entete[80];
    fichier.read(reinterpret_cast<char *>(entete), 80);

    /* lis nombre triangle */
    uint32_t nombre_triangles;
    fichier.read(reinterpret_cast<char *>(&nombre_triangles), sizeof(uint32_t));

    if (nombre_triangles == 0) {
        return;
    }

    maillage.reserveNombreDePolygones(nombre_triangles);
    maillage.reserveNombreDePoints(nombre_triangles * 3);

    auto attr_normaux = maillage.ajouteAttributPolygone<VEC3>("N");

    /* lis triangles */
    math::vec3f nor;
    math::vec3f v0;
    math::vec3f v1;
    math::vec3f v2;
    uint16_t mot_controle;

    int sommets[3] = {0, 1, 2};

    for (uint32_t i = 0; i < nombre_triangles; ++i) {
        fichier.read(reinterpret_cast<char *>(&nor[0]), sizeof(float) * 3);
        fichier.read(reinterpret_cast<char *>(&v0[0]), sizeof(float) * 3);
        fichier.read(reinterpret_cast<char *>(&v1[0]), sizeof(float) * 3);
        fichier.read(reinterpret_cast<char *>(&v2[0]), sizeof(float) * 3);
        fichier.read(reinterpret_cast<char *>(&mot_controle), sizeof(uint16_t));

        maillage.ajouteUnPoint(v0[0], v0[1], v0[2]);
        maillage.ajouteUnPoint(v1[0], v1[1], v1[2]);
        maillage.ajouteUnPoint(v2[0], v2[1], v2[2]);

        maillage.ajouteUnPolygone(sommets, 3);
        attr_normaux.ecris_vec3(i, nor);

        for (uint32_t e = 0; e < 3; ++e) {
            sommets[e] += 3;
        }
    }
}

void charge_fichier_STL(Maillage &maillage, std::string const &chemin)
{
    std::ifstream fichier;
    fichier.open(chemin.c_str());

    if (!fichier.is_open()) {
        return;
    }

    /* lis en-tête */
    uint8_t entete[5];
    fichier.read(reinterpret_cast<char *>(entete), 5);

    /* rembobine */
    fichier.seekg(0);

    if (est_entete_ascii(entete)) {
        charge_STL_ascii(maillage, fichier);
    }
    else {
        charge_STL_binaire(maillage, fichier);
    }
}

}  // namespace geo
