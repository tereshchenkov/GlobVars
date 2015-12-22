#include "metrologyFuncs.h"
#include "constants.h"

#include <stdio.h>
#include <string.h>


// Словарь языка
const char* languageWords[] =
{
	"def", "print", "input", "None", "not", "in", "return", "if", "else", 
	"for", "elif", "global", "lower", "append", "while", "range", "int"
};
#define LANGUAGE_WORDS_COUNT	(sizeof(languageWords) / sizeof(char*))

// Глобальные переменные для расчёта метрики
int glIdentifiersCount = 0;
char glIdentifiers[MAX_IDENTIFIER_COUNT][MAX_IDENTIFIER_LENGTH];
int globalUsing = 0;

// Глобальные переменные для обработки функций
int inFunction = FALSE;
int functionsCount = 0;
char functionsNames[MAX_IDENTIFIER_COUNT][MAX_IDENTIFIER_LENGTH];
int globalCount;
char globalVars[MAX_IDENTIFIER_COUNT][MAX_IDENTIFIER_LENGTH];
int localCount;
char localVars[MAX_IDENTIFIER_COUNT][MAX_IDENTIFIER_LENGTH];
int argsCount;
char argsVars[MAX_IDENTIFIER_COUNT][MAX_IDENTIFIER_LENGTH];

// Глобальные переменные для обработки главного кода программы
int firstLook = TRUE;
int stringLength;
int currentPosition;
int wasString = FALSE;
int globalMainCount = 0;
char globalMainVars[MAX_IDENTIFIER_COUNT][MAX_IDENTIFIER_LENGTH];

void setAsMainProcess()
{
	printf("Глобальных переменных %d:\n", glIdentifiersCount);
	int i;
	for (i = 0; i < glIdentifiersCount - 1; i++)
		printf("%s, ", glIdentifiers[i]);
	printf("%s\n\n", glIdentifiers[i]);
	printf("Функций %d:\n", functionsCount + 1);
	functionsCount = 0;
	firstLook = FALSE;
}

int isBeginningSymbol(char symbol)
{
	if ((symbol >= 'A' && symbol <= 'Z') ||
		(symbol >= 'a' && symbol <= 'z') ||
		(symbol == '_'))
		return TRUE;
	else
		return FALSE;
}

int isValidSymbol(char symbol)
{
	if (isBeginningSymbol(symbol) ||
		('0' <= symbol && symbol <= '9'))
		return TRUE;
	else
		return FALSE;
}

int compareWords(char wordToCompare[])
{
	int i;
	for (i = 0; i < LANGUAGE_WORDS_COUNT; i++)
		if (!strcmp(wordToCompare, languageWords[i]))
			break;
	return i;
}

int compareWith(char words[][MAX_IDENTIFIER_LENGTH], char wordToCompare[], int count)
{
	int i;
	for (i = 0; i < count; i++)
		if (!strcmp(wordToCompare, words[i]))
			return i;
	return NOT_FOUND;
}

result getNextWord(char dest[], char line[])
{
	int state = SPACE;
	if (wasString)
		state = STRING;
	int wordPosition;
	char buffer[MAX_IDENTIFIER_LENGTH];
	while (currentPosition <= stringLength)
	{
		char symbol = line[currentPosition++];
		switch (state)
		{
		case SPACE:
			if (isBeginningSymbol(symbol))
			{
				wordPosition = 0;
				buffer[wordPosition++] = symbol;
				state = IDENTIFIER;
			}
			else
				switch (symbol)
				{
				case '#':
					currentPosition = stringLength + 1;
				break;
				case '"':
					state = STRING;
					wasString = TRUE;
				break;
				case '=':
					if ((line[currentPosition] != '=') &&
						(line[currentPosition - 2] != '!'))
						return RESULT_ASSIGNMENT;
					else
						currentPosition++;
				break;
				}
		break;
		case IDENTIFIER:
			if (isValidSymbol(symbol))
			{
				buffer[wordPosition++] = symbol;
				state = IDENTIFIER;
			}
			else
			{
				currentPosition--;
				buffer[wordPosition] = '\0';
				int cmpResult = compareWords(buffer);
				if (cmpResult == RESULT_DEF)
					return RESULT_DEF;
				if (cmpResult == RESULT_GLOBAL)
					return RESULT_GLOBAL;
				if (cmpResult < LANGUAGE_WORDS_COUNT)
					return RESULT_WORD;
				strcpy_s(dest, MAX_IDENTIFIER_LENGTH, buffer);
				return RESULT_IDENTIFIER;
			}
		break;
		case STRING:
			if (symbol == '"')
			{
				state = SPACE;
				wasString = FALSE;
			}
		break;
		}
	}
	return RESULT_NONE;
}

void swapToGlobal(char var[])
{
	if (compareWith(glIdentifiers, var, glIdentifiersCount) == NOT_FOUND)
		strcpy_s(glIdentifiers[glIdentifiersCount++], MAX_IDENTIFIER_LENGTH, var);
	int position = compareWith(localVars, var, localCount);
	if (position != NOT_FOUND)
	{
		strcpy_s(globalVars[globalCount++], MAX_IDENTIFIER_LENGTH, localVars[position]);
		int i;
		localCount--;
		for (i = position; i < localCount; i++)
			strcpy_s(localVars[i], MAX_IDENTIFIER_LENGTH, localVars[i + 1]);
	}
	else
		if (compareWith(globalVars, var, globalCount) == NOT_FOUND)
			strcpy_s(globalVars[globalCount++], MAX_IDENTIFIER_LENGTH, var);
}

void printInformation()
{
	int finalCount = 0;
	int i;
	for (i = 0; i < globalCount; i++)
		if (compareWith(glIdentifiers, globalVars[i], glIdentifiersCount) != NOT_FOUND)
			finalCount++;
	printf("Фукнция %s обращается к %d глобальным переменным.\n", functionsNames[functionsCount - 1], finalCount);
	globalUsing += finalCount;
}

result processInFunction(char buffer[], result prevResult)
{
	if (prevResult == RESULT_GLOBAL)
	{
		swapToGlobal(buffer);
		return RESULT_NONE;
	}
	if ((compareWith(functionsNames, buffer, functionsCount) == NOT_FOUND) &&
	    (compareWith(argsVars, buffer, argsCount) == NOT_FOUND) &&
	    (compareWith(localVars, buffer, localCount) == NOT_FOUND) &&
	    (compareWith(globalVars, buffer, globalCount) == NOT_FOUND))
	{
		strcpy_s(globalVars[globalCount++], MAX_IDENTIFIER_LENGTH, buffer);
		return RESULT_IDENTIFIER;
	}
	return prevResult;
}

result processNotInFunction(char buffer[], result prevResult)
{
	if (compareWith(functionsNames, buffer, functionsCount) == NOT_FOUND)
	{
		if (compareWith(glIdentifiers, buffer, glIdentifiersCount) == NOT_FOUND)
			strcpy_s(glIdentifiers[glIdentifiersCount++], MAX_IDENTIFIER_LENGTH, buffer);
		if (compareWith(globalMainVars, buffer, globalMainCount) == NOT_FOUND)
			strcpy_s(globalMainVars[globalMainCount++], MAX_IDENTIFIER_LENGTH, buffer);
	}	
	return RESULT_NONE;
}

result processIdentifier(char buffer[], result prevResult)
{
	if (prevResult == RESULT_DEF)
	{
		if (compareWith(functionsNames, buffer, functionsCount) == NOT_FOUND)
			strcpy_s(functionsNames[functionsCount++], MAX_IDENTIFIER_LENGTH, buffer);
		return RESULT_ARGUMENTS;
	}
	else
	{
		if (prevResult == RESULT_ARGUMENTS)
		{
			strcpy_s(argsVars[argsCount++], MAX_IDENTIFIER_LENGTH, buffer);
			return prevResult;
		}
		else
		{
			if (inFunction)
				processInFunction(buffer, prevResult);
			else
				processNotInFunction(buffer, prevResult);
		}
		return prevResult;
	}
}

void processLine(char line[])
{
	if (inFunction && (line[0] != ' ') && (line[0] != '\t'))
	{
		if (!firstLook)
			printInformation();
		inFunction = FALSE;
	}
	stringLength = strlen(line);
	currentPosition = 0;
	result wordKind = RESULT_NONE;
	result prevResult = RESULT_NONE;
	do {
		char buffer[MAX_IDENTIFIER_LENGTH];
		wordKind = getNextWord(buffer, line);
		switch (wordKind)
		{
		case RESULT_DEF:
			globalCount = 0;
			localCount = 0;
			argsCount = 0;
			inFunction = TRUE;
			prevResult = RESULT_DEF;
		break;
		case RESULT_GLOBAL:
			prevResult = RESULT_GLOBAL;
		break;
		case RESULT_IDENTIFIER:
			prevResult = processIdentifier(buffer, prevResult);
		break;
		case RESULT_ASSIGNMENT:
			if (inFunction && (prevResult == RESULT_IDENTIFIER))
				strcpy_s(localVars[localCount++], MAX_IDENTIFIER_LENGTH, globalVars[--globalCount]);
		break;
		}
	} while (wordKind != RESULT_NONE);
}

void finalProcessing()
{
	printf("Главный код обращается к %d глобальным переменным.\n", globalMainCount);
	globalUsing += globalMainCount;
}

int getGlobalUsing()
{
	return globalUsing;
}

int getPossibleUsing()
{
	return ((functionsCount + 1) * glIdentifiersCount);
}

float getMetric()
{
	int possibleUsing = getPossibleUsing();
	return (float)globalUsing / possibleUsing;
}
