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

template <unsigned int CIBLE>
Texture<CIBLE>::Texture(unsigned int texture)
	: Texture<CIBLE>()
{
	m_unite = texture;
	glGenTextures(1, &m_code_attache);
}

template <unsigned int CIBLE>
Texture<CIBLE>::Texture(Texture<CIBLE> &&texture)
{
	swap(texture);
}

template <unsigned int CIBLE>
Texture<CIBLE>::~Texture()
{
	deloge(false);
}

template <unsigned int CIBLE>
Texture<CIBLE> &Texture<CIBLE>::operator=(Texture<CIBLE> &&texture)
{
	swap(texture);
	return *this;
}

template <unsigned int CIBLE>
typename Texture<CIBLE>::Ptr Texture<CIBLE>::cree_unique(unsigned int texture)
{
	return Ptr(new Texture(texture));
}

template <unsigned int CIBLE>
typename Texture<CIBLE>::SPtr Texture<CIBLE>::cree_partage(unsigned int texture)
{
	return SPtr(new Texture(texture));
}

template <unsigned int CIBLE>
void Texture<CIBLE>::deloge(bool renew)
{
	if (glIsTexture(m_code_attache)) {
		glDeleteTextures(1, &m_code_attache);
	}

	if (renew) {
		glGenTextures(1, &m_code_attache);
	}
}

template <unsigned int CIBLE>
void Texture<CIBLE>::attache() const
{
	glActiveTexture(GL_TEXTURE0 + m_unite);
	glBindTexture(CIBLE, m_code_attache);
}

template <unsigned int CIBLE>
void Texture<CIBLE>::detache() const
{
	glActiveTexture(GL_TEXTURE0 + m_unite);
	glBindTexture(CIBLE, 0);
}

template <unsigned int CIBLE>
void Texture<CIBLE>::type(unsigned int type, unsigned int format, int internal_format)
{
	m_type = type;
	m_format = format;
	m_format_interne = internal_format;
}

template <unsigned int CIBLE>
void Texture<CIBLE>::filtre_min_mag(int min, int mag) const
{
	glTexParameteri(CIBLE, GL_TEXTURE_MIN_FILTER, min);
	glTexParameteri(CIBLE, GL_TEXTURE_MAG_FILTER, mag);
}

template <unsigned int CIBLE>
void Texture<CIBLE>::enveloppe(int wrap) const
{
	glTexParameteri(CIBLE, GL_TEXTURE_WRAP_S, wrap);

	if (CIBLE == GL_TEXTURE_2D) {
		glTexParameteri(CIBLE, GL_TEXTURE_WRAP_T, wrap);
	}

	if (CIBLE == GL_TEXTURE_3D) {
		glTexParameteri(CIBLE, GL_TEXTURE_WRAP_T, wrap);
		glTexParameteri(CIBLE, GL_TEXTURE_WRAP_R, wrap);
	}
}

template <unsigned int CIBLE>
void Texture<CIBLE>::genere_mip_map(int base, int max) const
{
	glTexParameteri(CIBLE, GL_TEXTURE_BASE_LEVEL, base);
	glTexParameteri(CIBLE, GL_TEXTURE_MAX_LEVEL, max);
	glGenerateMipmap(CIBLE);
}

template <unsigned int CIBLE>
void Texture<CIBLE>::remplie(const void *data, int *size) const
{
	if (CIBLE == GL_TEXTURE_1D) {
		glTexImage1D(CIBLE, 0, m_format_interne, size[0], m_bordure, m_format,
		             m_type, data);
	}
	else if (CIBLE == GL_TEXTURE_2D) {
		glTexImage2D(CIBLE, 0, m_format_interne, size[0], size[1], m_bordure,
		             m_format, m_type, data);
	}
	else {
		glTexImage3D(CIBLE, 0, m_format_interne, size[0], size[1], size[2],
					 m_bordure, m_format, m_type, data);
	}
}

template <unsigned int CIBLE>
void Texture<CIBLE>::sous_image(const void *data, int *size, int *offset) const
{
	if (CIBLE == GL_TEXTURE_1D) {
		glTexSubImage1D(CIBLE, 0, offset[0], size[0], m_format, m_type, data);
	}
	else if (CIBLE == GL_TEXTURE_2D) {
		glTexSubImage2D(CIBLE, 0, offset[0], offset[1], size[0], size[1],
		                m_format, m_type, data);
	}
	else {
		glTexSubImage3D(CIBLE, 0, offset[0], offset[1], offset[2],
		                size[0], size[1], size[2], m_format, m_type, data);
	}
}

template <unsigned int CIBLE>
unsigned int Texture<CIBLE>::code_attache() const
{
	return m_code_attache;
}

template <unsigned int CIBLE>
unsigned int Texture<CIBLE>::unite() const
{
	return m_unite;
}

template <unsigned int CIBLE>
void Texture<CIBLE>::unite(unsigned int number)
{
	m_unite = number;
}
