/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "image.h"

#include "oiio.h"

#include <cassert>
#include <cmath>
#include <math.h>
#include <string.h>
#include <string>

#include "champs_de_distance.hh"
#include "filtrage.hh"
#include "simulation_grain.hh"

#define NANOSVG_IMPLEMENTATION
#define NANOSVG_ALL_COLOR_KEYWORDS
#include "nanosvg/nanosvg.h"
#undef NANOSVG_ALL_COLOR_KEYWORDS
#undef NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"
#undef NANOSVGRAST_IMPLEMENTATION

#define RETOURNE_SI_NUL(x)                                                                        \
    if (!(x)) {                                                                                   \
        return;                                                                                   \
    }

#define PASSE_APPEL(fonction, type_params)                                                        \
    void IMG_##fonction(const type_params *params,                                                \
                        const struct AdaptriceImage *image_entree,                                \
                        struct AdaptriceImage *image_sortie)                                      \
    {                                                                                             \
        RETOURNE_SI_NUL(params);                                                                  \
        RETOURNE_SI_NUL(image_entree);                                                            \
        RETOURNE_SI_NUL(image_sortie);                                                            \
        image::fonction(*params, *image_entree, *image_sortie);                                   \
    }

// ----------------------------------------------------------------------------
// Simumlation de grain sur image

PASSE_APPEL(simule_grain_image, ParametresSimulationGrain)

// ----------------------------------------------------------------------------
// Filtrage de l'image

PASSE_APPEL(filtre_image, IMG_ParametresFiltrageImage)

// ----------------------------------------------------------------------------
// Affinage de l'image.

PASSE_APPEL(affine_image, IMG_ParametresAffinageImage)

// ----------------------------------------------------------------------------
// Dilatation de l'image.

PASSE_APPEL(dilate_image, IMG_ParametresDilatationImage)

// ----------------------------------------------------------------------------
// Érosion d'image.

PASSE_APPEL(erode_image, IMG_ParametresDilatationImage)

// ----------------------------------------------------------------------------
// Filtrage médian de l'image.

PASSE_APPEL(filtre_median_image, IMG_ParametresMedianImage)

// ----------------------------------------------------------------------------
// Filtrage bilatéral de l'image.

PASSE_APPEL(filtre_bilateral_image, IMG_ParametresFiltreBilateralImage)

// ----------------------------------------------------------------------------
// Champs de distance de l'image.

PASSE_APPEL(genere_champs_de_distance, IMG_ParametresChampsDeDistance)

// ----------------------------------------------------------------------------
// Défocalisation de l'image.

void IMG_defocalise_image(const struct AdaptriceImage *image_entree,
                          struct AdaptriceImage *image_sortie,
                          IMG_Fenetre *fenetre,
                          const float *rayon_flou_par_pixel)
{
    RETOURNE_SI_NUL(image_entree);
    RETOURNE_SI_NUL(image_sortie);
    RETOURNE_SI_NUL(fenetre);
    RETOURNE_SI_NUL(rayon_flou_par_pixel);
    image::defocalise_image(*image_entree, *image_sortie, *fenetre, rayon_flou_par_pixel);
}

// ----------------------------------------------------------------------------
// Rééchantillonnage de l'image.

PASSE_APPEL(reechantillonne_image, IMG_ParametresReechantillonnage)

// ----------------------------------------------------------------------------
// Image SVG.

bool SVG_parse_image_depuis_contenu(char *data, SVGImage *resultat)
{
    auto image = nsvgParse(data, "px", 96.0f);
    if (image == nullptr) {
        return false;
    }

    resultat->image = image;
    resultat->ratisseuse = nullptr;
    resultat->height = image->height;
    resultat->width = image->width;
    return true;
}

void SVG_image_ratisse(SVGImage *image, uint8_t *sortie, int largeur, int hauteur)
{
    if (image->ratisseuse == nullptr) {
        image->ratisseuse = nsvgCreateRasterizer();
    }

    auto échelle = 1.0f;

    if (image->width >= image->height) {
        échelle = float(largeur) / float(image->width);
    }
    else if (image->width < image->height) {
        échelle = float(hauteur) / float(image->height);
    }

    nsvgRasterize(image->ratisseuse,
                  image->image,
                  0.0f,
                  0.0f,
                  échelle,
                  sortie,
                  largeur,
                  hauteur,
                  largeur * 4);
}

void SVG_image_detruit(SVGImage *image)
{
    if (image->image) {
        nsvgDelete(image->image);
        image->image = nullptr;
    }
    if (image->ratisseuse) {
        nsvgDeleteRasterizer(image->ratisseuse);
        image->ratisseuse = nullptr;
    }
    image->width = 0.0f;
    image->height = 0.0f;
}

// ----------------------------------------------------------------------------
// OIIO.

int64_t OIIO_AutoStride()
{
    return OIIO::AutoStride;
}

static void make_new_string(std::string_view source, OIIO_NewString *résultat)
{
    if (source.size()) {
        résultat->characters = new char[source.size()];
        résultat->size = source.size();
        memcpy(const_cast<char *>(résultat->characters), source.data(), source.size());
    }
    else {
        résultat->characters = nullptr;
        résultat->size = 0;
    }
}

void OIIO_NewString_delete(struct OIIO_NewString *new_string)
{
    if (!new_string) {
        return;
    }

    delete[] new_string->characters;
    new_string->characters = nullptr;
    new_string->size = 0;
}

bool OIIO_has_error()
{
    return OIIO::has_error();
}

void OIIO_geterror(bool clear, OIIO_NewString *résultat)
{
    auto erreur = OIIO::geterror(clear);
    make_new_string(erreur, résultat);
}

int OIIO_get_int_attribute(struct OIIO_StringView name, int default_value)
{
    return OIIO::get_int_attribute(std::string_view(name.characters, name.size), default_value);
}

float OIIO_get_float_attribute(struct OIIO_StringView name, float default_value)
{
    return OIIO::get_float_attribute(std::string_view(name.characters, name.size), default_value);
}

struct OIIO_StringView OIIO_get_string_attribute(struct OIIO_StringView name,
                                                 struct OIIO_StringView default_value)
{
    auto attribut = OIIO::get_string_attribute(
        std::string_view(name.characters, name.size),
        std::string_view(default_value.characters, default_value.size));
    auto résultat = OIIO_StringView();
    résultat.characters = attribut.data();
    résultat.size = attribut.size();
    return résultat;
}

static_assert(sizeof(OIIO::ustring) == sizeof(OIIO_ustring));
static_assert(alignof(OIIO::ustring) == alignof(OIIO_ustring));

const char *OIIO_ustring_c_str(struct OIIO_ustring *str)
{
    auto oiio_str = reinterpret_cast<OIIO::ustring *>(str);
    return oiio_str->c_str();
}

uint64_t OIIO_ustring_size(struct OIIO_ustring *str)
{
    auto oiio_str = reinterpret_cast<OIIO::ustring *>(str);
    return oiio_str->size();
}

uint64_t OIIO_TypeDesc_basesize(struct OIIO_TypeDesc *type_desc)
{
    auto oiio_desc = reinterpret_cast<OIIO::TypeDesc *>(type_desc);
    return oiio_desc->basesize();
}

static_assert(sizeof(OIIO::ParamValue) == sizeof(OIIO_ParamValue));
static_assert(alignof(OIIO::ParamValue) == alignof(OIIO_ParamValue));

OIIO_TypeDesc OIIO_ParamValue_type(OIIO_ParamValue *param)
{
    auto oiio_param = reinterpret_cast<OIIO::ParamValue *>(param);
    auto résultat = oiio_param->type();
    return *reinterpret_cast<OIIO_TypeDesc *>(&résultat);
}

OIIO_StringView OIIO_ParamValue_name(OIIO_ParamValue *param)
{
    auto oiio_param = reinterpret_cast<OIIO::ParamValue *>(param);
    auto name = oiio_param->name();
    auto résultat = OIIO_StringView();
    résultat.characters = name.c_str();
    résultat.size = name.size();
    return résultat;
}

const void *OIIO_ParamValue_data(OIIO_ParamValue *param)
{
    auto oiio_param = reinterpret_cast<OIIO::ParamValue *>(param);
    return oiio_param->data();
}

int OIIO_ParamValue_nvalues(struct OIIO_ParamValue *param)
{
    auto oiio_param = reinterpret_cast<OIIO::ParamValue *>(param);
    return oiio_param->nvalues();
}

static_assert(sizeof(OIIO::ImageSpec) == sizeof(OIIO_ImageSpec));
static_assert(alignof(OIIO::ImageSpec) == alignof(OIIO_ImageSpec));

void OIIO_ImageSpec_init(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    new (oiio_spec) OIIO::ImageSpec;
}

int OIIO_ImageSpec_donne_width(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->width;
}

void OIIO_ImageSpec_definis_width(struct OIIO_ImageSpec *spec, int width)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    oiio_spec->width = width;
}

int OIIO_ImageSpec_donne_height(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->height;
}

void OIIO_ImageSpec_definis_height(struct OIIO_ImageSpec *spec, int height)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    oiio_spec->height = height;
}

int OIIO_ImageSpec_donne_nchannels(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->nchannels;
}

void OIIO_ImageSpec_definis_nchannels(struct OIIO_ImageSpec *spec, int nchannels)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    oiio_spec->nchannels = nchannels;
}

int OIIO_ImageSpec_donne_z_channel(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->z_channel;
}

struct OIIO_TypeDesc OIIO_ImageSpec_donne_format(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return *reinterpret_cast<OIIO_TypeDesc *>(&oiio_spec->format);
}

void OIIO_ImageSpec_donne_channel_name(struct OIIO_ImageSpec *spec,
                                       int channel_index,
                                       struct OIIO_StringView *résultat)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    auto channel_name = oiio_spec->channel_name(channel_index);
    résultat->characters = channel_name.data();
    résultat->size = channel_name.size();
}

OIIO_ParamValue *OIIO_ImageSpec_donne_extra_attribs(OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return reinterpret_cast<OIIO_ParamValue *>(oiio_spec->extra_attribs.data());
}

void OIIO_ImageSpec_definis_format(struct OIIO_ImageSpec *spec, struct OIIO_TypeDesc format)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    oiio_spec->format = *reinterpret_cast<OIIO::TypeDesc *>(&format);
}

uint64_t OIIO_ImageSpec_donne_extra_attribs_size(OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->extra_attribs.size();
}

struct OIIO_Filesystem_IOProxy *OIIO_Filesytem_IOMemReader_new(void *buf, uint64_t size)
{
    return reinterpret_cast<OIIO_Filesystem_IOProxy *>(
        new OIIO::Filesystem::IOMemReader(buf, size));
}

class Custom_IOProxy final : public OIIO::Filesystem::IOProxy {
    OIIO_IOProxy_Callbacks *m_callbacks = nullptr;

  public:
    Custom_IOProxy(OIIO_IOProxy_Callbacks *callbacks)
        : OIIO::Filesystem::IOProxy(), m_callbacks(callbacks)
    {
    }

    const char *proxytype() const override
    {
        if (m_callbacks->proxytype) {
            return m_callbacks->proxytype(m_callbacks);
        }
        return "";
    }

    void close() override
    {
        if (m_callbacks->close) {
            m_callbacks->close(m_callbacks);
        }
    }

    bool opened() const override
    {
        if (m_callbacks->opened) {
            m_callbacks->opened(m_callbacks);
        }
        return OIIO::Filesystem::IOProxy::opened();
    }

    int64_t tell() const override
    {
        if (m_callbacks->tell) {
            m_callbacks->tell(m_callbacks);
        }
        return OIIO::Filesystem::IOProxy::tell();
    }

    bool seek(int64_t offset) override
    {
        if (m_callbacks->seek) {
            m_callbacks->seek(m_callbacks, offset);
        }
        return OIIO::Filesystem::IOProxy::seek(offset);
    }

    size_t read(void *buf, size_t size) override
    {
        if (m_callbacks->read) {
            return m_callbacks->read(m_callbacks, buf, size);
        }
        return 0;
    }

    size_t write(const void *buf, size_t size) override
    {
        if (m_callbacks->write) {
            return m_callbacks->write(m_callbacks, buf, size);
        }
        return 0;
    }

    size_t pread(void *buf, size_t size, int64_t offset) override
    {
        if (m_callbacks->pread) {
            return m_callbacks->pread(m_callbacks, buf, size, offset);
        }
        return 0;
    }

    size_t pwrite(const void *buf, size_t size, int64_t offset) override
    {
        if (m_callbacks->pwrite) {
            return m_callbacks->pwrite(m_callbacks, buf, size, offset);
        }
        return 0;
    }

    size_t size() const override
    {
        if (m_callbacks->size) {
            m_callbacks->size(m_callbacks);
        }
        return OIIO::Filesystem::IOProxy::size();
    }

    void flush() override
    {
        if (m_callbacks->flush) {
            m_callbacks->flush(m_callbacks);
        }
    }
};

struct OIIO_Filesystem_IOProxy *OIIO_Filesystem_IOProxy_new_from_callbacks(
    struct OIIO_IOProxy_Callbacks *callbacks)
{
    auto résultat = reinterpret_cast<OIIO_Filesystem_IOProxy *>(new Custom_IOProxy(callbacks));
    callbacks->proxy = résultat;
    return résultat;
}

void OIIO_Filesystem_IOProxy_delete(struct OIIO_Filesystem_IOProxy *proxy)
{
    auto ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(proxy);
    delete ioproxy;
}

const char *OIIO_Filesytem_IOProxy_proxytype(struct OIIO_Filesystem_IOProxy *proxy)
{
    auto ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(proxy);
    return ioproxy->proxytype();
}

enum OIIO_Filesystem_IOProxy_Mode OIIO_Filesystem_IOProxy_mode(
    struct OIIO_Filesystem_IOProxy *proxy)
{
    auto ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(proxy);
    return static_cast<OIIO_Filesystem_IOProxy_Mode>(ioproxy->mode());
}

OIIO_StringView OIIO_Filesystem_IOProxy_filename(struct OIIO_Filesystem_IOProxy *proxy)
{
    auto ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(proxy);
    auto résultat = OIIO_StringView();
    résultat.characters = ioproxy->filename().c_str();
    résultat.size = ioproxy->filename().size();
    return résultat;
}

OIIO_ImageInput *OIIO_ImageInput_open(OIIO_StringView chemin,
                                      OIIO_ImageSpec *config,
                                      OIIO_Filesystem_IOProxy *ioproxy)
{
    auto filename = std::string(chemin.characters, chemin.size);
    auto oiio_config = reinterpret_cast<OIIO::ImageSpec *>(config);
    auto oiio_proxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(ioproxy);
    auto résultat = OIIO::ImageInput::open(filename, oiio_config, oiio_proxy);
    return reinterpret_cast<OIIO_ImageInput *>(résultat.release());
}

bool OIIO_ImageInput_close(OIIO_ImageInput *image)
{
    auto oiio_image = reinterpret_cast<OIIO::ImageInput *>(image);
    return oiio_image->close();
}

void OIIO_ImageInput_delete(OIIO_ImageInput *image)
{
    auto oiio_image = reinterpret_cast<OIIO::ImageInput *>(image);
    delete oiio_image;
}

OIIO_ImageSpec *OIIO_ImageInput_spec(OIIO_ImageInput *image)
{
    auto oiio_image = reinterpret_cast<OIIO::ImageInput *>(image);
    auto résultat = &oiio_image->spec();
    return reinterpret_cast<OIIO_ImageSpec *>(const_cast<OIIO::ImageSpec *>(résultat));
}

bool OIIO_ImageInput_supports(OIIO_ImageInput *image, OIIO_StringView feature)
{
    auto oiio_image = reinterpret_cast<OIIO::ImageInput *>(image);
    auto oiio_feature = std::string_view(feature.characters, feature.size);
    return oiio_image->supports(oiio_feature);
}

bool OIIO_ImageInput_read_image(OIIO_ImageInput *image,
                                int subimage,
                                int miplevel,
                                int chbegin,
                                int chend,
                                OIIO_TypeDesc format,
                                void *data,
                                int64_t xstride,
                                int64_t ystride,
                                int64_t zstride,
                                OIIO_ProgressCallback progress_callback,
                                void *progress_callback_data)
{
    auto oiio_image = reinterpret_cast<OIIO::ImageInput *>(image);
    auto oiio_format = *reinterpret_cast<OIIO::TypeDesc *>(&format);
    return oiio_image->read_image(subimage,
                                  miplevel,
                                  chbegin,
                                  chend,
                                  oiio_format,
                                  data,
                                  xstride,
                                  ystride,
                                  zstride,
                                  progress_callback,
                                  progress_callback_data);
}

bool OIIO_ImageInput_has_error(struct OIIO_ImageInput *image)
{
    auto oiio_image = reinterpret_cast<OIIO::ImageInput *>(image);
    return oiio_image->has_error();
}

void OIIO_ImageInput_geterror(struct OIIO_ImageInput *image, bool clear, OIIO_NewString *résultat)
{
    auto oiio_image = reinterpret_cast<OIIO::ImageInput *>(image);
    auto erreur = oiio_image->geterror(clear);
    make_new_string(erreur, résultat);
}

OIIO_ImageOutput *OIIO_ImageOutput_create(OIIO_StringView filename,
                                          OIIO_Filesystem_IOProxy *ioproxy,
                                          OIIO_StringView plugin_searchpath)
{
    auto oiio_filename = std::string_view(filename.characters, filename.size);
    auto oiio_ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(ioproxy);
    auto oiio_plugin_searchpath = std::string_view(plugin_searchpath.characters,
                                                   plugin_searchpath.size);
    auto résultat = OIIO::ImageOutput::create(oiio_filename, oiio_ioproxy, oiio_plugin_searchpath);
    return reinterpret_cast<OIIO_ImageOutput *>(résultat.release());
}

bool OIIO_ImageOutput_close(struct OIIO_ImageOutput *output)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    return oiio_output->close();
}

void OIIO_ImageOutput_delete(struct OIIO_ImageOutput *output)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    delete oiio_output;
}

bool OIIO_ImageOutput_open(struct OIIO_ImageOutput *output,
                           struct OIIO_StringView filename,
                           struct OIIO_ImageSpec *newspec,
                           enum OIIO_ImageOutput_OpenMode open_mode)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto oiio_filename = std::string(filename.characters, filename.size);
    auto oiio_newspec = reinterpret_cast<OIIO::ImageSpec *>(newspec);
    auto oiio_open_mode = static_cast<OIIO::ImageOutput::OpenMode>(open_mode);
    return oiio_output->open(oiio_filename, *oiio_newspec, oiio_open_mode);
}

bool OIIO_ImageOutput_open_subimages(struct OIIO_ImageOutput *output,
                                     struct OIIO_StringView filename,
                                     struct OIIO_ImageSpec *newspec,
                                     int specnum)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto oiio_filename = std::string(filename.characters, filename.size);
    auto oiio_newspec = reinterpret_cast<OIIO::ImageSpec *>(newspec);
    return oiio_output->open(oiio_filename, specnum, oiio_newspec);
}

OIIO_ImageSpec *OIIO_ImageOutput_spec(OIIO_ImageOutput *output)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto résultat = &oiio_output->spec();
    return reinterpret_cast<OIIO_ImageSpec *>(const_cast<OIIO::ImageSpec *>(résultat));
}

bool OIIO_ImageOutput_supports(OIIO_ImageOutput *output, OIIO_StringView feature)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto oiio_feature = std::string_view(feature.characters, feature.size);
    return oiio_output->supports(oiio_feature);
}

bool OIIO_ImageOutput_write_image(struct OIIO_ImageOutput *output,
                                  struct OIIO_TypeDesc format,
                                  const void *data,
                                  int64_t xstride,
                                  int64_t ystride,
                                  int64_t zstride,
                                  OIIO_ProgressCallback progress_callback,
                                  void *progress_callback_data)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto oiio_format = *reinterpret_cast<OIIO::TypeDesc *>(&format);
    return oiio_output->write_image(
        oiio_format, data, xstride, ystride, zstride, progress_callback, progress_callback_data);
}

bool OIIO_ImageOutput_write_scanline(int y,
                                     struct OIIO_ImageOutput *output,
                                     struct OIIO_TypeDesc format,
                                     uint8_t *bytes,
                                     uint64_t num_bytes)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto oiio_format = *reinterpret_cast<OIIO::TypeDesc *>(&format);
    auto const &spec = oiio_output->spec();
    const OIIO::span<std::byte> data(reinterpret_cast<std::byte *>(bytes), num_bytes);
    const OIIO::image_span<std::byte> image_data(data.data(), spec.nchannels, spec.width, 1, 1);
    return oiio_output->write_scanline(y, oiio_format, image_data);
}

bool OIIO_ImageOutput_write_scanlines(int ybegin,
                                      int yend,
                                      struct OIIO_ImageOutput *output,
                                      struct OIIO_TypeDesc format,
                                      uint8_t *bytes,
                                      uint64_t num_bytes)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto oiio_format = *reinterpret_cast<OIIO::TypeDesc *>(&format);
    auto const &spec = oiio_output->spec();
    const OIIO::span<std::byte> data(reinterpret_cast<std::byte *>(bytes), num_bytes);
    const OIIO::image_span<std::byte> image_data(data.data(), spec.nchannels, spec.width, 1, 1);
    return oiio_output->write_scanlines(ybegin, yend, oiio_format, image_data);
}

bool OIIO_ImageOutput_has_error(struct OIIO_ImageOutput *output)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    return oiio_output->has_error();
}

void OIIO_ImageOutput_geterror(struct OIIO_ImageOutput *output,
                               bool clear,
                               OIIO_NewString *résultat)
{
    auto oiio_output = reinterpret_cast<OIIO::ImageOutput *>(output);
    auto erreur = oiio_output->geterror(clear);
    make_new_string(erreur, résultat);
}
