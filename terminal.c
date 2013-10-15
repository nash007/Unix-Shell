#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<malloc.h>
#include<limits.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<signal.h>
#include<linux/sched.h>
#include<fcntl.h>
#include <sys/stat.h>

char home_rel[1000];
int redirect_input=0;
int redirect_output=0;
int redirect_input_index=0;
int redirect_output_index=0;
int pipe_true=0;
int count_pipes=0;
int pipe_pos[100];
int redirect_out_end;
char infl[100];
char outfl[100];

typedef struct job
{
    char name[100];
    int pid;
    struct job *next;
}job;

job *head,*end;

void insert_job(char a[100],int id)
{
    if(head == NULL)
    {
        head = (job*)malloc(sizeof(job));
        strcpy(head->name,a);
        head->pid=id;
        head->next=NULL;
        end = (job*)malloc(sizeof(job));
        end = head;
    }
    else
    {
        job *temp = (job*)malloc(sizeof(job));
        strcpy(temp->name,a);
        temp->pid=id;
        end->next=temp;
        end=temp;
    }
}

void print_jobs()
{
    job *temp = (job*)malloc(sizeof(job));
    temp = head;
    int count = 1;
    while(temp!=NULL)
    {
        printf("[%d] %s %d\n",count++,temp->name,temp->pid);
        temp = temp -> next;
    }
}

void delete_job(int val)
{
    int del=0;
    job *p=(job*)malloc(sizeof(job));
    p=head;
    if(p==NULL)
        del=1;
    else
    {
        if(head->pid == val)
        {
            head=head->next;
            return;
        }
        job *q=(job*)malloc(sizeof(job));
        while(p!=NULL && p->pid!=val)
        {
            q=p;
            p=p->next;
        }
        if(p==NULL)
            return;
        q->next=p->next;
    }
    p=head;
    while(p!=NULL&&p->next!=NULL)
        p=p->next;
    end=p;
}

void sig_handler(int signo)
{
    int status;
    pid_t temp;
    if (signo == SIGUSR1)
        printf("received SIGUSR1\n");
    else if (signo == SIGKILL)
        printf("received SIGKILL\n");
    else if (signo == SIGSTOP)
        printf("received SIGSTOP\n");
    else if (signo == SIGINT)
        printf("received SIGINT\n");
    else if (signo == SIGCHLD)
    {
        int tmp,stats;
        tmp=waitpid(-1,&stats,WNOHANG|WUNTRACED|WCONTINUED);
        while(tmp>0)
        {
            printf("Process with pid %d exitted\n",tmp);
            if(WIFSIGNALED(stats)||WIFEXITED(stats))
                delete_job(tmp);
            else if(WIFSTOPPED(stats))
            {
                char p[100];
                sprintf(p,"/proc/%d/status",tmp);
                FILE *fil=fopen(p,"r");
                fgets(p,1000,fil);
                printf("%s\n",p);
                insert_job(p+6,tmp);
            }
            tmp=waitpid(-1,&stats,WNOHANG|WUNTRACED|WCONTINUED);
        }
    }
}

void ps1(char *home,char *username,char *hostname)
{
    int len=strlen(home);
    char cwd[100];
    getcwd(cwd,sizeof(cwd));
    if(strncmp(home,cwd,len) == 0)
        printf("<%s@%s:~%s> ",username,hostname,cwd+len);
    else
        printf("<%s@%s:%s> ",username,hostname,cwd);
}

int split(char *inp,char delim,char *a[100])
{
    char c;
    int i,j=0,t=0,row=0,len=strlen(inp);
    count_pipes = 0,pipe_true = 0,redirect_input = 0,redirect_output = 0,redirect_out_end = 0;
    a[0]=(char**)malloc(sizeof(char[100]));
    while(inp[t]==' ' || inp[t] == '\t')
        t++;
    while(inp[len-1] == ' ' || inp[len-1] == '\t')
        len--;
    for(i=t;i<len;i++)
    {
        if(inp[i]==delim || inp[i]==' ' || inp[i]=='\t')
        {
            while(inp[i]==' '|| inp[i] == '\t')
                i++;
            if(i!=len-1 || inp[i]!=' ' || inp[i]!='\t')
            {
                row++;
                a[row]=(char**)malloc(sizeof(char[100]));
                a[row][0]=inp[i];
                if(a[row-1]!=NULL)
                    a[row-1][j]='\0';
                j=1;
                if(a[row-1]!=NULL && a[row-1][0] == '<')
                {
                    redirect_input = 1;
                    redirect_input_index = row;
                }
                if(a[row-1]!=NULL && a[row-1][0] == '>')
                {
                    redirect_out_end = count_pipes;
                    redirect_output = 1;
                    redirect_output_index = row;
                }
                if(a[row]!=NULL && a[row][0] == '|')
                {
                    pipe_true = 1;
                    a[row]=NULL;
                    pipe_pos[count_pipes++] = row;
                }
            }
        }
        else
            a[row][j++]=inp[i];
    }
    a[row][j]='\0';
    a[row+1]=NULL;
    return row;
}

void pinfo(char pid[100],int flag,char info[2000])
{

    char buf[1024],contents[10000],*fields[100],status[100],id[100],mem[100];
    FILE *fp;
    int i,len_fields;
    ssize_t len;
    if(flag == 0)
    {
        fp=fopen("/proc/self/status","r");
        len = readlink("/proc/self/exe", buf, sizeof(buf));
    }
    else
    {
        char path[100],path1[100];
        sprintf(path,"/proc/%s/status",pid);
        sprintf(path1,"/proc/%s/exe",pid);
        fp=fopen(path,"r");
        if(fp==NULL)
        {
            printf("No such process!\n");
            return;
        }
        len = readlink(path1, buf, sizeof(buf));
    }
    for(i=0;i<15;i++)
    {
        fgets(info,150,fp);
        if(i==1)
        {
            len_fields = split(info,'\t',fields);
            strcpy(status,fields[1]);
        }
        else if(i==3)
        {
            len_fields = split(info,'\t',fields);
            strcpy(id,fields[1]);
        }
        else if(i==11)
        {
            len_fields = split(info,'\t',fields);
            sprintf(mem,"%s %s",fields[1],fields[2]);
        }
    }
    printf("Pid: %sStatus: %c%c\nMemory: %s",id,status[0],status[1],mem);
    int len_home = strlen(home_rel);
    if(strncmp(home_rel,buf,len_home) == 0)
        printf("Executable Path: ~%s\n",buf+len_home);
    else
        printf("Executable Path: %s\n",buf);
}

void redirect(char inp[1000])
{
    char *a[100];
    int normal_in,normal_out;
    split(inp,' ',a);
    if(redirect_input==1)
    {
        normal_in=dup(0);
        int in = open(a[redirect_input_index],O_RDONLY);
        dup2(in,0);
        close(in);
    }
    if(redirect_output==1)
    {
        normal_out=dup(1);
        int out = open(a[redirect_output_index],O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR,0777);
        dup2(out,1);
        close(out);
    }
    a[redirect_input_index-1]=NULL;
    a[redirect_output_index-1]=NULL;

    execvp(a[0],a);
    if(redirect_input==1)
        dup2(normal_in,0);
    if(redirect_output==1)
        dup2(normal_out,1);
    redirect_input = 0;
    redirect_output = 0;
}

void piping(char inp[1000])
{
    int i,status,pipes[20],j;
    char *a[100];
    split(inp,' ',a);
    for(i=0;i<count_pipes;i++)
        pipe(pipes+2*i);                //declaring the number of pipes required
    for(i=0;i<=count_pipes;i++)
    {
        if(fork()==0)
        {
            if(i==0)
            {
                if(redirect_input == 1)
                {
                    int infile = open(a[2],O_RDONLY);
                    a[1]=NULL;
                    dup2(infile,0);
                }
                dup2(pipes[1],1);
                for(j=0;j<count_pipes;j++)
                {
                    close(pipes[2*j]);
                    close(pipes[2*j+1]);
                }
                execvp(a[0],a);
            }
            else if(i==count_pipes)
            {
                if(redirect_out_end == count_pipes && redirect_output == 1)
                {
                    int outfile = open(a[redirect_output_index],O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR,0777);
                    a[redirect_output_index-1]=NULL;
                    dup2(outfile,1);
                }
                dup2(pipes[2*(count_pipes-1)],0);
                for(j=0;j<count_pipes;j++)
                {
                    close(pipes[2*j]);
                    close(pipes[2*j+1]);
                }
                execvp(a[pipe_pos[count_pipes-1]+1],a+pipe_pos[count_pipes-1]+1);
            }
            else
            {
                dup2(pipes[(i-1)*2],0);
                dup2(pipes[i*2+1],1);
                for(j=0;j<count_pipes;j++)
                {
                    close(pipes[2*j]);
                    close(pipes[2*j+1]);
                }
                execvp(a[pipe_pos[i-1]+1],a+pipe_pos[i-1]+1);
            }
        }
        else
            continue;
    }
    for(j=0;j<count_pipes;j++)
    {
        close(pipes[2*j]);
        close(pipes[2*j+1]);
    }
    for(i=0;i<count_pipes+1;i++)
        wait(&status);
    count_pipes = 0;
}

int main()
{
    char info[2000],temp[100],input[1000],*args[100],cwd[1000],user[100],host[100],PS1[100],newPS1[100],path[1000];
    int pipes[20],out,stats,tmp,status,argv,check,flag=0,jn,sn,cn,pd;
    pid_t child;
    job *tp = (job*)malloc(sizeof(job));
    getlogin_r(user,sizeof(user));
    getcwd(cwd,sizeof(cwd));
    strcpy(home_rel,cwd);
    gethostname(host,sizeof(host));
    write(1,"\e[1;1H\e[2J",10);
    while(strcmp(input,"quit"))
    {
        signal (SIGINT, SIG_IGN);
        signal (SIGQUIT, SIG_IGN);
        signal (SIGTSTP, SIG_IGN);
        signal (SIGTTIN, SIG_IGN);
        signal (SIGTTOU, SIG_IGN);
        signal (SIGCHLD, sig_handler);
        flag=0;
        setpgid(getpid(),getpid());
        ps1(cwd,user,host);
        tcsetpgrp(STDERR_FILENO,getpgid(getpid()));
        gets(input);
        argv = split(input,' ',args);
        if(strcmp(args[0],"cd") == 0)
        {
            if(argv!=0)
                realpath(args[1],path);
            else
                strcpy(path,cwd);
            check = chdir(path);
            if(check==-1)
                write(1,"No such directory\n",18);
            continue;
        }
        else if(strcmp(args[0],"pinfo") == 0)
        {
            if(argv==0)
                pinfo("1",0,info);
            else 
                pinfo(args[1],1,info);
        }
        else if(strcmp(args[0],"jobs") == 0)
            print_jobs();
        else if(strcmp(args[0],"kjob") == 0)
        {
            if(argv < 2)
            {
                printf("Error! Too few arguments\n");
                continue;
            }
            jn = atoi(args[1]);
            sn = atoi(args[2]);
            if(sn > 30 || sn < 1)
            {
                printf("Wrong signal number\n");
                continue;
            }
            tp = head;
            cn = 1;
            while(tp!=NULL)
            {
                if(cn == jn)
                {
                    pd = tp->pid;
                    break;
                }
                tp=tp->next;
                cn++;
            }
            kill(pd,sn);
        }
        else if(strcmp(args[0],"fg") == 0)
        {
            jn = atoi(args[1]);
            tp = head;
            cn = 1;
            while(tp!=NULL)
            {
                if(cn == jn)
                {
                    pd = tp->pid;
                    break;
                }
                tp=tp->next;
                cn++;
            }
            delete_job(pd);
            tcsetpgrp(STDIN_FILENO,getpgid(pd));
            kill(pd,SIGCONT);
            int tmp,stats;
            tmp=waitpid(-1,&stats,WUNTRACED);
            if(WIFSTOPPED(stats))
            {
                char pth[100];
                sprintf(pth,"/proc/%d/status",tmp);
                FILE *fl=fopen(pth,"r");
                fgets(pth,1000,fl);
                printf("%s\n",pth);
                insert_job(pth+6,tmp);
            }
            tcsetpgrp(STDIN_FILENO,getpgid(getpid()));
        }
        else if(strcmp(args[0],"overkill") == 0)
        {
            tp = head;
            while(tp!=NULL)
            {
                pd = tp->pid;
                tp=tp->next;
                kill(pd,9);
            }
        }
        else if(strcmp(args[0],"cd")!=0)
        {
            if(strcmp(args[argv],"&") == 0)
            {
                flag=1;
                args[argv]=NULL;
            }
            int savin=dup(0);
            int savout=dup(1);
            child = fork();
            if(child>=0)
            {
                if(child == 0)
                {
                    signal (SIGINT, SIG_DFL);
                    signal (SIGQUIT, SIG_DFL);
                    signal (SIGTSTP, SIG_DFL);
                    signal (SIGTTIN, SIG_DFL);
                    signal (SIGTTOU, SIG_DFL);
                    signal (SIGCHLD, SIG_DFL);
                    setpgid(getpid(),getpid());
                    if(pipe_true == 1)
                        piping(input);
                    else
                    {
                        if(redirect_input == 1 || redirect_output == 1)
                            redirect(input);
                        else
                            execvp(args[0],args);
                    }
                    exit(0);
                }
                else
                {
                    setpgid(child,child);
                    if(flag == 1)
                    {
                        insert_job(args[0],child);
                        continue;
                    }
                    else
                    {
                        tcsetpgrp(STDIN_FILENO,getpgid(child));
                        int tmp,stats;
                        tmp=waitpid(-1,&stats,WUNTRACED);
                        dup2(savin,0);
                        close(savin);
                        dup2(savout,1);
                        close(savout);
                        redirect_input = 0;
                        redirect_output = 0;
                        if(WIFSTOPPED(stats))
                        {
                            char ph[100];
                            sprintf(ph,"/proc/%d/status",tmp);
                            FILE *f=fopen(ph,"r");
                            fgets(ph,1000,f);
                            printf("%s\n",ph);
                            insert_job(ph+6,tmp);
                        }
                        tcsetpgrp(STDIN_FILENO,getpgid(getpid()));
                    }
                }
            }
            else
                write(1,"Failed\n",7);
        }
        redirect_input = 0;
        redirect_output = 0;
        redirect_input_index = 0;
        redirect_output_index = 0;
        count_pipes = 0;
    }
    write(1,"\e[1;1H\e[2J",10);
    return 0;
}
