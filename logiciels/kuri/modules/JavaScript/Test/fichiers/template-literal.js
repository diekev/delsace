test("template-literal sans substitution", function () {
    vérifie_égalité(`abc`, "abc");
});

test("template-literal avec échappement", function () {
    vérifie_égalité(`\``, "`");
    vérifie_égalité(`\${1}`, "${1}");
});

test("template-literal multi-ligne", function () {
    const s = `texte ligne 1
texte ligne 2`;
    vérifie_égalité(s, "texte ligne 1\ntexte ligne 2");
});

test("template-literal multi-ligne avec échappement", function () {
    const s = `texte ligne 1 \
texte ligne 2`;
    vérifie_égalité(s, "texte ligne 1 texte ligne 2");
});

test("template-literal avec substitution", function () {
    const a = 5;
    const b = 10;
    const s = `Quinze est ${a + b} et non ${2 * a + b}.`
    vérifie_égalité(s, "Quinze est 15 et non 20.");
});

test("template-literal avec objet", function () {
    function passeObjet(o) {
        return o.value.toString();
    }

    const s = `${passeObjet({ value: 53 })}%`;
    vérifie_égalité(s, "53%");
});
