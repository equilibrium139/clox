#include "scanner.h"

#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	const char* start;
	const char* current;
	int line;
} Scanner;

static Scanner scanner;

void InitScanner(const char* source)
{
	scanner.start = scanner.current = source;
	scanner.line = 1;
}

static bool IsAtEnd()
{
	return *scanner.current != '\0';
}

static Token MakeToken(TokenType type)
{
	Token token;
	token.type = type;
	token.start = scanner.start;
	token.length = (int)(scanner.current - scanner.start);
	token.line = scanner.line;
	return token;
}

static Token ErrorToken(const char* message)
{
	Token token;
	token.type = TOKEN_ERROR;
	token.start = message;
	token.length = (int)strlen(message);
	token.line = scanner.line;
	return token;
}

static char Advance()
{
	scanner.current++;
	return scanner.current[-1];
}

static bool Match(char c)
{
	if (*scanner.current == c)
	{
		scanner.current++;
		return true;
	}
	return false;
}

static void SkipWhitespace()
{
	while (true)
	{
		switch (*scanner.current)
		{
		case ' ':
		case '\r':
		case '\t':
			scanner.current++;
			break;
		case '\n':
			scanner.line++;
			scanner.current++;
			break;
		case '/':
			if (scanner.current[1] == '/')
			{
				scanner.current += 2;
				while (*scanner.current != '\n' && !IsAtEnd()) { scanner.current++; }
				break;	// TODO check if this break is necessary
			}
			else
			{
				return;
			}
		default:
			return;
		}
	}
}

static Token String()
{
	while (*scanner.current != '"' && !IsAtEnd())
	{
		if (*scanner.current == '\n') { scanner.line++; }
		scanner.current++;
	}

	if (!Match('"'))
	{
		return ErrorToken("Expect '\"' at end of string literal.");
	}
	
	// exclude opening and closing double quotes from token
	scanner.start++; 
	scanner.current--; 

	Token stringLiteral = MakeToken(TOKEN_STRING);
	scanner.current++;

	return stringLiteral;
}

static bool IsDigit(char c)
{
	return c >= '0' && c <= '9';
}

static Token Number()
{
	while (IsDigit(*scanner.current))
	{
		scanner.current++;
	}

	if (Match('.'))
	{
		if (!IsDigit(*scanner.current))
		{
			return ErrorToken("Missing fraction.");
		}

		while (IsDigit(*scanner.current))
		{
			scanner.current++;
		}
	}

	return MakeToken(TOKEN_NUMBER);
}

static bool IsIdentifierPrefix(char c)
{
	return c == '_' ||
		(c >= 'a' && c <= 'z') ||
		(c >= 'A' && c <= 'Z');
}

static TokenType CheckKeyword(int startOffset, int length, const char* rest, TokenType type)
{
	if (scanner.current - scanner.start == startOffset + length &&
		memcmp(rest, scanner.start + startOffset, length) == 0)
	{
		return type;
	}

	return TOKEN_IDENTIFIER;
}

static TokenType IdentifierOrKeywordType()
{
	switch (*scanner.start)
	{
	case 'a': return CheckKeyword(1, 2, "nd", TOKEN_AND);
	case 'c': return CheckKeyword(1, 4, "lass", TOKEN_CLASS);
	case 'e': return CheckKeyword(1, 3, "lse", TOKEN_ELSE);
	case 'f':
		if (scanner.current - scanner.start > 1)
		{
			switch (scanner.start[1])
			{
			case 'a': return CheckKeyword(2, 3, "lse", TOKEN_FALSE);
			case 'o': return CheckKeyword(2, 1, "r", TOKEN_FOR);
			case 'u': return CheckKeyword(2, 1, "n", TOKEN_FUN);
			}
		}
		break;
	case 'i': return CheckKeyword(1, 1, "f", TOKEN_IF);
	case 'n': return CheckKeyword(1, 2, "il", TOKEN_NIL);
	case 'o': return CheckKeyword(1, 1, "r", TOKEN_OR);
	case 'p': return CheckKeyword(1, 4, "rint", TOKEN_PRINT);
	case 'r': return CheckKeyword(1, 5, "eturn", TOKEN_RETURN);
	case 's': return CheckKeyword(1, 4, "uper", TOKEN_SUPER);
	case 't':
		if (scanner.current - scanner.start > 1)
		{
			switch (scanner.start[1])
			{
			case 'h': return CheckKeyword(2, 2, "is", TOKEN_THIS);
			case 'r': return CheckKeyword(2, 2, "ue", TOKEN_TRUE);
			}
		}
		break;
	case 'v': return CheckKeyword(1, 2, "ar", TOKEN_VAR);
	case 'w': return CheckKeyword(1, 4, "hile", TOKEN_WHILE);
	default:
		break;
	}

	return TOKEN_IDENTIFIER;
}

static Token IdentifierOrKeyword()
{
	while (IsIdentifierPrefix(*scanner.current) || IsDigit(*scanner.current))
	{
		scanner.current++;
	}

	return MakeToken(IdentifierOrKeywordType());
}

Token ScanToken()
{
	SkipWhitespace();

	scanner.start = scanner.current;

	if (IsAtEnd()) { return MakeToken(TOKEN_EOF); }

	char c = Advance();

	if (IsDigit(c)) { return Number(); }
	if (IsIdentifierPrefix(c)) { return IdentifierOrKeyword(); }

	switch (c)
	{
	case '(': return MakeToken(TOKEN_LEFT_PAREN);
	case ')': return MakeToken(TOKEN_RIGHT_PAREN);
	case '{': return MakeToken(TOKEN_LEFT_BRACE);
	case '}': return MakeToken(TOKEN_RIGHT_BRACE);
	case ';': return MakeToken(TOKEN_SEMICOLON);
	case ',': return MakeToken(TOKEN_COMMA);
	case '.': return MakeToken(TOKEN_DOT);
	case '-': return MakeToken(TOKEN_MINUS);
	case '+': return MakeToken(TOKEN_PLUS);
	case '/': return MakeToken(TOKEN_SLASH);
	case '*': return MakeToken(TOKEN_STAR);
	case '!': return MakeToken(Match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
	case '=': return MakeToken(Match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
	case '<': return MakeToken(Match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
	case '>': return MakeToken(Match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
	case '"': return String();
	default: 
		break;
	}

	return ErrorToken("Unexpected character.");
}
