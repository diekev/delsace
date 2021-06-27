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

#include <string_view>

#include "abc_ipa_c.h"
#include "alembic_types.h"

/* Structures non-portées
 * - GeomParams
 * - Metadata
 * - Array
 * - TimeSampling
 * - DataType
 * - SchemaType::Sample
 */

namespace Abc = Alembic::Abc;
namespace AbcGeom = Alembic::AbcGeom;
namespace AbcCoreFactory = Alembic::AbcCoreFactory;

std::string vers_std_string(ChaineKuri chaine)
{
    return {chaine.pointeur, static_cast<size_t>(chaine.taille)};
}

ChaineKuri vers_chaine_kuri(const std::string &str)
{
    return {str.c_str(), static_cast<long>(str.size())};
}

inline const EntetePropriete *depuis_alembic(const Abc::PropertyHeader *prop_header)
{
    return reinterpret_cast<const EntetePropriete *>(prop_header);
}

inline Abc::PropertyHeader *vers_alembic(EntetePropriete *entete)
{
    return reinterpret_cast<Abc::PropertyHeader *>(entete);
}

inline const EchantillonageTemps *depuis_alembic(const Abc::TimeSampling *prop_header)
{
    return reinterpret_cast<const EchantillonageTemps *>(prop_header);
}

inline Abc::TimeSampling *vers_alembic(EchantillonageTemps *entete)
{
    return reinterpret_cast<Abc::TimeSampling *>(entete);
}

inline const EnteteIObject *depuis_alembic(const AbcGeom::ObjectHeader *header)
{
    return reinterpret_cast<const EnteteIObject *>(header);
}

inline Abc::ObjectHeader *vers_alembic(EnteteIObject *entete)
{
    return reinterpret_cast<Abc::ObjectHeader *>(entete);
}

template <typename TypeAlembic>
struct BaseEchant {
    static TypeAlembic *vers_alembic(BaseEchant *echant)
    {
        return reinterpret_cast<TypeAlembic *>(echant);
    }

    static BaseEchant *depuis_alembic(TypeAlembic *echant)
    {
        return reinterpret_cast<BaseEchant *>(echant);
    }

    static TypeAlembic *cree_alembic()
    {
        return new TypeAlembic();
    }
};

using IEchantPoints = BaseEchant<AbcGeom::IPointsSchema::Sample>;
using IEchantCamera = BaseEchant<AbcGeom::CameraSample>;
using IEchantMaillage = BaseEchant<AbcGeom::IPolyMeshSchema::Sample>;
using IEchantSubdivision = BaseEchant<AbcGeom::ISubDSchema::Sample>;
using IEchantCourbes = BaseEchant<AbcGeom::ICurvesSchema::Sample>;
using IEchantGroupePoly = BaseEchant<AbcGeom::IFaceSetSchema::Sample>;
using IEchantXform = BaseEchant<AbcGeom::XformSample>;
// pas d'échantillon pour lumière
// pas d'échantillon pour matériau

#define VERS_POIGNEE_IOBJECT(x) reinterpret_cast<IObject *>((x.getPtr().get()))

#define DEPUIS_POIGNEE_IOBJECT(x)                                                                 \
    (Abc::IObject(reinterpret_cast<Abc::ObjectReader *>(x)->asObjectPtr()))

template <typename T>
struct InterfacePoignee;

template <>
struct InterfacePoignee<ICamera> {
    using TypePoignee = ICamera;
    using TypePoigneeSchema = ISchemaCamera;
    using TypeAlembic = Alembic::AbcGeom::ICamera;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<ICourbes> {
    using TypePoignee = ICourbes;
    using TypePoigneeSchema = ISchemaCourbes;
    using TypeAlembic = Alembic::AbcGeom::ICurves;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<IGroupePoly> {
    using TypePoignee = IGroupePoly;
    using TypePoigneeSchema = ISchemaGroupePoly;
    using TypeAlembic = Alembic::AbcGeom::IFaceSet;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<ILumiere> {
    using TypePoignee = ILumiere;
    using TypePoigneeSchema = ISchemaLumiere;
    using TypeAlembic = Alembic::AbcGeom::ILight;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<IMaillage> {
    using TypePoignee = IMaillage;
    using TypePoigneeSchema = ISchemaMaillage;
    using TypeAlembic = Alembic::AbcGeom::IPolyMesh;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<IMateriau> {
    using TypePoignee = IMateriau;
    using TypePoigneeSchema = ISchemaMateriau;
    using TypeAlembic = Alembic::AbcMaterial::IMaterial;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<IPoints> {
    using TypePoignee = IPoints;
    using TypePoigneeSchema = ISchemaPoints;
    using TypeAlembic = Alembic::AbcGeom::IPoints;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<ISubdivision> {
    using TypePoignee = ISubdivision;
    using TypePoigneeSchema = ISchemaSubdivision;
    using TypeAlembic = Alembic::AbcGeom::ISubD;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<IXform> {
    using TypePoignee = IXform;
    using TypePoigneeSchema = ISchemaXform;
    using TypeAlembic = Alembic::AbcGeom::IXform;
    using TypeSchema = TypeAlembic::schema_type;
};

template <>
struct InterfacePoignee<ISchemaMaillage> {
    using TypeAlembic = Alembic::AbcGeom::IPolyMeshSchema;
    using TypeEchant = IEchantMaillage;
};

template <>
struct InterfacePoignee<ISchemaCamera> {
    using TypeAlembic = Alembic::AbcGeom::ICameraSchema;
    using TypeEchant = IEchantCamera;
};

template <>
struct InterfacePoignee<ISchemaCourbes> {
    using TypeAlembic = Alembic::AbcGeom::ICurvesSchema;
    using TypeEchant = IEchantCourbes;
};

template <>
struct InterfacePoignee<ISchemaGroupePoly> {
    using TypeAlembic = Alembic::AbcGeom::IFaceSetSchema;
    using TypeEchant = IEchantGroupePoly;
};

template <>
struct InterfacePoignee<ISchemaLumiere> {
    using TypeAlembic = Alembic::AbcGeom::ILightSchema;
};

template <>
struct InterfacePoignee<ISchemaMateriau> {
    using TypeAlembic = Alembic::AbcMaterial::IMaterialSchema;
};

template <>
struct InterfacePoignee<ISchemaPoints> {
    using TypeAlembic = Alembic::AbcGeom::IPointsSchema;
    using TypeEchant = IEchantPoints;
};

template <>
struct InterfacePoignee<ISchemaSubdivision> {
    using TypeAlembic = Alembic::AbcGeom::ISubDSchema;
    using TypeEchant = IEchantSubdivision;
};

template <>
struct InterfacePoignee<ISchemaXform> {
    using TypeAlembic = Alembic::AbcGeom::IXformSchema;
    using TypeEchant = IEchantXform;
};

/* Mise en cache de tous les objets accédés, car Alembic nous donne accès aux
 * objets via des pointeurs partagés, et il nous faut que ces pointeurs restent
 * actifs plus longtemps que les fonctions d'interface. */
struct CacheObjet {
private:
    std::map<std::string, Abc::IObject> iobjects{};

    template <typename T>
    using type_map = std::map<T *, typename InterfacePoignee<T>::TypeAlembic>;

    type_map<ICamera> icameras{};
    type_map<ICourbes> icourbes{};
    type_map<IGroupePoly> igoupes_poly{};
    type_map<ILumiere> ilumieres{};
    type_map<IMaillage> imaillages{};
    type_map<IMateriau> imateriaux{};
    type_map<IPoints> ipoints{};
    type_map<ISubdivision> isubdivision{};
    type_map<IXform> ixforms{};

public:
    Abc::IObject ajoute_iobjet(Abc::IObject iobjet)
    {
        auto iter = iobjects.find(iobjet.getFullName());

        if (iter != iobjects.end()) {
            return iter->second;
        }

        iobjects.insert({iobjet.getFullName(), iobjet});
        return iobjet;
    }

    void supprime_objet(Abc::IObject iobject)
    {
        iobjects.erase(iobject.getFullName());
    }

#define AJOUTE_REQUIERS_TYPE(TypePoignee, map)                                                    \
    void ajoute(TypePoignee *pointeur, InterfacePoignee<TypePoignee>::TypeAlembic abc_geom)       \
    {                                                                                             \
    map.insert({pointeur, abc_geom});                                                         \
}                                                                                             \
    InterfacePoignee<TypePoignee>::TypeAlembic alembic_geom(TypePoignee *pointeur)                \
    {                                                                                             \
    return map[pointeur];                                                                     \
}

    AJOUTE_REQUIERS_TYPE(ICamera, icameras)
    AJOUTE_REQUIERS_TYPE(ICourbes, icourbes)
    AJOUTE_REQUIERS_TYPE(IGroupePoly, igoupes_poly)
    AJOUTE_REQUIERS_TYPE(ILumiere, ilumieres)
    AJOUTE_REQUIERS_TYPE(IMaillage, imaillages)
    AJOUTE_REQUIERS_TYPE(IMateriau, imateriaux)
    AJOUTE_REQUIERS_TYPE(IPoints, ipoints)
    AJOUTE_REQUIERS_TYPE(ISubdivision, isubdivision)
    AJOUTE_REQUIERS_TYPE(IXform, ixforms)
};

static CacheObjet cache_global;

static IProprieteComposee *depuis_alembic(Alembic::AbcGeom::ICompoundProperty prop)
{
    return reinterpret_cast<IProprieteComposee *>(prop.getPtr().get());
}

static Alembic::AbcGeom::ICompoundProperty vers_alembic(IProprieteComposee *p_prop)
{
    auto ptr = reinterpret_cast<Alembic::AbcGeom::CompoundPropertyReader *>(p_prop);
    auto compound_ptr = ptr->asCompoundPtr();
    return Alembic::AbcGeom::ICompoundProperty(compound_ptr);
}

template <typename T>
static bool apparie_entete(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return T::matches(iobject.getHeader());
}

template <typename TypePoignee>
static TypePoignee *comme(IObject *object)
{
    using interface = InterfacePoignee<TypePoignee>;
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    auto poly = typename interface::TypeAlembic(iobject, Abc::kWrapExisting);
    auto pointeur = reinterpret_cast<TypePoignee *>(poly.getPtr().get());
    cache_global.ajoute(pointeur, poly);
    return pointeur;
}

/* Le schéma est recrée est recréé à partir des propriétés des objets, donc nous
 * retournons un pointeur vers les propriétés. Ceci pour nous assurer une
 * stabilité des pointeurs. */
template <typename TypePoignee>
static auto obtiens_schema(TypePoignee *object)
{
    using interface = InterfacePoignee<TypePoignee>;
    auto poly_mesh = cache_global.alembic_geom(object);
    return reinterpret_cast<typename interface::TypePoigneeSchema *>(
                poly_mesh.getProperties().getPtr().get());
}

template <typename TypePoignee>
static auto schema_alembic(TypePoignee *ischema)
{
    using TypeSchema = typename InterfacePoignee<TypePoignee>::TypeAlembic;
    auto ptr = reinterpret_cast<Alembic::Abc::CompoundPropertyReader *>(ischema);
    return TypeSchema(ptr->asCompoundPtr(), TypeSchema::getDefaultSchemaName());
}

template <typename TypePoignee>
static auto obtiens_props_arbitraires(TypePoignee *p_schema)
{
    auto schema = schema_alembic(p_schema);
    auto arb_geom = schema.getArbGeomParams();
    auto arb_geom_ptr = arb_geom.getPtr();
    return reinterpret_cast<IProprieteComposee *>(arb_geom_ptr.get());
}

template <typename TypePoignee>
static auto obtiens_props_utilisateurs(TypePoignee *p_schema)
{
    auto schema = schema_alembic(p_schema);
    auto arb_geom = schema.getUserProperties();
    auto arb_geom_ptr = arb_geom.getPtr();
    return reinterpret_cast<IProprieteComposee *>(arb_geom_ptr.get());
}

namespace detail {

template <typename TypeSchema>
static auto ABC_echantillon_pour_index(TypeSchema *schema, long index)
{
    using TypeEchant = typename InterfacePoignee<TypeSchema>::TypeEchant;
    auto ischema = schema_alembic(schema);
    auto echant = TypeEchant::cree_alembic();
    ischema.get(*echant, Abc::ISampleSelector(index));
    return TypeEchant::depuis_alembic(echant);
}

template <typename TypeSchema>
static auto ABC_echantillon_pour_temps(TypeSchema *schema, double temps)
{
    using TypeEchant = typename InterfacePoignee<TypeSchema>::TypeEchant;
    auto ischema = schema_alembic(schema);
    auto echant = TypeEchant::cree_alembic();
    ischema.get(*echant, Abc::ISampleSelector(temps));
    return TypeEchant::depuis_alembic(echant);
}

template <typename TypeEchant>
void ABC_echant_detruit(TypeEchant *echant)
{
    auto isample = TypeEchant::vers_alembic(echant);
    delete isample;
}

}  // namespace detail

extern "C" {

/* interface IArchive */

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
    auto iobject = cache_global.ajoute_iobjet(archive->archive.getTop());
    return VERS_POIGNEE_IOBJECT(iobject);
}

/* interface IObject */

IObject *ABC_iobject_parent(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    auto ptr = cache_global.ajoute_iobjet(iobject.getParent());
    return VERS_POIGNEE_IOBJECT(ptr);
}

const EnteteIObject *ABC_iobject_entete(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return depuis_alembic(&iobject.getHeader());
}

const EnteteIObject *ABC_iobject_entete_enfant_par_index(IObject *object, long index)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return depuis_alembic(&iobject.getChildHeader(static_cast<size_t>(index)));
}

const EnteteIObject *ABC_iobject_entete_enfant_par_nom(IObject *object, ChaineKuri nom)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return depuis_alembic(iobject.getChildHeader(vers_std_string(nom)));
}

long ABC_iobject_nombre_enfants(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return static_cast<long>(iobject.getNumChildren());
}

IObject *ABC_iobject_enfant_index(IObject *object, long index)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    auto ptr = cache_global.ajoute_iobjet(iobject.getChild(static_cast<size_t>(index)));
    return VERS_POIGNEE_IOBJECT(ptr);
}

IObject *ABC_iobject_enfant_nom(IObject *object, ChaineKuri nom)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    auto ptr = cache_global.ajoute_iobjet(iobject.getChild(vers_std_string(nom)));
    return VERS_POIGNEE_IOBJECT(ptr);
}

ChaineKuri ABC_iobject_nom(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return {iobject.getName().c_str(), static_cast<long>(iobject.getName().size())};
}

ChaineKuri ABC_iobject_chemin(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return {iobject.getFullName().c_str(), static_cast<long>(iobject.getFullName().size())};
}

bool ABC_iobject_est_racine_instance(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return iobject.isInstanceRoot();
}

bool ABC_iobject_est_descendant_instance(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return iobject.isInstanceDescendant();
}

ChaineKuri ABC_iobject_chemin_source_instance(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return vers_chaine_kuri(iobject.instanceSourcePath());
}

bool ABC_iobject_est_enfant_par_nom_instance(IObject *object, ChaineKuri chaine)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return iobject.isChildInstance(vers_std_string(chaine));
}

bool ABC_iobject_est_enfant_par_index_instance(IObject *object, long index)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return iobject.isChildInstance(static_cast<size_t>(index));
}

IObject *ABC_iobject_pointeur_instance(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    auto instance = Abc::IObject(iobject.getInstancePtr(), AbcGeom::kWrapExisting);
    instance = cache_global.ajoute_iobjet(instance);
    return VERS_POIGNEE_IOBJECT(instance);
}

IProprieteComposee *ABC_iobject_proprietes(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    return depuis_alembic(iobject.getProperties());
}

void ABC_iobject_detruit(IObject *object)
{
    auto iobject = DEPUIS_POIGNEE_IOBJECT(object);
    cache_global.supprime_objet(iobject);
}

/* enquêtes sur type objet */

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

bool ABC_est_lumiere(IObject *object)
{
    return apparie_entete<Alembic::AbcGeom::ILight>(object);
}

bool ABC_est_materiau(IObject *object)
{
    return apparie_entete<Alembic::AbcMaterial::IMaterial>(object);
}

bool ABC_est_groupe_poly(IObject *object)
{
    return apparie_entete<Alembic::AbcGeom::IFaceSet>(object);
}

/* conversions vers type objet */

IMaillage *ABC_comme_maillage(IObject *object)
{
    return comme<IMaillage>(object);
}

ISubdivision *ABC_comme_subdivision(IObject *object)
{
    return comme<ISubdivision>(object);
}

IXform *ABC_comme_xform(IObject *object)
{
    return comme<IXform>(object);
}

IPoints *ABC_comme_points(IObject *object)
{
    return comme<IPoints>(object);
}

ICamera *ABC_comme_camera(IObject *object)
{
    return comme<ICamera>(object);
}

ICourbes *ABC_comme_courbes(IObject *object)
{
    return comme<ICourbes>(object);
}

ILumiere *ABC_comme_lumiere(IObject *object)
{
    return comme<ILumiere>(object);
}

IMateriau *ABC_comme_materiau(IObject *object)
{
    return comme<IMateriau>(object);
}

IGroupePoly *ABC_comme_groupe_poly(IObject *object)
{
    return comme<IGroupePoly>(object);
}

/* schéma depuis objets */

ISchemaCamera *ABC_schema_camera(ICamera *object)
{
    return obtiens_schema(object);
}

ISchemaCourbes *ABC_schema_courbes(ICourbes *object)
{
    return obtiens_schema(object);
}

ISchemaGroupePoly *ABC_schema_groupe_poly(IGroupePoly *object)
{
    return obtiens_schema(object);
}

ISchemaLumiere *ABC_schema_lumiere(ILumiere *object)
{
    return obtiens_schema(object);
}

ISchemaMaillage *ABC_schema_maillage(IMaillage *object)
{
    auto poly_mesh = cache_global.alembic_geom(object);
    return reinterpret_cast<ISchemaMaillage *>(poly_mesh.getProperties().getPtr().get());
}

ISchemaMateriau *ABC_schema_materiau(IMateriau *object)
{
    return obtiens_schema(object);
}

ISchemaPoints *ABC_schema_points(IPoints *object)
{
    return obtiens_schema(object);
}

ISchemaSubdivision *ABC_schema_subdivision(ISubdivision *object)
{
    return obtiens_schema(object);
}

ISchemaXform *ABC_schema_xform(IXform *object)
{
    return obtiens_schema(object);
}

/* propriétés arbitraires depuis des schémas */

IProprieteComposee *ABC_maillage_props_arbitraires(ISchemaMaillage *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

IProprieteComposee *ABC_points_props_arbitraires(ISchemaPoints *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

IProprieteComposee *ABC_courbes_props_arbitraires(ISchemaCourbes *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

IProprieteComposee *ABC_subdivision_props_arbitraires(ISchemaSubdivision *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

IProprieteComposee *ABC_camera_props_arbitraires(ISchemaCamera *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

IProprieteComposee *ABC_lumiere_props_arbitraires(ISchemaLumiere *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

IProprieteComposee *ABC_groupe_poly_props_arbitraires(ISchemaGroupePoly *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

IProprieteComposee *ABC_xform_props_arbitraires(ISchemaXform *ischema)
{
    return obtiens_props_arbitraires(ischema);
}

/* propriétés utilisateurs */

IProprieteComposee *ABC_maillage_props_utilisateurs(ISchemaMaillage *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

IProprieteComposee *ABC_points_props_utilisateurs(ISchemaPoints *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

IProprieteComposee *ABC_courbes_props_utilisateurs(ISchemaCourbes *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

IProprieteComposee *ABC_subdivision_props_utilisateurs(ISchemaSubdivision *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

IProprieteComposee *ABC_camera_props_utilisateurs(ISchemaCamera *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

IProprieteComposee *ABC_lumiere_props_utilisateurs(ISchemaLumiere *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

IProprieteComposee *ABC_groupe_poly_props_utilisateurs(ISchemaGroupePoly *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

IProprieteComposee *ABC_xform_props_utilisateurs(ISchemaXform *ischema)
{
    return obtiens_props_utilisateurs(ischema);
}

/* interface IProprieteComposee */

IProprieteComposee *ABC_prop_parent(IProprieteComposee *p_prop)
{
    auto prop = vers_alembic(p_prop);
    return depuis_alembic(prop.getParent());
}

long ABC_nombre_proprietes(IProprieteComposee *p_prop)
{
    auto prop = vers_alembic(p_prop);
    return static_cast<long>(prop.getNumProperties());
}

const EntetePropriete *ABC_entete_prop_par_index(IProprieteComposee *p_prop, long index)
{
    auto prop = vers_alembic(p_prop);
    return depuis_alembic(&prop.getPropertyHeader(static_cast<size_t>(index)));
}

const EntetePropriete *ABC_entete_prop_par_nom(IProprieteComposee *p_prop, ChaineKuri chaine)
{
    auto prop = vers_alembic(p_prop);
    return depuis_alembic(prop.getPropertyHeader(vers_std_string(chaine)));
}

/* interface EntetePropriete */

ChaineKuri ABC_entete_propriete_nom(EntetePropriete *entete)
{
    auto prop = vers_alembic(entete);

    if (!prop) {
        return {nullptr, 0};
    }

    return vers_chaine_kuri(prop->getName());
}

bool ABC_entete_prop_est_scalaire(EntetePropriete *entete)
{
    auto prop = vers_alembic(entete);

    if (!prop) {
        return false;
    }

    return prop->isScalar();
}

bool ABC_entete_prop_est_tableau(EntetePropriete *entete)
{
    auto prop = vers_alembic(entete);

    if (!prop) {
        return false;
    }

    return prop->isArray();
}

bool ABC_entete_prop_est_composee(EntetePropriete *entete)
{
    auto prop = vers_alembic(entete);

    if (!prop) {
        return false;
    }

    return prop->isCompound();
}

bool ABC_entete_prop_est_simple(EntetePropriete *entete)
{
    return !ABC_entete_prop_est_composee(entete);
}

const EchantillonageTemps *ABC_entete_prop_echant_temps(EntetePropriete *entete)
{
    auto prop = vers_alembic(entete);

    if (!prop) {
        return nullptr;
    }

    return depuis_alembic(prop->getTimeSampling().get());
}

/* interface EchantillonageTemps */

// long ABC_echant_temps_compte_stockes()
//{

//}

/* interface ISchemaMaillage */

long ABC_maillage_nombre_echantillons(ISchemaMaillage *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

int ABC_maillage_variance_topologie(ISchemaMaillage *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<int>(ischema.getTopologyVariance());
}

bool ABC_maillage_est_constant(ISchemaMaillage *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

const EchantillonageTemps *ABC_maillage_echantillonage_temps(ISchemaMaillage *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

#if 0
IV2fGeomParam getUVsParam() const
{
    return m_uvsParam;
}

IN3fGeomParam getNormalsParam() const
{
    return m_normalsParam;
}
#endif

/* interface ISchemaPoints */

long ABC_points_nombre_echantillons(ISchemaPoints *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

bool ABC_points_est_constant(ISchemaPoints *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

const EchantillonageTemps *ABC_points_echantillonage_temps(ISchemaMaillage *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface ISchemaSubdivision */

long ABC_subdivision_nombre_echantillons(ISchemaSubdivision *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

int ABC_subdivision_variance_topologie(ISchemaSubdivision *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<int>(ischema.getTopologyVariance());
}

bool ABC_subdivision_est_constant(ISchemaSubdivision *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

const EchantillonageTemps *ABC_subdivision_echantillonage_temps(ISchemaMaillage *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface ISchemaXform */

long ABC_xform_nombre_echantillons(ISchemaXform *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

bool ABC_xform_est_constant(ISchemaXform *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

bool ABC_xform_est_constant_identite(ISchemaXform *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstantIdentity();
}

const EchantillonageTemps *ABC_xform_echantillonage_temps(ISchemaXform *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface ISchemaCamera */

long ABC_camera_nombre_echantillons(ISchemaCamera *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

bool ABC_camera_est_constant(ISchemaCamera *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

const EchantillonageTemps *ABC_camera_echantillonage_temps(ISchemaCamera *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface ISchemaCourbes */

long ABC_courbes_nombre_echantillons(ISchemaCourbes *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

bool ABC_courbes_est_constant(ISchemaCourbes *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

int ABC_courbes_variance_topologie(ISchemaCourbes *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<int>(ischema.getTopologyVariance());
}

const EchantillonageTemps *ABC_courbes_echantillonage_temps(ISchemaCourbes *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface ISchemaLumiere */

long ABC_lumiere_nombre_echantillons(ISchemaLumiere *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

bool ABC_lumiere_est_constant(ISchemaLumiere *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

const EchantillonageTemps *ABC_lumiere_echantillonage_temps(ISchemaLumiere *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface ISchemaGroupePoly */

long ABC_groupe_poly_nombre_echantillons(ISchemaGroupePoly *schema)
{
    auto ischema = schema_alembic(schema);
    return static_cast<long>(ischema.getNumSamples());
}

bool ABC_groupe_poly_est_constant(ISchemaGroupePoly *schema)
{
    auto ischema = schema_alembic(schema);
    return ischema.isConstant();
}

const EchantillonageTemps *ABC_groupe_poly_echantillonage_temps(ISchemaGroupePoly *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface ISchemaMateriau */

const EchantillonageTemps *ABC_materiau_echantillonage_temps(ISchemaMateriau *schema)
{
    auto ischema = schema_alembic(schema);
    return depuis_alembic(ischema.getTimeSampling().get());
}

/* interface EchantillonTableau */

using EchantillonTableau = BaseEchant<Abc::ArraySample>;

long ABC_echant_tableau_taille(EchantillonTableau *echant)
{
    auto isample = EchantillonTableau::vers_alembic(echant);
    return static_cast<long>(isample->size());
}

const void *ABC_echant_tableau_donnees(EchantillonTableau *echant)
{
    auto isample = EchantillonTableau::vers_alembic(echant);
    return isample->getData();
}

bool ABC_echant_tableau_valide(EchantillonTableau *echant)
{
    if (!echant) {
        return false;
    }

    auto isample = EchantillonTableau::vers_alembic(echant);
    return isample->valid();
}

#if 0
void ABC_echant_type_donnees(EchantillonTableau *echant)
{
    auto isample = EchantillonTableau::vers_alembic(echant);
    return &isample->getDataType();
}
#endif

/* interface IEchantPoints */

IEchantPoints *ABC_points_echantillon_pour_index(ISchemaPoints *schema, long index)
{
    return detail::ABC_echantillon_pour_index(schema, index);
}

IEchantPoints *ABC_points_echantillon_pour_temps(ISchemaPoints *schema, double temps)
{
    return detail::ABC_echantillon_pour_temps(schema, temps);
}

void ABC_echant_points_detruit(IEchantPoints *echant)
{
    detail::ABC_echant_detruit(echant);
}

EchantillonTableau *ABC_echant_points_positions(IEchantPoints *echant)
{
    auto isample = IEchantPoints::vers_alembic(echant);
    auto array = isample->getPositions();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_points_ids(IEchantPoints *echant)
{
    auto isample = IEchantPoints::vers_alembic(echant);
    auto array = isample->getIds();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_points_velocites(IEchantPoints *echant)
{
    auto isample = IEchantPoints::vers_alembic(echant);
    auto pos = isample->getVelocities();
    return EchantillonTableau::depuis_alembic(pos.get());
}

bool ABC_echant_points_valide(IEchantPoints *echant)
{
    auto isample = IEchantPoints::vers_alembic(echant);
    return isample->valid();
}

/* interface IEchantCamera */

IEchantCamera *ABC_camera_echantillon_pour_index(ISchemaCamera *schema, long index)
{
    return detail::ABC_echantillon_pour_index(schema, index);
}

IEchantCamera *ABC_camera_echantillon_pour_temps(ISchemaCamera *schema, double temps)
{
    return detail::ABC_echantillon_pour_temps(schema, temps);
}

void ABC_echant_camera_detruit(IEchantCamera *echant)
{
    detail::ABC_echant_detruit(echant);
}

/* interface IEchantMaillage */

IEchantMaillage *ABC_maillage_echantillon_pour_index(ISchemaMaillage *schema, long index)
{
    return detail::ABC_echantillon_pour_index(schema, index);
}

IEchantMaillage *ABC_maillage_echantillon_pour_temps(ISchemaMaillage *schema, double temps)
{
    return detail::ABC_echantillon_pour_temps(schema, temps);
}

void ABC_echant_maillage_detruit(IEchantMaillage *echant)
{
    detail::ABC_echant_detruit(echant);
}

EchantillonTableau *ABC_echant_maillage_positions(IEchantMaillage *echant)
{
    auto isample = IEchantMaillage::vers_alembic(echant);
    auto array = isample->getPositions();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_maillage_compte_poly(IEchantMaillage *echant)
{
    auto isample = IEchantMaillage::vers_alembic(echant);
    auto array = isample->getFaceCounts();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_maillage_index_poly(IEchantMaillage *echant)
{
    auto isample = IEchantMaillage::vers_alembic(echant);
    auto array = isample->getFaceIndices();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_maillage_velocites(IEchantMaillage *echant)
{
    auto isample = IEchantMaillage::vers_alembic(echant);
    auto array = isample->getVelocities();
    return EchantillonTableau::depuis_alembic(array.get());
}

bool ABC_echant_maillage_valide(IEchantMaillage *echant)
{
    auto isample = IEchantMaillage::vers_alembic(echant);
    return isample->valid();
}

/* interface IEchantSubdivision */

IEchantSubdivision *ABC_subdivision_echantillon_pour_index(ISchemaSubdivision *schema, long index)
{
    return detail::ABC_echantillon_pour_index(schema, index);
}

IEchantSubdivision *ABC_subdivision_echantillon_pour_temps(ISchemaSubdivision *schema,
                                                           double temps)
{
    return detail::ABC_echantillon_pour_temps(schema, temps);
}

void ABC_echant_subdivision_detruit(IEchantSubdivision *echant)
{
    detail::ABC_echant_detruit(echant);
}

EchantillonTableau *ABC_echant_subdivision_positions(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getPositions();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_compte_poly(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getFaceCounts();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_index_poly(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getFaceIndices();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_velocites(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getVelocities();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_index_coins(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getCornerIndices();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_tranchant_coins(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getCornerSharpnesses();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_index_plis(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getCreaseIndices();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_longueur_plis(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getCreaseLengths();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_tranchant_plis(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getCreaseSharpnesses();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_subdivision_trous(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    auto array = isample->getHoles();
    return EchantillonTableau::depuis_alembic(array.get());
}

ChaineKuri ABC_echant_subdivision_schema_subdivision(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    return vers_chaine_kuri(isample->getSubdivisionScheme());
}

int ABC_echant_subdivision_interp_frontiere_fv(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    return isample->getFaceVaryingInterpolateBoundary();
}

int ABC_echant_subdivision_propage_coins_fv(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    return isample->getFaceVaryingPropagateCorners();
}

int ABC_echant_subdivision_interp_frontiere(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    return isample->getInterpolateBoundary();
}

bool ABC_echant_subdivision_valide(IEchantSubdivision *echant)
{
    auto isample = IEchantSubdivision::vers_alembic(echant);
    return isample->valid();
}

/* interface IEchantCourbes */

IEchantCourbes *ABC_courbes_echantillon_pour_index(ISchemaCourbes *schema, long index)
{
    return detail::ABC_echantillon_pour_index(schema, index);
}

IEchantCourbes *ABC_courbes_echantillon_pour_temps(ISchemaCourbes *schema, double temps)
{
    return detail::ABC_echantillon_pour_temps(schema, temps);
}

void ABC_echant_courbes_detruit(IEchantCourbes *echant)
{
    detail::ABC_echant_detruit(echant);
}

long ABC_echant_courbes_nombre_courbes(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    return static_cast<long>(isample->getNumCurves());
}

// CurveType
int ABC_echant_courbes_type(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    return static_cast<int>(isample->getType());
}

// CurvePeriodicity
int ABC_echant_courbes_periodicite(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    return static_cast<int>(isample->getWrap());
}

// BasisType
int ABC_echant_courbes_base(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    return static_cast<int>(isample->getBasis());
}

EchantillonTableau *ABC_echant_courbes_nombre_vertex(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    auto array = isample->getCurvesNumVertices();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_courbes_ordres(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    auto array = isample->getOrders();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_courbes_noeuds(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    auto array = isample->getKnots();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_courbes_positions(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    auto array = isample->getPositions();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_courbes_poids_positions(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    auto array = isample->getPositionWeights();
    return EchantillonTableau::depuis_alembic(array.get());
}

EchantillonTableau *ABC_echant_courbes_velocites(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    auto array = isample->getVelocities();
    return EchantillonTableau::depuis_alembic(array.get());
}

bool ABC_echant_courbes_valide(IEchantCourbes *echant)
{
    auto isample = IEchantCourbes::vers_alembic(echant);
    return isample->valid();
}

/* interface IEchantGroupePoly */

IEchantGroupePoly *ABC_groupe_poly_echantillon_pour_index(ISchemaGroupePoly *schema, long index)
{
    return detail::ABC_echantillon_pour_index(schema, index);
}

IEchantGroupePoly *ABC_groupe_poly_echantillon_pour_temps(ISchemaGroupePoly *schema, double temps)
{
    return detail::ABC_echantillon_pour_temps(schema, temps);
}

void ABC_echant_groupe_poly_detruit(IEchantGroupePoly *echant)
{
    detail::ABC_echant_detruit(echant);
}

EchantillonTableau *ABC_echant_groupe_poly_polygones(IEchantGroupePoly *echant)
{
    auto isample = IEchantGroupePoly::vers_alembic(echant);
    auto array = isample->getFaces();
    return EchantillonTableau::depuis_alembic(array.get());
}

bool ABC_echant_groupe_poly_valide(IEchantGroupePoly *echant)
{
    auto isample = IEchantGroupePoly::vers_alembic(echant);
    return isample->valid();
}

/* interface IEchantXform */

IEchantXform *ABC_xform_echantillon_pour_index(ISchemaXform *schema, long index)
{
    return detail::ABC_echantillon_pour_index(schema, index);
}

IEchantXform *ABC_xform_echantillon_pour_temps(ISchemaXform *schema, double temps)
{
    return detail::ABC_echantillon_pour_temps(schema, temps);
}

void ABC_echant_xform_detruit(IEchantXform *echant)
{
    detail::ABC_echant_detruit(echant);
}

}

/* Nouvelle Interface. */

// --------------------------------------------------------------
// Contexte pour la gestion de la mémoire.

template <typename T>
T *loge_objet(ContexteKuri *ctx)
{
    void *pointeur = ctx->loge_memoire(ctx, sizeof(T));
    new (pointeur) T();
    return static_cast<T *>(pointeur);
}

template <typename T>
T *reloge_objet(ContexteKuri *ctx, T *objet)
{
    void *nouveau_pointeur = ctx->reloge_memoire(ctx, objet, sizeof(T), sizeof(T));
    return static_cast<T *>(nouveau_pointeur);
}

template <typename T>
void deloge_objet(ContexteKuri *ctx, T *objet)
{
    if (objet) {
        objet->~T();
    }
    ctx->deloge_memoire(ctx, objet, sizeof(T));
}

extern "C" {

// --------------------------------------------------------------
// Ouverture de l'archive.

static Abc::ErrorHandler::Policy convertis_police_erreur(eAbcPoliceErreur notre_police)
{
    switch (notre_police) {
        case eAbcPoliceErreur::SILENCIEUSE: {
            return Abc::ErrorHandler::Policy::kQuietNoopPolicy;
        }
        case eAbcPoliceErreur::BRUYANTE: {
            return Abc::ErrorHandler::Policy::kNoisyNoopPolicy;
        }
        case eAbcPoliceErreur::LANCE_EXCEPTION: {
            return Abc::ErrorHandler::Policy::kThrowPolicy;
        }
    }

    /* Par défaut et par symétrie avec Alembic, retourne kThrowPolicy en cas de valeur invalide. */
    return Abc::ErrorHandler::Policy::kThrowPolicy;
}

static AbcCoreFactory::IFactory::OgawaReadStrategy convertis_strategie_ogawa(
        eAbcStrategieLectureOgawa notre_strategie)
{
    switch (notre_strategie) {
        case eAbcStrategieLectureOgawa::FLUX_DE_FICHIERS: {
            return AbcCoreFactory::IFactory::OgawaReadStrategy::kFileStreams;
        }
        case eAbcStrategieLectureOgawa::FICHIERS_MAPPES_MEMOIRE: {
            return AbcCoreFactory::IFactory::OgawaReadStrategy::kMemoryMappedFiles;
        }
    }

    /* Par défaut et par symétrie avec Alembic, retourne kMemoryMappedFiles en cas de valeur
     * invalide. */
    return AbcCoreFactory::IFactory::OgawaReadStrategy::kMemoryMappedFiles;
}

struct ArchiveCache {
    Abc::IArchive iarchive;
};

static void renseigne_politique_erreur(ContexteOuvertureArchive *ctx, AbcCoreFactory::IFactory &ifactory)
{
    if (ctx->police_erreur) {
        const eAbcPoliceErreur police = ctx->police_erreur(ctx);
        ifactory.setPolicy(convertis_police_erreur(police));
    }
}

static void renseigne_strategie_ogawa(ContexteOuvertureArchive *ctx, AbcCoreFactory::IFactory &ifactory)
{
    if (ctx->nombre_de_flux_ogawa_desires) {
        const int nombre_de_flux = ctx->nombre_de_flux_ogawa_desires(ctx);
        ifactory.setOgawaNumStreams(static_cast<size_t>(nombre_de_flux));
    }

    if (ctx->strategie_lecture_ogawa) {
        const eAbcStrategieLectureOgawa strategie = ctx->strategie_lecture_ogawa(ctx);
        ifactory.setOgawaReadStrategy(convertis_strategie_ogawa(strategie));
    }
}

ArchiveCache *ABC_cree_archive(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx)
{
    const int nombre_de_chemins = ctx->nombre_de_chemins(ctx);

    if (nombre_de_chemins == 0) {
        if (ctx->erreur_aucun_chemin) {
            ctx->erreur_aucun_chemin(ctx);
        }
        return nullptr;
    }

    std::vector<std::string> chemins;
    for (int i = 0; i < nombre_de_chemins; ++i) {
        char *pointeur = nullptr;
        size_t taille = 0;

        ctx->chemin(ctx, i, &pointeur, &taille);

        if (pointeur == nullptr || taille == 0) {
            continue;
        }

        chemins.push_back({pointeur, taille});
    }

    if (chemins.empty()) {
        if (ctx->erreur_aucun_chemin) {
            ctx->erreur_aucun_chemin(ctx);
        }
        return nullptr;
    }

    Alembic::AbcCoreFactory::IFactory factory;
    renseigne_politique_erreur(ctx, factory);
    renseigne_strategie_ogawa(ctx, factory);

    Abc::IArchive iarchive = factory.getArchive(chemins);

    if (!iarchive.valid()) {
        if (ctx->erreur_archive_invalide) {
            ctx->erreur_archive_invalide(ctx);
        }
        return nullptr;
    }

    auto poignee = loge_objet<ArchiveCache>(ctx_kuri);
    poignee->iarchive = iarchive;
    return poignee;
}

void ABC_detruit_archive(ContexteKuri *ctx, ArchiveCache *archive)
{
    deloge_objet(ctx, archive);
}

// --------------------------------------------------------------
// Traversé de l'archive.

static void passe_nom_via_contexte(ContexteTraverseArchive *ctx, const std::string &nom)
{
    ctx->extrait_nom_courant(ctx, nom.c_str(), nom.size());
}

static void abc_traverse_hierarchie(
        ContexteTraverseArchive *ctx,
        Abc::IObject parent,
        const Abc::ObjectHeader &header)
{
    if (ctx->annule && ctx->annule(ctx)) {
        return;
    }

    Abc::IObject next_object;

    if (AbcGeom::IXform::matches(header)) {
        AbcGeom::IXform xform(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, xform.getFullName());
        }

        next_object = xform;
    }
    else if (AbcGeom::ISubD::matches(header)) {
        AbcGeom::ISubD subd(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, subd.getFullName());
        }

        next_object = subd;
    }
    else if (AbcGeom::IPolyMesh::matches(header)) {
        AbcGeom::IPolyMesh mesh(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, mesh.getFullName());
        }

        next_object = mesh;
    }
    else if (AbcGeom::ICurves::matches(header)) {
        AbcGeom::ICurves curves(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, curves.getFullName());
        }

        next_object = curves;
    }
    else if (AbcGeom::IFaceSet::matches(header)) {
        // ignore the face set, it will be read along with the data
    }
    else if (AbcGeom::IPoints::matches(header)) {
        AbcGeom::IPoints points(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, points.getFullName());
        }

        next_object = points;
    }
    else if (AbcGeom::INuPatch::matches(header)) {
        AbcGeom::INuPatch nurbs(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, nurbs.getFullName());
        }

        next_object = nurbs;
    }
    else {
        next_object = parent.getChild(header.getName());

        if (next_object.isInstanceRoot()) {
            if (ctx->extrait_nom_courant) {
                passe_nom_via_contexte(ctx, next_object.getFullName());
            }
        }
    }

    if (!next_object.valid()) {
        return;
    }

    for (size_t i = 0; i < next_object.getNumChildren(); ++i) {
        abc_traverse_hierarchie(ctx, next_object, next_object.getChildHeader(i));
    }
}

void ABC_traverse_archive(ContexteKuri */*ctx_kuri*/, ArchiveCache *archive, ContexteTraverseArchive *ctx)
{
    if (!archive) {
        return;
    }

    const auto &top = archive->iarchive.getTop();

    if (!top.valid()) {
        return;
    }

    for (size_t i = 0; i < top.getNumChildren(); ++i) {
        abc_traverse_hierarchie(ctx, top, top.getChildHeader(i));
    }
}

// --------------------------------------------------------------
// Lecture des objets.

static void convertis_subd(ContexteKuri *ctx_kuri, ConvertisseuseSubD *convertisseuse, AbcGeom::ISubD &subd, const double time)
{

}

static void convertis_points(ContexteKuri *ctx_kuri, ConvertisseusePoints *convertisseuse, AbcGeom::IPoints &points, const double time)
{

}

static void convertis_courbes(ContexteKuri *ctx_kuri, ConvertisseuseCourbes *convertisseuse, AbcGeom::ICurves &curves, const double time)
{

}

static void convertis_nurbs(ContexteKuri *ctx_kuri, ConvertisseuseNurbs *convertisseuse, AbcGeom::INuPatch &nurbs, const double time)
{

}

static void convertis_xform(ContexteKuri *ctx_kuri, ConvertisseuseXform *convertisseuse, AbcGeom::IXform &xform, const double time)
{

}

void convertis_poly_mesh(ContexteKuri *ctx_kuri, ConvertisseusePolyMesh *convertisseuse, AbcGeom::IPolyMesh &polymesh, const double time)
{
    auto &schema = polymesh.getSchema();

    auto selector = Abc::ISampleSelector(time);

    AbcGeom::IPolyMeshSchema::Sample sample;
    schema.get(sample, selector);

    /* Convertis les points. */
    const auto &points = sample.getPositions();
    if (convertisseuse->reserve_points) {
        convertisseuse->reserve_points(convertisseuse->donnees, points->size());
    }

    if (convertisseuse->ajoute_tous_les_points) {
        convertisseuse->ajoute_tous_les_points(convertisseuse->donnees, &points->get()->x, points->size());
    }
    else if (convertisseuse->ajoute_un_point) {
        for (size_t i = 0; i < points->size(); ++i) {
            auto p = points->get()[i];
            convertisseuse->ajoute_un_point(convertisseuse->donnees, p.x, p.y, p.z);
        }
    }

    /* Convertis les polygones. */
    const auto &face_counts = sample.getFaceCounts();
    const auto &face_indices = sample.getFaceIndices();

    if (convertisseuse->reserve_polygones) {
        convertisseuse->reserve_polygones(convertisseuse->donnees, face_counts->size());
    }

    if (convertisseuse->reserve_coin) {
        convertisseuse->reserve_coin(convertisseuse->donnees, face_indices->size());
    }

    if (convertisseuse->ajoute_tous_les_polygones) {
        convertisseuse->ajoute_tous_les_polygones(convertisseuse->donnees, face_counts->get(), face_counts->size());
    }
    else {
        if (convertisseuse->reserve_coins_polygone) {
            for (size_t i = 0; i < face_counts->size(); ++i) {
                convertisseuse->reserve_coins_polygone(convertisseuse->donnees, i, face_counts->get()[i]);
            }
        }

        if (convertisseuse->ajoute_polygone) {
            auto decalage_coin = 0;

            for (size_t i = 0; i < face_counts->size(); ++i) {
                auto nombre_de_coins = face_counts->get()[i];
                auto ptr_coins = &face_indices->get()[decalage_coin];
                convertisseuse->ajoute_polygone(convertisseuse->donnees, i, ptr_coins, nombre_de_coins);
                decalage_coin += nombre_de_coins;
            }
        }
    }

    if (convertisseuse->ajoute_tous_les_coins) {
        convertisseuse->ajoute_tous_les_coins(convertisseuse->donnees, face_indices->get(), face_indices->size());
    }
    else {
        if (convertisseuse->ajoute_coin_polygone) {
            auto decalage_coin = 0;

            for (size_t i = 0; i < face_counts->size(); ++i) {
                auto nombre_de_coins = face_counts->get()[i];
                auto ptr_coins = &face_indices->get()[decalage_coin];

                for (int j = 0; j < nombre_de_coins; ++j) {
                    convertisseuse->ajoute_coin_polygone(convertisseuse->donnees, i, ptr_coins[j]);
                }

                decalage_coin += nombre_de_coins;
            }
        }
    }
}

struct iteratrice_chemin {
  private:
    std::string_view chemin{};
    std::string_view morceau_courant{};
    size_t pos = 0;

  public:
    iteratrice_chemin(const std::string &chemin_complet)
        : chemin(chemin_complet)
    {}

    std::string_view suivant()
    {
        if (fini()) {
            return "";
        }

        /* Saute le premier slash. */
        if (chemin[pos] == '/') {
            pos += 1;
        }

        auto nouvelle_pos = chemin.find('/', pos);
        if (nouvelle_pos == std::string::npos) {
            nouvelle_pos = chemin.size();
        }

        morceau_courant = chemin.substr(pos, nouvelle_pos - pos);
        pos = nouvelle_pos;
        return morceau_courant;
    }

    bool fini() const
    {
        return pos >= chemin.size();
    }
};

struct LectriceCache {
    Abc::IObject iobject;

    void *donnees;
};

LectriceCache *ABC_cree_lectrice_cache(ContexteKuri *ctx_kuri, ArchiveCache *archive, const char *ptr_nom, size_t taille_nom)
{
    if (!archive) {
        // std::cerr << "L'archive est nulle !\n";
        return nullptr;
    }

    auto &iarchive = archive->iarchive;

    if (!iarchive.valid()) {
        // std::cerr << "L'archive est invalide !\n";
        return nullptr;
    }

    const auto nom = std::string(ptr_nom, taille_nom);

    if (nom.empty()) {
        // std::cerr << "Le nom est vide !\n";
        return nullptr;
    }

    AbcGeom::IObject courant = iarchive.getTop();

    if (!courant.valid()) {
        // std::cerr << "La racine de l'archvie est invalide !\n";
        return nullptr;
    }

    auto iteratrice = iteratrice_chemin(nom);

    while (!iteratrice.fini()) {
        const auto morceau = iteratrice.suivant();
        auto enfant = courant.getChild(std::string(morceau));

        if (!enfant.valid()) {
            // std::cerr << "L'enfant est invalide !\n";
            return nullptr;
        }

        courant = enfant;
    }

    if (!courant.valid()) {
        // std::cerr << "L'enfant final est invalide !\n";
        return nullptr;
    }

    auto lectrice = loge_objet<LectriceCache>(ctx_kuri);
    lectrice->iobject = courant;
    return lectrice;
}

void ABC_detruit_lectrice(ContexteKuri *ctx_kuri, LectriceCache *lectrice)
{
    deloge_objet(ctx_kuri, lectrice);
}

void ABC_lectrice_ajourne_donnees(LectriceCache *lectrice, void *donnees)
{
    lectrice->donnees = donnees;
}

void ABC_lis_objet(ContexteKuri *ctx_kuri, ContexteLectureCache *contexte, LectriceCache *lectrice, double temps)
{
    if (!lectrice || !lectrice->iobject.valid()) {
        // contexte->rapporte_erreur(contexte, )
        return;
    }

    if (AbcGeom::IPolyMesh::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_polymesh) {
            ConvertisseusePolyMesh convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_polymesh(&convertisseuse);

            AbcGeom::IPolyMesh poly_mesh(lectrice->iobject, Abc::kWrapExisting);
            convertis_poly_mesh(ctx_kuri, &convertisseuse, poly_mesh, temps);
        }
    }
    else if (AbcGeom::ISubD::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_subd) {
            ConvertisseuseSubD convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_subd(&convertisseuse);

            AbcGeom::ISubD subd(lectrice->iobject, Abc::kWrapExisting);
            convertis_subd(ctx_kuri, &convertisseuse, subd, temps);
        }
    }
    else if (AbcGeom::IPoints::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_points) {
            ConvertisseusePoints convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_points(&convertisseuse);

            AbcGeom::IPoints points(lectrice->iobject, Abc::kWrapExisting);
            convertis_points(ctx_kuri, &convertisseuse, points, temps);
        }
    }
    else if (AbcGeom::INuPatch::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_nurbs) {
            ConvertisseuseNurbs convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_nurbs(&convertisseuse);

            AbcGeom::INuPatch nurbs(lectrice->iobject, Abc::kWrapExisting);
            convertis_nurbs(ctx_kuri, &convertisseuse, nurbs, temps);
        }
    }
    else if (AbcGeom::ICurves::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_courbes) {
            ConvertisseuseCourbes convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_courbes(&convertisseuse);

            AbcGeom::ICurves courbes(lectrice->iobject, Abc::kWrapExisting);
            convertis_courbes(ctx_kuri, &convertisseuse, courbes, temps);
        }
    }
    else if (AbcGeom::IXform::matches(lectrice->iobject.getHeader())) {
        if (contexte->initialise_convertisseuse_xform) {
            ConvertisseuseXform convertisseuse;
            convertisseuse.donnees = lectrice->donnees;
            contexte->initialise_convertisseuse_xform(&convertisseuse);

            AbcGeom::IXform xform(lectrice->iobject, Abc::kWrapExisting);
            convertis_xform(ctx_kuri, &convertisseuse, xform, temps);
        }
    }
    else {
        if (lectrice->iobject.isInstanceRoot()) {
            /* Que faire pour les instances ? */
            return;
        }
    }
}
}
