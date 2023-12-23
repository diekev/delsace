/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include <functional>
#include <inttypes.h>

struct Atome;
struct AtomeFonction;
struct Enchaineuse;
struct Instruction;

namespace kuri {
struct chaine;
template <typename T, typename TypeIndex>
struct tableau;
}  // namespace kuri

enum class OptionsImpressionType : uint32_t;

[[nodiscard]] kuri::chaine imprime_information_atome(Atome const *atome);

[[nodiscard]] kuri::chaine imprime_atome(Atome const *atome, OptionsImpressionType options);

[[nodiscard]] kuri::chaine imprime_atome(Atome const *atome);

void imprime_fonction(AtomeFonction const *atome_fonc,
                      Enchaineuse &sortie,
                      OptionsImpressionType options,
                      bool surligne_inutilisees = false,
                      std::function<void(Instruction const &, Enchaineuse &)> rappel = nullptr);

[[nodiscard]] kuri::chaine imprime_fonction(
    AtomeFonction const *atome_fonc,
    OptionsImpressionType options,
    bool surligne_inutilisees = false,
    std::function<void(Instruction const &, Enchaineuse &)> rappel = nullptr);

[[nodiscard]] kuri::chaine imprime_fonction(
    AtomeFonction const *atome_fonc,
    bool surligne_inutilisees = false,
    std::function<void(Instruction const &, Enchaineuse &)> rappel = nullptr);

[[nodiscard]] kuri::chaine imprime_instruction(Instruction const *inst,
                                               OptionsImpressionType options);

[[nodiscard]] kuri::chaine imprime_instruction(Instruction const *inst);

void imprime_instructions(
    kuri::tableau<Instruction *, int> const &instructions,
    Enchaineuse &os,
    OptionsImpressionType options,
    bool surligne_inutilisees = false,
    std::function<void(Instruction const &, Enchaineuse &)> rappel = nullptr);

[[nodiscard]] kuri::chaine imprime_instructions(
    kuri::tableau<Instruction *, int> const &instructions,
    OptionsImpressionType options,
    bool surligne_inutilisees = false,
    std::function<void(Instruction const &, Enchaineuse &)> rappel = nullptr);

[[nodiscard]] kuri::chaine imprime_arbre_instruction(Instruction const *racine);

[[nodiscard]] kuri::chaine imprime_commentaire_instruction(Instruction const *inst);
