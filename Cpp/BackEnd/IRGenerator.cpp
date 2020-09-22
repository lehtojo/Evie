#include "../../H/BackEnd/BackEnd.h"
#include "..\..\H\BackEnd\IRGenerator.h"

void IRGenerator::Factory()
{
}

void IRGenerator::Parse_Function(int i)
{
	if (!Input[i]->is(FUNCTION_NODE))
		return;

	//label
	IR* Label = Make_Label(Input[i]);
	Output->push_back(Label);

	//go through the childs of the function
	IRGenerator g(Input[i], Input[i]->Childs, Output);

}

void IRGenerator::Parse_If(int i)
{
	if (!Input[i]->is(IF_NODE))
		return;

	//check next condition goto it else goto check this condition then goto end
	//cmp eax, ecx		;1 != a
	//je L1
	//..
	//jmp L3
	//L1:
	//cmp eax, ebx		;1 == b
	//jne L2
	//..
	//jmp L3
	//L2:
	//..
	//L3:
	//do parameters
	Node* current = Input[i];
	while (true) {
		if (current == nullptr)
			break;
		//do an subfunction that can handle coditions and gets the label data for the condition data from the Parent given.
		IRGenerator p(Input[i], Input[i]->Parameters, Output);
		//then do childs
		IRGenerator c(Input[i], Input[i]->Childs, Output);

		current = current->Succsessor;
	}

}

void IRGenerator::Parse_Condition(int i)
{
	//NOTICE: this must happen after all operator is is created as IR!!!
	if (!Input[i]->is(CONDITION_OPERATOR_NODE))
		return;
	if (!Parent->is(IF_NODE) && !Parent->is(ELSE_IF_NODE)) {
		// a = b * c < d
		//...
		return;
	}
	// if (a == 1)
	//give the right side as left side to IRGenerator
	IRGenerator g(Parent, { Input[i]->Right, Input[i]->Left }, Output);

	//TODO: check later for the _END addon if conditions even use it :/
	string Next_Label = Parent->Name + "_END";
	if (Parent->Succsessor != nullptr)
		Next_Label = Parent->Succsessor->Name;

	//jmp if not correct
	Output->push_back(Make_Jump(Get_Inverted_Condition(Input[i]->Name), Next_Label));
}

void IRGenerator::Un_Wrap_Inline(int i)
{
	if (!(Input[i]->Header.size() > 0))
		return;

	IRGenerator g(Parent, Input[i]->Header, Output);
}

void IRGenerator::Parse_Operators(int i)
{
	if (!Input[i]->is(OPERATOR_NODE) && !Input[i]->is(CONDITION_OPERATOR_NODE) && !Input[i]->is(BIT_OPERATOR_NODE))
		return;
	//If this operator is handling with pointters we cant use general operator handles
	if (Input[i]->Left->is("ptr"))
		return;
	if (Input[i]->Right->is("ptr"))
		return;

	Token* Left = nullptr;
	Token* Right = nullptr;

	IRGenerator g(Parent, { Input[i]->Right }, Output);

	if (g.Handle != nullptr)
		Right = g.Handle;
	else {
		//create the register
		Token* Reg = new Token(TOKEN::REGISTER, "REG_" + Input[i]->Right->Name, Input[i]->Right->Size);
		//create a Token version out of the right side
		Token* Value = new Token(Input[i]->Right);
		//create the IR
		Token* Opc = new Token(TOKEN::OPERATOR, "=");
		IR* ir = new IR(Opc, { Reg, Value });

		Right = Reg;
		Output->push_back(ir);
	}
	
	g.Generate({ Input[i]->Left });

	if (g.Handle != nullptr)
		Left = g.Handle;
	else {
		if (Input[i]->Name == "=") {
			//dont load the value into a register
			Left = new Token(Input[i]->Left);
		}
		else {
			//create the register
			Token* Reg = new Token(TOKEN::REGISTER, "REG_" + Input[i]->Left->Name, Input[i]->Left->Size);
			//create a Token version out of the right side
			Token* Value = new Token(Input[i]->Left);
			//create the IR
			Token* Opc = new Token(TOKEN::OPERATOR, "=");
			IR* ir = new IR(Opc, { Reg, Value });

			Left = Reg;
			Output->push_back(ir);
		}
	}

	Token* Opcode = new Token(TOKEN::OPERATOR, Input[i]->Name);

	IR* ir = new IR(Opcode, {Left, Right});

	Handle = Left;
	Output->push_back(ir);
}

void IRGenerator::Parse_Pointers(int i)
{
	if ((Input[i]->Left->is("ptr") == -1) && Input[i]->Right->is("ptr") == -1)
		return;

	//int a = 0
	//int ptr b = a + f - e / g get the address of memory
	//int ptr c = b pass memory address of a to c
	//int d = c		load value of a into c

	//if true load the pointing value or move the inside value
	bool Load_Value = Get_Amount("ptr", Input[i]->Left) < Get_Amount("ptr", Input[i]->Right);
	bool Point_To_Value = Get_Amount("ptr", Input[i]->Left) > Get_Amount("ptr", Input[i]->Right);


	int Pointer_Depth = labs(Get_Amount("ptr", Input[i]->Left) - Get_Amount("ptr", Input[i]->Right));

	Token* Left = nullptr;
	Token* Right = nullptr;
	Token* Source = nullptr;
	
	IRGenerator g(Parent, { Input[i]->Left }, Output);

	if (g.Handle != nullptr)
		Left = g.Handle;
	else
		Left = new Token(Input[i]->Left);

	g.Generate({ Input[i]->Right });

	if (g.Handle != nullptr)
		Source = g.Handle;
	else
		Source = new Token(Input[i]->Right);

	if (Load_Value) {
		int Register_Count = 0;
		for (int j = 0; j < Pointer_Depth; j++) {
			//load reg1, right
			//make an register to load into
			Right = new Token(TOKEN::REGISTER, "REG_" + to_string(Register_Count++) + Source->Get_Name(), Source->Get_Size());

			Token* Load = new Token(TOKEN::OPERATOR, "load");

			IR* ir = new IR(Load, { Right, Source });
			Output->push_back(ir);

			Source = Right;
		}
	}
	else if (Point_To_Value) {
		if (Pointer_Depth > 1) {
			cout << "Error: Could not get temporary memory allocated for pointter depth more than 1 " << Input[i]->Left->Name << Input[i]->Name << Input[i]->Right->Name << endl;
			exit(1);
		}
		//save reg1, right		
		//make an register to save right into
		Right = new Token(TOKEN::REGISTER, "REG_" + Source->Get_Name(), Source->Get_Size());
		Token* Save = new Token(TOKEN::OPERATOR, "save");
		IR* ir = new IR(Save, { Right, Source });
		Output->push_back(ir);
	}
	Token* Opc = new Token(TOKEN::OPERATOR, Input[i]->Name);
	IR* Opcode = new IR(Opc, {Left, Right});
	Output->push_back(Opcode);

	Handle = Left;
}

string IRGenerator::Get_Inverted_Condition(string c)
{
	if (c == "==")
		return "!=";
	else if (c == "!=")
		return "==";
	else if (c == "<")
		return ">";
	else if (c == ">")
		return "<";
	else if (c == "!<")
		return ">=";
	else if (c == "!>")
		return "<=";
	else if (c == "<=")
		return ">";
	else if (c == ">=")
		return "<";
	cout << "Error: Undefined Condition type " << c << endl;
	return "";
}

IR* IRGenerator::Make_Label(Node* n)
{
	Token* label_name = new Token(TOKEN::LABEL, n->Name);
	IR* label = new IR();
	label->OPCODE = label_name;
	return label;
}

IR* IRGenerator::Make_Jump(string condition, string l)
{
	Token* jmp = new Token(TOKEN::FLOW, condition);
	Token* label = new Token(TOKEN::LABEL, l);
	IR* op = new IR();
	op->OPCODE = jmp;
	op->Arguments.push_back(label);
	return op;
}

int IRGenerator::Get_Amount(string t, Node* n)
{
	int result = 0;
	for (string s : n->Inheritted)
		if (s == t)
			result++;

	return result;
}