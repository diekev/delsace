/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "compression.h"

#include <cassert>
#include <zlib.h>

extern "C" {

/* Compresse des données depuis un flux d'entrée vers un flux de sortie. */
int ZLIB_compresse(ContexteCompression *ctx,
                   FluxCompression *flux_entree,
                   FluxCompression *flux_sortie,
                   int niveau)
{
    const auto chunk = ctx->taille_stockage_temporaire(ctx);

    /* Utilisé pour les codes de retour de Zlib. */
    int ret;
    /* Garde trace de l'état du flush de Zlib, soit aucun flush, ou flush jusqu'à la fin du flux
     * d'entrée. */
    int flush;
    /* Quantité de données renvoiée par deflate. */
    unsigned have;
    /* Passe des informations entre les fonctions de Zlib. */
    z_stream strm;
    unsigned char *in = ctx->cree_stockage_temporaire(ctx, chunk);
    unsigned char *out = ctx->cree_stockage_temporaire(ctx, chunk);

    /* Alloue l'état de deflate. */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm, niveau);
    if (ret != Z_OK) {
        return ret;
    }

    /* Compresse jusqu'à la fin du flux d'entrée. */
    do {
        strm.avail_in = flux_entree->lis(flux_entree, chunk, in);
        strm.next_in = in;

        if (flux_entree->possede_erreur(flux_entree)) {
            (void)deflateEnd(&strm);
            return -1;
        }

        flush = (flux_entree->fin_de_flux(flux_entree)) ? Z_FINISH : Z_NO_FLUSH;

        do {
            strm.avail_out = chunk;
            strm.next_out = out;

            ret = deflate(&strm, flush);   /* Pas de mauvaise valeur de retour. */
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */

            have = chunk - strm.avail_out;

            if (flux_sortie->ecris(flux_sortie, have, out) != have ||
                flux_sortie->possede_erreur(flux_sortie)) {
                (void)deflateEnd(&strm);
                return -1;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0); /* all input will be used */
    } while (flush != Z_FINISH);

    /* clean up and return */
    (void)deflateEnd(&strm);
    ctx->detruit_stockage_temporaire(ctx, in, chunk);
    ctx->detruit_stockage_temporaire(ctx, out, chunk);
    return 0;
}

int ZLIB_decompresse(ContexteCompression *ctx,
                     FluxCompression *flux_entree,
                     FluxCompression *flux_sortie)
{
    const auto chunk = ctx->taille_stockage_temporaire(ctx);

    /* Utilisé pour les codes de retour de Zlib. */
    int ret;
    /* Quantité de données renvoiée par deflate. */
    unsigned have;
    /* Passe des informations entre les fonctions de Zlib. */
    z_stream strm;
    unsigned char *in = ctx->cree_stockage_temporaire(ctx, chunk);
    unsigned char *out = ctx->cree_stockage_temporaire(ctx, chunk);

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
        return ret;

    /* Compresse jusqu'à la fin du flux d'entrée. */
    do {
        strm.avail_in = flux_entree->lis(flux_entree, chunk, in);
        strm.next_in = in;

        if (flux_entree->possede_erreur(flux_entree)) {
            (void)inflateEnd(&strm);
            return -1;
        }

        if (strm.avail_in == 0) {
            break;
        }
        /* run inflate() on input until output buffer not full */
        do {
            strm.avail_out = chunk;
            strm.next_out = out;

            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR); /* state not clobbered */
            switch (ret) {
                case Z_NEED_DICT:
                    ret = Z_DATA_ERROR; /* and fall through */
                    [[fallthrough]];
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    (void)inflateEnd(&strm);
                    return ret;
            }

            have = chunk - strm.avail_out;
            if (flux_sortie->ecris(flux_sortie, have, out) != have ||
                flux_sortie->possede_erreur(flux_sortie)) {
                (void)inflateEnd(&strm);
                return -1;
            }
        } while (strm.avail_out == 0); /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    ctx->detruit_stockage_temporaire(ctx, in, chunk);
    ctx->detruit_stockage_temporaire(ctx, out, chunk);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
}
