# -*- coding: utf-8 -*-

# ##### BEGIN GPL LICENSE BLOCK #####
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software  Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2016 Kévin Dietrich.
# All rights reserved.
#
# ##### END GPL LICENSE BLOCK #####

from PyQt4 import QtCore
from PyQt4.phonon import Phonon


class Sound(QtCore.QObject):
    def __init__(self, sound_file, parent=None):
        super(Sound, self).__init__(parent)

        self.sound_file = sound_file
        self.media_object = Phonon.MediaObject(self)
        self._audio_output = Phonon.AudioOutput(Phonon.MusicCategory, self)
        self._path = Phonon.createPath(self.media_object, self._audio_output)
        self.media_source = Phonon.MediaSource(sound_file)
        self.media_object.setCurrentSource(self.media_source)

    def play(self):
        self.media_object.stop()
        self.media_object.seek(0)
        self.media_object.play()
