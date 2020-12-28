#include "compiler.h"

#include "common.h"
#include "object.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CORE
#include "debug.h
#endif // DEBUG_PRINT_CORE

#include <stdio.h>
#include <stdlib.h>

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

typedef void (*ParseFn)();

typedef struct
{
	ParseFn prefix;
	ParseFn infix;
	Precedence precedence;
} ParseRule;

Parser parser;

Chunk* compilingChunk;

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

static void EmitByte(uint8_t byte)
{
	WriteChunk(CurrentChunk(), byte, parser.previous.line);
}

static void EmitConstant(Value value)
{
	WriteConstant(CurrentChunk(), value, parser.previous.line);
}

static void Number()
{
	double value = strtod(parser.previous.start, NULL);
	EmitConstant(NUMBER_VAL(value));
}

static void Expression();
static ParseRule* GetRule(TokenType type);
static void ParsePrecedence(Precedence);

static void Grouping()
{
	Expression();
	Consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void Unary()
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

static void Binary()
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

static void Literal()
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

static void String()
{
	EmitConstant(OBJ_VAL(ToObjString(parser.previous.start, parser.previous.length)));
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
	[TOKEN_LESS_EQUAL] = {Binary,     NULL,   PREC_COMPARISON},
	[TOKEN_IDENTIFIER] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_STRING] = {String,     NULL,   PREC_NONE},
	[TOKEN_NUMBER] = {Number,   NULL,   PREC_NONE},
	[TOKEN_AND] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_CLASS] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_ELSE] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_FALSE] = {Literal,     NULL,   PREC_NONE},
	[TOKEN_FOR] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_FUN] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_IF] = {NULL,     NULL,   PREC_NONE},
	[TOKEN_NIL] = {Literal,     NULL,   PREC_NONE},
	[TOKEN_OR] = {NULL,     NULL,   PREC_NONE},
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

	prefixRule();

	while (precedence <= GetRule(parser.current.type)->precedence)
	{
		Advance();
		ParseFn infixRule = GetRule(parser.previous.type)->infix;
		infixRule();
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

bool Compile(const char* source, Chunk* chunk)
{
	InitScanner(source);
	compilingChunk = chunk;
	parser.hadError = parser.panicMode = false;
	Advance();
	Expression();
	Consume(TOKEN_EOF, "Expect end of expression.");
	EndCompiler();
	return !parser.hadError;
}
