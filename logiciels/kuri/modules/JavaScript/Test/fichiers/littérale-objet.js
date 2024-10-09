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
