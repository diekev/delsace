test("littérale objet sans paramètres", function () {
    var o = {};
    o.x = 5;
    vérifie_égalité(o.x, 5)
});

test("littérale objet avec paramètres", function () {
    var o = { x: 5, y: "chaine", z: false };
    vérifie_égalité(o.x, 5)
    vérifie_égalité(o.y, "chaine")
    vérifie_égalité(o.z, false)
});

test("littérale avec des chaines comme noms de propriétés", function () {
    var o = { "x": 5, "y": "chaine", "z": false };
    vérifie_égalité(o.x, 5)
    vérifie_égalité(o.y, "chaine")
    vérifie_égalité(o.z, false)
});

test("littérale avec des index comme noms de propriétés", function () {
    var o = { 1: 5, 2: "chaine", 3: false };
    vérifie_égalité(o[1], 5)
    vérifie_égalité(o[2], "chaine")
    vérifie_égalité(o[3], false)
});

test("accès des propriétés avec des expressions calculées", function () {
    var o = { 1: 5, "deux": "chaine", 3: false };
    vérifie_égalité(o[2 / 2], 5)
    vérifie_égalité(o["de" + "ux"], "chaine")
    vérifie_égalité(o[1 + 2], false)
});

test("accès présence propriétés avec l'opérateur 'in'", function () {
    var o = { 1: 5, "deux": "chaine", 3: false };
    // À FAIRE : to_string sur des nombres inclu ".0"
    // vérifie(1 in o)
    vérifie("deux" in o)
    //vérifie(3 in o)
});

test("expression littérale avec postincrément", function () {
    var i = 0;
    var o = { x: i++, y: 2 };
    vérifie_égalité(i, 1);
    vérifie_égalité(o.x, 0);
    vérifie_égalité(o.y, 2);
})

test("propriété numérique déclarée en chaine", () => {
    const o = { "123": 456 };
    vérifie_égalité(o["123"], 456);
    vérifie_égalité(o[123], 456);
});
