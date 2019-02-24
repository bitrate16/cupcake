#pragma once

#include <std::wstring>

namespace ck_ast {
	class ASTObjectList {
		public:
		
		ASTObjectList *next;
		void *object;
		
		ASTObjectList() {
			next = NULL;
			object = NULL;
		};
		
		ASTObjectList(void *obj) {
			next = NULL;
			object = obj;
		};
		
		ASTObjectList(ASTObjectList *list) {
			next = list;
			object = NULL;
		};
		
		ASTObjectList(ASTObjectList *list, void *obj) {
			next = list;
			object = obj;
		};
		
		~ASTObjectList() {
			// // delete object;
			// delete next;
		};
	};
	
	class ASTNode {
		public:
		
		int       type;
		int     lineno;
		ASTNode  *left;
		ASTNode *right;
		ASTNode  *next;
		
		// For storing local data like arrays, integers, strings.. e.t.c.
		ASTObjectList *objectlist;
		
		ASTNode(int lineno) {
			this->lineno = lineno;
			this->left   = NULL;
			this->right  = NULL;
			this->next   = NULL;
			this->objectlist = NULL;
		};
		
		ASTNode(int lineno, int type) {
			this->lineno = lineno;
			this->type   = type;
			this->left   = NULL;
			this->right  = NULL;
			this->next   = NULL;
			this->objectlist = NULL;
		};
		
		ASTNode(int lineno, ASTNode *child) {
			this->lineno = lineno;
			this->left   = child;
			this->right  = child;
			child->next  = NULL;
			this->objectlist = NULL;
		};
		
		ASTNode(int lineno, ASTNode *left, ASTNode *right) {
			this->lineno = lineno;
			this->left   = left;
			this->right  = right;
			left->next   = right;
			right->next  = NULL;
			this->objectlist = NULL;
		};
		
		~ASTNode() {
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
					
				case INTEGER:
					if (objectlist)
						delete (int*) (objectlist->object);
					break;
					
				case LONG:
					if (objectlist)
						delete (long*) (objectlist->object);
					break;
					
				case BYTE:
					if (objectlist)
						delete (char*) (objectlist->object);
					break;
					
				case BOOLEAN:
					if (objectlist)
						delete (bool*) (objectlist->object);
					break;
					
				case DOUBLE:
					if (objectlist)
						delete (double*) (objectlist->object);
					break;
					
				case NAME:
				case STRING:
					if (objectlist)
						delete (std::wstring*) (objectlist->object);
					break;
					
				case ARRAY:
					if (objectlist)
						delete (int*) (objectlist->object);
					break;
					
				case FIELD:
					if (objectlist)
						delete (std::wstring*) (objectlist->object);
					break;
					
				case DEFINE: {
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
					
				case OBJECT: {
					ASTObjectList *list = objectlist;
					
					while (objectlist) {					
						delete (std::wstring*) (objectlist->object);
						list = objectlist->next;
						delete objectlist;
						objectlist = list;
					}
					break;
				}
					
				case FUNCTION: {
					ASTObjectList *list = objectlist;
					
					while (objectlist) {					
						delete (std::wstring*) (objectlist->object);
						list = objectlist->next;
						delete objectlist;
						objectlist = list;
					}
					break;
				}
				
				case TRY:				
					if (objectlist)
						delete (std::wstring*) (objectlist->object);
					break;
			}
			delete objectlist;
		};
		
		ASTNode *addChild(ASTNode *child) {
			if (right == NULL)
				left = right = child;
			else {
				right->next = child;
				right = child;
			}
			return this;
		};
		
		ASTNode *addLastChild(ASTNode *child) {
			if (left == NULL)
				left = right = child;
			else {
				child->next = left;
				left        = child;
			}
			return this;
		};
		
		ASTNode *addLastObject(void *object) {
			if (object != NULL) {
				ASTObjectList *tmp = objectlist;
			
				// Insert into end
			
				if (tmp == NULL) {
					objectlist = new ASTObjectList(object);
					return this;
				}
				
				while (tmp->next)
					tmp = tmp->next;
				
				tmp->next = new ASTObjectList(object);
			}
			return this;
		};
		
		ASTNode *addFirstObject(void *object) {
			if (object != NULL) {
				ASTObjectList *tmp = new ASTObjectList(object);
				tmp->next  = objectlist;
				objectlist = tmp;
			}
			return this;
		};
		
		ASTNode *setType(int type) {
			this->type = type;
			return this;
		};
		
		int getType() {
			return this->type;
		};
		
		void dispose() {
			delete this;
		};
	};
};