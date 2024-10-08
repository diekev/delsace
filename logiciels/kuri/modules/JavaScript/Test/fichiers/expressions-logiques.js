
test("&& retourne vrai que si toutes les branches retournent vrai", function () {
    vérifie_égalité(true && true && false && true, false)
    vérifie_égalité(true && true && true && true, true)
});

test("&& possède un court circuit", function () {
    vérifie_égalité(0.0 && 6.0, 0.0)
    vérifie_égalité(0.0 && variable_inconnue, 0.0)
});

test("|| possède un court circuit", function () {
    vérifie_égalité(6.0 || 0.0, 6.0)
    vérifie_égalité(6.0 || variable_inconnue, 6.0)
});

test("|| retourne vrai si au moins une branche retourne vrai", function () {
    vérifie_égalité(false || false || false || false, false)
    vérifie_égalité(true || false || false || false, true)
});
