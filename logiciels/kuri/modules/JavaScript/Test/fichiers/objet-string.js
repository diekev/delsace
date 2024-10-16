test("properties of the String constructor", function () {
    vérifie_égalité(String.length, 1)
    vérifie_égalité(String.name, "String")
});

test("properties of the String prototype", function () {
    vérifie_égalité(String.prototype.constructor, String)
});

test("String.prototype.indexOf", function () {
    var str = "Blue Whale";
    vérifie_égalité(str.indexOf("Blue"), 0);
    vérifie_égalité(str.indexOf("Blute"), -1);
    vérifie_égalité(str.indexOf("Whale", 0), 5);
    vérifie_égalité(str.indexOf("Whale", 5), 5);
    vérifie_égalité(str.indexOf(""), 0);
    vérifie_égalité(str.indexOf("", 9), 9);
    vérifie_égalité(str.indexOf("", 10), 10);
    vérifie_égalité(str.indexOf("", 11), 10);
});
