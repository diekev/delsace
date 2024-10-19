test("modulo", function () {
    vérifie_égalité(0 % 3, 0);
    vérifie_égalité(1 % 3, 1);
    vérifie_égalité(2 % 3, 2);
    vérifie_égalité(3 % 3, 0);
    vérifie_égalité(4 % 3, 1);
});

test("a & b", function () {
    let a = 3;
    let b = 5;
    vérifie_égalité(a & b, 1);

    vérifie_égalité(0 & 0, 0);
    vérifie_égalité(0 & 1, 0);
    vérifie_égalité(1 & 0, 0);
    vérifie_égalité(1 & 1, 1);
});

test("a &= b a le même résultat que a & b", function () {
    let a = 3;
    let b = 5;
    let c = a & b;
    a &= b
    vérifie_égalité(a, c);
});

test("a ^ b", function () {
    let a = 3;
    let b = 5;
    vérifie_égalité(a ^ b, 6);

    vérifie_égalité(0 ^ 0, 0);
    vérifie_égalité(0 ^ 1, 1);
    vérifie_égalité(1 ^ 0, 1);
    vérifie_égalité(1 ^ 1, 0);
});

test("a ^= b a le même résultat que a ^ b", function () {
    let a = 3;
    let b = 5;
    let c = a ^ b;
    a ^= b
    vérifie_égalité(a, c);
});

test("a | b", function () {
    let a = 3;
    let b = 5;
    vérifie_égalité(a | b, 7);

    vérifie_égalité(0 | 0, 0);
    vérifie_égalité(0 | 1, 1);
    vérifie_égalité(1 | 0, 1);
    vérifie_égalité(1 | 1, 1);
});

test("a |= b a le même résultat que a | b", function () {
    let a = 3;
    let b = 5;
    let c = a | b;
    a |= b
    vérifie_égalité(a, c);
});
