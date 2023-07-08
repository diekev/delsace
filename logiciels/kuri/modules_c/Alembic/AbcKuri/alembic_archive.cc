/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "alembic_archive.hh"

#include "../../InterfaceCKuri/contexte_kuri.hh"

#include "../alembic_types.h"

#include "alembic_export.hh"
#include "utilitaires.hh"

using namespace Alembic;

namespace AbcKuri {

// --------------------------------------------------------------
// Ouverture de l'archive.

static Abc::ErrorHandler::Policy convertis_police_erreur(eAbcPoliceErreur notre_police)
{
    switch (notre_police) {
        case eAbcPoliceErreur::SILENCIEUSE:
        {
            return Abc::ErrorHandler::Policy::kQuietNoopPolicy;
        }
        case eAbcPoliceErreur::BRUYANTE:
        {
            return Abc::ErrorHandler::Policy::kNoisyNoopPolicy;
        }
        case eAbcPoliceErreur::LANCE_EXCEPTION:
        {
            return Abc::ErrorHandler::Policy::kThrowPolicy;
        }
    }

    /* Par défaut et par symétrie avec Alembic, retourne kThrowPolicy en cas de valeur invalide. */
    return Abc::ErrorHandler::Policy::kThrowPolicy;
}

static AbcCoreFactory::IFactory::OgawaReadStrategy convertis_strategie_ogawa(
    eAbcStrategieLectureOgawa notre_strategie)
{
    switch (notre_strategie) {
        case eAbcStrategieLectureOgawa::FLUX_DE_FICHIERS:
        {
            return AbcCoreFactory::IFactory::OgawaReadStrategy::kFileStreams;
        }
        case eAbcStrategieLectureOgawa::FICHIERS_MAPPES_MEMOIRE:
        {
            return AbcCoreFactory::IFactory::OgawaReadStrategy::kMemoryMappedFiles;
        }
    }

    /* Par défaut et par symétrie avec Alembic, retourne kMemoryMappedFiles en cas de valeur
     * invalide. */
    return AbcCoreFactory::IFactory::OgawaReadStrategy::kMemoryMappedFiles;
}

static void renseigne_politique_erreur(ContexteOuvertureArchive *ctx,
                                       AbcCoreFactory::IFactory &ifactory)
{
    if (ctx->police_erreur) {
        const eAbcPoliceErreur police = ctx->police_erreur(ctx);
        ifactory.setPolicy(convertis_police_erreur(police));
    }
}

static void renseigne_strategie_ogawa(ContexteOuvertureArchive *ctx,
                                      AbcCoreFactory::IFactory &ifactory)
{
    if (ctx->nombre_de_flux_ogawa_desires) {
        const int nombre_de_flux = ctx->nombre_de_flux_ogawa_desires(ctx);
        ifactory.setOgawaNumStreams(static_cast<size_t>(nombre_de_flux));
    }

    if (ctx->strategie_lecture_ogawa) {
        const eAbcStrategieLectureOgawa strategie = ctx->strategie_lecture_ogawa(ctx);
        ifactory.setOgawaReadStrategy(convertis_strategie_ogawa(strategie));
    }
}

ArchiveCache *cree_archive(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx)
{
    const auto nombre_de_chemins = static_cast<size_t>(ctx->nombre_de_chemins(ctx));

    if (nombre_de_chemins == 0) {
        if (ctx->erreur_aucun_chemin) {
            ctx->erreur_aucun_chemin(ctx);
        }
        return nullptr;
    }

    std::vector<std::string> chemins;
    for (size_t i = 0; i < nombre_de_chemins; ++i) {
        auto str_chemin = string_depuis_rappel(ctx, i, ctx->chemin);
        if (str_chemin.empty()) {
            continue;
        }
        chemins.push_back(str_chemin);
    }

    if (chemins.empty()) {
        if (ctx->erreur_aucun_chemin) {
            ctx->erreur_aucun_chemin(ctx);
        }
        return nullptr;
    }

    Alembic::AbcCoreFactory::IFactory factory;
    renseigne_politique_erreur(ctx, factory);
    renseigne_strategie_ogawa(ctx, factory);

    Abc::IArchive iarchive = factory.getArchive(chemins);

    if (!iarchive.valid()) {
        if (ctx->erreur_archive_invalide) {
            ctx->erreur_archive_invalide(ctx);
        }
        return nullptr;
    }

    auto poignee = kuri_loge<ArchiveCache>(ctx_kuri);
    poignee->archive = iarchive;
    return poignee;
}

void detruit_archive(ContexteKuri *ctx, ArchiveCache *archive)
{
    kuri_deloge(ctx, archive);
}

/* ------------------------------------------------------------------------- */
/** \nom Autrice Archive.
 * \{ */

static void initialise_metadonnées(ContexteCreationArchive *ctx, Abc::MetaData &abc_metadata)
{
    auto nom_application = string_depuis_rappel(ctx, ctx->donne_nom_application);
    if (nom_application.empty()) {
        nom_application = "unknown";
    }
    abc_metadata.set(Abc::kApplicationNameKey, nom_application);

    auto description = string_depuis_rappel(ctx, ctx->donne_description);
    if (description.empty()) {
        description = "unknown";
    }
    abc_metadata.set(Abc::kUserDescriptionKey, description);

    if (ctx->donne_frames_par_seconde) {
        auto fps = ctx->donne_frames_par_seconde(ctx);
        if (fps != 0.0f) {
            abc_metadata.set("FramesPerTimeUnit", std::to_string(fps));
        }
    }

    time_t raw_time;
    time(&raw_time);
    char buffer[128];

#if defined _WIN32 || defined _WIN64
    ctime_s(buffer, 128, &raw_time);
#else
    ctime_r(&raw_time, buffer);
#endif

    const std::size_t buffer_len = strlen(buffer);
    if (buffer_len > 0 && buffer[buffer_len - 1] == '\n') {
        buffer[buffer_len - 1] = '\0';
    }

    abc_metadata.set(Alembic::Abc::kDateWrittenKey, buffer);
}

AutriceArchive *crée_autrice_archive(ContexteKuri *ctx_kuri,
                                     ContexteCreationArchive *ctx,
                                     ContexteEcritureCache *ctx_écriture)
{
    auto str_chemin = string_depuis_rappel(ctx, ctx->donne_chemin);
    if (str_chemin.empty()) {
        if (ctx->erreur_aucun_chemin) {
            ctx->erreur_aucun_chemin(ctx);
        }
        return nullptr;
    }

    AbcCoreOgawa::WriteArchive archive_writer;
    Abc::ErrorHandler::Policy policy = Abc::ErrorHandler::kThrowPolicy;

    Abc::MetaData abc_metadata;
    initialise_metadonnées(ctx, abc_metadata);

    auto oarchive = Abc::OArchive(
        archive_writer(str_chemin, abc_metadata), Abc::kWrapExisting, policy);

    auto poignee = kuri_loge<AutriceArchive>(ctx_kuri);
    poignee->archive = oarchive;
    poignee->ctx_écriture = *ctx_écriture;
    poignee->racine = kuri_loge<RacineAutriceCache>(ctx_kuri, *poignee);
    return poignee;
}

static void écris_données(EcrivainCache *écrivain)
{
    écrivain->écris_données();

    for (auto enfant : écrivain->m_enfants) {
        écris_données(enfant);
    }
}

void écris_données(AutriceArchive *autrice)
{
    écris_données(autrice->racine);
}

static void détruit_écrivain(ContexteKuri *ctx, EcrivainCache *écrivain)
{
    for (auto enfant : écrivain->m_enfants) {
        détruit_écrivain(ctx, enfant);
    }

    AbcKuri::detruit_ecrivain(ctx, écrivain);
}

void détruit_autrice(ContexteKuri *ctx, AutriceArchive *archive)
{
    détruit_écrivain(ctx, archive->racine);
    kuri_deloge(ctx, archive);
}

/** \} */

// --------------------------------------------------------------
// Traversé de l'archive.

static void passe_nom_via_contexte(ContexteTraverseArchive *ctx, const std::string &nom)
{
    if (!ctx->extrait_nom_courant) {
        return;
    }
    ctx->extrait_nom_courant(ctx, nom.c_str(), nom.size());
}

static void abc_traverse_hierarchie(ContexteTraverseArchive *ctx,
                                    Abc::IObject parent,
                                    const Abc::ObjectHeader &header)
{
    if (ctx->annule && ctx->annule(ctx)) {
        return;
    }

    Abc::IObject next_object;

    if (AbcGeom::IXform::matches(header)) {
        AbcGeom::IXform xform(parent, header.getName());
        passe_nom_via_contexte(ctx, xform.getFullName());
        next_object = xform;
    }
    else if (AbcGeom::ISubD::matches(header)) {
        AbcGeom::ISubD subd(parent, header.getName());
        passe_nom_via_contexte(ctx, subd.getFullName());
        next_object = subd;
    }
    else if (AbcGeom::IPolyMesh::matches(header)) {
        AbcGeom::IPolyMesh mesh(parent, header.getName());
        passe_nom_via_contexte(ctx, mesh.getFullName());
        next_object = mesh;
    }
    else if (AbcGeom::ICurves::matches(header)) {
        AbcGeom::ICurves curves(parent, header.getName());
        passe_nom_via_contexte(ctx, curves.getFullName());
        next_object = curves;
    }
    else if (AbcGeom::IFaceSet::matches(header)) {
        // ignore the face set, it will be read along with the data
    }
    else if (AbcGeom::IPoints::matches(header)) {
        AbcGeom::IPoints points(parent, header.getName());
        passe_nom_via_contexte(ctx, points.getFullName());
        next_object = points;
    }
    else if (AbcGeom::INuPatch::matches(header)) {
        AbcGeom::INuPatch nurbs(parent, header.getName());
        passe_nom_via_contexte(ctx, nurbs.getFullName());
        next_object = nurbs;
    }
    else if (AbcGeom::ILight::matches(header)) {
        AbcGeom::ILight lumiere(parent, header.getName());
        passe_nom_via_contexte(ctx, lumiere.getFullName());
        next_object = lumiere;
    }
    else if (AbcGeom::ICamera::matches(header)) {
        AbcGeom::ICamera camera(parent, header.getName());
        passe_nom_via_contexte(ctx, camera.getFullName());
        next_object = camera;
    }
    else if (AbcMaterial::IMaterial::matches(header)) {
        AbcMaterial::IMaterial materiau(parent, header.getName());
        passe_nom_via_contexte(ctx, materiau.getFullName());
        next_object = materiau;
    }
    else {
        next_object = parent.getChild(header.getName());

        if (next_object.isInstanceRoot()) {
            passe_nom_via_contexte(ctx, next_object.getFullName());
        }
    }

    if (!next_object.valid()) {
        return;
    }

    for (size_t i = 0; i < next_object.getNumChildren(); ++i) {
        abc_traverse_hierarchie(ctx, next_object, next_object.getChildHeader(i));
    }
}

void traverse_archive(ContexteKuri * /*ctx_kuri*/,
                      ArchiveCache *archive,
                      ContexteTraverseArchive *ctx)
{
    if (!archive) {
        return;
    }

    const auto &top = archive->racine_lecture();

    if (!top.valid()) {
        return;
    }

    for (size_t i = 0; i < top.getNumChildren(); ++i) {
        abc_traverse_hierarchie(ctx, top, top.getChildHeader(i));
    }
}

}  // namespace AbcKuri
