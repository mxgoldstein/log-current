/* 
    log-current - Tool for observing currently active log files
    (c) mxgoldstein, 2016
    
    Licensed under the MIT License. See LICENSE.
     
    Changelog:
    v0.2.1 (08/10/2016)
        * Fixed --list-only, -l printing something that should have been omitted
        * Contradictory parameters now result in a warning
        * More verbose parameter error messages
    v0.2.0 (08/10/2016)
        * Added --auto, -a option
        * Added --command, -c option
        * Added --list-only, -l option
        * Optimized memory allocation
        * Error messages are now use stderr
        * Fixed segmention fault if non-existent directory was specified
    v0.1.0 (07/10/2016)
        * Initial release
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef DEFAULT_LOG_DIR
    #error DEFAULT_LOG_DIR undefined
    #error Define it using cc -DDEFAULT_LOG_DIR='"/path/to/logs"'...
    #define DEFAULT_LOG_DIR ""
#endif

#ifndef DEFAULT_COMMAND
    #error DEFAULT_COMMAND undefined
    #error Define it using cc -DDEFAULT_COMMAND='"command-goes-here"'...
    #define DEFAULT_COMMAND ""
#endif

typedef struct file
{
    char* name;
    size_t size;
    struct file* next;
} file_t;

typedef enum
{
    MATCH_NONE=-1,
    MATCH_ALL=0,
    MATCH_NAME_ONLY=1
} match_t;


file_t* snapshot = NULL;
file_t* changed = NULL;
DIR* log_dir;


/* Modes */
int auto_mode = 0,
    list_only = 0;

int seconds = 2;
char* log_dir_path = NULL;
char* command = NULL;

char HELP[] = 
"\
Usage:\n%s [options]\n\n\
--auto, -a - Automatically select the first log file and ignore all others\n\
--command, -c <command> - Command to be applied to selected log file\n\
--directory, -d <directory> - Set directory to observe\n\
--list, -l - Only list files\n\
--wait, -w <delay> - Wait a specified amount of seconds\n\
";

/* Add a file to the list */
void file_add(file_t** root,const char* name,const size_t size)
{
    /* Allocate file_t */
    file_t* file;
    
    file = malloc(sizeof(file_t));
    
    /* Copy name into file->name */
    file->name = malloc(strlen(name)+1);
    strcpy(file->name,name);
    file->size = size;
    file->next = NULL;
    
    /* Append node to list, if list is NULL, turn it into the root node */
    if (*root == NULL) 
    {
        *root = file;
    }
    else
    {
        /* Get last node */
        file_t* last;
        for (last = *root;last->next != NULL;last = last->next);

        last->next = file;
    }
}

/* Try to find a node matching in name and size,
   if a matching node is found MATCH_ALL is returned,
   if there is no file name that matches to name MATCH_NONE is returned,
   if there is a file name that matches to name, but not to size, MATCH_NAME_ONLY is returned.
*/
match_t file_match(file_t* root,const char* name,const size_t size)
{
    file_t* f;
    for (f = root;f != NULL;f = f->next)
    {
        /* Check if names match */
        if (strcmp(f->name,name) == 0)
        {
            /* Check if sizes match */
            if (f->size == size) return MATCH_ALL;
            else             return MATCH_NAME_ONLY;
        }
    }
    return MATCH_NONE;
}

/* Remove all nodes and free their memory */
void file_cleanup(file_t* root)
{
    file_t* f;
    for (f = root;f != NULL;)
    {
        file_t* next = f->next;
        free(f->name);
        free(f);

        f = next;
    }
}

/* Return file size of file at log_dir_path/name, 
   return -1 if path does not point to a file
   if file can't be opened for reading, the whole program will be terminated
*/
size_t file_size(const char* name)
{
    struct stat st;
    FILE* fd;
    char* filename;
    size_t size;

    /* Comibine log_dir and name into a path */
    filename = malloc(strlen(log_dir_path)+strlen(name)+1);
    sprintf(filename,"%s%s",log_dir_path,name);

    /* Check if filename points to a regular file or not */
    stat(filename,&st);
    if (!S_ISREG(st.st_mode)) 
    {
        free(filename);
        return -1;
    }
    
    /* Open file */
    fd = fopen(filename,"r");
    if (!fd)
    {
        fprintf(stderr,"Failed to open %s for reading\nTry running log-current as super user\n",name);
        free(filename);
        free(command);
        free(log_dir_path);
        closedir(log_dir);
        file_cleanup(snapshot);
        file_cleanup(changed);
        exit(1);
    }
    /* Seek end of file and return offset from beginnig */
    fseek(fd,0,SEEK_END);
    size = ftell(fd);
    fclose(fd);
    free(filename);
    return size;
}

int main(int argc,char** argv)
{
    const char* tmp_dir = DEFAULT_LOG_DIR;
    struct dirent* ent;
    time_t t;

    if (argc != 1)
    {
        int i;
        for (i = 1;i < argc;i++)
        {
        
            #define ifcmd(sname,lname) if (strcmp(argv[i],sname) == 0 || strcmp(argv[i],lname == 0))
            
            if (strcmp(argv[i],"--auto") == 0 || strcmp(argv[i],"-a") == 0)
            {
                auto_mode = 1;
            }
            else if (strcmp(argv[i],"--command") == 0 || strcmp(argv[i],"-c") == 0)
            {
                if (argc == i+1) 
                {
                    fprintf(stderr,"Too few arguments given\n");
                    break;
                }
                command = malloc(strlen(argv[i+1])+1);
                strcpy(command,argv[++i]);
                if (list_only)
                    fprintf(stderr,"Warning: --command (-c) and --list-only (-l) contradict each other\n");    
            }
            else if (strcmp(argv[i],"--directory") == 0 || strcmp(argv[i],"-d") == 0)
            {
                if (argc == i+1) 
                {
                    fprintf(stderr,"Too few arguments given\n");
                    break;
                }
                tmp_dir = argv[++i];
            }
            else if (strcmp(argv[i],"--help") == 0 || strcmp(argv[i],"-h") == 0)
            {
                printf(HELP,argv[0],argv[0]);
                return 0;
            }
            else if (strcmp(argv[i],"--list") == 0 || strcmp(argv[i],"-l") == 0)
            {
                list_only = 1;
                if (command != NULL)
                    fprintf(stderr,"Warning: --command (-c) and --list-only (-l) contradict each other\n"); 
            }
            else if (strcmp(argv[i],"--wait") == 0 || strcmp(argv[i],"-w") == 0)
            {
                if (argc == i+1) 
                {
                    fprintf(stderr,"Too few arguments given\n");
                    break;
                }
                seconds = atoi(argv[++i]);
            }
            else 
            {
                fprintf(stderr,"Invalid argument '%s'\n",argv[i]);
                break;
            }
        }
        if (i != argc)
        {
            fprintf(stderr,"%s: Invalid arguments given\n",argv[0]);
            fprintf(stderr,HELP,argv[0],argv[0]);
            return 1;
        }
    }
    
    /* Initalize  command (if not already specified */
    if (command == NULL)
    {
        command = malloc(strlen(DEFAULT_COMMAND)+1);
        strcpy(command,DEFAULT_COMMAND);
    }

    /* Initialize log_dir_path (fixes missing / on tmp_dir) */
    log_dir_path = malloc(strlen(tmp_dir)+1+(tmp_dir[strlen(tmp_dir)-1] == '/' ? 0 : 1));
    sprintf(log_dir_path,"%s%s",tmp_dir,(tmp_dir[strlen(tmp_dir)-1] == '/' ? "" : "/"));
    
    log_dir = opendir(log_dir_path);    

    /* Check if specified directory exists */
    if (!log_dir)
    {
        fprintf(stderr,"%s can't be opened for reading\nCheck if the directory exists or try running log-current as super user\n",log_dir_path);
        return 1;
    }

    /* Create a snapshot of the directory state... */
    while ((ent = readdir(log_dir)))
    {
        if (strncmp(ent->d_name,".",1) == 0) continue; /* Ignore hidden files, . and .. */
        file_add(&snapshot,ent->d_name,file_size(ent->d_name)); /* Append file to snapshot */
    }

    
    /* This code works of the assumption that time_t is measured in seconds, this may not always be true */
    t = time(NULL);
    if (!list_only) printf("Waiting %d second%s...\n",seconds,seconds==1?"":"s");
    while (time(NULL) - t < seconds);

    /* Rewind directory */
    rewinddir(log_dir);

    /* ...and compare it with a newer version */
    while ((ent = readdir(log_dir)))
    {
        if (strncmp(ent->d_name,".",1) == 0) continue; /* Ignore hidden files, . and .. */
        if (file_match(snapshot,ent->d_name,file_size(ent->d_name)) != MATCH_ALL)
        {
            file_add(&changed,ent->d_name,file_size(ent->d_name)); /* Append file to changed */
        }
    }

    /* If a file was changed */
    if (changed)
    {
        file_t* f;
        int i,input;
    
        /* Print currently active log files */    
        i=0;
        if (!list_only) printf("Active log files:\n\n");
        for (f = changed;f != NULL;f = f->next)
        {
            if (list_only || auto_mode)
               printf("%s\n",f->name);
            else
                printf("%d: %s\n",i++,f->name);
            if (auto_mode) break;
        }
        
        /* Only do this when not in list mode */
        if (!list_only)
        {
            if (!auto_mode)
            {
                /* Let user pick between them */
                printf("%d: Exit\n",i);
                printf("[0-%d]: ",i);
                scanf("%d",&input);
            } 
            else input = 0;
            
            /* If input > i, do nothing (^= exit) */
            if (input < i || auto_mode)
            {
                char* cmd;
                i = 0;
            
                /* Skip to currently selected log file */
                for (f = changed;i != input;f = f->next);
            
                /* Run command log_dir_path/logfile */
                cmd = malloc(strlen(command)+1+strlen(log_dir_path)+strlen(f->name)+1);
                sprintf(cmd,"%s %s%s",command,log_dir_path,f->name);
                system(cmd);    
                free(cmd);
            }
        }
    }
    else
    {
        if (!list_only) printf("No log files are currently active.\n");
    }

    /* Clean up */
    free(log_dir_path);
    free(command);
    closedir(log_dir);
    file_cleanup(snapshot);
    file_cleanup(changed);
    return 0;
}
