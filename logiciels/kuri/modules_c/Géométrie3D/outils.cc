/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "outils.hh"

namespace geo {

long Maillage::nombreDePoints() const
{
    return this->nombre_de_points(this->donnees);
}

math::vec3f Maillage::pointPourIndex(long n) const
{
    math::vec3f pos;
    this->point_pour_index(this->donnees, n, &pos.x, &pos.y, &pos.z);
    return pos;
}

long Maillage::nombreDePolygones() const
{
    return this->nombre_de_polygones(this->donnees);
}

long Maillage::nombreDeSommetsPolygone(long n) const
{
    return this->nombre_de_sommets_polygone(this->donnees, n);
}

void Maillage::pointPourSommetPolygones(long n, long v, math::vec3f &pos) const
{
    this->point_pour_sommet_polygone(this->donnees, n, v, &pos.x, &pos.y, &pos.z);
}

void Maillage::remplacePointALIndex(long n, const math::vec3f &point)
{
    this->remplace_point_a_l_index(this->donnees, n, point.x, point.y, point.z);
}

void Maillage::indexPointsSommetsPolygone(long n, int *index) const
{
    this->index_points_sommets_polygone(this->donnees, n, index);
}

void Maillage::rafinePolygone(long i, const RafineusePolygone &rafineuse) const
{
    if (!this->rafine_polygone) {
        return;
    }
    this->rafine_polygone(this->donnees, i, &rafineuse);
}

limites<math::vec3f> Maillage::boiteEnglobante() const
{
    float min_x = std::numeric_limits<float>::max();
    float min_y = std::numeric_limits<float>::max();
    float min_z = std::numeric_limits<float>::max();
    float max_x = -std::numeric_limits<float>::max();
    float max_y = -std::numeric_limits<float>::max();
    float max_z = -std::numeric_limits<float>::max();
    this->calcule_boite_englobante(this->donnees, &min_x, &min_y, &min_z, &max_x, &max_y, &max_z);
    math::vec3f min = math::vec3f(min_x, min_y, min_z);
    math::vec3f max = math::vec3f(max_x, max_y, max_z);
    return {min, max};
}

math::vec3f Maillage::normalPolygone(long i) const
{
    math::vec3f pos;
    this->calcule_normal_polygone(this->donnees, i, &pos.x, &pos.y, &pos.z);
    return pos;
}

void Maillage::ajoutePoints(float *points, long nombre) const
{
    if (this->ajoute_plusieurs_points) {
        this->ajoute_plusieurs_points(this->donnees, points, nombre);
        return;
    }

    reserveNombreDePoints(nombre);

    for (int i = 0; i < nombre; i++) {
        ajouteUnPoint(points[0], points[1], points[2]);
        points += 3;
    }
}

void Maillage::reserveNombreDePoints(long nombre) const
{
    this->reserve_nombre_de_points(this->donnees, nombre);
}

void Maillage::reserveNombreDePolygones(long nombre) const
{
    this->reserve_nombre_de_polygones(this->donnees, nombre);
}

void Maillage::ajouteUnPoint(float x, float y, float z) const
{
    this->ajoute_un_point(this->donnees, x, y, z);
}

void Maillage::ajouteUnPoint(math::vec3f xyz) const
{
    this->ajoute_un_point(this->donnees, xyz.x, xyz.y, xyz.z);
}

void Maillage::ajouteListePolygones(const int *sommets,
                                    const int *sommets_par_polygones,
                                    long nombre_polygones)
{
    this->ajoute_liste_polygones(this->donnees, sommets, sommets_par_polygones, nombre_polygones);
}

void Maillage::ajouteUnPolygone(const int *sommets, int taille) const
{
    this->ajoute_un_polygone(this->donnees, sommets, taille);
}

void *Maillage::creeUnGroupeDePoints(const std::string &nom) const
{
    return this->cree_un_groupe_de_points(
        this->donnees, nom.c_str(), static_cast<long>(nom.size()));
}

void *Maillage::creeUnGroupeDePolygones(const std::string &nom) const
{
    return this->cree_un_groupe_de_polygones(
        this->donnees, nom.c_str(), static_cast<long>(nom.size()));
}

void Maillage::ajouteAuGroupe(void *poignee_groupe, long index) const
{
    this->ajoute_au_groupe(poignee_groupe, index);
}

void Maillage::ajoutePlageAuGroupe(void *poignee_groupe, long index_debut, long index_fin) const
{
    this->ajoute_plage_au_groupe(poignee_groupe, index_debut, index_fin);
}

bool Maillage::groupePolygonePossedePoint(const void *poignee_groupe, long index) const
{
    return this->groupe_polygone_possede_point(poignee_groupe, index);
}

}  // namespace geo
