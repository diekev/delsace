function sans_paramètres() {
    return (1 + 2) + 3;
}

c = 1
function accède_globale() {
    var a = 5;
    var b = 7;
    return (a + b) + c;
}

function avec_paramètres(a, b) {
}

test("fonction peut ne pas avoir de paramètres", function () {
    vérifie_égalité(sans_paramètres(), 6);
});

test("une fonction peut accéder à une variable globale", function () {
    vérifie_égalité(accède_globale(), 13);
});

test("les fonctions doivent avoir une propriété 'length'", function () {
    vérifie_égalité(accède_globale.length, 0)
    vérifie_égalité(sans_paramètres.length, 0)
    vérifie_égalité(avec_paramètres.length, 2)
});

test("une fonction peut être définie dans une autre", function () {
    function locale() {
        return 17;
    }

    vérifie_égalité(locale(), 17.0)
});

test("les fonctions ont une propriété 'prototype'", function () {
    function locale() {
        return 17;
    }
    vérifie(locale.hasOwnProperty("prototype"))
    vérifie(locale.prototype !== null)
});

test("le constructeur des prototypes des fonctions est la fonction", function () {
    function locale() {
        return 17;
    }
    vérifie(locale.prototype.hasOwnProperty("constructor"))
    vérifie_égalité(locale.prototype.constructor, locale)
});

test("les paramètres manquants sont initialisés à 'undefined'", function () {
    function locale(x) {
        if (x) throw "erreur"
        return 17;
    }
    vérifie_égalité(locale(), 17.0)
});

test("fonction arrow sans paramètres", function () {
    var a = () => { return 5; }
    var b = () => 6;

    vérifieQue(a()).doitÊtre(5)
    vérifieQue(b()).doitÊtre(6)
});

test("fonction arrow un paramètre sans parenthèse", function () {
    var a = x => { return x; }
    var b = x => x;

    vérifieQue(a(5)).doitÊtre(5)
    vérifieQue(b(6)).doitÊtre(6)
});

test("fonction arrow plusieurs paramètres", function () {
    var a = (x, y, z) => { return x + y + z; }
    var b = (x, y, z) => x + y + z;

    vérifieQue(a(5, 6, 7)).doitÊtre(18)
    vérifieQue(b(6, 7, 8)).doitÊtre(21)
});
