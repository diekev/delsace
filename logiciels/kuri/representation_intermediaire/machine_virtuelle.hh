/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "code_binaire.hh"

#include "biblinternes/structures/tableau_page.hh"

#include "structures/chaine.hh"
#include "structures/intervalle.hh"

struct AtomeFonction;
struct Compilatrice;
struct MetaProgramme;
struct Statistiques;
struct TypeFonction;

struct Erreur;

struct FrameAppel {
    AtomeFonction *fonction = nullptr;
    NoeudExpression *site = nullptr;
    octet_t *pointeur = nullptr;
    octet_t *pointeur_pile = nullptr;
};

static constexpr auto TAILLE_FRAMES_APPEL = 64;

struct DonneesExecution {
    octet_t *pile = nullptr;
    octet_t *pointeur_pile = nullptr;

    FrameAppel frames[TAILLE_FRAMES_APPEL];
    int profondeur_appel = 0;
    long instructions_executees = 0;
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

    void cree_rapports(FormatRapportProfilage format);

    void cree_rapport(InformationProfilage const &informations, FormatRapportProfilage format);
};

struct MachineVirtuelle {
    enum class ResultatInterpretation : int {
        OK,
        ERREUR,
        COMPILATION_ARRETEE,
        TERMINE,
        PASSE_AU_SUIVANT,
    };

  private:
    Compilatrice &compilatrice;

    tableau_page<DonneesExecution> donnees_execution{};

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
    long instructions_executees = 0;

    MetaProgramme *m_metaprogramme = nullptr;

    Profileuse profileuse{};

  public:
    bool stop = false;

    explicit MachineVirtuelle(Compilatrice &compilatrice_);
    ~MachineVirtuelle();

    COPIE_CONSTRUCT(MachineVirtuelle);

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
    inline void empile(NoeudExpression *site, T valeur);

    template <typename T>
    inline T depile(NoeudExpression *site);

    void depile(NoeudExpression *site, long n);

    bool appel(AtomeFonction *fonction, NoeudExpression *site);

    bool appel_fonction_interne(AtomeFonction *ptr_fonction,
                                int taille_argument,
                                FrameAppel *&frame,
                                NoeudExpression *site);
    void appel_fonction_externe(AtomeFonction *ptr_fonction,
                                int taille_argument,
                                InstructionAppel *inst_appel,
                                NoeudExpression *site,
                                ResultatInterpretation &resultat);

    inline void empile_constante(NoeudExpression *site, FrameAppel *frame);

    void installe_metaprogramme(MetaProgramme *metaprogramme);

    void desinstalle_metaprogramme(MetaProgramme *metaprogramme, int compte_executees);

    ResultatInterpretation execute_instructions(int &compte_executees);

    void imprime_trace_appel(NoeudExpression *site);

    void rapporte_erreur_execution(NoeudExpression *site, kuri::chaine_statique message);

    bool adresse_est_assignable(const void *adresse);

    ResultatInterpretation verifie_cible_appel(AtomeFonction *ptr_fonction, NoeudExpression *site);

    bool adressage_est_possible(NoeudExpression *site,
                                const void *adresse_ou,
                                const void *adresse_de,
                                const long taille,
                                bool assignation);
    void ajoute_trace_appel(Erreur &e);
};
