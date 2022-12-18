
meta[[

mlua.define_simple("inc", "$1 = $1 + 1")

mlua.define_simple("inc_e", "(function () $1 = $1 + 1; return $1 end)()")

]]

local a = 2

inc[[a]]

print(a)

print(inc_e[[a]])

print(a)

mu[[
gaga

$@#%$&^&*)()_)_((*)*$@$!@!)

]]