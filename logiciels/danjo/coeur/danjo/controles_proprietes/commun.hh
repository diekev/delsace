/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

class QPushButton;
class QWidget;
class QHBoxLayout;

namespace danjo {

QPushButton *crée_bouton_animation_controle(QWidget *parent);

void définis_état_bouton_animation(QPushButton *bouton, bool est_animé);

QPushButton *crée_bouton_échelle_valeur(QWidget *parent);

QHBoxLayout *crée_hbox_layout(QWidget *parent = nullptr);

}  // namespace danjo
