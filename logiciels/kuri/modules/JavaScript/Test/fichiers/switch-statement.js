test("switch avec blocs", function () {
    var x = 5;
    var y = false;
    switch (x) {
        case 0: {
            y = "ERREUR"
            break;
        }
        case 1: {
            y = "ERREUR"
            break;
        }
        case 5: {
            y = true;
            break;
        }
    }
    vérifie_égalité(y, true);
});

test("switch sans blocs", function () {
    var x = 5;
    var y = false;
    switch (x) {
        case 0:
            y = "ERREUR"
            break;
        case 1:
            y = "ERREUR"
            break;
        case 5:
            y = true;
            break;
    }
    vérifie_égalité(y, true);
});

test("switch avec groupes", function () {
    var x = 5;
    var y = false;
    switch (x) {
        case 0:
        case 1:
            y = "ERREUR"
            break;
        case 4:
        case 5:
            y = true;
            break;
    }
    vérifie_égalité(y, true);
});

test("switch avec default au début", function () {
    var x = 5;
    var y = false;
    switch (x) {
        default:
        case 0:
        case 1:
            y = "ERREUR"
            break;
        case 4:
        case 5:
            y = true;
            break;
    }
    vérifie_égalité(y, true);
});

test("switch avec default au milieu", function () {
    var x = 5;
    var y = false;
    switch (x) {
        default:
        case 0:
        case 1:
            y = "ERREUR"
            break;
        case 4:
        case 5:
            y = true;
            break;
    }
    vérifie_égalité(y, true);
});

test("switch avec default à la fin", function () {
    var x = 5;
    var y = false;
    switch (x) {
        case 0:
        case 1:
            y = "ERREUR"
            break;
        case 4:
        case 5:
            y = true;
            break;
        default:
            y = 5.0;
            break;
    }
    vérifie_égalité(y, true);
});
