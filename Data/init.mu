
func init()
{
    // loadLua();
    print("init<<<<");
}

func end()
{
    // endLua();
    print("end<<<<");
}

struct test {
	var x = 1;
	var y = "MuScript";
	func test() {
		x = 2;
	}
}

a = test();

if (a.x == 2) {
	print(a.y);
}

b = inspect(a.x);
print(b);