function foo() {
    var x = {};
    rm();
    return x;
}

test("le ramasse miette ne détruit pas des objets encore utiles", function () {
    var x = foo();
    vérifie_égalité(typeof x, "object");
})
