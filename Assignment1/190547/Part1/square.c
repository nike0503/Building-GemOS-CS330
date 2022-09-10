#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>
#include <unistd.h>

//function to return length of a string (as strlen is not mentioned in APIs available)
int len(char *str) {
	int count = 0;
	for(int i = 0; str[i]!='\0'; i++) {
    	count++;
    }
	return count;
}

//function to convert a string to unsigned long long interger
unsigned long long convertToULL(char *str) {
	unsigned long long no = 0;
	for (int i = 0; i < len(str); i++) {
		no *= 10;
		//if str[i] is not an integer, return 0;
		if (str[i] < '0' || str[i] > '9') {
			no = 0;
			break;
		}
		no += str[i]-'0';
	}
	return no;
}

int main(int argc, char *argv[])
{
	char errorMsg[] = "UNABLE TO EXECUTE";

	char* string_no = (char*)malloc((len(argv[argc-1])+1)*sizeof(char));
	strcpy(string_no, argv[argc-1]); //get the final argument(the integer) in a string

	unsigned long long no = convertToULL(string_no); //returns 0 if final argument is not an integer
	
	sprintf(string_no, "%llu", no);
	if (strcmp(argv[argc-1], string_no)) //check if the final argument is valid, i.e. an integer by comparing the returned integer and the original
	{
		printf("%s\n", errorMsg);
		exit(-1);
	}

	unsigned long long result = no * no; //do the current operation

	char result_str[40];
	sprintf(result_str, "%llu", result);
	argv[argc-1] = result_str; //store the result back in arguments list

	if (argc == 2) //this is the final operation
	{
		printf("%s\n", result_str);
	}
	else
	{
		//nextArgs is the list of arguments for the next process to be called using execvp
		char **nextArgs;
		nextArgs = (char**)malloc(argc*sizeof(char*));
		nextArgs[0] = (char*)malloc((len(argv[1])+3)*sizeof(char));
		strcpy(nextArgs[0], "./");
		strcat(nextArgs[0], argv[1]);
		for (int i = 2; i < argc; i++) {
			nextArgs[i-1] = (char*)malloc((len(argv[1])+1)*sizeof(char));
			strcat(nextArgs[i-1], argv[i]);		
		}
		//the list of arguments is NULL terminated
		nextArgs[argc-1] = NULL;
		if(execvp(nextArgs[0], nextArgs) < 0)
		{
			printf("%s\n", errorMsg);
			exit(-1);
		}
	}
	return 0;
}
