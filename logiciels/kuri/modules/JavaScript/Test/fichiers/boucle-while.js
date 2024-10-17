test("while simple", function () {
    var x = 0;
    while (x < 5) {
        x += 1
    }
    vérifie_égalité(x, 5)
});

test("while avec break", function () {
    var x = 0;
    while (x < 10) {
        x += 1

        if (x == 5)
            break;
    }
    vérifie_égalité(x, 5)
});

test("while avec continue seul", function () {
    let i = 0;
    while (i++ < 5) {
        continue;
    }
    vérifie_égalité(i, 6)
})
