/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2025 Kévin Dietrich. */

#pragma once

#include <shared_mutex>

namespace kuri {

template <typename T, class Mutex = std::shared_mutex>
class Synchrone {
    T m_donnée{}; /* CRUCIAL : non-mutable ! */
    mutable Mutex m_mutex{};

  public:
    class PointeurVerrouillé {
        Synchrone *m_parent;

      public:
        PointeurVerrouillé() = delete;

        explicit PointeurVerrouillé(Synchrone *parent) : m_parent(parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock();
            }
        }

        PointeurVerrouillé(const PointeurVerrouillé &rhs) : m_parent(rhs.m_parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock();
            }
        }

        const PointeurVerrouillé &operator=(const PointeurVerrouillé &rhs)
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

        PointeurVerrouillé(PointeurVerrouillé &&rhs) : m_parent(rhs.m_parent)
        {
            rhs.m_parent = nullptr;
        }

        PointeurVerrouillé &operator=(PointeurVerrouillé &&rhs)
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

        ~PointeurVerrouillé()
        {
            if (m_parent) {
                m_parent->m_mutex.unlock();
            }
        }

        T *operator->()
        {
            return m_parent ? &m_parent->m_donnée : nullptr;
        }

        T &operator*()
        {
            return m_parent->m_donnée;
        }
    };

    /* Cet opérateur va appelé PointeurVerrouillé::operator->() de manière
     * transitive. Quand cet opérateur sera créé, un objet de type
     * PointeurVerrouillé sera créé, puis son opérateur '->' sera appelé, enfin,
     * quand tout sera fini, l'objet sera détruit. Ce méchanisme, ctor/dtor, est
     * utilisé pour synchroniser le pointeur. */
    PointeurVerrouillé operator->()
    {
        return PointeurVerrouillé(this);
    }

    PointeurVerrouillé verrou_écriture()
    {
        return PointeurVerrouillé(this);
    }

    class ConstPointeurVerrouillé {
        const Synchrone *m_parent;

      public:
        ConstPointeurVerrouillé() = delete;

        explicit ConstPointeurVerrouillé(const Synchrone *parent) : m_parent(parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock_shared();
            }
        }

        ConstPointeurVerrouillé(const ConstPointeurVerrouillé &rhs) : m_parent(rhs.m_parent)
        {
            if (m_parent) {
                m_parent->m_mutex.lock_shared();
            }
        }

        const ConstPointeurVerrouillé &operator=(const ConstPointeurVerrouillé &rhs)
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

        ConstPointeurVerrouillé(ConstPointeurVerrouillé &&rhs) : m_parent(rhs.m_parent)
        {
            rhs.m_parent = nullptr;
        }

        ConstPointeurVerrouillé &operator=(ConstPointeurVerrouillé &&rhs)
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

        ~ConstPointeurVerrouillé()
        {
            if (m_parent) {
                m_parent->m_mutex.unlock_shared();
            }
        }

        T const *operator->() const
        {
            return m_parent ? &m_parent->m_donnée : nullptr;
        }

        T const &operator*() const
        {
            return m_parent->m_donnée;
        }
    };

    /* Cet opérateur va appelé ConstPointeurVerrouillé::operator->() de manière
     * transitive. Quand cet opérateur sera créé, un objet de type
     * ConstPointeurVerrouillé sera créé, puis son opérateur '->' sera appelé,
     * enfin, quand tout sera fini, l'objet sera détruit. Ce méchanisme,
     * ctor/dtor, est utilisé pour synchroniser le pointeur. */
    ConstPointeurVerrouillé operator->() const
    {
        return ConstPointeurVerrouillé(this);
    }

    ConstPointeurVerrouillé verrou_lecture() const
    {
        return ConstPointeurVerrouillé(this);
    }

    Synchrone() = default;

  private:
    Synchrone(const Synchrone &rhs, const ConstPointeurVerrouillé &) : m_donnée(rhs.m_donnée)
    {
    }

  public:
    Synchrone(const Synchrone &rhs) noexcept(std::is_nothrow_copy_constructible<T>::value)
        : Synchrone(rhs, rhs.operator->())
    {
    }

    Synchrone(Synchrone &&rhs) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_donnée(std::move(rhs.m_donnée))
    {
    }

    template <typename... Args>
    Synchrone(Args... args) : m_donnée(args...)
    {
    }

    explicit Synchrone(const T &rhs) noexcept(std::is_nothrow_copy_constructible<T>::value)
        : m_donnée(rhs)
    {
    }

    explicit Synchrone(T &&rhs) noexcept(std::is_nothrow_move_constructible<T>::value)
        : m_donnée(std::move(rhs))
    {
    }

    const Synchrone &operator=(
        const Synchrone &rhs)  // noexcept(std::is_nothrow_assignable<T>::value)
    {
        /* Fais en sorte que les verroux sont acquis dans le bon ordre. */
        if (std::less<void *>()(this, &rhs)) {
            auto g1 = operator->(), g2 = rhs.operator->();
            m_donnée = rhs.m_donnée;
        }
        else if (std::less<void *>()(&rhs, this)) {
            auto g1 = rhs.operator->(), g2 = operator->();
            m_donnée = rhs.m_donnée;
        }

        return *this;
    }

    const Synchrone &operator=(Synchrone &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        /* Fais en sorte que les verroux sont acquis dans le bon ordre. */
        if (std::less<void *>()(this, &rhs)) {
            auto g1 = operator->(), g2 = rhs.operator->();
            m_donnée = std::move(rhs.m_donnée);
        }
        else if (std::less<void *>()(&rhs, this)) {
            auto g1 = rhs.operator->(), g2 = operator->();
            m_donnée = std::move(rhs.m_donnée);
        }

        return *this;
    }

    const Synchrone &operator=(const T &rhs)  // noexcept(std::is_nothrow_assignable<T>::value)
    {
        auto g1 = operator->();
        m_donnée = rhs;

        return *this;
    }

    const Synchrone &operator=(T &&rhs) noexcept(std::is_nothrow_move_assignable<T>::value)
    {
        auto g1 = operator->();
        m_donnée = std::move(rhs);

        return *this;
    }

    void swap(Synchrone &&rhs) noexcept
    {
        /* Fais en sorte que les verroux sont acquis dans le bon ordre. */
        if (std::less<void *>()(this, &rhs)) {
            auto g1 = operator->(), g2 = rhs.operator->();
            std::swap(m_donnée, rhs.m_donnée);
        }
        else if (std::less<void *>()(&rhs, this)) {
            auto g1 = rhs.operator->(), g2 = operator->();
            std::swap(m_donnée, rhs.m_donnée);
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
        fonction(m_donnée);
    }

    template <typename Fonction>
    void avec_verrou_écriture(Fonction &&fonction)
    {
        operator->();
        fonction(m_donnée);
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
