#ifndef INCLUDED_TUN_H
#define INCLUDED_TUN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

// Taken from the gssm project (http://www.thre.at/gsm)

int mktun(const char *, unsigned char *);
int write_interface(int, unsigned char *, unsigned int, uint64_t, uint64_t, unsigned short);

#endif /* INCLUDED_TUN_H */
