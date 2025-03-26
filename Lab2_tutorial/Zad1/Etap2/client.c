#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))

typedef struct msg {
  	pid_t pid;
  	int num1;
  	int num2;
} msg_t;

char filename[64];

int main(int argc, char **argv) {
  	if(argc != 2) {
      	return EXIT_FAILURE;
    }
  
  	pid_t pid = getpid();
  
	snprintf(filename, 64, "/%d", pid);
  
  	struct mq_attr attr;
  	attr.mq_maxmsg = 10;
  	attr.mq_msgsize = sizeof(int);
  
	mqd_t mqd;
	if((mqd = mq_open(filename, O_RDONLY | O_CREAT, 0600, &attr)) < 0) {
    	ERR("mq_open");
    }
  
  	printf("queue: %s\n", filename);
  
  	mqd_t server_mq = mq_open(argv[1], O_WRONLY);
  	msg_t msg;
  	scanf("%d %d", &msg.num1, &msg.num2);
  	msg.pid = pid;
  	mq_send(server_mq, (char *) &msg, sizeof(msg_t), 0);
  
  	int ans;
  	mq_receive(mqd, (char *) &ans, sizeof(int), NULL);
  
  	printf("received: %d\n", ans);
  	
  	if(mq_close(mqd)) {
    	ERR("mq_close");
    }
  
  	if(mq_unlink(filename)) {
      	ERR("mq_unlink");
    }
}