#pragma once

template <typename T>
class courbe {
	dls::tableau<T> m_valeurs;

public:
    courbe();

    void ajoute_point(T const &valeur)
    {
        m_valeurs.push_back(valeur);
    }
};
