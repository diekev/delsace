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

function vérifieQue(v) {
    var o = {
        valeur: v,
        doitÊtre: function (nv) {
            if (nv !== this.valeur) {
                // throw new ExpectationError("doitÊtre", this.valeur, nv);
                throw "ExpectationError: " + "doitÊtre" + ": expected _" + this.valeur + "_, got _" + nv + "_";
            }
        },
        doitÊtreNaN: function () {
            if (!isNaN(this.valeur)) {
                throw "ExpectationError: " + "doitÊtreNaN" + ": got _" + this.valeur + "_";
            }
        }
    }

    return o;
}

function ExpectationError(classe, expected, got) {
    this.classe = classe;
    this.expected = expected;
    this.got = got;
}

ExpectationError.prototype.toString = function () {
    return "ExpectationError: " + this.classe + ": expected _" + this.expected + "_, got _" + this.got + "_";
}
