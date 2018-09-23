#pragma once

template <typename T>
class courbe {
    std::vector<T> m_valeurs;

public:
    courbe();

    void ajoute_point(const T &valeur)
    {
        m_valeurs.push_back(valeur);
    }
};
