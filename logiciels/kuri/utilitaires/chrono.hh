/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include <assert.h>
#include <cstdint>
#include <functional>
#include <sys/time.h>
#include <unistd.h>

namespace kuri {
namespace chrono {

/**
 * Structure pour enrober les fonctions maintenant() et delta(temps) afin de
 * mieux controler leurs précisions (heure/minute/secondes...)
 */
template <int64_t D>
struct compte_temps {
  private:
    double m_temps = 0.0;

    /**
     * Retourne le temps courrant en microseconde.
     */
    [[nodiscard]] inline double maintenant() const noexcept
    {
        struct timeval now;
        gettimeofday(&now, nullptr);

        return static_cast<double>(now.tv_sec) * 1000000.0 + static_cast<double>(now.tv_usec);
    }

    /**
     * Retourne le temps en microseconde s'étant écoulé depuis le temps passé en paramètre.
     */
    [[nodiscard]] inline double delta(double temps) const noexcept
    {
        return maintenant() - temps;
    }

  public:
    explicit compte_temps(bool commence_ = true)
    {
        if (commence_) {
            commence();
        }
    }

    inline void commence()
    {
        m_temps = maintenant();
    }

    [[nodiscard]] inline double temps() const
    {
        return delta(m_temps) / static_cast<double>(D);
    }
};

using compte_milliseconde = compte_temps<1000>;
using compte_seconde = compte_temps<1000000>;

template <int64_t N>
struct chrono_rappel {
  private:
    compte_temps<N> chrono{};
    std::function<void(double)> m_rappel{};

  public:
    chrono_rappel(std::function<void(double)> rappel) : m_rappel(rappel)
    {
    }

    ~chrono_rappel()
    {
        m_rappel(chrono.temps());
    }
};

using chrono_rappel_milliseconde = chrono_rappel<1000>;

/**
 * Structure définissant un chronomètre pouvant être arrêté et repris. Elle
 * s'appele seulement 'metre' car avec l'espace de nom, cela donne
 * chrono::metre, forçant ainsi une bonne utilisation des espaces de nom.
 */
template <int64_t D>
struct metre {
  private:
    compte_temps<D> m_compteuse{false};
    double m_total = 0.0;
    bool m_lance = false;

  public:
    inline void commence()
    {
        m_lance = true;
        m_total = 0.0;
        m_compteuse.commence();
    }

    [[nodiscard]] inline double arrete()
    {
        if (m_lance) {
            m_total += m_compteuse.temps();
            m_lance = false;
        }

        return m_total;
    }

    inline void reprend()
    {
        if (!m_lance) {
            m_compteuse.commence();
            m_lance = true;
        }
    }

    [[nodiscard]] inline double lis()
    {
        if (m_lance) {
            m_total = arrete();
            reprend();
        }

        return m_total;
    }
};

using metre_seconde = metre<1000000>;

inline void dors_microsecondes(int microsecondes)
{
    assert(microsecondes >= 0);
    usleep(static_cast<unsigned>(microsecondes));
}

}  // namespace chrono
}  // namespace kuri
