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

#include "courbure.hh"

#include "bibliotheques/Patate/common/surface_mesh/surfaceMesh.h"
#include "bibliotheques/Patate/grenaille.h"

#include "../corps/corps.h"

using namespace Grenaille;

using Scalaire = double;
using Vecteur = Eigen::Matrix<Scalaire, 3, 1>;

class MaillagePatate : public PatateCommon::SurfaceMesh {
public:
	using Scalaire = ::Scalaire;
	using Vecteur  = ::Vecteur;

private:
	using PropScalaire = PatateCommon::SurfaceMesh::VertexProperty<Scalaire>;
	using PropVecteur  = PatateCommon::SurfaceMesh::VertexProperty<Vecteur>;

	PropVecteur m_positions{};
	PropVecteur m_normals{};
	PropVecteur m_minDirection{};
	PropVecteur m_maxDirection{};
	PropScalaire m_minCurvature{};
	PropScalaire m_maxCurvature{};
	PropScalaire m_geomVar{};

public:
	MaillagePatate()
	{
		m_positions    = addVertexProperty<Vecteur>("v:position",     Vecteur::Zero());
		m_normals      = addVertexProperty<Vecteur>("v:normals",      Vecteur::Zero());
		m_minDirection = addVertexProperty<Vecteur>("v:minDirection", Vecteur::Zero());
		m_maxDirection = addVertexProperty<Vecteur>("v:maxDirection", Vecteur::Zero());

		m_minCurvature = addVertexProperty<Scalaire>("v:minCurvature", 0);
		m_maxCurvature = addVertexProperty<Scalaire>("v:maxCurvature", 0);
		m_geomVar      = addVertexProperty<Scalaire>("v:geomVar",      0);
	}

	template <typename Derived>
	inline Vertex addVertex(const Eigen::DenseBase<Derived>& pos)
	{
		Vertex vx = PatateCommon::SurfaceMesh::addVertex();
		m_positions[vx] = pos;
		return vx;
	}

	inline const Vecteur& position(Vertex vx) const
	{
		return m_positions[vx];
	}

	inline Vecteur& position(Vertex vx)
	{
		return m_positions[vx];
	}

	inline const Vecteur& normal(Vertex vx) const
	{
		return m_normals[vx];
	}

	inline Vecteur& normal(Vertex vx)
	{
		return m_normals[vx];
	}

	inline const Vecteur& minDirection(Vertex vx) const
	{
		return m_minDirection[vx];
	}

	inline Vecteur& minDirection(Vertex vx)
	{
		return m_minDirection[vx];
	}

	inline const Vecteur& maxDirection(Vertex vx) const
	{
		return m_maxDirection[vx];
	}

	inline Vecteur& maxDirection(Vertex vx)
	{
		return m_maxDirection[vx];
	}

	inline Scalaire minCurvature(Vertex vx) const
	{
		return m_minCurvature[vx];
	}

	inline Scalaire& minCurvature(Vertex vx)
	{
		return m_minCurvature[vx];
	}

	inline Scalaire maxCurvature(Vertex vx) const
	{
		return m_maxCurvature[vx];
	}

	inline Scalaire& maxCurvature(Vertex vx)
	{
		return m_maxCurvature[vx];
	}

	inline Scalaire geomVar(Vertex vx) const
	{
		return m_geomVar[vx];
	}

	inline Scalaire& geomVar(Vertex vx)
	{
		return m_geomVar[vx];
	}

	void computeNormals()
	{
		for (auto vit = verticesBegin(); vit != verticesEnd(); ++vit) {
			normal(*vit) = Vecteur::Zero();
		}

		for (auto fit = facesBegin(); fit != facesEnd(); ++fit) {
			auto vit = vertices(*fit);
			auto const &p0 = position(*(++vit));
			auto const &p1 = position(*(++vit));
			auto const &p2 = position(*(++vit));

			auto n = (p1 - p0).cross(p2 - p0);

			auto vEnd = vit;
			do {
				normal(*vit) += n;
				++vit;
			} while (vit != vEnd);
		}

		for (auto vit = verticesBegin(); vit != verticesEnd(); ++vit) {
			normal(*vit).normalize();
		}
	}
};

using Vertex = MaillagePatate::Vertex;

/* Cette classe définie le format des données d'entrées. */
class GLSPoint {
	Vecteur m_pos{};
	Vecteur m_norm{};

public:
	enum {
		Dim = 3
	};

	using Scalar = ::Scalaire;
	using VectorType = Eigen::Matrix<Scalaire, Dim, 1>;
	using MatrixType = Eigen::Matrix<Scalaire, Dim, Dim>;

	MULTIARCH inline GLSPoint() = default;

	MULTIARCH inline GLSPoint(MaillagePatate const &mesh, Vertex vx)
		: m_pos(mesh.position(vx))
		, m_norm(mesh.normal(vx))
	{}

	MULTIARCH inline const VectorType &pos()    const
	{
		return m_pos;
	}

	MULTIARCH inline const VectorType &normal() const
	{
		return m_norm;
	}
};

using WeightFunc = Grenaille::DistWeightFunc<GLSPoint, Grenaille::SmoothWeightKernel<Scalaire> >;

using Fit = Basket<GLSPoint, WeightFunc, OrientedSphereFit, GLSParam,
			   OrientedSphereScaleSpaceDer, GLSDer, GLSCurvatureHelper, GLSGeomVar>;

class Grid {
public:
	using Cell = Eigen::Array3i;
	using CellBlock = Eigen::AlignedBox3i;
	using IndexVecteur = std::vector<unsigned>;
	using PointVecteur = std::vector<GLSPoint, Eigen::aligned_allocator<GLSPoint> >;

private:
	Scalaire      _cellSize{};
	Cell        _gridSize{};
	Vecteur      _gridBase{}; // min corner
	IndexVecteur _cellsIndex{};
	PointVecteur _points{};

public:
	Grid(MaillagePatate const &maillage, Scalaire taille_cellule)
		: _cellSize(taille_cellule)
		, _gridSize()
		, _gridBase()
	{
		// Compute bounding box.
		using BoundingBox = Eigen::AlignedBox<Scalaire, 3>;

		BoundingBox bb;
		for (auto vit = maillage.verticesBegin(); vit != maillage.verticesEnd(); ++vit) {
			bb.extend(maillage.position(*vit));
		}

		_gridBase = bb.min();
		_gridSize = gridCoord(bb.max()) + Cell::Constant(1);

		std::cout << "GridSize: " << _gridSize.transpose()
				  << " (" << nCells() << ")\n";

		// Compute cells size (number of points in each cell)
		_cellsIndex.resize(nCells() + 1, 0);
		for (auto vit = maillage.verticesBegin(); vit != maillage.verticesEnd(); ++vit) {
			auto i = static_cast<unsigned>(cellIndex(maillage.position(*vit)));
			++_cellsIndex[i + 1];
		}

		// Prefix sum to get indices
		for (unsigned i = 1; i <= nCells(); ++i) {
			_cellsIndex[i] += _cellsIndex[i - 1];
		}

		// Fill the point array
		IndexVecteur pointCount(nCells(), 0);
		_points.resize(_cellsIndex.back());
		for (auto vit = maillage.verticesBegin(); vit != maillage.verticesEnd(); ++vit) {
			auto i = static_cast<unsigned>(cellIndex(maillage.position(*vit)));
			*(_pointsBegin(i) + pointCount[i]) = GLSPoint(maillage, *vit);
			++pointCount[i];
		}

		for (unsigned i = 0; i < nCells(); ++i) {
			assert(pointCount[i] == _cellsIndex[i+1] - _cellsIndex[i]);
		}
	}

	inline unsigned nCells() const
	{
		return static_cast<unsigned>(_gridSize.prod());
	}

	inline Cell gridCoord(const Vecteur& p) const
	{
		return Eigen::Array3i(((p - _gridBase) / _cellSize).cast<int>());
	}

	inline int cellIndex(const Cell& c) const
	{
		return c(0) + _gridSize(0) * (c(1) + _gridSize(1) * c(2));
	}

	inline int cellIndex(const Vecteur& p) const
	{
		Cell c = gridCoord(p);
		assert(c(0) >= 0 && c(1) >= 0 && c(2) >= 0
			&& c(0) < _gridSize(0) && c(1) < _gridSize(1) && c(2) < _gridSize(2));
		return c(0) + _gridSize(0) * (c(1) + _gridSize(1) * c(2));
	}

	inline const GLSPoint* pointsBegin(unsigned ci) const
	{
		assert(ci < nCells());
		return &_points[0] + _cellsIndex[ci];
	}

	inline const GLSPoint* pointsEnd(unsigned ci) const
	{
		assert(ci < nCells());
		return &_points[0] + _cellsIndex[ci + 1];
	}

	CellBlock cellsAround(const Vecteur& p, Scalaire radius)
	{
		return CellBlock(gridCoord(p - Vecteur::Constant(radius))
								 .max(Cell(0, 0, 0)).matrix(),
						 (gridCoord(p + Vecteur::Constant(radius)) + Cell::Constant(1))
								 .min(_gridSize).matrix());
	}

private:
	inline GLSPoint* _pointsBegin(unsigned ci)
	{
		assert(ci < nCells());
		return &_points[0] + _cellsIndex[ci];
	}
};

static auto calcul_courbure(MaillagePatate &maillage, Scalaire radius)
{
	auto donnees_ret = DonneesCalculCourbure{};

	auto const r = Scalaire(2) * radius;
	auto const r2 = r * r;
	auto const nombre_sommets = maillage.verticesSize();

	auto courbure_max = Scalaire(0);

	auto grid = Grid(maillage, r);

	/* À FAIRE : multithreading, courbure_max -> reduction parallèle. */
	for (unsigned i = 0; i < nombre_sommets; ++i) {
		auto v0 = Vertex(static_cast<int>(i));

		if (maillage.isDeleted(v0)) {
			continue;
		}

		auto p0 = maillage.position(v0);

		Fit fit;
		fit.init(p0);
		fit.setWeightFunc(WeightFunc(r));

		auto nb_voisins = 0;

		auto cells = grid.cellsAround(p0, r);
		Grid::Cell c = cells.min().array();

		for (c(2) = cells.min()(2); c(2) < cells.max()(2); ++c(2)) {
			for (c(1) = cells.min()(1); c(1) < cells.max()(1); ++c(1)) {
				for (c(0) = cells.min()(0); c(0) < cells.max()(0); ++c(0)) {
					auto p = grid.pointsBegin(static_cast<unsigned>(grid.cellIndex(c)));
					auto end = grid.pointsEnd(static_cast<unsigned>(grid.cellIndex(c)));

					for (; p != end; ++p) {
						if ((p->pos() - p0).squaredNorm() < r2) {
							fit.addNeighbor(*p);
							++nb_voisins;
						}
					}
				}
			}
		}

		fit.finalize();

		if (fit.isReady()) {
			if (!fit.isStable()) {
				++donnees_ret.nombre_instable;
			}

			maillage.maxDirection(v0) = fit.GLSk1Direction();
			maillage.minDirection(v0) = fit.GLSk2Direction();
			maillage.maxCurvature(v0) = fit.GLSk1();
			maillage.minCurvature(v0) = fit.GLSk2();
			maillage.geomVar(v0)      = fit.geomVar();

			auto courbure = std::abs(fit.GLSGaussianCurvature());

			if (courbure > courbure_max) {
				courbure_max = courbure;
			}
		}
		else {
			++donnees_ret.nombre_impossible;

			maillage.maxDirection(v0) = Vecteur::Zero();
			maillage.minDirection(v0) = Vecteur::Zero();
			maillage.maxCurvature(v0) = std::numeric_limits<Scalaire>::quiet_NaN();
			maillage.minCurvature(v0) = std::numeric_limits<Scalaire>::quiet_NaN();
			maillage.geomVar(v0)      = std::numeric_limits<Scalaire>::quiet_NaN();
		}
	}

	donnees_ret.courbure_max = courbure_max;

	return donnees_ret;
}

Vecteur getColor(Scalaire valeur, Scalaire valeur_min, Scalaire valeur_max)
{
	auto c = Vecteur(1.0, 1.0, 1.0);

	if (valeur == 0.) {
		return c;
	}

	if (std::isnan(valeur)) {
		return Vecteur(0.0, 1.0, 0.0);
	}

	if (valeur < valeur_min) {
		valeur = valeur_min;
	}

	if (valeur > valeur_max) {
		valeur = valeur_max;
	}

	auto dv = valeur_max - valeur_min;

	if (valeur < (valeur_min + 0.5 * dv)) {
		c(0) = 2 * (valeur - valeur_min) / dv;
		c(1) = 2 * (valeur - valeur_min) / dv;
		c(2) = 1;
	}
	else {
		c(2) = 2 - 2 * (valeur - valeur_min) / dv;
		c(1) = 2 - 2 * (valeur - valeur_min) / dv;
		c(0) = 1;
	}

	return c;
}

Vecteur getColor2(Scalaire value, Scalaire p)
{
	auto sign = value < 0;
	value = 2 * std::min(std::max(std::pow(std::abs(value), p), Scalaire(0)), Scalaire(1));
	return Vecteur(std::abs(1 - value),
				  std::min(2 - value, Scalaire(1)),
				  Scalaire(sign));
}

DonneesCalculCourbure calcule_courbure(
		Corps &corps,
		bool relatif,
		double rayon,
		double courbure_max)
{
	auto points_entree = corps.points();
	auto prims_entree = corps.prims();

	/* Conversion du maillage en représentation Grenaille */
	auto maillage = MaillagePatate{};

	for (auto i = 0; i < points_entree->taille(); ++i) {
		auto point = points_entree->point(i);
		maillage.addVertex(Vecteur(static_cast<double>(point.x), static_cast<double>(point.y), static_cast<double>(point.z)));
	}

	auto sommets_face = std::vector<Vertex>{};

	for (auto i = 0; i < prims_entree->taille(); ++i) {
		auto prim = prims_entree->prim(i);

		if (prim->type_prim() != type_primitive::POLYGONE) {
			continue;
		}

		auto poly = dynamic_cast<Polygone *>(prim);

		if (poly->type != type_polygone::FERME) {
			continue;
		}

		sommets_face.clear();

		for (auto j = 0; j < poly->nombre_sommets(); ++j) {
			sommets_face.push_back(Vertex(static_cast<int>(poly->index_point(j))));
		}

		maillage.addFace(sommets_face);
	}

	maillage.triangulate();
	maillage.computeNormals(); /* À FAIRE : déplace du coté de Mikisa. */

	if (relatif) {
		auto barycentre_maillage = Vecteur(0.0, 0.0, 0.0);
		auto rayon_maillage = 0.0;

		auto vend = maillage.verticesEnd();
		for (auto vit = maillage.verticesBegin(); vit != vend; ++vit) {
			barycentre_maillage += maillage.position(*vit);
		}

		barycentre_maillage /= maillage.nVertices();

		for (auto vit = maillage.verticesBegin(); vit != vend; ++vit) {
			auto dist = (maillage.position(*vit) - barycentre_maillage).norm();

			if (dist > rayon_maillage) {
				rayon_maillage = dist;
			}
		}

		rayon *= rayon_maillage;
	}

	auto donnees_ret = calcul_courbure(maillage, rayon);

	if (courbure_max == 0.0) {
		courbure_max = donnees_ret.courbure_max;
	}

	/* transfère de la courbure sous forme de couleur */

	maillage.garbageCollection();

	auto attr_C = corps.ajoute_attribut("C", type_attribut::VEC3, portee_attr::POINT);
	attr_C->redimensionne(points_entree->taille());

	for (auto vit = maillage.verticesBegin(); vit != maillage.verticesEnd(); ++vit) {
		Scalaire curv = maillage.minCurvature(*vit) * maillage.maxCurvature(*vit);
		//            Vecteur color = getColor(curv, -maxCurv, maxCurv);
		Vecteur color = getColor2(curv / courbure_max, .5);

		attr_C->vec3((*vit).idx(), dls::math::vec3f(static_cast<float>(color[0]), static_cast<float>(color[1]), static_cast<float>(color[2])));
	}

	return donnees_ret;
}
