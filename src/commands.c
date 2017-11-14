#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include "commands.h"
#include "built_in.h"
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <pthread.h>

#define CLIENT_PATH "tpf_unix_sock.clinet"
#define SERVER_PATH "tpf_unix_sock.server"
#define UNI_PATH_MAX 108
#define SOCK_PATH "tpf_unix_sock.server"
#define DATA "Hello from server"
#define DATA2 "Hello from clinet"



static struct built_in_command built_in_commands[] = {
  { "cd", do_cd, validate_cd_argv },
  { "pwd", do_pwd, validate_pwd_argv },
  { "fg", do_fg, validate_fg_argv }
};

static int is_built_in_command(const char* command_name)
{
  static const int n_built_in_commands = sizeof(built_in_commands) / sizeof(built_in_commands[0]);

  for (int i = 0; i < n_built_in_commands; ++i) {
    if (strcmp(command_name, built_in_commands[i].command_name) == 0) {
      return i;
    }
  }

  return -1; // Not found
}

void* server_section(void* com1){
  struct single_command *com = (struct single_command *)com1;
  //server setup
    int server_sock, client_sock,len,rc;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    int backlog = 10;
    int bytes_rec= 0;
    char buf[256];
    memset(&server_sockaddr, 0 , sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(buf,0,256);

    server_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_sock == -1){
        printf("SOCKET ERROR\n");
        exit(1);
    }

    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SOCK_PATH);
    len = sizeof(server_sockaddr);

    unlink(SOCK_PATH);

    rc = bind(server_sock,(struct sockaddr *)&server_sockaddr, len);
    if(rc == -1){
      printf("BIND ERROR\n");
      close(server_sock);
      exit(1);
    }
    
    rc = listen(server_sock, backlog);
    if(rc == -1){
      printf("LISTEN ERROR\n");
      close(server_sock);
      exit(1);
    }

    printf("socket listening...\n");
 
    client_sock = accept(server_sock, (struct sockaddr*)&client_sockaddr,&len);
    
    if(client_sock == -1){
      printf("ACCPT ERROR\n");
      close(server_sock);
      close(client_sock);
      exit(1);
    }

    len = sizeof(client_sockaddr);
    rc = getpeername(client_sock, (struct sockaddr*)&client_sockaddr,&len);
    if(rc == -1){
      printf("GETPEERNAME ERROR\n");
      close(server_sock);
      close(client_sock);
      exit(1);

    }
    else {
      printf("Client socket filepath : %s\n", client_sockaddr.sun_path);

    }

    dup2(server_sock,0);

    printf("waiting to read...\n");
    bytes_rec = recv(client_sock, buf, sizeof(buf), 0);
    if(bytes_rec == -1){
      printf("RECV ERROR\n");
      close(server_sock);
      close(client_sock);
      exit(1);
    }
    else{
      printf("DATA RECEIVED = %s\n", buf);

    }

    memset(buf, 0, 256);
  
    close(server_sock);
    close(client_sock);

    return 0;
  }

void* client_section(void* com2){
    struct single_command *com = (struct single_command *)com2;
    int client_sock, rc, len;
    struct sockaddr_un server_sockaddr;
    struct sockaddr_un client_sockaddr;
    char buf[256];
    memset(&server_sockaddr, 0, sizeof(struct sockaddr_un));
    memset(&client_sockaddr, 0, sizeof(struct sockaddr_un));

    client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&server_sockaddr, 0, sizeof(server_sockaddr));
    if(client_sock == -1){
      printf("SOCKET ERROR\n");
      exit(1);
    }

    client_sockaddr.sun_family = AF_UNIX;
    strcpy(client_sockaddr.sun_path, CLIENT_PATH);
    len = sizeof(client_sockaddr);

    unlink(CLIENT_PATH);
    rc = bind(client_sock, (struct sockaddr*)&client_sockaddr,len);
    if(rc ==-1){
      printf("BIND ERROR\n");
      close(client_sock);
      exit(1);
    }

    server_sockaddr.sun_family = AF_UNIX;
    strcpy(server_sockaddr.sun_path, SERVER_PATH);

    rc= connect(client_sock, (struct sockaddr *)&server_sockaddr,len);

    if(rc == -1){
      printf("CONNECT ERROR\n");
      close(client_sock);
      exit(1);
    }

    dup2(client_sock, 1);

    execv(com->argv[0], com->argv);
    close(client_sock);
    return 0;
  }

/*
 * Description: Currently this function only handles single built_in commands. You should modify this structure to launch process and offer pipeline functionality.
 */
int evaluate_command(int n_commands, struct single_command (*commands)[512])
{
  if (n_commands == 1) {
  	
    struct single_command* com = (*commands);

    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);


    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }

     }else if (strcmp(com->argv[0], "") == 0) {
      return 0;

     }else if (strcmp(com->argv[0], "exit") == 0) {
      	return 1;
      }else {

	   	pid_t pid;
 		  int status;
      int i;
      char* newpath[5];
      for(i=0;i<5;i++){
        newpath[i] = (char*)malloc(sizeof(char)*100);
      }
      char* Path[5] = {"/usr/local/bin", "/usr/bin", "/bin", "/usr/sbin", "/sbin" };
	     pid = fork(); 

      for(i =0; i<5;i++){
        strcpy(newpath[i], Path[i]);
        strcat(newpath[i], "/");
        strcat(newpath[i], com->argv[0]);
       }

	     	if(pid == -1){
		      printf("fork fail\n");
		      return -1;
        	}
		    else if(pid == 0){
          for(i = 0; i<5; i++){
          execv(newpath[i], com->argv);
          }
          fprintf(stderr, "%s: command not found\n", com->argv[0]);
          exit(0);
        
         }
          
		    else {
	   	   waitpid(-1, &status, 0);
	       }
      } 
  }
  else if(n_commands >= 2){

      struct sockaddr_un{
      unsigned short int sun_family;
      char sun_path[UNI_PATH_MAX];

      };
    struct single_command* com = (*commands);


    assert(com->argc != 0);

    int built_in_pos = is_built_in_command(com->argv[0]);

    if (built_in_pos != -1) {
      if (built_in_commands[built_in_pos].command_validate(com->argc, com->argv)) {
        if (built_in_commands[built_in_pos].command_do(com->argc, com->argv) != 0) {
          fprintf(stderr, "%s: Error occurs\n", com->argv[0]);
        }
      } else {
        fprintf(stderr, "%s: Invalid arguments\n", com->argv[0]);
        return -1;
      }

     }else if (strcmp(com->argv[0], "") == 0) {
      return 0;

     }else if (strcmp(com->argv[0], "exit") == 0) {
        return 1;
      }else {
      pthread_t tid;
      pthread_t thread_id;
      pid_t pid;
      int status;
      int i;
      char* newpath[5];
      

      for(i=0;i<5;i++){
        newpath[i] = (char*)malloc(sizeof(char)*100);
      }
      char* Path[5] = {"/usr/local/bin", "/usr/bin", "/bin", "/usr/sbin", "/sbin" };
       pid = fork(); 

      for(i =0; i<5;i++){
        strcpy(newpath[i], Path[i]);
        strcat(newpath[i], "/");
        strcat(newpath[i], com->argv[0]);
       }

        if(pid == -1){
          printf("fork fail\n");
          return -1;
          }
        else if(pid == 0){
          pid = fork();
          if(pid == -1){
            printf("fork fail\n");
            return -1;
          }
          else if(pid ==0){
               client_section(com);
               printf("client finish");
            }

          else {
           waitpid(-1, &status, WNOHANG); //
          
            pthread_create(&tid,NULL,server_section,com->argv[1]);
            pthread_join(tid, NULL);
            printf("server finish");
            }
          }
        else {
           waitpid(-1, &status, 0);
         }
      
    }
}
  
  return 0;
}

void free_commands(int n_commands, struct single_command (*commands)[512])
{
  for (int i = 0; i < n_commands; ++i) {
    struct single_command *com = (*commands) + i;
    int argc = com->argc;
    char** argv = com->argv;

    for (int j = 0; j < argc; ++j) {
      free(argv[j]);
    }

    free(argv);
  }

  memset((*commands), 0, sizeof(struct single_command) * n_commands);
}
