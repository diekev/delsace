/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "tacheronne.hh"

#include <fstream>

#include "parsage/lexeuse.hh"

#include "arbre_syntaxique/assembleuse.hh"
#include "arbre_syntaxique/cas_genre_noeud.hh"
#include "arbre_syntaxique/noeud_code.hh"

#include "compilatrice.hh"
#include "coulisse.hh"
#include "espace_de_travail.hh"
#include "programme.hh"
#include "syntaxeuse.hh"
#include "validation_semantique.hh"

#include "representation_intermediaire/analyse.hh"
#include "representation_intermediaire/machine_virtuelle.hh"
#include "representation_intermediaire/optimisations.hh"

#include "utilitaires/log.hh"

std::ostream &operator<<(std::ostream &os, DrapeauxTacheronne drapeaux)
{
    const char *virgule = "";
#define ENUMERE_CAPACITE(VERBE, ACTION, CHAINE, INDEX)                                            \
    if (drapeau_est_actif(drapeaux, static_cast<DrapeauxTacheronne>(1 << INDEX))) {               \
        os << virgule << "PEUT_" #VERBE;                                                          \
        virgule = "|";                                                                            \
    }

    ENUMERE_TACHES_POSSIBLES(ENUMERE_CAPACITE)

#undef ENUMERE_CAPACITE
    return os;
}

static int file_pour_raison_d_être(RaisonDÊtre raison_d_être)
{
    switch (raison_d_être) {
        case RaisonDÊtre::CHARGEMENT_FICHIER:
        {
            return OrdonnanceuseTache::FILE_CHARGEMENT;
        }
        case RaisonDÊtre::LEXAGE_FICHIER:
        {
            return OrdonnanceuseTache::FILE_LEXAGE;
        }
        case RaisonDÊtre::PARSAGE_FICHIER:
        {
            return OrdonnanceuseTache::FILE_PARSAGE;
        }
        case RaisonDÊtre::TYPAGE:
        {
            return OrdonnanceuseTache::FILE_TYPAGE;
        }
        case RaisonDÊtre::GENERATION_RI:
        case RaisonDÊtre::GENERATION_RI_PRINCIPALE_MP:
        {
            return OrdonnanceuseTache::FILE_GENERATION_RI;
        }
        case RaisonDÊtre::EXECUTION:
        {
            return OrdonnanceuseTache::FILE_EXECUTION;
        }
        case RaisonDÊtre::GENERATION_CODE_MACHINE:
        {
            return OrdonnanceuseTache::FILE_GENERATION_CODE_MACHINE;
        }
        case RaisonDÊtre::LIAISON_PROGRAMME:
        {
            return OrdonnanceuseTache::FILE_LIAISON_PROGRAMME;
        }
        case RaisonDÊtre::ENVOIE_MESSAGE:
        {
            return OrdonnanceuseTache::FILE_ENVOIE_MESSAGE;
        }
        case RaisonDÊtre::CONVERSION_NOEUD_CODE:
        {
            return OrdonnanceuseTache::FILE_CONVERSION_NOEUD_CODE;
        }
        case RaisonDÊtre::CREATION_FONCTION_INIT_TYPE:
        {
            return OrdonnanceuseTache::FILE_CREATION_FONCTION_INIT_TYPE;
        }
        case RaisonDÊtre::CALCULE_TAILLE_TYPE:
        {
            return OrdonnanceuseTache::FILE_CALCULE_TAILLE_TYPE;
        }
        case RaisonDÊtre::AUCUNE:
        {
            return -1;
        }
    }

    return -1;
}

OrdonnanceuseTache::OrdonnanceuseTache(Compilatrice *compilatrice) : m_compilatrice(compilatrice)
{
}

void OrdonnanceuseTache::crée_tâche_pour_unité(UniteCompilation *unite)
{
    assert(unite);
    assert(unite->espace);

    const auto index_file = file_pour_raison_d_être(unite->donne_raison_d_être());

    auto tache = Tâche{};
    tache.unité = unite;
    tache.espace = unite->espace;
    tache.genre = static_cast<GenreTâche>(index_file + 2);

    tâches[index_file].enfile(tache);

    pique_taille.tâches[index_file] = std::max(pique_taille.tâches[index_file],
                                               tâches[index_file].taille());
}

int64_t OrdonnanceuseTache::nombre_de_tâches_en_attente() const
{
    auto résultat = int64_t(0);
    POUR (tâches) {
        résultat += it.taille();
    }
    return résultat;
}

Tâche OrdonnanceuseTache::tâche_suivante(Tâche &tache_terminee, DrapeauxTacheronne drapeaux)
{
    if (nombre_de_tâches_en_attente() == 0) {
        m_compilatrice->gestionnaire_code->crée_tâches(*this);
    }

    if (compilation_terminée) {
        return Tâche::compilation_terminée();
    }

    for (int i = 0; i < NOMBRE_FILES; ++i) {
        if (!drapeau_est_actif(drapeaux, static_cast<DrapeauxTacheronne>(1 << i))) {
            continue;
        }

        if (!tâches[i].est_vide()) {
            return tâches[i].defile();
        }
    }

    auto unite = tache_terminee.unité;
    auto espace = EspaceDeTravail::nul();

    // unité peut-être nulle pour les tâches DORS du début de la compilation
    if (unite) {
        espace = unite->espace;
    }
    else {
        espace = tache_terminee.espace;
    }

    if (espace->possède_erreur) {
        /* Puisque l'espace possède une erreur, nous allons dormir sur l'espace par défaut de la
         * compilation. Ceci car la tâche sera retournée dans tache_suivante suivant sa complétion.
         */
        espace = m_compilatrice->espace_defaut_compilation();
    }

    return Tâche::dors(espace);
}

int64_t OrdonnanceuseTache::mémoire_utilisée() const
{
    auto memoire = int64_t(0);
    POUR (pique_taille.tâches) {
        memoire += it * taille_de(Tâche);
    }
    return memoire;
}

int OrdonnanceuseTache::enregistre_tacheronne(Badge<Tacheronne> /*badge*/)
{
    return nombre_de_tacheronnes++;
}

void OrdonnanceuseTache::supprime_toutes_les_tâches()
{
    POUR (tâches) {
        it.efface();
    }

    /* Il faut que toutes les tacheronnes soient notifiées de la fin de la compilation, donc enfile
     * un nombre égal de tâcheronnes de tâches de fin de compilation. */
    for (int i = 0; i < nombre_de_tacheronnes; ++i) {
        POUR (tâches) {
            it.enfile(Tâche::compilation_terminée());
        }
    }
}

void OrdonnanceuseTache::supprime_toutes_les_tâches_pour_espace(const EspaceDeTravail *espace,
                                                                UniteCompilation::État état)
{
    auto predicat = [&](Tâche const &tache) { return tache.espace == espace; };
    POUR (tâches) {
        for (auto &tache : it) {
            if (tache.espace != espace) {
                continue;
            }

            tache.unité->définis_état(état);
        }

        it.efface_si(predicat);
    }
}

void OrdonnanceuseTache::imprime_donnees_files(std::ostream &os)
{
    os << "Nombre de tâches dans les files :\n";
#define IMPRIME_NOMBRE_DE_TACHES(VERBE, ACTION, CHAINE, INDEX)                                    \
    os << "-- " << CHAINE << " : " << tâches[INDEX].taille() << '\n';

    ENUMERE_TACHES_POSSIBLES(IMPRIME_NOMBRE_DE_TACHES)

#undef IMPRIME_NOMBRE_DE_TACHES
}

Tacheronne::Tacheronne(Compilatrice &comp)
    : compilatrice(comp), analyseuse_ri(memoire::loge<ContexteAnalyseRI>("ContexteAnalyseRI")),
      assembleuse(memoire::loge<AssembleuseArbre>("AssembleuseArbre", this->allocatrice_noeud)),
      id(compilatrice.ordonnanceuse->enregistre_tacheronne({}))
{
}

Tacheronne::~Tacheronne()
{
    memoire::deloge("MachineVirtuelle", mv);
    memoire::deloge("AssembleuseArbre", assembleuse);
    memoire::deloge("ContexteAnalyseRI", analyseuse_ri);
}

bool Tacheronne::gère_tâche()
{
    auto temps_debut = dls::chrono::compte_seconde();
    auto tâche = Tâche::dors(compilatrice.espace_de_travail_defaut);
    auto &ordonnanceuse = compilatrice.ordonnanceuse;

    {
        tâche = ordonnanceuse->tâche_suivante(tâche, drapeaux);

        if (tâche.genre != GenreTâche::DORS) {
            nombre_dodos = 0;
        }

        if (tâche.unité) {
            tâche.unité->définis_état(
                UniteCompilation::État::EN_COURS_DE_TRAITEMENT_PAR_TACHERONNE);
        }

        switch (tâche.genre) {
            case GenreTâche::COMPILATION_TERMINÉE:
            {
                temps_scene += temps_debut.temps() - temps_executable - temps_fichier_objet;
                return true;
            }
            case GenreTâche::DORS:
            {
                /* les tâches d'exécutions sont marquées comme terminées dès que leurs
                 * métaprogrammes sont ajoutés à la machine virtuelle, il se peut qu'il en reste à
                 * exécuter alors qu'il n'y a plus de tâches à exécuter */
                if (mv && !mv->terminee()) {
                    nombre_dodos = 0;
                    exécute_métaprogrammes();
                }
                else {
                    if (compilatrice.arguments.compile_en_mode_parallèle) {
                        nombre_dodos += 1;
                        dls::chrono::dors_microsecondes(100 * nombre_dodos);
                        temps_passe_à_dormir += 0.1 * nombre_dodos;
                    }
                    return false;
                }

                break;
            }
            case GenreTâche::CHARGEMENT:
            {
                auto fichier = tâche.unité->fichier;

                if (!fichier->fut_chargé) {
                    fichier->mutex.lock();

                    if (!fichier->fut_chargé) {
                        auto debut_chargement = dls::chrono::compte_seconde();
                        auto texte = charge_contenu_fichier(
                            dls::chaine(fichier->chemin().pointeur(), fichier->chemin().taille()));
                        temps_chargement += debut_chargement.temps();

                        auto debut_tampon = dls::chrono::compte_seconde();
                        fichier->charge_tampon(lng::tampon_source(std::move(texte)));
                        temps_tampons += debut_tampon.temps();
                    }

                    fichier->mutex.unlock();
                }

                if (fichier->fut_chargé) {
                    compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                }
                else {
                    compilatrice.gestionnaire_code->mets_en_attente(tâche.unité,
                                                                    Attente::sur_lexage(fichier));
                }

                break;
            }
            case GenreTâche::LEXAGE:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_LEXER));
                auto unite = tâche.unité;
                auto fichier = unite->fichier;

                if (!fichier->fut_lexé) {
                    fichier->mutex.lock();

                    if (!fichier->en_lexage) {
                        fichier->en_lexage = true;
                        auto debut_lexage = dls::chrono::compte_seconde();
                        auto lexeuse = Lexeuse(compilatrice.contexte_lexage(unite->espace),
                                               fichier);
                        lexeuse.performe_lexage();
                        temps_lexage += debut_lexage.temps();
                        fichier->en_lexage = false;
                    }

                    fichier->mutex.unlock();
                }

                if (fichier->fut_lexé) {
                    compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                }
                else {
                    compilatrice.gestionnaire_code->mets_en_attente(
                        tâche.unité, Attente::sur_chargement(fichier));
                }

                break;
            }
            case GenreTâche::PARSAGE:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_PARSER));
                auto unite = tâche.unité;
                auto debut_parsage = dls::chrono::compte_seconde();
                auto syntaxeuse = Syntaxeuse(*this, unite);
                syntaxeuse.analyse();
                unite->fichier->fut_parsé = true;
                compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                temps_parsage += debut_parsage.temps();
                break;
            }
            case GenreTâche::TYPAGE:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_TYPER));
                auto unite = tâche.unité;
                auto debut_validation = dls::chrono::compte_seconde();
                gère_unité_pour_typage(unite);
                temps_validation += debut_validation.temps();
                break;
            }
            case GenreTâche::GENERATION_RI:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_GENERER_RI));
                auto debut_generation = dls::chrono::compte_seconde();
                if (gère_unité_pour_ri(tâche.unité)) {
                    compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                }
                constructrice_ri.temps_generation += debut_generation.temps();
                break;
            }
            case GenreTâche::OPTIMISATION:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_OPTIMISER));
                auto debut_generation = dls::chrono::compte_seconde();
                temps_optimisation += debut_generation.temps();
                break;
            }
            case GenreTâche::EXECUTION:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_EXECUTER));
                gère_unité_pour_exécution(tâche.unité);
                break;
            }
            case GenreTâche::GENERATION_CODE_MACHINE:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
                auto programme = tâche.unité->programme;
                auto coulisse = programme->coulisse();
                auto &espace = *tâche.unité->espace;
                auto repr_inter = représentation_intermédiaire_programme(
                    espace, constructrice_ri, *programme);
                if (!repr_inter.has_value()) {
                    espace.rapporte_erreur_sans_site(
                        "Impossible de générer le code machine car une erreur est survenue lors "
                        "de la création de la représentation intermédiaire du programme.");
                    return true;
                }
                if (compilatrice.arguments.émets_ri && !programme->pour_métaprogramme()) {
                    auto chemin_fichier_ri = programme->donne_chemin_pour_fichier_ri();
                    info() << "Écriture de '" << chemin_fichier_ri << "' ...";
                    std::ofstream of(vers_std_path(chemin_fichier_ri));
                    imprime_ri_programme(*repr_inter, of);
                }
                auto args = crée_args_génération_code(
                    compilatrice, *tâche.unité->espace, programme, *repr_inter, broyeuse);
                if (coulisse->crée_fichier_objet(args)) {
                    compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                }
                temps_generation_code += coulisse->temps_génération_code;
                temps_fichier_objet += coulisse->temps_fichier_objet;
                break;
            }
            case GenreTâche::LIAISON_PROGRAMME:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_GENERER_CODE));
                auto programme = tâche.unité->programme;
                auto coulisse = programme->coulisse();
                auto args = crée_args_liaison_objets(compilatrice, *tâche.espace, programme);
                if (coulisse->crée_exécutable(args)) {
                    compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                }
                temps_executable += coulisse->temps_exécutable;
                break;
            }
            case GenreTâche::CONVERSION_NOEUD_CODE:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_CONVERTIR_NOEUD_CODE));
                auto espace = tâche.unité->espace;
                auto noeud = tâche.unité->noeud;

                // À FAIRE(noeuds codes) : ne convertis pas les corps s'ils n'ont pas encore été
                // validés
                // À FAIRE(noeuds codes) : ne convertis pas les déclarations référées
                // (créer un NoeudCode prématurément peut se faire)
                auto convertisseuse = compilatrice.donne_convertisseuse_noeud_code_disponible();
                convertisseuse->convertis_noeud_syntaxique(espace, noeud);
                compilatrice.dépose_convertisseuse(convertisseuse);
                compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                break;
            }
            case GenreTâche::ENVOIE_MESSAGE:
            {
                assert(drapeau_est_actif(drapeaux, DrapeauxTacheronne::PEUT_ENVOYER_MESSAGE));
                compilatrice.messagère->envoie_message(tâche.unité->message);
                compilatrice.gestionnaire_code->tâche_unité_terminée(tâche.unité);
                break;
            }
            case GenreTâche::CREATION_FONCTION_INIT_TYPE:
            {
                assert(drapeau_est_actif(drapeaux,
                                         DrapeauxTacheronne::PEUT_CREER_FONCTION_INIT_TYPE));

                auto unite = tâche.unité;
                auto espace = unite->espace;
                auto type = unite->type;
                assert(type);

                crée_noeud_initialisation_type(espace, type, this->assembleuse);
                compilatrice.gestionnaire_code->tâche_unité_terminée(unite);
                break;
            }
            case GenreTâche::CALCULE_TAILLE_TYPE:
            {
                auto unite = tâche.unité;
                auto type = unite->type;
                assert(type);
                calcule_taille_type_composé(type->comme_type_composé(), false, 0);
                break;
            }
            case GenreTâche::NOMBRE_ELEMENTS:
            {
                break;
            }
        }
    }

    temps_scene += temps_debut.temps() - temps_executable - temps_fichier_objet;
    return false;
}

void Tacheronne::gère_unité_pour_typage(UniteCompilation *unite)
{
    auto sémanticienne = compilatrice.donne_sémanticienne_disponible(*this);
    auto résultat = sémanticienne->valide(unite);
    if (est_erreur(résultat)) {
        assert(unite->espace->possède_erreur);
        compilatrice.dépose_sémanticienne(sémanticienne);
        return;
    }
    if (est_attente(résultat)) {
        compilatrice.gestionnaire_code->mets_en_attente(unite, std::get<Attente>(résultat));
        compilatrice.dépose_sémanticienne(sémanticienne);
        return;
    }

    CHRONO_TYPAGE(sémanticienne->donne_stats_typage().finalisation, FINALISATION__FINALISATION);
    compilatrice.gestionnaire_code->tâche_unité_terminée(unite);
    compilatrice.dépose_sémanticienne(sémanticienne);
}

bool Tacheronne::gère_unité_pour_ri(UniteCompilation *unite)
{
    auto noeud = unite->noeud;

    if (noeud->type == nullptr && !noeud->est_déclaration_variable_multiple()) {
        unite->espace->rapporte_erreur(
            noeud, "Erreur interne: type nul sur une déclaration avant la génération de RI");
        return false;
    }

    if (unite->est_pour_generation_ri_principale_mp()) {
        constructrice_ri.génère_ri_pour_fonction_métaprogramme(unite->espace,
                                                               noeud->comme_entête_fonction());
    }
    else {
        constructrice_ri.génère_ri_pour_noeud(unite->espace, noeud);
    }

    auto entete = donne_entête_fonction(noeud);
    if (entete) {
        analyseuse_ri->analyse_ri(*unite->espace,
                                  constructrice_ri.donne_constructrice(),
                                  entete->atome->comme_fonction());
    }

    noeud->drapeaux |= DrapeauxNoeud::RI_FUT_GENEREE;
    return true;
}

void Tacheronne::gère_unité_pour_optimisation(UniteCompilation *unite)
{
    auto noeud = unite->noeud;
    auto entete = donne_entête_fonction(noeud);
    assert(entete);

    if (entete->possède_drapeau(DrapeauxNoeudFonction::EST_EXTERNE)) {
        return;
    }

    /* N'optimise pas cette fonction car le manque de retour supprime tout le code. */
    if (entete == compilatrice.interface_kuri->decl_creation_contexte) {
        return;
    }

    optimise_code(
        *unite->espace, constructrice_ri.donne_constructrice(), entete->atome->comme_fonction());
}

void Tacheronne::gère_unité_pour_exécution(UniteCompilation *unite)
{
    if (!mv) {
        mv = memoire::loge<MachineVirtuelle>("MachineVirtuelle", compilatrice);
    }

    auto metaprogramme = unite->metaprogramme;
    assert(metaprogramme->fonction->drapeaux & DrapeauxNoeud::RI_FUT_GENEREE);

    metaprogramme->données_exécution = mv->loge_données_exécution();
    mv->ajoute_métaprogramme(metaprogramme);

    exécute_métaprogrammes();
}

void Tacheronne::exécute_métaprogrammes()
{
    mv->exécute_métaprogrammes_courants();

    POUR (mv->métaprogrammes_terminés()) {
        auto espace = it->unité->espace;

        // À FAIRE : précision des messages d'erreurs
        if (it->résultat == MetaProgramme::RésultatExécution::ERREUR) {
            if (!espace->possède_erreur) {
                /* Ne rapporte qu'une seule erreur. */
                espace->rapporte_erreur(it->directive,
                                        "Erreur lors de l'exécution du métaprogramme");
            }
        }
        else if (!it->a_rapporté_une_erreur) {
            if (it->directive && it->directive->ident == ID::assert_) {
                auto résultat = *reinterpret_cast<bool *>(it->données_exécution->pointeur_pile);

                if (!résultat) {
                    espace->rapporte_erreur(it->directive, "Échec de l'assertion");
                }
            }
            else if (it->directive && it->directive->ident == ID::exécute) {
                auto type = it->directive->type;
                auto pointeur = it->données_exécution->pointeur_pile;

                // Les directives pour des expressions dans des fonctions n'ont pas d'unités
                if (!it->directive->unité) {
                    auto résultat = noeud_syntaxique_depuis_résultat(
                        espace,
                        it->directive,
                        it->directive->lexème,
                        type,
                        pointeur,
                        it->données_exécution->détectrice_fuite_de_mémoire);
                    résultat->drapeaux |= DrapeauxNoeud::NOEUD_PROVIENT_DE_RESULTAT_DIRECTIVE;
                    it->directive->substitution = résultat;
                }
            }
            else if (it->corps_texte) {
                auto résultat = *reinterpret_cast<kuri::chaine_statique *>(
                    it->données_exécution->pointeur_pile);

                if (résultat.taille() == 0) {
                    espace->rapporte_erreur(it->corps_texte,
                                            "Le corps-texte a retourné une chaine vide");
                }

                auto tampon = dls::chaine(résultat.pointeur(), résultat.taille());

                if (*tampon.fin() != '\n') {
                    tampon.ajoute('\n');
                }

                /* Remplis les sources du fichier. */
                auto fichier = it->fichier;
                assert(it->fichier);

                fichier->charge_tampon(lng::tampon_source(tampon.c_str()));

                fichier->décalage_fichier = compilatrice.chaines_ajoutées_à_la_compilation->ajoute(
                    résultat);
                compilatrice.gestionnaire_code->requiers_lexage(espace, fichier);

                /* La mémoire dû être allouée par notre_alloc, donc nous devrions pouvoir appeler
                 * free. */
                it->données_exécution->détectrice_fuite_de_mémoire.supprime_bloc(
                    const_cast<char *>(résultat.pointeur()));

                free(const_cast<char *>(résultat.pointeur()));
            }

            imprime_fuites_de_mémoire(it);
        }

        /* Maintenant que nous avons le résultat des opérations, nous pouvons indiquer que le
         * métaprogramme fut exécuté. Nous ne pouvons le faire plus tôt car un autre fil
         * d'exécution pourrait tenté d'accéder au résultat avant sa création. */
        it->état = ÉtatMétaprogramme::EXÉCUTION_TERMINÉE;
        it->vidange_logs_sur_disque();

        mv->déloge_données_exécution(it->données_exécution);

        compilatrice.gestionnaire_code->tâche_unité_terminée(it->unité);
    }
}

NoeudExpression *Tacheronne::noeud_syntaxique_depuis_résultat(
    EspaceDeTravail *espace,
    NoeudDirectiveExécute *directive,
    Lexème const *lexeme,
    Type *type,
    octet_t *pointeur,
    DétectriceFuiteDeMémoire &détectrice_fuites_de_mémoire)
{
    switch (type->genre) {
        case GenreNoeud::EINI:
        case GenreNoeud::POINTEUR:
        case GenreNoeud::POLYMORPHIQUE:
        case GenreNoeud::RÉFÉRENCE:
        case GenreNoeud::VARIADIQUE:
        case GenreNoeud::TYPE_ADRESSE_FONCTION:
        {
            espace
                ->rapporte_erreur(directive,
                                  "Type non pris en charge pour le résutat des exécutions !")
                .ajoute_message("Le type est : ", chaine_type(type), "\n");
            break;
        }
        case GenreNoeud::RIEN:
        {
            break;
        }
        case GenreNoeud::TUPLE:
        {
            // pour les tuples de retours, nous les convertissons en expression-virgule
            auto tuple = type->comme_type_tuple();
            auto virgule = assembleuse->crée_virgule(lexeme);

            POUR (tuple->membres) {
                auto pointeur_membre = pointeur + it.decalage;
                virgule->expressions.ajoute(
                    noeud_syntaxique_depuis_résultat(espace,
                                                     directive,
                                                     lexeme,
                                                     it.type,
                                                     pointeur_membre,
                                                     détectrice_fuites_de_mémoire));
            }

            return virgule;
        }
        case GenreNoeud::OCTET:
        case GenreNoeud::ENTIER_CONSTANT:
        case GenreNoeud::ENTIER_RELATIF:
        {
            uint64_t valeur = 0;

            if (type->taille_octet == 1) {
                valeur = static_cast<uint64_t>(*reinterpret_cast<char *>(pointeur));
            }
            else if (type->taille_octet == 2) {
                valeur = static_cast<uint64_t>(*reinterpret_cast<short *>(pointeur));
            }
            else if (type->taille_octet == 4 || type->taille_octet == 0) {
                valeur = static_cast<uint64_t>(*reinterpret_cast<int *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = static_cast<uint64_t>(*reinterpret_cast<int64_t *>(pointeur));
            }

            return assembleuse->crée_littérale_entier(lexeme, type, valeur);
        }
        case GenreNoeud::DÉCLARATION_ÉNUM:
        case GenreNoeud::ENUM_DRAPEAU:
        case GenreNoeud::ERREUR:
        case GenreNoeud::ENTIER_NATUREL:
        {
            uint64_t valeur = 0;
            if (type->taille_octet == 1) {
                valeur = static_cast<uint64_t>(*pointeur);
            }
            else if (type->taille_octet == 2) {
                valeur = static_cast<uint64_t>(*reinterpret_cast<unsigned short *>(pointeur));
            }
            else if (type->taille_octet == 4) {
                valeur = static_cast<uint64_t>(*reinterpret_cast<uint32_t *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = *reinterpret_cast<uint64_t *>(pointeur);
            }

            return assembleuse->crée_littérale_entier(lexeme, type, valeur);
        }
        case GenreNoeud::BOOL:
        {
            auto valeur = *reinterpret_cast<bool *>(pointeur);
            auto noeud_syntaxique = assembleuse->crée_littérale_bool(lexeme);
            noeud_syntaxique->valeur = valeur;
            noeud_syntaxique->type = type;
            return noeud_syntaxique;
        }
        case GenreNoeud::RÉEL:
        {
            double valeur = 0.0;

            // À FAIRE(r16)
            if (type->taille_octet == 4) {
                valeur = static_cast<double>(*reinterpret_cast<float *>(pointeur));
            }
            else if (type->taille_octet == 8) {
                valeur = *reinterpret_cast<double *>(pointeur);
            }

            return assembleuse->crée_littérale_réel(lexeme, type, valeur);
        }
        case GenreNoeud::DÉCLARATION_STRUCTURE:
        {
            auto type_structure = type->comme_type_structure();

            auto construction_structure = assembleuse->crée_construction_structure(lexeme,
                                                                                   type_structure);

            POUR (type_structure->donne_membres_pour_code_machine()) {
                auto pointeur_membre = pointeur + it.decalage;
                auto noeud_membre = noeud_syntaxique_depuis_résultat(espace,
                                                                     directive,
                                                                     lexeme,
                                                                     it.type,
                                                                     pointeur_membre,
                                                                     détectrice_fuites_de_mémoire);
                construction_structure->paramètres_résolus.ajoute(noeud_membre);
            }

            return construction_structure;
        }
        case GenreNoeud::DÉCLARATION_UNION:
        {
            auto type_union = type->comme_type_union();
            auto construction_union = assembleuse->crée_construction_structure(lexeme, type_union);

            if (type_union->est_nonsure) {
                auto expr = noeud_syntaxique_depuis_résultat(espace,
                                                             directive,
                                                             lexeme,
                                                             type_union->type_le_plus_grand,
                                                             pointeur,
                                                             détectrice_fuites_de_mémoire);
                construction_union->paramètres_résolus.ajoute(expr);
            }
            else {
                auto pointeur_donnees = pointeur;
                auto pointeur_discriminant = *reinterpret_cast<int *>(pointeur +
                                                                      type_union->décalage_index);
                auto index_membre = pointeur_discriminant - 1;

                auto type_donnees = type_union->membres[index_membre].type;

                for (auto i = 0; i < index_membre; ++i) {
                    construction_union->paramètres_résolus.ajoute(nullptr);
                }

                auto expr = noeud_syntaxique_depuis_résultat(espace,
                                                             directive,
                                                             lexeme,
                                                             type_donnees,
                                                             pointeur_donnees,
                                                             détectrice_fuites_de_mémoire);
                construction_union->paramètres_résolus.ajoute(expr);
            }

            return construction_union;
        }
        case GenreNoeud::CHAINE:
        {
            auto valeur_pointeur = pointeur;
            auto valeur_chaine = *reinterpret_cast<int64_t *>(pointeur + 8);

            kuri::chaine_statique chaine = {*reinterpret_cast<char **>(valeur_pointeur),
                                            valeur_chaine};

            /* Supprime le bloc s'il fut alloué. */
            auto const la_mémoire_fut_allouée = détectrice_fuites_de_mémoire.supprime_bloc(
                const_cast<char *>(chaine.pointeur()));

            auto lit_chaine = assembleuse->crée_littérale_chaine(lexeme);
            lit_chaine->valeur = compilatrice.gérante_chaine->ajoute_chaine(chaine);
            lit_chaine->type = type;

            if (la_mémoire_fut_allouée) {
                free(const_cast<char *>(chaine.pointeur()));
            }

            return lit_chaine;
        }
        case GenreNoeud::TYPE_DE_DONNÉES:
        {
            auto type_de_donnees = *reinterpret_cast<Type **>(pointeur);
            type_de_donnees = compilatrice.typeuse.type_type_de_donnees(type_de_donnees);
            return assembleuse->crée_référence_type(lexeme, type_de_donnees);
        }
        case GenreNoeud::FONCTION:
        {
            auto fonction = *reinterpret_cast<AtomeFonction **>(pointeur);

            if (!fonction->decl) {
                espace->rapporte_erreur(directive,
                                        "La fonction retournée n'a pas de déclaration !\n");
            }

            return assembleuse->crée_référence_déclaration(
                lexeme, const_cast<NoeudDéclarationEntêteFonction *>(fonction->decl));
        }
        case GenreNoeud::DÉCLARATION_OPAQUE:
        {
            auto type_opaque = type->comme_type_opaque();
            auto expr = noeud_syntaxique_depuis_résultat(espace,
                                                         directive,
                                                         lexeme,
                                                         type_opaque->type_opacifié,
                                                         pointeur,
                                                         détectrice_fuites_de_mémoire);

            /* comme dans la simplification de l'arbre, ceci doit être un transtypage vers le type
             * opaque */
            auto comme = assembleuse->crée_comme(lexeme, expr, nullptr);
            comme->type = type_opaque;
            comme->transformation = {TypeTransformation::CONVERTI_VERS_TYPE_CIBLE, type_opaque};
            comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;
            return comme;
        }
        case GenreNoeud::TABLEAU_FIXE:
        {
            auto type_tableau = type->comme_type_tableau_fixe();

            auto virgule = assembleuse->crée_virgule(lexeme);
            virgule->expressions.réserve(type_tableau->taille);

            for (auto i = 0; i < type_tableau->taille; ++i) {
                auto pointeur_valeur = pointeur + type_tableau->type_pointé->taille_octet *
                                                      static_cast<unsigned>(i);
                auto expr = noeud_syntaxique_depuis_résultat(espace,
                                                             directive,
                                                             lexeme,
                                                             type_tableau->type_pointé,
                                                             pointeur_valeur,
                                                             détectrice_fuites_de_mémoire);
                virgule->expressions.ajoute(expr);
            }

            auto construction = assembleuse->crée_construction_tableau(lexeme, virgule);
            construction->type = type_tableau;
            return construction;
        }
        case GenreNoeud::TABLEAU_DYNAMIQUE:
        {
            auto type_tableau = type->comme_type_tableau_dynamique();

            auto pointeur_donnees = *reinterpret_cast<octet_t **>(pointeur);
            auto taille_donnees = *reinterpret_cast<int64_t *>(pointeur + 8);

            if (taille_donnees == 0) {
                espace->rapporte_erreur(directive, "Retour d'un tableau dynamique de taille 0 !");
            }

            /* Supprime le bloc s'il fut alloué. */
            auto const la_mémoire_fut_allouée = détectrice_fuites_de_mémoire.supprime_bloc(
                pointeur_donnees);

            /* crée un tableau fixe */
            auto type_tableau_fixe = compilatrice.typeuse.type_tableau_fixe(
                type_tableau->type_pointé, static_cast<int>(taille_donnees));
            auto construction = noeud_syntaxique_depuis_résultat(espace,
                                                                 directive,
                                                                 lexeme,
                                                                 type_tableau_fixe,
                                                                 pointeur_donnees,
                                                                 détectrice_fuites_de_mémoire);

            /* convertis vers un tableau dynamique */
            auto comme = assembleuse->crée_comme(lexeme, construction, nullptr);
            comme->type = compilatrice.typeuse.crée_type_tranche(type_tableau->type_pointé);
            comme->transformation = {TypeTransformation::CONVERTI_TABLEAU_FIXE_VERS_TRANCHE,
                                     comme->type};
            comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

            if (la_mémoire_fut_allouée) {
                free(pointeur_donnees);
            }

            return comme;
        }
        case GenreNoeud::TYPE_TRANCHE:
        {
            auto type_tranche = type->comme_type_tranche();

            auto pointeur_donnees = *reinterpret_cast<octet_t **>(pointeur);
            auto taille_donnees = *reinterpret_cast<int64_t *>(pointeur + 8);

            if (taille_donnees == 0) {
                espace->rapporte_erreur(directive, "Retour d'une tranche de taille 0 !");
            }

            /* Supprime le bloc s'il fut alloué. */
            auto const la_mémoire_fut_allouée = détectrice_fuites_de_mémoire.supprime_bloc(
                pointeur_donnees);

            /* crée un tableau fixe */
            auto type_tableau_fixe = compilatrice.typeuse.type_tableau_fixe(
                type_tranche->type_élément, static_cast<int>(taille_donnees));
            auto construction = noeud_syntaxique_depuis_résultat(espace,
                                                                 directive,
                                                                 lexeme,
                                                                 type_tableau_fixe,
                                                                 pointeur_donnees,
                                                                 détectrice_fuites_de_mémoire);

            /* convertis vers un tableau dynamique */
            auto comme = assembleuse->crée_comme(lexeme, construction, nullptr);
            comme->type = type_tranche;
            comme->transformation = {TypeTransformation::CONVERTI_TABLEAU_FIXE_VERS_TRANCHE,
                                     comme->type};
            comme->drapeaux |= DrapeauxNoeud::TRANSTYPAGE_IMPLICITE;

            if (la_mémoire_fut_allouée) {
                free(pointeur_donnees);
            }

            return comme;
        }
        CAS_POUR_NOEUDS_HORS_TYPES:
        {
            assert_rappel(false, [&]() { dbg() << "Noeud non-géré pour type : " << type->genre; });
            break;
        }
    }

    return nullptr;
}

void Tacheronne::rassemble_statistiques(Statistiques &stats)
{
    stats.temps_exécutable = std::max(stats.temps_exécutable, temps_executable);
    stats.temps_fichier_objet = std::max(stats.temps_fichier_objet, temps_fichier_objet);
    stats.temps_génération_code = std::max(stats.temps_génération_code, temps_generation_code);
    stats.temps_ri = std::max(stats.temps_ri, constructrice_ri.temps_generation);
    stats.temps_lexage = std::max(stats.temps_lexage, temps_lexage);
    stats.temps_parsage = std::max(stats.temps_parsage, temps_parsage);
    stats.temps_typage = std::max(stats.temps_typage, temps_validation);
    stats.temps_scène = std::max(stats.temps_scène, temps_scene);
    stats.temps_chargement = std::max(stats.temps_chargement, temps_chargement);
    stats.temps_tampons = std::max(stats.temps_tampons, temps_tampons);

    constructrice_ri.rassemble_statistiques(stats);
    allocatrice_noeud.rassemble_statistiques(stats);

    stats.ajoute_mémoire_utilisée("Compilatrice", lexèmes_extra.mémoire_utilisée());

    if (mv) {
        mv->rassemble_statistiques(stats);
    }

    // std::cerr << "tâcheronne " << id << " a dormis pendant " << temps_passe_a_dormir << "ms\n";
}
