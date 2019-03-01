#pragma once

#include "parser.h"

void printAST(ASTNode *localroot) {
	if (localroot == NULL)
		return;
	
	switch (localroot->type) {
		case ck_token::ASTROOT:
			ASTNode *tmp;
			tmp = localroot->left;
			while (tmp) {
				printAST(tmp);
				putchar(10);
				tmp = tmp->next;
			}
			break;
		
		case ck_token::INTEGER:
			wprintf("%d", *(long long*) (localroot->objectlist->object));
			break;
			
		case ck_token::BOOLEAN:
			wprintf(*(bool*) localroot->objectlist->object ? "true" : "false");
			break;
			
		case ck_token::DOUBLE:
			wprintf("%f", *(double*)localroot->objectlist->object);
			break;
			
		case ck_token::NAME:
			std::wcout << (std::wstring*) localroot->objectlist->object;
			break;
			
		case ck_token::STRING:
			putwchar('\"');
			std::wcout << (std::wstring*) localroot->objectlist->object;
			putwchar('\"');
			break;
			
		case ck_token::ARRAY: {
			ASTNode *p = localroot->left;
			
			wprintf("[ ");
			
			while (p) {
				printAST(p);
				p = p->next;
				if (p)
					wprintf(", ");
			}
			wprintf("]");
			break;
		}
			
		case ck_token::OBJECT: {
			ASTNode *p       = localroot->left;
			ASTObjectList *o = localroot->objectlist;
			
			wprintf("{ ");
			
			while (p) {
				std::wcout << (std::wstring*) (o->object);
				wprintf(": ");
				printAST(p);
				p = p->next;
				o = o->next;
				if (p)
					wprintf(", ");
			}
			wprintf("}");
			break;
		}
			
		case ck_token::FUNCTION: {
			ASTObjectList *o = localroot->objectlist;
			
			wprintf("function (");
			
			while (o) {
				std::wcout << std::wstring*) (o->object);
				o = o->next;
				if (o)
					wprintf(", ");
			}
			wprintf(") ");
			printAST(localroot->left);
			break;
		}
			
		case ck_token::THIS:
			wprintf("this");
			break;
			
		case ck_token::TNULL:
			wprintf("null");
			break;
			
		case ck_token::UNDEFINED:
			wprintf("undefined");
			break;
			
		case ck_token::CONDITION:
			printAST(localroot->left);
			wprintf("? ");
			printAST(localroot->left->next);
			wprintf(": ");
			printAST(localroot->right);
			break;
			
		case ck_token::POS:
			wprintf("+ ");
			printAST(localroot->left);
			break;
			
		case ck_token::NEG:
			wprintf("- ");
			printAST(localroot->left);
			break;
			
		case ck_token::POS_INC:
			printAST(localroot->left);
			wprintf("++");
			break;
			
		case ck_token::POS_DEC:
			printAST(localroot->left);
			wprintf("--");
			break;
			
		case ck_token::PRE_INC:
			wprintf("++");
			printAST(localroot->left);
			break;
			
		case ck_token::PRE_DEC:
			wprintf("--");
			printAST(localroot->left);
			break;
			
		case ck_token::NOT:
			wprintf("!");
			printAST(localroot->left);
			break;
			
		case ck_token::BITNOT:
			wprintf("~");
			printAST(localroot->left);
			break;
			
		case ck_token::PLUS:
			printAST(localroot->left);
			wprintf("+ ");
			printAST(localroot->right);
			break;
			
		case ck_token::MINUS:
			printAST(localroot->left);
			wprintf("- ");
			printAST(localroot->right);
			break;
			
		case ck_token::MUL:
			printAST(localroot->left);
			wprintf("* ");
			printAST(localroot->right);
			break;
			
		case ck_token::DIV:
			printAST(localroot->left);
			wprintf("/ ");
			printAST(localroot->right);
			break;
			
		case ck_token::PATH:
			printAST(localroot->left);
			wprintf("\\\\ ");
			printAST(localroot->right);
			break;
			
		case ck_token::MOD:
			printAST(localroot->left);
			wprintf("% ");
			printAST(localroot->right);
			break;
			
		case ck_token::HASH:
			printAST(localroot->left);
			wprintf("# ");
			printAST(localroot->right);
			break;
			
		case ck_token::AND:
			printAST(localroot->left);
			wprintf("&& ");
			printAST(localroot->right);
			break;
			
		case ck_token::OR:
			printAST(localroot->left);
			wprintf("|| ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITAND:
			printAST(localroot->left);
			wprintf("& ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITOR:
			printAST(localroot->left);
			wprintf("| ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITXOR:
			printAST(localroot->left);
			wprintf("^ ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITRSH:
			printAST(localroot->left);
			wprintf(">> ");
			printAST(localroot->right);
			break;
			
		case ck_token::DOG:
			printAST(localroot->left);
			wprintf("@ ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITLSH:
			printAST(localroot->left);
			wprintf("<< ");
			printAST(localroot->right);
			break;
			
		case ck_token::BITURSH:
			printAST(localroot->left);
			wprintf(">>> ");
			printAST(localroot->right);
			break;
			
		case ck_token::GT:
			printAST(localroot->left);
			wprintf("> ");
			printAST(localroot->right);
			break;
			
		case ck_token::GE:
			printAST(localroot->left);
			wprintf(">= ");
			printAST(localroot->right);
			break;
			
		case ck_token::LT:
			printAST(localroot->left);
			wprintf("< ");
			printAST(localroot->right);
			break;
			
		case ck_token::LE:
			printAST(localroot->left);
			wprintf("<= ");
			printAST(localroot->right);
			break;
			
		case ck_token::EQ:
			printAST(localroot->left);
			wprintf("== ");
			printAST(localroot->right);
			break;
			
		case ck_token::NEQ:
			printAST(localroot->left);
			wprintf("!= ");
			printAST(localroot->right);
			break;
			
		case ck_token::PATH:
			printAST(localroot->left);
			wprintf("\\ ");
			printAST(localroot->right);
			break;
			
		case ck_token::PUSH:
			printAST(localroot->left);
			wprintf("-> ");
			printAST(localroot->right);
			break;
			
		case ck_token::ARROW:
			printAST(localroot->left);
			wprintf("=> ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN:
			printAST(localroot->left);
			wprintf("= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_ADD:
			printAST(localroot->left);
			wprintf("+= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGH_HASH:
			printAST(localroot->left);
			wprintf("+= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_SUB:
			printAST(localroot->left);
			wprintf("-= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_MUL:
			printAST(localroot->left);
			wprintf("*= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_DIV:
			printAST(localroot->left);
			wprintf("/= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_MOD:
			printAST(localroot->left);
			wprintf("%= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITRSH:
			printAST(localroot->left);
			wprintf(">>= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITLSH:
			printAST(localroot->left);
			wprintf("<<= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITURSH:
			printAST(localroot->left);
			wprintf(">>>= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITNOT:
			printAST(localroot->left);
			wprintf("~= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITXOR:
			printAST(localroot->left);
			wprintf("^= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITOR:
			printAST(localroot->left);
			wprintf("|= ");
			printAST(localroot->right);
			break;
			
		case ck_token::ASSIGN_BITAND:
			printAST(localroot->left);
			wprintf("&= ");
			printAST(localroot->right);
			break;
			
		case ck_token::FIELD: 
			printAST(localroot->left);
			wprintf(". ");
			std::wcout << (std::wstring*) (localroot->objectlist->object);
			break;
			
		case ck_token::MEMBER: 
			printAST(localroot->left);
			wprintf("[ ");
			printAST(localroot->right);
			putchar(']');
			break;
			
		case ck_token::CALL: {
			ASTNode *p = localroot->left;
			
			printAST(p);
			
			p = p->next;
			
			wprintf("( ");
			
			while (p) {
				printAST(p);
				p = p->next;
				if (p)
					wprintf(", ");
			}
			wprintf(")");
			
			break;
		}
			
		case ck_token::EMPTY:
		default:
			wprintf("{ }");
			break;
			
		case ck_token::EXPRESSION:
			if (localroot->left != NULL)
				printAST(localroot->left);
			
			putchar(';');
			break;
			
		case ck_token::DEFINE: {
			tmp = localroot->left;
			ASTObjectList *list = localroot->objectlist;
			
			while (list) {
				int type = *(int*)list->object;
				list = list->next;
				std::wstring *name = (std::wstring*) list->object;
				list = list->next;
				
				if (type & 0b0100) wprintf("safe ");
				if (type & 0b0010) wprintf("local ");
				if (type & 0b0001) wprintf("const ");
				wprintf("var ");
				std::wcout << name;
				
				if (type & 0b1000) {
					wprintf(" = ");
					printAST(tmp);
					tmp = tmp->next;
				}
			};
			
			putchar(';');
			break;
		}
		
		case ck_token::IF:
			wprintf("if ( ");
			printAST(localroot->left);
			wprintf(") ");
			printAST(localroot->left->next);
			if (localroot->left->next->next->type != EMPTY) {
				wprintf("else ");
				printAST(localroot->left->next->next);
			}
			break;
		
		case ck_token::SWITCH:
			wprintf("switch ( ");
			printAST(localroot->left);
			wprintf(") ");
			tmp = localroot->left->next;
			wprintf("{ ");
			while (tmp) {
				if (tmp->type == CASE) {
					wprintf("case ck_token::");
					printAST(tmp->left);
					wprintf(": ");
					printAST(tmp->left->next);
				} else {
					wprintf("default: ");
					printAST(tmp->left);
				}
				tmp = tmp->next;
			}
			wprintf("}");
			break;
			
		case ck_token::BREAK:
			wprintf("break");
			break;
			
		case ck_token::CONTINUE:
			wprintf("break");
			break;
			
		case ck_token::RETURN:
			wprintf("return");
			if (localroot->left) {
				wprintf(" ");
				printAST(localroot->left);
			}
			
			putchar(';');
			break;
			
		case ck_token::RAISE:
			wprintf("raise");
			if (localroot->left) {
				wprintf(" ");
				printAST(localroot->left);
			}
			
			putchar(';');
			break;
			
		case ck_token::WHILE:
			wprintf("while ( ");
			printAST(localroot->left);
			wprintf(") ");
			printAST(localroot->left->next);
			break;
			
		case ck_token::DO:
			wprintf("do ");
			printAST(localroot->left->next);
			wprintf("while ( ");
			printAST(localroot->left);
			wprintf(")");
			break;
			
		case ck_token::FOR:
			wprintf("for ( ");
			if (localroot->left->type != EMPTY)
				printAST(localroot->left);
	
			wprintf("; ");
			if (localroot->left->next->type != EMPTY)
				printAST(localroot->left->next);
	
			wprintf("; ");
			if (localroot->left->next->next->type != EMPTY)
				printAST(localroot->left->next->next);
			
			wprintf(") ");
			printAST(localroot->left->next->next->next);
			break;
			
		case ck_token::BLOCK:
			tmp = localroot->left;
			wprintf("{ ");
			while (tmp) {
				printAST(tmp);
				tmp = tmp->next;
			}
			wprintf("}");
			break;
			
		case ck_token::TRY:
			wprintf("try ");
			printAST(localroot->left);
			if (localroot->left->next->type != EMPTY) {
				wprintf("expect ");
				if (localroot->objectlist != NULL) {
					wprintf("( ");
				std::wcout << (std::wstring*)localroot->objectlist->object;
					wprintf(" ) ");
				}
				printAST(localroot->left->next);
			}
			break;
	};
	
	putchar(32);
};