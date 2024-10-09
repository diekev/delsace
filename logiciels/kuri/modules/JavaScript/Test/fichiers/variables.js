test("nous pouvons déclarer des variables avec 'var'", function () {
    var x = 5;
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
