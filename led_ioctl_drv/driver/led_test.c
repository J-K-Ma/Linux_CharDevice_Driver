/*
 * test_led.c - LED 驱动测试程序
 *
 * 用法:
 *   ./test_led <设备节点> <命令>
 * 命令:
 *   on     - 点亮 LED (发送 LED_ON)
 *   off    - 熄灭 LED (发送 LED_OFF)
 *   blink  - 启动闪烁 (发送 LED_BLINK)
 *
 * 示例:
 *   ./test_led /dev/led-1 on
 *   ./test_led /dev/led-1 off
 *   ./test_led /dev/led-1 blink
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

/* 与驱动中定义一致 */
#define LED_ON    _IOW('l', 1, int)
#define LED_OFF   _IOW('l', 0, int)
#define LED_BLINK _IOW('l', 3, int)

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s <device> <on|off|blink>\n", prog);
    fprintf(stderr, "Example: %s /dev/led-1 blink\n", prog);
}

int main(int argc, char *argv[])
{
    int fd;
    int ret;
    unsigned int cmd;

    if (argc != 3) {
        usage(argv[0]);
        return -1;
    }

    const char *dev_path = argv[1];
    const char *op = argv[2];

    if (strcmp(op, "on") == 0) {
        cmd = LED_ON;
    } else if (strcmp(op, "off") == 0) {
        cmd = LED_OFF;
    } else if (strcmp(op, "blink") == 0) {
        cmd = LED_BLINK;
    } else {
        fprintf(stderr, "Invalid command: %s\n", op);
        usage(argv[0]);
        return -1;
    }

    fd = open(dev_path, O_RDWR);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    ret = ioctl(fd, cmd, 0);
    if (ret < 0) {
        perror("ioctl");
        close(fd);
        return -1;
    }

    printf("Command '%s' sent to %s successfully.\n", op, dev_path);
    close(fd);
    return 0;
}