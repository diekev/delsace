/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "alembic_archive.hh"

#include "../../InterfaceCKuri/contexte_kuri.hh"

#include "../alembic_types.h"

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

ArchiveCache *cree_archive_export(ContexteKuri *ctx_kuri, ContexteOuvertureArchive *ctx)
{
    const auto nombre_de_chemins = static_cast<size_t>(ctx->nombre_de_chemins(ctx));

    if (nombre_de_chemins == 0) {
        if (ctx->erreur_aucun_chemin) {
            ctx->erreur_aucun_chemin(ctx);
        }
        return nullptr;
    }

    auto str_chemin = string_depuis_rappel(ctx, 0, ctx->chemin);
    if (str_chemin.empty()) {
        if (ctx->erreur_aucun_chemin) {
            ctx->erreur_aucun_chemin(ctx);
        }
        return nullptr;
    }

    AbcCoreOgawa::WriteArchive archive_writer;

    // À FAIRE: métadonnées
    auto oarchive = Abc::OArchive(archive_writer(str_chemin, {}));

    auto poignee = kuri_loge<ArchiveCache>(ctx_kuri);
    poignee->archive = oarchive;
    return poignee;
}

void detruit_archive(ContexteKuri *ctx, ArchiveCache *archive)
{
    kuri_deloge(ctx, archive);
}

// --------------------------------------------------------------
// Traversé de l'archive.

static void passe_nom_via_contexte(ContexteTraverseArchive *ctx, const std::string &nom)
{
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

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, xform.getFullName());
        }

        next_object = xform;
    }
    else if (AbcGeom::ISubD::matches(header)) {
        AbcGeom::ISubD subd(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, subd.getFullName());
        }

        next_object = subd;
    }
    else if (AbcGeom::IPolyMesh::matches(header)) {
        AbcGeom::IPolyMesh mesh(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, mesh.getFullName());
        }

        next_object = mesh;
    }
    else if (AbcGeom::ICurves::matches(header)) {
        AbcGeom::ICurves curves(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, curves.getFullName());
        }

        next_object = curves;
    }
    else if (AbcGeom::IFaceSet::matches(header)) {
        // ignore the face set, it will be read along with the data
    }
    else if (AbcGeom::IPoints::matches(header)) {
        AbcGeom::IPoints points(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, points.getFullName());
        }

        next_object = points;
    }
    else if (AbcGeom::INuPatch::matches(header)) {
        AbcGeom::INuPatch nurbs(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, nurbs.getFullName());
        }

        next_object = nurbs;
    }
    else if (AbcGeom::ILight::matches(header)) {
        AbcGeom::ILight lumiere(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, lumiere.getFullName());
        }

        next_object = lumiere;
    }
    else if (AbcGeom::ICamera::matches(header)) {
        AbcGeom::ICamera camera(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, camera.getFullName());
        }

        next_object = camera;
    }
    else if (AbcMaterial::IMaterial::matches(header)) {
        AbcMaterial::IMaterial materiau(parent, header.getName());

        if (ctx->extrait_nom_courant) {
            passe_nom_via_contexte(ctx, materiau.getFullName());
        }

        next_object = materiau;
    }
    else {
        next_object = parent.getChild(header.getName());

        if (next_object.isInstanceRoot()) {
            if (ctx->extrait_nom_courant) {
                passe_nom_via_contexte(ctx, next_object.getFullName());
            }
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

    if (!archive->est_lecture()) {
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
