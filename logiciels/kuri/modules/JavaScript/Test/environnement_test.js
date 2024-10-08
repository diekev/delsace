function vérifie(p) {
    if (p == false) {
        throw "Échec de l'assertion";
    }
}

function vérifie_égalité(a, b) {
    if (a === b) {
        return
    }

    throw a + " est différent de " + b;
}

function test(cas, rappel) {
    try {
        rappel()
    }
    catch (e) {
        throw cas + "\n    " + e;
    }
}
