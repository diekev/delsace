test("do while simple", function () {
    var x = 0;
    do {
        x += 1
    } while (x < 5)
    vérifie_égalité(x, 5)
});

test("do while avec break", function () {
    var x = 0;
    do {
        x += 1

        if (x == 5)
            break;
    } while (x < 10)
    vérifie_égalité(x, 5)
});

test("do while avec continue seul", function () {
    let i = 0;
    do { continue; }
    while (i++ < 5);
    vérifie_égalité(i, 6)
})
