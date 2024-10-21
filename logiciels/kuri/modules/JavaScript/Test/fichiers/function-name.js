test("fonction déclarée avec un nom", function () {
    function bar() { }
    vérifieQue(bar.name).doitÊtre("bar");
    vérifieQue((bar.name = "baz")).doitÊtre("baz");
    vérifieQue(bar.name).doitÊtre("bar");
});

test("fonction anonyme assignée à une variable", function () {
    let foo = function () { }
    vérifieQue(foo.name).doitÊtre("foo");
    vérifieQue((foo.name = "bar")).doitÊtre("bar");
    vérifieQue(foo.name).doitÊtre("foo");

    let a, b;
    a = b = function () { }
    vérifieQue(a.name).doitÊtre("b");
    vérifieQue(b.name).doitÊtre("b");
});

test("fonction nommée assignée à une variable", function () {
    let foo = function bar() { }
    vérifieQue(foo.name).doitÊtre("bar");
    vérifieQue((foo.name = "baz")).doitÊtre("baz");
    vérifieQue(foo.name).doitÊtre("bar");

    foo = function baz() { }
    vérifieQue(foo.name).doitÊtre("baz");
    vérifieQue((foo.name = "bar")).doitÊtre("bar");
    vérifieQue(foo.name).doitÊtre("baz");
});

test("fonctions nommées dans un tableau assigné à une variable", function () {
    const arr = [function foo() { }, function bar() { }, function baz() { }]
    vérifieQue(arr[0].name).doitÊtre("foo");
    vérifieQue(arr[1].name).doitÊtre("bar");
    vérifieQue(arr[2].name).doitÊtre("baz");
});

test("fonctions anonymes dans un tableau assigné à une variable", function () {
    const arr = [function () { }, function () { }, function () { }]
    vérifieQue(arr[0].name).doitÊtre("");
    vérifieQue(arr[1].name).doitÊtre("");
    vérifieQue(arr[2].name).doitÊtre("");
});

test("&&= assigne un nom aux fonctions anonymes", function () {
    let b = 1;
    b &&= function () { };
    vérifieQue(b.name).doitÊtre("b");
});

test("||= assigne un nom aux fonctions anonymes", function () {
    let b = 0;
    b ||= function () { };
    vérifieQue(b.name).doitÊtre("b");
});

test("??= assigne un nom aux fonctions anonymes", function () {
    let b = void 0;
    b ??= function () { };
    vérifieQue(b.name).doitÊtre("b");
});

// À FAIRE : fonctions dans les objets

test("fonction arrow assignée à une variable", function () {
    let foo = () => { }
    vérifieQue(foo.name).doitÊtre("foo");
    vérifieQue((foo.name = "bar")).doitÊtre("bar");
    vérifieQue(foo.name).doitÊtre("foo");

    let a, b;
    a = b = () => { }
    vérifieQue(a.name).doitÊtre("b");
    vérifieQue(b.name).doitÊtre("b");
});

test("fonctions arrow dans un tableau assigné à une variable", function () {
    const arr = [() => { }, () => { }, () => { }]
    vérifieQue(arr[0].name).doitÊtre("");
    vérifieQue(arr[1].name).doitÊtre("");
    vérifieQue(arr[2].name).doitÊtre("");
});

test("&&= assigne un nom aux fonctions arrow", function () {
    let b = 1;
    b &&= () => { };
    vérifieQue(b.name).doitÊtre("b");
});

test("||= assigne un nom aux fonctions arrow", function () {
    let b = 0;
    b ||= () => { };
    vérifieQue(b.name).doitÊtre("b");
});

test("??= assigne un nom aux fonctions arrow", function () {
    let b = void 0;
    b ??= () => { };
    vérifieQue(b.name).doitÊtre("b");
});
