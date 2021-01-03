#include "compiler.h"

#include "common.h"
#include "object.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CORE
#include "debug.h
#endif // DEBUG_PRINT_CORE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	Token current;
	Token previous;
	bool hadError;
	bool panicMode; // resynchronize if true (prevents cascading errors).
} Parser;

typedef enum
{
	PREC_NONE,
	PREC_ASSIGNMENT,
	PREC_OR,
	PREC_AND,
	PREC_EQUALITY,
	PREC_COMPARISON,
	PREC_TERM,
	PREC_FACTOR,
	PREC_UNARY,
	PREC_CALL,
	PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)(bool canAssign);

typedef struct
{
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

Parser parser;

Chunk* compilingChunk;

typedef struct
{
	Token name;
	int depth;  // depth of -1 means undefined
} Local;

#define MAX_LOCALS 500 // random number over 255 so I have an excuse to add _LONG_ op codes for local variable ops 

typedef struct
{
	Local locals[MAX_LOCALS]; // Too lazy to make a dynamic array of Locals
	int localsCount;
	int currentScopeDepth;
} Compiler;

Compiler* currentCompiler;

static void InitCompiler(Compiler* compiler)
{
	compiler->localsCount = compiler->currentScopeDepth = 0;
	currentCompiler = compiler;
}

static Chunk* CurrentChunk()
{
	return compilingChunk;
}

static void ErrorAt(Token* token, const char* message)
{
	if (parser.panicMode) { return; }
	parser.panicMode = true;

	fprintf(stderr, "[line %d] Error", token->line);

	if (token->type == TOKEN_EOF)
	{
		fprintf(stderr, " at end");
	}
	else if (token->type != TOKEN_ERROR)
	{
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.hadError = true;
}

static void ErrorAtCurrent(const char* message)
{
	ErrorAt(&parser.current, message);
}

static void Error(const char* message)
{
	ErrorAt(&parser.previous, message);
}

static void Advance()
{
	parser.previous = parser.current;

	while (true)
	{
		parser.current = ScanToken();
		if (parser.current.type != TOKEN_ERROR) { break; }
		ErrorAtCurrent(parser.current.start);
	}
}

static void Consume(TokenType type, const char* message)
{
	if (parser.current.type == type)
	{
		Advance();
	}
	else
	{
		ErrorAtCurrent(message);
	}
}

static bool Check(TokenType type)
{
	return parser.current.type == type;
}

static bool Match(TokenType type)
{
	if (Check(type)) {
		Advance();
		return true;
	}
	return false;
}

static void EmitByte(uint8_t byte)
{
	WriteChunk(CurrentChunk(), byte, parser.previous.line);
}

static int EmitConstant(Value value)
{
	return WriteConstant(CurrentChunk(), value, parser.previous.line);
}

static void Number(bool canAssign)
{
	double value = strtod(parser.previous.start, NULL);
	EmitConstant(NUMBER_VAL(value));
}

static void Expression();
static void Statement();
static void Declaration();
static ParseRule* GetRule(TokenType type);
static void ParsePrecedence(Precedence);
static void And(bool);
static void Or(bool);
static void VarDeclaration();

static void Grouping(bool canAssign)
{
	Expression();
	Consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void Unary(bool canAssign)
{
	TokenType opType = parser.previous.type;

	ParsePrecedence(PREC_UNARY);

	switch (opType)
	{
	case TOKEN_MINUS: EmitByte(OP_NEGATE); break;
	case TOKEN_BANG: EmitByte(OP_NOT); break;
	default:
		break;
	}
}

static void Binary(bool canAssign)
{
	TokenType opType = parser.previous.type;

	ParseRule* rule = GetRule(opType);
	ParsePrecedence(rule->precedence + 1);

	switch (opType)
	{
	case TOKEN_PLUS: EmitByte(OP_ADD); break;
	case TOKEN_MINUS: EmitByte(OP_SUB); break;
	case TOKEN_STAR: EmitByte(OP_MULT); break;
	case TOKEN_SLASH: EmitByte(OP_MULT); break;
	case TOKEN_EQUAL_EQUAL: EmitByte(OP_EQUAL); break;
	case TOKEN_BANG_EQUAL: EmitByte(OP_NOT_EQUAL); break;
	case TOKEN_GREATER: EmitByte(OP_GREATER); break;
	case TOKEN_GREATER_EQUAL: EmitByte(OP_GREATER_EQUAL); break;
	case TOKEN_LESS: EmitByte(OP_LESS); break;
	case TOKEN_LESS_EQUAL: EmitByte(OP_LESS_EQUAL); break;
	default:
		return; // unreachable
	}
}

static void Literal(bool canAssign)
{
	switch (parser.previous.type)
	{
	case TOKEN_NIL: EmitByte(OP_NIL); break;
	case TOKEN_TRUE: EmitByte(OP_TRUE); break;
	case TOKEN_FALSE: EmitByte(OP_FALSE); break;
	default:
		return; // unreachable
	}
}

static void String(bool canAssign)
{
	EmitConstant(OBJ_VAL(CopyString(parser.previous.start, parser.previous.length)));
}

static bool IdentifiersEqual(Token* a, Token* b)
{
	return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static int ResolveLocal(Token* name)
{
	for (int i = currentCompiler->localsCount - 1; i >= 0; i--)
	{
		Local* local = &currentCompiler->locals[i];
		if (IdentifiersEqual(name, &local->name))
		{
			if (local->depth == -1)
			{
				Error("Can't use variable in it's own initializer.");
			}
			else return i;
		}
	}

	return -1;
}

static void NamedVariable(Token name, bool canAssign)
{
	OpCode getOp, getOpLong, setOp, setOpLong;
	int index = ResolveLocal(&name);
	if (index != -1)
	{
		getOp = OP_GET_LOCAL;
		getOpLong = OP_GET_LOCAL_LONG;
		setOp = OP_SET_LOCAL;
		setOpLong = OP_SET_LOCAL_LONG;
	}
	else
	{
		index = IdentifierConstant(&name);
		getOp = OP_GET_GLOBAL;
		getOpLong = OP_GET_GLOBAL_LONG;
		setOp = OP_SET_GLOBAL;
		setOpLong = OP_SET_GLOBAL_LONG;
	}
	if (canAssign && Match(TOKEN_EQUAL))
	{
		Expression();
		WriteIndexOp(CurrentChunk(), index, name.line, setOp, setOpLong);
	}
	else { WriteIndexOp(CurrentChunk(), index, name.line, getOp, getOpLong); }
}

static void Variable(bool canAssign)
{
	NamedVariable(parser.previous, canAssign);
}

ParseRule rules[] = {
	[TOKEN_LEFT_PAREN] = {Grouping, NULL, PREC_NONE},
	[TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
	[TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
	[TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
	[TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
	[TOKEN_DOT] = {NULL, NULL, PREC_NONE},
	[TOKEN_MINUS] = {Unary, Binary, PREC_TERM},
	[TOKEN_PLUS] = {NULL, Binary, PREC_TERM},
	[TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
	[TOKEN_SLASH] = {NULL, Binary, PREC_FACTOR},
	[TOKEN_STAR] = {NULL, Binary, PREC_FACTOR},
	[TOKEN_BANG] = {Unary,     NULL,   PREC_NONE},
	[TOKEN_BANG_EQUAL] = {NULL,     Binary,   PREC_EQUALITY},
	[TOKEN_EQUAL] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_EQUAL_EQUAL] = {NULL,     Binary,   PREC_EQUALITY},
	[TOKEN_GREATER] = {NULL,     Binary,   PREC_COMPARISON},
	[TOKEN_GREATER_EQUAL] = {NULL,     Binary,   PREC_COMPARISON},
	[TOKEN_LESS] = {NULL, Binary,   PREC_COMPARISON},
	[TOKEN_LESS_EQUAL] = {NULL,     Binary,   PREC_COMPARISON},
	[TOKEN_IDENTIFIER] = {Variable,     NULL,   PREC_NONE},
	[TOKEN_STRING] = {String,     NULL,   PREC_NONE},
	[TOKEN_NUMBER] = {Number,   NULL,   PREC_NONE},
	[TOKEN_AND] = {NULL,     And,   PREC_AND},
	[TOKEN_CLASS] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_ELSE] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_FALSE] = {Literal,     NULL,   PREC_NONE},
	[TOKEN_FOR] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_FUN] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_IF] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_NIL] = {Literal,     NULL,   PREC_NONE},
	[TOKEN_OR] = {NULL,     Or,   PREC_OR},
	[TOKEN_OR] = {NULL,     Or,   PREC_OR},
	[TOKEN_PRINT] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_RETURN] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_SUPER] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_THIS] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_TRUE] = {Literal,     NULL,   PREC_NONE},
	[TOKEN_VAR] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_WHILE] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_ERROR] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_EOF] = {NULL,     NULL,   PREC_NONE},
};

ParseRule* GetRule(TokenType type)
{
	return &rules[type];
}

static void ParsePrecedence(Precedence precedence)
{
	Advance();

	ParseFn prefixRule = GetRule(parser.previous.type)->prefix;
	if (prefixRule == NULL)
	{
		Error("Expect expression.");
		return;
	}

	bool canAssign = precedence <= PREC_ASSIGNMENT;
	prefixRule(canAssign);

	while (precedence <= GetRule(parser.current.type)->precedence)
	{
		Advance();
		ParseFn infixRule = GetRule(parser.previous.type)->infix;
		infixRule(canAssign);
	}

	if (canAssign && Match(TOKEN_EQUAL))
	{
		Error("Invalid assignment target.");
	}
}

static void EndCompiler()
{
	EmitByte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
	if (!parser.hadError)
	{
		DisassembleChunk(CurrentChunk(), "code");
	}
#endif
}

static void Expression()
{
	ParsePrecedence(PREC_ASSIGNMENT);
}

static void PrintStatement()
{
	Expression();
	Consume(TOKEN_SEMICOLON, "Expect ';' after print statement.");
	EmitByte(OP_PRINT);
}

static void ExpressionStatement()
{
	Expression();
	Consume(TOKEN_SEMICOLON, "Expect ';' after expression statement.");
	EmitByte(OP_POP);
}

static void Block()
{
	while (!Check(TOKEN_RIGHT_BRACE) && !Check(TOKEN_EOF))
	{
		Declaration();
	}

	Consume(TOKEN_RIGHT_BRACE, "Expect '}' at end of block.");
}

static void BeginScope()
{
	currentCompiler->currentScopeDepth++;
}

static void EndScope()
{
	int scopeToClose = currentCompiler->currentScopeDepth--;
	int popCount = 0;
	for (int i = currentCompiler->localsCount - 1; i >= 0; i--)
	{
		if (currentCompiler->locals[i].depth != scopeToClose) { break; }
		popCount++;
		currentCompiler->localsCount--;
	}

	assert(popCount <= UINT8_MAX);
	if (popCount > 0)
	{
		EmitByte(OP_POPN);
		EmitByte((uint8_t)popCount);
	}
}

static int EmitJump(OpCode op)
{
	EmitByte(op);
	EmitByte(0xFF);
	EmitByte(0xFF);
	EmitByte(0xFF);
	return CurrentChunk()->count - 3;
}

static void PatchJump(int jumpIndex)
{
	int dest = CurrentChunk()->count;
	int offset = dest - jumpIndex - 3; // exclude the 3 bytes used to store offset

	CurrentChunk()->code[jumpIndex] = offset & 0xFF;
	CurrentChunk()->code[jumpIndex + 1] = (offset >> 8) & 0xFF;
	CurrentChunk()->code[jumpIndex + 2] = (offset >> 16) & 0xFF;
}

static void IfStatement()
{
	Consume(TOKEN_LEFT_PAREN, "Expect '(' before if condition.");
	Expression();
	Consume(TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");

	int falseJump = EmitJump(OP_JUMP_IF_FALSE);

	EmitByte(OP_POP); // Pop condition expression for true case
	Statement();
	int trueJump = EmitJump(OP_JUMP);

	PatchJump(falseJump);

	EmitByte(OP_POP); // Pop condition expression for false case

	if (Match(TOKEN_ELSE))
	{
		Statement();
	}

	PatchJump(trueJump);
}

static void EmitLoopJump(int jumpDestination)
{
	assert(jumpDestination < CurrentChunk()->count);

	EmitByte(OP_JUMP_BACK);

	int offset = CurrentChunk()->count - jumpDestination + 3; // include 3 bytes
	EmitByte(offset & 0xFF);
	EmitByte((offset >> 8) & 0xFF);
	EmitByte((offset >> 16) & 0xFF);
}

static void And(bool canAssign)
{
	int falseJump = EmitJump(OP_JUMP_IF_FALSE);

	EmitByte(OP_POP);
	ParsePrecedence(PREC_AND);

	PatchJump(falseJump);
}

static void Or(bool canAssign)
{
	int trueJump = EmitJump(OP_JUMP_IF_TRUE);

	EmitByte(OP_POP);
	ParsePrecedence(PREC_OR);

	PatchJump(trueJump);
}


static void WhileStatement()
{
	Consume(TOKEN_LEFT_PAREN, "Expect '(' before while condition.");

	int loopJumpLocation = CurrentChunk()->count;
	Expression();

	Consume(TOKEN_RIGHT_PAREN, "Expect ')' after while condition.");

	int falseJump = EmitJump(OP_JUMP_IF_FALSE);
	EmitByte(OP_POP); // true case condition pop
	Statement();
	EmitLoopJump(loopJumpLocation);

	PatchJump(falseJump);
	EmitByte(OP_POP); // false case condition pop
}

// for(init; cnd; incr)
static void ForStatement()
{
	Consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

	bool hasInitializer = false;
	if (!Match(TOKEN_SEMICOLON))
	{
		if (Match(TOKEN_VAR)) 
		{
			hasInitializer = true;
			BeginScope();
			VarDeclaration();
		}
		else {
			/*Expression();
			Consume(TOKEN_SEMICOLON, "Expect ';' after for loop initializer.");*/
			ExpressionStatement();
		}
	}

	int falseJumpIndex = -1;
	int conditionBeginIndex = 0, conditionEndIndex = 0;
	if (!Match(TOKEN_SEMICOLON))
	{
		conditionBeginIndex = CurrentChunk()->count;
		// condition- note for self: NOT ExpressionStatement() b/c we want value of condition to remain on stack
		Expression(); 
		conditionEndIndex = CurrentChunk()->count;

		Consume(TOKEN_SEMICOLON, "Expect ';' after for loop condition.");

		falseJumpIndex = EmitJump(OP_JUMP_IF_FALSE);
		EmitByte(OP_POP);
	}

	// This jump prevents the "increment" from running before the first iteration of the loop
	int conditionAndIncrJump = EmitJump(OP_JUMP);
	int loopJumpLocation = CurrentChunk()->count;

	// Copy condition
	for (int i = conditionBeginIndex; i < conditionEndIndex; i++)
	{
		EmitByte(CurrentChunk()->code[i]);
	}

	// TODO fix this super ugly logic.

	/*
		It's not easy to generate the bytecode for increment and simply append it to the bytecode for the 
		for loop body so this ugly logic is somewhat necessary and simpler. The bytecode for the condition is 
		generated twice. The first time the condition bytecode is generated, it is followed by a jump instruction
		which skips over the second condition bytecode and the increment bytecode, preventing the increment code 
		from running before the loop body. 

		1. condition_bytecode1
		2. jump_if_false 9 // (here '9' is absolute index but in actual bytecode it's stored as offset aka 9-2 = 7 instead).
		3. jump 5	// condition met the first time around, execute loop body and skip increment
		4. condition_bytecode2
		5. jump_if_false 9
		6. increment_bytecode
		7. while_loop_body
		8. jump 4	// jump back to condition_bytecode2 and run increment code
		9. ... 
	*/

	int falseJumpIndex2 = -1;
	if (conditionBeginIndex != conditionEndIndex) {
		falseJumpIndex2 = EmitJump(OP_JUMP_IF_FALSE);
		EmitByte(OP_POP);
	}

	if (!Match(TOKEN_SEMICOLON))
	{
		Expression(); // increment
		EmitByte(OP_POP); // pop incr value
	}

	PatchJump(conditionAndIncrJump);

	Consume(TOKEN_RIGHT_PAREN, "Expect ')' before for loop body.");

	// EmitByte(OP_POP); // pop condition true case
	Statement(); // loop body
	EmitLoopJump(loopJumpLocation);
	
	if (falseJumpIndex > -1)
	{
		PatchJump(falseJumpIndex);
		PatchJump(falseJumpIndex2);
	}

	EmitByte(OP_POP); // pop condition false case


	if (hasInitializer) { EndScope(); }
}

static void Statement()
{
	if (Match(TOKEN_PRINT)) {
		PrintStatement();
	}
	else if (Match(TOKEN_LEFT_BRACE))
	{
		BeginScope();
		Block();
		EndScope();
	}
	else if (Match(TOKEN_IF))
	{
		IfStatement();
	}
	else if (Match(TOKEN_WHILE))
	{
		WhileStatement();
	}
	else if (Match(TOKEN_FOR))
	{
		ForStatement();
	}
	else
	{
		ExpressionStatement();
	}
}

static void Synchronize()
{
	parser.panicMode = false;

	while (parser.current.type != TOKEN_EOF)
	{
		if (parser.previous.type == TOKEN_SEMICOLON) return;

		switch (parser.current.type)
		{
		case TOKEN_CLASS:
		case TOKEN_FUN:
		case TOKEN_VAR:
		case TOKEN_FOR:
		case TOKEN_IF:
		case TOKEN_WHILE:
		case TOKEN_PRINT:
		case TOKEN_RETURN:
			return;
		default:
			;
		}

		Advance();
	}
}

static int IdentifierConstant(Token* identifier)
{
	return AddConstant(CurrentChunk(), OBJ_VAL(CopyString(identifier->start, identifier->length)));
}

static void AddLocal(Token* name)
{
	if (currentCompiler->localsCount >= MAX_LOCALS)
	{
		Error("Too many local variables in function.");
		return;
	}
	Local* local = &currentCompiler->locals[currentCompiler->localsCount++];
	local->name = *name;
	local->depth = -1;
}

static void DeclareVariable()
{
	Token* name = &parser.previous;
	for (int i = currentCompiler->localsCount - 1; i >= 0; i--)
	{
		Local* local = &currentCompiler->locals[i];
		if (local->depth != currentCompiler->currentScopeDepth) { break; }
		if (IdentifiersEqual(&local->name, name))
		{
			Error("Already variable with this name in this scope.");
		}
	}

	AddLocal(name);
}

static int ParseVariable(const char* errorMessage)
{
	Consume(TOKEN_IDENTIFIER, errorMessage);
	if (currentCompiler->currentScopeDepth != 0)
	{
		DeclareVariable();
		return 0; // dummy value, don't add local var to constants array
	}
	return IdentifierConstant(&parser.previous);
}

static void DefineVariable(int global, int line)
{
	if (currentCompiler->currentScopeDepth != 0)
	{
		currentCompiler->locals[currentCompiler->localsCount - 1].depth = currentCompiler->currentScopeDepth;
	}
	else WriteGlobalDeclaration(CurrentChunk(), global, line);
}

static void VarDeclaration()
{
	int global = ParseVariable("Expect variable name.");
	int line = parser.previous.line;

	if (Match(TOKEN_EQUAL))
	{
		Expression();
	}
	else
	{
		EmitByte(OP_NIL);
	}

	Consume(TOKEN_SEMICOLON, "Expect ';' after var declaration.");

	DefineVariable(global, line);
}

static void Declaration()
{
	if (Match(TOKEN_VAR))
	{
		VarDeclaration();
	}
	else
	{
		Statement();
	}

	if (parser.panicMode) { Synchronize(); }
}

bool Compile(const char* source, Chunk* chunk)
{
	InitScanner(source);
	compilingChunk = chunk;
	parser.hadError = parser.panicMode = false;
	Compiler compiler;
	InitCompiler(&compiler);
	Advance();
	while (!Match(TOKEN_EOF))
	{
		Declaration();
	}
	EndCompiler();
	return !parser.hadError;
}
