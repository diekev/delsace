/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "../alembic.h"
#include "../alembic_types.h"

struct ContexteKuri;
struct LectriceCache;

struct EcrivainCache {
    EcrivainCache *m_parent = nullptr;
    std::vector<EcrivainCache *> m_enfants{};

    template <typename T>
    static EcrivainCache *cree(ContexteKuri *ctx,
                               AutriceArchive *autrice,
                               EcrivainCache *parent,
                               void *données,
                               const std::string &nom);

    template <typename TypeObjetAlembic>
    bool est_un() const
    {
        return TypeObjetAlembic::matches(oobject().getHeader());
    }

    virtual ~EcrivainCache() = default;

    virtual Alembic::AbcGeom::OObject oobject() = 0;
    virtual Alembic::AbcGeom::OObject oobject() const = 0;

    virtual void écris_données() = 0;

  protected:
    void définit_parent(EcrivainCache *parent)
    {
        m_parent = parent;
        parent->m_enfants.push_back(this);
    }
};

/* ÉcrivainCache factice pour représenter la racine de l'archive. */
class RacineAutriceCache final : public EcrivainCache {
    AutriceArchive *m_autrice = nullptr;

  public:
    explicit RacineAutriceCache(AutriceArchive &autrice);

    void écris_données() override
    {
        /* Rien à faire pour la racine. */
    }

    Alembic::AbcGeom::OObject oobject() override;

    Alembic::AbcGeom::OObject oobject() const override;
};

namespace AbcKuri {

EcrivainCache *cree_ecrivain_cache_depuis_ref(ContexteKuri *ctx,
                                              AutriceArchive *autrice,
                                              LectriceCache *lectrice,
                                              EcrivainCache *parent,
                                              void *données);

EcrivainCache *cree_ecrivain_cache(ContexteKuri *ctx,
                                   AutriceArchive *autrice,
                                   EcrivainCache *parent,
                                   const char *nom,
                                   uint64_t taille_nom,
                                   void *données,
                                   eTypeObjetAbc type_objet);

EcrivainCache *crée_instance(ContexteKuri *ctx,
                             AutriceArchive *autrice,
                             EcrivainCache *parent,
                             EcrivainCache *origine,
                             const char *nom,
                             uint64_t taille_nom);

void detruit_ecrivain(ContexteKuri *ctx_kuri, EcrivainCache *ecrivain);

}  // namespace AbcKuri
