test("a % b", function () {
    vérifieQue(0 % 3).doitÊtre(0);
    vérifieQue(1 % 3).doitÊtre(1);
    vérifieQue(2 % 3).doitÊtre(2);
    vérifieQue(3 % 3).doitÊtre(0);
    vérifieQue(4 % 3).doitÊtre(1);

    vérifieQue(13 % 5).doitÊtre(3);
    vérifieQue(-13 % 5).doitÊtre(-3);
    vérifieQue(4 % 2).doitÊtre(0.0);
    vérifieQue(-4 % 2).doitÊtre(-0.0);
});

test("a %= b", function () {
    let a = 3;
    vérifieQue(a %= 2).doitÊtre(1);
    vérifieQue(a %= NaN).doitÊtreNaN();
    vérifieQue(a %= "Hello").doitÊtreNaN();
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

test("a >>> b", function () {
    const a = 5; //  00000000000000000000000000000101
    const b = 2; //  00000000000000000000000000000010
    const c = -5; //  11111111111111111111111111111011

    vérifie_égalité(a >>> b, 1); //  00000000000000000000000000000001
    vérifie_égalité(c >>> b, 1073741822); //  00111111111111111111111111111110
});

test("a >>>= b", function () {
    let a = 5; //  00000000000000000000000000000101
    a >>>= 2; //  00000000000000000000000000000001
    vérifie_égalité(a, 1);

    let b = -5; // -00000000000000000000000000000101
    b >>>= 2; //  00111111111111111111111111111110
    vérifie_égalité(b, 1073741822);
});

test("a >> b", function () {
    const a = 5; //  00000000000000000000000000000101
    const b = 2; //  00000000000000000000000000000010
    const c = -5; //  11111111111111111111111111111011
    vérifie_égalité(a >> b, 1); //  00000000000000000000000000000001
    vérifie_égalité(c >> b, -2); //  11111111111111111111111111111110
});

test("a >>= b", function () {
    let a = 5; //  00000000000000000000000000000101
    a >>= 2; //  00000000000000000000000000000001
    vérifie_égalité(a, 1);

    let b = -5; //  11111111111111111111111111111011
    b >>= 2; //  11111111111111111111111111111110
    vérifie_égalité(b, -2);
});

test("a << b", function () {
    const a = 5; // 00000000000000000000000000000101
    const b = 2; // 00000000000000000000000000000010
    vérifie_égalité(a << b, 20);
});

test("a <<= b", function () {
    let a = 5; // 00000000000000000000000000000101
    a <<= 2; // 00000000000000000000000000010100
    vérifie_égalité(a, 20);
});
