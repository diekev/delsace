/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2016 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "base_editrice.h"

class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QSlider;
class QPushButton;
class QSpinBox;
class QVBoxLayout;

namespace danjo {

class RepondantBouton;

} /* namespace danjo */

class EditriceLigneTemps : public BaseEditrice {
    Q_OBJECT

    QSlider *m_slider;

    QHBoxLayout *m_tc_layout;
    QHBoxLayout *m_num_layout;
    QVBoxLayout *m_vbox_layout;
    QSpinBox *m_end_frame, *m_start_frame, *m_cur_frame;
    QDoubleSpinBox *m_fps;

  public:
    explicit EditriceLigneTemps(JJL::Jorjala &jorjala, QWidget *parent = nullptr);

    EditriceLigneTemps(EditriceLigneTemps const &) = default;
    EditriceLigneTemps &operator=(EditriceLigneTemps const &) = default;

    void ajourne_état(JJL::TypeEvenement évènement) override;

    void ajourne_manipulable() override
    {
    }

  private Q_SLOTS:
    void setStartFrame(int value);
    void setCurrentFrame(int value);
    void setEndFrame(int value);
    void setFPS(double value);
};
