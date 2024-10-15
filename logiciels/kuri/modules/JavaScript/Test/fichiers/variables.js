test("nous pouvons déclarer des variables avec 'var'", function () {
    var x = 5;
    vérifie_égalité(x, 5);
});

test("nous pouvons déclarer des variables sans initialisation", function () {
    var x;
    x = 5;
    vérifie_égalité(x, 5);
});

test("nous pouvons déclarer des variables avec 'let'", function () {
    let x = 5;
    vérifie_égalité(x, 5);
});

test("nous pouvons déclarer des variables avec 'const'", function () {
    const x = 5;
    vérifie_égalité(x, 5);
});

test("nous pouvons déclarer plusieurs variables avec un seul 'var'", function () {
    var x = 5, y = 6, z = x + y;
    vérifie_égalité(x, 5);
    vérifie_égalité(y, 6);
    vérifie_égalité(z, 11);
});

test("nous pouvons déclarer plusieurs variables avec un seul 'let'", function () {
    let x = 5, y = 6, z = x + y;
    vérifie_égalité(x, 5);
    vérifie_égalité(y, 6);
    vérifie_égalité(z, 11);
});

test("nous pouvons déclarer plusieurs variables avec un seul 'const'", function () {
    const x = 5, y = 6, z = x + y;
    vérifie_égalité(x, 5);
    vérifie_égalité(y, 6);
    vérifie_égalité(z, 11);
});

test("les variables sont hissées au début de la fonction", function () {
    var b = a;
    var a = 5;
    vérifie_égalité(a, 5);
});

test("nous pouvons assigner des fonctions anonymes à des variables", function () {
    var x = function () {
        return 5
    }

    vérifie_égalité(x.name, "x")
    vérifie_égalité(x(), 5)

    x = function () {
        return 6
    }
    vérifie_égalité(x(), 6)
});

test("nous pouvons assigner des fonctions nommées à des variables", function () {
    var x = function foo() {
        return 5
    }

    vérifie_égalité(x.name, "foo")
    vérifie_égalité(x(), 5)

    x = function bar() {
        return 6
    }
    vérifie_égalité(x.name, "bar")
    vérifie_égalité(x(), 6)
});
