/* Charge modèle. */
void charge_modele()
{
	/* Vertex data (géométrie arrangée en quad). */
	/* Patch Texture (images texture puissance-de-2). */
	/* Information d'adjacence (4 voisins pour chaque patch quad). */
	/* Texel data.
	 * - Par face : charge le mip map le plus grand disponible.
	 * - En mémoire : placer les données chargées dans un tampon qui a une bordure fixe.
	 * - Bordure : assez grande pour le plus gros kernel de filtrage (1 pixel pour bilinéaire, 8 pixel pour 16x16 aniso).
	 */
}

/* Bucket et tri. */
void bucket_et_tri()
{
	/* Bucket les surfaces ptex par ratio d'aspect.
	 * - chaque ratio sera un seau
	 * - seau 1:1, seau 2:1, etc...
	 */

	 /* Dans chaque seau, tri par surface décroissante et assigne des ID
	  * - ça nous permettra de packer densément les tableaux de textures pour éviter des surfaces "vides".
	  */
}

/* Génère MipMaps. */

/* Rempli les bordures. */

/* Packes tableaux de textures. */

/* Reordonne le tampon d'index. */

/* Packes les constantes des patch. */
