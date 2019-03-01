#pragma once

#include <string>

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
		
		~ASTNode();
		
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