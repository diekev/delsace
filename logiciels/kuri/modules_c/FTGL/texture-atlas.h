/* Freetype GL - A C OpenGL Freetype engine
 *
 * Distributed under the OSI-approved BSD 2-Clause License.  See accompanying
 * file `LICENSE` for more details.
 *  ============================================================================
 *
 *
 * This source is based on the article by Jukka Jylänki :
 * "A Thousand Ways to Pack the Bin - A Practical Approach to
 * Two-Dimensional Rectangle Bin Packing", February 27, 2010.
 *
 * More precisely, this is an implementation of the Skyline Bottom-Left
 * algorithm based on C++ sources provided by Jukka Jylänki at:
 * http://clb.demon.fi/files/RectangleBinPack/
 *
 *  ============================================================================
 */
#ifndef __TEXTURE_ATLAS_H__
#define __TEXTURE_ATLAS_H__

#include <stdint.h>
#include <stdlib.h>

#include "vec234.h"
#include "vector.h"

/**
 * @file   texture-atlas.h
 * @author Nicolas Rougier (Nicolas.Rougier@inria.fr)
 *
 * @defgroup texture-atlas Texture atlas
 *
 * A texture atlas is used to pack several small regions into a single texture.
 *
 * The actual implementation is based on the article by Jukka Jylänki : "A
 * Thousand Ways to Pack the Bin - A Practical Approach to Two-Dimensional
 * Rectangle Bin Packing", February 27, 2010.
 * More precisely, this is an implementation of the Skyline Bottom-Left
 * algorithm based on C++ sources provided by Jukka Jylänki at:
 * http://clb.demon.fi/files/RectangleBinPack/
 *
 *
 * Example Usage:
 * @code
 * #include "texture-atlas.h"
 *
 * ...
 *
 * / Creates a new atlas of 512x512 with a depth of 1
 * texture_atlas_t * atlas = texture_atlas_new( 512, 512, 1 );
 *
 * // Allocates a region of 20x20
 * ivec4 region = texture_atlas_get_region( atlas, 20, 20 );
 *
 * // Fill region with some data
 * texture_atlas_set_region( atlas, region.x, region.y, region.width, region.height, data, stride )
 *
 * ...
 *
 * @endcode
 *
 * @{
 */

typedef struct byte_scratch_buffer_t {
    unsigned char *data;
    size_t size;
} byte_scratch_buffer_t;

void byte_scratch_buffer_delete(byte_scratch_buffer_t *buffer);

typedef struct double_scratch_buffer_t {
    double *data;
    size_t size;
} double_scratch_buffer_t;

void double_scratch_buffer_delete(double_scratch_buffer_t *buffer);

/**
 * A texture atlas is used to pack several small regions into a single texture.
 */
typedef struct texture_atlas_t {
    /**
     * Allocated nodes
     */
    vector_t *nodes;

    /**
     *  Width (in pixels) of the underlying texture
     */
    size_t width;

    /**
     * Height (in pixels) of the underlying texture
     */
    size_t height;

    /**
     * Depth (in bytes) of the underlying texture
     */
    size_t depth;

    /**
     * Allocated surface size
     */
    size_t used;

    /**
     * Texture identity (OpenGL)
     */
    uint32_t id;

    /**
     * Set to 1 if new glyphs were loaded. Can be set to 0 by
     * clients to dynamically update the OpenGL texture.
     */
    uint32_t dirty;

    /**
     * Atlas data
     */
    unsigned char *data;

    /**
     * Scratch buffers
     */
    byte_scratch_buffer_t glyph_scratch_buffer;
    byte_scratch_buffer_t distance_byte_scratch_buffer;
    double_scratch_buffer_t distance_double_scratch_buffer;
} texture_atlas_t;

/**
 * Creates a new empty texture atlas.
 *
 * @param   width   width of the atlas
 * @param   height  height of the atlas
 * @param   depth   bit depth of the atlas
 * @return          a new empty texture atlas.
 *
 */
texture_atlas_t *texture_atlas_new(const size_t width, const size_t height, const size_t depth);

/**
 *  Deletes a texture atlas.
 *
 *  @param self a texture atlas structure
 *
 */
void texture_atlas_delete(texture_atlas_t *self);

/**
 *  Allocate a new region in the atlas.
 *
 *  @param self   a texture atlas structure
 *  @param width  width of the region to allocate
 *  @param height height of the region to allocate
 *  @return       Coordinates of the allocated region
 *
 */
ivec4 texture_atlas_get_region(texture_atlas_t *self, const size_t width, const size_t height);

/**
 *  Upload data to the specified atlas region.
 *
 *  @param self   a texture atlas structure
 *  @param x      x coordinate the region
 *  @param y      y coordinate the region
 *  @param width  width of the region
 *  @param height height of the region
 *  @param data   data to be uploaded into the specified region
 *  @param stride stride of the data
 *
 */
void texture_atlas_set_region(texture_atlas_t *self,
                              const size_t x,
                              const size_t y,
                              const size_t width,
                              const size_t height,
                              const unsigned char *data,
                              const size_t stride);

unsigned char *texture_atlas_get_glyph_buffer(texture_atlas_t *self, size_t width, size_t height);
unsigned char *texture_atlas_get_distance_byte_buffer(texture_atlas_t *self, size_t width, size_t height);
double *texture_atlas_get_distance_double_buffer(texture_atlas_t *self, size_t width, size_t height);

/**
 *  Remove all allocated regions from the atlas.
 *
 *  @param self   a texture atlas structure
 */
void texture_atlas_clear(texture_atlas_t *self);

/** @} */

#endif /* __TEXTURE_ATLAS_H__ */
