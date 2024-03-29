#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "compiler-1.h"

#define MAX_CODE_LENGTH 1000
#define MAX_SYMBOL_COUNT 100

instruction *code;
int cIndex;
symbol *table;
int tIndex;

//added globals
int error;
int lexLevel;
int lIndex;

void emit(int opname, int level, int mvalue);
void addToSymbolTable(int k, char n[], int v, int l, int a, int m);
void printparseerror(int err_code);
void printsymboltable();
void printassemblycode();

//added functions
void program(lexeme *list);
void block(lexeme *list);
void constDec(lexeme *list);
int varDec(lexeme *list);
void procedureDec(lexeme *list);
void statement(lexeme *list);
void condition(lexeme *list);
void expression(lexeme *list);
void term(lexeme *list);
void factor(lexeme *list);
int multiDeclarationCheck(lexeme item);
int findSymbol(lexeme item, int kind);
void mark();

instruction *parse(lexeme *list, int printTable, int printCode)
{
	code = malloc(sizeof(instruction) * MAX_CODE_LENGTH);
	table = malloc(sizeof(symbol) * MAX_SYMBOL_COUNT);
	lexLevel = 0;
	lIndex = 0;
	error = 0;

	//Starts compilation, the if statement check for the error flag in order to safely stop execution, once an eerror is encountered
	//the error flag will be flipped then it will begin returning up the chain of fucntion calls
	program(list);
	if(error)
		return NULL;

	/* this line is EXTREMELY IMPORTANT, you MUST uncomment it
		when you test your code otherwise IT WILL SEGFAULT in 
		vm.o THIS LINE IS HOW THE VM KNOWS WHERE THE CODE ENDS
		WHEN COPYING IT TO THE PAS*/
	code[cIndex].opcode = -1;

	//checks for print flags
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

void program(lexeme *list)
{
	//makes space for main and will set the correct value later
	emit(7, 0, 0); //JMP
	addToSymbolTable(3, "main", 0, 0, 0, 0);
	lexLevel = -1;
	
	//starts the block that will contain the whole program
	block(list);
	if(error)
		return;

	//program must end with a period
	if(list[lIndex].type != periodsym)
	{
		printparseerror(1);
		error = 1;
		return;
	}

	emit(9, 0, 3); //HALT

	/*for(int i = 0; i < cIndex; i++)
	{
		if(code[i].opcode == 5)
		{
			code[i].m = table[code[i].m].addr;
		}
	}*/

	//sets correct main address
	code[0].m = table[0].addr;
	
	return;
}

void block(lexeme *list)
{
	//sets scope for the block
	lexLevel++;
	int index = tIndex - 1;
	
	//declares variables/consts/procedures at begginng of the block
	constDec(list);
	if(error)
		return;

	int x = varDec(list);
	if(error)
		return;

	procedureDec(list);
	if(error)
		return;
	
	table[index].addr = cIndex * 3;
	//makes space for each variable
	if(lexLevel == 0)
	{
		emit(6, 0, x); //INC
	}
	else	{
		emit(6, 0, x + 4); //INC
	}

	//now that declarations are done go to actual statements
	statement(list);
	if(error)
		return;
	
	//mark all declarations that are going out of scope
	mark();
	lexLevel--;
}

void constDec(lexeme *list)
{
	if(list[lIndex].type == constsym)
	{
		do
		{
			lIndex++;
			if(list[lIndex].type != identsym)
			{
				printparseerror(2);
				error = 1;
				return;
			}

			//checks to ensure that there are not muliple identifiers with the same name
			int symidx = multiDeclarationCheck(list[lIndex]);

			if(symidx != -1)
			{
				printparseerror(2);
				error = 1;
				return;
			}

			char name[12];
			strcpy(name, list[lIndex].name);

			lIndex++;
			if(list[lIndex].type != assignsym)
			{
				printparseerror(2);
				error = 1;
				return;
			}

			lIndex++;
			if(list[lIndex].type != numbersym)
			{
				printparseerror(2);
				error = 1;
				return;
			}

			addToSymbolTable(1, name, list[lIndex].value, lexLevel, 0, 0);

			lIndex++;
		}while(list[lIndex].type == commasym);
		
		if(list[lIndex].type != semicolonsym)
		{
			if(list[lIndex].type == identsym)
			{
				printparseerror(13);
				error = 1;
				return;
			}
			else
			{
				printparseerror(14);
				error = 1;
				return;
			}
		}
		lIndex++;
	}

	return;
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
				error = 1;
				return -1;
			}

			int symidx = multiDeclarationCheck(list[lIndex]);

			if(symidx != -1)
			{
				printparseerror(18);
				error = 1;
				return -1;
			}

			if(lexLevel == 0)
			{
				addToSymbolTable(2, list[lIndex].name, 0, lexLevel, numVars - 1, 0);
			}
			else
			{
				addToSymbolTable(2, list[lIndex].name, 0, lexLevel, numVars + 3, 0);
			}

			lIndex++;
		}while(list[lIndex].type == commasym);

		if(list[lIndex].type != semicolonsym)
		{
			if(list[lIndex].type == identsym)
			{
				printparseerror(13);
				error = 1;
				return -1;
			}
			else
			{
				printparseerror(14);
				error = 1; 
				return -1;
			}
		}

		lIndex++;
	}
	return numVars;
}

void procedureDec(lexeme *list)
{
	while(list[lIndex].type == procsym)
	{
		lIndex++;
		if(list[lIndex].type != identsym)
		{
			printparseerror(4);
			error = 1;
			return;
		}
		
		int symidx = multiDeclarationCheck(list[lIndex]);
		if(symidx != -1)
		{
			printparseerror(4);
			error = 1;
			return;
		}
		addToSymbolTable(3, list[lIndex].name, 0, lexLevel, 0, 0);

		lIndex++;
		if(list[lIndex].type != semicolonsym)
		{
			printparseerror(4);
			error = 1;
			return;
		}

		//starts the block that will be the logic for the procedure
		lIndex++;
		block(list);
		if(error)
			return;

		if(list[lIndex].type != semicolonsym)
		{
			printparseerror(14);
			error = 1;
			return;
		}

		lIndex++;
		emit(2, 0, 0); //RTN
	}

	return;
}

void statement(lexeme *list)
{
	//identifiers
	if(list[lIndex].type == identsym)
	{
		//look for an identifier of this name and type
		int symidx = findSymbol(list[lIndex], 2);
		if(symidx == -1)
		{
			if(findSymbol(list[lIndex], 1) != findSymbol(list[lIndex], 3))
			{
				printparseerror(6);
				error = 1;
				return;
			}
			else
			{
				printparseerror(19);
				error = 1;
				return;
			}
		}

		lIndex++;
		if(list[lIndex].type != assignsym)
		{
			printparseerror(5);
			error = 1;
			return;
		}

		//math expression that will be stored in the id
		lIndex++;
		expression(list);
		if(error)
			return;

		emit(4, lexLevel - table[symidx].level, table[symidx].addr); //STO
		return;
	}

	//begins an extended series of statements, must be closed with od
	if(list[lIndex].type == dosym)
	{
		do
		{
			lIndex++;
			statement(list);
			if(error)
				return;
		}
		while(list[lIndex].type == semicolonsym);
		
		if(list[lIndex].type != odsym)
		{
			if(list[lIndex].type == identsym || list[lIndex].type == dosym || list[lIndex].type == whensym || list[lIndex].type == whilesym || list[lIndex].type == readsym || list[lIndex].type == writesym || list[lIndex].type == callsym)
			{
				printparseerror(15);
				error = 1;
				return;
			}
			else
			{
				printparseerror(16);
				error = 1;
				return;
			}
		}
		lIndex++;
		return;
	}

	if(list[lIndex].type == whensym)
	{
		lIndex++;
		condition(list);
		if(error)
			return;

		//creates a jpc which will be fixed later
		int jpcidx = cIndex;
		emit(8, 0, 0); //JPC

		if(list[lIndex].type != dosym)
		{
			printparseerror(8);
			error = 1;
			return;
		}

		lIndex++;
		statement(list);
		if(error)
			return;
		
		//elsedo is optional is basically the same thing as if but changes the original jpc index
		if(list[lIndex].type == elsedosym)
		{
			int jmpidx = cIndex;
			emit(7, 0, 0); //JMP
			code[jpcidx].m = cIndex * 3;

			lIndex++;
			statement(list);
			if(error)
				return;

			code[jmpidx].m = cIndex * 3;
		}
		else
		{
			code[jpcidx].m = cIndex * 3;
		}

		return;
	}

	//while is similar to if but can repeat
	if(list[lIndex].type == whilesym)
	{
		lIndex++;
		int loopidx = cIndex;
		condition(list);
		if(error)
			return;

		if(list[lIndex].type != dosym)
		{
			printparseerror(9);
			error = 1;
			return;
		}

		lIndex++;
		int jpcidx = cIndex;
		
		emit(8, 0, 0); //JPC

		statement(list);
		if(error)
			return;

		emit(7, 0, loopidx * 3); //JMP
		code[jpcidx].m = cIndex * 3;

		return;
	}

	if(list[lIndex].type == readsym)
	{
		lIndex++;
		if(list[lIndex].type != identsym)
		{
			printparseerror(6);
			error = 1;
			return;
		}

		//looks for the symbol to read
		int symidx = findSymbol(list[lIndex], 2);

		if(symidx == -1)
		{
			if(findSymbol(list[lIndex], 1) != findSymbol(list[lIndex], 3))
			{
				printparseerror(6);
				error = 1;
				return;
			}
			else
			{
				printparseerror(19);
				error = 1;
				return;
			}
		}

		//whem it finds that symbol you read it and store it
		lIndex++;
		emit(9, 0, 2); //READ
		emit(4, lexLevel - table[symidx].level, table[symidx].addr); //STO
		
		return;
	}

	if(list[lIndex].type == writesym)
	{
		lIndex++;
		
		expression(list);
		if(error)
			return;
		
		emit(9, 0, 1); //WRITE

		return;
	}

	if(list[lIndex].type == callsym)
	{
		lIndex++;

		//looks for the given proc id 
		int symidx = findSymbol(list[lIndex], 3);
		if(symidx == -1)
		{
			if(findSymbol(list[lIndex], 1) != findSymbol(list[lIndex], 2))
			{
				printparseerror(7);
				error = 1;
				return;
			}
			else
			{
				printparseerror(19);
				error = 1;
				return;
			}
		}

		//emits the proc address to run it
		lIndex++;
		emit(5, lexLevel - table[symidx].level, table[symidx].addr); //CAL
	}
}

//emits all conditional statemtns
void condition(lexeme *list)
{
	if(list[lIndex].type == oddsym)
	{
		lIndex++;
		expression(list);
		if(error)
			return;
		emit(2, 0, 6); //ODD
	}
	else
	{
		expression(list);
		if(error)
			return;
		
		if(list[lIndex].type == eqlsym)
		{
			lIndex++;
			expression(list);
			if(error)
				return;
			emit(2, 0, 8); //EQL
		}
		else if(list[lIndex].type == neqsym)
		{
			lIndex++;
			expression(list);
			if(error)
				return;
			emit(2, 0, 9); //NEQ
		}
		else if(list[lIndex].type == lsssym)
		{
			lIndex++;
			expression(list);
			if(error)
				return;
			emit(2, 0, 10); //LSS
		}
		else if(list[lIndex].type == leqsym)
		{
			lIndex++;
			expression(list);
			if(error)
				return;
			emit(2, 0, 11); //LEQ
		}
		else if(list[lIndex].type == gtrsym)
		{
			lIndex++;
			expression(list);
			if(error)
				return;
			emit(2, 0, 12); //GTR
		}
		else if(list[lIndex].type == geqsym)
		{
			lIndex++;
			expression(list);
			if(error)
				return;
			emit(2, 0, 13); //GEQ
		}
		else
		{
			printparseerror(10);
			error = 1;
			return;
		}
	}
}

//emits all math expressions
void expression(lexeme *list)
{
	if(list[lIndex].type == subsym)
	{
		lIndex++;
		term(list);
		if(error)
			return;

		emit(2, 0, 1); //NEG
		while(list[lIndex].type == addsym || list[lIndex].type == subsym)
		{
			if(list[lIndex].type == addsym)
			{
				lIndex++;
				term(list);
				if(error)
					return;
				emit(2, 0, 2); //ADD
			}
			else
			{
				lIndex++;
				term(list);
				if(error)
					return;
				emit(2, 0, 3); //SUB
			}
		}
	}
	else
	{
		if(list[lIndex].type == addsym)
		{
			lIndex++;
		}
		term(list);
		if(error)
			return;
		while(list[lIndex].type == addsym || list[lIndex].type == subsym)
		{
			if(list[lIndex].type == addsym)
			{
				lIndex++;
				term(list);
				if(error)
					return;
				emit(2, 0, 2); //ADD
			}
			else
			{
				lIndex++;
				term(list);
				if(error)
					return;
				emit(2, 0, 3); //SUB
			}
		}
	}

	//ensures balanced expressions
	if(list[lIndex].type == lparensym || list[lIndex].type == identsym || list[lIndex].type == numbersym || list[lIndex].type == oddsym)
	{
		printparseerror(17);
		error = 1;
		return;
	}
}

//extends expression for order of operations
void term(lexeme *list)
{
	factor(list);
	if(error)
		return;
	
	while(list[lIndex].type == multsym || list[lIndex].type == divsym || list[lIndex].type == modsym)
	{
		if(list[lIndex].type == multsym)
		{
			lIndex++;
			factor(list);
			if(error)
				return;
			emit(2, 0, 4); //MUL
		}
		else if(list[lIndex].type == divsym)
		{
			lIndex++;
			factor(list);
			if(error)
				return;
			emit(2, 0, 5); //DIV
		}
		else
		{
			lIndex++;
			factor(list);
			if(error)
				return;
			emit(2, 0, 7); //MOD
		}
	}

	return;
}

//further extends expression for order of operations for parenthesis and literals
void factor(lexeme *list)
{
	if(list[lIndex].type == identsym)
	{
		int symidxVar = findSymbol(list[lIndex], 2);
		int symidxConst = findSymbol(list[lIndex], 1);

		if(symidxVar == -1 && symidxConst == -1)
		{
			if(findSymbol(list[lIndex], 3) != -1)
			{
				printparseerror(11);
				error = 1;
				return;
			}
			else
			{
				printparseerror(19);
				error = 1;
				return;
			}
		}

		if(symidxVar == -1)
		{
			emit(1, 0, table[symidxConst].val); //LIT
		}
		else if(symidxConst == -1 || table[symidxVar].level > table[symidxConst].level)
		{
			emit(3, lexLevel - table[symidxVar].level, table[symidxVar].addr); //LOD 
		}
		else
		{
			emit(1, 0, table[symidxConst].val); //LIT
		}

		lIndex++;
	}
	else if(list[lIndex].type == numbersym)
	{
		emit(1, 0, list[lIndex].value); //LIT
		lIndex++;
	}
	else if(list[lIndex].type == lparensym)
	{
		lIndex++;
		expression(list);
		if(error)
			return;
		
		if(list[lIndex].type != rparensym)
		{
			printparseerror(12);
			error = 1;
			return;
		}

		lIndex++;
	}
	else
	{
		printparseerror(11);
		error = 1;
		return;
	}
}

//linear search until it finds a id with the same name and in the same scope
int multiDeclarationCheck(lexeme item)
{
	int i;
	for(i = 0; i < tIndex; i++)
	{
		if(!strcmp(item.name, table[i].name) && table[i].mark == 0 && table[i].level == lexLevel)
		{
			return i;
		}
	}

	return -1;
}

//linear search that finds teh same name and kind in the scope but returns with the location in the code
int findSymbol(lexeme item, int kind)
{
	int i;
	int level = 0;
	int retval = -1;
	for(i = 0; i < tIndex; i++)
	{
		if(!strcmp(item.name, table[i].name) && table[i].kind == kind && table[i].mark == 0)
		{
			if(level <= table[i].level)
			{
				level = table[i].level;
				retval = i;
			}
		}
	}

	return retval;
}

//marks all symbols that are going out of scope as the block closes so that they are not accessible anymore
void mark()
{
	for(int i = tIndex - 1; i >= 0; i--)
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
			printf("Parser Error: when must be followed by do\n");
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
			printf("Parser Error: Statements within do-od must be separated by a semicolon\n");
			break;
		case 16:
			printf("Parser Error: do must be followed by od\n");
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