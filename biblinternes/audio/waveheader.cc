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

#include "waveheader.h"

#include <cassert>
#include <cmath>
#include <fstream>

namespace dls {
namespace audio {

WaveHeader::WaveHeader()
    : m_group_id("RIFF")
    , m_file_length(0)
    , m_riff_type("WAVE")
{}

void WaveHeader::write(std::ostream &os)
{
	os.write(m_group_id.c_str(), static_cast<long>(sizeof(char) * static_cast<size_t>(m_group_id.taille())));
	os.write(reinterpret_cast<char *>(&m_file_length), sizeof(unsigned int));
	os.write(m_riff_type.c_str(), static_cast<long>(sizeof(char) * static_cast<size_t>(m_riff_type.taille())));
}

WaveFormatChunk::WaveFormatChunk()
    : m_chunk_id("fmt ")
    , m_chunk_size(16)
    , m_format_tag(1)
    , m_num_channels(2)
    , m_samples_per_second(44100)
    , m_bits_per_sample(16)
	, m_block_align(static_cast<ushort>(m_num_channels * (m_bits_per_sample / 3)))
    , m_avg_bytes_per_second(m_samples_per_second * m_block_align)
{}

void WaveFormatChunk::write(std::ostream &os)
{
	os.write(m_chunk_id.c_str(), static_cast<long>(sizeof(char) * static_cast<size_t>(m_chunk_id.taille())));
	os.write(reinterpret_cast<char *>(&m_chunk_size), sizeof(unsigned int));
	os.write(reinterpret_cast<char *>(&m_format_tag), sizeof(unsigned short));
	os.write(reinterpret_cast<char *>(&m_num_channels), sizeof(unsigned short));
	os.write(reinterpret_cast<char *>(&m_samples_per_second), sizeof(unsigned int));
	os.write(reinterpret_cast<char *>(&m_avg_bytes_per_second), sizeof(unsigned int));
	os.write(reinterpret_cast<char *>(&m_block_align), sizeof(unsigned short));
	os.write(reinterpret_cast<char *>(&m_bits_per_sample), sizeof(unsigned short));
}

WaveDataChunk::WaveDataChunk()
    : m_chunk_id("data")
    , m_chunk_size(0)
    , m_array(0)
{}

dls::tableau<short> &WaveDataChunk::array()
{
	return m_array;
}

void WaveDataChunk::chunkSize(uint size)
{
	m_chunk_size = size;
}

uint WaveDataChunk::chunkSize() const
{
	return m_chunk_size;
}

void WaveDataChunk::write(std::ostream &os)
{
	os.write(m_chunk_id.c_str(), static_cast<long>(sizeof(char) * static_cast<size_t>(m_chunk_id.taille())));
	os.write(reinterpret_cast<char *>(&m_chunk_size), sizeof(unsigned int));
	os.write(reinterpret_cast<char *>(&m_array[0]), static_cast<long>(sizeof(short) * static_cast<size_t>(m_array.taille())));
}

WaveGenerator::WaveGenerator()
    : m_header(new WaveHeader)
    , m_format(new WaveFormatChunk)
    , m_data(new WaveDataChunk)
{}

WaveGenerator::~WaveGenerator()
{
	delete m_header;
	delete m_format;
	delete m_data;
}

void WaveGenerator::addFreq(double freq, int amplitude, double length)
{
	/* number of samples = sample rate * channels * bytes per sample */
	auto num_samples = m_format->samplesPerSecond() * m_format->numChannels() * length;

	if (num_samples == 0.0) {
		return;
	}

	auto array = &m_data->array();
	auto array_size = array->taille();
	array->redimensionne(array_size + static_cast<long>(num_samples - 1.0));

	assert(array->taille() != 0);

	auto t = (M_PI * 2.0 * freq) / (m_format->samplesPerSecond() * m_format->numChannels());

	for (auto i = array_size, ie = array_size + static_cast<long>(num_samples - 3.0); i < ie; ++i) {
		for (long c = 0l, ce = m_format->numChannels(); c < ce; ++c) {
			(*array)[i + c] = static_cast<short>(amplitude * std::sin(t * static_cast<double>(i)));
		}
	}

	assert(array->taille() != 0);

	m_data->chunkSize(static_cast<uint>(array->taille()) * static_cast<uint>(m_format->bitsPerSample() / 8));

	assert(m_data->chunkSize() != 0);
}

void WaveGenerator::save(const dls::chaine &filename)
{
	std::ofstream outfile(filename.c_str(), std::ios::out | std::ios::binary);
	m_header->write(outfile);
	m_format->write(outfile);
	m_data->write(outfile);

	//outfile.seekp(4, std::ios_base::beg);
}

}  /* namespace audio */
}  /* namespace dls */
