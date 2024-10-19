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

test("a / b", function () {
    vérifieQue(12 / 2).doitÊtre(6);
    vérifieQue(3 / 2).doitÊtre(1.5);
    vérifieQue(6 / '3').doitÊtre(2);
    vérifieQue(2 / 0).doitÊtre(Infinity);
});

test("a /= b", function () {
    let a = 3;

    a /= 2;
    vérifieQue(a).doitÊtre(1.5);

    a /= 0;
    vérifieQue(a).doitÊtre(Infinity);

    a /= 'hello';
    vérifieQue(a).doitÊtreNaN();
});

test("a * b", function () {
    vérifieQue(3 * 4).doitÊtre(12);
    vérifieQue(-3 * 4).doitÊtre(-12);
    vérifieQue('3' * 2).doitÊtre(6);
    vérifieQue('foo' * 2).doitÊtreNaN();
});

test("a *= b", function () {
    let a = 2;

    vérifieQue((a *= 3)).doitÊtre(6);
    vérifieQue((a *= 'hello')).doitÊtreNaN();
});

test("a ** b", function () {
    vérifieQue(3 ** 4).doitÊtre(81);
    vérifieQue(10 ** -2).doitÊtre(0.01);
    vérifieQue(2 ** (3 ** 2)).doitÊtre(512);
    vérifieQue((2 ** 3) ** 2).doitÊtre(64);
});

test("a **= b", function () {
    let a = 3;

    vérifieQue((a **= 2)).doitÊtre(9);
    vérifieQue((a **= 0)).doitÊtre(1);
    vérifieQue((a **= 'hello')).doitÊtreNaN();
});

test("a - b", function () {
    vérifieQue(5 - 3).doitÊtre(2);
    vérifieQue(3.5 - 5).doitÊtre(-1.5);
    vérifieQue(5 - 'hello').doitÊtreNaN();
    vérifieQue(5 - true).doitÊtre(4);
});

test("a -= b", function () {
    let a = 2;
    vérifieQue((a -= 3)).doitÊtre(-1);
    vérifieQue((a -= 'Hello')).doitÊtreNaN();
});

test("a + b", function () {
    vérifieQue(2 + 2).doitÊtre(4);
    vérifieQue(2 + true).doitÊtre(3);
    vérifieQue('hello ' + 'everyone').doitÊtre("hello everyone");
    vérifieQue(2001 + ': A Space Odyssey').doitÊtre("2001: A Space Odyssey");
});

test("a += b", function () {
    let a = 2;
    let b = 'hello';
    vérifieQue((a += 3)).doitÊtre(5); // Addition
    vérifieQue((b += ' world')).doitÊtre("hello world"); // Concatenation
});

test("a !== b", function () {
    vérifieQue(1 !== 1).doitÊtre(false);
    vérifieQue('hello' !== 'hello').doitÊtre(false);
    vérifieQue('1' !== 1).doitÊtre(true);
    vérifieQue(1 !== '1').doitÊtre(true);
    vérifieQue(0 !== false).doitÊtre(true);
    vérifieQue(false !== 0).doitÊtre(true);
});

test("a === b", function () {
    vérifieQue(1 === 1).doitÊtre(true);
    vérifieQue('hello' === 'hello').doitÊtre(true);
    vérifieQue('1' === 1).doitÊtre(false);
    vérifieQue(1 === '1').doitÊtre(false);
    vérifieQue(0 === false).doitÊtre(false);
    vérifieQue(false === 0).doitÊtre(false);
});

test("a != b", function () {
    vérifieQue(1 != 1).doitÊtre(false);
    vérifieQue('hello' != 'hello').doitÊtre(false);
    vérifieQue('1' != 1).doitÊtre(false);
    vérifieQue(1 != '1').doitÊtre(false);
    vérifieQue(0 != false).doitÊtre(false);
    vérifieQue(false != 0).doitÊtre(false);
});

test("a == b", function () {
    vérifieQue(1 == 1).doitÊtre(true);
    vérifieQue('hello' == 'hello').doitÊtre(true);
    vérifieQue('1' == 1).doitÊtre(true);
    vérifieQue(1 == '1').doitÊtre(true);
    vérifieQue(0 == false).doitÊtre(true);
    vérifieQue(false == 0).doitÊtre(true);
});

test("a instanceof b", function () {
    function Car(make, model, year) {
        this.make = make;
        this.model = model;
        this.year = year;
    }
    const auto = new Car('Honda', 'Accord', 1998);

    vérifieQue(auto instanceof Car).doitÊtre(true);
    vérifieQue(auto instanceof Object).doitÊtre(true);
    vérifieQue(auto instanceof Array).doitÊtre(false);
});
