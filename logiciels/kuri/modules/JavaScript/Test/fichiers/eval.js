test("eval() doit pouvoir déclarer dans l'environnement", function () {
    eval("x = 3");
    vérifie_égalité(x, 3);
});

test("eval() doit pouvoir accéder à l'environnement", function () {
    vérifie_égalité(foo(2), 5);
});

function foo(y) {
    var a = 3;
    eval("a += y;");
    return a
}

test("eval() doit retourner la valeur évaluée", function () {
    vérifie_égalité(eval("1 + 2;"), 3);
});
