test("array-literal", function () {
    var a = [1, 2, 3];
    vérifie_égalité(a.length, 3)
    vérifie_égalité(a.toString(), "1.0,2.0,3.0")

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

test("array-constructor", function () {
    var b = Array(16);
    vérifie_égalité(b.length, 16)
    vérifie_égalité(b.toString(), ",,,,,,,,,,,,,,,")
});
