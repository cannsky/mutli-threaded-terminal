#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#define CREATE_FlAGS_APPEND (O_WRONLY | O_CREAT | O_APPEND)
#define CREATE_FlAGS_TRUNC (O_WRONLY | O_CREAT | O_TRUNC)
#define CREATE_FlAGS_READ (O_RDONLY)

#define CREATE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define CREATE_MODE_IN (S_IRUSR | S_IRGRP | S_IROTH)

#define MAX_LINE 80 /* 80 chars per line, per command, should be enough. */
#define MAX 1024

int fd;

struct Alias
{
    char *alias;
    char *aliasCommands[25];
    struct Alias* next;
    struct Alias* previous;
} typedef Alias;

char pathArray[20][80];
char fullPath[255];

char *path;
char *ch;

int argsFinalIndex = 0;

pid_t backgroundProcessArray[100];

Alias* UpdateAliasData(Alias* newAlias, char *alias, char *args[]){
    newAlias = (Alias*)malloc(sizeof(Alias));
    newAlias->alias = malloc(MAX);
    strcpy(newAlias->alias, alias);
    int i = 0;
    for(i = 1; i < argsFinalIndex; i++){
        if(args[i] != NULL){
            newAlias->aliasCommands[i-1] = malloc(MAX);
            strcpy(newAlias->aliasCommands[i-1], args[i]);
        }
        else break;
    }
    return newAlias;
}

Alias* CreateAlias(Alias* rootAlias, char *alias, char *args[]){
    Alias* newAlias = rootAlias;
    Alias* previousAlias;
    int isAliasAlreadyAvailable = 0;
    while(newAlias != NULL){
        if(strcmp(newAlias->alias, alias) == 0) isAliasAlreadyAvailable = 1;
        previousAlias = newAlias;
        newAlias = newAlias->next;
    }
    if(isAliasAlreadyAvailable == 0){
        newAlias = UpdateAliasData(newAlias, alias, args);
        if(previousAlias != NULL) previousAlias->next = newAlias;
        newAlias->previous = previousAlias;
    }
    else perror("This alias already exists!");
    return (rootAlias == NULL ? newAlias : rootAlias);
}

Alias* FindAlias(Alias* rootAlias, char *alias){
    Alias *currentAlias;
    currentAlias = rootAlias;
    while(currentAlias != NULL){
        if(currentAlias->alias != NULL && alias != NULL){
            if(strcmp(currentAlias->alias, alias) == 0) break;
            currentAlias = currentAlias->next;
        }
    }
    return currentAlias;
}

void PrintAlias(Alias* rootAlias){
    Alias *currentAlias;
    currentAlias = rootAlias;
    while(currentAlias != NULL){
        if(currentAlias->alias != NULL){
            fprintf(stderr, "%s ", currentAlias->alias);
            int i = 0;
            for(i = 0; i < 32; i++){
                if(currentAlias->aliasCommands[i] == NULL) break;
                if(i==0) fprintf(stderr, "\"%s ", currentAlias->aliasCommands[i]);
                else if(currentAlias->aliasCommands[i+1] == NULL) fprintf(stderr, "%s\"", currentAlias->aliasCommands[i]);
                else fprintf(stderr, "%s ", currentAlias->aliasCommands[i]);
            }
            fprintf(stderr, "\n");
            currentAlias = currentAlias->next;
        }
    }
}


Alias* RemoveAlias(Alias* rootAlias, char *alias){
    Alias* currentAlias = NULL;
    Alias* previousAlias = NULL;
    int isDeletingRoot = 0;
    currentAlias = rootAlias;
    while(currentAlias != NULL){
        if(currentAlias->alias != NULL)
            if(currentAlias->alias != NULL && alias != NULL){
                if(strcmp(currentAlias->alias, alias) == 0){
                    if(previousAlias != NULL){
                        previousAlias->next = currentAlias->next;
                        if(currentAlias->next != NULL) (currentAlias->next)->previous = previousAlias;
                        break;
                    }
                    else{
                        //We are deleting the root!
                        if(currentAlias->next != NULL) (currentAlias->next)->previous = NULL;
                        currentAlias = currentAlias->next;
                        isDeletingRoot = 1;
                        break;
                    }
                }
                previousAlias = currentAlias;
                currentAlias = currentAlias->next;
            }
    }
    return (isDeletingRoot == 0 ? rootAlias : currentAlias);
}


Alias* rootAlias;

//Get path variable to the global character pointer
void GetPathVariable(){
    path = getenv("PATH");
}

//Split path string to an array
void SplitString(){
    int index = 0;
    ch = strtok(path, ":");
    while (ch != NULL){
        strcpy(pathArray[index++], ch);
        ch = strtok(NULL, ":");
    }
}

void CreatePathArray(){
    GetPathVariable();
    SplitString();
}

void ExecuteCommand(char *args[]){
    int i = 0, val = 0, systemCommand = 0;
    for(i = 0; i < 20; i++){
        if(pathArray[i][0] == '\0') break;
        strcat(fullPath, pathArray[i]);
        strcat(fullPath, "/");
        strcat(fullPath, args[0]);
        val = execv(fullPath, args);
        if(val != -1) break;
        fullPath[0] = '\0';
    }
}

int CheckAliasCommandAvailable(char *args[]){
    Alias* aliasNode = FindAlias(rootAlias, args[0]);
    if(aliasNode != NULL) ExecuteCommand(aliasNode->aliasCommands);
    return 0;
}

void ForkChild(int background, char *args[]){
    pid_t childPid = fork();
    if(childPid < 0) {
        return;
    }

    else if(childPid != 0){
        //Parent
        if(!background) {
            waitpid(childPid, NULL, 0);
        }
        else{
            waitpid(childPid, NULL, WNOHANG);
            int i = 0;
            for(i = 0; i < 100; i++){
                if(backgroundProcessArray[i] == 0){
                    backgroundProcessArray[i] = childPid;
                    fprintf(stderr, "Hello guys!");
                    break;
                }
            }
        }
    }

    else {
        if(CheckAliasCommandAvailable(args) == 0) ExecuteCommand(args);
        exit(0);
    }
}

/* The setup function below will not return any value, but it will just: read
in the next command line; separate it into distinct arguments (using blanks as
delimiters), and set the args array entries to point to the beginning of what
will become null-terminated, C-style strings. */

void setup(char inputBuffer[], char *args[],int *background)
{
    int length, /* # of characters in the command line */
    i,      /* loop index for accessing inputBuffer array */
    start,  /* index where beginning of next command parameter is */
    ct;     /* index of where to place the next parameter into args[] */

    ct = 0;

    /* read what the user enters on the command line */
    length = read(STDIN_FILENO,inputBuffer,MAX_LINE);

    /* 0 is the system predefined file descriptor for stdin (standard input),
       which is the user's screen in this case. inputBuffer by itself is the
       same as &inputBuffer[0], i.e. the starting address of where to store
       the command that is read, and length holds the number of characters
       read in. inputBuffer is not a null terminated C-string. */

    start = -1;
    if (length == 0)
        exit(0);            /* ^d was entered, end of user command stream */

/* the signal interrupted the read system call */
/* if the process is in the read() system call, read returns -1
  However, if this occurs, errno is set to EINTR. We can check this  value
  and disregard the -1 value */
    if ( (length < 0) && (errno != EINTR) ) {
        perror("error reading the command");
        exit(-1);           /* terminate with error code of -1 */
    }

    printf(">>%s<<",inputBuffer);
    for (i=0;i<length;i++){ /* examine every character in the inputBuffer */

        switch (inputBuffer[i]){
            case ' ':
            case '\t' :               /* argument separators */
                if(start != -1){
                    args[ct] = &inputBuffer[start];    /* set up pointer */
                    ct++;
                }
                inputBuffer[i] = '\0'; /* add a null char; make a C string */
                start = -1;
                break;

            case '\n':                 /* should be the final char examined */
                if (start != -1){
                    args[ct] = &inputBuffer[start];
                    ct++;
                }
                inputBuffer[i] = '\0';
                args[ct] = NULL; /* no more arguments to this command */
                break;

            default :             /* some other character */
                if (start == -1)
                    start = i;
                if (inputBuffer[i] == '&'){
                    args[ct] = NULL; //DELETE & CHARACTER FROM ARRAY!
                    *background  = 1;
                    inputBuffer[i-1] = '\0';
                }
        } /* end of switch */
    }    /* end of for */
    argsFinalIndex = ct-1;
    args[ct] = NULL; /* just in case the input line was > 80 */

    for (i = 0; i <= ct; i++)
        printf("args %d = %s\n",i,args[i]);
} /* end of setup routine */

void removeChar(char *str, char garbage) {
    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}

void HandleAlias(char *args[]){
    int i = 0;
    for(i = 1; i < argsFinalIndex; i++){
        if(i == 1 || i == (argsFinalIndex - 1)) removeChar(args[i], '"');

    }
    rootAlias = CreateAlias(rootAlias, args[argsFinalIndex], args);
}

void HandleUnAlias(char *args[]){
    rootAlias = RemoveAlias(rootAlias, args[1]);
}

void ReturnBackgroundProcessStatus(){
    pid_t pid = getpid();
    int counter = 0;
    for(int i = 0; i < 100; i++){
        if(backgroundProcessArray[i] == 0) break;
        pid_t childProcessId = waitpid(backgroundProcessArray[i], NULL, WNOHANG);
        if(childProcessId == 0) counter++;
        else if(childProcessId == -1) fprintf(stderr, "error when closing shell!");
    }
    if(counter == 0) exit(0);
    else fprintf(stderr, "There are background processes still running! Close them before exiting!");
}

void HandleCommand(char *args[], int background){
    if(background == 1) args[argsFinalIndex] = '\0';
    if(args[0] == NULL) return;
    if(strcmp(args[0], "alias") == 0){
        if(args[1] != NULL && strcmp(args[1], "-l") == 0) PrintAlias(rootAlias);
        else HandleAlias(args);
        return;
    }
    else if(strcmp(args[0], "unalias") == 0){
        HandleUnAlias(args);
        return;
    }
    else if(strcmp(args[0], "exit") == 0){
        ReturnBackgroundProcessStatus();
    }
    else ForkChild(background, args);
    return;
}

void setOutput(int ioType, char *filename) {
    if(ioType == 2 || ioType == 5) fd = open(filename, CREATE_FlAGS_TRUNC, CREATE_MODE);
    else if (ioType == 3) fd = open(filename, CREATE_FlAGS_APPEND, CREATE_MODE);
    if (fd==-1)
        perror("Failed to open file.");
    if (dup2(fd, STDOUT_FILENO) == -1)
        perror("Failed to redirect standart output.");
    if (close(fd) == -1)
        perror("Failed to close the file.");
}

void setInput(int ioType, char *filename) {
    if(ioType == 4 || ioType == 5) fd = open(filename, CREATE_FlAGS_READ, CREATE_MODE_IN);
    if (fd==-1)
        perror("Failed to open file.");
    if (dup2(fd, STDIN_FILENO) == -1)
        perror("Failed to redirect standart output.");
    if (close(fd) == -1)
        perror("Failed to close the file.");
}



int main(void)
{
    CreatePathArray();
    char inputBuffer[MAX_LINE]; /*buffer to hold command entered */
    int background; /* equals 1 if a command is followed by '&' */
    char *args[MAX_LINE/2 + 1]; /*command line arguments */
    while (1){
        background = 0;
        fprintf(stderr, "myshell: ");
        /*setup() calls exit() when Control-D is entered */
        setup(inputBuffer, args, &background);

        char *fullArgument = malloc(sizeof(MAX));

        int i = 0;
        for(i = 0; i < 32; i++){
            if(args[i] == NULL) break;
            strcat(fullArgument, args[i]);
            strcat(fullArgument, " ");
        }

        int ioType;
        //1: no io, 2: output truncate, 3: output append, 4: input file, 5: input&output file.
        if(strstr(fullArgument, " > ")!=NULL && strstr(fullArgument, "<")==NULL)
            ioType=2;
        else if(strstr(fullArgument, ">>")!=NULL && strstr(fullArgument, "<")==NULL)
            ioType=3;
        else if(strstr(fullArgument, " > ")==NULL && strstr(fullArgument, " < ")!=NULL)
            ioType=4;
        else if(strstr(fullArgument, " > ")!=NULL && strstr(fullArgument, " < ")!=NULL)
            ioType=5;
        else
            ioType=1;
        //output and input file names
        char *outputFile;
        char *inputFile;
        //describing on output and input names
        if(ioType == 2 || ioType == 3 || ioType == 5) outputFile = args[argsFinalIndex];
        if(ioType == 5) inputFile = args[argsFinalIndex-2];
        if(ioType == 4) inputFile = args[argsFinalIndex];
        if(ioType == 2 || ioType == 3 || ioType == 5 || ioType == 4) {
            args[argsFinalIndex] = NULL;
            args[argsFinalIndex-1] = NULL;
            if(ioType == 5) args[argsFinalIndex-2] = NULL;
        }
        if(ioType != 1) setOutput(ioType, outputFile);
        if(ioType == 4 || ioType == 5) setInput(ioType, inputFile);
        HandleCommand(args, background);
    }
}

