#include "../../H/BackEnd/IRPostProsessor.h"

extern Selector* selector;

void IRPostProsessor::Scale_To_Same_Size(int i)
{
	if (!Input->at(i)->is(TOKEN::OPERATOR))
		return;

	if (!selector->Get_Opcode(Input->at(i))->is(TOKEN::ALL_ARGS_SAME_SIZE))
		return;

	//there IR's are still universal so dont worry about, if there is more than 2 arguments with the operator.
	//convert always the smaller into the bigger size
	Token* Scalable = nullptr;
	int Destination_Size = 0;
	int ArgIndex = 0;

	if (Input->at(i)->Arguments[0]->Get_Size() > Input->at(i)->Arguments[1]->Get_Size()) {
		//this means that right side is smaller, scale.
		Scalable = Input->at(i)->Arguments[1];
		ArgIndex = 1;	//tell later that were the scalable originates from.
		Destination_Size = Input->at(i)->Arguments[0]->Get_Size();
	}
	else if (Input->at(i)->Arguments[0]->Get_Size() < Input->at(i)->Arguments[1]->Get_Size()) {
		//this means thatthe left side is smaller, scale.
		Scalable = Input->at(i)->Arguments[0];
		ArgIndex = 0; //tell later where scalable originates from.
		Destination_Size = Input->at(i)->Arguments[1]->Get_Size();
	}
	else {
		//this means that theyre same size
		return;
	}

	Token* NewReg = new Token(TOKEN::REGISTER, Scalable->Get_Name() + "_converted", Destination_Size);

	Token* Convert = new Token(TOKEN::OPERATOR, "convert");
	IR* CONV = new IR(Convert, { NewReg, new Token(*Scalable) });

	//now put the newreg where the scalable was.
	Input->at(i)->Arguments[ArgIndex] = NewReg;

	//now insert the new converter before the operator
	Input->insert(Input->begin() + i, CONV);
}

void IRPostProsessor::Registerize(int i)
{
	//mov eax, ebx
	for (auto j : Input->at(i)->Arguments) {
		if (j->is(TOKEN::REGISTER)) {
			if (selector->Get_Register(j) == nullptr)
				if (selector->Get_New_Reg(Input, i, j) == nullptr)
					selector->Allocate_Register(Input, i, j);
			j->ID = selector->Get_Register(j)->Get_Name();
		}
	}
}

void IRPostProsessor::Handle_Calls(int i)
{
	if (!Input->at(i)->is(TOKEN::CALL))
		return;

	//handle the given registers
	//hmm i think the registerizer can handle these too, 
	//the parameters in OPCODE emeber are just for information for the selector.

}

void IRPostProsessor::Factory()
{
	for (int i = 0; i < Input->size(); i++)
		Scale_To_Same_Size(i);
	for (int i = 0; i < Input->size(); i++)
		Registerize(i);
}