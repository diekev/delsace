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

#include "image.h"

#include <OpenImageIO/imageio.h>

#include <string.h>
#include <math.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

extern "C" {

ResultatOperation IMG_ouvre_image(const char *chemin, Image *image)
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
		OIIO::ImageInput::destroy(input);
		return ResultatOperation::TYPE_IMAGE_NON_SUPPORTE;
	}

	input->close();
	OIIO::ImageInput::destroy(input);

	return ResultatOperation::OK;
}

ResultatOperation IMG_ecris_image(const char *chemin, Image *image)
{
	auto out = OIIO::ImageOutput::create(chemin);

	if (out == nullptr) {
		return ResultatOperation::IMAGE_INEXISTANTE;
	}

	auto spec = OIIO::ImageSpec(image->largeur, image->hauteur, image->nombre_composants, OIIO::TypeDesc::FLOAT);
	out->open(chemin, spec);

	if (!out->write_image(OIIO::TypeDesc::FLOAT, image->donnees)) {
		out->close();
		OIIO::ImageOutput::destroy(out);
		return ResultatOperation::IMAGE_INEXISTANTE;
	}

	out->close();
	OIIO::ImageOutput::destroy(out);

	return ResultatOperation::OK;
}

void IMG_detruit_image(Image *image)
{
	delete [] image->donnees;
	image->donnees = nullptr;
	image->taille_donnees = 0;
	image->largeur = 0;
	image->hauteur = 0;
	image->nombre_composants = 0;
}

#if 0
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
struct Image
{
		size_t width, height;
		std::vector<unsigned char> image; // pixels rgb
};

// Decode a blurhash to an image with size width*height
Image
decode(std::string blurhash, size_t width, size_t height, size_t bytesPerPixel = 3);

// Encode an image of rgb pixels (without padding) with size width*height into a blurhash with x*y
// components
std::string
encode(unsigned char *image, size_t width, size_t height, int x, int y);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#endif

//using namespace std::literals;

namespace {
constexpr std::array<char, 84> int_to_b83{
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~"};

std::string
leftPad(std::string str, size_t len)
{
		if (str.size() >= len)
				return str;
		return str.insert(0, len - str.size(), '0');
}

//constexpr std::array<int, 255> b83_to_int = []() constexpr
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

std::string
encode83(int value)
{
		std::string buffer;

		do {
				buffer += int_to_b83[value % 83];
		} while ((value = value / 83));

		std::reverse(buffer.begin(), buffer.end());
		return buffer;
}

struct Components
{
		int x, y;
};

int
packComponents(const Components &c)
{
		return (c.x - 1) + (c.y - 1) * 9;
}

Components
unpackComponents(int c)
{
		return {c % 9 + 1, c / 9 + 1};
}

//int
//decode83(std::string value)
//{
//		int temp = 0;

//		for (char c : value)
//				if (b83_to_int[static_cast<unsigned char>(c)] < 0)
//						throw std::invalid_argument("invalid character in blurhash");

//		for (char c : value)
//				temp = temp * 83 + b83_to_int[static_cast<unsigned char>(c)];
//		return temp;
//}

//float
//decodeMaxAC(int quantizedMaxAC)
//{
//		return (quantizedMaxAC + 1) / 166.;
//}

//float
//decodeMaxAC(const std::string &maxAC)
//{
//		assert(maxAC.size() == 1);
//		return decodeMaxAC(decode83(maxAC));
//}

int
encodeMaxAC(float maxAC)
{
		return std::max(0, std::min(82, int(maxAC * 166 - 0.5)));
}

float
srgbToLinear(int value)
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

		return srgbToLinearF(value / 255.f);
}

int
linearToSrgb(float value)
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

		return int(linearToSrgbF(value) * 255.f + 0.5);
}

struct Color
{
		float r, g, b;

		Color &operator*=(float scale)
		{
				r *= scale;
				g *= scale;
				b *= scale;
				return *this;
		}
		friend Color operator*(Color lhs, float rhs) { return (lhs *= rhs); }
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

//Color
//decodeDC(int value)
//{
//		const int intR = value >> 16;
//		const int intG = (value >> 8) & 255;
//		const int intB = value & 255;
//		return {srgbToLinear(intR), srgbToLinear(intG), srgbToLinear(intB)};
//}

//Color
//decodeDC(std::string value)
//{
//		assert(value.size() == 4);
//		return decodeDC(decode83(value));
//}

int
encodeDC(const Color &c)
{
		return (linearToSrgb(c.r) << 16) + (linearToSrgb(c.g) << 8) + linearToSrgb(c.b);
}

float
signPow(float value, float exp)
{
		return std::copysign(std::pow(std::abs(value), exp), value);
}

int
encodeAC(const Color &c, float maximumValue)
{
		auto quantR =
		  int(std::max(0., std::min(18., std::floor(signPow(c.r / maximumValue, 0.5) * 9 + 9.5))));
		auto quantG =
		  int(std::max(0., std::min(18., std::floor(signPow(c.g / maximumValue, 0.5) * 9 + 9.5))));
		auto quantB =
		  int(std::max(0., std::min(18., std::floor(signPow(c.b / maximumValue, 0.5) * 9 + 9.5))));

		return quantR * 19 * 19 + quantG * 19 + quantB;
}

Color
decodeAC(int value, float maximumValue)
{
		auto quantR = value / (19 * 19);
		auto quantG = (value / 19) % 19;
		auto quantB = value % 19;

		return {signPow((float(quantR) - 9) / 9, 2) * maximumValue,
				signPow((float(quantG) - 9) / 9, 2) * maximumValue,
				signPow((float(quantB) - 9) / 9, 2) * maximumValue};
}

//Color
//decodeAC(std::string value, float maximumValue)
//{
//		return decodeAC(decode83(value), maximumValue);
//}

Color
multiplyBasisFunction(Components components, int width, int height, unsigned char *pixels)
{
		Color c{};
		float normalisation = (components.x == 0 && components.y == 0) ? 1 : 2;

		for (int y = 0; y < height; y++) {
				for (int x = 0; x < width; x++) {
						float basis = std::cos(M_PI * components.x * x / float(width)) *
									  std::cos(M_PI * components.y * y / float(height));
						c.r += basis * srgbToLinear(pixels[3 * x + 0 + y * width * 3]);
						c.g += basis * srgbToLinear(pixels[3 * x + 1 + y * width * 3]);
						c.b += basis * srgbToLinear(pixels[3 * x + 2 + y * width * 3]);
				}
		}

		float scale = normalisation / (width * height);
		c *= scale;
		return c;
}
}

namespace blurhash {
//Image
//decode(std::string blurhash, size_t width, size_t height, size_t bytesPerPixel)
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

std::string
encode(unsigned char *image, size_t width, size_t height, int components_x, int components_y)
{
		if (width < 1 || height < 1 || components_x < 1 || components_x > 9 || components_y < 1 ||
			components_y > 9 || !image)
				return "";

		std::vector<Color> factors;
		factors.reserve(components_x * components_y);
		for (int y = 0; y < components_y; y++) {
				for (int x = 0; x < components_x; x++) {
						factors.push_back(multiplyBasisFunction({x, y}, width, height, image));
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
				maximumValue              = ((float)quantisedMaximumValue + 1) / 166;
				h += leftPad(encode83(quantisedMaximumValue), 1);
		} else {
				maximumValue = 1;
				h += leftPad(encode83(0), 1);
		}

		h += leftPad(encode83(encodeDC(dc)), 4);

		for (auto ac : factors)
				h += leftPad(encode83(encodeAC(ac, maximumValue)), 2);

		return h;
}
} // https://github.com/Nheko-Reborn/blurhash
#endif

void IMG_calcul_empreinte_floue(const char *chemin, int composant_x, int composant_y, char *resultat, long *taille_resultat)
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
		OIIO::ImageInput::destroy(input);
		return;
	}

	auto res = blurhash::encode(donnees.data(), static_cast<size_t>(xres), static_cast<size_t>(yres), composant_x, composant_y);

	for (auto c : res) {
		*resultat++ = c;
	}

	*taille_resultat = static_cast<long>(res.size());

	input->close();
	OIIO::ImageInput::destroy(input);
}

}
