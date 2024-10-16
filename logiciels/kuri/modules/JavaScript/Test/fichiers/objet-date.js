test("properties of the Date constructor", function () {
    vérifie_égalité(Date.length, 7)
    vérifie_égalité(Date.name, "Date")
});

test("properties of the Date prototype", function () {
    vérifie_égalité(Date.prototype.constructor, Date)
});
