/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include <shared_mutex>

namespace kuri {

template <typename T, class Mutex = std::shared_mutex>
class Synchrone {
    T m_donnee{}; /* CRUCIAL : non-mutable ! */
    mutable Mutex m_mutex{};

  public:
    class PointeurVerrouille {
        Synchrone *m_parent;

      public:
        PointeurVerrouille() = delete;

        explicit PointeurVerrouille(Synchrone *parent) : m_parent(parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock();
            }
        }

        PointeurVerrouille(const PointeurVerrouille &rhs) : m_parent(rhs.m_parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock();
            }
        }

        const PointeurVerrouille &operator=(const PointeurVerrouille &rhs)
        {
            if (this != &rhs) {
                if (m_parent) {
                    m_parent->m_mutex.unlock();
                }

                m_parent = rhs.m_parent;

                if (m_parent) {
                    m_parent->m_mutex.lock();
                }
            }

            return *this;
        }

        PointeurVerrouille(PointeurVerrouille &&rhs) : m_parent(rhs.m_parent)
        {
            rhs.m_parent = nullptr;
        }

        PointeurVerrouille &operator=(PointeurVerrouille &&rhs)
        {
            if (this != &rhs) {
                if (m_parent) {
                    m_parent->m_mutex.unlock();
                }

                m_parent = rhs.m_parent;
                rhs.m_parent = nullptr;
            }

            return *this;
        }

        ~PointeurVerrouille()
        {
            if (m_parent) {
                m_parent->m_mutex.unlock();
            }
        }

        T *operator->()
        {
            return m_parent ? &m_parent->m_donnee : nullptr;
        }

        T &operator*()
        {
            return m_parent->m_donnee;
        }
    };

    /* Cet opérateur va appelé PointeurVerrouille::operator->() de manière
     * transitive. Quand cet opérateur sera créé, un objet de type
     * PointeurVerrouille sera créé, puis son opérateur '->' sera appelé, enfin,
     * quand tout sera fini, l'objet sera détruit. Ce méchanisme, ctor/dtor, est
     * utilisé pour synchroniser le pointeur. */
    PointeurVerrouille operator->()
    {
        return PointeurVerrouille(this);
    }

    PointeurVerrouille verrou_ecriture()
    {
        return PointeurVerrouille(this);
    }

    class ConstPointeurVerrouille {
        const Synchrone *m_parent;

      public:
        ConstPointeurVerrouille() = delete;

        explicit ConstPointeurVerrouille(const Synchrone *parent) : m_parent(parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock_shared();
            }
        }

        ConstPointeurVerrouille(const ConstPointeurVerrouille &rhs) : m_parent(rhs.m_parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock_shared();
            }
        }

        const ConstPointeurVerrouille &operator=(const ConstPointeurVerrouille &rhs)
        {
            if (this != &rhs) {
                if (m_parent) {
                    m_parent->m_mutex.unlock_shared();
                }

                m_parent = rhs.m_parent;

                if (m_parent) {
                    m_parent->m_mutex.lock_shared();
                }
            }

            return *this;
        }

        ConstPointeurVerrouille(ConstPointeurVerrouille &&rhs) : m_parent(rhs.m_parent)
        {
            rhs.m_parent = nullptr;
        }

        ConstPointeurVerrouille &operator=(ConstPointeurVerrouille &&rhs)
        {
            if (this != &rhs) {
                if (m_parent) {
                    m_parent->m_mutex.unlock_shared();
                }

                m_parent = rhs.m_parent;
                rhs.m_parent = nullptr;
            }

            return *this;
        }

        ~ConstPointeurVerrouille()
        {
            if (m_parent) {
                m_parent->m_mutex.unlock_shared();
            }
        }

        T const *operator->() const
        {
            return m_parent ? &m_parent->m_donnee : nullptr;
        }

        T const &operator*() const
        {
            return m_parent->m_donnee;
        }
    };

    /* Cet opérateur va appelé ConstPointeurVerrouille::operator->() de manière
     * transitive. Quand cet opérateur sera créé, un objet de type
     * ConstPointeurVerrouille sera créé, puis son opérateur '->' sera appelé,
     * enfin, quand tout sera fini, l'objet sera détruit. Ce méchanisme,
     * ctor/dtor, est utilisé pour synchroniser le pointeur. */
    ConstPointeurVerrouille operator->() const
    {
        return ConstPointeurVerrouille(this);
    }

    ConstPointeurVerrouille verrou_lecture() const
    {
        return ConstPointeurVerrouille(this);
    }

    Synchrone() = default;

  private:
    Synchrone(const Synchrone &rhs, const ConstPointeurVerrouille &) : m_donnee(rhs.m_donnee)
    {
    }

  public:
    Synchrone(const Synchrone &rhs) noexcept(std::is_nothrow_copy_constructible<T>::value)
        : Synchrone(rhs, rhs.operator->())
    {
    }

    Synchrone(Synchrone &&rhs) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_donnee(std::move(rhs.m_donnee))
    {
    }

    template <typename... Args>
    Synchrone(Args... args) : m_donnee(args...)
    {
    }

    explicit Synchrone(const T &rhs) noexcept(std::is_nothrow_copy_constructible<T>::value)
        : m_donnee(rhs)
    {
    }

    explicit Synchrone(T &&rhs) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_donnee(std::move(rhs))
    {
    }

    const Synchrone &operator=(
        const Synchrone &rhs)  // noexcept(std::is_nothrow_assignable<T>::value)
    {
        /* Fais en sorte que les verroux sont acquis dans le bon ordre. */
        if (std::less<void *>()(this, &rhs)) {
            auto g1 = operator->(), g2 = rhs.operator->();
            m_donnee = rhs.m_donnee;
        }
        else if (std::less<void *>()(&rhs, this)) {
            auto g1 = rhs.operator->(), g2 = operator->();
            m_donnee = rhs.m_donnee;
        }

        return *this;
    }

    const Synchrone &operator=(Synchrone &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        /* Fais en sorte que les verroux sont acquis dans le bon ordre. */
        if (std::less<void *>()(this, &rhs)) {
            auto g1 = operator->(), g2 = rhs.operator->();
            m_donnee = std::move(rhs.m_donnee);
        }
        else if (std::less<void *>()(&rhs, this)) {
            auto g1 = rhs.operator->(), g2 = operator->();
            m_donnee = std::move(rhs.m_donnee);
        }

        return *this;
    }

    const Synchrone &operator=(const T &rhs)  // noexcept(std::is_nothrow_assignable<T>::value)
    {
        auto g1 = operator->();
        m_donnee = rhs;

        return *this;
    }

    const Synchrone &operator=(T &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        auto g1 = operator->();
        m_donnee = std::move(rhs);

        return *this;
    }

    void swap(Synchrone &&rhs) noexcept
    {
        /* Fais en sorte que les verroux sont acquis dans le bon ordre. */
        if (std::less<void *>()(this, &rhs)) {
            auto g1 = operator->(), g2 = rhs.operator->();
            std::swap(m_donnee, rhs.m_donnee);
        }
        else if (std::less<void *>()(&rhs, this)) {
            auto g1 = rhs.operator->(), g2 = operator->();
            std::swap(m_donnee, rhs.m_donnee);
        }
    }

    const Synchrone &en_const() const
    {
        return *this;
    }

    template <typename Fonction>
    void avec_verrou_lecture(Fonction &&fonction) const
    {
        operator->();
        fonction(m_donnee);
    }

    template <typename Fonction>
    void avec_verrou_ecriture(Fonction &&fonction)
    {
        operator->();
        fonction(m_donnee);
    }
};

#define ARG_1(a, ...) a

#define ARG_2_OU_1_IMPL(a, b, ...) b

#define ARG_2_OU_1(...) ARG_2_OU_1_IMPL(__VA_ARGS__, __VA_ARGS__)

#define SYNCHRONISE(...)                                                                          \
    if (bool _1 = false) {                                                                        \
    }                                                                                             \
    else                                                                                          \
        for (auto _2 = ARG_2_OU_1(__VA_ARGS__).operator->(); !_1; _1 = true)                      \
            for (auto &ARG_1(__VA_ARGS__) = *_2.operator->(); !_1; _1 = true)

#define SYNCHRONISE_CONST(...)                                                                    \
    SYNCHRONISE(ARG_1(__VA_ARGS__), (ARG_2_OU_1(__VA_ARGS__)).en_const())

}  // namespace kuri
