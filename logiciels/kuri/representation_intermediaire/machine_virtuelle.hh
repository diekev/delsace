/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "code_binaire.hh"

#include "biblinternes/structures/tableau_page.hh"

#include "structures/pile.hh"

#include "structures/chaine.hh"
#include "structures/intervalle.hh"
#include "structures/table_hachage.hh"

struct AtomeFonction;
struct Compilatrice;
struct Erreur;
struct MetaProgramme;
struct Statistiques;

struct FrameAppel {
    AtomeFonction *fonction = nullptr;
    NoeudExpression const *site = nullptr;
    octet_t *pointeur = nullptr;
    octet_t *pointeur_pile = nullptr;
};

static constexpr auto TAILLE_FRAMES_APPEL = 64;

#undef UTILISE_NOTRE_TABLE

/* ------------------------------------------------------------------------- */
/** \name Fuites de mémoire.
 * \{ */

/* Enregistre les allocations et désallocations. Utilisée pour détecter les fuites de mémoire dans
 * le métaprogrammes. */
struct DétectriceFuiteDeMémoire {
    /* Taille et frame d'appel d'un bloc de mémoire alloué. */
    struct InformationsBloc {
        size_t taille = 0ul;
        kuri::tableau<FrameAppel> frame{};
    };

  private:
#ifdef UTILISE_NOTRE_TABLE
    kuri::table_hachage<void *, InformationsBloc> table_allocations{""};
#else
    std::unordered_map<void *, InformationsBloc> table_allocations{};
#endif
    friend void imprime_fuites_de_mémoire(MetaProgramme *métaprogramme);

  public:
    void ajoute_bloc(void *ptr, size_t taille, kuri::tableau<FrameAppel> const &frame);

    /* Supprime les informations du bloc. Retourne vrai si le bloc existe (ou si le pointeur est
     * nul). */
    bool supprime_bloc(void *ptr);

    void réinitialise();
};

void imprime_fuites_de_mémoire(MetaProgramme *métaprogramme);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Profilage.
 * \{ */

struct EchantillonProfilage {
    FrameAppel frames[TAILLE_FRAMES_APPEL];
    int profondeur_frame_appel = 0;
    int poids = 0;
};

struct PaireEnchantillonFonction {
    AtomeFonction *fonction = nullptr;
    int nombre_echantillons = 0;
};

enum class FormatRapportProfilage : int;

struct Profileuse {
  private:
    kuri::tableau<EchantillonProfilage> échantillons{};

  public:
    void réinitialise();

    void ajoute_echantillon(MetaProgramme *métaprogramme, int poids);

    void crée_rapport(MetaProgramme *métaprogramme, FormatRapportProfilage format);
};

/** \} */

struct DonnéesExécution {
    octet_t *pile = nullptr;
    octet_t *pointeur_pile = nullptr;

    FrameAppel frames[TAILLE_FRAMES_APPEL];
    int profondeur_appel = 0;
    int64_t instructions_exécutées = 0;

    DétectriceFuiteDeMémoire détectrice_fuite_de_mémoire{};

    int compte_instructions[NOMBRE_OP_CODE] = {};

    struct DonnéesTailleEmpilée {
        FrameAppel *frame = nullptr;
        octet_t *adresse = nullptr;
        uint32_t taille = 0;
    };

    kuri::pile<DonnéesTailleEmpilée> tailles_empilées{};

    Profileuse profileuse{};

    void réinitialise();

    void imprime_stats_instructions(Enchaineuse &os);
};

struct MachineVirtuelle {
    enum class RésultatInterprétation : int {
        OK,
        ERREUR,
        COMPILATION_ARRÊTÉE,
        TERMINÉ,
        PASSE_AU_SUIVANT,
    };

    static constexpr auto TAILLE_PILE = 1024 * 1024;

  private:
    Compilatrice &compilatrice;

    tableau_page<DonnéesExécution> données_exécution{};
    /* Ramasse-miettes pour les données d'exécutions des métaprogrammes exécutés. */
    kuri::tableau<DonnéesExécution *> m_données_exécution_libres{};

    DonnéesConstantesExécutions *données_constantes = nullptr;

    kuri::tableau<MetaProgramme *, int> m_métaprogrammes{};
    kuri::tableau<MetaProgramme *, int> m_métaprogrammes_terminés{};

    bool m_métaprogrammes_terminés_lu = false;

    /* données pour l'exécution de chaque métaprogramme */
    octet_t *pile = nullptr;
    octet_t *pointeur_pile = nullptr;

    unsigned char *ptr_données_constantes = nullptr;
    unsigned char *ptr_données_globales = nullptr;

    Intervalle<void *> intervalle_adresses_globales{};
    Intervalle<void *> intervalle_adresses_pile_exécution{};

    FrameAppel *frames = nullptr;
    int profondeur_appel = 0;

    int nombre_de_métaprogrammes_exécutés = 0;
    double temps_exécution_métaprogammes = 0;
    int64_t instructions_exécutées = 0;

    MetaProgramme *m_métaprogramme = nullptr;

  public:
    bool stop = false;

    explicit MachineVirtuelle(Compilatrice &compilatrice_);
    ~MachineVirtuelle();

    EMPECHE_COPIE(MachineVirtuelle);

    void ajoute_métaprogramme(MetaProgramme *métaprogramme);

    void exécute_métaprogrammes_courants();

    kuri::tableau<MetaProgramme *, int> const &métaprogrammes_terminés()
    {
        m_métaprogrammes_terminés_lu = true;
        return m_métaprogrammes_terminés;
    }

    DonnéesExécution *loge_données_exécution();
    void déloge_données_exécution(DonnéesExécution *&donnees);

    bool terminee() const
    {
        return m_métaprogrammes.est_vide();
    }

    void rassemble_statistiques(Statistiques &stats);

  private:
    template <typename T>
    inline void empile(T valeur)
    {
        *reinterpret_cast<T *>(this->pointeur_pile) = valeur;
        incrémente_pointeur_de_pile(static_cast<int64_t>(sizeof(T)));
    }

    template <typename T>
    inline T dépile()
    {
        décrémente_pointeur_de_pile(static_cast<int64_t>(sizeof(T)));
        return *reinterpret_cast<T *>(this->pointeur_pile);
    }

    void incrémente_pointeur_de_pile(int64_t taille);
    void décrémente_pointeur_de_pile(int64_t taille);

    bool appel(AtomeFonction *fonction, NoeudExpression const *site);

    bool appel_fonction_interne(AtomeFonction *ptr_fonction,
                                int taille_argument,
                                FrameAppel *&frame);
    void appel_fonction_externe(AtomeFonction *ptr_fonction,
                                int taille_argument,
                                InstructionAppel *inst_appel,
                                RésultatInterprétation &résultat);
    void appel_fonction_compilatrice(AtomeFonction *ptr_fonction,
                                     RésultatInterprétation &resultat);
    void appel_fonction_intrinsèque(AtomeFonction *ptr_fonction);

    inline void empile_constante(FrameAppel *frame);

    void installe_métaprogramme(MetaProgramme *métaprogramme);

    void désinstalle_métaprogramme(MetaProgramme *métaprogramme, int compte_exécutées);

    RésultatInterprétation exécute_instructions(int &compte_exécutées);

    void imprime_trace_appel(NoeudExpression const *site);

    void rapporte_erreur_exécution(kuri::chaine_statique message);

    bool adresse_est_assignable(const void *adresse);

    RésultatInterprétation vérifie_cible_appel(AtomeFonction *ptr_fonction);

    bool adressage_est_possible(const void *adresse_ou,
                                const void *adresse_de,
                                const int64_t taille,
                                bool assignation);
    void ajoute_trace_appel(Erreur &e);

    kuri::tableau<FrameAppel> donne_tableau_frame_appel() const;

    NoeudExpression const *donne_site_adresse_courante() const;

    void notifie_empile(FrameAppel *frame, octet_t *adresse_op, uint32_t);
    [[nodiscard]] RésultatInterprétation notifie_dépile(FrameAppel *frame,
                                                        octet_t *adresse_op,
                                                        uint32_t);
};
