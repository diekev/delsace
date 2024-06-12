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
#include "simulation_grain.hh"

#define RETOURNE_SI_NUL(x)                                                                        \
    if (!(x)) {                                                                                   \
        return;                                                                                   \
    }

#if 0
#    ifndef M_PI
#        define M_PI 3.14159265358979323846
#    endif

static float *multiplyBasisFunction(int xComponent, int yComponent, int width, int height, uint8_t *rgb, size_t bytesPerRow);
static char *encode_int(int value, int length, char *destination);

static int linearTosRGB(float value);
static float sRGBToLinear(int value);
static int encodeDC(float r, float g, float b);
static int encodeAC(float r, float g, float b, float maximumValue);
static float signPow(float value, float exp);

static const char *blurHashForPixels(int xComponents, int yComponents, int width, int height, uint8_t *rgb, size_t bytesPerRow) {
	static char buffer[2 + 4 + (9 * 9 - 1) * 2 + 1];

	if(xComponents < 1 || xComponents > 9) return nullptr;
	if(yComponents < 1 || yComponents > 9) return nullptr;

	float factors[yComponents][xComponents][3];
	memset(factors, 0, sizeof(factors));

	for(int y = 0; y < yComponents; y++) {
		for(int x = 0; x < xComponents; x++) {
			float *factor = multiplyBasisFunction(x, y, width, height, rgb, bytesPerRow);
			factors[y][x][0] = factor[0];
			factors[y][x][1] = factor[1];
			factors[y][x][2] = factor[2];
		}
	}

	float *dc = factors[0][0];
	float *ac = dc + 3;
	int acCount = xComponents * yComponents - 1;
	char *ptr = buffer;

	int sizeFlag = (xComponents - 1) + (yComponents - 1) * 9;
	ptr = encode_int(sizeFlag, 1, ptr);

	float maximumValue;
	if(acCount > 0) {
		float actualMaximumValue = 0;
		for(int i = 0; i < acCount * 3; i++) {
			actualMaximumValue = fmaxf(fabsf(ac[i]), actualMaximumValue);
		}

		int quantisedMaximumValue = fmaxf(0, fminf(82, floorf(actualMaximumValue * 166 - 0.5)));
		maximumValue = ((float)quantisedMaximumValue + 1) / 166;
		ptr = encode_int(quantisedMaximumValue, 1, ptr);
	} else {
		maximumValue = 1;
		ptr = encode_int(0, 1, ptr);
	}

	ptr = encode_int(encodeDC(dc[0], dc[1], dc[2]), 4, ptr);

	for(int i = 0; i < acCount; i++) {
		ptr = encode_int(encodeAC(ac[i * 3 + 0], ac[i * 3 + 1], ac[i * 3 + 2], maximumValue), 2, ptr);
	}

	*ptr = 0;

	return buffer;
}

static float *multiplyBasisFunction(int xComponent, int yComponent, int width, int height, uint8_t *rgb, size_t bytesPerRow) {
	float r = 0, g = 0, b = 0;
	float normalisation = (xComponent == 0 && yComponent == 0) ? 1 : 2;

	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			float basis = cosf(M_PI * xComponent * x / width) * cosf(M_PI * yComponent * y / height);
			r += basis * sRGBToLinear(rgb[3 * x + 0 + y * bytesPerRow]);
			g += basis * sRGBToLinear(rgb[3 * x + 1 + y * bytesPerRow]);
			b += basis * sRGBToLinear(rgb[3 * x + 2 + y * bytesPerRow]);
		}
	}

	float scale = normalisation / (width * height);

	static float result[3];
	result[0] = r * scale;
	result[1] = g * scale;
	result[2] = b * scale;

	return result;
}

static int linearTosRGB(float value) {
	float v = fmaxf(0, fminf(1, value));
	if(v <= 0.0031308) return v * 12.92 * 255 + 0.5;
	else return (1.055 * powf(v, 1 / 2.4) - 0.055) * 255 + 0.5;
}

static float sRGBToLinear(int value) {
	float v = (float)value / 255;
	if(v <= 0.04045) return v / 12.92;
	else return powf((v + 0.055) / 1.055, 2.4);
}

static int encodeDC(float r, float g, float b) {
	int roundedR = linearTosRGB(r);
	int roundedG = linearTosRGB(g);
	int roundedB = linearTosRGB(b);
	return (roundedR << 16) + (roundedG << 8) + roundedB;
}

static int encodeAC(float r, float g, float b, float maximumValue) {
	int quantR = fmaxf(0, fminf(18, floorf(signPow(r / maximumValue, 0.5) * 9 + 9.5)));
	int quantG = fmaxf(0, fminf(18, floorf(signPow(g / maximumValue, 0.5) * 9 + 9.5)));
	int quantB = fmaxf(0, fminf(18, floorf(signPow(b / maximumValue, 0.5) * 9 + 9.5)));

	return quantR * 19 * 19 + quantG * 19 + quantB;
}

static float signPow(float value, float exp) {
	return copysignf(powf(fabsf(value), exp), value);
}

static char characters[84]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~";

static char *encode_int(int value, int length, char *destination) {
	int divisor = 1;
	for(int i = 0; i < length - 1; i++) divisor *= 83;

	for(int i = 0; i < length; i++) {
		int digit = (value / divisor) % 83;
		divisor /= 83;
		*destination++ = characters[digit];
	}
	return destination;
}
#else

namespace blurhash {
struct Image {
    size_t width, height;
    std::vector<unsigned char> image;  // pixels rgb
};

// Decode a blurhash to an image with size width*height
Image decode(std::string blurhash, size_t width, size_t height, size_t bytesPerPixel = 3);

// Encode an image of rgb pixels (without padding) with size width*height into a blurhash with x*y
// components
std::string encode(unsigned char *image, size_t width, size_t height, int x, int y);
}  // namespace blurhash

#    ifndef M_PI
#        define M_PI 3.14159265358979323846
#    endif

#    ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#        include <doctest.h>
#    endif

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

// constexpr std::array<int, 255> b83_to_int = []() constexpr
//{
//		std::array<int, 255> a{};

//		for (auto &e : a)
//				e = -1;

//		for (int i = 0; i < 83; i++) {
//				a[static_cast<unsigned char>(int_to_b83[i])] = i;
//		}

//		return a;
//}
//();

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

// Components
// unpackComponents(int c)
//{
//	return {c % 9 + 1, c / 9 + 1};
//}

// int
// decode83(std::string value)
//{
//		int temp = 0;

//		for (char c : value)
//				if (b83_to_int[static_cast<unsigned char>(c)] < 0)
//						throw std::invalid_argument("invalid character in blurhash");

//		for (char c : value)
//				temp = temp * 83 + b83_to_int[static_cast<unsigned char>(c)];
//		return temp;
//}

// float
// decodeMaxAC(int quantizedMaxAC)
//{
//		return (quantizedMaxAC + 1) / 166.;
//}

// float
// decodeMaxAC(const std::string &maxAC)
//{
//		assert(maxAC.size() == 1);
//		return decodeMaxAC(decode83(maxAC));
//}

int encodeMaxAC(float maxAC)
{
    return std::max(0, std::min(82, static_cast<int>(maxAC * 166.0f - 0.5f)));
}

float srgbToLinear(int value)
{
    auto srgbToLinearF = [](float x) {
        if (x <= 0.0f)
            return 0.0f;
        else if (x >= 1.0f)
            return 1.0f;
        else if (x < 0.04045f)
            return x / 12.92f;
        else
            return std::pow((x + 0.055f) / 1.055f, 2.4f);
    };

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

// Color
// decodeDC(int value)
//{
//		const int intR = value >> 16;
//		const int intG = (value >> 8) & 255;
//		const int intB = value & 255;
//		return {srgbToLinear(intR), srgbToLinear(intG), srgbToLinear(intB)};
//}

// Color
// decodeDC(std::string value)
//{
//		assert(value.size() == 4);
//		return decodeDC(decode83(value));
//}

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

// Color decodeAC(int value, float maximumValue)
//{
//	auto quantR = value / (19 * 19);
//	auto quantG = (value / 19) % 19;
//	auto quantB = value % 19;

//	return {signPow((float(quantR) - 9) / 9, 2) * maximumValue,
//				signPow((float(quantG) - 9) / 9, 2) * maximumValue,
//				signPow((float(quantB) - 9) / 9, 2) * maximumValue};
//}

// Color
// decodeAC(std::string value, float maximumValue)
//{
//		return decodeAC(decode83(value), maximumValue);
//}

Color multiplyBasisFunction(Components components, int width, int height, unsigned char *pixels)
{
    Color c{};
    float normalisation = (components.x == 0 && components.y == 0) ? 1 : 2;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float basis = static_cast<float>(std::cos(M_PI * components.x * x / double(width)) *
                                             std::cos(M_PI * components.y * y / double(height)));
            c.r += basis * srgbToLinear(pixels[3 * x + 0 + y * width * 3]);
            c.g += basis * srgbToLinear(pixels[3 * x + 1 + y * width * 3]);
            c.b += basis * srgbToLinear(pixels[3 * x + 2 + y * width * 3]);
        }
    }

    float scale = normalisation / static_cast<float>(width * height);
    c *= scale;
    return c;
}
}  // namespace

namespace blurhash {
// Image
// decode(std::string blurhash, size_t width, size_t height, size_t bytesPerPixel)
//{
//		Image i{};

//		if (blurhash.size() < 10)
//				return i;

//		Components components{};
//		std::vector<Color> values;
//		values.reserve(blurhash.size() / 2);
//		try {
//				components = unpackComponents(decode83(blurhash.substr(0, 1)));

//				if (components.x < 1 || components.y < 1 ||
//					blurhash.size() != size_t(1 + 1 + 4 + (components.x * components.y - 1) * 2))
//						return {};

//				auto maxAC    = decodeMaxAC(blurhash.substr(1, 1));
//				Color average = decodeDC(blurhash.substr(2, 4));

//				values.push_back(average);
//				for (size_t c = 6; c < blurhash.size(); c += 2)
//						values.push_back(decodeAC(blurhash.substr(c, 2), maxAC));
//		} catch (std::invalid_argument &) {
//				return {};
//		}

//		i.image.reserve(height * width * bytesPerPixel);

//		for (size_t y = 0; y < height; y++) {
//				for (size_t x = 0; x < width; x++) {
//						Color c{};

//						for (size_t nx = 0; nx < size_t(components.x); nx++) {
//								for (size_t ny = 0; ny < size_t(components.y); ny++) {
//										float basis =
//										  std::cos(M_PI * float(x) * float(nx) / float(width)) *
//										  std::cos(M_PI * float(y) * float(ny) / float(height));
//										c += values[nx + ny * components.x] * basis;
//								}
//						}

//						i.image.push_back(static_cast<unsigned char>(linearToSrgb(c.r)));
//						i.image.push_back(static_cast<unsigned char>(linearToSrgb(c.g)));
//						i.image.push_back(static_cast<unsigned char>(linearToSrgb(c.b)));

//						for (size_t p = 3; p < bytesPerPixel; p++)
//								i.image.push_back(255);
//				}
//		}

//		i.height = height;
//		i.width  = width;

//		return i;
//}

std::string encode(
    unsigned char *image, size_t width, size_t height, int components_x, int components_y)
{
    if (width < 1 || height < 1 || components_x < 1 || components_x > 9 || components_y < 1 ||
        components_y > 9 || !image)
        return "";

    std::vector<Color> factors;
    factors.reserve(static_cast<size_t>(components_x * components_y));
    for (int y = 0; y < components_y; y++) {
        for (int x = 0; x < components_x; x++) {
            factors.push_back(multiplyBasisFunction(
                {x, y}, static_cast<int>(width), static_cast<int>(height), image));
        }
    }

    assert(factors.size() > 0);

    auto dc = factors.front();
    factors.erase(factors.begin());

    std::string h;

    h += leftPad(encode83(packComponents({components_x, components_y})), 1);

    float maximumValue;
    if (!factors.empty()) {
        float actualMaximumValue = 0;
        for (auto ac : factors) {
            actualMaximumValue = std::max({
                std::abs(ac.r),
                std::abs(ac.g),
                std::abs(ac.b),
                actualMaximumValue,
            });
        }

        int quantisedMaximumValue = encodeMaxAC(actualMaximumValue);
        maximumValue = (static_cast<float>(quantisedMaximumValue + 1)) / 166;
        h += leftPad(encode83(quantisedMaximumValue), 1);
    }
    else {
        maximumValue = 1;
        h += leftPad(encode83(0), 1);
    }

    h += leftPad(encode83(encodeDC(dc)), 4);

    for (auto ac : factors)
        h += leftPad(encode83(encodeAC(ac, maximumValue)), 2);

    return h;
}
}  // namespace blurhash
#endif

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

ResultatOperation IMG_ouvre_image(const char *chemin, ImageIO *image)
{
    auto input = OIIO::ImageInput::open(chemin);

    if (input == nullptr) {
        return ResultatOperation::TYPE_IMAGE_NON_SUPPORTE;
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

void IMG_calcul_empreinte_floue(
    const char *chemin, int composant_x, int composant_y, char *resultat, int64_t *taille_resultat)
{
    auto input = OIIO::ImageInput::open(chemin);

    if (input == nullptr) {
        return;
    }

    const auto &spec = input->spec();
    int xres = spec.width;
    int yres = spec.height;
    int channels = spec.nchannels;

    std::vector<uint8_t> donnees(static_cast<size_t>(xres * yres * channels));

    if (!input->read_image(OIIO::TypeDesc::UINT8, donnees.data())) {
        input->close();
        return;
    }

    auto res = blurhash::encode(donnees.data(),
                                static_cast<size_t>(xres),
                                static_cast<size_t>(yres),
                                composant_x,
                                composant_y);

    for (auto c : res) {
        *resultat++ = c;
    }

    *taille_resultat = static_cast<int64_t>(res.size());

    input->close();
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
