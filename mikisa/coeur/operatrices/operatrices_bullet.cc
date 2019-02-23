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

#include "operatrices_bullet.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#define BT_THREADSAFE 1
#define BT_USE_DOUBLE_PRECISION
#include <bullet/btBulletDynamicsCommon.h>
#include <bullet/LinearMath/btConvexHullComputer.h>
#pragma GCC diagnostic pop

#include <delsace/math/quaternion.hh>

#include "bibliotheques/outils/definitions.hh"

#include "../contexte_evaluation.hh"
#include "../donnees_simulation.hh"
#include "../operatrice_corps.h"
#include "../usine_operatrice.h"

/* ************************************************************************** */

static btCollisionShape *cree_forme_pour_corps(Corps &corps)
{
	/* calcul de la boite englobante du corps */
	auto verts = std::vector<dls::math::vec3f>{};
	verts.reserve(static_cast<size_t>(corps.points()->taille()));

	auto min = dls::math::vec3f( std::numeric_limits<float>::max());
	auto max = dls::math::vec3f(-std::numeric_limits<float>::min());
	for (auto i = 0; i < corps.points()->taille(); ++i) {
		auto point = corps.points()->point(i);
		extrait_min_max(point, min, max);
		verts.push_back(point);
	}

	auto taille = max - min;
	auto colShape = new btBoxShape(btVector3(
									   static_cast<double>(taille.x * 0.5f),
									   static_cast<double>(taille.y * 0.5f),
									   static_cast<double>(taille.z * 0.5f)));

//	auto has_volume = std::min(taille.x, std::min(taille.y, taille.z)) > 0.0f;
//	auto hull_margin = 0.04;

//	/* conversion des représentations */
//	auto hull_computer = btConvexHullComputer();
//	auto verts_ptr = reinterpret_cast<float *>(verts.data());
//	auto stride = static_cast<int>(sizeof(dls::math::vec3f));
//	auto count = static_cast<int>(corps.points()->taille());

//	if (hull_computer.compute(verts_ptr, stride, count, hull_margin, 0.0) < 0.0) {
//		hull_computer.compute(verts_ptr, stride, count, 0.0, 0.0);
//	}

//	auto hull_shape = new btConvexHullShape(&(hull_computer.vertices[0].getX()), hull_computer.vertices.size());

//	auto forme = new rbCollisionShape();
//	forme->cshape = hull_shape;
//	forme->cshape->setMargin(hull_margin);
//	forme->mesh = nullptr;

	return colShape;
}

static btTransform converti_transformation(Corps &corps)
{
	dls::math::vec3d loc;
	dls::math::quaternion<double> rot;

	loc_quat_depuis_mat4(corps.transformation.matrice(), loc, rot);

	btTransform trans;
	trans.setOrigin(btVector3(loc.x, loc.y, loc.z));
	/* Bullet utilise un système droitier pour les matrices, donc réarrange les
	 * éléments pour être correcte. */
	trans.setRotation(btQuaternion(rot.vecteur.y, rot.vecteur.z, rot.poids, rot.vecteur.x));

	return trans;
}

/* ************************************************************************** */

class MondePhysique {
	btDiscreteDynamicsWorld *m_monde_dynamics = nullptr;
	btDefaultCollisionConfiguration *m_configuration_collision = nullptr;
	btDispatcher *m_repartitrice = nullptr;
	btBroadphaseInterface *m_tampon_paires = nullptr;
	btConstraintSolver *m_solveur_constraintes = nullptr;

	/* À FAIRE : btOverlapFilterCallback, voir Blender. */

	btAlignedObjectArray<btCollisionShape *> m_formes_collisions{};

public:
	~MondePhysique()
	{
		supprime_monde();
	}

	void ajoute_forme(btCollisionShape *forme)
	{
		m_formes_collisions.push_back(forme);
	}

	void ajoute_corps_rigide(btRigidBody *corps_rigide)
	{
		m_monde_dynamics->addRigidBody(corps_rigide);
	}

	btDiscreteDynamicsWorld *ptr()
	{
		return m_monde_dynamics;
	}

	void initialise_monde()
	{
		if (m_monde_dynamics != nullptr) {
			supprime_monde();
		}

		/* La configuration de collision contiens les réglages de base pour la
		 * mémoire, et configuration de collision. */
		m_configuration_collision = new btDefaultCollisionConfiguration();
		//m_configuration_collision->setConvexConvexMultipointIterations();

		/* Utilisation de la répartitrice par défaut.
		 * Pour les procès parallèles, on peut utiliser une autre répartitrice.
		 * Voir Extras/BulletMultiThreaded. */
		m_repartitrice = new btCollisionDispatcher(m_configuration_collision);

		m_tampon_paires = new btDbvtBroadphase();

		/* Utilisation du solveur par défaut.
		 * Pour les procès parallèles, on peut utiliser un autre solveur.
		 * Voir Extras/BulletMultiThreaded. */
		m_solveur_constraintes = new btSequentialImpulseConstraintSolver();

		m_monde_dynamics = new btDiscreteDynamicsWorld(
					m_repartitrice,
					m_tampon_paires,
					m_solveur_constraintes,
					m_configuration_collision);
	}

	void supprime_monde()
	{
		for (int i = m_monde_dynamics->getNumCollisionObjects() - 1; i >= 0; i--) {
			auto obj = m_monde_dynamics->getCollisionObjectArray()[i];
			auto body = btRigidBody::upcast(obj);

			if (body && body->getMotionState()) {
				delete body->getMotionState();
			}

			m_monde_dynamics->removeCollisionObject(obj);
			delete obj;
		}

		for (int j = 0; j < m_formes_collisions.size(); j++) {
			auto shape = m_formes_collisions[j];
			delete shape;
		}

		m_formes_collisions.clear();

		/* supprime données monde */
		delete m_solveur_constraintes;
		delete m_tampon_paires;
		delete m_repartitrice;
		delete m_configuration_collision;
		delete m_monde_dynamics;
	}
};

/* ************************************************************************** */

class AjoutCorpsRigide final : public OperatriceCorps {
public:
	static constexpr auto NOM = "Ajout Corps Rigide";
	static constexpr auto AIDE = "";

	AjoutCorpsRigide(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(2);
		sorties(1);
	}

	const char *chemin_entreface() const override
	{
		return "entreface/operatrice_ajout_corps_rigide.jo";
	}

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte) override
	{
		/* L'entrée 0 est pour le corps. */
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte);

		/* L'entrée 1 est pour accumuler les corps. */
		entree(1)->requiers_corps(contexte);

		auto monde = std::any_cast<MondePhysique *>(m_donnees_simulation->table["monde_physique"]);

		auto forme = cree_forme_pour_corps(m_corps);
		monde->ajoute_forme(forme);

		auto tranformation = converti_transformation(m_corps);

		auto corps_rigide = cree_corps_rigide(tranformation, forme);

		monde->ajoute_corps_rigide(corps_rigide);

		return EXECUTION_REUSSIE;
	}

	btRigidBody *cree_corps_rigide(btTransform const &transforme_initiale, btCollisionShape *forme_collision)
	{
		btAssert((!forme_collision || forme_collision->getShapeType() != INVALID_SHAPE_PROXYTYPE));

		auto intertie_locale = btVector3(0, 0, 0);

		/* Une masse différente de 0 définie un corps dynamique, c-à-d
		 * non-static. */
		auto masse = static_cast<double>(evalue_decimal("masse"));

		if (masse != 0.0) {
			forme_collision->calculateLocalInertia(masse, intertie_locale);
		}

		/* L'utilisation de btDefaultMotionState est recommandée, car cela
		 * permet des interpolations, et synchronsie uniquement les objets
		 * 'actifs'. */
		auto etat_mouvement = new btDefaultMotionState(transforme_initiale);

		auto infos = btRigidBody::btRigidBodyConstructionInfo(
					masse,
					etat_mouvement,
					forme_collision,
					intertie_locale);

		auto const friction = evalue_decimal("friction");
		auto const restitution = evalue_decimal("restitution");
		auto const amort_lin = evalue_decimal("amort_lin");
		auto const amort_ang = evalue_decimal("amort_ang");
		auto const seuil_lin = evalue_decimal("seuil_lin");
		auto const seuil_ang = evalue_decimal("seuil_ang");

		auto corps_rigide = new btRigidBody(infos);
		corps_rigide->setUserIndex(-1);
		//corps_rigide->setContactProcessingThreshold(m_defaultContactProcessingThreshold);
		corps_rigide->setFriction(static_cast<double>(friction));
		corps_rigide->setRestitution(static_cast<double>(restitution));
		corps_rigide->setDamping(
					static_cast<double>(amort_lin),
					static_cast<double>(amort_ang));
		corps_rigide->setSleepingThresholds(
					static_cast<double>(seuil_lin),
					static_cast<double>(seuil_ang));
		corps_rigide->setActivationState(1);
		corps_rigide->setLinearFactor(btVector3(1, 1, 1)); /* 0 = vérouille position */
		corps_rigide->setAngularFactor(btVector3(1, 1, 1)); /* 0 = vérouille axes */

		/* cela désactive les mouvements pour quelque raison */
	//	corps_rigide->setCollisionFlags(corps_rigide->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

		return corps_rigide;
	}
};

/* ************************************************************************** */

/* À FAIRE : stockage des objets créés, ajournement, libération mémoire */
class OperatriceDynCorpsRigide final : public OperatriceCorps {
	MondePhysique m_monde{};

public:
	static constexpr auto NOM = "Dynamiques Corps Rigides";
	static constexpr auto AIDE = "";

	explicit OperatriceDynCorpsRigide(Graphe &graphe_parent, Noeud *noeud)
		: OperatriceCorps(graphe_parent, noeud)
	{
		entrees(1);
		sorties(1);
	}

	OperatriceDynCorpsRigide(OperatriceDynCorpsRigide const &) = default;
	OperatriceDynCorpsRigide &operator=(OperatriceDynCorpsRigide const &) = default;

	const char *nom_classe() const override
	{
		return NOM;
	}

	const char *texte_aide() const override
	{
		return AIDE;
	}

	int execute(ContexteEvaluation const &contexte) override
	{
		m_donnees_simulation->table.insert({ "monde_physique", &m_monde });

		/* À FAIRE : réinitialisation. */
		if (m_monde.ptr() == nullptr || contexte.temps_courant == m_donnees_simulation->temps_debut) {
			m_monde.initialise_monde();

			/* il faut exécuter les noeuds en amont après avoir initialisé la table */
			m_corps.reinitialise();
			entree(0)->requiers_copie_corps(&m_corps, contexte);
		}

		if (contexte.temps_courant > m_donnees_simulation->temps_debut) {
			m_monde.ptr()->stepSimulation(1.0 / contexte.cadence);
		}

		ajourne_corps_depuis_sim();

		return EXECUTION_REUSSIE;
	}

	void ajourne_corps_depuis_sim()
	{
		/* À FAIRE :
		 * - pour modifier les matrices pour chaque objet, il vaudrait mieux
		 *   pouvoir accéder aux corps des objets via des pointeurs. */
		for (int i = 0; i < m_monde.ptr()->getNumCollisionObjects(); i++) {
			btCollisionObject* obj = m_monde.ptr()->getCollisionObjectArray()[i];
			btRigidBody* body = btRigidBody::upcast(obj);
			auto ms = body->getMotionState();

			if (ms != nullptr) {
				btTransform trans;
				ms->getWorldTransform(trans);

				double mat[4][4];
				trans.getOpenGLMatrix(reinterpret_cast<double *>(mat));
				auto transformation = math::transformation(mat);
				m_corps.transformation = transformation;
			}
		}
	}
};

/* ************************************************************************** */

void enregistre_operatrices_bullet(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<AjoutCorpsRigide>());
	usine.enregistre_type(cree_desc<OperatriceDynCorpsRigide>());
}
