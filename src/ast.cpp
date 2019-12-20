#include "ast.h"
#include "parser.h"

ck_ast::ASTNode::~ASTNode() {
	if (left == NULL);
	else if (left == right)
		delete left;
	else while (left) {
		ASTNode *nxt = left->next;
		delete left;
		left = nxt;
	};
	
	// Proper delete objects for each type of ASTNode
	switch (type) {
		default:
			break;
			
		case ck_token::INTEGER:
			if (objectlist)
				delete (int64_t*) (objectlist->object);
			break;
			
		case ck_token::BOOLEAN:
			if (objectlist)
				delete (bool*) (objectlist->object);
			break;
			
		case ck_token::DOUBLE:
			if (objectlist)
				delete (double*) (objectlist->object);
			break;
			
		case ck_token::NAME:
		case ck_token::STRING:
			if (objectlist)
				delete (std::wstring*) (objectlist->object);
			break;
			
		case ck_token::ARRAY:
			if (objectlist)
				delete (int*) (objectlist->object);
			break;
			
		case ck_token::FIELD:
			if (objectlist)
				delete (std::wstring*) (objectlist->object);
			break;
			
		case ck_token::DEFINE: {
			ASTObjectList *list = objectlist;
			
			while (objectlist) {
				delete (int*) (objectlist->object);
				list = objectlist->next;
				delete objectlist;
				objectlist = list;
				
				delete (std::wstring*) (objectlist->object);
				list = objectlist->next;
				delete objectlist;
				objectlist = list;
			}
			break;
		}
			
		case ck_token::OBJECT: {
			ASTObjectList *list = objectlist;
			
			while (objectlist) {					
				delete (std::wstring*) (objectlist->object);
				list = objectlist->next;
				delete objectlist;
				objectlist = list;
			}
			break;
		}
			
		case ck_token::FUNCTION: {
			ASTObjectList *list = objectlist;
			
			while (objectlist) {					
				delete (std::wstring*) (objectlist->object);
				list = objectlist->next;
				delete objectlist;
				objectlist = list;
			}
			break;
		}
		
		case ck_token::TRY:				
			if (objectlist)
				delete (std::wstring*) (objectlist->object);
			break;
	}
	delete objectlist;
};