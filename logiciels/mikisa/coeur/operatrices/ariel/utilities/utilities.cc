// UtilityCore: A utility library. Part of the TAKUA Render project.
// Written by Yining Karl Li
// Version 0.5.13.39a
//  
// File: utilities.inl
// A collection/kitchen sink of generally useful functions

#include "utilities.h"

#include <iostream>
#include <sys/timeb.h>
#include <cstdio>
#include <cstring>
#include <fstream>

#include "biblinternes/outils/chaine.hh"
#include "biblinternes/rmsd/rmsd.h"
#include "biblinternes/structures/flux_chaine.hh"

namespace utilityCore {

float clamp(float f, float min, float max)
{
	if (f<min) {
		return min;
	}else if (f>max) {
		return max;
	}else {
		return f;
	}
}

dls::math::vec3f clampRGB(dls::math::vec3f color)
{
	if (color[0]<0) {
		color[0]=0;
	}else if (color[0]>255) {
		color[0]=255;
	}
	if (color[1]<0) {
		color[1]=0;
	}else if (color[1]>255) {
		color[1]=255;
	}
	if (color[2]<0) {
		color[2]=0;
	}else if (color[2]>255) {
		color[2]=255;
	}
	return color;
}

bool epsilonCheck(float a, float b)
{
	return std::abs(std::abs(a) - std::abs(b)) < EPSILON;
}

int getMilliseconds()
{
	timeb tb;
	ftime( &tb );
	auto nCount = tb.millitm + (tb.time & 0xfffff) * 1000;
	return static_cast<int>(nCount);
}

int compareMilliseconds(int referenceTime)
{
	int elapsedMilliseconds = getMilliseconds() - referenceTime;
	if ( elapsedMilliseconds < 0 ) {
		elapsedMilliseconds += 0x100000 * 1000;
	}
	return elapsedMilliseconds;
}

dls::math::vec3f calculateKabschRotation(dls::math::vec3f mov0, dls::math::vec3f mov1, dls::math::vec3f mov2, dls::math::vec3f ref0, dls::math::vec3f ref1, dls::math::vec3f ref2)
{
	double rotatedList[3][3];
	rotatedList[0][0] = static_cast<double>(mov0.x);
	rotatedList[0][1] = static_cast<double>(mov0.y);
	rotatedList[0][2] = static_cast<double>(mov0.z);

	rotatedList[1][0] = static_cast<double>(mov1.x);
	rotatedList[1][1] = static_cast<double>(mov1.y);
	rotatedList[1][2] = static_cast<double>(mov1.z);

	rotatedList[2][0] = static_cast<double>(mov2.x);
	rotatedList[2][1] = static_cast<double>(mov2.y);
	rotatedList[2][2] = static_cast<double>(mov2.z);

	double referenceList[3][3];
	referenceList[0][0] = static_cast<double>(ref0.x);
	referenceList[0][1] = static_cast<double>(ref0.y);
	referenceList[0][2] = static_cast<double>(ref0.z);

	referenceList[1][0] = static_cast<double>(ref1.x);
	referenceList[1][1] = static_cast<double>(ref1.y);
	referenceList[1][2] = static_cast<double>(ref1.z);

	referenceList[2][0] = static_cast<double>(ref2.x);
	referenceList[2][1] = static_cast<double>(ref2.y);
	referenceList[2][2] = static_cast<double>(ref2.z);

	int listCount = 3;
	dls::math::vec3f movcom = (mov0+mov1+mov2)/3.0f;

	double mov_com[3];
	mov_com[0] = static_cast<double>(movcom.x);
	mov_com[1] = static_cast<double>(movcom.y);
	mov_com[2] = static_cast<double>(movcom.z);

	dls::math::vec3f refcom = (ref0+ref1+ref2)/3.0f;
	dls::math::vec3f movtoref = refcom - movcom;

	double mov_to_ref[3];
	mov_to_ref[0] = static_cast<double>(movtoref[0]);
	mov_to_ref[1] = static_cast<double>(movtoref[1]);
	mov_to_ref[2] = static_cast<double>(movtoref[2]);

	double U[3][3];
	double rmsd;

	calculate_rotation_rmsd(referenceList, rotatedList, listCount, mov_com, mov_to_ref, U, &rmsd);

	auto x = std::atan2( U[1][2], U[2][2]  ) * constantes<double>::POIDS_RAD_DEG;
	auto y = std::atan2(-U[0][2], std::sqrt(U[1][2]*U[1][2] + U[2][2]*U[2][2])) * constantes<double>::POIDS_RAD_DEG;
	auto z = std::atan2( U[0][1], U[0][0]) * constantes<double>::POIDS_RAD_DEG;

	return dls::math::vec3f(
				static_cast<float>(x) - 180.0f,
				180.0f - static_cast<float>(y),
				180.0f - static_cast<float>(z));
}

dls::math::mat4x4f buildTransformationMatrix(dls::math::vec3f translation, dls::math::vec3f rotation, dls::math::vec3f scale)
{
	dls::math::mat4x4f translationMat = buildTranslation(translation);
	dls::math::mat4x4f rotationMat = buildRotation(toRadian(rotation.z), dls::math::vec3f(0.0f, 0.0f, 1.0f));
	rotationMat = rotationMat*buildRotation(toRadian(rotation.y), dls::math::vec3f(0.0f, 1.0f, 0.0f));
	rotationMat = rotationMat*buildRotation(toRadian(rotation.x), dls::math::vec3f(1.0f, 0.0f, 0.0f));
	dls::math::mat4x4f scaleMat = buildScale(scale);
	return translationMat*rotationMat*scaleMat;
}

dls::math::mat4x4f buildInverseTransformationMatrix(dls::math::vec3f translation, dls::math::vec3f rotation, dls::math::vec3f scale)
{
	dls::math::mat4x4f translationMat = buildTranslation(-translation);
	dls::math::mat4x4f rotationMat = buildRotation(toRadian(-rotation.x), dls::math::vec3f(1.0f, 0.0f, 0.0f));
	rotationMat = rotationMat*buildRotation(toRadian(-rotation.y), dls::math::vec3f(0.0f, 1.0f, 0.0f));
	rotationMat = rotationMat*buildRotation(toRadian(-rotation.z), dls::math::vec3f(0.0f, 0.0f, 1.0f));
	dls::math::mat4x4f scaleMat = buildScale(dls::math::vec3f(1.0f)/scale);
	return scaleMat*rotationMat*translationMat;
}

dls::math::vec4f multiply(dls::math::mat4x4f m, dls::math::vec4f v) {
	dls::math::vec4f r;
	r.x = m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w;
	r.y = m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w;
	r.z = m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w;
	r.w = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w;
	return r;
}

dls::math::mat4x4f buildTranslation(dls::math::vec3f translation)
{
	dls::math::mat4x4f m = dls::math::mat4x4f();
	m[3][0] = translation[0];
	m[3][1] = translation[1];
	m[3][2] = translation[2];
	return m;
}

dls::math::mat4x4f buildRotation(float radian, dls::math::vec3f axis)
{
	axis = axis/ longueur(axis);
	float a = radian;
	float c = std::cos(a);
	float s = std::sin(a);
	dls::math::mat4x4f m = dls::math::mat4x4f();
	m[0][0] = c + (1.0f - c)      * axis.x     * axis.x;
	m[0][1] = (1.0f - c) * axis.x * axis.y + s * axis.z;
	m[0][2] = (1.0f - c) * axis.x * axis.z - s * axis.y;
	m[0][3] = 0.0f;
	m[1][0] = (1.0f - c) * axis.y * axis.x - s * axis.z;
	m[1][1] = c + (1.0f - c) * axis.y * axis.y;
	m[1][2] = (1.0f - c) * axis.y * axis.z + s * axis.x;
	m[1][3] = 0.0f;
	m[2][0] = (1.0f - c) * axis.z * axis.x + s * axis.y;
	m[2][1] = (1.0f - c) * axis.z * axis.y - s * axis.x;
	m[2][2] = c + (1.0f - c) * axis.z * axis.z;
	m[2][3] = 0.0f;
	return m;
}

dls::math::mat4x4f buildScale(dls::math::vec3f scale)
{
	dls::math::mat4x4f m = dls::math::mat4x4f();
	m[0][0] = scale[0];
	m[1][1] = scale[1];
	m[2][2] = scale[2];
	return m;
}

void fovToPerspective(float fovy, float aspect, float zNear, dls::math::vec2f &xBounds, dls::math::vec2f &yBounds)
{
	yBounds[1] = zNear * std::tan(fovy*constantes<float>::PI/360.0f);
	yBounds[0] = -yBounds[1];
	xBounds[0] = yBounds[0]*aspect;
	xBounds[1] = yBounds[1]*aspect;
}

}
