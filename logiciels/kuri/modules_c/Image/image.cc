/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "image.h"

#include "oiio.h"

#include "biblinternes/outils/garde_portee.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <math.h>
#include <sstream>
#include <stdexcept>
#include <string.h>
#include <string>
#include <string_view>
#include <vector>

#include "champs_de_distance.hh"
#include "filtrage.hh"
#include "gif.hh"
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

static char characters[84] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~";

static char *encode_int(int value, int length, char *destination)
{
    int divisor = 1;
    for (int i = 0; i < length - 1; i++)
        divisor *= 83;

    for (int i = 0; i < length; i++) {
        int digit = (value / divisor) % 83;
        divisor /= 83;
        *destination++ = characters[digit];
    }
    return destination;
}

namespace blurhash {
struct Image {
    size_t width, height;
    std::vector<unsigned char> image;  // pixels rgb
};

// Decode a blurhash to an image with size width*height
Image decode(std::string blurhash, size_t width, size_t height, size_t bytesPerPixel = 3);

// Encode an image of rgb pixels (without padding) with size width*height into a blurhash with x*y
// components
std::string encode_byte(
    unsigned char *image, size_t width, size_t height, int nombre_canaux, int x, int y);
std::string encode_float(
    float *image, size_t width, size_t height, int nombre_canaux, int x, int y);
}  // namespace blurhash

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#    include <doctest.h>
#endif

// using namespace std::literals;

namespace {
constexpr std::array<char, 84> int_to_b83{
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~"};

std::string leftPad(std::string str, size_t len)
{
    if (str.size() >= len)
        return str;
    return str.insert(0, len - str.size(), '0');
}

std::string encode83(int value)
{
    std::string buffer;

    do {
        buffer += int_to_b83[static_cast<size_t>(value % 83)];
    } while ((value = value / 83));

    std::reverse(buffer.begin(), buffer.end());
    return buffer;
}

struct Components {
    int x, y;
};

int packComponents(const Components &c)
{
    return (c.x - 1) + (c.y - 1) * 9;
}

int encodeMaxAC(float maxAC)
{
    return std::max(0, std::min(82, static_cast<int>(maxAC * 166.0f - 0.5f)));
}

static float srgbToLinearF(float x)
{
    if (x <= 0.0f)
        return 0.0f;
    else if (x >= 1.0f)
        return 1.0f;
    else if (x < 0.04045f)
        return x / 12.92f;
    else
        return std::pow((x + 0.055f) / 1.055f, 2.4f);
}

static float srgbToLinear(int value)
{
    return srgbToLinearF(static_cast<float>(value) / 255.0f);
}

int linearToSrgb(float value)
{
    auto linearToSrgbF = [](float x) -> float {
        if (x <= 0.0f)
            return 0.0f;
        else if (x >= 1.0f)
            return 1.0f;
        else if (x < 0.0031308f)
            return x * 12.92f;
        else
            return std::pow(x, 1.0f / 2.4f) * 1.055f - 0.055f;
    };

    return int(linearToSrgbF(value) * 255.0f + 0.5f);
}

struct Color {
    float r, g, b;

    Color &operator*=(float scale)
    {
        r *= scale;
        g *= scale;
        b *= scale;
        return *this;
    }
    friend Color operator*(Color lhs, float rhs)
    {
        return (lhs *= rhs);
    }
    Color &operator/=(float scale)
    {
        r /= scale;
        g /= scale;
        b /= scale;
        return *this;
    }
    Color &operator+=(const Color &rhs)
    {
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }
};

int encodeDC(const Color &c)
{
    return (linearToSrgb(c.r) << 16) + (linearToSrgb(c.g) << 8) + linearToSrgb(c.b);
}

float signPow(float value, float exp)
{
    return std::copysign(std::pow(std::abs(value), exp), value);
}

int encodeAC(const Color &c, float maximumValue)
{
    auto quantR = int(
        std::max(0.0f, std::min(18.0f, std::floor(signPow(c.r / maximumValue, 0.5f) * 9 + 9.5f))));
    auto quantG = int(
        std::max(0.0f, std::min(18.0f, std::floor(signPow(c.g / maximumValue, 0.5f) * 9 + 9.5f))));
    auto quantB = int(
        std::max(0.0f, std::min(18.0f, std::floor(signPow(c.b / maximumValue, 0.5f) * 9 + 9.5f))));

    return quantR * 19 * 19 + quantG * 19 + quantB;
}

Color multiplyBasisFunction(
    Components components, int width, int height, int nombre_canaux, unsigned char *pixels)
{
    Color c{};
    float normalisation = (components.x == 0 && components.y == 0) ? 1 : 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float basis = static_cast<float>(std::cos(M_PI * components.x * x / double(width)) *
                                             std::cos(M_PI * components.y * y / double(height)));
            auto const index = (x + y * width) * nombre_canaux;

            c.r += basis * srgbToLinearF(pixels[index]);
            if (nombre_canaux > 1) {
                c.g += basis * srgbToLinearF(pixels[index + 1]);

                if (nombre_canaux > 2) {
                    c.b += basis * srgbToLinearF(pixels[index + 2]);
                }
            }
        }
    }

    float scale = normalisation / static_cast<float>(width * height);
    c *= scale;
    return c;
}

Color multiplyBasisFunction(
    Components components, int width, int height, int nombre_canaux, float *pixels)
{
    Color c{};
    float normalisation = (components.x == 0 && components.y == 0) ? 1 : 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float basis = static_cast<float>(std::cos(M_PI * components.x * x / double(width)) *
                                             std::cos(M_PI * components.y * y / double(height)));

            auto const index = (x + y * width) * nombre_canaux;

            c.r += basis * srgbToLinearF(pixels[index]);
            if (nombre_canaux > 1) {
                c.g += basis * srgbToLinearF(pixels[index + 1]);

                if (nombre_canaux > 2) {
                    c.b += basis * srgbToLinearF(pixels[index + 2]);
                }
            }
        }
    }

    float scale = normalisation / static_cast<float>(width * height);
    c *= scale;
    return c;
}
}  // namespace

namespace blurhash {
static char chars[84] =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~";

static inline uint8_t clampToUByte(int *src)
{
    if (*src >= 0 && *src <= 255)
        return *src;
    return (*src < 0) ? 0 : 255;
}

static inline uint8_t *createByteArray(int size)
{
    return (uint8_t *)malloc(size * sizeof(uint8_t));
}

int decodeToInt(const char *string, int start, int end)
{
    int value = 0, iter1 = 0, iter2 = 0;
    for (iter1 = start; iter1 < end; iter1++) {
        int index = -1;
        for (iter2 = 0; iter2 < 83; iter2++) {
            if (chars[iter2] == string[iter1]) {
                index = iter2;
                break;
            }
        }
        if (index == -1)
            return -1;
        value = value * 83 + index;
    }
    return value;
}

bool isValidBlurhash(const char *blurhash)
{

    const int hashLength = strlen(blurhash);

    if (!blurhash || strlen(blurhash) < 6)
        return false;

    int sizeFlag = decodeToInt(blurhash, 0, 1);  // Get size from first character
    int numY = (int)floorf(sizeFlag / 9) + 1;
    int numX = (sizeFlag % 9) + 1;

    if (hashLength != 4 + 2 * numX * numY)
        return false;
    return true;
}

static int linearTosRGB(float value)
{
    float v = fmaxf(0, fminf(1, value));
    if (v <= 0.0031308)
        return v * 12.92 * 255 + 0.5;
    else
        return (1.055 * powf(v, 1 / 2.4) - 0.055) * 255 + 0.5;
}

static float sRGBToLinear(int value)
{
    float v = (float)value / 255;
    if (v <= 0.04045)
        return v / 12.92;
    else
        return powf((v + 0.055) / 1.055, 2.4);
}

void decodeDC(int value, float *r, float *g, float *b)
{
    *r = sRGBToLinear(value >> 16);         // R-component
    *g = sRGBToLinear((value >> 8) & 255);  // G-Component
    *b = sRGBToLinear(value & 255);         // B-Component
}

void decodeAC(int value, float maximumValue, float *r, float *g, float *b)
{
    int quantR = (int)floorf(value / (19 * 19));
    int quantG = (int)floorf(value / 19) % 19;
    int quantB = (int)value % 19;

    *r = signPow(((float)quantR - 9) / 9, 2.0) * maximumValue;
    *g = signPow(((float)quantG - 9) / 9, 2.0) * maximumValue;
    *b = signPow(((float)quantB - 9) / 9, 2.0) * maximumValue;
}

int decodeToArray(
    const char *blurhash, int width, int height, int punch, int nChannels, uint8_t *pixelArray)
{
    if (!isValidBlurhash(blurhash))
        return -1;
    if (punch < 1)
        punch = 1;

    int sizeFlag = decodeToInt(blurhash, 0, 1);
    int numY = (int)floorf(sizeFlag / 9) + 1;
    int numX = (sizeFlag % 9) + 1;
    int iter = 0;

    float r = 0, g = 0, b = 0;
    int quantizedMaxValue = decodeToInt(blurhash, 1, 2);
    if (quantizedMaxValue == -1)
        return -1;

    float maxValue = ((float)(quantizedMaxValue + 1)) / 166;

    int colors_size = numX * numY;
    float colors[colors_size][3];

    for (iter = 0; iter < colors_size; iter++) {
        if (iter == 0) {
            int value = decodeToInt(blurhash, 2, 6);
            if (value == -1)
                return -1;
            decodeDC(value, &r, &g, &b);
            colors[iter][0] = r;
            colors[iter][1] = g;
            colors[iter][2] = b;
        }
        else {
            int value = decodeToInt(blurhash, 4 + iter * 2, 6 + iter * 2);
            if (value == -1)
                return -1;
            decodeAC(value, maxValue * punch, &r, &g, &b);
            colors[iter][0] = r;
            colors[iter][1] = g;
            colors[iter][2] = b;
        }
    }

    int bytesPerRow = width * nChannels;
    int x = 0, y = 0, i = 0, j = 0;
    int intR = 0, intG = 0, intB = 0;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {

            float r = 0, g = 0, b = 0;

            for (j = 0; j < numY; j++) {
                for (i = 0; i < numX; i++) {
                    float basics = cos((M_PI * x * i) / width) * cos((M_PI * y * j) / height);
                    int idx = i + j * numX;
                    r += colors[idx][0] * basics;
                    g += colors[idx][1] * basics;
                    b += colors[idx][2] * basics;
                }
            }

            intR = linearTosRGB(r);
            intG = linearTosRGB(g);
            intB = linearTosRGB(b);

            pixelArray[nChannels * x + 0 + y * bytesPerRow] = clampToUByte(&intR);
            pixelArray[nChannels * x + 1 + y * bytesPerRow] = clampToUByte(&intG);
            pixelArray[nChannels * x + 2 + y * bytesPerRow] = clampToUByte(&intB);

            if (nChannels == 4)
                pixelArray[nChannels * x + 3 + y * bytesPerRow] =
                    255;  // If nChannels=4, treat each pixel as RGBA instead of RGB
        }
    }

    return 0;
}

uint8_t *decode(const char *blurhash, int width, int height, int punch, int nChannels)
{
    int bytesPerRow = width * nChannels;
    uint8_t *pixelArray = createByteArray(bytesPerRow * height);

    if (decodeToArray(blurhash, width, height, punch, nChannels, pixelArray) == -1)
        return NULL;
    return pixelArray;
}

void freePixelArray(uint8_t *pixelArray)
{
    if (pixelArray) {
        free(pixelArray);
    }
}

static std::string encode_factors(float *dc, int components_x, int components_y)
{
    float *ac = dc + 3;
    int acCount = components_x * components_y - 1;

    char buffer[2 + 4 + (9 * 9 - 1) * 2 + 1];
    char *ptr = buffer;

    int sizeFlag = (components_x - 1) + (components_y - 1) * 9;
    ptr = encode_int(sizeFlag, 1, ptr);

    float maximumValue;
    if (acCount > 0) {
        float actualMaximumValue = 0;
        for (int i = 0; i < acCount * 3; i++) {
            actualMaximumValue = fmaxf(fabsf(ac[i]), actualMaximumValue);
        }

        int quantisedMaximumValue = fmaxf(0, fminf(82, floorf(actualMaximumValue * 166 - 0.5)));
        maximumValue = ((float)quantisedMaximumValue + 1) / 166;
        ptr = encode_int(quantisedMaximumValue, 1, ptr);
    }
    else {
        maximumValue = 1;
        ptr = encode_int(0, 1, ptr);
    }

    ptr = encode_int(encodeDC(Color{dc[0], dc[1], dc[2]}), 4, ptr);

    for (int i = 0; i < acCount; i++) {
        ptr = encode_int(
            encodeAC(Color{ac[i * 3 + 0], ac[i * 3 + 1], ac[i * 3 + 2]}, maximumValue), 2, ptr);
    }

    *ptr = 0;

    return buffer;
}

std::string encode_byte(unsigned char *image,
                        size_t width,
                        size_t height,
                        int nombre_canaux,
                        int components_x,
                        int components_y)
{
    if (width < 1 || height < 1 || components_x < 1 || components_x > 9 || components_y < 1 ||
        components_y > 9 || !image)
        return "";

    float factors[components_y][components_x][3];
    for (int y = 0; y < components_y; y++) {
        for (int x = 0; x < components_x; x++) {
            auto const factor = multiplyBasisFunction(
                {x, y}, static_cast<int>(width), static_cast<int>(height), nombre_canaux, image);
            factors[y][x][0] = factor.r;
            factors[y][x][1] = factor.g;
            factors[y][x][2] = factor.b;
        }
    }

    float *dc = factors[0][0];
    return encode_factors(dc, components_x, components_y);
}

std::string encode_float(float *image,
                         size_t width,
                         size_t height,
                         int nombre_canaux,
                         int components_x,
                         int components_y)
{
    if (width < 1 || height < 1 || components_x < 1 || components_x > 9 || components_y < 1 ||
        components_y > 9 || !image)
        return "";

    float factors[components_y][components_x][3];
    for (int y = 0; y < components_y; y++) {
        for (int x = 0; x < components_x; x++) {
            auto const factor = multiplyBasisFunction(
                {x, y}, static_cast<int>(width), static_cast<int>(height), nombre_canaux, image);
            factors[y][x][0] = factor.r;
            factors[y][x][1] = factor.g;
            factors[y][x][2] = factor.b;
        }
    }

    float *dc = factors[0][0];
    return encode_factors(dc, components_x, components_y);
}
}  // namespace blurhash

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

ImageIO_Chaine IMG_donne_filtre_extensions()
{
    auto map = OIIO::get_extension_map();

    std::stringstream flux_tous_fichiers;
    flux_tous_fichiers << "Images";
    auto virgule = " (";
    for (auto const &pair : map) {
        for (auto const &v : pair.second) {
            flux_tous_fichiers << virgule << "*." << v;
            virgule = " ";
        }
    }
    flux_tous_fichiers << ");;";

    std::stringstream flux;
    for (auto const &pair : map) {
        flux << pair.first;
        virgule = " (";
        for (auto const &v : pair.second) {
            flux << virgule << "*." << v;
            virgule = " ";
        }
        flux << ");;";
    }

    flux_tous_fichiers << flux.str();

    auto const str = flux_tous_fichiers.str();
    /* -2 pour supprimer les derniers point-virugles. */
    auto const taille = str.size() - 2;
    auto caractères = new char[taille];

    memcpy(caractères, str.data(), taille);

    ImageIO_Chaine résultat;
    résultat.caractères = caractères;
    résultat.taille = taille;
    return résultat;
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

struct ImageIOProxy *IMG_cree_proxy_memoire(void *buf, uint64_t size)
{
    return reinterpret_cast<ImageIOProxy *>(new OIIO::Filesystem::IOMemReader(buf, size));
}

void IMG_detruit_proxy(ImageIOProxy *proxy)
{
    auto ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(proxy);
    delete ioproxy;
}

ResultatOperation IMG_ouvre_image(const char *chemin, ImageIO *image)
{
    return IMG_ouvre_image_avec_proxy(chemin, image, nullptr);
}

ResultatOperation IMG_ouvre_image_avec_proxy(const char *chemin,
                                             ImageIO *image,
                                             ImageIOProxy *proxy)
{
    auto ioproxy = reinterpret_cast<OIIO::Filesystem::IOProxy *>(proxy);
    auto input = OIIO::ImageInput::open(chemin, nullptr, ioproxy);

    if (input == nullptr) {
        return ResultatOperation::TYPE_IMAGE_NON_SUPPORTE;
    }

    if (proxy && !input->supports("ioproxy")) {
        return ResultatOperation::PROXY_NON_SUPPORTE;
    }

    const auto &spec = input->spec();
    int xres = spec.width;
    int yres = spec.height;
    int channels = spec.nchannels;

    image->donnees = new float[xres * yres * channels];
    image->taille_donnees = xres * yres * channels;
    image->largeur = xres;
    image->hauteur = yres;
    image->nombre_composants = channels;

    if (!input->read_image(image->donnees)) {
        input->close();
        return ResultatOperation::TYPE_IMAGE_NON_SUPPORTE;
    }

    input->close();

    return ResultatOperation::OK;
}

static enum ResultatOperation img_ouvre_gif_impl(gd_GIF *gif, ImageIO *resultat)
{
    if (!gif) {
        return ResultatOperation::ERREUR_INCONNUE;
    }

    if (gd_get_frame(gif) == -1) {
        gd_close_gif(gif);
        return ResultatOperation::ERREUR_INCONNUE;
    }

    int xres = gif->width;
    int yres = gif->height;
    int channels = 3;

    resultat->donnees = new float[xres * yres * channels];
    resultat->taille_donnees = xres * yres * channels;
    resultat->largeur = xres;
    resultat->hauteur = yres;
    resultat->nombre_composants = channels;

    uint8_t *buffer = new uint8_t[xres * yres * channels];
    gd_render_frame(gif, buffer);

    uint8_t *color = buffer;
    float *pixel = resultat->donnees;

    for (int i = 0; i < gif->height; i++) {
        for (int j = 0; j < gif->width; j++) {
            pixel[0] = float(color[0]) / 255.0;
            pixel[1] = float(color[1]) / 255.0;
            pixel[2] = float(color[2]) / 255.0;
            color += 3;
            pixel += 3;
        }
    }

    delete[] buffer;
    gd_close_gif(gif);

    return ResultatOperation::OK;
}

enum ResultatOperation IMG_ouvre_gif_depuis_fichier(const char *chemin, struct ImageIO *resultat)
{
    gd_GIF *gif = gd_open_gif_from_file(chemin);
    return img_ouvre_gif_impl(gif, resultat);
}

enum ResultatOperation IMG_ouvre_gif_depuis_memoire(const void *donnees,
                                                    uint64_t taille,
                                                    struct ImageIO *resultat)
{
    gd_GIF *gif = gd_open_gif_from_memory(donnees, taille);
    return img_ouvre_gif_impl(gif, resultat);
}

ResultatOperation IMG_ecris_image(const char *chemin, ImageIO *image)
{
    auto out = OIIO::ImageOutput::create(chemin);

    if (out == nullptr) {
        return ResultatOperation::IMAGE_INEXISTANTE;
    }

    auto spec = OIIO::ImageSpec(
        image->largeur, image->hauteur, image->nombre_composants, OIIO::TypeDesc::FLOAT);
    out->open(chemin, spec);

    if (!out->write_image(OIIO::TypeDesc::FLOAT, image->donnees)) {
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

void IMG_calcule_empreinte_floue_octet(unsigned char *image,
                                       int largeur,
                                       int hauteur,
                                       int nombre_canaux,
                                       int composant_x,
                                       int composant_y,
                                       char *resultat,
                                       int64_t *taille_resultat)
{
    auto res = blurhash::encode_byte(image,
                                     static_cast<size_t>(largeur),
                                     static_cast<size_t>(hauteur),
                                     nombre_canaux,
                                     composant_x,
                                     composant_y);

    for (auto c : res) {
        *resultat++ = c;
    }

    *taille_resultat = static_cast<int64_t>(res.size());
}

void IMG_calcule_empreinte_floue_reel(float *image,
                                      int largeur,
                                      int hauteur,
                                      int nombre_canaux,
                                      int composant_x,
                                      int composant_y,
                                      char *resultat,
                                      int64_t *taille_resultat)
{
    auto res = blurhash::encode_float(image,
                                      static_cast<size_t>(largeur),
                                      static_cast<size_t>(hauteur),
                                      nombre_canaux,
                                      composant_x,
                                      composant_y);

    for (auto c : res) {
        *resultat++ = c;
    }

    *taille_resultat = static_cast<int64_t>(res.size());
}

uint8_t *IMG_decode_empreinte_floue(
    const char *empreinte, int largeur, int hauteur, int punch, int canaux)
{
    return blurhash::decode(empreinte, largeur, hauteur, punch, canaux);
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

    nsvgRasterize(
        image->ratisseuse, image->image, 0.0f, 0.0f, 1.0f, sortie, largeur, hauteur, largeur * 4);
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
