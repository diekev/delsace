#pragma once

template <typename T>
class courbe {
    std::vector<T> m_valeurs;

public:
    courbe();

    void ajoute_point(T const &valeur)
    {
        m_valeurs.push_back(valeur);
    }
};
