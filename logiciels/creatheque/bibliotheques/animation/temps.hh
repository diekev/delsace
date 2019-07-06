#pragma once

/**
 * La classe temps_obturateur sert à définir le type de temps s'écoulant
 * entre l'ouverture et la fermeture de l'obturateur de la caméra.
 */
template <typename T>
class temps_obturateur {
    T m_valeur;

public:
    explicit temps_obturateur(T valeur = static_cast<T>(0))
        : m_valeur(valeur)
    {}

    explicit operator T ()
    {
        return m_valeur;
    }
};

/**
 * La classe temps_scene sert à définir le type de temps s'écoulant
 * entre le début et la fin de l'animation de la scène.
 */
template <typename T>
class temps_scene {
    T m_valeur;

public:
    explicit temps_scene(T valeur = static_cast<T>(0))
        : m_valeur(valeur)
    {}

    explicit operator T ()
    {
        return m_valeur;
    }
};

/**
 * La classe temps_image sert à définir le type de temps pour une image
 * entre le début et la fin de l'animation de la scène.
 */
template <typename T>
class temps_image {
    T m_valeur;

public:
    explicit temps_image(T valeur = static_cast<T>(0))
        : m_valeur(valeur)
    {}

    explicit operator T ()
    {
        return m_valeur;
    }
};
