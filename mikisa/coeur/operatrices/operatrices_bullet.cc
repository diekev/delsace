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

/* À FAIRE : stockage des objets créés, ajournement, libération mémoire */
class OperatriceDynCorpsRigide final : public OperatriceCorps {
	btDiscreteDynamicsWorld *m_monde_dynamics = nullptr;
	btDefaultCollisionConfiguration *m_configuration_collision = nullptr;
	btDispatcher *m_repartitrice = nullptr;
	btBroadphaseInterface *m_tampon_paires = nullptr;
	btConstraintSolver *m_solveur_constraintes = nullptr;

	/* À FAIRE : btOverlapFilterCallback, voir Blender. */

	btAlignedObjectArray<btCollisionShape *> m_formes_collisions{};

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

	~OperatriceDynCorpsRigide() override
	{
		supprime_monde();
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
		m_corps.reinitialise();
		entree(0)->requiers_copie_corps(&m_corps, contexte);

		/* À FAIRE : réinitialisation. */
		if (m_monde_dynamics == nullptr || contexte.temps_courant == 1) {
			initialise_monde();

			auto forme = cree_forme_pour_corps(m_corps);
			m_formes_collisions.push_back(forme);

			auto tranformation = converti_transformation(m_corps);
			auto masse = 1.0;

			cree_corps_rigide(masse, tranformation, forme);
		}

		if (contexte.temps_courant > 1) {
			m_monde_dynamics->stepSimulation(1.0 / 24.0);
		}

		ajourne_corps_depuis_sim();

		return EXECUTION_REUSSIE;
	}

	void ajourne_corps_depuis_sim()
	{
		for (int i = m_monde_dynamics->getNumCollisionObjects() - 1; i >= 0; i--) {
			btCollisionObject* obj = m_monde_dynamics->getCollisionObjectArray()[i];
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

	btRigidBody *cree_corps_rigide(double mass, btTransform const &transforme_initiale, btCollisionShape *forme_collision)
	{
		btAssert((!forme_collision || forme_collision->getShapeType() != INVALID_SHAPE_PROXYTYPE));

		auto intertie_locale = btVector3(0, 0, 0);

		/* Une masse différente de 0 définie un corps dynamique, c-à-d
		 * non-static. */
		if (mass != 0.0) {
			forme_collision->calculateLocalInertia(mass, intertie_locale);
		}

		/* L'utilisation de btDefaultMotionState est recommandée, car cela
		 * permet des interpolations, et synchronsie uniquement les objets
		 * 'actifs'. */
		auto etat_mouvement = new btDefaultMotionState(transforme_initiale);

		auto infos = btRigidBody::btRigidBodyConstructionInfo(
					mass,
					etat_mouvement,
					forme_collision,
					intertie_locale);

		auto corps_rigide = new btRigidBody(infos);
		corps_rigide->setUserIndex(-1);
		//corps_rigide->setContactProcessingThreshold(m_defaultContactProcessingThreshold);
		corps_rigide->setFriction(0.5); /* défaut bullet */
		corps_rigide->setRestitution(0.0);
		corps_rigide->setDamping(0.04, 0.1);
		corps_rigide->setSleepingThresholds(0.4, 0.5); /* moitié défaut bullet */
		corps_rigide->setActivationState(1);
		corps_rigide->setLinearFactor(btVector3(1, 1, 1)); /* 0 = vérouille position */
		corps_rigide->setAngularFactor(btVector3(1, 1, 1)); /* 0 = vérouille axes */

		/* cela désactive les mouvements pour quelque raison */
	//	corps_rigide->setCollisionFlags(corps_rigide->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);

		m_monde_dynamics->addRigidBody(corps_rigide);

		return corps_rigide;
	}

	/* ********************************************************************** */

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

void enregistre_operatrices_bullet(UsineOperatrice &usine)
{
	usine.enregistre_type(cree_desc<OperatriceDynCorpsRigide>());
}
