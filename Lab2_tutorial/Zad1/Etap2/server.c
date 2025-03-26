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

// nazwy kolejek serwera
char filename_s[64]; 
char filename_d[64];
char filename_m[64];
char client_filename[64];

int main() {
  	pid_t pid = getpid();
  
  	struct mq_attr attr;
  	attr.mq_maxmsg = 10;
  	attr.mq_msgsize = sizeof(msg_t);
  
	snprintf(filename_s, 64, "/%d_s", pid);
	snprintf(filename_d, 64, "/%d_d", pid);
	snprintf(filename_m, 64, "/%d_m", pid);
  
	mqd_t mqd_s;
	mqd_t mqd_d;
	mqd_t mqd_m;
	if((mqd_s = mq_open(filename_s, O_RDONLY | O_CREAT, 0600, &attr)) < 0) {
    	ERR("mq_open");
    }
	if((mqd_d = mq_open(filename_d, O_RDONLY | O_CREAT, 0600, &attr)) < 0) {
    	ERR("mq_open");
    }
	if((mqd_m = mq_open(filename_m, O_RDONLY | O_CREAT, 0600, &attr)) < 0) {
    	ERR("mq_open");
    }
  	printf("PID_s: %s\n", filename_s);
  	printf("PID_d: %s\n", filename_d);
  	printf("PID_m: %s\n", filename_m);
  	
  	msg_t msg;
  	mq_receive(mqd_s, (char *) &msg, sizeof(msg_t), NULL);
  	printf("got request pid=%d, num1=%d, num2=%d\n", msg.pid, msg.num1, msg.num2);
  	int ans = msg.num1 + msg.num2;
  
  	mqd_t client_mq;
	snprintf(client_filename, 64, "/%d", msg.pid);
  	client_mq = mq_open(client_filename, O_WRONLY);
  	mq_send(client_mq, (char *) &ans, sizeof(int), 0);
  	if(mq_close(client_mq)) {
    	ERR("mq_close");
    }
  
  	if(mq_close(mqd_s)) {
    	ERR("mq_close");
    }
  	if(mq_close(mqd_d)) {
      	ERR("mq_close");
    }
  	if(mq_close(mqd_m)) {
    	ERR("mq_close");
    }
  
  	if(mq_unlink(filename_s)) {
      	ERR("mq_unlink");
    }
  	if(mq_unlink(filename_d)) {
      	ERR("mq_unlink");
    }
  	if(mq_unlink(filename_m)) {
      	ERR("mq_unlink");
    }
}