// https://blog.kevinchisholm.com/javascript/javascript-wat/

test("wat-arithmérique-string", function () {
    vérifie_égalité('5' - 3.0, 2.0);
    vérifie_égalité('5' + 3.0, "53.0");
    vérifie_égalité('5' - '4', 1.0);
    vérifie_égalité('5' + + '5', "55.0");
    vérifie_égalité('b' + 'a' + + 'a' + 'a', "banana");
    vérifie_égalité('<scrip' + !0, "<scriptrue");
});

// test("wat-arithmétique-object", function () {
//     assert([] + [] == "");
//     assert([] + {} == "[object Object]");
//     assert({} + [] == 0.0);
//     assert({} + {} == Nan);
// });

test("wat-array", function () {
    vérifie_égalité(Array(16).toString(), ",,,,,,,,,,,,,,,");
    vérifie_égalité(Array(16).join("wat"), "watwatwatwatwatwatwatwatwatwatwatwatwatwatwat")
    vérifie_égalité(Array(16).join("wat" + 1), "wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0wat1.0")
    vérifie_égalité(Array(16).join("wat" - 1) + " Batman!", "nannannannannannannannannannannannannannannan Batman!")
});
