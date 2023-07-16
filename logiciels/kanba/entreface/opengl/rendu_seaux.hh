/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

#include "biblinternes/opengl/tampon_rendu.h"
#include "biblinternes/outils/definitions.h"

namespace KNB {
class Kanba;
}

class ContexteRendu;
class TamponRendu;

/* ------------------------------------------------------------------------- */
/** \name RenduSeaux
 * \{ */

class RenduSeaux {
    std::unique_ptr<TamponRendu> m_tampon = nullptr;
    KNB::Kanba *m_kanba = nullptr;

    int m_id_cannevas = -1;

  public:
    explicit RenduSeaux(KNB::Kanba *kanba);

    EMPECHE_COPIE(RenduSeaux);

    void initialise();

    void dessine(ContexteRendu const &contexte);
};

/** \} */
