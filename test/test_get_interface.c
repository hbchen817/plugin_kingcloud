#include "if_helper.h"
#include <net/if.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>

#define MAXINTERFACES 16

int get_interface() {
    register int fd, interface, retn = 0;
    struct ifreq buf[MAXINTERFACES];
    struct ifconf ifc;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
            interface = ifc.ifc_len / sizeof(struct ifreq);
            printf("interface num is interface=%d\n", interface);
            while (interface-- > 0) {
                printf("  net device %s\n", buf[interface].ifr_name);
                if (!(ioctl(fd, SIOCGIFFLAGS, (char *)&buf[interface]))) {
                    if (buf[interface].ifr_flags & IFF_PROMISC) {
                        puts("    the interface is PROMISC");
                        retn++;
                    }
                } else {
                    char str[256];
                    sprintf(str, "cpm: ioctl device %s", buf[interface].ifr_name);
                    perror(str);
                }
                if (buf[interface].ifr_flags & IFF_UP) {
                    puts("    the interface status is UP");
                } else {
                    puts("    the interface status is DOWN");
                }
                if (!(ioctl(fd, SIOCGIFADDR, (char *)&buf[interface]))) {
                    printf("    IP address is: %s\n", inet_ntoa(((struct sockaddr_in *)(&buf[interface].ifr_addr))->sin_addr));
                } else {
                    char str[256];
                    sprintf(str, "cpm: ioctl device %s", buf[interface].ifr_name);
                    perror(str);
                }
                if (!(ioctl(fd, SIOCGIFHWADDR, (char *)&buf[interface]))) {
                    printf("    HW address is: %02x:%02x:%02x:%02x:%02x:%02x\n",
                        (unsigned char)buf[interface].ifr_hwaddr.sa_data[0],
                        (unsigned char)buf[interface].ifr_hwaddr.sa_data[1],
                        (unsigned char)buf[interface].ifr_hwaddr.sa_data[2],
                        (unsigned char)buf[interface].ifr_hwaddr.sa_data[3],
                        (unsigned char)buf[interface].ifr_hwaddr.sa_data[4],
                        (unsigned char)buf[interface].ifr_hwaddr.sa_data[5]);
                } else {
                    char str[256];
                    sprintf(str, "cpm: ioctl device %s", buf[interface].ifr_name);
                    perror(str);
                }
            }
        } else {
            perror("cpm: ioctl");
        }
    } else {
        perror("cpm: socket");
    }
    close(fd);
    return retn;
}

int main(int argc, char ** argv) {
    if (argc == 1) {
        printf("Usage: %s <ifname>\n", argv[0]);
        return 1;
    }
    printf("%s\n", if_getaddr(argv[1]));
    return 0;
}