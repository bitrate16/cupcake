#include "executer.h"


using namespace std;
using namespace ck_core;


ck_executer::ck_executer() {
	
};

ck_executer::~ck_executer() {
	
};

void ck_executer::execute(ck_core::ck_script* scr) {};

void ck_executer::execute(ck_core::ck_script* scr, std::wstring argn, ck_vobject::vobject* argv) {};

void ck_executer::call_object(ck_vobject::vobject* obj) {};

void ck_executer::goto_address(int bytecode_address) {};