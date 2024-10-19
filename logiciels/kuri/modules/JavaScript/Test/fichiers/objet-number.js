test("properties of the Number constructor", function () {
    vérifie_égalité(Number.name, "Number")
});

test("properties of the Number prototype", function () {
    vérifie_égalité(Number.prototype.constructor, Number)
});

test("nombres littéraux", function () {
    vérifie_égalité(255, 0xff);
    vérifie_égalité(255, 0XFF);

    vérifie_égalité(255, 0xff);
    vérifie_égalité(255, 0Xff);

    vérifie_égalité(255, 0b1111_1111);
    vérifie_égalité(255, 0B1111_1111);

    vérifie_égalité(255, 0o377);
    vérifie_égalité(255, 0O377);

    // À FAIRE : la comparaison des nombres pourrait utiliser une méthode prenant en compte 
    //           les ULP.
    // vérifie_égalité(255, 0.255e3);
    vérifie_égalité(255, 255.0);
});
