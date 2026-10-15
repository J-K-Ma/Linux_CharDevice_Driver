#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int fd;
    struct input_event ev;
    const char *dev_path = "/dev/input/event2";

    fd = open(dev_path, O_RDONLY);
    if (fd < 0)
    {
        perror("open /dev/input/event2 failed");
        return -1;
    }
    printf("Success open %s, waiting for events...\n", dev_path);

    while (1)
    {
        ssize_t ret = read(fd, &ev, sizeof(struct input_event));
        if (ret != sizeof(struct input_event))
        {
            perror("read event error");
            break;
        }

        printf("time: %ld.%06ld\ttype:%d, code:%d, value:%d\n",
               ev.time.tv_sec, ev.time.tv_usec,
               ev.type, ev.code, ev.value);
    }

    close(fd);
    return 0;
}
