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
 * The Original Code is Copyright (C) 2019 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "operatrices_simulation_foule.hh"

#include <random>

#include "coeur/contexte_evaluation.hh"
#include "coeur/operatrice_corps.h"
#include "coeur/usine_operatrice.h"

#include "outils_visualisation.hh"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"

/* ************************************************************************** */

/**
 * Simulation de foule.
 *
 * Sources :
 * - CrowD : https://github.com/DannySortino/Project
 * - BlenderPeople : http://www.harkyman.com/bp.html
 * - CrowdMaster : https://github.com/johnroper100/CrowdMaster
 */

/* ************************************************************************** */

//Target
//verts = [[1,1,0],[1,-1,0], [-1,-1,0], [-1,1,0] ]
//        edges = [[0,1],[1,2],[2,3],[3,0]]
//        faces = [[0,1,2,3]]

//Spawn Cube
//verts = [[1,1,0],[1,-1,0], [-1,-1,0], [-1,1,0], [1,1,1],[1,-1,1], [-1,-1,1], [-1,1,1] ]# draws the vertices of the square cube
//        edges = [[0,1],[1,2],[2,3],[3,0], [0,4], [1,5], [2,6], [3,7], [4, 5], [5,6], [6,7], [7,4] ] # connects the vertices of the square cube to make edges
//        faces = []

//Spawn Plane
//		verts = [[1,1,0],[1,-1,0], [-1,-1,0], [-1,1,0] ] # draws the vertices of the square plane
//				edges = [[0,1],[1,2],[2,3],[3,0]] # connects the vertices of the square to make edges
//				faces = []

struct Personne {
	dls::chaine nom = "";
	dls::math::vec3f location = dls::math::vec3f(0.0f);
	dls::math::vec3f parcouru = dls::math::vec3f(0.0f);
	dls::math::vec3f min = dls::math::vec3f(0.0f);
	dls::math::vec3f max = dls::math::vec3f(0.0f);
	float vitesse = 0.0f;
	bool en_mouvement = false;
	bool a_fini = false;
	REMBOURRE(2);
	int id = 0;

	/* depuis l'add-on */
	dls::math::vec3f Target = dls::math::vec3f(0.0f);
	dls::math::vec3f *SecondaryTarget = nullptr;
};

static void bouge(Personne &personne, dls::math::vec3f const &cible_globale)
{
	if (!personne.en_mouvement) {
		return;
	}

	auto const delta = cible_globale - personne.location;

	auto const dist_delta = longueur(delta);

	/* À FAIRE : paramètre de distance minimale. */
	if (dist_delta < 1.0f) {
		personne.en_mouvement = false;
		personne.a_fini = true;
		return;
	}

	auto const mouvement_requis = std::abs(delta.x) + std::abs(delta.y) + std::abs(delta.z);

	if (mouvement_requis != 0.0f) {
		personne.parcouru = delta * (personne.vitesse / mouvement_requis);
	}
}

#if 0
void CheckCollision(size_t PersonCount, dls::tableau<Personne> &Person, int ColNumb)
{
	//Count = 0 // sets the count for people to start off at 0
	for (auto Count = 0ul; Count < Person.taille(); ++Count) {
		if (Count != PersonCount)  { //# means person doesnt exist
			if (Person[PersonCount].a_fini != true) { //# says they have reached their final destination
				if (/*Done_Moving.count(Count) < Person.taille()*/false) { //# if the cube it collides with has stopped
					float TargetX, TargetY, TargetZ;

					if (Person[Count].SecondaryTarget == nullptr) {
						TargetX = Person[Count].Target[0];
						TargetY = Person[Count].Target[1];
						TargetZ = Person[Count].Target[2];
					}
					else {
						TargetX = (*Person[Count].SecondaryTarget)[0];
						TargetY = (*Person[Count].SecondaryTarget)[1];
						TargetZ = (*Person[Count].SecondaryTarget)[2];
					}

					Person[PersonCount].SecondaryTarget = nullptr;
					auto Frame = 0;//int(bpy.context.scene.frame_current)-int(bpy.data.scenes[Scene_Name].Frame_Start); //# sets the frame number for which is being addressed
					if (abs(abs(Person[Count].Target[0] - Person[Count].location[0]) + abs(Person[Count].Target[1] - Person[Count].location[1]) + abs(Person[Count].Target[2] - Person[Count].location[2])) != 0) {
						auto TotalMovPos = Person[Count].vitesse / abs(abs(Person[Count].Target[0] - Person[Count].location[0]) + abs(Person[Count].Target[1] - Person[Count].location[1]) + abs(Person[Count].Target[2] - Person[Count].location[2]));// # recalculates the total movement that can be done
						auto MovingX = TotalMovPos * (TargetX - Person[Count].location[0]);// #PersonChecked moved location in x direction
						auto MovingY = TotalMovPos * (TargetY - Person[Count].location[1]);
						auto MovingZ = TotalMovPos * (TargetZ - Person[Count].location[2]);
						auto Frame = 0;int(bpy.context.scene.frame_current)-int(bpy.data.scenes[Scene_Name].Frame_Start)# sets the frame number for which is being addressed
						//print(Frame)

						if (Person[Count].CollisionCount == 100) {
						   // print('Colided 100 times') # Find way to judge movement behind person in way, and set position there
							Person[Count].Xloc[Frame + 1] = (Person[Count].Xloc[Frame] + (MovingX / 100))
							Person[Count].Yloc[Frame + 1] = (Person[Count].Yloc[Frame] + (MovingY / 100))
							Person[Count].Zloc[Frame + 1] = (Person[Count].Zloc[Frame] + (MovingZ / 100))
							Person[Count].location[0] = Person[Count].Xloc[Frame + 1]
							Person[Count].location[1] = Person[Count].Yloc[Frame + 1]
							Person[Count].location[2] = Person[Count].Zloc[Frame + 1]
							Person[Count].MaxX[Frame + 1] = (Person[Count].MaxX[Frame] + (MovingX / 100))
							Person[Count].MinX[Frame + 1] = (Person[Count].MinX[Frame] + (MovingX / 100))
							Person[Count].MaxY[Frame + 1] = (Person[Count].MaxY[Frame] + (MovingY / 100))
							Person[Count].MinY[Frame + 1] = (Person[Count].MinY[Frame] + (MovingY / 100))
							Person[Count].MaxZ[Frame + 1] = (Person[Count].MaxZ[Frame] + (MovingZ / 100))
							Person[Count].MinZ[Frame + 1] = (Person[Count].MaxZ[Frame] + (MovingZ / 100))
							Person[Count].CollisionCount = 0
							Person[Count].Reroute = true
//                            Person[PersonCount].CollisionCount = 0
//                            Person[Count].SecondaryTarget[0] = 1
//                            if Person[PersonCount].Xloc[Frame] >= Person[Count].Target[0]:
//                                Person[Count].SecondaryTarget[1] = Person[PersonCount].Xloc[Frame] - 0.005
//                            else:
//                                Person[Count].SecondaryTarget[1] = Person[PersonCount].Xloc[Frame] + 0.005
//                            if Person[PersonCount].Yloc[Frame] >= Person[Count].Target[1]:
//                                Person[Count].SecondaryTarget[2] = Person[PersonCount].Yloc[Frame] - 0.005
//                            else:
//                               Person[Count].SecondaryTarget[2] = Person[PersonCount].Yloc[Frame] + 0.005
//                            if Person[PersonCount].Zloc[Frame] >= Person[Count].Target[2]:
//                                Person[Count].SecondaryTarget[3] = Person[PersonCount].Zloc[Frame] - 0.005
//                            else:
//                                Person[Count].SecondaryTarget[3] = Person[PersonCount].Zloc[Frame] + 0.005
//                            Person[PersonCount].SecondaryTarget[0] = 0
//                            Person[PersonCount].Obstacle = false
//                            Person[PersonCount].en_mouvement = false # says there is no more movement from this person
//                            Person[PersonCount].a_fini = true # says they have reached their final destination
//                            Done_Moving.append(Person[PersonCount].Number)
						}
						if  Person[Count].Reroute == true {
							//pass
							continue;
						}
						else {
							//if (abs(int(abs(Person[Count].MaxX[Frame]) - abs(Person[PersonCount].MaxX[FraAme]))) < 0.4) or (abs(int(abs(Person[Count].MinX[Frame]) - abs(Person[PersonCount].MinX[Frame]))) < 0.4): # checks to see if the persons vertex location in X direction is within 0.06 otherwise it is too far away and doesnt need to check collision.
								//if (abs(int(abs(Person[Count].MaxY[Frame]) - abs(Person[PersonCount].MaxY[Frame]))) < 0.4) or (abs(int(abs(Person[Count].MinY[Frame]) - abs(Person[PersonCount].MinY[Frame]))) < 0.4):

								  //if (abs(int(abs(Person[Count].MaxZ[Frame]) - abs(Person[PersonCount].MaxZ[Frame]))) < 0.4) or (abs(int(abs(Person[Count].MinZ[Frame]) - abs(Person[PersonCount].MinZ[Frame]))) < 0.4):
							if any([(Xloc >= Person[PersonCount].MinX[Frame] + Person[PersonCount].MovedX) and (Xloc <= Person[PersonCount].MaxX[Frame] + Person[PersonCount].MovedX) for Xloc in [(Person[Count].MinX[Frame] + MovingX), Person[Count].MaxX[Frame] + MovingX]]):
								//print('x intersection')
								if Person[PersonCount].MaxX[Frame] + Person[PersonCount].MovedX < Person[Count].MaxX[Frame] + MovingX {
									XPoint = [(Person[Count].MaxX[Frame] + MovingX), 0];
								}
								else {
									XPoint = [(Person[Count].MinX[Frame] + MovingX), 1];
								}

								if any([(Yloc >= Person[PersonCount].MinY[Frame] + Person[PersonCount].MovedY) and (Yloc <= Person[PersonCount].MaxY[Frame] + Person[PersonCount].MovedY) for Yloc in [(Person[Count].MinY[Frame] + MovingY), Person[Count].MaxY[Frame] + MovingY]]) {
									print('y intersection')
									if Person[PersonCount].MaxY[Frame] + Person[PersonCount].MovedY < Person[Count].MaxY[Frame] + MovingY {
										YPoint = [(Person[Count].MaxY[Frame] + MovingY), 0]
									}
									else {
										YPoint = [(Person[Count].MinY[Frame] + MovingY), 1]
									}

									if any([(Zloc >= Person[PersonCount].MinZ[Frame] + Person[PersonCount].MovedZ) and (Zloc <= Person[PersonCount].MaxZ[Frame] + Person[PersonCount].MovedZ) for Zloc in [(Person[Count].MinZ[Frame] + MovingZ), Person[Count].MaxZ[Frame] + MovingZ]]) {
										print('z intersection')
										if Person[PersonCount].MaxZ[Frame] + Person[PersonCount].MovedZ < Person[Count].MaxZ[Frame] + MovingZ {
											ZPoint = [(Person[Count].MaxZ[Frame] + MovingZ), 0]
										}
										else {
											ZPoint = [(Person[Count].MinZ[Frame] + MovingZ), 1]
										}
										print('****', Person[PersonCount].Name, Person[Count].Name)
										Person[Count].CollisionCount = Person[Count].CollisionCount + 1
										Person[Count].Obstacle = true # it has found an obstacle
										Person[Count].Check = Person[Count].Check + 1 # starts counting up with direction to check for an alternate route
										Person[Count].SecondaryTarget[0] = 1
										CollisionReroute(Count, Person, XPoint, YPoint, ZPoint, Frame, PersonCount) # calculate the collision reroute
									}
								}
							}
						}
					}
					else {
						Person[Count].Obstacle = false;
						Person[Count].en_mouvement = false;// # says there is no more movement from this person
						Person[Count].a_fini = true; // says they have reached their final destination
						Done_Moving.append(Person[Count].Number);
					}
				}
			}
		}
		auto Frame = 0; //int(bpy.context.scene.frame_current)-int(bpy.data.scenes[Scene_Name].Frame_Start)# sets the frame number for which is being addressed
		if (Person[Count].a_fini != true) { // says they have reached their final destination
			if (Person[Count].Xloc[Frame + 1] == 0) {
				float TargetX, TargetY, TargetZ;

				if (Person[Count].SecondaryTarget[0] == 0) {
					TargetX = Person[Count].Target[0];
					TargetY = Person[Count].Target[1];
					TargetZ = Person[Count].Target[2];
				}
				else {
					TargetX = Person[Count].SecondaryTarget[1];
					TargetY = Person[Count].SecondaryTarget[2];
					TargetZ = Person[Count].SecondaryTarget[3];
				}

				Person[PersonCount].SecondaryTarget[0] = 0;
				TotalMovPos = Person[Count].vitesse / abs(abs(Person[Count].Target[0] - Person[Count].location[0]) + abs(Person[Count].Target[1] - Person[Count].location[1]) + abs(Person[Count].Target[2] - Person[Count].location[2])) # recalculates the total movement that can be done
				MovingX = TotalMovPos * (TargetX - Person[Count].location[0]);// #PersonChecked moved location in x direction
				MovingY = TotalMovPos * (TargetY - Person[Count].location[1]);
				MovingZ = TotalMovPos * (TargetZ - Person[Count].location[2]);
				Person[Count].Xloc[Frame + 1] = (Person[Count].Xloc[Frame] + MovingX);
				Person[Count].Yloc[Frame + 1] = (Person[Count].Yloc[Frame] + MovingY);
				Person[Count].Zloc[Frame + 1] = (Person[Count].Zloc[Frame] + MovingZ);

				if (abs(abs(Person[Count].Target[0] - Person[Count].Xloc[Frame + 1]) + abs(Person[Count].Target[1] -Person[Count].Yloc[Frame + 1]) + abs(Person[Count].Target[2] - Person[Count].Zloc[Frame + 1])) < 0.15) {
						Person[Count].Xloc[Frame + 1] = 0;
						Person[Count].Yloc[Frame + 1] = 0;
						Person[Count].Zloc[Frame + 1] = 0;
						Person[Count].MaxX.append(1);
						Person[Count].MinX.append(-1);
						Person[Count].MaxY.append(1);
						Person[Count].MinY.append(-1);
						Person[Count].MaxZ.append(1);
						Person[Count].MinZ.append(-1);
						Person[Count].location[0] = Person[Count].Xloc[Frame + 1];
						Person[Count].location[1] = Person[Count].Yloc[Frame + 1];
						Person[Count].location[2] = Person[Count].Zloc[Frame + 1];
				}
				else {
					if (abs(abs(Person[Count].Target[0] - Person[Count].location[0]) + abs(Person[Count].Target[1] - Person[Count].location[1]) + abs(Person[Count].Target[2] - Person[Count].location[2])) != 0) {
						Person[Count].Xloc[Frame + 1] = (Person[Count].Xloc[Frame] + MovingX);
						Person[Count].Yloc[Frame + 1] = (Person[Count].Yloc[Frame] + MovingY);
						Person[Count].Zloc[Frame + 1] = (Person[Count].Zloc[Frame] + MovingZ);
						Person[Count].location[0] = Person[Count].Xloc[Frame + 1];
						Person[Count].location[1] = Person[Count].Yloc[Frame + 1];
						Person[Count].location[2] = Person[Count].Zloc[Frame + 1];
						Person[Count].MaxX[Frame + 1] = (Person[Count].MaxX[Frame] + MovingX);
						Person[Count].MinX[Frame + 1] = (Person[Count].MinX[Frame] + MovingX);
						Person[Count].MaxY[Frame + 1] = (Person[Count].MaxY[Frame] + MovingY);
						Person[Count].MinY[Frame + 1] = (Person[Count].MinY[Frame] + MovingY);
						Person[Count].MaxZ[Frame + 1] = (Person[Count].MaxZ[Frame] + MovingZ);
						Person[Count].MinZ[Frame + 1] = (Person[Count].MaxZ[Frame] + MovingZ);
					}
				}
			}
		}
		else {
			Person[Count].Xloc[Frame + 1] = Person[Count].Xloc[Frame];
			Person[Count].Yloc[Frame + 1] = Person[Count].Yloc[Frame];
			Person[Count].Zloc[Frame + 1] = Person[Count].Zloc[Frame];
		}
	}
}
#endif

static void verifie_collision(
		Personne &personne,
		dls::tableau<Personne> const &personnes,
		dls::math::vec3f const &cible_globale)
{
	INUTILISE(cible_globale);

	for (auto &autre : personnes) {
		if (autre.id == personne.id) {
			continue;
		}


	}
}

class OperatriceSimFoule final : public OperatriceCorps {
	dls::tableau<Personne> m_personnes{};

public:
	static constexpr auto NOM = "Simulation Foule";
	static constexpr auto AIDE = "Simule une foule.";

	OperatriceSimFoule(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_simulation_foule.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte, DonneesAval *donnees_aval) override
	{
		INUTILISE(donnees_aval);
		m_corps.reinitialise();

		//auto melee = evalue_bool("méléee");
		//auto rend_cible = evalue_bool("rend_cible");
		auto const vitesse_min = evalue_decimal("vitesse_min");
		auto const vitesse_max = evalue_decimal("vitesse_max");
		auto const nom_progenitures = evalue_chaine("nom_progénitures");
		auto const nombre_progenitures = evalue_entier("nombre_progénitures");
		//auto const temps_debut = evalue_entier("temps_début");
		//auto const temps_fin = evalue_entier("temps_fin");

		auto rng = std::mt19937(1);
		auto dist_vitesse = std::uniform_real_distribution<float>(vitesse_min, vitesse_max);
		auto dist_location = std::uniform_real_distribution<float>(-10.0f, 10.0f);

		/* À FAIRE : prends objets en entrée */
		auto const cible_globale = dls::math::vec3f(0.0f, 1.0f, 0.0f);

		/* progénère */
		if (contexte.temps_courant == 1 || m_personnes.est_vide() || m_personnes.taille() != nombre_progenitures) {
			m_personnes.efface();
			m_personnes.reserve(nombre_progenitures);

			for (auto i = 0; i < nombre_progenitures; ++i) {
				auto personne = Personne{};
				personne.nom = nom_progenitures;
				personne.en_mouvement = true;
				personne.vitesse = dist_vitesse(rng);
				personne.location = dls::math::vec3f(
							dist_location(rng),
							1.0f,
							dist_location(rng));

				personne.min = personne.location - dls::math::vec3f(0.5f);
				personne.max = personne.location + dls::math::vec3f(0.5f);
				personne.id = static_cast<int>(m_personnes.taille());

				m_personnes.pousse(personne);
			}
		}

		/* intègre */
		if (contexte.temps_courant > 1) {
			for (auto &personne : m_personnes) {
				bouge(personne, cible_globale);
				verifie_collision(personne, m_personnes, cible_globale);

				personne.location += personne.parcouru;
				personne.parcouru = dls::math::vec3f(0.0f);
			}
		}

		auto attr_C = m_corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);

		dessine_boite(
					m_corps,
					attr_C,
					cible_globale - dls::math::vec3f(0.5f),
					cible_globale + dls::math::vec3f(0.5f),
					dls::math::vec3f(0.0f, 0.0f, 1.0f));

		for (auto &personne : m_personnes) {
			dessine_boite(
						m_corps,
						attr_C,
						personne.location - dls::math::vec3f(0.5f),
						personne.location + dls::math::vec3f(0.5f),
						dls::math::vec3f(0.0f, 1.0f, 0.0f));

		//	m_corps.ajoute_point(personne.location.x, personne.location.y, personne.location.z);
		}

		return EXECUTION_REUSSIE;
	}
};

/* ************************************************************************** */

void enregistre_operatrices_sim_foule(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceSimFoule>());
}

#pragma clang diagnostic pop
