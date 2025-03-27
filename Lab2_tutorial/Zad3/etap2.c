#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/wait.h>

#define ERR(source) \
    (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), perror(source), exit(EXIT_FAILURE))

#define BUFFER_SIZE 256

void usage(char *name)
{
    fprintf(stderr, "USAGE: %s P T1 T2\n", name);
    exit(EXIT_FAILURE);
}

void child_work(pid_t prev_pid, char *buf)
{
    char queue_name[BUFFER_SIZE];
    mqd_t rmqd, wmqd;
    struct mq_attr attr;
    attr.mq_maxmsg = 2;
    attr.mq_msgsize = BUFFER_SIZE;

    // For the first child, prev_pid equals parent's pid.
    if (prev_pid == getppid()) {
        // First child: create its own queue for reading.
        snprintf(queue_name, sizeof(queue_name), "/sop_cwg_%d", getpid());
        if ((rmqd = mq_open(queue_name, O_CREAT | O_RDONLY, 0600, &attr)) < 0)
            ERR("mq_open (first child)");
    } else {
        // Subsequent children: open the previous child's queue.
        snprintf(queue_name, sizeof(queue_name), "/sop_cwg_%d", prev_pid);
        if ((rmqd = mq_open(queue_name, O_RDONLY)) < 0)
            ERR("mq_open");
    }

    // Create own message queue for writing (for the next child in the chain)
    snprintf(queue_name, sizeof(queue_name), "/sop_cwg_%d", getpid());
    if ((wmqd = mq_open(queue_name, O_CREAT | O_WRONLY, 0600, &attr)) < 0)
        ERR("mq_open");

    char wordbuf[BUFFER_SIZE];
    ssize_t wordlen;
    unsigned int priority;
    while (1)
    {
        wordlen = mq_receive(rmqd, wordbuf, BUFFER_SIZE, &priority);
        if (wordlen < 0)
            ERR("mq_receive");
        // Add a null terminator to ensure proper string printing.
        if (wordlen < BUFFER_SIZE)
            wordbuf[wordlen] = '\0';
        else
            wordbuf[BUFFER_SIZE - 1] = '\0';

        printf("pid %d got message: %s\n", getpid(), wordbuf);
        if (priority == 1)
            break;
    }
}

int main(int argc, char **argv)
{
    if (argc != 4)
        usage(argv[0]);

    int P = atoi(argv[1]);
    int T1 = atoi(argv[2]);
    int T2 = atoi(argv[3]);

    char buf[BUFFER_SIZE];
    int child_count = 0;

    pid_t res;
    pid_t prev_pid = getpid();
    pid_t first_pid = -1;

    while (1) {
        if (!fgets(buf, sizeof(buf), stdin))
            break; // Exit gracefully on EOF

        buf[strlen(buf) - 1] = '\0';

        // If the input line does NOT start with "start ", fork a child.
        if (strncmp(buf, "start ", 6) != 0) {
            switch ((res = fork())) {
            case -1:
                ERR("fork");
            case 0:
                child_work(prev_pid, buf);
                exit(EXIT_SUCCESS);
            default:
                if (first_pid == -1)
                    first_pid = res;
                prev_pid = res;
                child_count++;
                break;
            }
        }
        else
        {
            // "start" command: send words to the first child's queue.
            char queue_name[BUFFER_SIZE];
            mqd_t queue;

            snprintf(queue_name, sizeof(queue_name), "/sop_cwg_%d", first_pid);
            if ((queue = mq_open(queue_name, O_WRONLY)) < 0)
                ERR("mq_open");

            char *start = buf + 6;
            size_t len = 0;
            while (start[len] != '\0') {
                if (start[len] != ' ') {
                    len++;
                } else {
                    if (mq_send(queue, start, len, 0))
                        ERR("mq_send");
                    start += len + 1;
                    len = 0;
                }
            }
            if (mq_send(queue, start, len, 1))
                ERR("mq_send");

            // After sending the termination message (priority 1) and if we've created all children,
            // break out of the input loop.
            if (child_count == P)
                break;
        }
    }

    // Wait for all children to finish.
    while (wait(NULL) > 0)
        ;

    return EXIT_SUCCESS;
}
