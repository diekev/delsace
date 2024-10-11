test("seule la dernière expression d'une expression virgule est retournée", function () {
    function foo() {
        return 2, 3;
    }

    var x = foo();
    vérifie_égalité(x, 3);
})

test("une virgule après une assignation n'assigne pas la dernière valeur", function () {
    var x = 1;
    x = 2, 3;
    vérifie_égalité(x, 2);
})
