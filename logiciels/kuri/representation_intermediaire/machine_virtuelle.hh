/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "code_binaire.hh"

#include "biblinternes/structures/tableau_page.hh"

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
#undef STATS_OP_CODES

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

struct DonneesExecution {
    octet_t *pile = nullptr;
    octet_t *pointeur_pile = nullptr;

    FrameAppel frames[TAILLE_FRAMES_APPEL];
    int profondeur_appel = 0;
    int64_t instructions_executees = 0;

    DétectriceFuiteDeMémoire détectrice_fuite_de_mémoire{};

    int compte_instructions[NOMBRE_OP_CODE] = {};

    void réinitialise();

    void imprime_stats_instructions(std::ostream &os);
};

struct EchantillonProfilage {
    FrameAppel frames[TAILLE_FRAMES_APPEL];
    int profondeur_frame_appel = 0;
    int poids = 0;
};

struct InformationProfilage {
    MetaProgramme *metaprogramme = nullptr;
    kuri::tableau<EchantillonProfilage> echantillons{};
};

struct PaireEnchantillonFonction {
    AtomeFonction *fonction = nullptr;
    int nombre_echantillons = 0;
};

enum class FormatRapportProfilage : int;

struct Profileuse {
    kuri::tableau<InformationProfilage> informations_pour_metaprogrammes{};

    InformationProfilage &informations_pour(MetaProgramme *metaprogramme);

    void ajoute_echantillon(MetaProgramme *metaprogramme, int poids);

    void crée_rapports(FormatRapportProfilage format);

    void crée_rapport(InformationProfilage const &informations, FormatRapportProfilage format);
};

struct MachineVirtuelle {
    enum class ResultatInterpretation : int {
        OK,
        ERREUR,
        COMPILATION_ARRETEE,
        TERMINE,
        PASSE_AU_SUIVANT,
    };

    static constexpr auto TAILLE_PILE = 1024 * 1024;

  private:
    Compilatrice &compilatrice;

    tableau_page<DonneesExecution> donnees_execution{};
    /* Ramasse-miettes pour les données d'exécutions des métaprogrammes exécutés. */
    kuri::tableau<DonneesExecution *> m_données_exécution_libres{};

    DonneesConstantesExecutions *donnees_constantes = nullptr;

    kuri::tableau<MetaProgramme *, int> m_metaprogrammes{};
    kuri::tableau<MetaProgramme *, int> m_metaprogrammes_termines{};

    bool m_metaprogrammes_termines_lu = false;

    /* données pour l'exécution de chaque métaprogramme */
    octet_t *pile = nullptr;
    octet_t *pointeur_pile = nullptr;

    unsigned char *ptr_donnees_constantes = nullptr;
    unsigned char *ptr_donnees_globales = nullptr;

    Intervalle<void *> intervalle_adresses_globales{};
    Intervalle<void *> intervalle_adresses_pile_execution{};

    FrameAppel *frames = nullptr;
    int profondeur_appel = 0;

    int nombre_de_metaprogrammes_executes = 0;
    double temps_execution_metaprogammes = 0;
    int64_t instructions_executees = 0;

    MetaProgramme *m_metaprogramme = nullptr;

    Profileuse profileuse{};

  public:
    bool stop = false;

    explicit MachineVirtuelle(Compilatrice &compilatrice_);
    ~MachineVirtuelle();

    EMPECHE_COPIE(MachineVirtuelle);

    void ajoute_metaprogramme(MetaProgramme *metaprogramme);

    void execute_metaprogrammes_courants();

    kuri::tableau<MetaProgramme *, int> const &metaprogrammes_termines()
    {
        m_metaprogrammes_termines_lu = true;
        return m_metaprogrammes_termines;
    }

    DonneesExecution *loge_donnees_execution();
    void deloge_donnees_execution(DonneesExecution *&donnees);

    bool terminee() const
    {
        return m_metaprogrammes.est_vide();
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
    inline T depile()
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
                                InstructionAppel *inst_appel);
    void appel_fonction_compilatrice(AtomeFonction *ptr_fonction,
                                     ResultatInterpretation &resultat);
    void appel_fonction_intrinsèque(AtomeFonction *ptr_fonction);

    inline void empile_constante(FrameAppel *frame);

    void installe_metaprogramme(MetaProgramme *metaprogramme);

    void desinstalle_metaprogramme(MetaProgramme *metaprogramme, int compte_executees);

    ResultatInterpretation execute_instructions(int &compte_executees);

    void imprime_trace_appel(NoeudExpression const *site);

    void rapporte_erreur_execution(kuri::chaine_statique message);

    bool adresse_est_assignable(const void *adresse);

    ResultatInterpretation verifie_cible_appel(AtomeFonction *ptr_fonction);

    bool adressage_est_possible(const void *adresse_ou,
                                const void *adresse_de,
                                const int64_t taille,
                                bool assignation);
    void ajoute_trace_appel(Erreur &e);

    kuri::tableau<FrameAppel> donne_tableau_frame_appel() const;

    NoeudExpression const *donne_site_adresse_courante() const;
};
