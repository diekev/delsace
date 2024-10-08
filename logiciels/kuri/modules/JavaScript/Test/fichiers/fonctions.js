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

test("les fonctions doivent avoir une propriété 'name'", function () {
    vérifie_égalité(accède_globale.name, "accède_globale")
    vérifie_égalité(sans_paramètres.name, "sans_paramètres")
});

test("les fonctions doivent avoir une propriété 'length'", function () {
    vérifie_égalité(accède_globale.length, 0)
    vérifie_égalité(sans_paramètres.length, 0)
    vérifie_égalité(avec_paramètres.length, 2)
});
