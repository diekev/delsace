/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

// #pragma once

/* MSVC définis pour ses extensions "interface" comme "struct". */

#ifdef _MSC_VER
#    undef interface
#    undef OPAQUE
#    undef DIFFERENCE
#endif
