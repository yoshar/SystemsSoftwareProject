#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler.h"

#define MAX_CODE_LENGTH 1000
#define MAX_SYMBOL_COUNT 100

instruction *code;
int cIndex;
symbol *table;
int tIndex;
//added globals
int lexLevel;
int lIndex;

void emit(int opname, int level, int mvalue);
void addToSymbolTable(int k, char n[], int v, int l, int a, int m);
void printparseerror(int err_code);
void printsymboltable();
void printassemblycode();

//added functions
int program(lexeme *list);
int block(lexeme *list);
int constDec(lexeme *list);
int varDec(lexeme *list);
int procedureDec(lexeme *list);
int statement(lexeme *list);
int condition(lexeme *list);
int expression(lexeme *list);
int term(lexeme *list);
int factor(lexeme *list);
int multiDeclarationCheck(lexeme item);
int findSymbol(lexeme item, int kind);
void mark();

instruction *parse(lexeme *list, int printTable, int printCode)
{
	code = malloc(sizeof(instruction) * MAX_CODE_LENGTH);
	table = malloc(sizeof(symbol) * MAX_SYMBOL_COUNT);
	lexLevel = 0;
	lIndex = 0;

	if(!program(list))
		return NULL;

	/* this line is EXTREMELY IMPORTANT, you MUST uncomment it
		when you test your code otherwise IT WILL SEGFAULT in 
		vm.o THIS LINE IS HOW THE VM KNOWS WHERE THE CODE ENDS
		WHEN COPYING IT TO THE PAS*/
	code[cIndex].opcode = -1;

	if(printTable)
	{
		printsymboltable();
	}

	if(printCode)
	{
		printassemblycode();
	}

	return code;
}

int program(lexeme *list)
{
	emit(7, lexLevel, 0); //JMP
	addToSymbolTable(3, "main", 0, 0, 0, 0);
	lexLevel = -1;
	
	if(!block(list))
		return 1;
	
	if(list[lIndex].type != periodsym)
	{
		printparseerror(1);
		return 1;
	}

	emit(9, 0, 3); //HALT

	for(int i = 0; i < 3000; i++)
	{
		if(list[i].type == callsym)
		{
			code[i].m = table[code[i].m].addr;
		}
	}
	code[0].m = table[0].addr;
	
	return 0;
}

int block(lexeme *list)
{
	lexLevel++;
	int index = tIndex - 1;
	
	if(!constDec(list))
		return 1;
	
	int x = varDec(list);
	if(x == -1)
		return 1;

	if(!procedureDec(list))
		return 1;
	
	table[index].addr = cIndex * 3;
	if(lexLevel == 0)
	{
		emit(6, lexLevel, x); //INC
	}
	else	{
		emit(6, lexLevel, x + 3); //INC
	}

	if(!statement(list))
		return 1;
	
	mark();
	lexLevel--;

	return 0;
}

int constDec(lexeme *list)
{
	if(list[lIndex].type == constsym)
	{
		do
		{
			lIndex++;
			if(list[lIndex].type != identsym)
			{
				printparseerror(2);
				return 1;
			}

			int symidx = multiDeclarationCheck(list[lIndex]);

			if(symidx != -1)
			{
				printparseerror(2);
				return 1;
			}

			char name[12];
			strcpy(name, list[lIndex].name);

			lIndex++;
			if(list[lIndex].type != assignsym)
			{
				printparseerror(2);
				return 1;
			}

			lIndex++;
			if(list[lIndex].type != numbersym)
			{
				printparseerror(2);
				return 1;
			}

			addToSymbolTable(1, name, list[lIndex].value, lexLevel, 0, 0);

			lIndex++;
		}while(list[lIndex].type == commasym);
		
		if(list[lIndex].type != semicolonsym)
		{
			if(list[lIndex].type == identsym)
			{
				printparseerror(13);
				return 1;
			}
			else
			{
				printparseerror(14);
				return 1;
			}
		}
		lIndex++;
	}

	return 0;
}

int varDec(lexeme *list)
{
	int numVars = 0;
	if(list[lIndex].type == varsym)
	{
		do
		{
			numVars++;
			lIndex++;
			if(list[lIndex].type != identsym)
			{
				printparseerror(3);
				return -1;
			}

			int symidx = multiDeclarationCheck(list[lIndex]);

			if(symidx != -1)
			{
				printparseerror(13);
				return -1;
			}

			if(lexLevel == 0)
			{
				addToSymbolTable(2, list[lIndex].name, 0, lexLevel, numVars - 1, 0);
			}
			else
			{
				addToSymbolTable(2, list[lIndex].name, 0, lexLevel, numVars + 2, 0);
			}

			lIndex++;
		}while(list[lIndex].type == commasym);

		if(list[lIndex].type != semicolonsym)
		{
			if(list[lIndex].type == identsym)
			{
				printparseerror(3);
				return -1;
			}
			else
			{
				printparseerror(14);
				return -1;
			}
		}

		lIndex++;
	}
	return numVars;
}

int procedureDec(lexeme *list)
{
	while(list[lIndex].type == procsym)
	{
		lIndex++;
		if(list[lIndex].type != identsym)
		{
			printparseerror(4);
			return 1;
		}
		
		int symidx = multiDeclarationCheck(list[lIndex]);
		if(symidx != -1)
		{
			printparseerror(4);
			return 1;
		}
		addToSymbolTable(3, list[lIndex].name, 0, lexLevel, 0, 0);

		lIndex++;
		if(list[lIndex].type != semicolonsym)
		{
			printparseerror(4);
			return 1;
		}

		lIndex++;
		if(!block(list))
			return 1;

		if(list[lIndex].type != semicolonsym)
		{
			printparseerror(4);
			return 1;
		}

		lIndex++;
		emit(2, lexLevel, 0); //RTN
	}

	return 0;
}

int statement(lexeme *list)
{
	if(list[lIndex].type == identsym)
	{
		int symidx = findSymbol(list[lIndex], 2);
		if(symidx == -1)
		{
			if(findSymbol(list[lIndex], 1) != findSymbol(list[lIndex], 3))
			{
				printparseerror(20);
				return 1;
			}
			else
			{
				printparseerror(20);
				return 1;
			}
		}

		lIndex++;
		if(list[lIndex].type != assignsym)
		{
			printparseerror(5);
			return 1;
		}

		lIndex++;
		if(!expression(list))
			return 1;

		emit(4, lexLevel - table[symidx].level, table[symidx].addr); //STO
		return 0;
	}

	if(list[lIndex].type == beginsym)
	{
		do
		{
			lIndex++;
			if(!statement(list))
				return 1;
		}
		while(list[lIndex].type == semicolonsym);
		
		if(list[lIndex].type != endsym)
		{
			if(list[lIndex].type == identsym || list[lIndex].type == beginsym || list[lIndex].type == ifsym || list[lIndex].type == whilesym || list[lIndex].type == readsym || list[lIndex].type == writesym || list[lIndex].type == callsym)
			{
				printparseerror(15);
				return 1;
			}
			else
			{
				printparseerror(16);
				return 1;
			}
		}
		lIndex++;
		return 0;
	}

	if(list[lIndex].type == ifsym)
	{
		lIndex++;
		if(!condition(list))
			return 1;

		int jpcidx = cIndex;
		emit(8, lexLevel, jpcidx); //JPC

		if(list[lIndex].type != thensym)
		{
			printparseerror(8);
			return 1;
		}

		lIndex++;
		if(!statement(list))
			return 1;

		if(list[lIndex].type == elsesym)
		{
			jpcidx = cIndex;
			emit(7, lexLevel, jpcidx);
			code[jpcidx].m = cIndex * 3;
			cIndex++;

			if(!statement(list))
				return 1;
			
			code[jpcidx].m = cIndex * 3;
		}
		else
		{
			code[jpcidx].m = cIndex * 3;
		}

		return 0;
	}

	if(list[lIndex].type == whilesym)
	{
		lIndex++;
		int loopidx = cIndex;
		if(!condition(list))
			return 1;

		if(list[lIndex].type != dosym)
		{
			printparseerror(9);
			return 1;
		}

		lIndex++;
		int jpcidx = cIndex;
		
		emit(8, lexLevel, jpcidx); //JPC

		if(!statement(list))
			return 1;
		
		emit(7, lexLevel, loopidx * 3);
		code[jpcidx].m = cIndex * 3;

		return 0;
	}

	if(list[lIndex].type == readsym)
	{
		lIndex++;
		if(list[lIndex].type != identsym)
		{
			printparseerror(20);
			return 1;
		}

		int symidx = findSymbol(list[lIndex], 2);

		if(symidx == -1)
		{
			if(findSymbol(list[lIndex], 1) != findSymbol(list[lIndex], 3))
			{
				printparseerror(20);
				return 1;
			}
			else
			{
				printparseerror(20);
				return 1;
			}
		}

		lIndex++;
		emit(9, lexLevel, 2); //READ
		emit(4, lexLevel - table[symidx].level, table[symidx].addr); //STO
		
		return 0;
	}

	if(list[lIndex].type == writesym)
	{
		lIndex++;
		
		if(!expression(list))
			return 1;
		
		emit(9, lexLevel, 1); //WRITE

		return 0;
	}

	if(list[lIndex].type == callsym)
	{
		lIndex++;
		int symidx = findSymbol(list[lIndex], 3);
		if(symidx == -1)
		{
			if(findSymbol(list[lIndex], 1) != findSymbol(list[lIndex], 2))
			{
				printparseerror(7);
				return 1;
			}
			else
			{
				printparseerror(20);
				return 1;
			}
		}

		lIndex++;
		emit(5, lexLevel - table[symidx].level, symidx); //CAL
	}
}

int condition(lexeme *list)
{
	if(list[lIndex].type == oddsym)
	{
		lIndex++;
		if(!expression(list))
			return 1;
		emit(2, lexLevel, 6); //ODD
	}
	else
	{
		if(!expression(list))
			return 1;
		
		if(list[lIndex].type == eqlsym)
		{
			lIndex++;
			if(!expression(list))
				return 1;
			emit(2, lexLevel, 8); //EQL
		}
		else if(list[lIndex].type == neqsym)
		{
			lIndex++;
			if(!expression(list))
				return 1;
			emit(2, lexLevel, 9); //NEQ
		}
		else if(list[lIndex].type == lsssym)
		{
			lIndex++;
			if(!expression(list))
				return 1;
			emit(2, lexLevel, 10); //LSS
		}
		else if(list[lIndex].type == leqsym)
		{
			lIndex++;
			if(!expression(list))
				return 1;
			emit(2, lexLevel, 11); //LEQ
		}
		else if(list[lIndex].type == gtrsym)
		{
			lIndex++;
			if(!expression(list))
				return 1;
			emit(2, lexLevel, 12); //GTR
		}
		else if(list[lIndex].type == geqsym)
		{
			lIndex++;
			if(!expression(list))
				return 1;
			emit(2, lexLevel, 13); //GEQ
		}
		else
		{
			printparseerror(20);
			return 1;
		}
	}

	return 0;
}

int expression(lexeme *list)
{
	if(list[lIndex].type == subsym)
	{
		lIndex++;
		if(!term(list))
			return 1;
		emit(2, lexLevel, 1); //NEG
		while(list[lIndex].type == addsym || list[lIndex].type == subsym)
		{
			if(list[lIndex].type == addsym)
			{
				lIndex++;
				if(!term(list))
					return 1;
				emit(2, lexLevel, 2); //ADD
			}
			else
			{
				lIndex++;
				if(!term(list))
					return 1;
				emit(2, lexLevel, 3); //SUB
			}
		}
	}
	else
	{
		if(list[lIndex].type == addsym)
		{
			lIndex++;
		}
		if(!term(list))
					return 1;
		while(list[lIndex].type == addsym || list[lIndex].type == subsym)
		{
			if(list[lIndex].type == addsym)
			{
				lIndex++;
				if(!term(list))
					return 1;
				emit(2, lexLevel, 2); //ADD
			}
			else
			{
				lIndex++;
				if(!term(list))
					return 1;
				emit(2, lexLevel, 3); //SUB
			}
		}
	}

	if(list[lIndex].type == lparensym || list[lIndex].type == identsym || list[lIndex].type == numbersym || list[lIndex].type == oddsym)
	{
		printparseerror(12);
		return 1;
	}

	return 0;
}

int term(lexeme *list)
{
	if(!factor(list))
		return 1;
	
	while(list[lIndex].type == multsym || list[lIndex].type == divsym || list[lIndex].type == modsym)
	{
		if(list[lIndex].type == multsym)
		{
			lIndex++;
			if(!factor(list))
				return 1;
			emit(2, lexLevel, 4); //MUL
		}
		else if(list[lIndex].type == divsym)
		{
			lIndex++;
			if(!factor(list))
				return 1;
			emit(2, lexLevel, 5); //DIV
		}
		else
		{
			lIndex++;
			if(!factor(list))
				return 1;
			emit(2, lexLevel, 7); //MOD
		}
	}

	return 0;
}

int factor(lexeme *list)
{
	if(list[lIndex].type == identsym)
	{
		int symidxVar = findSymbol(list[lIndex], 2);
		int symidxConst = findSymbol(list[lIndex], 1);

		if(symidxVar == -1 && symidxConst == -1)
		{
			if(findSymbol(list[lIndex], 3) != -1)
			{
				printparseerror(20);
				return 1;
			}
			else
			{
				printparseerror(20);
				return 1;
			}
		}

		if(symidxVar == -1)
		{
			emit(1, lexLevel, table[symidxConst].val); //LIT
		}
		else if(symidxConst == -1 || table[symidxVar].level > table[symidxConst].level)
		{
			emit(3, lexLevel - table[symidxVar].level, table[symidxVar].addr);
		}
		else
		{
			emit(1, lexLevel, table[symidxConst].val); //LIT
		}

		lIndex++;
	}
	else if(list[lIndex].type == numbersym)
	{
		emit(1, lexLevel, list[lIndex].value); //LIT
		lIndex++;
	}
	else if(list[lIndex].type == lparensym)
	{
		lIndex++;
		if(!expression(list))
				return 1;
		
		if(list[lIndex].type != rparensym)
		{
			printparseerror(12);
			return 1;
		}

		lIndex++;
	}
	else
	{
		printparseerror(20);
		return 1;
	}

	return 0;
}

int multiDeclarationCheck(lexeme item)
{
	int i;
	for(i = 0; i < MAX_SYMBOL_COUNT; i++)
	{
		if(strcmp(item.name, table[i].name) && table[i].mark == 0 && table[i].level == lexLevel)
		{
			return i;
		}
	}

	return -1;
}

int findSymbol(lexeme item, int kind)
{
	int i;
	int level = 0;
	int retval = -1;
	for(i = 0; i < MAX_SYMBOL_COUNT; i++)
	{
		if(strcmp(item.name, table[i].name) && table[i].kind == kind && table[i].mark == 0)
		{
			if(level < table[i].level)
			{
				level = table[i].level;
				retval = i;
			}
		}
	}

	return retval;
}

void mark()
{
	for(int i = MAX_SYMBOL_COUNT - 1; i >= 0; i--)
	{
		if(table[i].mark == 0 && table[i].level < lexLevel)
		{
			return;
		}

		if(table[i].level == lexLevel)
		{
			table[i].mark = 1;
		}
	}
}

void emit(int opname, int level, int mvalue)
{
	code[cIndex].opcode = opname;
	code[cIndex].l = level;
	code[cIndex].m = mvalue;
	cIndex++;
}

void addToSymbolTable(int k, char n[], int v, int l, int a, int m)
{
	table[tIndex].kind = k;
	strcpy(table[tIndex].name, n);
	table[tIndex].val = v;
	table[tIndex].level = l;
	table[tIndex].addr = a;
	table[tIndex].mark = m;
	tIndex++;
}


void printparseerror(int err_code)
{
	switch (err_code)
	{
		case 1:
			printf("Parser Error: Program must be closed by a period\n");
			break;
		case 2:
			printf("Parser Error: Constant declarations should follow the pattern 'ident := number {, ident := number}'\n");
			break;
		case 3:
			printf("Parser Error: Variable declarations should follow the pattern 'ident {, ident}'\n");
			break;
		case 4:
			printf("Parser Error: Procedure declarations should follow the pattern 'ident ;'\n");
			break;
		case 5:
			printf("Parser Error: Variables must be assigned using :=\n");
			break;
		case 6:
			printf("Parser Error: Only variables may be assigned to or read\n");
			break;
		case 7:
			printf("Parser Error: call must be followed by a procedure identifier\n");
			break;
		case 8:
			printf("Parser Error: if must be followed by then\n");
			break;
		case 9:
			printf("Parser Error: while must be followed by do\n");
			break;
		case 10:
			printf("Parser Error: Relational operator missing from condition\n");
			break;
		case 11:
			printf("Parser Error: Arithmetic expressions may only contain arithmetic operators, numbers, parentheses, constants, and variables\n");
			break;
		case 12:
			printf("Parser Error: ( must be followed by )\n");
			break;
		case 13:
			printf("Parser Error: Multiple symbols in variable and constant declarations must be separated by commas\n");
			break;
		case 14:
			printf("Parser Error: Symbol declarations should close with a semicolon\n");
			break;
		case 15:
			printf("Parser Error: Statements within begin-end must be separated by a semicolon\n");
			break;
		case 16:
			printf("Parser Error: begin must be followed by end\n");
			break;
		case 17:
			printf("Parser Error: Bad arithmetic\n");
			break;
		case 18:
			printf("Parser Error: Confliciting symbol declarations\n");
			break;
		case 19:
			printf("Parser Error: Undeclared identifier\n");
			break;
		default:
			printf("Implementation Error: unrecognized error code\n");
			break;
	}
	
	free(code);
	free(table);
}

void printsymboltable()
{
	int i;
	printf("Symbol Table:\n");
	printf("Kind | Name        | Value | Level | Address | Mark\n");
	printf("---------------------------------------------------\n");
	for (i = 0; i < tIndex; i++)
		printf("%4d | %11s | %5d | %5d | %5d | %5d\n", table[i].kind, table[i].name, table[i].val, table[i].level, table[i].addr, table[i].mark); 
	
	free(table);
	table = NULL;
}

void printassemblycode()
{
	int i;
	printf("Line\tOP Code\tOP Name\tL\tM\n");
	for (i = 0; i < cIndex; i++)
	{
		printf("%d\t", i);
		printf("%d\t", code[i].opcode);
		switch (code[i].opcode)
		{
			case 1:
				printf("LIT\t");
				break;
			case 2:
				switch (code[i].m)
				{
					case 0:
						printf("RTN\t");
						break;
					case 1:
						printf("NEG\t");
						break;
					case 2:
						printf("ADD\t");
						break;
					case 3:
						printf("SUB\t");
						break;
					case 4:
						printf("MUL\t");
						break;
					case 5:
						printf("DIV\t");
						break;
					case 6:
						printf("ODD\t");
						break;
					case 7:
						printf("MOD\t");
						break;
					case 8:
						printf("EQL\t");
						break;
					case 9:
						printf("NEQ\t");
						break;
					case 10:
						printf("LSS\t");
						break;
					case 11:
						printf("LEQ\t");
						break;
					case 12:
						printf("GTR\t");
						break;
					case 13:
						printf("GEQ\t");
						break;
					default:
						printf("err\t");
						break;
				}
				break;
			case 3:
				printf("LOD\t");
				break;
			case 4:
				printf("STO\t");
				break;
			case 5:
				printf("CAL\t");
				break;
			case 6:
				printf("INC\t");
				break;
			case 7:
				printf("JMP\t");
				break;
			case 8:
				printf("JPC\t");
				break;
			case 9:
				switch (code[i].m)
				{
					case 1:
						printf("WRT\t");
						break;
					case 2:
						printf("RED\t");
						break;
					case 3:
						printf("HAL\t");
						break;
					default:
						printf("err\t");
						break;
				}
				break;
			default:
				printf("err\t");
				break;
		}
		printf("%d\t%d\n", code[i].l, code[i].m);
	}
	if (table != NULL)
		free(table);
}