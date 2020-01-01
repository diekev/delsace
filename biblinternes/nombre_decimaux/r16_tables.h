/**
 * Fichier d'en-tête pour prédéclarer les tables de conversions pour les nombres
 * r16. Ainsi nous pouvons précompiler les tables dans un fichier objet, évitant
 * au compilateur de charger les grosses tables.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int table_r16_r32[65536];
extern unsigned short table_r32_r16[512];

#ifdef __cplusplus
}
#endif
