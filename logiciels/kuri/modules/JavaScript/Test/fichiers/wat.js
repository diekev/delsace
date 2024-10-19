// https://blog.kevinchisholm.com/javascript/javascript-wat/

test("wat-arithmérique-string", function () {
    vérifie_égalité('5' - 3.0, 2.0);
    vérifie_égalité('5' + 3.0, "53");
    vérifie_égalité('5' - '4', 1.0);
    vérifie_égalité('5' + + '5', "55");
    vérifie_égalité('b' + 'a' + + 'a' + 'a', "baNaNa");
    vérifie_égalité('<scrip' + !0, "<scriptrue");
});

test("wat-arithmétique-object", function () {
    vérifie_égalité([] + [], "");
    // vérifie_égalité([] + {}, "[object Object]");
    // vérifie_égalité({} + [], 0.0);
    // vérifie_égalité({} + {}, Nan);
});

test("wat-array", function () {
    vérifie_égalité(Array(16).toString(), ",,,,,,,,,,,,,,,");
    vérifie_égalité(Array(16).join("wat"), "watwatwatwatwatwatwatwatwatwatwatwatwatwatwat")
    vérifie_égalité(Array(16).join("wat" + 1), "wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1wat1")
    vérifie_égalité(Array(16).join("wat" - 1) + " Batman!", "NaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaN Batman!")
});
