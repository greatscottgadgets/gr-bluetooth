#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_tun.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>


static struct option long_options[] = {
	{ "delete", no_argument, 0, 'd'},
	{ "help", no_argument, 0, 'h'},
	{ 0, 0, 0, 0}
};


void usage(char *prog) {

	fprintf(stderr, "note: you must be root (except perhaps to delete "
	   "the interface)\n");
	fprintf(stderr, "usage: %s [--help | -h] | [--delete | -d] "
	   "<interface_name>\n", basename(prog));
	exit(-1);
}


int main(int argc, char **argv) {

	struct ifreq ifr, ifw;
	char if_name[IFNAMSIZ], *chan_name = "gsm";
	int option_index = 0, c, delete_if = 0, persist = 1, fd, sd;

	while((c = getopt_long(argc, argv, "dh?", long_options, &option_index))
	   != -1) {
		switch(c) {
			case 'd':
				delete_if = 1;
				break;
			case 'h':
			case '?':
			default:
				usage(argv[0]);
				break;
		}
	}

	if(optind >= argc)
		usage(argv[0]);

	chan_name = argv[optind];

	if(!delete_if && getuid() && geteuid())
		usage(argv[0]);

	// construct TUN interface
	if((fd = open("/dev/net/tun", O_RDWR)) == -1) {
		perror("open");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	snprintf(ifr.ifr_name, IFNAMSIZ, "%s", chan_name);
	if(ioctl(fd, TUNSETIFF, (void *)&ifr) == -1) {
		perror("TUNSETIFF");
		close(fd);
		return -1;
	}

	// save actual name
	memcpy(if_name, ifr.ifr_name, IFNAMSIZ);

	if(delete_if)
		persist = 0;

	if(ioctl(fd, TUNSETPERSIST, (void *)persist) == -1) {
		perror("TUNSETPERSIST");
		close(fd);
		return -1;
	}

	if(delete_if) {
		close(fd);
		return 0;
	}

	// set interface up
	if((sd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		close(fd);
		return -1;
	}

	// get current flags
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, if_name, IFNAMSIZ - 1);
	if(ioctl(sd, SIOCGIFFLAGS, &ifr) == -1) {
		perror("SIOCGIFFLAGS");
		close(sd);
		close(fd);
		return -1;
	}

	// set up
	memset(&ifw, 0, sizeof(ifw));
	strncpy(ifw.ifr_name, if_name, IFNAMSIZ - 1);
	ifw.ifr_flags = ifr.ifr_flags | IFF_UP | IFF_RUNNING;
	if(ioctl(sd, SIOCSIFFLAGS, &ifw) == -1) {
		perror("SIOCSIFFLAGS");
		close(sd);
		close(fd);
		return -1;
	}

	close(sd);
	close(fd);

	return 0;
}
