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
#include <arpa/inet.h>
#include <assert.h>

#include "bs.h"

static char *lap2str(uint8_t *lap)
{
	static char crap[24];

	snprintf(crap, sizeof(crap), "%.2X:%.2X:%.2X",
		 lap[2], lap[1], lap[0]);

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
	printf("[EVENT CLOCK] %s remote %llx local %llx\n",
	       mac2str(be), be->be_rclock, be->be_lclock);
}

static void btev_packet(struct btevent *be)
{
	printf("[EVENT PACKET] %s remote %llx local %llx\n",
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

static void pwn(BS *bs, char *fname)
{
	RXINFO rx;
	void *data;
	size_t len;
	int fd;
	struct stat st;
	static btclock_t clock = 0;
	int did;
	uint8_t *p;

	fd = open(fname, O_RDONLY);
	if (fd == -1)
		err(1, "open()");

	if (fstat(fd, &st) == -1)
		err(1, "fstat()");

	len = st.st_size;

	p = data = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (data == MAP_FAILED)
		err(1, "mmap()");

	if (*p == 0x02) {
		uint32_t *clk = (uint32_t *) (p + 1);

		p = (uint8_t*) (clk + 1);

		clock = ntohl(*clk);
		clock *= 2;
	}

	assert(clock >= 0);

	memset(&rx, 0, sizeof(rx));
	rx.rx_clock = clock;
	rx.rx_chan  = 1;

	did = bs_process(bs, &rx, p, len);
	clock += did;

	close(fd);

	if (munmap(data, len) == -1)
		err(1, "munmap()");
}

static void usage(char *prog)
{
	printf("Usage: %s [opts] <files>\n"
	       "-h\thelp\n"
	       , prog);
	exit(0);
}

int main(int argc, char *argv[])
{
	int ch;
	BS* bs;

	bs = bs_new(btev, NULL);
	if (!bs)
		err(1, "bs_new()");

	while ((ch = getopt(argc, argv, "h")) != -1) {
		switch (ch) {
		case 'h':
		default:
			usage(argv[0]);
			break;
		}
	}

	for (; optind < argc; optind++)
		pwn(bs, argv[optind]);

	bs_delete(bs);

	exit(0);
}
