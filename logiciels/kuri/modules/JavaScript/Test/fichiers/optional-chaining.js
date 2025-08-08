test("l'opérateur '?.' permet d'accéder aux rubriques", function () {
    const o = { x: 123 };
    const valeur = o?.x;
    vérifie_égalité(valeur, 123);
});

test("l'opérateur '?.' permet d'accéder aux rubriques calculées", function () {
    const o = { x: 123 };
    const rubrique = "x"
    const valeur = o?.[rubrique];
    vérifie_égalité(valeur, 123);
});

test("l'opérateur '?.' court-circuite l'évaluation", function () {
    const potentiallyNullObj = null;
    let x = 0;
    const prop = potentiallyNullObj?.[x++];

    vérifie_égalité(x, 0);
});

test("accéder à des rubriques après '?.' ne lance pas d'exception", function () {
    const potentiallyNullObj = null;
    const prop = potentiallyNullObj?.a.b;
});

test("'?.' peut être à gauche d'une expression d'appel", function () {
    const o = { donne_valeur: function () { return 456; } }
    const valeur = o?.donne_valeur();
    vérifie_égalité(valeur, 456);
});

test("'?.' peut être à gauche d'une expression d'appel indéfini", function () {
    const o = {}
    const valeur = o?.donne_valeur();
});
