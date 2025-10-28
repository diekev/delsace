/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include <elf.h>

#include "structures/chaine.hh"
#include "structures/tableau.hh"

struct DonnéesConstantes;

struct SectionELF {
    kuri::chaine nom{};
    uint32_t type{};

    Elf64_Shdr *entête = nullptr;
};

class FichierELF {
    kuri::tableau<SectionELF *> m_sections{};
    DonnéesConstantes const *m_données_constantes = nullptr;

  public:
    static FichierELF *crée_fichier_objet();

    SectionELF *crée_section(kuri::chaine_statique nom, uint32_t type);
    SectionELF *donne_section(kuri::chaine_statique nom) const;
    uint16_t donne_indice_section(kuri::chaine_statique nom) const;

    void écris_données_constantes(DonnéesConstantes const *données);

    void écris_vers(kuri::chaine_statique chemin) const;
};
