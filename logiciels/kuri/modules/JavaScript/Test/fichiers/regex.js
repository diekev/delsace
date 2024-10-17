test("lexage des littérales regex", function () {
    var re1 = /d(b+)d/g;
    var re2 = /[.*+?^${}()|[\]\\]/g;
    var re3 = /ab+c/;
    var re4 = /ab+c/i;
    var re5 = /^[\s\uFEFF\xA0]+|[\s\uFEFF\xA0]+$/g;
    var re6 = /^<(\w+)\s*\/?>(?:<\/\1>|)$/
})