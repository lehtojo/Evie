use "https://github.com/Gabidal/Evie/IO/cstd.e"

export int main(){
	return 1
}

type foo{
	int X = 1
	int Y = 2
}

export int Start_Test(){
	foo F = foo(New<foo>())
	foo ptr handle_1 = F


	return 1
}