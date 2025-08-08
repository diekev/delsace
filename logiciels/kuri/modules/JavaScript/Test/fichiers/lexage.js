test("les opérateurs sont proprement lexés même si les espaces sont manquants", function () {
    var a=-1;
    vérifie_égalité(a, -1);
    var b =a>=-1;
    vérifie_égalité(b, true);
});

test("les caractères échappées ne compte pas comme des caractères finaux", function() {
    var s = "*([^\\]'\"]*?)";
    var t = "\\";
    var s = '*([^\\]\'"]*?)';
    var t = '\\';
});

test("les identifiants peuvent avoir '_' et '$' au début et au milieu", function() {
    var _ = "*([^\\]'\"]*?)";
    var $ = "\\";
    var _$ = '*([^\\]\'"]*?)';
    var x$3_v1 = '\\';
});

test("'?.5' n'est pas un opérateur '?.' mais '? .5' suivi d'un ':'", function() {
    const t = true;
    const v = t?.5:.25;
    vérifie_égalité(v, 0.5);
});

test("lexage de /*/*/ (/ commenté)", function () {
    const code = 47/*/*/;
    vérifie_égalité(code, 47);
});
