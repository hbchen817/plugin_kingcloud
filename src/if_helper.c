#include "if_helper.h"
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <sys/ioctl.h>

#define MAXINTERFACES 16

long long if_gethwaddr(const char *ifname) {
    register int fd, interface;
    struct ifreq buf[MAXINTERFACES];
    struct ifconf ifc;
    long long result = 0;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
            interface = ifc.ifc_len / sizeof(struct ifreq);
            while (interface-- > 0) {
                if (strcmp(buf[interface].ifr_name, ifname) != 0) {
                    continue;
                }
                if (ioctl(fd, SIOCGIFFLAGS, (char *)&buf[interface])) {
                    continue;
                }
                if (ioctl(fd, SIOCGIFHWADDR, (char *)&buf[interface])) {
                    continue;
                }
                memcpy(&result, buf[interface].ifr_hwaddr.sa_data, 6);
                break;
            }
        }
    }
    close(fd);
    return result;
}

const char * if_getaddr(const char *ifname) {
    register int fd, interface;
    struct ifreq buf[MAXINTERFACES];
    struct ifconf ifc;
    const char * result = NULL;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;
        if (!ioctl(fd, SIOCGIFCONF, (char *)&ifc)) {
            interface = ifc.ifc_len / sizeof(struct ifreq);
            while (interface-- > 0) {
                if (strcmp(buf[interface].ifr_name, ifname) != 0) {
                    continue;
                }
                if (ioctl(fd, SIOCGIFFLAGS, (char *)&buf[interface])) {
                    continue;
                }
                if (ioctl(fd, SIOCGIFADDR, (char *)&buf[interface])) {
                    continue;
                }
                result = inet_ntoa(((struct sockaddr_in *)(&buf[interface].ifr_addr))->sin_addr);
                break;
            }
        }
    }
    close(fd);
    return result;
}