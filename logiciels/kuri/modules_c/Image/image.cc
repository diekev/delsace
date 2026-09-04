/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "image.h"

#include "oiio.h"

#include "biblinternes/outils/garde_portee.h"

#include <cassert>
#include <cmath>
#include <math.h>
#include <string.h>
#include <string>
#include <vector>

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

extern "C" {

void IMG_detruit_chaine(struct ImageIO_Chaine *chn)
{
    if (!chn) {
        return;
    }

    delete[] chn->caractères;
    chn->caractères = nullptr;
    chn->taille = 0;
}

struct NomCanal {
    std::string nom_calque{};
    std::string nom_canal{};
};

static NomCanal parse_nom_canal(std::string const &nom)
{
    auto const pos_point = nom.find('.');

    if (pos_point == std::string::npos) {
        return {"", nom};
    }

    auto resultat = NomCanal{};
    resultat.nom_calque = nom.substr(0, pos_point);
    resultat.nom_canal = nom.substr(pos_point + 1);
    return resultat;
}

struct DescriptionCanal {
    std::string nom{};
    int index{};
};

struct DescriptionCalque {
    std::string nom{};

    std::vector<DescriptionCanal> canaux{};
};

struct ParseuseDonneesImage {
    std::vector<DescriptionCalque> calques{};

    void parse_canaux(std::vector<std::string> const &canaux, int canal_z)
    {
        auto index = 0;
        for (auto const &name : canaux) {
            ajoute_canal(name, index++, canal_z);
        }
    }

    void ajoute_canal(std::string const &nom, int index, int canal_z)
    {
        auto nom_calque_canal = parse_nom_canal(nom);

        auto nom_calque = nom_calque_canal.nom_calque;

        if (nom_calque == "") {
            nom_calque = "rgba";
        }

        if (index == canal_z) {
            nom_calque = "depth";
        }

        auto &calque = trouve_ou_ajoute_calque(nom_calque);
        calque.canaux.push_back({nom_calque_canal.nom_canal, index});
    }

    DescriptionCalque &trouve_ou_ajoute_calque(std::string const &nom)
    {
        for (auto &calque : calques) {
            if (calque.nom == nom) {
                return calque;
            }
        }

        calques.emplace_back();
        calques.back().nom = nom;
        return calques.back();
    }
};

static bool rappel_progression(void *opaque_data, float portion_done)
{
    auto rappels = static_cast<ImageIO_RappelsProgression *>(opaque_data);
    return rappels->rappel_progression(rappels, portion_done);
}

static OIIO::TypeDesc donne_typedesc_depuis_data_type(ImageIO_DataType format)
{
#define CAS_RUBRIQUE_ENUM(nom_ipa, nom_oiio)                                                      \
    case nom_ipa:                                                                                 \
    {                                                                                             \
        return nom_oiio;                                                                          \
        break;                                                                                    \
    }
    switch (format) {
        ENUM_IMAGEIO_DATATYPE(CAS_RUBRIQUE_ENUM)
    }
    return OIIO::TypeDesc::FLOAT;
#undef CAS_RUBRIQUE_ENUM
}

static ImageIO_DataType convertis_data_type(OIIO::TypeDesc::BASETYPE type)
{
    switch (type) {
        ENUM_IMAGEIO_DATATYPE(ENUMERE_TRANSLATION_ENUM_NATIF_VERS_IPA)
        case OIIO::TypeDesc::LASTBASE:
        {
            return IMAGEIO_DATATYPE_UNKNOWN;
        }
    }
    return IMAGEIO_DATATYPE_UNKNOWN;
}

static ImageIO_AggregateType convertis_aggregate_type(OIIO::TypeDesc::AGGREGATE type)
{
    switch (type) {
        ENUM_IMAGEIO_AGGREGATETYPE(ENUMERE_TRANSLATION_ENUM_NATIF_VERS_IPA)
    }
    return IMAGEIO_AGGREGATETYPE_SCALAR;
}

ResultatOperation IMG_ouvre_image_avec_adaptrice(const char *chemin,
                                                 int64_t taille_chemin,
                                                 AdaptriceImage *image,
                                                 ImageIO_RappelsProgression *rappels,
                                                 ImageIO_Options_Lecture options)
{
    OIIO::ProgressCallback progress_callback = rappel_progression;

    if (!rappels || !rappels->rappel_progression) {
        rappels = nullptr;
        progress_callback = nullptr;
    }

    const auto chemin_ = std::string(chemin, size_t(taille_chemin));

    auto input = OIIO::ImageInput::open(chemin_);

    if (input == nullptr) {
        return ResultatOperation::TYPE_IMAGE_NON_SUPPORTE;
    }

    DIFFERE {
        input->close();
    };

    const auto &spec = input->spec();

    DescriptionImage desc_image{};
    desc_image.hauteur = spec.height;
    desc_image.largeur = spec.width;

    if ((options & IMAGEIO_OPTIONS_LECTURE_LIS_ATTRIBUTS) != 0 && image->ajoute_attribut) {
        for (auto const &param : spec.extra_attribs) {
            auto const &param_type = param.type();

            auto type_données = convertis_data_type(OIIO::TypeDesc::BASETYPE(param_type.basetype));
            auto type_aggregat = convertis_aggregate_type(
                OIIO::TypeDesc::AGGREGATE(param_type.aggregate));

            ImageIO_Chaine nom;
            nom.caractères = param.name().c_str();
            nom.taille = param.name().size();

            if (param.type() == OIIO::TypeDesc::STRING) {
                auto value = static_cast<const OIIO::ustring *>(param.data());

                ImageIO_Chaine chaine;
                chaine.caractères = value->c_str();
                chaine.taille = value->size();

                image->ajoute_attribut(
                    image, &nom, type_données, type_aggregat, &chaine, param.nvalues());
            }
            else {
                image->ajoute_attribut(
                    image, &nom, type_données, type_aggregat, param.data(), param.nvalues());
            }
        }
    }

    if ((options & IMAGEIO_OPTIONS_LECTURE_LIS_PIXELS) == 0) {
        return ResultatOperation::OK;
    }

    auto parseuse = ParseuseDonneesImage();
    parseuse.parse_canaux(spec.channelnames, spec.z_channel);

    auto format = OIIO::TypeDesc::FLOAT;

    image->initialise_image(image, &desc_image);

    for (auto const &desc_calque : parseuse.calques) {
        void *calque = image->cree_calque(
            image, desc_calque.nom.c_str(), int64_t(desc_calque.nom.size()));

        if (!calque) {
            return ResultatOperation::AJOUT_CALQUE_IMPOSSIBLE;
        }

        for (auto const &desc_canal : desc_calque.canaux) {
            void *canal = image->ajoute_canal(
                image, calque, desc_canal.nom.c_str(), int64_t(desc_canal.nom.size()));

            if (!canal) {
                return ResultatOperation::AJOUT_CANAL_IMPOSSIBLE;
            }

            float *donnees_canal = image->donnees_canal_pour_ecriture(image, canal);

            auto const index_canal = desc_canal.index;

            auto const succes = input->read_image(0,
                                                  0,
                                                  index_canal,
                                                  index_canal + 1,
                                                  format,
                                                  donnees_canal,
                                                  OIIO::AutoStride,
                                                  OIIO::AutoStride,
                                                  OIIO::AutoStride,
                                                  progress_callback,
                                                  rappels);
            if (!succes) {
                return ResultatOperation::LECTURE_DONNEES_IMPOSSIBLE;
            }
        }
    }

    return ResultatOperation::OK;
}

ImageIO_Chaine IMG_donne_liste_extensions(void)
{
    auto all_extensions = OIIO::get_string_attribute("extension_list");
    ImageIO_Chaine résultat;
    résultat.caractères = all_extensions.data();
    résultat.taille = all_extensions.size();
    return résultat;
}

void IMG_donne_erreur(ImageIO_Chaine *résultat)
{
    auto erreur = OIIO::geterror();
    if (erreur.size()) {
        résultat->caractères = new char[erreur.size()];
        résultat->taille = erreur.size();
        memcpy(const_cast<char *>(résultat->caractères), erreur.c_str(), erreur.size());
    }
    else {
        résultat->caractères = nullptr;
        résultat->taille = 0;
    }
}

// À FAIRE : paramétrise les calques à écrire.
ResultatOperation IMG_ecris_image_avec_adaptrice(const char *chemin,
                                                 int64_t taille_chemin,
                                                 AdaptriceImage *image,
                                                 ImageIO_RappelsProgression *rappels)
{
    OIIO::ProgressCallback progress_callback = rappel_progression;

    if (!rappels || !rappels->rappel_progression) {
        rappels = nullptr;
        progress_callback = nullptr;
    }

    const auto chemin_ = std::string(chemin, size_t(taille_chemin));
    auto out = OIIO::ImageOutput::create(chemin_);

    if (out == nullptr) {
        return ResultatOperation::IMAGE_INEXISTANTE;
    }

    DIFFERE {
        out->close();
    };

    DescriptionImage desc;
    image->decris_image(image, &desc);

    /* Considère uniquement le premier calque. */
    const void *calque = image->calque_pour_index(image, 0);
    const int nombre_de_canaux = image->nombre_de_canaux(image, calque);

    auto spec = OIIO::ImageSpec(
        desc.largeur, desc.hauteur, nombre_de_canaux, OIIO::TypeDesc::FLOAT);
    out->open(chemin_, spec);

    float *donnees = new float[desc.largeur * desc.hauteur * nombre_de_canaux];
    DIFFERE {
        delete[] donnees;
    };

    std::vector<const float *> canaux(nombre_de_canaux);
    for (int i = 0; i < nombre_de_canaux; i++) {
        auto canal = image->canal_pour_index(image, calque, i);
        canaux[i] = image->donnees_canal_pour_lecture(image, canal);
    }

    auto index = 0;
    for (int y = 0; y < desc.hauteur; y++) {
        for (int x = 0; x < desc.largeur; x++, index++) {
            auto index_ptr = index * nombre_de_canaux;

            for (int c = 0; c < nombre_de_canaux; c++) {
                donnees[index_ptr + c] = canaux[c][index];
            }
        }
    }

    if (!out->write_image(OIIO::TypeDesc::FLOAT,
                          donnees,
                          OIIO::AutoStride,
                          OIIO::AutoStride,
                          OIIO::AutoStride,
                          progress_callback,
                          rappels)) {
        return ResultatOperation::IMAGE_INEXISTANTE;
    }

    return ResultatOperation::OK;
}

ResultatOperation IMG_ecris_image(const char *chemin, ImageIO *image)
{
    if (!image || image->donnees == nullptr || image->taille_donnees == 0 || image->hauteur == 0 ||
        image->largeur == 0) {
        return ResultatOperation::IMAGE_NULLE;
    }

    auto out = OIIO::ImageOutput::create(chemin);

    if (out == nullptr) {
        return ResultatOperation::IMAGE_INEXISTANTE;
    }

    auto type_desc = donne_typedesc_depuis_data_type(image->format);

    auto spec = OIIO::ImageSpec(
        image->largeur, image->hauteur, image->nombre_composants, type_desc);
    out->open(chemin, spec);

    if (!out->write_image(type_desc, image->donnees)) {
        out->close();
        return ResultatOperation::IMAGE_INEXISTANTE;
    }

    out->close();

    return ResultatOperation::OK;
}

void IMG_detruit_image(ImageIO *image)
{
    delete[] image->donnees;
    image->donnees = nullptr;
    image->taille_donnees = 0;
    image->largeur = 0;
    image->hauteur = 0;
    image->nombre_composants = 0;
}
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

int OIIO_ParamValue_nvavlues(struct OIIO_ParamValue *param)
{
    auto oiio_param = reinterpret_cast<OIIO::ParamValue *>(param);
    return oiio_param->nvalues();
}

static_assert(sizeof(OIIO::ImageSpec) == sizeof(OIIO_ImageSpec));
static_assert(alignof(OIIO::ImageSpec) == alignof(OIIO_ImageSpec));

int OIIO_ImageSpec_donne_width(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->width;
}

int OIIO_ImageSpec_donne_height(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->height;
}

int OIIO_ImageSpec_donne_nchannels(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return oiio_spec->nchannels;
}

struct OIIO_TypeDesc OIIO_ImageSpec_donne_format(struct OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return *reinterpret_cast<OIIO_TypeDesc *>(&oiio_spec->format);
}

OIIO_ParamValue *OIIO_ImageSpec_donne_extra_attribs(OIIO_ImageSpec *spec)
{
    auto oiio_spec = reinterpret_cast<OIIO::ImageSpec *>(spec);
    return reinterpret_cast<OIIO_ParamValue *>(oiio_spec->extra_attribs.data());
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

void OIIO_Filesytem_IOMemReader_delete(struct OIIO_Filesystem_IOProxy *proxy)
{
    auto ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(proxy);
    delete ioproxy;
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
