#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include <string.h>

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

enum operation {
  	OP_SUM,
  	OP_DIV,
  	OP_MOD,
};

typedef struct thread_params {
	mqd_t mqd;
  	enum operation op;
} thread_params;

void *server_work(void *_params) {
  	thread_params *params = _params;
    char client_filename[64];
  	int ans;
  	msg_t msg;
  	mqd_t client_mq;
  	unsigned int priority;
  
  	while(true) {
        if(mq_receive(params->mqd, (char *) &msg, sizeof(msg_t), &priority) < 0) {
			ERR("mq_receive");
		}
      	if(priority == 1) {
          	break;
        }

        printf("got request pid=%d, num1=%d, num2=%d\n", msg.pid, msg.num1, msg.num2);
        switch(params->op) {
        case OP_SUM:
            ans = msg.num1 + msg.num2;
            break;
        case OP_DIV:
            ans = msg.num1 / msg.num2;
            break;
        case OP_MOD:
            ans = msg.num1 % msg.num2;
            break;
        }

        snprintf(client_filename, 64, "/%d", msg.pid);
        client_mq = mq_open(client_filename, O_WRONLY);
		if(client_mq < 0) {
			ERR("mq_open");
		}
        if(mq_send(client_mq, (char *) &ans, sizeof(int), 0)) {
			ERR("mq_send");
		}
        if(mq_close(client_mq)) {
            ERR("mq_close");
        }
    }
  
  	return NULL;
}

int main() {
  	pid_t pid = getpid();
  
  	sigset_t sigset;
  	sigemptyset(&sigset);
  	sigaddset(&sigset, SIGINT);
  	if(pthread_sigmask(SIG_BLOCK, &sigset, NULL)) {
      	ERR("pthread_sigmask");
    }
  
  	struct mq_attr attr;
  	attr.mq_maxmsg = 10;
  	attr.mq_msgsize = sizeof(msg_t);
  
	snprintf(filename_s, 64, "/%d_s", pid);
	snprintf(filename_d, 64, "/%d_d", pid);
	snprintf(filename_m, 64, "/%d_m", pid);
  
	mqd_t mqd_s;
	mqd_t mqd_d;
	mqd_t mqd_m;
	if((mqd_s = mq_open(filename_s, O_RDWR | O_CREAT, 0600, &attr)) < 0) {
    	ERR("mq_open");
    }
	if((mqd_d = mq_open(filename_d, O_RDWR | O_CREAT, 0600, &attr)) < 0) {
    	ERR("mq_open");
    }
	if((mqd_m = mq_open(filename_m, O_RDWR | O_CREAT, 0600, &attr)) < 0) {
    	ERR("mq_open");
    }
  	printf("PID_s: %s\n", filename_s);
  	printf("PID_d: %s\n", filename_d);
  	printf("PID_m: %s\n", filename_m);
  	
  	pthread_t thread_s;
  	thread_params params_s;
  	params_s.mqd = mqd_s;
  	params_s.op = OP_SUM;
  	if(pthread_create(&thread_s, NULL, server_work, &params_s)) {
      	ERR("pthread_create");
    }
  	pthread_t thread_d;
  	thread_params params_d;
  	params_d.mqd = mqd_d;
  	params_d.op = OP_DIV;
  	if(pthread_create(&thread_d, NULL, server_work, &params_d)) {
      	ERR("pthread_create");
    }
  	pthread_t thread_m;
  	thread_params params_m;
  	params_m.mqd = mqd_m;
  	params_m.op = OP_MOD;
  	if(pthread_create(&thread_m, NULL, server_work, &params_m)) {
      	ERR("pthread_create");
    }
  
  	int sig;
    sigwait(&sigset, &sig);
  
  	msg_t msg;
  	if(mq_send(mqd_s, (char *) &msg, sizeof(msg_t), 1)) {
      	ERR("mq_send");
    }
  	if(mq_send(mqd_d, (char *) &msg, sizeof(msg_t), 1)) {
      	ERR("mq_send");
    }
  	if(mq_send(mqd_m, (char *) &msg, sizeof(msg_t), 1)) {
      	ERR("mq_send");
    }
  
  	if(pthread_join(thread_s, NULL)) {
      	ERR("pthread_join");
    }
  	if(pthread_join(thread_d, NULL)) {
      	ERR("pthread_join");
    }
  	if(pthread_join(thread_m, NULL)) {
      	ERR("pthread_join");
    }
  
  	printf("\ngraceful exit\n");
  
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