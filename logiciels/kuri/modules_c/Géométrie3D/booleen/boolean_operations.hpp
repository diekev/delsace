// Copyright (c) 2012-2019 University of Lyon and CNRS (France).
// All rights reserved.
//
// This file is part of MEPP2; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 3 of
// the License, or (at your option) any later version.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

#pragma once

#include "boolops_polyhedra.hpp"

namespace FEVV {
namespace Filters {

//--------------------- UNION -------------------------

/**
 * \brief  Computes the union of two polyhedra.
 *
 *         Ref: "Exact and Efficient Booleans for Polyhedra", C. Leconte,
 *              H. Barki, F. Dupont, Rapport de recherche RR-LIRIS-2010-018,
 *              2010
 *
 * \param  gA      1st input mesh
 * \param  gB      2nd input mesh
 * \param  g_out   output mesh
 */
void boolean_union(EnrichedPolyhedron &gA, EnrichedPolyhedron &gB, EnrichedPolyhedron &g_out)
{
    BoolPolyhedra(gA, gB, g_out, UNION);
}

//--------------------- INTERSECTION -------------------------

/**
 * \brief  Computes the intersection of two polyhedra.
 *
 *         Ref: "Exact and Efficient Booleans for Polyhedra", C. Leconte,
 *              H. Barki, F. Dupont, Rapport de recherche RR-LIRIS-2010-018,
 *              2010
 *
 * \param  gA      1st input mesh
 * \param  gB      2nd input mesh
 * \param  g_out   output mesh
 */
void boolean_inter(EnrichedPolyhedron &gA, EnrichedPolyhedron &gB, EnrichedPolyhedron &g_out)
{
    BoolPolyhedra(gA, gB, g_out, INTER);
}

//--------------------- SUBTRACTION -------------------------

/**
 * \brief  Computes the subtraction of two polyhedra.
 *
 *         Ref: "Exact and Efficient Booleans for Polyhedra", C. Leconte,
 *              H. Barki, F. Dupont, Rapport de recherche RR-LIRIS-2010-018,
 *              2010
 *
 * \param  gA      1st input mesh
 * \param  gB      2nd input mesh
 * \param  g_out   output mesh
 */
void boolean_minus(EnrichedPolyhedron &gA, EnrichedPolyhedron &gB, EnrichedPolyhedron &g_out)
{
    BoolPolyhedra(gA, gB, g_out, MINUS);
}

}  // namespace Filters
}  // namespace FEVV
