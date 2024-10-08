function Voiture(marque, modèle, année) {
    this.marque = marque;
    this.modèle = modèle;
    this.année = année;
}

test("new peut s'utiliser sur une fonction", function () {
    const car = new Voiture("Volkswagen", "Golf TDi", 1997);
    vérifie_égalité(car.marque, 'Volkswagen');
    vérifie_égalité(car.modèle, 'Golf TDi');
    vérifie_égalité(car.année, 1997);
});

function VoitureOpaque() { }

test("une fonction utilise comme construteur retourne un objet configurable", function () {
    var voiture1 = new VoitureOpaque();
    // var voiture2 = new VoitureOpaque();

    vérifie_égalité(voiture1.hasOwnProperty("couleur"), false);

    // À FAIRE
    // Voiture.prototype.couleur = "couleur standard";
    // vérifie_égalité(voiture1.hasOwnProperty("couleur"), false);
    // vérifie_égalité(voiture1.couleur, "couleur standard");

    voiture1.couleur = "noir";
    vérifie_égalité(voiture1.hasOwnProperty("couleur"), true);

    // vérifie_égalité(voiture1.__proto__.couleur, "couleur standard");
    // vérifie_égalité(voiture2.__proto__.couleur, "couleur standard");
    vérifie_égalité(voiture1.couleur, "noir")
    // vérifie_égalité(voiture1.couleur, "couleur standard")
});
