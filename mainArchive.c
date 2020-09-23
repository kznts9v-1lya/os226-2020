#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int echo(int argc, char* argv[])
{
	for (int i = 1; i < argc; i++) 
	{
		printf("%s%c", argv[i], i == argc - 1 ? '\n' : ' ');
	}

	return(argc - 1);
}

int retcode(int echoArgs) 
{
	printf("%d\n", echoArgs);

	return(echoArgs);
}

int main(int argc, char* argv[])
{
	char string[256];
	char* saveSubstring = NULL;
	char* saveWord = NULL;
	int echoArgs = 0;

	while(fgets(string, sizeof(string), stdin))
	{
		char* substring = strtok_r(string, ";\n", &saveSubstring);

		while(substring != NULL)
		{
			char* word = strtok_r(substring, " ", &saveWord);

			argc = 0;

			while (word != NULL)
			{
				argv[argc] = word;
				argc++;

				word = strtok_r(NULL, " ", &saveWord);
			}

			if(strcmp(argv[0], "echo") == 0)
			{
				echoArgs = echo(argc, argv);
			}
			else if(strcmp(argv[0], "retcode") == 0)
			{
				retcode(echoArgs);
			}
			else
			{
				printf("Unknown command\n");
			}

			substring = strtok_r(NULL, ";\n", &saveSubstring);
		}
	}

	return(0);
}