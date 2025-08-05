test("'...' peut être utilisé dans la construction d'un array", function () {
    const parts = ["shoulders", "knees"];
    const lyrics = ["head", ...parts, "and", "toes"];

    vérifie_égalité(lyrics.length, 5);
    vérifie_égalité(lyrics[0], "head");
    vérifie_égalité(lyrics[1], "shoulders");
    vérifie_égalité(lyrics[2], "knees");
    vérifie_égalité(lyrics[3], "and");
    vérifie_égalité(lyrics[4], "toes");
});

test("'...' n'ajoute rien si nous développons un array vide", function () {
    let isSummer = false;
    let fruits = ["apple", "banana", ...(isSummer ? ["watermelon"] : [])];

    vérifie_égalité(fruits.length, 2);
    vérifie_égalité(fruits[0], "apple");
    vérifie_égalité(fruits[1], "banana");

    isSummer = true;
    fruits = ["apple", "banana", ...(isSummer ? ["watermelon"] : [])];

    vérifie_égalité(fruits.length, 3);
    vérifie_égalité(fruits[0], "apple");
    vérifie_égalité(fruits[1], "banana");
    vérifie_égalité(fruits[2], "watermelon");
});

test("'...' crée une copie superficielle d'un array", function () {
    const arr = [1, 2, 3];
    const arr2 = [...arr]; // like arr.slice()

    vérifie_égalité(arr2.length, 3);
    vérifie_égalité(arr2[0], 1);
    vérifie_égalité(arr2[1], 2);
    vérifie_égalité(arr2[2], 3);

    arr2.push(4);
    // arr2 becomes [1, 2, 3, 4]

    vérifie_égalité(arr2.length, 4);
    vérifie_égalité(arr2[3], 4);

    // arr remains unaffected
    vérifie_égalité(arr.length, 3);
    vérifie_égalité(arr[0], 1);
    vérifie_égalité(arr[1], 2);
    vérifie_égalité(arr[2], 3);
});

test("Les ArrayLiteral peuvent recevoir plusieurs spread", function () {
    const arr1 = [0, 1, 2];
    const arr2 = [3, 4, 5];

    let arr3 = [...arr1, ...arr2];
    // arr3 is [0, 1, 2, 3, 4, 5]
    vérifie_égalité(arr3.length, 6);
    vérifie_égalité(arr3[0], 0);
    vérifie_égalité(arr3[1], 1);
    vérifie_égalité(arr3[2], 2);
    vérifie_égalité(arr3[3], 3);
    vérifie_égalité(arr3[4], 4);
    vérifie_égalité(arr3[5], 5);

    arr3 = [...arr2, ...arr1];
    // arr3 is [0, 1, 2, 3, 4, 5]
    vérifie_égalité(arr3.length, 6);
    vérifie_égalité(arr3[0], 3);
    vérifie_égalité(arr3[1], 4);
    vérifie_égalité(arr3[2], 5);
    vérifie_égalité(arr3[3], 0);
    vérifie_égalité(arr3[4], 1);
    vérifie_égalité(arr3[5], 2);
});