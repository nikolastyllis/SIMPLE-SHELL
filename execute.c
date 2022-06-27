/* execute.c - code used by small shell to execute commands */

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<signal.h>
#include	<sys/wait.h>
#include 	<string.h>
#include 	<glob.h>

/*
 * purpose: run a program passing it arguments
 * returns: status returned via wait, or -1 on error
 *  errors: -1 on fork() or wait() errors

 */
int execute(char *argv[])
{
	if (argv[0] == NULL) return 0;

	int child_info = -1;
	
	int prev_pipe[2];
    if(pipe(prev_pipe) == -1) printf("Pipe Creation Failed.");

    int firstCommand = 0;
	int lastCommand = 0;

	char* outputFile = NULL;
	char* inputFile = NULL;
	char* globFileExt = NULL;

	glob_t gstruct;

	//Get length and look for input redirect
	int totalLength = 0;
	while(1)
	{		
		if(argv[totalLength] == NULL) break;
		if(strcmp(argv[totalLength],"<")==0)
		{
			inputFile = argv[totalLength+1];
			argv[totalLength] = NULL;
			break;
		}

		//Look for glob symbol to find the glob path match
		int k=0;
		while(argv[totalLength][k] != '\0')
		{
			if(argv[totalLength][k] == '*')
			{
				globFileExt=argv[totalLength];
				break;
			}
			k++;
		}

		totalLength++;
	}


	//Run glob if there is globbing
	if(globFileExt != NULL)
	{
		gstruct.gl_offs = 1;
		glob(globFileExt, GLOB_DOOFFS , NULL, &gstruct);
	} 
		
	//Run through each execution
	int i=0;
	while(i<totalLength && outputFile == NULL)
	{
		lastCommand = 0;
		
		if(i==0) firstCommand = 1;
		else firstCommand = 0;

		//Run through and look for pipe and output redirect
		int length = 0;
		int iterator = i;
		while(1)
		{
			if(argv[iterator] == NULL) 
			{
				break;
			}
			if(strcmp(argv[iterator],"|")==0)
			{
				lastCommand = 0;
				break;
			}
			if(strcmp(argv[iterator],">")==0)
			{
				lastCommand = 1;
				outputFile = argv[iterator+1];
				break;
			}
			iterator++;
			length++;
		}

		char **currentCommand = (char**) calloc(length+1, sizeof(char*));
		for(int j=0;j<length;j++)
		{
			currentCommand[j] = argv[i+j];
		}
		currentCommand[length] = NULL;
		i+=(length+1);
		
		//Check if last command
		if(i>totalLength) lastCommand=1;
	
		int new_pipe[2];

    	//If not final command then create new pipe
    	if(!lastCommand &&pipe(new_pipe) == -1) 
    	{
    		printf("Pipe Creation Failed.");
    		child_info=-1;
    		break;
    	}


    	//Fork and handle errors
    	int pid = fork();
    	if(pid<0) printf("Fork Failed");

    	//Child
    	if(pid==0)
    	{  
    		//Check if input is being redirected and redirect it
    		if(inputFile != NULL)
    		{
    			FILE* inputFilePointer = fopen(inputFile, "r");
    			int inputFileDescriptor = fileno(inputFilePointer);

    			dup2(inputFileDescriptor, 0);
    			close(inputFileDescriptor);
    		}

    		//Check if ouput is being redirected and redirect it
    		if(lastCommand && outputFile != NULL)
    		{
    			FILE* outputFilePointer = fopen(outputFile, "w");
    			int outputFileDescriptor = fileno(outputFilePointer);

    			dup2(outputFileDescriptor, 1);
    			close(outputFileDescriptor);
    		}

            //if not first command
    		if(!firstCommand)
    		{
    			close(0);
                dup(prev_pipe[0]);
                close(prev_pipe[0]);
                close(prev_pipe[1]);

    		}

    		//if not final command
    		if(!lastCommand)
    		{
    			close(1);
                dup(new_pipe[1]);
                close(new_pipe[0]);
                close(new_pipe[1]);
    		}

    		//Check if we globbing first
    		if(globFileExt==NULL) execvp(currentCommand[0], currentCommand);

    		gstruct.gl_pathv[0] = currentCommand[0];
    		execvp(gstruct.gl_pathv[0], gstruct.gl_pathv);
    		
   		}

   		//Parent
    	if(pid>0)
    	{  
            //if not first command
            if(!firstCommand)
            {
                close(prev_pipe[0]);
                close(prev_pipe[1]);
            }

    		//if not final command
    		if(!lastCommand)
    		{
                dup2(new_pipe[0],prev_pipe[0]);
                dup2(new_pipe[1],prev_pipe[1]);
                close(new_pipe[0]);
                close(new_pipe[1]);

    		}

           if(wait(NULL) == -1) perror("wait");
   		}

		free(currentCommand);
		
	}

	return child_info;
}
