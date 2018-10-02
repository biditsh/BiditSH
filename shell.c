/* Shell Project - CSCI 273, Operating Systems, Spring 2018
    Author: Bidit Sharma,
    Date: 3/11/2018

This is a source code of a custom shell build using C programming language*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define MAXTOKS 20
#define MAXBUFF 1024

enum status_value {
    NORMAL = 1,
    EOF_OR_ERROR = 0,
    TOO_MANY_TOKENS=2
};

/*name strucure holds the information about tokens*/
struct name{
    char** tok;
    int count;
    int status;
};


int wordcounter(const char* string, int length){
    int ret=0;
    int i =0;   //i is starting from 1 to omit the \n that comes after program is called ./shell\n
    while (string[i]==' ')    //getting rid of spaces at the beginning
        ++i;
    if (i>=length-1)
        ret=0;  //returning 0 if there if input is only spaces
    else{
        for (i; i<length; i++){
          if (string[i]==' ' && string[i+1]!=' ' && string[i+1]!='\0' && string[i+1]!='\n')
            ret++;
        }
    ret++; //for the last word
    }
    return ret;
}




int read_name(struct name * str){
    char* getlinebuff = NULL;
    size_t getlinelen = 0;
    fseek(stdin,0,SEEK_END);
    int length = getline(&getlinebuff, &getlinelen, stdin);
    str-> count= wordcounter(getlinebuff, length);
   // printf("Length of input: %d  , Input : '%s', count : %d\n", length, getlinebuff, str->count);
    if (length <0 || str->count == 0){
        str->status = EOF_OR_ERROR;
        return str->status;         //exit from the function
    }
    else
        str->status = NORMAL;

    //char** allocator1;
    if (str->count > MAXTOKS){
        str->status = TOO_MANY_TOKENS;
        str->count=MAXTOKS;
    }
    str->tok = (char**) malloc((str->count+1)*sizeof(char*));  //+1 for 0 pointer at last tok


    int startbuff=0;
    int endbuff=0;
    int tokencount=0;
    int i=0;


    for (i; i<length; i++){
        while (getlinebuff[i]==' '){    //getting rid of spaces at the beginning
            i++;
            startbuff=i;
        }

        if (getlinebuff[i]!=' ' && (getlinebuff[i+1] == '\0' || getlinebuff[i+1] == ' ' || getlinebuff[i+1] == '\n')){
            endbuff=i+1;  //+1 has \0 \n or ' ', and will be used to store nullbyte
            int tokensize = endbuff-startbuff;
            // printf("tokensize : %d\n", tokensize); 
            str->tok[tokencount] = (char*) malloc((tokensize+1)*sizeof(char)); /*the last position (of ' ', or '\n' or '\0') '\0' character */
            int j=startbuff;
            for (j; j<endbuff; j++)
                str->tok[tokencount][j-startbuff] = getlinebuff[j];    //str->tok[x] will contain a 'word'
            str->tok[tokencount][tokensize] = '\0';
            startbuff = i+2; //new startbuff starts from the position after ' ', '\n', or '\0'
            tokencount++;
            if (tokencount == MAXTOKS - 1 ){
                endbuff = length-1;
                str->tok[tokencount] = (char*) malloc((length-startbuff)*sizeof(char));  //allocating if the total no. of tokens > MAXTOKS
                j=startbuff;
                for (j; j<endbuff; j++){
                    str->tok[tokencount][j-startbuff] = getlinebuff[j];  
                }
                str->tok[tokencount][startbuff-endbuff] = '\0';
                i=length+1; //make the loop break
            }
        }
    }
   str->tok[str->count] = 0;
     return str->status;  // function returns the status
}


/* the commands are executed as a child process using this function*/
int run_as_child(char ** commands, char ** envp){
    pid_t child_pid;
 
 //forking
    if ((child_pid = fork()) < 0) {
        printf("fork() FAILED");
        perror("fork");
        _exit(EOF_OR_ERROR);
    }
            
    /* fork() succeeded */
    wait(NULL);
    if (child_pid == 0){
        execve(commands[0], commands, envp);
        //if execve doesn't execute
        printf("%s: Unknown command\n", commands[0]);
        _exit(EOF_OR_ERROR);
         }

 }    


/*function to check the presence of pipe or i/o redirections '<', '>', '|', returns 1, 2 or 3 respectively*/
int check_redirs(char ** commands){
    int i = 0;
    while (commands[i] != NULL){
        if (!strcmp(commands[i], "|"))
            return 1;
        if (!strcmp(commands[i],">"))
            return 2;
        if (!strcmp(commands[i],"<"))
            return 3;
        i++;
    }
    return 0; 
}


 //implementation of the pipe feature   --- EXTRA FEATURE
int pipeExec(char ** commands,char ** envp){  

     int A[2];         //input output array for the pipe
     pid_t p1, p2;

/* Breaking the token into two parts: right and left of the pipe*/ 
    int length1 = 0;
    int length2 = 0;
    int toggle = 0;  
    char ** commands_parsed1;
    char ** commands_parsed2;
    int i=0;

    while (commands[i] != NULL){
        if (toggle == 0)
            length1++;
        else
            length2++;
        if (!strcmp(commands[i], "|"))
            toggle = 1;
        i++;
    }

    length1--;   //length1 also takes pipe into it, so removing it

/* parsing the tokens */
    commands_parsed1 = (char**) malloc((length1+1)*sizeof(char*));
    for (i=0; i<length1; i++)
        commands_parsed1[i] = commands[i]; 
    commands_parsed1[length1] = 0;

    commands_parsed2 = (char**) malloc((length2+1)*sizeof(char*));
    for (i=0; i<length2; i++)
        commands_parsed2[i] = commands[length1 + 1 + i]; 
    commands_parsed2[length2] = 0;

//fork for the right part of pipe
    if (pipe(A) < 0){
        printf ("pipe FAILED");
        perror("pipe");
        return EOF_OR_ERROR;
    }

    if ((p1 = fork()) < 0){
        printf ("pipe fork FAILED");
        perror("pipe_fork");
        _exit(EOF_OR_ERROR);
        }


    if (p1 == 0){
        close(A[0]);
        dup2(A[1], 1);  //A[1] will be acting as the standard output
        printf("PID in: %d\n", getpid());
        execvp(commands_parsed1[0], commands_parsed1);   //the command is executed
             //if execve doesn't execute
        printf("%s: Unknown command\n", commands_parsed1[0]);
        _exit(0);        
    }


//fork for the left part of pipe
    if ((p2 = fork()) < 0){
        printf ("pipe fork FAILED");
        perror("pipe_fork");
        _exit(EOF_OR_ERROR);
        }

    if (p2 == 0){
        close(A[1]);
        dup2(A[0], 0);   //the input is read from A[0] instead of the standard input

        execvp(commands_parsed2[0], commands_parsed2);  //the command is executed
            //if execve doesn't execute
        printf("%s: Unknown command\n", commands_parsed2[0]);
        _exit(0);
    }

    waitpid(p1);  //waiting for right hand side of pipe to execute
    close(A[1]);  
    waitpid(p2);    //waiting for left hand sidef to complete execution
    close(A[0]);


//memory management
    free(commands_parsed1);
    free(commands_parsed2);

    //_exit(0);
    return 0;
 }


/* funciton to deal with input and output redirection. args are array of tokens, environment and a characted 'i' for input redirection and 'o' for output redirection  --- EXTRA FEATURE */
int redir_output(char ** commands, char ** envp, char io){
    int length1 = 0;
    int length2 = 0;
    int toggle = 0;
    int i;    
    char ** commands_parsed1;
    char ** commands_parsed2;
    int pid;

/* separating the input token into right and and left hand side commands */

    while (commands[i] != NULL){
        if (toggle == 0)
            length1++;
        if (!strcmp(commands[i], ">") || !strcmp(commands[i], "<"))
            toggle = 1;
        i++;
    }

    length1--;   //length1 also takes pipe into it, so removing it

    /*parsing the commands */
    commands_parsed1 = (char**) malloc((length1+1)*sizeof(char*));
    for (i=0; i<length1; i++)
        commands_parsed1[i] = commands[i]; 
    commands_parsed1[length1] = 0;

    commands_parsed2 = (char**) malloc((2)*sizeof(char*));
        commands_parsed2[0] = commands[length1+1]; 
        commands_parsed2[1] = 0;

/* a child process is created for the execution*/
        if ((pid = fork())<0){
            printf ("pipe fork FAILED");
            perror("pipe_fork");
            _exit(EOF_OR_ERROR);
        }


        if (pid == 0){
           // int fd = open (commands_parsed2[0], O_CREAT | O_TRUNC | O_WRONLY);
/* if io = 'o', output stream is redirected to the file*/
            if (io == 'o'){
                int fd = creat (commands_parsed2[0], 0644);
                if (fd < 0){
                    printf ("file write FAILED");
                    perror("output_redirection");
                    return(EOF_OR_ERROR);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

/* if io='i', input stream is redirected from the file*/
            if (io == 'i'){
                int fd = open ("a.txt", O_RDONLY);
                if (fd < 0){
                    printf ("file write FAILED");
                    perror("output_redirection");
                    return(EOF_OR_ERROR);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
/*execution of command*/
            execve(commands_parsed1[0], commands_parsed1, envp);
             //if execve doesn't execute
            printf("%s: Unknown command\n", commands_parsed1[0]);
            _exit(0);
        }
        else{ 
            waitpid(pid, 0 ,0);
        }

        free(commands_parsed1);
        free(commands_parsed2);
        return 0;          
}

//==================================================================================

/*the main function */
int main(int argc, char * * argv, char * * envp){   
    int i = 0;
    char cwd[MAXBUFF];
    int parent_pid = getpid();
        // printf(" Parent PID: %d\n", parent_pid);
    struct name Command;

    while(getpid() == parent_pid){
        if (getcwd(cwd, sizeof(cwd)) == NULL){             //retrieves current directory information for the prompt
            printf("Error retrieving current directory info.\n");
            perror("getcwd() error");
            _exit(EOF_OR_ERROR);
        }

        printf("bidish::%s%% ", cwd);  //prints the prompt
        int ret = read_name(&Command);

    //      i =0;
    //     while (Command.tok[i] != NULL){
    //     printf("%s ", Command.tok[i]);
    //     i++;
    //          }
    // printf("\n");

        if (ret == NORMAL){       //if read_name returns NORMAL, following if-else checks occur for proper execution of command
                if (!strcmp("exit", Command.tok[0])){
                    printf("Thank you for using Bidit's Shell v1.0\n");
                    _exit(EOF_OR_ERROR);
                }

                /*clear screen  --- EXTRA FEATURE*/
                else if (strcmp(Command.tok[0],"clear") == 0)
                    system("clear");

                /*help function*/
                else if (!strcmp("help", Command.tok[0])){
                    printf("Welcome to Bidit's Shell v1.0.\n");
                    printf("Standard Commands: \n cd -> change directory (similar syntax to bash shell)\n list -> lists the items in the current directory \n exit -> exit the shell\n");
                }

                /*cd command to change the working directory  --- EXTRA FEATURE*/
                else if (!strcmp("cd", Command.tok[0])){
                    if ((chdir(Command.tok[1]))<0){
                        printf("chdir() FAILED\n");
                        perror("cd");
                    }
                }

                /*list command to list the contents of the working directory  --- EXTRA FEATURE*/
                else if (!strcmp("list", Command.tok[0])){
                    char ** list_cmd;
                    list_cmd = (char**) malloc(3*sizeof(char*));
                    list_cmd[0] = (char*) malloc(20*sizeof(char));
                    strncpy(list_cmd[0], "/bin/ls", 20);  //uses /bin/ls to display the contents of directoy
                    list_cmd[1] = (char*) malloc(5*sizeof(char));
                    strncpy(list_cmd[1], ".", 5);
                    list_cmd[2] = 0;

                    run_as_child(list_cmd, envp);

                    //memory management
                    free (list_cmd[0]);
                    free (list_cmd[1]);
                    free (list_cmd);
                }

                /*using check_redirs funciton to check the presence of pipe  --- EXTRA FEATURE*/
                else if (check_redirs(Command.tok) == 1){
                    pipeExec(Command.tok, envp);
                   // _exit(0);
                }

                /*using check_redirs funciton to check the presence of output redirection  --- EXTRA FEATURE*/
                else if (check_redirs(Command.tok) == 2){
                    redir_output(Command.tok, envp, 'o');
                }

                /*using check_redirs funciton to check the presence of input redirection --- EXTRA FEATURE*/ 
                else if (check_redirs(Command.tok) == 3){
                    redir_output(Command.tok, envp, 'i');
                }

                //execution starts here for rest of the commands
                else{
                    run_as_child(Command.tok, envp);
               }
           }

           /*error reports*/
        else if (ret == 2)
            printf("Maximum no. of tokens read, Return: %d\n",ret );
     
        else
            printf("Error Reading Tokens, Return: %d\n",ret );
            
        //memory management
        i = 0;
        for (i; i<=Command.count; i++)
            free(Command.tok[i]);
        if (Command.count > 0)
            free(Command.tok);
    
    }
    _exit(0);
}
