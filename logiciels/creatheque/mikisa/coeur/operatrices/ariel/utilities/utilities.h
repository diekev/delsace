// UtilityCore: A utility library. Part of the TAKUA Render project.
// Written by Yining Karl Li
// Version 0.5.13.39a
//  
// File: utilities.h
// A collection/kitchen sink of generally useful functions

#ifndef UTILITIES_H
#define UTILITIES_H

#include <delsace/math/matrice.hh>
#include <delsace/math/vecteur.hh>
#include <algorithm>
#include <istream>
#include <ostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "bibliotheques/outils/constantes.h"

//====================================
// Useful Math Constants
//====================================
//#define PI                        3.1415926535897932384626422832795028841971
#define TWO_PI                    6.2831853071795864769252867665590057683943
// #define SQRT_OF_ONE_THIRD         0.5773502691896257645091487805019574556476
// #define E                         2.7182818284590452353602874713526624977572
#define EPSILON                   0.000000001f
// #define ZERO_ABSORPTION_EPSILON   0.00001
// #define RAY_BIAS_AMOUNT           0.0002
#define REALLY_BIG_NUMBER         1000000000000000000.0f

namespace utilityCore {

//====================================
// Function Declarations
//====================================

//Math stuff
template <typename T>
auto toRadian(T degree)
{
	return degree * constantes<T>::POIDS_DEG_RAD;
}

template <typename T>
auto toDegree(float radian)
{
	return radian * constantes<T>::POIDS_RAD_DEG;
}

float clamp(float f, float min, float max);

dls::math::vec3f clampRGB(dls::math::vec3f color);

bool epsilonCheck(float a, float b);

//====================================
// String wrangling stuff
//====================================

std::string padString(size_t length, std::string str);

bool replaceString(std::string& str, const std::string& from, const std::string& to);

std::string convertIntToString(int number);

std::vector<std::string> tokenizeString(std::string str, std::string separator);

std::vector<std::string> tokenizeStringByAllWhitespace(std::string str);

std::string getLastNCharactersOfString(std::string s, size_t n);

std::string getFirstNCharactersOfString(std::string s, size_t n);

//====================================
// Time stuff
//====================================

int getMilliseconds();

int compareMilliseconds(int referenceTime);

//====================================
// Matrix stuff
//====================================

dls::math::vec3f calculateKabschRotation(dls::math::vec3f mov0, dls::math::vec3f mov1, dls::math::vec3f mov2,
											   dls::math::vec3f ref0, dls::math::vec3f ref1, dls::math::vec3f ref2);

dls::math::mat4x4f buildTransformationMatrix(dls::math::vec3f translation, dls::math::vec3f rotation,
												 dls::math::vec3f scale);

dls::math::mat4x4f buildInverseTransformationMatrix(dls::math::vec3f translation, dls::math::vec3f rotation,
														dls::math::vec3f scale);

dls::math::vec4f multiply(dls::math::mat4x4f m, dls::math::vec4f v);

//this duplicates GLM functionality, but is necessary to work on CUDA
dls::math::mat4x4f buildTranslation(dls::math::vec3f translation);

dls::math::mat4x4f buildRotation(float radian, dls::math::vec3f axis);

dls::math::mat4x4f buildScale(dls::math::vec3f scale);

//====================================
// GLM Printers
//====================================

void printMat4(dls::math::mat4x4f m);

void printVec4(dls::math::vec4f m);

void printVec3(dls::math::vec3f m);

//====================================
// GL Stuff
//====================================

void fovToPerspective(float fovy, float aspect, float zNear, dls::math::vec2f& xBounds,
								   dls::math::vec2f& yBounds);

//====================================
// IO Stuff
//====================================

std::string readFileAsString(std::string filename);

std::string getRelativePath(std::string path);

}

#endif
