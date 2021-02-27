
template <typename T>
struct rammasse_miette {
private:
    struct noeud {
        noeud *suivant = nullptr;
    };

    static_assert(sizeof(T) >= sizeof(noeud), "Type trop petit pour être une miette");

    noeud *racine = nullptr;

public:
    void ajoute(T *ptr)
    {
        auto n = reinterpret_cast<noeud *>(ptr);
        if (racine) {
            racine->suivant = racine;
            racine = n;
        }
        else {
            racine = n;
            racine->suivant = nullptr;
        }
    }

    T *donne_miette()
    {
        auto m = racine;
        racine = racine->suivant;
        return reinterpret_cast<T *>(m);
    }

    bool est_vide()
    {
        return racine == nullptr;
    }
};

template <typename T>
struct file_segmentee {
private:
    using type_stockage = file_fixe<T, 32>;

    type_stockage file_defaut{};
    type_stockage *file_courante = nullptr;

    tableau<type_stockage *> file_attente_defilage{};

    tableau<type_stockage> files{};
    rammasse_miette<type_stockage> rm{};

    type_stockage *trouve_ou_cree_file()
    {
        if (rm.est_vide()) {
            auto nouvelle_file = memoire::loge<type_stockage>("stockage pour file_segmentee");
            tableau.ajoute(nouvelle_file);
            return nouvelle_file;
        }

        return rm.donne_miette();
    }

public:
    file_segmentee()
        : file_courante_pour_enfilage(&file_defaut)
        , file_courante_pour_defilage(&file_defaut)
    {}

    ~file_segmentee()
    {
        POUR (files) {
            memoire::deloge("stockage pour file_segmentee", it);
        }

        files.efface();
        file_defaut.efface();
    }

    void enfile(T valeur)
    {
        if (file_courante_pour_enfilage->est_pleine()) {
            file_attente_defilage.ajoute(file_courante_pour_enfilage);
            file_courante_pour_enfilage = trouve_ou_cree_file();
        }

        file_courante_pour_enfilage->enfile(valeur);
    }

    T defile()
    {
        auto v = file_courante_pour_defilage->defile();
        if (file_courante_pour_defilage->est_vide() && file_courante_pour_defilage->est_pleine()) {
            rm.ajoute(file_courante_pour_defilage);
            file_courante_pour_defilage = file_attente_defilage.premier();
        }
        return v;
    }

    bool est_vide()
    {
        return file_attente_defilage.est_vide() && file_courante_pour_defilage->est_vide();
    }
};
