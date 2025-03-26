#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))

typedef struct msg {
  	pid_t pid;
  	int num1;
  	int num2;
} msg_t;

char filename[64];

void from_now(int millis, struct timespec *future) {
    struct timespec now;

    // Get current time
    clock_gettime(CLOCK_REALTIME, &now);

    // Add milliseconds
    future->tv_sec = now.tv_sec + millis / 1000;
    future->tv_nsec = now.tv_nsec + (millis % 1000) * 1000000;

    // Normalize the timespec
    if (future->tv_nsec >= 1000000000) {
        future->tv_sec += 1;
        future->tv_nsec -= 1000000000;
    }
}

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
  
  	mqd_t server_mq;
  	msg_t msg;
  	int ans;
  	struct timespec timeout;
  
  	while(true) {
        if((server_mq = mq_open(argv[1], O_WRONLY)) < 0) {
            ERR("mq_open");
        }
        if(scanf("%d %d", &msg.num1, &msg.num2) == EOF) {
            break;
        }
        msg.pid = pid;
        if(mq_send(server_mq, (char *) &msg, sizeof(msg_t), 0) < 0) {
          	ERR("mq_send");
        }
        if(mq_close(server_mq)) {
            ERR("mq_close");
        }

      	from_now(100, &timeout);
        if(mq_timedreceive(mqd, (char *) &ans, sizeof(int), NULL, &timeout) < 0) {
          	if(errno == ETIMEDOUT) {
              	break;
            } else {
              	ERR("mq_timedreceive");
            }
        }

        printf("answer: %d\n", ans);
    }
  	
  	if(mq_close(mqd)) {
    	ERR("mq_close");
    }
  
  	if(mq_unlink(filename)) {
      	ERR("mq_unlink");
    }
}