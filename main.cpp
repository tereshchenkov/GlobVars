#include "constants.h"
#include "metrologyFuncs.h"

#include <stdio.h>
#include <locale.h>


void readFile(FILE* file)
{
	while (!feof(file))
	{
		char buffer[MAX_LINE_LENGTH];
		fgets(buffer, MAX_LINE_LENGTH, file);
		processLine(buffer);
	}
	fseek(file, 0, SEEK_SET);
}

int main(void)
{
	setlocale(LC_ALL, "Russian");

	FILE* file;
	fopen_s(&file, "program.py", "r");

	if (file == NULL)
	{
		printf("Ошибка! Файл отсутствует.\n");
	}
	else
	{
		readFile(file);
		setAsMainProcess();
		readFile(file);
		fclose(file);
		finalProcessing();
		printf("\nИтоговая метрика: %d / %d = %.3f", getGlobalUsing(), getPossibleUsing(), getMetric());
	}

	getchar();
	return 0;
}
