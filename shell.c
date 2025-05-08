#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>
#include <dirent.h>

#define SH_RL_BUFSIZE 1024
#define SH_TOK_BUFSIZE 64
#define SH_TOK_DELIM " \t\r\n\a"

/*
    Declarations for shell builtins:
*/
int shell_cd(char **args);
int shell_dir();
int shell_ls(char **args);
int shell_echo(char **args);
int shell_help(char **args);
int shell_exit(char **args);

/*
    List of builtins followed by the corresponding funcs
*/
char *builtin_str[] = {
    "cd",
    "dir",
    "ls",
    "echo",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &shell_cd,
    &shell_dir,
    &shell_ls,
    &shell_echo,
    &shell_help,
    &shell_exit
};

int shell_num_builtins() {
    return sizeof(builtin_str) / sizeof(char *);
}

/*
    Builting func implementations
*/
int shell_cd(char **args)
{
    if  (args[1]==NULL) {
        fprintf(stderr, "shell: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1])!=0) {
            perror("shell");
        }
    }
    return 1;
}

int shell_dir()
{
    char cwd[PATH_MAX]; //create an array with size of PATH_MAX
    if (getcwd(cwd,sizeof(cwd)) != NULL) { //Populate cwd array and ensure its existence
        printf("%s\n", cwd);
    } else {
        perror("getcwd error\n");
    }
}

int shell_ls(char **args) {
    DIR *d;
    struct dirent *dir;
    if (args[1]==NULL) {
        d=opendir(".");
    } else {
        d=opendir(args[1]);
    }
    if (d) {
        printf("-> | ");
        while ((dir=readdir(d)) != NULL) {
            printf("%s | ", dir->d_name);
        }
        printf("\n");
        closedir(d);
    }
    return 1;
}

int shell_echo(char **args)
{
    printf("Sorry, I'm not implemented yet!\n");
}

int shell_help(char **args)
{
    int i;
    printf("Blooper7's Shell\n");
    printf("The following functions are built in:\n");

    for (i=0; i<shell_num_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }

    printf("Use the man command for info on other programs.\n");
    return 1;
}

int shell_exit(char **args)
{
    printf("Bye!\n");
    return 0;
}

char *shell_read_line(void)
{
    char *line=NULL;
    ssize_t bufsize=0; // Make getline allocate the buffer instead.

    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS); //Got an EOF
        } else {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }

    return line;
}

char **shell_split_line(char *line)
{
    int bufsize=SH_TOK_BUFSIZE, position=0;
    char **tokens = malloc(bufsize*sizeof(char*));
    char *token;

    if (!tokens) {
        fprintf(stderr, "shell: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token=strtok(line, SH_TOK_DELIM);
    while (token != NULL) {
        tokens[position]=token;
        position++;

        if (position>=bufsize) {
            bufsize+=SH_TOK_BUFSIZE;
            tokens=realloc(tokens, bufsize*sizeof(char*));
            if (!tokens) {
                fprintf(stderr, "shell: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
        token=strtok(NULL, SH_TOK_DELIM);
    }
    tokens[position]=NULL;
    return tokens;
}

int shell_launch(char **args)
{
    pid_t pid, wpid;
    int status;

    pid=fork();
    if (pid==0) {
        //child proc
        if (execvp(args[0], args)==-1) {
            perror("shell");
        }
        exit(EXIT_FAILURE);
    } else if (pid<0) {
        //Fork errors
        perror("shell");
    } else {
        //Parent proc
        do {
            wpid=waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    return 1;
}

int shell_execute(char **args)
{
    int i;

    if (args[0]==NULL) {
        // an empty command was entered
        return 1;
    }

    for (i=0; i<shell_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i])==0) {
            return (*builtin_func[i])(args);
        }
    }
    return shell_launch(args);
}

void shell_loop(void)
{
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line=shell_read_line();
        args=shell_split_line(line);
        status=shell_execute(args); //return 1 for a successful exec, keeps the shell alive until failure

        free(line);
        free(args);
    } while (status);
}

void shell_init(void)
{
    printf("Blooper7's Shell\n");
}

int main(int argc, char **argv)
{
    //Load configs (if any)

    //Init
    shell_init();

    //CMD loop
    shell_loop();

    //Shutdown/cleanup

    return EXIT_SUCCESS;
}
