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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <string>
#include <thread>
#include <vector>

#include "bibliotheques/observation/observation.hh"
#include "bibliotheques/commandes/commande.h"

#include "chef_execution.hh"
#include "gestionnaire_fichier.hh"
#include "usine_operatrice.h"

class BaseEditrice;
class Composite;
class Configuration;
class FenetrePrincipale;
class Graphe;
class Manipulatrice3D;
class Manipulatrice2D;
class Noeud;
class ProjectSettings;
class RepondantCommande;
class TaskNotifier;

namespace vision {
class Camera2D;
class Camera3D;
}  /* namespace vision */

namespace danjo {
class GestionnaireInterface;
}  /* namespace danjo */

enum {
	FICHIER_OUVERTURE,
	FICHIER_SAUVEGARDE,
};

enum {
	GRAPHE_COMPOSITE,
	GRAPHE_PIXEL,
	GRAPHE_SCENE,
	GRAPHE_OBJET,
	GRAPHE_MAILLAGE,
	GRAPHE_SIMULATION,
};

class Mikisa : public Sujette {
	UsineCommande m_usine_commande{};
	UsineOperatrice m_usine_operatrices;
	RepondantCommande *m_repondant_commande = nullptr;

	std::vector<std::string> m_fichiers_recents{};
	std::string m_chemin_projet{};

	bool m_projet_ouvert = false;

public:
	Mikisa();
	~Mikisa();

	/* Les usines et le répondant commande peuvent et doivent être partagés. */
	Mikisa(Mikisa const &autre) = default;
	Mikisa &operator=(Mikisa const &autre) = default;

	void initialise();

	UsineCommande &usine_commandes();

	UsineOperatrice &usine_operatrices();

	std::string requiers_dialogue(int type);
	void affiche_erreur(std::string const &message);

	std::string chemin_projet() const;

	void chemin_projet(std::string const &chemin);

	std::vector<std::string> const &fichiers_recents();
	void ajoute_fichier_recent(std::string const &chemin);

	bool projet_ouvert() const;

	void projet_ouvert(bool ouinon);

	RepondantCommande *repondant_commande() const;

	Composite *composite;

	/* entreface */
	FenetrePrincipale *fenetre_principale;
	BaseEditrice *editrice_active;
	danjo::GestionnaireInterface *gestionnaire_entreface;

	/* Preferences and settings. */
	ProjectSettings *project_settings;

	/* Information de sortie. */
	std::string chemin_sortie = "";
	std::string nom_calque_sortie = "";

	/* vue 2d */
	vision::Camera2D *camera_2d = nullptr;

	/* vue 3d */
	vision::Camera3D *camera_3d = nullptr;

	/* temps */
	int temps_debut = 1;
	int temps_courant = 1;
	int temps_fin = 250;
	double cadence = 24.0;
	bool animation = false;

	/* contexte graphe */
	int contexte = GRAPHE_COMPOSITE;
	Graphe *graphe = nullptr;

	Noeud *derniere_visionneuse_selectionnee = nullptr;
	Noeud *derniere_scene_selectionnee = nullptr;

	/* manipulation objets 2d */
	bool manipulation_2d_activee = false;
	int type_manipulation_2d = 0;
	Manipulatrice2D *manipulatrice_2d = nullptr;

	/* manipulation objets 3d */
	bool manipulation_3d_activee = false;
	int type_manipulation_3d = 0;
	Manipulatrice3D *manipulatrice_3d = nullptr;

	/* chemin du graphe courant */
	std::string chemin_courant = "";

	/* pour les tâches */
	bool tache_en_cours = false;
	bool interrompu = false;

	/* thread utilisé pour jouer des animations */
	std::thread *thread_animation{};

	TaskNotifier *notifiant_thread{};

	GestionnaireFichier gestionnaire_fichier{};

	ChefExecution chef_execution;

	void ajourne_pour_nouveau_temps(const char *message);
};
