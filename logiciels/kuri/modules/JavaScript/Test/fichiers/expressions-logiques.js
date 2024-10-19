
test("&& retourne vrai que si toutes les branches retournent vrai", function () {
    vérifie_égalité(true && true && false && true, false)
    vérifie_égalité(true && true && true && true, true)
});

test("&& possède un court circuit", function () {
    vérifie_égalité(0.0 && 6.0, 0.0)
    vérifie_égalité(0.0 && variable_inconnue, 0.0)
});

test("&&= ne change la valeur d'une variable que si elle est 'vrai'", function () {
    let a = 1;
    a &&= 2;
    vérifie_égalité(a, 2);
});

test("&&= ne change pas la valeur d'une variable si elle est 'fausse'", function () {
    let b = 0;
    b &&= 2;
    vérifie_égalité(b, 0);
});

test("|| possède un court circuit", function () {
    vérifie_égalité(6.0 || 0.0, 6.0)
    vérifie_égalité(6.0 || variable_inconnue, 6.0)
});

test("|| retourne vrai si au moins une branche retourne vrai", function () {
    vérifie_égalité(false || false || false || false, false)
    vérifie_égalité(true || false || false || false, true)
});

test("||= ne change pas la valeur des membres qui sont 'vrai'", function () {
    const a = { duration: 50, title: '' };
    a.duration ||= 10;
    vérifie_égalité(a.duration, 50);
});

test("||= change la valeur des membres qui sont 'faux'", function () {
    const a = { duration: 50, title: '' };
    a.title ||= 'title is empty.';
    vérifie_égalité(a.title, 'title is empty.');
});

test("?? retourne la valeur non-nulle et non-undefined", function () {
    const foo = null ?? 'default string';
    vérifie_égalité(foo, 'default string')

    const baz = 0 ?? 42;
    vérifie_égalité(baz, 0)
});

test("??= peut créer des membres qui n'existent pas", function () {
    const a = { duration: 50 };
    a.speed ??= 25;
    vérifie_égalité(a.speed, 25);
});

test("??= ne remplace pas les membres qui existent", function () {
    const a = { duration: 50 };
    a.duration ??= 10;
    vérifie_égalité(a.duration, 50);
});
