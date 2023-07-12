/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

class ContexteRendu;
class TamponRendu;
class Kanba;

/* ------------------------------------------------------------------------- */
/** \name RenduSeaux
 * \{ */

class RenduSeaux {
    TamponRendu *m_tampon = nullptr;
    Kanba *m_kanba = nullptr;

    int m_id_cannevas = -1;

  public:
    explicit RenduSeaux(Kanba *kanba);

    RenduSeaux(RenduSeaux const &) = delete;
    RenduSeaux &operator=(RenduSeaux const &) = delete;

    ~RenduSeaux();

    void initialise();

    void dessine(ContexteRendu const &contexte);
};

/** \} */
