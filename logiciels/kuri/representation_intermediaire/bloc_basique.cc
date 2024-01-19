/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#include "bloc_basique.hh"

#include "compilation/espace_de_travail.hh"
#include "utilitaires/log.hh"

#include "constructrice_ri.hh"
#include "impression.hh"
#include "instructions.hh"

/* ********************************************************************************************* */

static uint32_t donne_drapeau_masque_pour_instruction(GenreInstruction genre)
{
    return 1u << uint32_t(genre);
}

void Bloc::ajoute_instruction(Instruction *inst)
{
    masque_instructions |= donne_drapeau_masque_pour_instruction(inst->genre);
    instructions.ajoute(inst);
}

bool Bloc::possède_instruction_de_genre(GenreInstruction genre) const
{
    auto const drapeau = donne_drapeau_masque_pour_instruction(genre);
    return (masque_instructions & drapeau) != 0;
}

int Bloc::donne_id() const
{
    return label->id;
}

void Bloc::ajoute_enfant(Bloc *enfant)
{
    enfant->ajoute_parent(this);

    POUR (enfants) {
        if (it == enfant) {
            return;
        }
    }

    enfants.ajoute(enfant);
}

void Bloc::remplace_parent(Bloc *parent, Bloc *par)
{
    enlève_du_tableau(parents, parent);
    ajoute_parent(par);
    par->ajoute_enfant(this);
}

void Bloc::enlève_parent(Bloc *parent)
{
    enlève_du_tableau(parents, parent);
}

void Bloc::enlève_enfant(Bloc *enfant)
{
    enlève_du_tableau(enfants, enfant);
}

void Bloc::fusionne_enfant(Bloc *enfant)
{
    this->instructions.supprime_dernier();
    this->instructions.réserve_delta(enfant->instructions.taille());

    POUR (enfant->instructions) {
        this->ajoute_instruction(it);
    }

    /* Supprime la référence à l'enfant dans la hiérarchie. */
    this->enlève_enfant(enfant);
    enfant->enlève_parent(this);

    /* Remplace l'enfant pour nou-même comme parent dans ses enfants. */
    POUR (enfant->enfants) {
        this->ajoute_enfant(it);
        it->remplace_parent(enfant, this);
    }

    enfant->instructions.efface();
}

void Bloc::réinitialise()
{
    masque_instructions = 0;
    label = nullptr;
    est_atteignable = false;
    instructions.efface();
    parents.efface();
    enfants.efface();
}

void Bloc::déconnecte_pour_branche_morte(Bloc *parent)
{
    enlève_parent(parent);
    parent->enlève_enfant(this);

    if (parents.taille() == 0) {
        /* Nous n'étions accessible que depuis le parent, supprimons-nous de nos enfants. */
        POUR (enfants) {
            it->enlève_parent(this);
        }

        instructions.efface();
        enfants.efface();
    }
}

void Bloc::tag_instruction_à_supprimer(Instruction *inst)
{
    inst->drapeaux |= DrapeauxAtome::EST_À_SUPPRIMER;
    instructions_à_supprimer = true;
}

bool Bloc::supprime_instructions_à_supprimer()
{
    if (!instructions_à_supprimer) {
        return false;
    }

    auto nouvelle_fin = std::stable_partition(
        instructions.debut(), instructions.fin(), [](Instruction *inst) {
            return !inst->possède_drapeau(DrapeauxAtome::EST_À_SUPPRIMER);
        });

    auto nouvelle_taille = std::distance(instructions.debut(), nouvelle_fin);
    instructions.redimensionne(static_cast<int>(nouvelle_taille));

    instructions_à_supprimer = false;
    fonction_et_blocs->marque_instructions_modifiés();
    return true;
}

void Bloc::enlève_du_tableau(kuri::tableau<Bloc *, int> &tableau, Bloc const *bloc)
{
    for (auto i = 0; i < tableau.taille(); ++i) {
        if (tableau[i] == bloc) {
            std::swap(tableau[i], tableau[tableau.taille() - 1]);
            tableau.redimensionne(tableau.taille() - 1);
            break;
        }
    }
}

void Bloc::ajoute_parent(Bloc *parent)
{
    POUR (parents) {
        if (it == parent) {
            return;
        }
    }

    parents.ajoute(parent);
}

static void imprime_bloc(Bloc const *bloc,
                         int decalage_instruction,
                         bool surligne_inutilisees,
                         Enchaineuse &os)
{
    os << "Bloc " << bloc->label->id << ' ';

    auto virgule = " [";
    if (bloc->parents.est_vide()) {
        os << virgule;
    }
    for (auto parent : bloc->parents) {
        os << virgule << parent->label->id;
        virgule = ", ";
    }
    os << "]";

    virgule = " [";
    if (bloc->enfants.est_vide()) {
        os << virgule;
    }
    for (auto enfant : bloc->enfants) {
        os << virgule << enfant->label->id;
        virgule = ", ";
    }
    os << "]\n";

    POUR (bloc->instructions) {
        it->numero = decalage_instruction++;
    }

    imprime_instructions(
        bloc->instructions, os, OptionsImpressionType::AUCUNE, surligne_inutilisees);
}

kuri::chaine imprime_bloc(Bloc const *bloc, int decalage_instruction, bool surligne_inutilisees)
{
    Enchaineuse sortie;
    imprime_bloc(bloc, decalage_instruction, surligne_inutilisees, sortie);
    return sortie.chaine();
}

static void imprime_blocs(const kuri::tableau<Bloc *, int> &blocs, Enchaineuse &os)
{
    os << "=================== Blocs ===================\n";

    int decalage_instruction = 0;
    POUR (blocs) {
        imprime_bloc(it, decalage_instruction, false, os);
        decalage_instruction += it->instructions.taille();
    }
}

kuri::chaine imprime_blocs(const kuri::tableau<Bloc *, int> &blocs)
{
    Enchaineuse sortie;
    imprime_blocs(blocs, sortie);
    return sortie.chaine();
}

static Bloc *trouve_bloc_pour_label(kuri::tableau<Bloc *, int> &table_blocs,
                                    InstructionLabel const *label)
{
    return table_blocs[label->numero];
}

static Bloc *crée_bloc_pour_label(kuri::tableau<Bloc *, int> &blocs,
                                  kuri::tableau<Bloc *, int> &blocs_libres,
                                  kuri::tableau<Bloc *, int> &table_blocs,
                                  InstructionLabel *label)
{
    Bloc *bloc;
    if (!blocs_libres.est_vide()) {
        bloc = blocs_libres.dernier_élément();
        blocs_libres.supprime_dernier();
    }
    else {
        bloc = memoire::loge<Bloc>("Bloc");
    }

    bloc->label = label;
    blocs.ajoute(bloc);
    table_blocs[label->numero] = bloc;
    return bloc;
}

static void détruit_blocs(kuri::tableau<Bloc *, int> &blocs)
{
    POUR (blocs) {
        memoire::deloge("Bloc", it);
    }
    blocs.efface();
}

/* ------------------------------------------------------------------------- */
/** \name Graphe.
 * \{ */

void Graphe::ajoute_connexion(Atome *a, Atome *b, int index_bloc)
{
    connexions.ajoute({a, b, index_bloc});

    if (connexions_pour_inst.possède(a)) {
        auto &idx = connexions_pour_inst.trouve_ref(a);
        idx.ajoute(static_cast<int>(connexions.taille() - 1));
    }
    else {
        kuri::tablet<int, 4> idx;
        idx.ajoute(static_cast<int>(connexions.taille() - 1));
        connexions_pour_inst.insère(a, idx);
    }
}

void Graphe::construit(const kuri::tableau<Instruction *, int> &instructions, int index_bloc)
{
    POUR (instructions) {
        visite_opérandes_instruction(
            it, [&](Atome *atome_courant) { ajoute_connexion(atome_courant, it, index_bloc); });
    }
}

bool Graphe::est_uniquement_utilisé_dans_bloc(Instruction const *inst, int index_bloc) const
{
    auto idx = connexions_pour_inst.valeur_ou(inst, {});
    POUR (idx) {
        auto &connexion = connexions[it];
        if (index_bloc != connexion.index_bloc) {
            return false;
        }
    }

    return true;
}

void Graphe::réinitialise()
{
    connexions_pour_inst.reinitialise();
    connexions.efface();
}

/** \} */

FonctionEtBlocs::~FonctionEtBlocs()
{
    détruit_blocs(blocs);
    détruit_blocs(blocs_libres);
}

bool FonctionEtBlocs::convertis_en_blocs(EspaceDeTravail &espace, AtomeFonction *atome_fonc)
{
    fonction = atome_fonc;

    auto numero_instruction = atome_fonc->params_entrée.taille();

    table_blocs.redimensionne(atome_fonc->instructions.taille() + numero_instruction);

    POUR (atome_fonc->instructions) {
        it->numero = numero_instruction++;

        if (it->est_label()) {
            auto bloc = crée_bloc_pour_label(blocs, blocs_libres, table_blocs, it->comme_label());
            bloc->fonction_et_blocs = this;
        }
    }

    Bloc *bloc_courant = nullptr;
    POUR (atome_fonc->instructions) {
        if (it->est_label()) {
            bloc_courant = trouve_bloc_pour_label(table_blocs, it->comme_label());

            if (!bloc_courant) {
                espace.rapporte_erreur(it->site, "Erreur interne, aucun bloc pour le label");
                return false;
            }

            continue;
        }

        bloc_courant->ajoute_instruction(it);

        if (it->est_branche()) {
            auto bloc_cible = trouve_bloc_pour_label(table_blocs, it->comme_branche()->label);

            if (!bloc_cible) {
                espace.rapporte_erreur(
                    it->site,
                    "Erreur interne, aucun bloc pour le label de la branche inconditionnelle");
                return false;
            }
            bloc_courant->ajoute_enfant(bloc_cible);
            continue;
        }

        if (it->est_branche_cond()) {
            auto label_si_vrai = it->comme_branche_cond()->label_si_vrai;
            auto label_si_faux = it->comme_branche_cond()->label_si_faux;

            auto bloc_si_vrai = trouve_bloc_pour_label(table_blocs, label_si_vrai);
            auto bloc_si_faux = trouve_bloc_pour_label(table_blocs, label_si_faux);

            if (!bloc_si_vrai) {
                espace.rapporte_erreur(
                    it->site, "Erreur interne, aucun bloc pour le label de la branche si vrai");
                return false;
            }
            if (!bloc_si_faux) {
                espace.rapporte_erreur(
                    it->site, "Erreur interne, aucun bloc pour le label de la branche si faux");
                return false;
            }

            bloc_courant->ajoute_enfant(bloc_si_vrai);
            bloc_courant->ajoute_enfant(bloc_si_faux);
            continue;
        }
    }

    return true;
}

void FonctionEtBlocs::réinitialise()
{
    graphe.réinitialise();
    graphe_nécessite_ajournement = true;

    fonction = nullptr;
    les_blocs_ont_été_modifiés = false;

    POUR (blocs) {
        it->réinitialise();
        blocs_libres.ajoute(it);
    }

    blocs.efface();
}

void FonctionEtBlocs::marque_blocs_modifiés()
{
    les_blocs_ont_été_modifiés = true;
    graphe_nécessite_ajournement = true;
}

void FonctionEtBlocs::marque_instructions_modifiés()
{
    graphe_nécessite_ajournement = true;
}

static void marque_blocs_atteignables(VisiteuseBlocs &visiteuse)
{
    visiteuse.prépare_pour_nouvelle_traversée();
    while (Bloc *bloc_courant = visiteuse.bloc_suivant()) {
        bloc_courant->est_atteignable = true;
    }
}

void FonctionEtBlocs::supprime_blocs_inatteignables(VisiteuseBlocs &visiteuse)
{
    /* Réinitalise les drapaux. */
    POUR (blocs) {
        it->est_atteignable = false;
    }

    marque_blocs_atteignables(visiteuse);

    auto résultat = std::stable_partition(
        blocs.begin(), blocs.end(), [](Bloc const *bloc) { return bloc->est_atteignable; });

    if (résultat == blocs.end()) {
        return;
    }

    auto nombre_de_nouveaux_blocs = int(std::distance(blocs.begin(), résultat));

    for (auto i = nombre_de_nouveaux_blocs; i < blocs.taille(); i++) {
        blocs[i]->réinitialise();
        blocs_libres.ajoute(blocs[i]);
    }

    blocs.redimensionne(nombre_de_nouveaux_blocs);
    marque_blocs_modifiés();
}

void FonctionEtBlocs::ajourne_instructions_fonction_si_nécessaire()
{
    if (!les_blocs_ont_été_modifiés) {
        return;
    }

    transfère_instructions_blocs_à_fonction(blocs, fonction);
    les_blocs_ont_été_modifiés = false;
}

Graphe &FonctionEtBlocs::donne_graphe_ajourné()
{
    if (!graphe_nécessite_ajournement) {
        return graphe;
    }

    graphe.réinitialise();

    POUR (blocs) {
        graphe.construit(it->instructions, it->donne_id());
    }

    graphe_nécessite_ajournement = false;
    return graphe;
}

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxiliaires.
 * \{ */

void transfère_instructions_blocs_à_fonction(kuri::tableau_statique<Bloc *> blocs,
                                             AtomeFonction *fonction)
{
#undef IMPRIME_STATS
#ifdef IMPRIME_STATS
    static int instructions_supprimées = 0;
    static int instructions_totales = 0;
    auto const ancien_compte = fonction->instructions.taille();
#endif

    auto nombre_instructions = 0;
    POUR (blocs) {
        nombre_instructions += 1 + it->instructions.taille();
    }

    fonction->instructions.redimensionne(nombre_instructions);

    int décalage_instruction = 0;

    POUR (blocs) {
        fonction->instructions[décalage_instruction++] = it->label;

        for (auto inst : it->instructions) {
            fonction->instructions[décalage_instruction++] = inst;
        }
    }

    fonction->instructions.redimensionne(décalage_instruction);

#ifdef IMPRIME_STATS
    auto const supprimées = (ancien_compte - fonction->instructions.taille());
    instructions_totales += ancien_compte;

    if (supprimées != 0) {
        instructions_supprimées += supprimées;
        dbg() << "Supprimé " << instructions_supprimées << " / " << instructions_totales
              << " instructions ("
              << (double(instructions_supprimées) / double(instructions_totales)) << "%)";
    }
#endif
}

/** \} */

/* ------------------------------------------------------------------------- */
/** \name VisiteuseBlocs
 * \{ */

VisiteuseBlocs::VisiteuseBlocs(const FonctionEtBlocs &fonction_et_blocs)
    : m_fonction_et_blocs(fonction_et_blocs)
{
}

void VisiteuseBlocs::prépare_pour_nouvelle_traversée()
{
    blocs_visités.efface();
    à_visiter.efface();
    à_visiter.enfile(m_fonction_et_blocs.blocs[0]);
}

bool VisiteuseBlocs::a_visité(Bloc *bloc) const
{
    return blocs_visités.possède(bloc);
}

Bloc *VisiteuseBlocs::bloc_suivant()
{
    while (!à_visiter.est_vide()) {
        auto bloc_courant = à_visiter.defile();

        if (blocs_visités.possède(bloc_courant)) {
            continue;
        }

        blocs_visités.insère(bloc_courant);

        POUR (bloc_courant->enfants) {
            à_visiter.enfile(it);
        }

        return bloc_courant;
    }

    return nullptr;
}

/** \} */
