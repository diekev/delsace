/* Freetype GL - A C OpenGL Freetype engine
 *
 * Distributed under the OSI-approved BSD 2-Clause License.  See accompanying
 * file `LICENSE` for more details.
 */
#include "edtaa3func.h"
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ftgl.h"

double *make_distance_mapd(double *data, uint32_t width, uint32_t height)
{
    short *xdist = (short *)FTGL_malloc(width * height * sizeof(short));
    short *ydist = (short *)FTGL_malloc(width * height * sizeof(short));
    double *gx = (double *)FTGL_calloc(width * height, sizeof(double));
    double *gy = (double *)FTGL_calloc(width * height, sizeof(double));
    double *outside = (double *)FTGL_calloc(width * height, sizeof(double));
    double *inside = (double *)FTGL_calloc(width * height, sizeof(double));
    double vmin = DBL_MAX;
    uint32_t i;

    // Compute outside = edtaa3(bitmap); % Transform background (0's)
    computegradient(data, width, height, gx, gy);
    edtaa3(data, gx, gy, width, height, xdist, ydist, outside);
    for (i = 0; i < width * height; ++i)
        if (outside[i] < 0.0)
            outside[i] = 0.0;

    // Compute inside = edtaa3(1-bitmap); % Transform foreground (1's)
    memset(gx, 0, sizeof(double) * width * height);
    memset(gy, 0, sizeof(double) * width * height);
    for (i = 0; i < width * height; ++i)
        data[i] = 1 - data[i];
    computegradient(data, width, height, gx, gy);
    edtaa3(data, gx, gy, width, height, xdist, ydist, inside);
    for (i = 0; i < width * height; ++i)
        if (inside[i] < 0)
            inside[i] = 0.0;

    // distmap = outside - inside; % Bipolar distance field
    for (i = 0; i < width * height; ++i) {
        outside[i] -= inside[i];
        if (outside[i] < vmin)
            vmin = outside[i];
    }

    vmin = fabs(vmin);

    for (i = 0; i < width * height; ++i) {
        double v = outside[i];
        if (v < -vmin)
            outside[i] = -vmin;
        else if (v > +vmin)
            outside[i] = +vmin;
        data[i] = (outside[i] + vmin) / (2 * vmin);
    }

    FTGL_free(xdist, width * height * sizeof(short));
    FTGL_free(ydist, width * height * sizeof(short));
    FTGL_free(gx, width * height * sizeof(double));
    FTGL_free(gy, width * height * sizeof(double));
    FTGL_free(outside, width * height * sizeof(double));
    FTGL_free(inside, width * height * sizeof(double));
    return data;
}

unsigned char *make_distance_mapb(unsigned char *img, unsigned char *out, double *data, uint32_t width, uint32_t height)
{
    uint32_t i;

    // find minimimum and maximum values
    double img_min = DBL_MAX;
    double img_max = DBL_MIN;

    for (i = 0; i < width * height; ++i) {
        double v = img[i];
        data[i] = v;
        if (v > img_max)
            img_max = v;
        if (v < img_min)
            img_min = v;
    }

    // Map values from 0 - 255 to 0.0 - 1.0
    for (i = 0; i < width * height; ++i)
        data[i] = (img[i] - img_min) / img_max;

    data = make_distance_mapd(data, width, height);

    // map values from 0.0 - 1.0 to 0 - 255
    for (i = 0; i < width * height; ++i)
        out[i] = (unsigned char)(255 * (1 - data[i]));

    return out;
}
