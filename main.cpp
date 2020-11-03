#include <iostream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>

using namespace std;

vector<string> parseCommand(char ** argv);
void commandLineDriver(vector<string> commandLine);
int getIndex(vector<string> vec, string el);

int main(int argc, char ** argv)
{
	vector<string> commandLine = parseCommand(argv);
	//parse user input from command line and place in commandLine vector
	commandLineDriver(commandLine);
	//function that runs rest of program

	return 0;
}
//------------------------------------------------------------------------------------------
vector<string> parseCommand(char ** argv)
{
	vector<string> retVec;

	argv++;
	//get rid of program name

	while(*argv)
	{
		retVec.push_back(*(argv++)); 
		//push back each command line arguement into retVec vector
	}
	
	return retVec;
}
//------------------------------------------------------------------------------------------
void commandLineDriver(vector<string> commandLine)
{
	if(count(commandLine.begin(), commandLine.end(), "-1") != 0)
	{
		//-1 is provided
		if(count(commandLine.begin(), commandLine.end(), "-d") != 0)
		{
			//-d is provided, switch working directory
			char charArray[260];
			strcpy(charArray, commandLine[getIndex(commandLine, "-d")+1].c_str());

			if(chdir(charArray) == -1)
			{
				//Specified path does not lead to directory, ERROR exit program
				cerr << "Specified path does not lead to a directory" << endl;
				exit(1);
			}
		}
		
		if(count(commandLine.begin(), commandLine.end(), "-p") != 0)
		{
			//-p specified, print current working directory
			char cwd[260];
			getcwd(cwd, sizeof(cwd));
			printf("Current working directory: %s\n", cwd);
		}

		int numO = count(commandLine.begin(), commandLine.end(), "-o");
		int numA = count(commandLine.begin(), commandLine.end(), "-a");
		int numI = count(commandLine.begin(), commandLine.end(), "-i");
		//counters to see if -o, -a, or -i were specified in the command line
		int myFileDesc;

		if(numO != 0 && numA != 0)
		{
			//both -o and -a specified, need to determine which one was entered last
			if(getIndex(commandLine, "-o") > getIndex(commandLine, "-a"))
			{
				//-o comes last
				char charArray[260];
				strcpy(charArray, commandLine[getIndex(commandLine, "-o")+1].c_str());
				myFileDesc = open(charArray, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
			}
			else
			{
				//-a comes last
				char charArray[260];
				strcpy(charArray, commandLine[getIndex(commandLine, "-a")+1].c_str());
				myFileDesc = open(charArray, O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
			}
		}
		else if(numO != 0)
		{
			//just -o specified
			char charArray[260];
			strcpy(charArray, commandLine[getIndex(commandLine, "-o")+1].c_str());
			myFileDesc = open(charArray, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
		}
		else if(numA != 0)
		{
			//just -a specified
			char charArray[260];
			strcpy(charArray, commandLine[getIndex(commandLine, "-a")+1].c_str());
			myFileDesc = open(charArray, O_CREAT|O_WRONLY|O_APPEND, S_IRWXU);
		}

		if(numI != 0)
		{
			//-i specified
			char charArray[260];
			strcpy(charArray, commandLine[getIndex(commandLine, "-i")+1].c_str());
			myFileDesc = open(charArray, O_RDONLY, S_IRWXU);
		}
			
		int p[2];
		bool twoProgs = false;
		if(count(commandLine.begin(), commandLine.end(), "-2") != 0)
		{
			//-2 specified, create pipe
			twoProgs = true;
			if(pipe(p) < 0)
			{
				cerr << "Pipe failed" << endl;
				exit(1);
			}
		}

		int c1 = fork();
			
		//--------FORK CREATED--------//

		if(c1 == 0)
		{
			//child 1
			if((numO != 0 || numA != 0) && !twoProgs)
			{
				//redirect standard output, -a and/or -o specified
				close(STDOUT_FILENO);
				dup(myFileDesc);
				close(myFileDesc);
			}

			if(numI != 0)
			{
				//redirect standard input, -i specified
				close(STDIN_FILENO);
				dup(myFileDesc);
				close(myFileDesc);
			}

			if(twoProgs)
			{
				//-2 specified
				close(p[0]);
				close(STDOUT_FILENO);
				dup(p[1]);
				close(p[1]);
			}

			char *c = const_cast<char*>(commandLine[getIndex(commandLine, "-1")+1].c_str());
			char *myargs[] = {c, NULL};

			execvp(myargs[0], myargs);
		}
		else
		{
			int c2 = -1;
			if(twoProgs)
			{
				//-2 specified, 2 forks
				c2 = fork();

		//--------SECOND FORK CREATED--------//

				if (c2 == 0)
				{
					//child 2
					if(numO != 0 || numA != 0)
					{
						//redirect standard output, -a and/or -o specified
						close(STDOUT_FILENO);
						dup(myFileDesc);
						close(myFileDesc);
					}

					close(p[1]);
					close(STDIN_FILENO);
					dup(p[0]);
					close(p[0]);

					char *c = const_cast<char*>(commandLine[getIndex(commandLine, "-2")+1].c_str());
					char *myargs[] = {c, NULL};

					execvp(myargs[0], myargs);
				}
			}
			//parent
			if(twoProgs && c1 > 0 && c2 > 0)
			{
				//parent, 2 forks
				if(numI != 0 || numA != 0 || numO != 0)
					close(myFileDesc);
				if(twoProgs)
				{
					close(p[0]);
					close(p[1]);
				}
				
				int pid = wait(&c1);
				int pid2 = wait(&c2);
				printf("Child 2: %d returns %d\n", pid2, c2);
				printf("Child 1: %d returns %d\n", pid, c1);
			}
			else if(!twoProgs && c1 > 0)
			{
				//parent, 1 fork
				if(numI != 0 || numA != 0 || numO != 0)
					close(myFileDesc);

				int pid = wait(&c1);
				printf("Child 1: %d returns %d\n", pid, c1);
			}
			else
			{
				cerr << "Fork failed" << endl;
				exit(1);
			}
		}
	}
	else
	{
		//-1 not provided, ERROR exit program
		cerr << "Missing required command line option -1" << endl;
		exit(1);
	}
}
//------------------------------------------------------------------------------------------
int getIndex(vector<string> vec, string el)
{
	for(unsigned int i = 0; i < vec.size(); i++)
	{
		if(vec[i] == el)
			return i;
		//returns index of el
	}

	return -1;
	//returns -1 if not found
}
//------------------------------------------------------------------------------------------