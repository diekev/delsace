template <typename T>
struct file {
private:
    tableau<T> donnees{};

public:
    void enfile(T &&valeur)
    {
        donnees.ajoute(std::move(valeur));
    }

    T defile()
    {
        T valeur = donnees[curseur];
        curseur += 1;
        comprime();

        return valeur;
    }

    void defile(tableau<T> &valeurs, long compte)
    {
        auto taille_restante = taille();

        if (compte > taille_restante) {
            compte = taille_restante;
        }

        for (auto i = 0; i < compte; ++i) {
            valeurs.pousse(std::move(donnees[curseur]));
            curseur += 1;
        }

        comprime();
    }

    long taille() const
    {
        return donnees.taille() - curseur;
    }

    bool est_vide() const
    {
        return taille() == 0;
    }

private:
    void comprime()
    {
        if (curseur <= (donnees.taille() / 2)) {
            return;
        }

        auto taille_restante = taille();
        std::rotate(donnees.debut(), donnees.debut() + curseur, donnees.fin());
        donnees.redimensionne(taille_restante);
        curseur = 0;
    }
};
