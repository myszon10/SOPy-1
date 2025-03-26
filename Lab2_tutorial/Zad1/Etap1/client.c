#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))

char filename[64];

int main() {
  	pid_t pid = getpid();
  
	snprintf(filename, 64, "/%d", pid);
  
	mqd_t mqd;
	if((mqd = mq_open(filename, O_RDWR | O_CREAT, 0600, NULL)) < 0) {
    	ERR("mq_open");
    }
  
  	printf("PID: %s\n", filename);
  	
    sleep(1);
  	if(mq_close(mqd)) {
    	ERR("mq_close");
    }
  
  	if(mq_unlink(filename)) {
      	ERR("mq_unlink");
    }
}