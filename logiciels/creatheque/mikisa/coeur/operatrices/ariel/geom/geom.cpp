// TAKUA Render: Physically Based Renderer
// Written by Yining Karl Li
//
// File: geom.cpp
// Implements geom.hpp

#include "geom.hpp"

namespace geomCore {

//====================================
// Geom Class
//====================================

Geom::Geom()
{
    SetContents(nullptr);
}

Geom::Geom(GeomInterface* geom)
{
    SetContents(geom);
}

void Geom::SetContents(GeomInterface* geom)
{
    m_geom = geom;
}

void Geom::Intersect(const rayCore::Ray& r, spaceCore::TraverseAccumulator& result)
{
    m_geom->Intersect(r, result);
}

GeomType Geom::GetType()
{
    return m_geom->GetType();
}

//====================================
// GeomTransform Class
//====================================

GeomTransform::GeomTransform()
{
	SetContents(dls::math::vec3f(0.0f), dls::math::vec3f(0.0f), dls::math::vec3f(1.0f));
}

GeomTransform::GeomTransform(const dls::math::vec3f& t, const dls::math::vec3f& r, const dls::math::vec3f& s)
{
    SetContents(t, r, s);
}

void GeomTransform::SetContents(const dls::math::vec3f& t, const dls::math::vec3f& r, const dls::math::vec3f& s) {
    m_translation = t;
    m_rotation = r;
    m_scale = s;
}
}
