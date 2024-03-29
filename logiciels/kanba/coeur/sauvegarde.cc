/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "sauvegarde.hh"

#include <fstream>

#include "brosse.h"
#include "kanba.h"
#include "maillage.h"

namespace KNB {

/* ------------------------------------------------------------------------- */
/** \name Écriture
 * \{ */

template <typename T>
void ecris_fichier(std::ofstream &fichier, T valeur)
{
    fichier.write(reinterpret_cast<char *>(&valeur), sizeof(T));
}

void ecris_fichier(std::ofstream &fichier, dls::chaine const &valeur)
{
    ecris_fichier(fichier, valeur.taille());
    fichier.write(valeur.c_str(), valeur.taille());
}

template <typename T>
void lis_fichier(std::ifstream &fichier, T &valeur)
{
    fichier.read(reinterpret_cast<char *>(&valeur), sizeof(T));
}

template <typename T>
static T lis_valeur(std::ifstream &fichier)
{
    T résultat;
    fichier.read(reinterpret_cast<char *>(&résultat), sizeof(T));
    return résultat;
}

void lis_fichier(std::ifstream &fichier, dls::chaine &valeur)
{
    long taille;
    lis_fichier(fichier, taille);

    valeur.redimensionne(taille);

    fichier.read(&valeur[0], taille);
}

static bool lis_nombre_magic(std::ifstream &fichier)
{
    char c;

    lis_fichier(fichier, c);

    if (c != 'C') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'N') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'V') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'S') {
        return false;
    }

    return true;
}

static void ecris_nombre_magique(std::ofstream &fichier)
{
    ecris_fichier(fichier, 'C');
    ecris_fichier(fichier, 'N');
    ecris_fichier(fichier, 'V');
    ecris_fichier(fichier, 'S');
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Écriture
 * \{ */

static bool lis_projet(std::ifstream &fichier)
{
    char c;

    lis_fichier(fichier, c);

    if (c != 'P') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'R') {
        return false;
    }

    dls::chaine nom;
    lis_fichier(fichier, nom);

    std::cerr << "Nom projet : " << nom << '\n';

    return true;
}

static void ecris_projet(std::ofstream &fichier)
{
    ecris_fichier(fichier, 'P');
    ecris_fichier(fichier, 'R');
    dls::chaine nom("stargate SG-1");
    ecris_fichier(fichier, nom);
}

static bool lis_brosse(std::ifstream &fichier, Brosse *brosse)
{
    char c;

    lis_fichier(fichier, c);

    if (c != 'B') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'R') {
        return false;
    }

    brosse->définis_couleur(lis_valeur<dls::math::vec4f>(fichier));
    brosse->définis_opacité(lis_valeur<float>(fichier));
    brosse->définis_rayon(lis_valeur<int>(fichier));

    return true;
}

static void ecris_brosse(std::ofstream &fichier, Brosse *brosse)
{
    ecris_fichier(fichier, 'B');
    ecris_fichier(fichier, 'R');
    ecris_fichier(fichier, brosse->donne_couleur());
    ecris_fichier(fichier, brosse->donne_opacité());
    ecris_fichier(fichier, brosse->donne_rayon());
}

static bool lis_maillage(std::ifstream &fichier, Maillage *maillage)
{
    char c;

    lis_fichier(fichier, c);

    if (c != 'M') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'A') {
        return false;
    }

    dls::chaine nom;
    lis_fichier(fichier, nom);
    maillage->nom(nom);

    long nombre_sommets, nombre_arretes, nombre_polygones;
    lis_fichier(fichier, nombre_sommets);

    dls::math::vec3f pos;

    for (auto i = 0; i < nombre_sommets; ++i) {
        lis_fichier(fichier, pos.x);
        lis_fichier(fichier, pos.y);
        lis_fichier(fichier, pos.z);

        maillage->ajoute_sommet(pos);
    }

    lis_fichier(fichier, nombre_arretes);

    lis_fichier(fichier, nombre_polygones);

    dls::math::vec4i poly;

    for (auto i = 0; i < nombre_polygones; ++i) {
        lis_fichier(fichier, poly.x);
        lis_fichier(fichier, poly.y);
        lis_fichier(fichier, poly.z);
        lis_fichier(fichier, poly.w);

        maillage->ajoute_quad(poly.x, poly.y, poly.z, poly.w);

        auto polygone = maillage->polygone(i);

        lis_fichier(fichier, polygone->res_u);
        lis_fichier(fichier, polygone->res_v);
        lis_fichier(fichier, polygone->x);
        lis_fichier(fichier, polygone->y);
        lis_fichier(fichier, polygone->index);
    }

    return true;
}

static void ecris_maillage(std::ofstream &fichier, Maillage *maillage)
{
    ecris_fichier(fichier, 'M');
    ecris_fichier(fichier, 'A');

    ecris_fichier(fichier, maillage->nom());

    auto nombre_sommets = maillage->nombre_sommets();
    ecris_fichier(fichier, nombre_sommets);

    for (auto i = 0; i < nombre_sommets; ++i) {
        auto sommet = maillage->sommet(i);

        ecris_fichier(fichier, sommet->pos.x);
        ecris_fichier(fichier, sommet->pos.y);
        ecris_fichier(fichier, sommet->pos.z);
    }

    auto nombre_arretes = maillage->nombre_arretes();
    ecris_fichier(fichier, nombre_arretes);

    auto nombre_polygones = maillage->nombre_polygones();
    ecris_fichier(fichier, nombre_polygones);

    for (auto i = 0; i < nombre_polygones; ++i) {
        auto polygone = maillage->polygone(i);

        ecris_fichier(fichier, polygone->s[0]->index);
        ecris_fichier(fichier, polygone->s[1]->index);
        ecris_fichier(fichier, polygone->s[2]->index);
        ecris_fichier(fichier, polygone->s[3]->index);
        ecris_fichier(fichier, polygone->res_u);
        ecris_fichier(fichier, polygone->res_v);
        ecris_fichier(fichier, polygone->x);
        ecris_fichier(fichier, polygone->y);
        ecris_fichier(fichier, polygone->index);
    }
}

static bool lis_calque(std::ifstream &fichier, Calque *calque, size_t resolution)
{
    char c;

    lis_fichier(fichier, c);

    if (c != 'C') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'A') {
        return false;
    }

    lis_fichier(fichier, calque->chemin);
    lis_fichier(fichier, calque->nom);
    lis_fichier(fichier, calque->mode_fusion);
    lis_fichier(fichier, calque->opacite);
    lis_fichier(fichier, calque->couleur.x);
    lis_fichier(fichier, calque->couleur.y);
    lis_fichier(fichier, calque->couleur.z);
    lis_fichier(fichier, calque->couleur.w);
    lis_fichier(fichier, calque->drapeaux);
    lis_fichier(fichier, calque->type_donnees);
    lis_fichier(fichier, calque->type_calque);
    lis_fichier(fichier, calque->taille_u);
    lis_fichier(fichier, calque->taille_v);
    lis_fichier(fichier, calque->octaves);
    lis_fichier(fichier, calque->taille);

    size_t taille_tampon;
    lis_fichier(fichier, taille_tampon);

    switch (calque->type_donnees) {
        case TypeDonnees::SCALAIRE:
            calque->tampon = new float[resolution];
            break;
        case TypeDonnees::COULEUR:
            calque->tampon = new dls::math::vec4f[resolution];
            break;
        case TypeDonnees::VECTEUR:
            calque->tampon = new dls::math::vec3f[resolution];
            break;
    }

    std::cerr << "Lecture d'un calque de " << taille_tampon << " octets\n";

    fichier.read(static_cast<char *>(calque->tampon), static_cast<long int>(taille_tampon));

    return true;
}

static bool lis_canaux(std::ifstream &fichier, CanauxTexture &canaux)
{
    char c;

    lis_fichier(fichier, c);

    if (c != 'C') {
        return false;
    }

    lis_fichier(fichier, c);

    if (c != 'X') {
        return false;
    }

    lis_fichier(fichier, canaux.hauteur);
    lis_fichier(fichier, canaux.largeur);

    long nombre_calques;
    lis_fichier(fichier, nombre_calques);

    std::cerr << "Lecture de " << nombre_calques << " calques....\n";

    canaux.calques[TypeCanal::DIFFUSION].redimensionne(nombre_calques);

    for (auto &calque : canaux.calques[TypeCanal::DIFFUSION]) {
        calque = new Calque;

        lis_calque(fichier, calque, canaux.hauteur * canaux.largeur);
    }

    return true;
}

static void ecris_calque(std::ofstream &fichier, Calque *calque, size_t taille_tampon)
{
    ecris_fichier(fichier, 'C');
    ecris_fichier(fichier, 'A');

    ecris_fichier(fichier, calque->chemin);
    ecris_fichier(fichier, calque->nom);
    ecris_fichier(fichier, calque->mode_fusion);
    ecris_fichier(fichier, calque->opacite);
    ecris_fichier(fichier, calque->couleur.x);
    ecris_fichier(fichier, calque->couleur.y);
    ecris_fichier(fichier, calque->couleur.z);
    ecris_fichier(fichier, calque->couleur.w);
    ecris_fichier(fichier, calque->drapeaux);
    ecris_fichier(fichier, calque->type_donnees);
    ecris_fichier(fichier, calque->type_calque);
    ecris_fichier(fichier, calque->taille_u);
    ecris_fichier(fichier, calque->taille_v);
    ecris_fichier(fichier, calque->octaves);
    ecris_fichier(fichier, calque->taille);
    ecris_fichier(fichier, taille_tampon);

    fichier.write(static_cast<char *>(calque->tampon), static_cast<long int>(taille_tampon));
}

static void ecris_canaux(std::ofstream &fichier, CanauxTexture &canaux)
{
    ecris_fichier(fichier, 'C');
    ecris_fichier(fichier, 'X');

    ecris_fichier(fichier, canaux.hauteur);
    ecris_fichier(fichier, canaux.largeur);

    auto nombre_calques = canaux.calques[TypeCanal::DIFFUSION].taille();
    ecris_fichier(fichier, nombre_calques);

    auto resolution = canaux.hauteur * canaux.largeur;

    for (auto const &calque : canaux.calques[TypeCanal::DIFFUSION]) {
        size_t taille_tampon = resolution;

        switch (calque->type_donnees) {
            case TypeDonnees::SCALAIRE:
                taille_tampon *= sizeof(float);
                break;
            case TypeDonnees::COULEUR:
                taille_tampon *= sizeof(dls::math::vec4f);
                break;
            case TypeDonnees::VECTEUR:
                taille_tampon *= sizeof(dls::math::vec3f);
                break;
        }

        ecris_calque(fichier, calque, taille_tampon);
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Points d'entrée pour la lecture et l'écriture.
 * \{ */

void écris_projet(Kanba &kanba, dls::chaine const &chemin_projet)
{
    std::ofstream fichier;
    fichier.open(chemin_projet.c_str(), std::ios_base::binary);

    KNB::ecris_nombre_magique(fichier);
    KNB::ecris_projet(fichier);
    KNB::ecris_brosse(fichier, kanba.donne_brosse());
    KNB::ecris_maillage(fichier, kanba.donne_maillage());
    KNB::ecris_canaux(fichier, kanba.donne_maillage()->canaux_texture());
}

bool lis_projet(Kanba &kanba, dls::chaine const &chemin_projet)
{
    std::ifstream fichier;
    fichier.open(chemin_projet.c_str(), std::ios_base::binary);

    if (!fichier.is_open()) {
        return false;
    }

    if (!KNB::lis_nombre_magic(fichier)) {
        std::cerr << "Le fichier ne contient pas de nombre magique valide !\n";
        return false;
    }

    if (!KNB::lis_projet(fichier)) {
        std::cerr << "Le fichier ne contient pas de projet valide !\n";
        return false;
    }

    KNB::Brosse brosse;

    if (!lis_brosse(fichier, &brosse)) {
        std::cerr << "Le fichier ne contient pas de brosse valide !\n";
        return false;
    }

    KNB::Maillage *maillage = new KNB::Maillage;

    if (!lis_maillage(fichier, maillage)) {
        std::cerr << "Le fichier ne contient pas de maillage valide !\n";
        delete maillage;
        return false;
    }

    if (!lis_canaux(fichier, maillage->canaux_texture())) {
        std::cerr << "Le fichier ne contient pas de canaux valide !\n";
        delete maillage;
        return false;
    }

    kanba.installe_maillage(maillage);
    *kanba.donne_brosse() = brosse;

    return true;
}

/** \} */

}  // namespace KNB
