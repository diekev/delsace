#include <functional>
#include <glm/glm.hpp>
#include <random>

#include "camera.h"

std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dis(0.0f, 1.0f);

float random1f()
{
	return dis(gen);
}

glm::vec2 random2f()
{
	return glm::vec2(dis(gen), dis(gen));
}

constexpr auto M_TAU = 6.28318530717958647692528676655900576839433879875021f;
std::function<float(glm::vec3)> DistanceEstimator;

struct Intersection {
	float t, u, v;
	int object;
	int prim;
	int type;
};

/**
 * To be defined
 */
glm::vec3 sunDirection;
glm::vec3 sunColor = glm::vec3(1.0f), skyColor = glm::vec3(0.0f, 0.0f, 1.0f);

glm::vec3 diskPoint(glm::vec3 nor);
void worldMoveObjects(float time);
glm::vec3 worlMoveCamera(float time);
glm::vec3 worldGetBackground(glm::vec3 rd);
glm::vec3 worldGetNormal(glm::vec3 pos, float objectID);
glm::vec3 worldGetColor(glm::vec3 pos, glm::vec3 nor, float objectID);

/**
 * Returns the closest intersection of a ray with origin ro and normalized
 * direction rd in the form of a distance and object ID
 *
 * ray march distance estimator
 */
glm::vec2 worldIntersect(Ray *ray, float maxlen)
{
	static_cast<void>(maxlen);
	int MaximumRaySteps = 10;
	float MinimumDistance = 0.01f;

	float totalDistance = 0.0f;
	int steps = 0;

	for (steps = 0; steps < MaximumRaySteps; ++steps) {
		glm::vec3 p = ray->orig + totalDistance * ray->dir;
		float distance = DistanceEstimator(p);

		if (distance < MinimumDistance) {
			break;
		}
	}

	return glm::vec2(0.0f, 1.0f - float(steps) / float(MaximumRaySteps));
}

/**
 * Returns 0.0f if there is any intersection, 1.0f otherwise
 */
float worldShadow(glm::vec3 ro, glm::vec3 rd);

glm::vec3 coneDirection(glm::vec3 dir, float angle);

glm::vec3 cosineDirection(glm::vec3 nor)
{
	float xi1 = random1f();
	float xi2 = random1f();

	float theta = glm::acos(glm::sqrt(1.0f - xi1));
	float phi = M_TAU * xi2;

	float xs = glm::sin(theta) * glm::cos(phi);
	float ys = glm::cos(theta);
	float zs = glm::sin(theta) * glm::sin(phi);

	glm::vec3 y = nor;
	glm::vec3 h = y;

	if (glm::abs(h.x) <= glm::abs(h.y) && glm::abs(h.x) <= glm::abs(h.z)) {
		h.x = 1.0f;
	}
	else if (glm::abs(h.y) <= glm::abs(h.x) && glm::abs(h.y) <= glm::abs(h.z)) {
		h.y = 1.0f;
	}
	else {
		h.z = 1.0f;
	}

	glm::vec3 x = glm::normalize(h ^ y);
	glm::vec3 z = glm::normalize(x ^ y);

	return glm::normalize(x * xs + y * ys + z * zs);
}

glm::vec3 worldApplyLighting(glm::vec3 pos, glm::vec3 nor)
{
	glm::vec3 dcol = glm::vec3(1.0f);

	// sample sun
	{
		glm::vec3 point = 1000.0f * sunDirection + 50.0f * diskPoint(nor);
		glm::vec3 liray = glm::normalize(point - pos);
		float ndl = glm::max(0.0f, glm::dot(liray, nor));
		dcol += ndl * sunColor * worldShadow(pos, liray);
	}

	// sample sky
	{
		glm::vec3 point = 1000.0f * cosineDirection(nor);
		glm::vec3 liray = glm::normalize(point - pos);
		dcol += skyColor * worldShadow(pos, liray);
	}

	return dcol;
}

glm::vec3 reflect(glm::vec3 dir, glm::vec3 nor)
{
	return dir - (2.0f * (dir * nor) * nor);
}

glm::vec3 worldGetBRDFRay(glm::vec3 pos, glm::vec3 nor, glm::vec3 eye, float materialID)
{
	static_cast<void>(pos);
	static_cast<void>(materialID);

	if (random1f() < 0.8f) {
		return cosineDirection(nor);
	}
	else {
		return coneDirection(reflect(eye, nor), 0.9f);
	}
}

glm::vec3 renderCalculateColor(glm::vec3 ro, glm::vec3 rd, int numLevels)
{
	glm::vec3 tcol = glm::vec3(0.0f);
	glm::vec3 fcol = glm::vec3(1.0f);
	Ray ray;
	ray.orig = ro;
	ray.dir = rd;

	for (int i = 0; i < numLevels; ++i) {
		// intersect scene
		glm::vec2 tres = worldIntersect(&ray, 1000.0f);

		// if nothing is found, return background color
		if (tres.y < 0.0f) {
			if (i == 0) {
				fcol += worldGetBackground(rd);
				continue;
			}
			else {
				break;
			}
		}

		// get position and normal at the intersection point
		glm::vec3 pos = ro + rd * tres.x;
		glm::vec3 nor = worldGetNormal(pos, tres.x);

		// get color for the surface
		glm::vec3 scol = worldGetColor(pos, nor, tres.y);

		// compute direct lighting
		glm::vec3 dcol = worldApplyLighting(pos, nor);

		// prepare ray for indirect lighting gathering
		ro = pos;
		rd = worldGetBRDFRay(pos, nor, rd, tres.y);

		// surface * lighting
		fcol *= scol;
		tcol += fcol * dcol;
	}

	return tcol;
}

glm::vec3 calcPixelColor(glm::ivec2 pixel, glm::ivec2 resolution)
{
	float shutterAperture = 0.6f;
	float fov = 2.5f;
	float focusDistance = 1.3f;
	float blurAmount = 0.0015f;
	int numLevels = 5;

	glm::vec3 col = glm::vec3(0.0f);
	Camera *camera = new Camera();

	// 256 paths per pixel
	for (int i = 0; i < 256; ++i) {
		// screen coords with antialising
		glm::vec2 p = glm::vec2(glm::vec2(-resolution) + 2.0f*(glm::vec2(pixel) - random2f())) / (float)resolution.y;

		// motion blur
		//float ctime = frameTime + shutterAperture * (1.0f/24.0f) * random1f();

		// move objects
		//worldMoveObjects(ctime);

		// get camera position
		glm::vec3 ro, uu, vv, ww;
		ro = uu = vv = ww = worlMoveCamera(ctime);

		// create ray with depth of field
		glm::vec3 er = glm::normalize(glm::vec3(p.x, p.y, fov));
		glm::vec3 rd = er.x * uu + er.y * vv + er.z * ww;

		glm::vec3 go = blurAmount * glm::vec3(-1.0f + 2.0f * random2f(), 0.0f);
		glm::vec3 gd = glm::normalize(er * focusDistance - go);
		ro += go.x * uu + go.y * vv;
		rd += gd.x * uu + gd.y * vv;

		// accumulate path
		col += renderCalculateColor(ro, glm::normalize(rd), numLevels);
	}

	col = col / 256.0f;

	// apply gamma correction
	col = glm::pow(col, glm::vec3(0.45f));

	return col;
}

void renderImage(glm::vec3 *image, glm::ivec2 resolution)
{
#define TILE_SIZE 16

	// prep data
	const int num_tiles_x = resolution.x / TILE_SIZE;
	const int num_tiles_y = resolution.y / TILE_SIZE;
	const int num_tiles = num_tiles_x * num_tiles_y;

	// render tiles
	for (int tile = 0; tile < num_tiles; ++tile) {
		// tile offset
		const int ia = TILE_SIZE * (tile % num_tiles_x);
		const int ja = TILE_SIZE * (tile / num_tiles_x);
		
		// for every pixel in this tile, compute color
		for (int y = 0; y < TILE_SIZE; ++y) {
			for (int x = 0; x < TILE_SIZE; ++x) {
				image[resolution.x * (ja + y) + (ia + x)] = calcPixelColor(glm::ivec2(ia + x, ja + y), resolution);
			}
		}
	}
}

