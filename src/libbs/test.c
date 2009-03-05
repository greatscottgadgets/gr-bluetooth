#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>

#include "bs.h"

static struct conf {
	char	*c_in;
} _conf;

static char *lap2str(uint8_t *lap)
{
	static char crap[24];

	snprintf(crap, sizeof(crap), "%.2X:%.2X:%.2X",
		 lap[0], lap[1], lap[2]);

	return crap;
}

static char *mac2str(struct btevent *be)
{
	static char crap[24];

	snprintf(crap, sizeof(crap), "%.2X:%s",
		 be->be_uap, lap2str(be->be_lap));

	return crap;
}

static void btev_lap(struct btevent *be)
{
	printf("[EVENT LAP] %s\n", lap2str(be->be_lap));
}

static void btev_uap(struct btevent *be)
{
	printf("[EVENT UAP] %s\n", mac2str(be));
}

static void btev_clock(struct btevent *be)
{
	printf("[EVENT CLOCK] %s remote %x local %x\n",
	       mac2str(be), be->be_rclock, be->be_lclock);
}

static void btev_packet(struct btevent *be)
{
	printf("[EVENT PACKET] %s remote %x local %x\n",
	       mac2str(be), be->be_rclock, be->be_lclock);
}

static int btev(struct btevent *be)
{

	switch (be->be_type) {
	case EVENT_LAP:
		btev_lap(be);
		break;

	case EVENT_UAP:
		btev_uap(be);
		break;

	case EVENT_CLOCK:
		btev_clock(be);
		break;

	case EVENT_PACKET:
		btev_packet(be);
		break;

	default:
		printf("Unknown event %d\n", be->be_type);
		break;
	}

	return 0;
}

static void pwn(void)
{
	RXINFO rx;
	void *data;
	size_t len;
	int fd;
	struct stat st;

	BS* bs = bs_new(btev, NULL);
	if (!bs)
		err(1, "bs_new()");

	memset(&rx, 0, sizeof(rx));

	fd = open(_conf.c_in, O_RDONLY);
	if (fd == -1)
		err(1, "open()");

	if (fstat(fd, &st) == -1)
		err(1, "fstat()");

	len = st.st_size;

	data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED)
		err(1, "mmap()");

	bs_process(bs, &rx, data, len);

	close(fd);

	if (munmap(data, len) == -1)
		err(1, "munmap()");

	bs_delete(bs);
}

static void usage(char *prog)
{
	printf("Usage: %s <opts>\n"
	       "-h\thelp\n"
	       "-i\t<input file>\n"
	       , prog);
	exit(0);
}

int main(int argc, char *argv[])
{
	int ch;

	while ((ch = getopt(argc, argv, "hi:")) != -1) {
		switch (ch) {
		case 'i':
			_conf.c_in = optarg;
			break;

		case 'h':
		default:
			usage(argv[0]);
			break;
		}
	}

	pwn();
	exit(0);
}
