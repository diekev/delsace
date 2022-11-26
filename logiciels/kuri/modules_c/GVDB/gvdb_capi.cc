/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "gvdb_capi.h"

#include "src/gvdb_camera.h"
#include "src/gvdb_scene.h"
#include "src/gvdb_volume_3D.h"
#include "src/gvdb_volume_base.h"
#include "src/gvdb_volume_gvdb.h"

using nvdb::Camera3D;
using nvdb::Scene;
using nvdb::VolumeGVDB;

extern "C" {

// Initialisation bibliothèque

VolumeGVDB *GVDB_cree_volume()
{
    return new VolumeGVDB;
}

void GVDB_volume_active_verbosite(VolumeGVDB *gvdb, bool verbeux)
{
    gvdb->SetVerbose(verbeux);
}

void GVDB_volume_initialise(VolumeGVDB *gvdb)
{
    gvdb->Initialize();
}

void GVDB_volume_mute_device_cuda(VolumeGVDB *gvdb, int id_device)
{
    gvdb->SetCudaDevice(id_device);
}

// gestion des chemins pour FindFile, devrait se faire via un module séparé
void GVDB_volume_ajoute_chemin(VolumeGVDB *gvdb, const char *chemin)
{
    gvdb->AddPath(chemin);
}

void GVDB_volume_charge_vbx(VolumeGVDB *gvdb, const char *chemin)
{
    gvdb->LoadVBX(chemin);
}

void NVDB_volume_applique_fonction_transfert(VolumeGVDB *volume)
{
    volume->CommitTransferFunc();
}

void NVDB_volume_mute_transformation(
    VolumeGVDB *volume, float *pretranslation, float *taille, float *angles, float *translation)
{
    volume->SetTransform(pretranslation, taille, angles, translation);
}

void NVDB_volume_ajoute_tampon_rendu(
    VolumeGVDB *volume, int canal, int largeur, int hauteur, int octets_par_pixel)
{
    volume->AddRenderBuf(canal, largeur, hauteur, octets_par_pixel);
}

void NVDB_volume_redimensionne_tampon_rendu(
    VolumeGVDB *volume, int canal, int largeur, int hauteur, int octets_par_pixel)
{
    volume->ResizeRenderBuf(canal, largeur, hauteur, octets_par_pixel);
}

void NVDB_volume_calcule_rendu(VolumeGVDB *volume,
                               char shade_mode,
                               uchar canal_entree,
                               uchar tampon_sortie)
{
    volume->Render(shade_mode, canal_entree, tampon_sortie);
}

void NVDB_volume_copie_rendu_vers_opengl(VolumeGVDB *volume, int canal, int id_gl)
{
    volume->ReadRenderTexGL(canal, id_gl);
}

void NVDB_volume_demarre_chronometre(VolumeGVDB *volume)
{
    volume->TimerStart();
}

void NVDB_volume_arrete_chronometre(VolumeGVDB *volume)
{
    volume->TimerStop();
}

// Scene

Scene *NVDB_scene(VolumeGVDB *gvdb)
{
    return gvdb->getScene();
}

void NVDB_scene_mute_extinction_volume(Scene *scene, float a, float b, float c)
{
    scene->SetExtinct(a, b, c);
}

void NVDB_scene_mute_etapes_raycasting(Scene *scene, float directes, float ombres, float fines)
{
    scene->SetSteps(directes, ombres, fines);
}

void NVDB_scene_mute_plage_valeurs_volumes(Scene *scene, float iso, float min, float max)
{
    scene->SetVolumeRange(iso, min, max);
}

void NVDB_scene_mute_cutoff(Scene *scene, float a, float b, float c)
{
    scene->SetCutoff(a, b, c);
}

void NVDB_scene_mute_couleur_arriere_plan(Scene *scene, float r, float v, float b, float a)
{
    scene->SetBackgroundClr(r, v, b, a);
}

void NVDB_scene_mute_fonction_transfert(Scene *scene, float t0, float t1, float *a, float *b)
{
    scene->LinearTransferFunc(t0, t1, a, b);
}

void NVDB_scene_mute_camera(Scene *scene, Camera3D *camera)
{
    scene->SetCamera(camera);
}

void NVDB_scene_mute_lumiere(Scene *scene, int index, Light *lumiere)
{
    scene->SetLight(index, lumiere);
}

// Camera

Camera3D *NVDB_cree_camera()
{
    return new Camera3D;
}

void NVDB_detruit_camera(Camera3D *camera)
{
    delete camera;
}

void NVDB_camera_mute_champs_de_vision(Camera3D *camera, float cdv)
{
    camera->setFov(cdv);
}

void NVDB_camera_mute_orbite(Camera3D *camera, float *angles, float *tp, float dist, float dolly)
{
    camera->setOrbit(angles, tp, dist, dolly);
}

void NVDB_camera_accede_angles(Camera3D *camera, float *angles)
{
    auto angs = camera->getAng();
    angles[0] = angs.X();
    angles[1] = angs.Y();
    angles[2] = angs.Z();
}

void NVDB_camera_accede_position(Camera3D *camera, float *pos)
{
    auto to_pos = camera->getToPos();
    pos[0] = to_pos.X();
    pos[1] = to_pos.Y();
    pos[2] = to_pos.Z();
}

float NVDB_camera_accede_distance_orbite(Camera3D *camera)
{
    return camera->getOrbitDist();
}

float NVDB_camera_accede_dolly(Camera3D *camera)
{
    return camera->getDolly();
}

float NVDB_camera_bouge_relatif(Camera3D *camera, float dx, float dy, float dz)
{
    camera->moveRelative(dx, dy, dz);
}

// Lumière

Light *NVDB_cree_lumiere()
{
    return new Light;
}

void NVDB_detruit_lumiere(Light *lumiere)
{
    delete lumiere;
}

void NVDB_lumiere_mute_orbite(Light *lumiere, float *angles, float *tp, float dist, float dolly)
{
    lumiere->setOrbit(angles, tp, dist, dolly);
}

}  // extern "C"
