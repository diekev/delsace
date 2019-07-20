namespace v1 {

Carreau *trouve_par_UV(const dls::tableau<Carreau> &carreaux, int u, int v)
{
	for (const auto &carreau : carreaux) {
		if (carreau.u == u && carreau.v == v) {
			return &carreau;
		}
	}

	return nullptr;
}

struct Carreau {
	matrice<vec4f> donnees;
	int hauteur;
	int largeur;
	int u;
	int v;
	dls::chaine chemin;
};

struct Effets {

};

struct Calque {
	dls::tableau<Carreau> carreaux;
	dls::tableau<Effets> effets;
	int mode_fusion;
	dls::chaine nom;
};

struct Peinture {
	dls::tableau<Calque> calques;
};

}

namespace mudbox {

struct Layer {
	bool visible;
	float opacity;
	int blend_mode;

	/* parametres création */
	dls::chaine name;
	int size;
	int save_as; // TIFF, PNG, etc...
	///int channel; // diffuse, specular, bump...

	Layer *mask;
};

enum ChannelType {
	DIFFUSE,
	SPECULAR,
	GLOSS,
	INCANDESCENCE,
	BUMP_MAP,
	NORMAL_MAP,
	REFLECTION_MAP
};

struct Channel { // diffuse, specular, bump, etc...
	dls::tableau<Layer> layers;
	bool visible;
};

struct X {
	dls::tableau<Channel> channels;
};

struct PaintingData {
	Layer *paint_layer;
};

struct Brush {
	vec4f color;
	bool mirror;
	float size;
	float strength;
};

}

namespace kanba {

enum ModeFusion {
	NORMAL,
	AJOUTER,
	SOUSTRAIRE,
	MULTIPLIER,
	DIVISER,
};

enum TypeCalque {
	DIFFUS,
	SPECULARITE,
	BRILLANCE,
	INCANDESCENCE,
	RELIEF,
	NORMAL,
	REFLECTION
};

enum TypePixel {
	FLOAT,
	FLOAT3,
	FLOAT4,
};

struct Calque {
	float *donnees;
	ModeFusion mode_fusion;
	float opacite;
	dls::chaine nom;
	bool visible;
};

struct DonneesCalques {
	dls::tableau<Calque *> calques_diffus; // FLOAT4
	dls::tableau<Calque *> calques_specularite; // FLOAT
	dls::tableau<Calque *> calques_brillance; // FLOAT
	dls::tableau<Calque *> calques_incandescence; // FLOAT
	dls::tableau<Calque *> calques_relief; // FLOAT
	dls::tableau<Calque *> calques_normal; // FLOAT3
	dls::tableau<Calque *> calques_reflection; // FLOAT

	float *diffusion_finale;
	float *specularite_finale;
	float *brillance_finale;
	float *incandescence_finale;
	float *relief_final;
	float *normal_final;
	float *reflection_finale;
};

struct Maillage {
	Calque *calque_actif;

	DonneesCalques *donnees_calques;
};

struct Kanba {
	Maillage *maillage_actif;
};

}
