#pragma once

#include "parser.h"

void printAST(ck_ast::ASTNode *localroot) { // XXX: how to save line numbers?
	if (localroot == NULL)
		return;
	
	switch (localroot->type) {
		case ck_token::ASTROOT:
			ck_ast::ASTNode *tmp;
			tmp = localroot->left;
			while (tmp) {
				printAST(tmp);
				putwchar(10);
				tmp = tmp->next;
			}
			break;
		
		case ck_token::INTEGER:
			wprintf(L"%d", *(int64_t*) (localroot->objectlist->object));
			break;
			
		case ck_token::BOOLEAN:
			wprintf(*(bool*) localroot->objectlist->object ? L"true" : L"false");
			break;
			
		case ck_token::DOUBLE:
			wprintf(L"%f", *(double*)localroot->objectlist->object);
			break;
			
		case ck_token::NAME:
			std::wcout << *(std::wstring*) localroot->objectlist->object;
			break;
			
		case ck_token::STRING:
			putwchar('\"');
			std::wcout << *(std::wstring*) localroot->objectlist->object;
			putwchar('\"');
			break;
			
		case ck_token::ARRAY: {
			ck_ast::ASTNode *p = localroot->left;
			
			wprintf(L"[ ");
			
			while (p) {
				printAST(p);
				p = p->next;
				if (p)
					wprintf(L", ");
			}
			wprintf(L"]");
			break;
		}
			
		case ck_token::OBJECT: { // Incorrect due the wrong direction
			ck_ast::ASTNode *p       = localroot->left;
			ck_ast::ASTObjectList *o = localroot->objectlist;
			
			wprintf(L"{ ");
			
			while (p) {
				std::wcout << *(std::wstring*) (o->object);
				wprintf(L": ");
				printAST(p);
				p = p->next;
				o = o->next;
				if (p)
					wprintf(L", ");
			}
			wprintf(L"}");
			break;
		}
			
		case ck_token::FUNCTION: {
			ck_ast::ASTObjectList *o = localroot->objectlist;
			
			wprintf(L"function (");
			
			while (o) {
				std::wcout << *(std::wstring*) (o->object);
				o = o->next;
				if (o)
					wprintf(L", ");
			}
			wprintf(L") ");
			printAST(localroot->left);
			break;
		}
			
		case ck_token::THIS:
			wprintf(L"this");
			break;
			
		case ck_token::TNULL:
			wprintf(L"null");
			break;
			
		case ck_token::UNDEFINED:
			wprintf(L"undefined");
			break;
			
		case ck_token::CONDITION:
			printAST(localroot->left);
			wprintf(L"? ");
			printAST(localroot->left->next);
			wprintf(L": ");
			printAST(localroot->right);
			break;
			
		case ck_token::POS:
			wprintf(L"+ ");
			printAST(localroot->left);
			break;
			
		case ck_token::NEG:
			wprintf(L"- ");
			printAST(localroot->left);
			break;
			
		case ck_token::POS_INC:
			printAST(localroot->left);
			wprintf(L"++");
			break;
			
		case ck_token::POS_DEC:
			printAST(localroot->left);
			wprintf(L"--");
			break;
			
		case ck_token::PRE_INC:
			wprintf(L"++");
			printAST(localroot->left);
			break;
			
		case ck_token::PRE_DEC:
			wprintf(L"--");
			printAST(localroot->left);
			break;
			
		case ck_token::NOT:
			wprintf(L"!");
			printAST(localroot->left);
			break;
			
		case ck_token::BITNOT:
			wprintf(L"~");
			printAST(localroot->left);
			break;
			
		case ck_token::PLUS:
			printAST(localroot->left);
			wprintf(L"+ ");
			printAST(localroot->right);
			break;
			
		case ck_token::MINUS:
			printAST(localroot->left);
			wprintf(L"- ");
			printAST(localroot->right);
			break;
			
		case ck_token::MUL:
			printAST(localroot->left);
			wprintf(L"* ");
			printAST(localroot->right);
			break;
			
		case ck_token::DIV:
			printAST(localroot->left);
			wprintf(L"/ ");
			printAST(localroot->right);
			break;
			
		case ck_token::DIR:
			printAST(localroot->left);
			wprintf(L"\\\\ ");
			printAST(localroot->right);
			break;
			
		case ck_token::MOD:
			printAST(localroot->left);
			wprintf(L"% ");
			printAST(localroot->right);
			break;
			
		case ck_token::HASH:
			printAST(localroot->left);
			wprintf(L"# ");
			printAST(localroot->right);
			break;
			
		case ck_token::AND:
			printAST(localroot->left);
			wprintf(L"&& ");
			printAST(localroot->right);
			break;
			
		case ck_token::OR:
			printAST(localroot->left);
			wprintf(L"|| ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITAND:
			printAST(localroot->left);
			wprintf(L"& ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITOR:
			printAST(localroot->left);
			wprintf(L"| ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITXOR:
			printAST(localroot->left);
			wprintf(L"^ ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITRSH:
			printAST(localroot->left);
			wprintf(L">> ");
			printAST(localroot->right);
			break;
			
		case ck_token::DOG:
			printAST(localroot->left);
			wprintf(L"@ ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITLSH:
			printAST(localroot->left);
			wprintf(L"<< ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITURSH:
			printAST(localroot->left);
			wprintf(L">>> ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITULSH:
			printAST(localroot->left);
			wprintf(L"<<< ");
			printAST(localroot->right);
			break;
			
		case ck_token::GT:
			printAST(localroot->left);
			wprintf(L"> ");
			printAST(localroot->right);
			break;
			
		case ck_token::GE:
			printAST(localroot->left);
			wprintf(L">= ");
			printAST(localroot->right);
			break;
			
		case ck_token::LT:
			printAST(localroot->left);
			wprintf(L"< ");
			printAST(localroot->right);
			break;
			
		case ck_token::LE:
			printAST(localroot->left);
			wprintf(L"<= ");
			printAST(localroot->right);
			break;
			
		case ck_token::EQ:
			printAST(localroot->left);
			wprintf(L"== ");
			printAST(localroot->right);
			break;
			
		case ck_token::NEQ:
			printAST(localroot->left);
			wprintf(L"!= ");
			printAST(localroot->right);
			break;
			
		case ck_token::LEQ:
			printAST(localroot->left);
			wprintf(L"=== ");
			printAST(localroot->right);
			break;
			
		case ck_token::NLEQ:
			printAST(localroot->left);
			wprintf(L"!== ");
			printAST(localroot->right);
			break;
			
		case ck_token::PATH:
			printAST(localroot->left);
			wprintf(L"\\ ");
			printAST(localroot->right);
			break;
			
		case ck_token::PUSH:
			printAST(localroot->left);
			wprintf(L"-> ");
			printAST(localroot->right);
			break;
			
		case ck_token::ARROW:
			printAST(localroot->left);
			wprintf(L"=> ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN:
			printAST(localroot->left);
			wprintf(L"= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_ADD:
			printAST(localroot->left);
			wprintf(L"+= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_HASH:
			printAST(localroot->left);
			wprintf(L"+= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_SUB:
			printAST(localroot->left);
			wprintf(L"-= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_MUL:
			printAST(localroot->left);
			wprintf(L"*= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_DIV:
			printAST(localroot->left);
			wprintf(L"/= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_MOD:
			printAST(localroot->left);
			wprintf(L"%= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITRSH:
			printAST(localroot->left);
			wprintf(L">>= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITLSH:
			printAST(localroot->left);
			wprintf(L"<<= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITURSH:
			printAST(localroot->left);
			wprintf(L">>>= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITULSH:
			printAST(localroot->left);
			wprintf(L"<<<= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITXOR:
			printAST(localroot->left);
			wprintf(L"^= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITOR:
			printAST(localroot->left);
			wprintf(L"|= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITAND:
			printAST(localroot->left);
			wprintf(L"&= ");
			printAST(localroot->right);
			break;
			
		case ck_token::FIELD: 
			printAST(localroot->left);
			wprintf(L". ");
			std::wcout << *(std::wstring*) (localroot->objectlist->object);
			break;
			
		case ck_token::MEMBER: 
			printAST(localroot->left);
			wprintf(L"[ ");
			printAST(localroot->right);
			putwchar(']');
			break;
			
		case ck_token::CALL: {
			ck_ast::ASTNode *p = localroot->left;
			
			printAST(p);
			
			p = p->next;
			
			wprintf(L"( ");
			
			while (p) {
				printAST(p);
				p = p->next;
				if (p)
					wprintf(L", ");
			}
			wprintf(L")");
			
			break;
		}
			
		case ck_token::EMPTY:
		default:
			wprintf(L"{ }");
			break;
			
		case ck_token::EXPRESSION:
			if (localroot->left != NULL)
				printAST(localroot->left);
			
			putwchar(';');
			break;
			
		case ck_token::DEFINE: { // Incorrect due the wrong direction
			tmp = localroot->left;
			ck_ast::ASTObjectList *list = localroot->objectlist;
			
			while (list) {
				int type = *(int*)list->object;
				list = list->next;
				std::wstring *name = (std::wstring*) list->object;
				list = list->next;
				
				if (type & 0b0100) wprintf(L"safe ");
				if (type & 0b0010) wprintf(L"local ");
				if (type & 0b0001) wprintf(L"const ");
				wprintf(L"var ");
				std::wcout << *name;
				
				if (type & 0b1000) {
					wprintf(L" = ");
					printAST(tmp);
					tmp = tmp->next;
				}
			};
			
			putwchar(';');
			break;
		}
		
		case ck_token::IF:
			wprintf(L"if ( ");
			printAST(localroot->left);
			wprintf(L") ");
			printAST(localroot->left->next);
			if (localroot->left->next->next->type != ck_token::EMPTY) {
				wprintf(L"else ");
				printAST(localroot->left->next->next);
			}
			break;
		
		case ck_token::SWITCH:
			wprintf(L"switch ( ");
			printAST(localroot->left);
			wprintf(L") ");
			tmp = localroot->left->next;
			wprintf(L"{ ");
			while (tmp) {
				if (tmp->type == ck_token::CASE) {
					wprintf(L"case ck_token::");
					printAST(tmp->left);
					wprintf(L": ");
					printAST(tmp->left->next);
				} else {
					wprintf(L"default: ");
					printAST(tmp->left);
				}
				tmp = tmp->next;
			}
			wprintf(L"}");
			break;
			
		case ck_token::BREAK:
			wprintf(L"break");
			break;
			
		case ck_token::CONTINUE:
			wprintf(L"break");
			break;
			
		case ck_token::RETURN:
			wprintf(L"return");
			if (localroot->left) {
				wprintf(L" ");
				printAST(localroot->left);
			}
			
			putwchar(';');
			break;
			
		case ck_token::THROW:
			wprintf(L"throw");
			if (localroot->left->type != ck_token::EMPTY) {
				wprintf(L" ");
				printAST(localroot->left);
			}
			
			putwchar(';');
			break;
			
		case ck_token::WHILE:
			wprintf(L"while ( ");
			printAST(localroot->left);
			wprintf(L") ");
			printAST(localroot->left->next);
			break;
			
		case ck_token::DO:
			wprintf(L"do ");
			printAST(localroot->left->next);
			wprintf(L"while ( ");
			printAST(localroot->left);
			wprintf(L")");
			break;
			
		case ck_token::FOR:
			wprintf(L"for ( ");
			if (localroot->left->type != ck_token::EMPTY)
				printAST(localroot->left);
	
			wprintf(L"; ");
			if (localroot->left->next->type != ck_token::EMPTY)
				printAST(localroot->left->next);
	
			wprintf(L"; ");
			if (localroot->left->next->next->type != ck_token::EMPTY)
				printAST(localroot->left->next->next);
			
			wprintf(L") ");
			printAST(localroot->left->next->next->next);
			break;
			
		case ck_token::BLOCK:
			tmp = localroot->left;
			wprintf(L"{ ");
			while (tmp) {
				printAST(tmp);
				tmp = tmp->next;
			}
			wprintf(L"}");
			break;
			
		case ck_token::TRY:
			wprintf(L"try ");
			printAST(localroot->left);
			if (localroot->left->next->type != ck_token::EMPTY) {
				wprintf(L"expect ");
				if (localroot->objectlist != NULL) {
					wprintf(L"( ");
				std::wcout << *(std::wstring*)localroot->objectlist->object;
					wprintf(L" ) ");
				}
				printAST(localroot->left->next);
			}
			break;
			
		case ck_token::IN:
			std::wcout << *(std::wstring*) localroot->objectlist->object << " in ";
			printAST(localroot->left);
			break;
	};
	
	putwchar(32);
};