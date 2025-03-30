#pragma once

#include <iterator>

namespace dls {
namespace outils {

/* À FAIRE : déduplique. */
template <typename T>
struct plage_iterable {
	T m_debut;
	T m_fin;

public:
	plage_iterable(T d, T f)
		: m_debut(d)
		, m_fin(f)
	{}

	T debut()
	{
		return m_debut;
	}

	T fin()
	{
		return m_fin;
	}

	T begin()
	{
		return m_debut;
	}

	T end()
	{
		return m_fin;
	}

	long taille() const
	{
		return (m_fin - m_debut);
	}
};
template <typename T>
struct plage_iterable_liste {
	T m_debut;
	T m_fin;

public:
	plage_iterable_liste(T d, T f)
		: m_debut(d)
		, m_fin(f)
	{}

	T debut()
	{
		return m_debut;
	}

	T fin()
	{
		return m_fin;
	}

	T begin()
	{
		return m_debut;
	}

	T end()
	{
		return m_fin;
	}

	long taille() const
	{
		auto t = 0;
		auto d = m_debut;

		while (d != m_fin) {
			++t;
			++d;
		}

		return t;
	}
};

/**
 * Simple itérateur avant pour utiliser des boucle for basé sur des plages.
 * L'itérateur peut aller en arrière si le pas est négatif.
 */
template <typename T>
class iterateur_plage {
	T m_valeur;
	T m_pas;

public:
	iterateur_plage(T valeur, T pas) noexcept
		: m_valeur(valeur)
		, m_pas(pas)
	{}

	iterateur_plage(const iterateur_plage &autre) noexcept
		: iterateur_plage(autre.m_valeur, autre.m_pas)
	{}

	iterateur_plage &operator=(const iterateur_plage &autre)
	{
		m_valeur = autre.m_valeur;
		m_pas = autre.m_pas;

		return *this;
	}

	iterateur_plage &operator++() noexcept
	{
		this->m_valeur += m_pas;
		return (*this);
	}

	const T &operator*() const noexcept
	{
		return m_valeur;
	}

	const T &step() const noexcept
	{
		return m_pas;
	}
};

template <typename T>
iterateur_plage<T> operator++(iterateur_plage<T> &it, int) noexcept
{
	auto tmp(it);
	++it;
	return (tmp);
}

template <typename T>
bool operator==(const iterateur_plage<T> &ita, const iterateur_plage<T> &itb) noexcept
{
	return (*ita == *itb);
}

template <typename T>
bool operator!=(const iterateur_plage<T> &ita, const iterateur_plage<T> &itb) noexcept
{
	/* Petit bidouillage pour s'assure que ita est dans les bornes puisque cet
	 * opérateur est utilisé terminer les boucles. */
	if (ita.step() < 0) {
		return !(ita == itb) && (*ita > *itb);
	}

	return !(ita == itb) && (*ita < *itb);
}

/**
 * Classe auxiliaire pour créer une plage itérable.
 */
template <typename T>
class adaptateur_plage {
	T m_debut, m_fin, m_pas;

public:
	adaptateur_plage(T debut, T fin, T pas) noexcept
		: m_debut(debut)
		, m_fin(fin)
		, m_pas(pas)
	{}

	using iterateur = iterateur_plage<T>;

	iterateur begin() const noexcept
	{
		return { m_debut, m_pas };
	}

	iterateur end() const noexcept
	{
		return { m_fin, 0 };
	}
};

/**
 * Crée une plage allant de 'debut' à 'fin' (exclusif) avec le 'pas' spécifié.
 *
 * Utilisation :
 *
 * for (auto i : plage(0, 2)) {
 *     // i va de 0 à 1
 * }
 *
 * for (auto i : plage(10, 2, -1)) {
 *     // i va de 10 à 3
 * }
 */
template <typename T>
adaptateur_plage<T> plage(T debut, T fin, T pas = 1)
{
	/* make end equal to begin if it is less, so as to avoid infinite loops */
	if (fin < debut && pas > 0) {
		fin = debut;
	}

	return { debut, fin, pas };
}

/**
 * Crée une plage allant de 0 à 'fin' avec un pas de 1.
 *
 * Utilisation :
 *
 * for (auto i : plage(20)) {
 *     // i va de 0 à 19
 * }
 */
template <typename T>
adaptateur_plage<T> plage(T fin)
{
	return { 0, fin, 1 };
}

/* ************************************************************************** */

/**
 * Classe adaptatrice pour obtenir des itérateurs avant depuis des itérateurs
 * inverse.
 */
template <typename T>
class adaptateur_inverse {
	T &m_conteneur;

public:
	explicit adaptateur_inverse(T &container) noexcept
		: m_conteneur(container)
	{}

	typename T::iteratrice_inverse begin() const noexcept
	{
		return m_conteneur.debut_inverse();
	}

	typename T::iteratrice_inverse end() const noexcept
	{
		return m_conteneur.debut_inverse();
	}
};

/**
 * Classe adaptatrice pour obtenir des itérateurs avant constants depuis des
 * itérateurs inverse constants.
 */
template <typename T>
class adaptateur_inverse_const {
	const T &m_conteneur;

public:
	explicit adaptateur_inverse_const(const T &container) noexcept
		: m_conteneur(container)
	{}

	typename T::const_iteratrice_inverse begin() const noexcept
	{
		return m_conteneur.debut_inverse();
	}

	typename T::const_iteratrice_inverse end() const noexcept
	{
		return m_conteneur.fin_inverse();
	}
};

/**
 * Inverse la direction des itérateurs du conteneur spécifié, pour effectivement
 * avoir une plage inverse pour une boucle for.
 */
template <typename T>
adaptateur_inverse<T> inverse_iterateur(T &container)
{
	return adaptateur_inverse<T>(container);
}

/**
 * Inverse la direction des itérateurs du conteneur spécifié, pour effectivement
 * avoir une plage inverse pour une boucle for.
 * Ceci est une surcharge pour des itérateurs constants.
 */
template <typename T>
adaptateur_inverse_const<T> inverse_iterateur(const T &container)
{
	return adaptateur_inverse_const<T>(container);
}

/* ************************************************************************** */

#if 0
/**
 * Itérateur à accès aléatoire conforme aux spécifications de la STL.
 */
template<typename T>
class iterateur_normal : public std::iterator<std::random_access_iterator_tag, T> {
protected:
	T *m_pointeur;

public:
	explicit iterateur_normal(T* pointeur = nullptr)
		: m_pointeur(pointeur)
	{}

	iterateur_normal(const iterateur_normal &autre) = default;
	~iterateur_normal() = default;

	iterateur_normal &operator=(T *pointeur) noexcept
	{
		m_pointeur = pointeur;
		return (*this);
	}

	iterateur_normal &operator=(const iterateur_normal &autre) = default;

    explicit operator bool() const noexcept
    {
		return (m_pointeur != nullptr);
    }

	iterateur_normal &operator+=(const int &n) noexcept
	{
		m_pointeur += n;
		return (*this);
	}

	iterateur_normal &operator-=(const int &n) noexcept
	{
		m_pointeur -= n;
		return (*this);
	}

	iterateur_normal &operator++() noexcept
	{
		++m_pointeur;
		return (*this);
	}

	iterateur_normal &operator--() noexcept
	{
		--m_pointeur;
		return (*this);
	}

	std::ptrdiff_t operator-(const iterateur_normal &autre) noexcept
	{
		return std::distance(autre.getPtr(), this->getPtr());
	}

	T &operator*() noexcept { return *m_pointeur; }
	const T &operator*() const noexcept { return *m_pointeur; }
	T *operator->() noexcept { return m_pointeur; }

	T *getPtr() const noexcept { return m_pointeur; }
	const T *getConstPtr() const noexcept { return m_pointeur; }
};

template <typename T>
iterateur_normal<T> operator+(const iterateur_normal<T> &it, const int &n) noexcept
{
	return iterateur_normal<T>(it.getPtr() + n);
}

template <typename T>
iterateur_normal<T> operator-(const iterateur_normal<T> &it, const int &n) noexcept
{
	return iterateur_normal<T>(it.getPtr() - n);
}

template <typename T>
iterateur_normal<T> operator++(iterateur_normal<T> &iter, int) noexcept
{
	auto temp(iter);
	++iter;
	return temp;
}

template <typename T>
iterateur_normal<T> operator--(iterateur_normal<T> &iter, int) noexcept
{
	auto temp(iter);
	--iter;
	return temp;
}

template <typename T>
bool operator==(const iterateur_normal<T> &ita, const iterateur_normal<T> &itb) noexcept
{
	return (ita.getConstPtr() == itb.getConstPtr());
}

template <typename T>
bool operator!=(const iterateur_normal<T> &ita, const iterateur_normal<T> &itb) noexcept
{
	return !(ita == itb);
}
#endif

}  /* namespace outils */
}  /* namespace dls */
