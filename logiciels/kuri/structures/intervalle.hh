/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

template <typename T>
struct Intervalle {
    T min{};
    T max{};

    template <typename T1>
    bool possède_inclusif(T1 valeur)
    {
        return valeur >= min && valeur <= max;
    }
};
