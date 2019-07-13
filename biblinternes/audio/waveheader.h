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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/chaine.hh"
#include "biblinternes/structures/tableau.hh"

namespace dls {
namespace audio {

class WaveHeader {
	dls::chaine m_group_id;
	unsigned int m_file_length;
	dls::chaine m_riff_type;

public:
	WaveHeader();
	~WaveHeader() = default;

	void write(std::ostream &os);
};

class WaveFormatChunk {
	dls::chaine m_chunk_id;
	unsigned int m_chunk_size;
	unsigned short m_format_tag;
	unsigned short m_num_channels;
	unsigned int m_samples_per_second;
	unsigned short m_bits_per_sample;
	unsigned short m_block_align;
	unsigned int m_avg_bytes_per_second;

public:
	WaveFormatChunk();

	auto samplesPerSecond() const -> decltype(m_samples_per_second)
	{
		return m_samples_per_second;
	}

	auto numChannels() const -> decltype(m_num_channels)
	{
		return m_num_channels;
	}

	auto bitsPerSample() const -> decltype(m_bits_per_sample)
	{
		return m_bits_per_sample;
	}

	void write(std::ostream &os);
};

class WaveDataChunk {
	dls::chaine m_chunk_id;
	unsigned int m_chunk_size;
	dls::tableau<short> m_array;

public:
	WaveDataChunk();
	~WaveDataChunk() = default;

	dls::tableau<short> &array();

	void chunkSize(uint size);
	uint chunkSize() const;

	void write(std::ostream &os);
};

class WaveGenerator {
	WaveHeader *m_header;
	WaveFormatChunk *m_format;
	WaveDataChunk *m_data;

public:
	WaveGenerator();
	~WaveGenerator();

	WaveGenerator(const WaveGenerator &) = delete;
	WaveGenerator &operator=(const WaveGenerator &) = delete;

	void addFreq(double freq, int amplitude, double length);
	void save(const dls::chaine &filename);
};

}  /* namespace audio */
}  /* namespace dls */
