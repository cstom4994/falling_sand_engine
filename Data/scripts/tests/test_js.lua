
local jsparser = require("jsparser")

local test = [[

let test = 3;
test++;
test--;
test += 5;
test /= 4;
test *= 3;
test ^= 2;
test -= 1;

var test2 = 6;
print(test2)

let test = {5, 6, 7};
for(c in test) {
    print(c);
}

for(c of test) {
    print(c);
}

for(k,v in test) {
    print(k, v);
}


let test = () => {
    print("hello, world!");
}
test();

let test = (a, b) => a + b;
print(test(1, 2));

const vv = 3;

try {
    vv = 4;
} catch {
    print("can't change vv!");
}

try {
    NoExistingFunc();
} catch {
    print("can't call this function!");
}


]]


test = jsparser(test)

print(test, "\n---------------------")

local v = load(test)
assert(v, "Error")
v()

print("ok")