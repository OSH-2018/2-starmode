#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>

extern char **environ;

void pre(char * cmd, char *args[128]){
    int i;
    args[0] = cmd;
    for (i = 0; cmd[i] != '\n'; i++)
        ;
    cmd[i] = '\0';
    for (i = 0; *args[i]; i++)
        for (args[i+1] = args[i] + 1; *args[i+1]; args[i+1]++)
            if (*args[i+1] == ' ' || *args[i+1] == '\t'){
                *args[i+1] = '\0';
                args[i+1]++;
//                while(*args[i+1] == ' ' || *args[i+1] == '\t'){
//                    args[i+1]++;
//                }
                break;
            }
    args[i] = NULL;
}

char * inner(char *args[128]){
    if (strcmp(args[0], "cd") == 0){
        if (args[1])
            chdir(args[1]);
        return "1";
    }
    if (strcmp(args[0], "pwd") == 0){
        char wd[4096];
        return getcwd(wd, 4096);
    }
    if (strcmp(args[0], "exit") == 0)
        exit(0);

//    if (strcmp(args[0], "env") == 0){
//        //获得全部环境变量
//        if (!args[1]){
//            char * out = malloc(128*sizeof(char));
//            memset(out, 0, sizeof(out));
//            char *s = *environ;
//
//            for (int i=1; *(environ + i); i++){
//                s = *(environ + i);
//                strcat(out, s);
//                strcat(out, "\n");
//            }
////            strcat(out, "\0");
//            return out;
//        }
//            //获取单个环境变量
//        else if(args[1] && getenv(args[1])){
//            return (getenv(args[1]));
//        } else{
//            return "-1";
//        }
//    }
    if (strcmp(args[0], "export") == 0){
        if(args[1]){
            for(int i=0; *(args[1]+i); i++){
                if(*(args[1]+i)=='='){
                    *(args[1]+i) = '\0';
                    args[2] = (args[1]+i+1);
                    break;
                }
            }
            if(setenv(args[1], args[2], 1)!=0){
                return "-1";
            } else{
                return "1";
            }
        }
    }
}

char * usePipes(char *pipeArgs[32][128], int i){
    int sod = dup(STDOUT_FILENO), sid = dup(STDOUT_FILENO);
    int cd[2], fd[2];
    pid_t tmp1, tmp2;
    int red_in = 0, red_out = 0, f, addi=-1;
    if ((tmp1=fork()) == 0){
        pipe(cd);
        if((tmp2=fork()) == 0){
            for(int j=1;pipeArgs[i-1][j];j++){
                if(strcmp(pipeArgs[i-1][j], "<") == 0){
                    red_in = j;
                }
            }
            close(cd[0]);
            dup2(cd[1], STDOUT_FILENO);
            dup2(cd[1], STDERR_FILENO);
            if(red_in > 0){
                f = open(pipeArgs[i-1][red_in+1], O_RDONLY);
                dup2(f, STDIN_FILENO);
            }
            if(red_in != 0){
                pipeArgs[i - 1][red_in] = NULL;
            }
            oneStruct(pipeArgs[i-1]);
//            if(red_in == 0){
//                puts(-1);
//            }
            close(f);
        }else{
            wait(NULL);
            for(int j=1;pipeArgs[i][j];j++){
                if(strcmp(pipeArgs[i][j], ">") == 0){
                    red_out = j;
                    addi = 0;
                }else if(strcmp(pipeArgs[i][j], ">>") == 0){
                    red_out = j;
                    addi = 1;
                }
            }
            close(cd[1]);
            wait(NULL);
            char line[32768];
            memset(line, 0, sizeof(line));
            if (dup2(cd[0], STDIN_FILENO) == -1){
                exit(0);
            }
            if(addi == 0){
                f = open(pipeArgs[i][red_out+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(f, STDOUT_FILENO);
                dup2(f, STDERR_FILENO);
            }else if(addi == 1){
                f = open(pipeArgs[i][red_out+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                dup2(f, STDOUT_FILENO);
            }
            if(red_out != 0){
                pipeArgs[i][red_out] = NULL;
            }
            oneStruct(pipeArgs[i]);
            fflush(stdout);
            dup2(sid, STDIN_FILENO);
            dup2(sod, STDOUT_FILENO);
            close(f);
        }

    }else{
        wait(NULL);
    }
}

char * useTwoPipes(char *pipeArgs[32][128], int i){
    int sod = dup(STDOUT_FILENO), sid = dup(STDOUT_FILENO);
    int cd[2], fd[2];
    pid_t tmp1, tmp2, tmp3;
    int red_in = 0, red_out = 0, f, addi=-1;
    if ((tmp1=fork()) == 0){
        pipe(cd);
        if((tmp2=fork()) == 0){
            pipe(fd);
            if((tmp3=fork()) == 0){
                for (int j = 1; pipeArgs[i - 2][j]; j++)
                {
                    if (strcmp(pipeArgs[i - 2][j], "<") == 0)
                    {
                        red_in = j;
                    }
                }
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                if (red_in > 0)
                {
                    f = open(pipeArgs[i - 2][red_in + 1], O_RDONLY);
                    dup2(f, STDIN_FILENO);
                }
                if (red_in != 0)
                {
                    pipeArgs[i - 2][red_in] = NULL;
                }
                oneStruct(pipeArgs[i - 2]);
//                if (red_in == 0)
//                {
//                    puts(-1);
//                }
                close(f);
            }else{
                wait(NULL);
                close(cd[0]);
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                dup2(cd[1], STDOUT_FILENO);
                if (red_in > 0)
                {
                    f = open(pipeArgs[i - 1][red_in + 1], O_RDONLY);
                    dup2(f, STDIN_FILENO);
                }
                oneStruct(pipeArgs[i - 1]);
//                puts(-1);
                close(f);
            }
        }else{
            for(int j=1;pipeArgs[i][j];j++){
                if(strcmp(pipeArgs[i][j], ">") == 0){
                    red_out = j;
                    addi = 0;
                }else if(strcmp(pipeArgs[i][j], ">>") == 0){
                    red_out = j;
                    addi = 1;
                }
            }
            close(cd[1]);
            wait(NULL);
            char line[32768];
            memset(line, 0, sizeof(line));
            if (dup2(cd[0], STDIN_FILENO) == -1){
                exit(0);
            }
            if(addi == 0){
                f = open(pipeArgs[i][red_out+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(f, STDOUT_FILENO);
                dup2(f, STDERR_FILENO);
            }else if(addi == 1){
                f = open(pipeArgs[i][red_out+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                dup2(f, STDOUT_FILENO);
            }
            if(red_out != 0){
                pipeArgs[i][red_out] = NULL;
            }
            oneStruct(pipeArgs[i]);
            fflush(stdout);
            dup2(sid, STDIN_FILENO);
            dup2(sod, STDOUT_FILENO);
            close(f);
        }

    }else{
        wait(NULL);
    }
}

char * noPipes(char *args[128]){
    int sod = dup(STDOUT_FILENO), sid = dup(STDOUT_FILENO);
    int fd[2];
    pid_t tmp1;
    int red_in = 0, red_out = 0, f, addi=-1;
    pipe(fd);
    if ((tmp1=fork()) == 0){
        for(int j=1;args[j];j++){
            if(strcmp(args[j], "<") == 0){
                red_in = j;
            }
        }
        for(int j=1;args[j];j++){
            if(strcmp(args[j], ">") == 0){
                red_out = j;
                addi = 0;
            }else if(strcmp(args[j], ">>") == 0){
                red_out = j;
                addi = 1;
            }
        }
        if(red_in > 0){
            f = open(args[red_in+1], O_RDONLY);
            dup2(f, STDIN_FILENO);
        }
        if(addi == 0){
            f = open(args[red_out+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
            dup2(f, STDOUT_FILENO);
        }else if(addi == 1){
            f = open(args[red_out+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
            dup2(f, STDOUT_FILENO);
        }
        if(red_out != 0){
            args[red_out] = NULL;
        }
        if(red_in != 0){
            args[red_in] = NULL;
        }
        oneStruct(args);
        dup2(sid, STDIN_FILENO);
        dup2(sod, STDOUT_FILENO);

    }else{
        wait(NULL);
    }
}

void oneStruct(char *args[128]){
    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "pwd") == 0 ||
         strcmp(args[0], "exit") == 0 || strcmp(args[0], "export") == 0){
        char *output = inner(args);
        puts(output);
    } else
    {
        //wait(NULL);
        execvp(args[0], args);
        /* execvp失败 */
    }
}

int main(){
    /* 输入的命令行 */
    char cmd[256];
    /* 命令行拆解成的各部分，以空指针结尾 */
    char *args[128];
    char * pipeArgs[32][128];
    while (1) {
        for(int i=0;i<128;i++){
            args[i] = NULL;
        }
        for(int i=0;i<32;i++){
            for(int j=0;j<128;j++){
                pipeArgs[i][j] = NULL;
            }
        }
        /* 提示符 */
        printf("# ");
        fflush(stdin);
        fgets(cmd, 256, stdin);
        /* 预处理 */
        pre(cmd, args);
        /* 没有输入命令 */
        if (!args[0])
            continue;
        if (strcmp(args[0], "exit") == 0)
            exit(0);
        int last = 0;
        int pipes = 0;
        for(int i=0; args[i]; i++){
            if (strcmp(args[i], "|")==0){
                for (int j=last; j<i; j++){
                    pipeArgs[pipes][j-last] = args[j];
                }
                last = i+1;
                pipes ++;
            }
        }
        for (int j=last; args[j]; j++){
            pipeArgs[pipes][j-last] = args[j];
        }

        if(pipes==0){
            noPipes(args);
            continue;
        }
        else if (pipes == 1){/* 多个命令使用pipe*/
            usePipes(pipeArgs, pipes);
            continue;
        }else if (pipes == 2){/* 多个命令使用pipe*/
            useTwoPipes(pipeArgs, pipes);
            continue;
        }
    }
}