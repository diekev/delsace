/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "abc_common.hh"

#include <string_view>

#include "alembic_ipa_c.h"

#include "../InterfaceCKuri/contexte_kuri.hh"

/*
    - [x] Alembic
    - [ ] OpenColorIO (reprend code de Sergey)
    - [x] OpenImageIO
    - [ ] OpenSubDiv (reprend code de Sergey)
    - [ ] PTex

    - [x] Alembic
    - [ ] Bullet
    - [ ] Cycles
    - [ ] CUDA
    - [ ] OpenColorIO (reprend code de Sergey)
    - [ ] OpenCV (ou ffmpeg)
    - [x] OpenImageIO
    - [ ] OpenSubDiv (reprend code de Sergey)
    - [ ] OpenVDB
    - [ ] OptiX
    - [ ] OSL (avec Cycles)
    - [ ] PTex
    - [ ] USD
    - [ ] Vulkan

    Pipeline rendu :
    - charge les objets
    - construction des tampons
    - déforme les positions selon les armatures
    - applique quelconque algorithme de sous-division
    - applique quelconque déplacement
    - recalcule les normaux
 */

#define ABC_STRING_FROM_C_STRING(x) Abc_String{x, sizeof(x) - 1}

Abc_Attribute_Type_Descriptor *abc_get_attribute_type_descriptors(uint64_t *r_len)
{
#define DECLARE_DESCRIPTOR(type_geom, type_abc, type_c, nom_court)                                \
    {ABC_STRING_FROM_C_STRING(#type_geom),                                                        \
     ABC_STRING_FROM_C_STRING(#type_c),                                                           \
     ABC_STRING_FROM_C_STRING(#nom_court)},

    static Abc_Attribute_Type_Descriptor descriptors[] = {
        ENUMERATE_ABC_ATTRIBUTE_TYPES(DECLARE_DESCRIPTOR)};

#undef DECLARE_DESCRIPTOR

    if (r_len) {
        *r_len = sizeof(descriptors) / sizeof(descriptors[0]);
    }

    return descriptors;
}

Abc_Attribute_Type_Descriptor *abc_get_pod_type_descriptors(uint64_t *r_len)
{
#define DECLARE_DESCRIPTOR(type_geom, type_abc, type_c, nom_court)                                \
    {ABC_STRING_FROM_C_STRING(#type_geom),                                                        \
     ABC_STRING_FROM_C_STRING(#type_c),                                                           \
     ABC_STRING_FROM_C_STRING(#nom_court)},

    static Abc_Attribute_Type_Descriptor descriptors[] = {
        ENUMERATE_ABC_POD_TYPE(DECLARE_DESCRIPTOR)};

#undef DECLARE_DESCRIPTOR

    if (r_len) {
        *r_len = sizeof(descriptors) / sizeof(descriptors[0]);
    }

    return descriptors;
}

/* ------------------------------------------------------------------------- */
/** \nom Abc_String
 * \{ */

Abc_String::operator std::string()
{
    return vers_std_string(*this);
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_String
 * \{ */

static Abc_New_String make_new_string(std::string_view str)
{
    auto characters = new char[str.size()];
    memcpy(characters, str.data(), str.size());
    Abc_New_String résultat;
    résultat.characters = characters;
    résultat.size = str.size();
    return résultat;
}

void abc_new_string_destroy(struct Abc_New_String *new_string)
{
    delete[] new_string->characters;
    new_string->characters = nullptr;
    new_string->size = 0;
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom MetaData
 * \{ */

struct Abc_MetaData_Iterator {
    struct Abc_MetaData *metadata = nullptr;
    /* Pour la liste libre. */
    Abc_MetaData_Iterator *next = nullptr;
    Abc::MetaData::const_iterator current{};
    Abc::MetaData::const_iterator end{};
};

Abc_MetaData *make_metadata(ContexteKuri *ctx_kuri, const Abc::MetaData &metadata)
{
    auto résultat = kuri_loge<Abc_MetaData>(ctx_kuri);
    résultat->ctx_kuri = ctx_kuri;
    résultat->metadata = metadata;
    return résultat;
}

void abc_metadata_destroy(struct Abc_MetaData *metadata)
{
    if (metadata) {
        kuri_deloge_liste(metadata->ctx_kuri, metadata->iterators);
        kuri_deloge(metadata->ctx_kuri, metadata);
    }
}

void abc_metadata_set(struct Abc_MetaData *metadata, Abc_String key, Abc_String data)
{
    metadata->metadata.set(key, data);
}

void abc_metadata_set_unique(struct Abc_MetaData *metadata, Abc_String key, Abc_String data)
{
    metadata->metadata.setUnique(key, data);
}

// À FAIRE : get, getRequired

void abc_metadata_append(struct Abc_MetaData *metadata, struct Abc_MetaData *source)
{
    metadata->metadata.append(source->metadata);
}

void abc_metadata_append_only_unique(struct Abc_MetaData *metadata, struct Abc_MetaData *source)
{
    metadata->metadata.appendOnlyUnique(source->metadata);
}

void abc_metadata_append_unique(struct Abc_MetaData *metadata, struct Abc_MetaData *source)
{
    metadata->metadata.appendUnique(source->metadata);
}

struct Abc_MetaData_Iterator *abc_metadata_get_iterator(struct Abc_MetaData *metadata)
{
    if (!metadata) {
        return nullptr;
    }

    Abc_MetaData_Iterator *résultat;
    if (metadata->iterators) {
        résultat = metadata->iterators;
        metadata->iterators = metadata->iterators->next;
    }
    else {
        résultat = kuri_loge<Abc_MetaData_Iterator>(metadata->ctx_kuri);
    }
    résultat->metadata = metadata;
    résultat->next = nullptr;
    résultat->current = résultat->metadata->metadata.begin();
    résultat->end = résultat->metadata->metadata.end();
    return résultat;
}

bool abc_metadata_iterator_next(Abc_MetaData_Iterator *iterator,
                                Abc_String *key,
                                Abc_String *value)
{
    if (iterator->current == iterator->end) {
        liste_ajoute(&iterator->metadata->iterators, iterator);
        return false;
    }

    if (key) {
        key->characters = iterator->current->first.data();
        key->size = iterator->current->first.size();
    }
    if (value) {
        value->characters = iterator->current->second.data();
        value->size = iterator->current->second.size();
    }

    iterator->current++;
    return true;
}

void abc_set_source_name(struct Abc_MetaData *metadata, struct Abc_String source_name)
{
    Abc::SetSourceName(metadata->metadata, vers_std_string(source_name));
}

struct Abc_New_String abc_get_source_name(struct Abc_MetaData *metadata)
{
    return make_new_string(Abc::GetSourceName(metadata->metadata));
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Time_Sampling
 * \{ */

uint64_t abc_time_sampling_get_num_stored_times(struct Abc_Time_Sampling *time_sampling)
{
    return time_sampling->ptr->getNumStoredTimes();
}

void abc_time_sampling_get_stored_times(struct Abc_Time_Sampling *time_sampling,
                                        double **r_times,
                                        uint64_t *r_num_times)
{
    if (r_times && r_num_times) {
        const std::vector<Abc::chrono_t> &stored_times = time_sampling->ptr->getStoredTimes();
        *r_times = const_cast<double *>(stored_times.data());
        *r_num_times = stored_times.size();
    }
}

void abc_time_sampling_get_time_sampling_type(struct Abc_Time_Sampling *time_sampling,
                                              struct Abc_Time_Sampling_Type *r_type)
{
    if (r_type) {
        auto type = time_sampling->ptr->getTimeSamplingType();
        r_type->time_per_cycle = type.getTimePerCycle();
        r_type->num_samples_per_cycle = type.getNumSamplesPerCycle();
    }
}

double abc_time_sampling_get_sample_time(struct Abc_Time_Sampling *time_sampling, int64_t index)
{
    return time_sampling->ptr->getSampleTime(index);
}

void abc_time_sampling_get_floor_index(struct Abc_Time_Sampling *time_sampling,
                                       double time,
                                       int64_t num_samples,
                                       struct Abc_Sample_Time_Index *r_index)
{
    if (r_index) {
        auto result = time_sampling->ptr->getFloorIndex(time, num_samples);
        r_index->index = result.first;
        r_index->time = result.second;
    }
}

void abc_time_sampling_get_ceil_index(struct Abc_Time_Sampling *time_sampling,
                                      double time,
                                      int64_t num_samples,
                                      struct Abc_Sample_Time_Index *r_index)
{
    if (r_index) {
        auto result = time_sampling->ptr->getCeilIndex(time, num_samples);
        r_index->index = result.first;
        r_index->time = result.second;
    }
}

void abc_time_sampling_get_near_index(struct Abc_Time_Sampling *time_sampling,
                                      double time,
                                      int64_t num_samples,
                                      struct Abc_Sample_Time_Index *r_index)
{
    if (r_index) {
        auto result = time_sampling->ptr->getNearIndex(time, num_samples);
        r_index->index = result.first;
        r_index->time = result.second;
    }
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Property_Header
 * \{ */

void abc_property_header_get_name(struct Abc_Property_Header *header, struct Abc_String *name)
{
    vers_abc_string(name, header->header.getName());
}

enum Abc_Property_Type abc_property_header_get_property_type(struct Abc_Property_Header *header)
{
    return static_cast<Abc_Property_Type>(header->header.getPropertyType());
}

bool abc_property_header_is_scalar(struct Abc_Property_Header *header)
{
    return header->header.isScalar();
}

bool abc_property_header_is_array(struct Abc_Property_Header *header)
{
    return header->header.isArray();
}

bool abc_property_header_is_compound(struct Abc_Property_Header *header)
{
    return header->header.isCompound();
}

bool abc_property_header_is_simple(struct Abc_Property_Header *header)
{
    return header->header.isSimple();
}

Abc_MetaData *abc_property_header_get_metadata(struct Abc_Property_Header *header)
{
    return make_metadata(header->ctx_kuri, header->header.getMetaData());
}

void abc_property_header_get_data_type(struct Abc_Property_Header *header,
                                       struct Abc_Data_Type *r_data_type)
{
    make_abc_data_type(header->header.getDataType(), r_data_type);
}

// À FAIRE TimeSamplingPtr getTimeSampling() const

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Object_Header
 * \{ */

void abc_object_header_get_name(struct Abc_Object_Header *header, Abc_String *name)
{
    vers_abc_string(name, header->header.getName());
}

void abc_object_header_get_full_name(struct Abc_Object_Header *header, Abc_String *name)
{
    vers_abc_string(name, header->header.getFullName());
}

Abc_MetaData *abc_object_header_get_metadata(struct Abc_Object_Header *header)
{
    return make_metadata(header->ctx_kuri, header->header.getMetaData());
}

#define DECLARE_OBJECT_MATCHES_FUNCTIONS(type_abc, type_kuri, lname)                              \
    bool abc_object_header_matches_##lname(struct Abc_Object_Header *header)                      \
    {                                                                                             \
        return type_abc::matches(header->header);                                                 \
    }

ENUMERATE_INPUT_OBJECT_TYPES(DECLARE_OBJECT_MATCHES_FUNCTIONS)

#undef DECLARE_OBJECT_MATCHES_FUNCTIONS

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Xform_Op
 * \{ */

static_assert(sizeof(Abc_Xform_Op) == sizeof(AbcGeom::XformOp));
static_assert(alignof(Abc_Xform_Op) == alignof(AbcGeom::XformOp));

void abc_xform_op_init(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    new (abc_op) AbcGeom::XformOp;
}

void abc_xform_op_init_type_hint(struct Abc_Xform_Op *op,
                                 enum Abc_Xform_Operation_Type type,
                                 uint8_t hint)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    new (abc_op) AbcGeom::XformOp(static_cast<AbcGeom::XformOperationType>(type), hint);
}

enum Abc_Xform_Operation_Type abc_xform_op_get_type(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return static_cast<Abc_Xform_Operation_Type>(abc_op->getType());
}

void abc_xform_op_set_type(Abc_Xform_Op *op, Abc_Xform_Operation_Type type)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setType(static_cast<AbcGeom::XformOperationType>(type));
}

uint8_t abc_xform_op_get_hint(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getHint();
}

void abc_xform_op_set_hint(Abc_Xform_Op *op, uint8_t hint)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setHint(hint);
}

bool abc_xform_op_is_x_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isXAnimated();
}

bool abc_xform_op_is_y_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isYAnimated();
}

bool abc_xform_op_is_z_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isZAnimated();
}

bool abc_xform_op_is_angle_animated(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isAngleAnimated();
}

bool abc_xform_op_is_channel_animated(Abc_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isChannelAnimated(index);
}

uint64_t abc_xform_op_get_num_channels(struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getNumChannels();
}

double abc_xform_op_get_default_channel_value(struct Abc_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getDefaultChannelValue(index);
}

double abc_xform_op_get_channel_value(struct Abc_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getChannelValue(index);
}

void abc_xform_op_set_channel_value(Abc_Xform_Op *op, uint64_t index, double val)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setChannelValue(index, val);
}

void abc_xform_op_set_vector(Abc_Xform_Op *op, Abc_V3d *vec)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_vec = *reinterpret_cast<Abc::V3d *>(vec);
    abc_op->setVector(abc_vec);
}

void abc_xform_op_set_translate(Abc_Xform_Op *op, Abc_V3d *trans)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_trans = *reinterpret_cast<Abc::V3d *>(trans);
    abc_op->setTranslate(abc_trans);
}

void abc_xform_op_set_scale(Abc_Xform_Op *op, Abc_V3d *scale)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_scale = *reinterpret_cast<Abc::V3d *>(scale);
    abc_op->setScale(abc_scale);
}

void abc_xform_op_set_axis(Abc_Xform_Op *op, Abc_V3d *axis)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_axis = *reinterpret_cast<Abc::V3d *>(axis);
    abc_op->setAxis(abc_axis);
}

void abc_xform_op_set_angle(Abc_Xform_Op *op, double angle)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    abc_op->setAngle(angle);
}

void abc_xform_op_set_matrix(Abc_Xform_Op *op, Abc_M44d *matrix)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto abc_matrix = *reinterpret_cast<Abc::M44d *>(matrix);
    abc_op->setMatrix(abc_matrix);
}

void abc_xform_op_get_vector(Abc_Xform_Op *op, Abc_V3d *r_vec)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto vec = abc_op->getVector();
    *r_vec = *reinterpret_cast<Abc_V3d *>(&vec);
}

void abc_xform_op_get_translate(Abc_Xform_Op *op, Abc_V3d *r_trans)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto trans = abc_op->getTranslate();
    *r_trans = *reinterpret_cast<Abc_V3d *>(&trans);
}

void abc_xform_op_get_scale(Abc_Xform_Op *op, Abc_V3d *r_scale)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto scale = abc_op->getScale();
    *r_scale = *reinterpret_cast<Abc_V3d *>(&scale);
}

void abc_xform_op_get_axis(Abc_Xform_Op *op, Abc_V3d *r_axis)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto axis = abc_op->getAxis();
    *r_axis = *reinterpret_cast<Abc_V3d *>(&axis);
}

double abc_xform_op_get_angle(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getAngle();
}

void abc_xform_op_get_matrix(Abc_Xform_Op *op, Abc_M44d *r_matrix)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    auto matrix = abc_op->getMatrix();
    *r_matrix = *reinterpret_cast<Abc_M44d *>(&matrix);
}

double abc_xform_op_get_x_rotation(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getXRotation();
}

double abc_xform_op_get_y_rotation(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getYRotation();
}

double abc_xform_op_get_z_rotation(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->getZRotation();
}

bool abc_xform_op_is_translate_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isTranslateOp();
}

bool abc_xform_op_is_scale_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isScaleOp();
}

bool abc_xform_op_is_rotate_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateOp();
}

bool abc_xform_op_is_matrix_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isMatrixOp();
}

bool abc_xform_op_is_rotate_x_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateXOp();
}

bool abc_xform_op_is_rotate_y_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateYOp();
}

bool abc_xform_op_is_rotate_z_op(Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    return abc_op->isRotateZOp();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Xform_Sample
 * \{ */

void abc_xform_sample_destroy(Abc_Xform_Sample *sample)
{
    if (sample) {
        kuri_deloge(sample->ctx_kuri, sample);
    }
}

uint64_t abc_xform_sample_add_traslate_or_scale_op(Abc_Xform_Sample *sample,
                                                   Abc_Xform_Op *translate_or_scale_op,
                                                   Abc_V3d *val)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(translate_or_scale_op);
    auto abc_val = convertis_vers_abc(val);
    return sample->sample.addOp(abc_op, abc_val);
}

uint64_t abc_xform_sample_add_rotate_op(Abc_Xform_Sample *sample,
                                        Abc_Xform_Op *rotate_op,
                                        Abc_V3d *axis,
                                        Abc_Degrees degrees)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(rotate_op);
    auto abc_axis = convertis_vers_abc(axis);
    return sample->sample.addOp(abc_op, abc_axis, degrees);
}

uint64_t abc_xform_sample_add_matrix_op(Abc_Xform_Sample *sample,
                                        Abc_Xform_Op *matrix_op,
                                        Abc_M44d *matrix)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(matrix_op);
    auto abc_matrix = convertis_vers_abc(matrix);
    return sample->sample.addOp(abc_op, abc_matrix);
}

uint64_t abc_xform_sample_add_single_rotate_op(Abc_Xform_Sample *sample,
                                               Abc_Xform_Op *single_rotate_op,
                                               Abc_Degrees single_axis_rotation)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(single_rotate_op);
    return sample->sample.addOp(abc_op, single_axis_rotation);
}

uint64_t abc_xform_sample_add_op(Abc_Xform_Sample *sample, Abc_Xform_Op *op)
{
    auto abc_op = *reinterpret_cast<AbcGeom::XformOp *>(op);
    return sample->sample.addOp(abc_op);
}

void abc_xform_sample_get_op(struct Abc_Xform_Sample *sample,
                             uint64_t index,
                             struct Abc_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::XformOp *>(op);
    *abc_op = sample->sample.getOp(index);
}

uint64_t abc_xform_sample_get_num_ops(struct Abc_Xform_Sample *sample)
{
    return sample->sample.getNumOps();
}

uint64_t abc_xform_sample_get_num_op_channels(struct Abc_Xform_Sample *sample)
{
    return sample->sample.getNumOpChannels();
}

void abc_xform_sample_set_inherits_xforms(Abc_Xform_Sample *sample, bool inherits)
{
    sample->sample.setInheritsXforms(inherits);
}

bool abc_xform_sample_get_inherits_xforms(Abc_Xform_Sample *sample)
{
    return sample->sample.getInheritsXforms();
}

void abc_xform_sample_set_translation(Abc_Xform_Sample *sample, Abc_V3d *trans)
{
    auto abc_trans = convertis_vers_abc(trans);
    sample->sample.setTranslation(abc_trans);
}

void abc_xform_sample_get_translation(Abc_Xform_Sample *sample, Abc_V3d *r_trans)
{
    auto trans = sample->sample.getTranslation();
    convertis_vers_kuri(r_trans, trans);
}

void abc_xform_sample_set_rotation(Abc_Xform_Sample *sample, Abc_V3d *axis, Abc_Degrees degrees)
{
    auto abc_axis = convertis_vers_abc(axis);
    sample->sample.setRotation(abc_axis, degrees);
}

void abc_xform_sample_et_axis(Abc_Xform_Sample *sample, Abc_V3d *r_axis)
{
    auto axis = sample->sample.getAxis();
    convertis_vers_kuri(r_axis, axis);
}

double abc_xform_sample_get_angle(Abc_Xform_Sample *sample)
{
    return sample->sample.getAngle();
}

void abc_xform_sample_set_x_rotation(Abc_Xform_Sample *sample, Abc_Degrees degrees)
{
    return sample->sample.setXRotation(degrees);
}

void abc_xform_sample_get_x_rotation(Abc_Xform_Sample *sample, Abc_Degrees *r_degrees)
{
    *r_degrees = sample->sample.getXRotation();
}

void abc_xform_sample_set_y_rotation(Abc_Xform_Sample *sample, Abc_Degrees degrees)
{
    return sample->sample.setYRotation(degrees);
}

void abc_xform_sample_get_y_rotation(Abc_Xform_Sample *sample, Abc_Degrees *r_degrees)
{
    *r_degrees = sample->sample.getYRotation();
}

void abc_xform_sample_set_z_rotation(Abc_Xform_Sample *sample, Abc_Degrees degrees)
{
    return sample->sample.setZRotation(degrees);
}

void abc_xform_sample_get_z_rotation(Abc_Xform_Sample *sample, Abc_Degrees *r_degrees)
{
    *r_degrees = sample->sample.getZRotation();
}

void abc_xform_sample_set_scale(Abc_Xform_Sample *sample, Abc_V3d *scale)
{
    auto abc_scale = convertis_vers_abc(scale);
    sample->sample.setScale(abc_scale);
}

void abc_xform_sample_get_scale(Abc_Xform_Sample *sample, Abc_V3d *r_scale)
{
    auto scale = sample->sample.getScale();
    convertis_vers_kuri(r_scale, scale);
}

void abc_xform_sample_set_matrix(Abc_Xform_Sample *sample, Abc_M44d *matrix)
{
    auto abc_matrix = convertis_vers_abc(matrix);
    sample->sample.setMatrix(abc_matrix);
}

void abc_xform_sample_get_matrix(Abc_Xform_Sample *sample, Abc_M44d *r_matrix)
{
    auto matrix = sample->sample.getMatrix();
    convertis_vers_kuri(r_matrix, matrix);
}

bool abc_xform_sample_is_topology_equal(Abc_Xform_Sample *sample, Abc_Xform_Sample *other)
{
    return sample->sample.isTopologyEqual(other->sample);
}

bool abc_xform_sample_get_is_topology_frozen(Abc_Xform_Sample *sample)
{
    return sample->sample.getIsTopologyFrozen();
}

void abc_xform_sample_reset(Abc_Xform_Sample *sample)
{
    sample->sample.reset();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Film_Back_Xform_Op
 * \{ */

static_assert(sizeof(Abc_Film_Back_Xform_Op) == sizeof(AbcGeom::FilmBackXformOp));
static_assert(alignof(Abc_Film_Back_Xform_Op) == alignof(AbcGeom::FilmBackXformOp));

void abc_film_back_xform_op_init(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    new (abc_op) AbcGeom::FilmBackXformOp();
}

void abc_film_back_xform_op_init_type_hint(struct Abc_Film_Back_Xform_Op *op,
                                           enum Abc_Film_Back_Xform_Operation_Type type,
                                           struct Abc_String hint)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_type = static_cast<AbcGeom::FilmBackXformOperationType>(type);
    new (abc_op) AbcGeom::FilmBackXformOp(abc_type, hint);
}

Abc_Film_Back_Xform_Operation_Type abc_film_back_xform_op_get_type(
    struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return static_cast<Abc_Film_Back_Xform_Operation_Type>(abc_op->getType());
}

Abc_New_String abc_film_back_xform_op_get_hint(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto hint = abc_op->getHint();
    return make_new_string(hint);
}

Abc_New_String abc_film_back_xform_op_get_type_and_hint(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto type_and_hint = abc_op->getTypeAndHint();
    return make_new_string(type_and_hint);
}

uint64_t abc_film_back_xform_op_get_num_channels(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->getNumChannels();
}

double abc_film_back_xform_op_get_channel_value(struct Abc_Film_Back_Xform_Op *op, uint64_t index)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->getChannelValue(index);
}

void abc_film_back_xform_op_set_channel_value(struct Abc_Film_Back_Xform_Op *op,
                                              uint64_t index,
                                              double val)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->setChannelValue(index, val);
}

void abc_film_back_xform_op_setTranslate(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *trans)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_trans = convertis_vers_abc(trans);
    abc_op->setTranslate(abc_trans);
}

void abc_film_back_xform_op_setScale(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *scale)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_scale = convertis_vers_abc(scale);
    abc_op->setScale(abc_scale);
}

void abc_film_back_xform_op_setMatrix(struct Abc_Film_Back_Xform_Op *op, Abc_M33d *matrix)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_matrix = convertis_vers_abc(matrix);
    abc_op->setMatrix(abc_matrix);
}

void abc_film_back_xform_op_getTranslate(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *result)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_trans = abc_op->getTranslate();
    convertis_vers_kuri(result, abc_trans);
}

void abc_film_back_xform_op_getScale(struct Abc_Film_Back_Xform_Op *op, Abc_V2d *result)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_scale = abc_op->getScale();
    convertis_vers_kuri(result, abc_scale);
}

void abc_film_back_xform_op_getMatrix(struct Abc_Film_Back_Xform_Op *op, Abc_M33d *result)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    auto abc_matrix = abc_op->getMatrix();
    convertis_vers_kuri(result, abc_matrix);
}

bool abc_film_back_xform_op_isTranslateOp(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->isTranslateOp();
}

bool abc_film_back_xform_op_isScaleOp(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->isScaleOp();
}

bool abc_film_back_xform_op_isMatrixOp(struct Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return abc_op->isMatrixOp();
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \nom Abc_Camera_Sample
 * \{ */

void abc_camera_sample_get_screen_window(struct Abc_Camera_Sample *sample,
                                         double *r_top,
                                         double *r_bottom,
                                         double *r_left,
                                         double *r_right)
{
    double top;
    double bottom;
    double left;
    double right;
    sample->sample.getScreenWindow(top, bottom, left, right);
    *r_top = top;
    *r_bottom = bottom;
    *r_left = left;
    *r_right = right;
}

#define DEFINE_CAMERA_SAMPLE_GET_SET(nom_ipa, nom_method, type_ipa)                               \
    type_ipa abc_camera_sample_get_##nom_ipa(struct Abc_Camera_Sample *sample)                    \
    {                                                                                             \
        return sample->sample.get##nom_method();                                                  \
    }                                                                                             \
    void abc_camera_sample_set_##nom_ipa(struct Abc_Camera_Sample *sample, type_ipa value)        \
    {                                                                                             \
        sample->sample.set##nom_method(value);                                                    \
    }

ENUMERATE_CAMERA_SAMPLE_PROPERTIES_SIMPLE(DEFINE_CAMERA_SAMPLE_GET_SET);

#undef DEFINE_CAMERA_SAMPLE_GET_SET

#define DEFINE_CAMERA_SAMPLE_GET_SET(nom_ipa, nom_method, type_ipa)                               \
    void abc_camera_sample_get_##nom_ipa(struct Abc_Camera_Sample *sample, type_ipa *value)       \
    {                                                                                             \
        *value = sample->sample.get##nom_method();                                                \
    }                                                                                             \
    void abc_camera_sample_set_##nom_ipa(struct Abc_Camera_Sample *sample, type_ipa *value)       \
    {                                                                                             \
        sample->sample.set##nom_method(*value);                                                   \
    }

ENUMERATE_CAMERA_SAMPLE_PROPERTIES_COMPLEX(DEFINE_CAMERA_SAMPLE_GET_SET);

void abc_camera_sample_get_child_bounds(struct Abc_Camera_Sample *sample, Abc_Box3d *r_value)
{
    auto value = sample->sample.getChildBounds();
    convertis_vers_kuri(r_value, value);
}

void abc_camera_sample_set_child_bounds(struct Abc_Camera_Sample *sample, Abc_Box3d *value)
{
    auto abc_value = convertis_vers_abc(value);
    sample->sample.setChildBounds(abc_value);
}

#undef DEFINE_CAMERA_SAMPLE_GET_SET

void abc_camera_sample_reset(struct Abc_Camera_Sample *sample)
{
    sample->sample.reset();
}

void abc_camera_sample_destroy(struct Abc_Camera_Sample *sample)
{
    if (sample) {
        kuri_deloge(sample->ctx_kuri, sample);
    }
}

double abc_camera_sample_get_core_value(struct Abc_Camera_Sample *sample, uint64_t index)
{
    return sample->sample.getCoreValue(index);
}

double abc_camera_sample_get_field_of_view(struct Abc_Camera_Sample *sample)
{
    return sample->sample.getFieldOfView();
}

uint64_t abc_camera_sample_add_op(Abc_Camera_Sample *sample, Abc_Film_Back_Xform_Op *op)
{
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(op);
    return sample->sample.addOp(*abc_op);
}

void abc_camera_sample_get_op(Abc_Camera_Sample *sample,
                              uint64_t index,
                              Abc_Film_Back_Xform_Op *r_op)
{
    auto op = sample->sample.getOp(index);
    auto abc_op = reinterpret_cast<AbcGeom::FilmBackXformOp *>(r_op);
    *abc_op = op;
}

void abc_camera_sample_get_film_back_matrix(Abc_Camera_Sample *sample, Abc_M33d *r_matrix)
{
    auto matrix = sample->sample.getFilmBackMatrix();
    convertis_vers_kuri(r_matrix, matrix);
}

uint64_t abc_camera_sample_get_num_ops(Abc_Camera_Sample *sample)
{
    return sample->sample.getNumOps();
}

uint64_t abc_camera_sample_get_num_op_channels(Abc_Camera_Sample *sample)
{
    return sample->sample.getNumOpChannels();
}

/** \} */
