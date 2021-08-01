/* Freetype GL - A C OpenGL Freetype engine
 *
 * Distributed under the OSI-approved BSD 2-Clause License.  See accompanying
 * file `LICENSE` for more details.
 */
#ifndef __VEC234_H__
#define __VEC234_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

/**
 * Tuple of 4 ints.
 *
 * Each field can be addressed using several aliases:
 *  - First component:  <b>x</b>, <b>r</b>, <b>red</b> or <b>vstart</b>
 *  - Second component: <b>y</b>, <b>g</b>, <b>green</b> or <b>vcount</b>
 *  - Third component:  <b>z</b>, <b>b</b>, <b>blue</b>, <b>width</b> or <b>istart</b>
 *  - Fourth component: <b>w</b>, <b>a</b>, <b>alpha</b>, <b>height</b> or <b>icount</b>
 *
 */
typedef struct {
    int x;
    int y;
    int z;
    int w;
} ivec4;

/**
 * Tuple of 3 ints.
 *
 * Each field can be addressed using several aliases:
 *  - First component:  <b>x</b>, <b>r</b> or <b>red</b>
 *  - Second component: <b>y</b>, <b>g</b> or <b>green</b>
 *  - Third component:  <b>z</b>, <b>b</b> or <b>blue</b>
 *
 */
typedef struct {
    int x;
    int y;
    int z;
} ivec3;

/**
 * Tuple of 2 ints.
 *
 * Each field can be addressed using several aliases:
 *  - First component: <b>x</b>, <b>s</b> or <b>start</b>
 *  - Second component: <b>y</b>, <b>t</b> or <b>end</b>
 *
 */
typedef struct {
    int x;
    int y;
} ivec2;

/**
 * Tuple of 4 floats.
 *
 * Each field can be addressed using several aliases:
 *  - First component:  <b>x</b>, <b>left</b>, <b>r</b> or <b>red</b>
 *  - Second component: <b>y</b>, <b>top</b>, <b>g</b> or <b>green</b>
 *  - Third component:  <b>z</b>, <b>width</b>, <b>b</b> or <b>blue</b>
 *  - Fourth component: <b>w</b>, <b>height</b>, <b>a</b> or <b>alpha</b>
 */
typedef struct vec4 {
    float x;
    float y;
    float z;
    float w;
} vec4;

/**
 * Tuple of 3 floats
 *
 * Each field can be addressed using several aliases:
 *  - First component:  <b>x</b>, <b>r</b> or <b>red</b>
 *  - Second component: <b>y</b>, <b>g</b> or <b>green</b>
 *  - Third component:  <b>z</b>, <b>b</b> or <b>blue</b>
 */
typedef struct {
    float x;
    float y;
    float z;
} vec3;

/**
 * Tuple of 2 floats
 *
 * Each field can be addressed using several aliases:
 *  - First component:  <b>x</b> or <b>s</b>
 *  - Second component: <b>y</b> or <b>t</b>
 */
typedef struct vec2 {
    float x;
    float y;
} vec2;

#pragma GCC diagnostic pop

#endif /* __VEC234_H__ */
