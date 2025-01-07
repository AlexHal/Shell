#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


// LATEST VERSION
//                 seems to be working 
//                 TODO: read, understand, comment.
//                 maybe add piping and redirection to builtin
//                 
//

// Job datastructure, decided to use a simple liked list to have it be more dynamic
typedef struct Job {
    int id; // Job id, mainly used by fg
    pid_t pid;  // parent ID
    char command[20]; // Command
    struct Job *next; // pointer
    int done; // flag to say if job is done
} Jobs;

// Initializing linked list
Jobs *init_jobs_list() {
    Jobs *head = (Jobs *)malloc(sizeof(Jobs));
    if (head == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    head->id = 0;
    head->pid = 0;
    strcpy(head->command, "Parent\n");
    head->next = NULL;
    return head;
}
// linked list opperation
int add_job(Jobs *head, pid_t pid, const char *command) {
    Jobs *current = head;
    while (current->next != NULL) {
        current = current->next;
    }

    // Check if the job is already done
    // usefull for things like ls which compute very fast
    int status;
    if (waitpid(pid, &status, WNOHANG) == 0) {


    Jobs *new_job = (Jobs *)malloc(sizeof(Jobs));
    if (new_job == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    new_job->pid = pid;
    strncpy(new_job->command, command, sizeof(new_job->command) - 1);
    new_job->id = current->id + 1;
    new_job->done = 0; // Initialize as not done
    current->next = new_job;
    new_job->next = NULL;
    return new_job->id;
    }
}
// linked list opperation
int remove_job(Jobs **head, int id) {
    if (*head == NULL) {
        return 0; // Return 0 to indicate failure (empty list)
    }

    Jobs *current = *head;
    Jobs *prev = NULL;

    while (current != NULL) {
        if (current->id == id) {
            if (prev == NULL) {
                // If the first element matches, update the head
                *head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current); // Free memory of the removed job
            return 1; // Return 1 to indicate success (job removed)
        }
        prev = current;
        current = current->next;
    }

    return 0; // Return 0 to indicate failure (job not found)
}

int getcmd(char *prompt, char *args[], int *background) {
    int length, i = 0;
    char *token;
    char *line = NULL;
    size_t linecap = 0;
    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);
    if (length <= 0) {
        exit(-1);
    }

    // Check if background is specified.
    if (line[length - 2] == '&') {
        *background = 1;
        line[length - 2] = '\0'; // Remove the '&' character.
    } else {
        *background = 0;
    }

    token = strtok(line, " \t\n");
    while (token != NULL) {
        args[i++] = strdup(token);
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL; // Null-terminate the argument array.
    free(line);     // Free memory.

    return i;
}

// Check if the input is a built-in command
int is_builtin(char **args) {
    char *builtins[] = {"echo", "cd", "pwd", "exit", "fg", "jobs"};
    for (int i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        if (strcmp(args[0], builtins[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

void run_builtin(Jobs *head, char **args) {
    if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; args[i] != NULL; i++) {
            printf("%s ", args[i]);
        }
        printf("\n");
    }

    // cd
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "shell: cd: missing argument\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("chdir");
            }
        }
    }

    // pwd
    if (strcmp(args[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            perror("getcwd");
        } else {
            printf("%s\n", cwd);
        }
    }

    // exit
    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }

    if (strcmp(args[0], "jobs") == 0) {
        printf("List of jobs:\n");
        Jobs *current = head->next;
        while (current != NULL) {
            if (!current->done) { // Only print jobs that are not done
                printf("Job %d with pid %d and command %s\n", current->id, current->pid, current->command);
            }
            current = current->next;
        }
    }

    // fg
    if (strcmp(args[0], "fg") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, "shell: fg: missing argument (job ID)\n");
        } else {
            int job_id = atoi(args[1]);
            Jobs *current = head->next;
            while (current != NULL) {
                if (current->id == job_id) {
                    if (!current->done) { // Only bring to foreground if the job is not done
                        printf("Bringing Job %d with pid %d to foreground...\n", current->id, current->pid);
                        int status;
                        waitpid(current->pid, &status, 0);
                        current->done = 1; // Mark the job as done
                    } else {
                        printf("Job %d with pid %d is already done.\n", current->id, current->pid);
                    }
                    break;
                }
                current = current->next;
            }
            if (current == NULL) {
                fprintf(stderr, "shell: fg: job ID not found\n");
            }
        }
    }
}

// Function to periodically check and update the status of background jobs
void update_background_jobs(Jobs *head) {
    Jobs *current = head->next;
    while (current != NULL) {
        if (!current->done) {
            int status;
            pid_t result = waitpid(current->pid, &status, WNOHANG);
            if (result == -1) {
                perror("waitpid");
            } else if (result > 0) {
                // The job has completed
                current->done = 1;
                printf("\nJob %d with pid %d has completed.\n", current->id, current->pid);
                // can add line to remove code from the jobs list, althought it might mess up with the job_id's 
                // if so just change how job_id are alocated
            }
        }
        current = current->next;
    }
}

void executeCommand(Jobs *head, int background, char *args[]) {
    // Logic:
    // If it is to be ran in the FG it is loaded, started then we waitpid() till the jobs is completed. 
    // hence the user looses "control" of the shell for that time, afterwhice the job is completed and 
    // the user gets its control back.
    // If it is to be ran in the BG, similarly like before it is loaded & executed. However, we do not use waitpid
    // we add it to the jobs linked list and give control of the shell imediatly back to the user.
    // the os can then complete the tast in the BG and the user also has the possibility to call it to the FG using fg [Job ID]
    pid_t pid = fork();
    if (pid == 0) {
        // Child process

        int in_fd = -1, out_fd = -1;
        int pipe_fd[2];
        int is_piped = 0;

        for (int i = 0; args[i] != NULL; i++) {
            // checking for input redirection
            // input redirection did not get thorougly test, just added some of the code for fun
            if (strcmp(args[i], "<") == 0) {
                in_fd = open(args[i + 1], O_RDONLY);
                if (in_fd < 0) {
                    perror("open");
                    exit(1);
                }
                dup2(in_fd, 0); // Redirect stdin
                close(in_fd);
                args[i] = NULL; // Remove '<' from arguments
            } else if (strcmp(args[i], ">") == 0) {
                // similartly for output redirection
                out_fd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (out_fd < 0) {
                    perror("open");
                    exit(1);
                }
                dup2(out_fd, 1); // Redirect stdout
                close(out_fd);
                args[i] = NULL; // Remove '>' from arguments
            } else if (strcmp(args[i], "|") == 0) {
                // checking for piping
                is_piped = 1;
                args[i] = NULL; // Split the command at the pipe symbol
                if (pipe(pipe_fd) == -1) {
                    perror("Pipe error");
                    exit(1);
                }

                pid_t pipe_pid = fork();
                if (pipe_pid == 0) {
                    // Child process for the first command
                    close(pipe_fd[0]); // Close read end of the pipe
                    dup2(pipe_fd[1], 1); // Redirect stdout to the pipe
                    close(pipe_fd[1]); // Close write end of the pipe

                    execvp(args[0], args);
                    perror("execvp");
                    exit(1);
                } else if (pipe_pid < 0) {
                    perror("fork");
                    exit(1);
                }

                // In the parent process, execute the second command after the pipe
                close(pipe_fd[1]); // Close write end of the pipe
                dup2(pipe_fd[0], 0); // Redirect stdin from the pipe
                close(pipe_fd[0]); // Close read end of the pipe

                execvp(args[i + 1], &args[i + 1]);
                perror("execvp");
                exit(1);
            }
        }

        // If no pipe or redirection, execute normally
        if (!is_piped && execvp(args[0], args) == -1) {
            perror("execvp");
            exit(1);
        }
    } else if (pid < 0) {
        perror("fork");
    } else {
        // Parent process
        if (!background) {
            int status;
            waitpid(pid, &status, 0);
            // Remove the job from the list as it has completed
            remove_job(&head, pid);
        } else {
            // Add the job to the list if it's a background job
            int job_id = add_job(head, pid, args[0]);
            printf("Added[%d] %d\n", job_id, pid);
        }
    }
}

int main(void) {
    char *args[20];
    int bg;

    // Initialize linked list
    Jobs *head = init_jobs_list();
    printf("%s",  "Welcome to my simple shell env.");
    while (1) {
        bg = 0;
        
        //Get usr input
        int cnt = getcmd("\n>> ", args, &bg);
        // Check if built-in // assuming no piping or redirection
        if (cnt > 0 && is_builtin(args)) {
            // Ignore bg
            run_builtin(head, args);
        } else if (cnt > 0) {
            executeCommand(head, bg, args);
        }

        // Periodically check and update the status of background jobs
        update_background_jobs(head);
        
    }

    return 0;
}

