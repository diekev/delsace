 /* ************* bruits ************* */

void evalue_bruit_turbulence(in int bruit,in vec3 position,in float octaves,in float gain,in float lacunarite,in float amplitude,out float valeur,out vec3 derivee)
{ // A FAIRE
}

void evalue_bruit(in int bruit,in vec3 position,out float valeur,out vec3 derivee)
{ // A FAIRE
}

void bruit_ondelette(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_voronoi_f1f2(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_valeur(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_cellule(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_flux(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_fourier(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_perlin(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_simplex(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_voronoi_f1(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_voronoi_f2(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_voronoi_f3(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_voronoi_f4(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

void bruit_voronoi_cr(in int graine,in vec3 origine_bruit,in vec3 taille_bruit,in float decalage_valeur,in float echelle_valeur,in float temps,out int bruit)
{ // A FAIRE
}

/* ************* couleurs ************* */

void couleur_depuis_vec3(in vec3 valeur,out vec4 valeur_sortie)
{ // A FAIRE
    valeur_sortie = vec4(valeur, 1.0);
}

void sature(in vec4 couleur,in float luminance,in float facteur,out vec4 valeur)
{ // A FAIRE
}

void luminance(in vec4 couleur,out float valeur)
{ // A FAIRE
}

void contraste(in vec4 avant_plan,in vec4 arriere_plan,out float valeur)
{ // A FAIRE
}

void couleur_depuis_decimal(in float valeur,out vec4 valeur_sortie)
{
    valeur_sortie = vec4(valeur, valeur, valeur, 1.0);
}

void corps_noir(in float valeur,out vec4 valeur_sortie)
{ // A FAIRE
}

void longueur_onde(in float valeur,out vec4 valeur_sortie)
{ // A FAIRE
}

/* ************* images ************* */

void projection_spherique(in vec3 pos,out vec2 uv)
{ // A FAIRE
}

void echantillonne_triplan(in int image,in vec3 pos,in vec3 nor,out vec4 couleur)
{ // A FAIRE
}

void echantillonne_image(in int image,in vec2 uv,out vec4 couleur)
{ // A FAIRE
}

void projection_camera(in int camera,in vec3 pos,out vec2 uv)
{ // A FAIRE
}

void projection_cylindrique(in vec3 pos,out vec2 uv)
{ // A FAIRE
}

/* ************* vecteurs ************* */

void echantillone_sphere(out vec3 valeur)
{ // A FAIRE
    valeur = vec3(0.0);
}

void combine_vec3(in float x,in float y,in float z,out vec3 valeur)
{
    valeur = vec3(x, y, z);
}

void alea(in float min,in float max,out float valeur)
{ // A FAIRE
    valeur = 0.0;
}

void vec3_depuis_couleur(in vec4 valeur,out vec3 valeur_sortie)
{
    valeur_sortie = valeur.xyz;
}

void separe_vec3(in vec3 valeur,out float x,out float y,out float z)
{
    x = valeur.x;
    y = valeur.y;
    z = valeur.z;
}

void combine_vec2(in float x,in float y,out vec2 valeur)
{
    valeur = vec2(x, y);
}

void produit_scalaire(in vec3 vecteur1,in vec3 vecteur2,out float valeur)
{
    valeur = dot(vecteur1, vecteur2);
}

void separe_vec2(in vec2 valeur,out float x,out float y)
{
    x = valeur.x;
    y = valeur.y;
}

void produit_croix(in vec3 vecteur1,in vec3 vecteur2,out vec3 valeur)
{
    valeur = cross(vecteur1, vecteur2);
}

void longueur(in vec3 vecteur,out float longeur)
{
    longueur = length(vecteur);
}

void fresnel(in vec3 I, in vec3 N, in float idr, out float kr)
{
	float cosi = dot(I, N);
	float eta_dehors = 1.0;
	float eta_dedans = idr;

	if (cosi > 0) {
	    float tmp = eta_dehors;
	    eta_dehors = eta_dedans;
	    eta_dedans = tmp;
	}

	float sint = eta_dehors / eta_dedans * sqrt(max(0.0, 1.0 - cosi * cosi));

	if (sint >= 1.0) {
        kr = 1.0;
		return;
	}

	float cost = sqrt(max(0.0, 1.0 - sint * sint));
	cosi = abs(cosi);
	float Rs = ((eta_dedans * cosi) - (eta_dehors * cost)) / ((eta_dedans * cosi) + (eta_dehors * cost));
	float Rp = ((eta_dehors * cosi) - (eta_dedans * cost)) / ((eta_dehors * cosi) + (eta_dedans * cost));
	kr = (Rs * Rs + Rp * Rp) / 2.0;
}

void reflechi(in vec3 entree,in vec3 normal,out vec3 valeur)
{
	normal = entree - 2.0 * dot(entree, normal) * normal;
}

void refracte(in vec3 I, in vec3 N, in float idr, out vec3 kr)
{
	float Nrefr = N;
	float cos_theta = dot(Nrefr, I);
	float eta_dehors = 1.0;
	float eta_dedans = idr;

	if (cos_theta < 0.0) {
		cos_theta = -cos_theta;
	}
	else {
		Nrefr = -N;

	    float tmp = eta_dehors;
	    eta_dehors = eta_dedans;
	    eta_dedans = tmp;
	}

	float eta = eta_dehors / eta_dedans;
	float k = 1.0 - eta * eta * (1.0 - cos_theta * cos_theta);

	if (k < 0.0) {
		kr = vec3(0.0);
		return;
	}

	kr = eta * I + (eta * cos_theta - sqrt(k)) * Nrefr;
}

void base_orthonormale(in vec3 n,out vec3 base_a,out vec3 base_b)
{
	float sign = (n.z < 0.0) ? -1.0 : 1.0;
	float a = -1.0 / (sign + n.z);
	float b = n.x * n.y * a;
	base_a = vec3(1.0 + sign * n.x * n.x * a, sign * b, -sign * n.x);
	base_b = vec3(b, sign + n.y * n.y * a, -n.y);
}

void normalise(in vec3 vecteur,out vec3 normal,out float longeur)
{
    normal = normalize(vecteur);
    longueur = length(vecteur);
}
