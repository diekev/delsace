test("array-literal", function () {
    var a = [1, 2, 3];
    vérifie_égalité(a.length, 3)
    vérifie_égalité(a.toString(), "1,2,3")

    a[1] = 5;
    vérifie_égalité(a[1], 5)

    var push_result = a.push(7);
    vérifie_égalité(a.length, 4)

    vérifie_égalité(a[0], 1)
    vérifie_égalité(a[1], 5)
    vérifie_égalité(a[2], 3)
    vérifie_égalité(a[3], 7)

    vérifie_égalité(push_result, 4)
});

test("array-literal vide", function () {
    var a = [];
    vérifie_égalité(a.length, 0)
});

test("assignation array-literal vide", function () {
    var a = [];
    vérifie_égalité(a.length, 0)
    a[0] = 4.0;
    a[1] = 3.0;
    a[2] = 2.0;
    a[3] = 1.0;
    vérifie_égalité(a.length, 4)
    vérifie_égalité(a[0], 4.0)
    vérifie_égalité(a[1], 3.0)
    vérifie_égalité(a[2], 2.0)
    vérifie_égalité(a[3], 1.0)
});

test("assignation array-literal vide 2", function () {
    var a = [];
    vérifie_égalité(a.length, 0)
    a[0] = a[1] = a[2] = a[3] = 123;
    vérifie_égalité(a.length, 4)
    vérifie_égalité(a[0], 123)
    vérifie_égalité(a[1], 123)
    vérifie_égalité(a[2], 123)
    vérifie_égalité(a[3], 123)
});

test("array-constructor", function () {
    var b = Array(16);
    vérifie_égalité(b.length, 16)
    vérifie_égalité(b.toString(), ",,,,,,,,,,,,,,,")
});

test("array-constructor new", function () {
    var b = new Array(16);
    vérifie_égalité(b.length, 16)
    vérifie_égalité(b.toString(), ",,,,,,,,,,,,,,,")
});

test("array-constructor multiples valeurs", function () {
    var b = Array(0, 1, 2, 3, 4, 5);
    vérifie_égalité(b.length, 6)

    for (var i = 0; i < 6; i++) {
        vérifie_égalité(b[i], i)
    }
});

test("ajouter des éléments par index à un Array ne change pas sa taille si l'index est inférieure à la taille allouée", function () {
    var b = new Array(16);
    b[0] = 5;
    vérifie_égalité(b.length, 16);
});
