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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "alembic.h"

#include "abc_ipa_c.h"

namespace Abc = Alembic::Abc;

std::string vers_std_string(ChaineKuri chaine)
{
	return { chaine.pointeur, static_cast<size_t>(chaine.taille) };
}

#define VERS_POIGNEE_IOBJECT(x) reinterpret_cast<IObject *>((x))

#define DEPUIS_POIGNEE_IOBJECT(x) (reinterpret_cast<Abc::IObject *>(x))

template <typename T>
static bool apparie_entete(IObject *object)
{
	auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
	return T::matches(iobject->getHeader());
}

template <typename TypePoignee, typename TypeAlembic>
static TypePoignee *comme(IObject *object)
{
	auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
	auto poly = new TypeAlembic(*iobject);
	return reinterpret_cast<TypePoignee *>(poly);
}

extern "C" {

struct IArchive {
	Abc::IArchive archive{};
};

IArchive *ABC_ouvre_archive(ChaineKuri chemin)
{
	Alembic::AbcCoreFactory::IFactory factory;
	factory.setPolicy(Alembic::Abc::ErrorHandler::kQuietNoopPolicy);

	Abc::IArchive iarchive = factory.getArchive(vers_std_string(chemin));

	if (!iarchive.valid()) {
		return nullptr;
	}

	return new IArchive{iarchive};
}

void ABC_ferme_archive(IArchive *archive)
{
	delete archive;
}

IObject *ABC_archive_iobject_racine(IArchive *archive)
{
	auto iobject = new Abc::IObject(archive->archive.getTop());
	return VERS_POIGNEE_IOBJECT(iobject);
}

long ABC_iobject_nombre_enfants(IObject *object)
{
	auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
	return static_cast<long>(iobject->getNumChildren());
}

IObject *ABC_iobject_enfant_index(IObject *object, long index)
{
	auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
	auto ptr = new Abc::IObject(iobject->getChild(static_cast<size_t>(index)));
	return VERS_POIGNEE_IOBJECT(ptr);
}

IObject *ABC_iobject_enfant_nom(IObject *object, ChaineKuri nom)
{
	auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
	auto ptr = new Abc::IObject(iobject->getChild(vers_std_string(nom)));
	return VERS_POIGNEE_IOBJECT(ptr);
}

ChaineKuri ABC_iobject_nom(IObject *object)
{
	auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
	return { iobject->getName().c_str(), static_cast<long>(iobject->getName().size()) };
}

ChaineKuri ABC_iobject_chemin(IObject *object)
{
	auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
	return { iobject->getFullName().c_str(), static_cast<long>(iobject->getFullName().size()) };
}

void ABC_detruit_iobject(IObject *iobject)
{
	delete DEPUIS_POIGNEE_IOBJECT(iobject);
}

bool ABC_est_maillage(IObject *object)
{
	return apparie_entete<Alembic::AbcGeom::IPolyMesh>(object);
}

bool ABC_est_subdivision(IObject *object)
{
	return apparie_entete<Alembic::AbcGeom::ISubD>(object);
}

bool ABC_est_xform(IObject *object)
{
	return apparie_entete<Alembic::AbcGeom::IXform>(object);
}

bool ABC_est_points(IObject *object)
{
	return apparie_entete<Alembic::AbcGeom::IPoints>(object);
}

bool ABC_est_camera(IObject *object)
{
	return apparie_entete<Alembic::AbcGeom::ICamera>(object);
}

bool ABC_est_courbes(IObject *object)
{
	return apparie_entete<Alembic::AbcGeom::ICurves>(object);
}

IMaillage *ABC_comme_maillage(IObject *object)
{
	return comme<IMaillage, Alembic::AbcGeom::IPolyMesh>(object);
}

ISubdivision *ABC_comme_subdivision(IObject *object)
{
	return comme<ISubdivision, Alembic::AbcGeom::ISubD>(object);
}

IXform *ABC_comme_xform(IObject *object)
{
	return comme<IXform, Alembic::AbcGeom::IXform>(object);
}

IPoints *ABC_comme_points(IObject *object)
{
	return comme<IPoints, Alembic::AbcGeom::IPoints>(object);
}

ICamera *ABC_comme_camera(IObject *object)
{
	return comme<ICamera, Alembic::AbcGeom::ICamera>(object);
}

ICourbes *ABC_comme_courbes(IObject *object)
{
	return comme<ICourbes, Alembic::AbcGeom::ICurves>(object);
}

}
