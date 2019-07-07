#ifndef _CODEGEN_H
#define _CODEGEN_H

#define STACK_CHILDREN 8

typedef struct ASTNode {
	int type;
	int totchildren;
	struct ASTNode** children;
	struct ASTNode* _static_children[STACK_CHILDREN]; //used to save allocation effort
} ASTNode;

typedef struct ASTGraph {
	ASTNode* root;
	ASTNode** nodes;
} ASTGraph;

ASTGraph* AST_InitGraph(ASTGraph* graph);
void AST_FreeGraph(ASTGraph* graph);
ASTNode* AST_MakeNode(ASTGraph* graph, int type);
void AST_FreeNode(ASTGraph* graph, ASTNode *node);

typedef struct StateMachineIF {
	
} StateMachineIF;

typedef struct CodeGeneratorIF {
	
} CodeGeneratorIF;

#endif /* _CODEGEN_H*/