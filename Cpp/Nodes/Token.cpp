#include "../../H/Nodes/Token.h"
#include "../../H/BackEnd/Selector.h"

extern Selector* selector;

Token::Token(Node* n) {
	if (n->is(OBJECT_NODE) || n->is(OBJECT_DEFINTION_NODE)) {
		if (n->Find(n, n->Parent)->Parent->Name == "GLOBAL_SCOPE")
			Flags = TOKEN::GLOBAL_VARIABLE;
		else if (n->Find(n, n->Parent)->Requires_Address)
			Flags = TOKEN::CONTENT;
		else
			Flags = TOKEN::REGISTER;
	}
	else if (n->is(NUMBER_NODE)) {
		if (n->Has_Floating_Point_Value) {
			Flags = TOKEN::DECIMAL;
			Has_Floating_Point_Value = true;
		}
		else {
			Flags = TOKEN::NUM;
		}
	}
	else if (n->is(PARAMETER_NODE)) {
		//first get the index this paremet is
		int Max_Integer_Registers = selector->Get_Numerical_Parameter_Register_Count();
		int Current_Integer_Register_Count = 0;
		int Max_Floating_Registers = selector->Get_Floating_Parameter_Register_Count();
		int Current_Float_Register_Count = 0;
		//find the curresponding register
		for (int i = 0; i < n->Parent->Parameters.size(); i++) {
			if (n->Parent->Parameters[i]->Name == n->Name) {
				if (n->Parent->Parameters[i]->Has_Floating_Point_Value) {
					if (Current_Float_Register_Count < Max_Floating_Registers) {
						Flags = TOKEN::REGISTER | TOKEN::DECIMAL | TOKEN::PARAMETER;
						Parameter_Index = i;
					}
					else
						Flags = TOKEN::CONTENT | TOKEN::DECIMAL;
					break;
				}
				else {
					if (Current_Integer_Register_Count < Max_Integer_Registers) {
						Flags = TOKEN::REGISTER | TOKEN::PARAMETER;
						Parameter_Index = i;
					}
					else
						Flags = TOKEN::CONTENT;
					break;
				}
			}
			if (n->Parent->Parameters[i]->Has_Floating_Point_Value)
				Current_Float_Register_Count++;
			else
				Current_Integer_Register_Count++;
		}
	}
	else if (n->is(STRING_NODE))
		Flags = TOKEN::STRING;
	else if (n->is(LABEL_NODE)) {
		Flags = TOKEN::LABEL;
		n->Size = _SYSTEM_BIT_SIZE_;//for giving the address
	}
	else
		return;
	n->Find(n, n->Parent)->Update_Size_By_Inheritted();
	Size = n->Find(n, n->Parent)->Size;
	Name = n->Name;
	Parent = n->Parent;
}