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
 * The Original Code is Copyright (C) 2017 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

namespace dls {
namespace outils {

/**
 * Concept pour les types pouvant être utilisé pour les masques binaires.
 *
 * Les types doivent connaîtres les opérations suivantes :
 * ^, ^=, &, &= |, |=, ~.
 */
template <typename T>
concept bool ConceptValeurMasque = requires(T a, T b)
{
	a = a | b;
	a |= b;
	a = a & b;
	a &= b;
	a = a ^ b;
	a ^= b;
	a = ~b;
};

/**
 * La classe `MasqueBinaire` sert à créer un type pouvant être utilisé pour
 * s'assurer que l'on passe bel et bien un masque binaire en paramètre des
 * fonctions utilisant des masque, ou que l'on obtienne bel et bien un masque
 * binaire depuis les fonctions ou opérations retournant des masques
 * et non un nombre quelconque.
 */
template <ConceptValeurMasque T>
class MasqueBinaire {
	T m_masque = static_cast<T>(0);

public:
	/**
	 * Construit un masque binaire par défaut où la valeur du masque est mise à
	 * zéro.
	 */
	MasqueBinaire() = default;

	/**
	 * Construit un masque binaire dont la valeur est égale à celle passée en
	 * paramètre.
	 */
	explicit MasqueBinaire(T valeur)
		: m_masque(valeur)
	{}

	/**
	 * Construit un masque binaire dont la valeur est égale à celle du masque
	 * passé en paramètre.
	 */
	MasqueBinaire(const MasqueBinaire &autre)
		: m_masque(autre.masque())
	{}

	/**
	 * Retourne la valeur du masque.
	 */
	T masque() const
	{
		return m_masque;
	}

	/**
	 * Met à jour la valeur du masque avec celle passée en paramètre. Après que
	 * cette fonction soit appelée, la condition `masque == valeur` est vraie.
	 */
	void masque(T valeur)
	{
		m_masque = valeur;
	}

	/**
	 * Met à jour la valeur du masque avec celle passée en paramètre. Après que
	 * cette fonction soit appelée, la condition `masque == valeur` est vraie.
	 */
	MasqueBinaire &operator=(const T &valeur)
	{
		m_masque = valeur;
		return *this;
	}

	/**
	 * Met à jour la valeur du masque avec celle de celui passé en paramètre.
	 * Après que cette fonction soit appelée, la condition `masque == autre` est
	 * vraie.
	 */
	MasqueBinaire &operator=(const MasqueBinaire &autre)
	{
		m_masque = autre.masque();
		return *this;
	}

	/**
	 * Met à jour la valeur du masque en performant un `ou` inclusif entre ce
	 * masque et la valeur passée en paramètre.
	 */
	MasqueBinaire &operator|=(const T &autre)
	{
		m_masque |= autre;
		return *this;
	}

	/**
	 * Met à jour la valeur du masque en performant un `ou` inclusif entre ce
	 * masque et celui passé en paramètre.
	 */
	MasqueBinaire &operator|=(const MasqueBinaire &autre)
	{
		m_masque |= autre.masque();
		return *this;
	}

	/**
	 * Met à jour la valeur du masque en performant un `et` entre ce masque et
	 * la valeur passée en paramètre.
	 */
	MasqueBinaire &operator&=(const T &autre)
	{
		m_masque &= autre;
		return *this;
	}

	/**
	 * Met à jour la valeur du masque en performant un `et` entre ce masque et
	 * celui passé en paramètre.
	 */
	MasqueBinaire &operator&=(const MasqueBinaire &autre)
	{
		m_masque &= autre.masque();
		return *this;
	}

	/**
	 * Met à jour la valeur du masque en performant un `ou` exclusif entre ce
	 * masque et la valeur passée en paramètre.
	 */
	MasqueBinaire &operator^=(const T &autre)
	{
		m_masque ^= autre;
		return *this;
	}

	/**
	 * Met à jour la valeur du masque en performant un `ou` exclusif entre ce
	 * masque et celui passé en paramètre.
	 */
	MasqueBinaire &operator^=(const MasqueBinaire &autre)
	{
		m_masque ^= autre.masque();
		return *this;
	}
};

/**
 * Retourne vrai si les deux masques passés en paramètre ont des valeurs égales.
 */
template <ConceptValeurMasque T>
auto operator==(const MasqueBinaire<T> &masque1, const MasqueBinaire<T> &masque2)
{
	return masque1.masque() == masque2.masque();
}

/**
 * Retourne vrai si la valeur du masque et la valeur passés en paramètre sont
 * égales.
 */
template <ConceptValeurMasque T>
auto operator==(const MasqueBinaire<T> &masque, const T &valeur)
{
	return masque.masque() == valeur;
}

/**
 * Retourne vrai si la valeur du masque et la valeur passés en paramètre sont
 * égales.
 */
template <ConceptValeurMasque T>
auto operator==(const T &valeur, const MasqueBinaire<T> &masque)
{
	return masque.masque() == valeur;
}

/**
 * Retourne vrai si les deux masques passés en paramètre ont des valeurs
 * différentes.
 */
template <ConceptValeurMasque T>
auto operator!=(const MasqueBinaire<T> &masque1, const MasqueBinaire<T> &masque2)
{
	return !(masque1 == masque2);
}

/**
 * Retourne vrai si la valeur du masque et la valeur passés en paramètre sont
 * différentes.
 */
template <ConceptValeurMasque T>
auto operator!=(const MasqueBinaire<T> &masque, const T &valeur)
{
	return !(masque == valeur);
}

/**
 * Retourne vrai si la valeur du masque et la valeur passés en paramètre sont
 * différentes.
 */
template <ConceptValeurMasque T>
auto operator!=(const T &valeur, const MasqueBinaire<T> &masque)
{
	return !(masque == valeur);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération `et`
 * entre les deux masques spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator&(const MasqueBinaire<T> &masque1, const MasqueBinaire<T> &masque2)
{
	MasqueBinaire<T> temp(masque1);
	return (temp &= masque2);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération `et`
 * entre le masque et la valeur spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator&(const MasqueBinaire<T> &masque, const T &valeur)
{
	MasqueBinaire<T> temp(masque);
	return (temp ^= valeur);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération `et`
 * entre la valeur et le masque spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator&(const T &valeur, const MasqueBinaire<T> &masque)
{
	MasqueBinaire<T> temp(masque);
	return (temp &= valeur);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération `ou`
 * entre les deux masques spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator|(const MasqueBinaire<T> &masque1, const MasqueBinaire<T> &masque2)
{
	MasqueBinaire<T> temp(masque1);
	return (temp |= masque2);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération `ou`
 * entre le masque et la valeur spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator|(const MasqueBinaire<T> &masque, const T &valeur)
{
	MasqueBinaire<T> temp(masque);
	return (temp |= valeur);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération `ou`
 * entre la valeur et le masque spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator|(const T &valeur, const MasqueBinaire<T> &masque)
{
	MasqueBinaire<T> temp(masque);
	return (temp |= valeur);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération
 * `ou exclusif` entre les deux masques spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator^(const MasqueBinaire<T> &masque1, const MasqueBinaire<T> &masque2)
{
	MasqueBinaire<T> temp(masque1);
	return (temp ^= masque2);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération
 * `ou exclusif` entre le masque et la valeur spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator^(const MasqueBinaire<T> &masque, const T &valeur)
{
	MasqueBinaire<T> temp(masque);
	return (temp ^= valeur);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération
 * `ou exclusif` entre la valeur et le masque spécifiés en paramètre.
 */
template <ConceptValeurMasque T>
auto operator^(const T &valeur, const MasqueBinaire<T> &masque)
{
	MasqueBinaire<T> temp(masque);
	return (temp ^= valeur);
}

/**
 * Retourne un masque binaire dont la valeur est le résultat de l'opération
 * `non` appliquée sur la masque spécifié en paramètre.
 */
template <ConceptValeurMasque T>
auto operator~(const MasqueBinaire<T> &masque)
{
	return MasqueBinaire<T>(~masque.masque());
}

}  /* namespace outils */
}  /* namespace dls */
