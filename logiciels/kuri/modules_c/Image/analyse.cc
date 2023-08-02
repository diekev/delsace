/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "analyse.hh"

namespace image {

enum TypeAnalyse {
    ANALYSE_GRADIENT = 0,
    ANALYSE_DIVERGENCE = 1,
    ANALYSE_LAPLACIEN = 2,
    ANALYSE_COURBE = 3,
};

enum DirectionAnalyse {
    DIRECTION_X = 0,
    DIRECTION_Y = 1,
    DIRECTION_XY = 2,
};

enum Differentiel {
    DIFF_AVANT = 0,
    DIFF_ARRIERE = 1,
};

struct AnalyseGradient {
    static float direction_x(float a, float b)
    {
        return b - a;
    }

    static float direction_y(float a, float b)
    {
        return b - a;
    }

    static float direction_xy(float xa, float xb, float ya, float yb)
    {
        return (direction_x(xa, xb) + direction_y(ya, yb)) * 0.5f;
    }
};

struct AnalyseLaplacien {
    static float direction_x(float a, float b, float c)
    {
        return b + a - c * 2.0f;
    }

    static float direction_y(float a, float b, float c)
    {
        return b + a - c * 2.0f;
    }

    static float direction_xy(float xa, float xb, float ya, float yb, float c)
    {
        return (direction_x(xa, xb, c) + direction_y(xa, xb, c)) * 0.5f;
    }
};

}  // namespace image
