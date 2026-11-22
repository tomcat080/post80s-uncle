/*
 * btscan: 用内核 kctl com.apple.uart.bluetooth 做一次真实 BLE 被动扫描
 * 复用 hcitest 的 kctl/termios/H4 逻辑，只在上面加 LE 扫描命令与广播解析
 *
 *   btscan [-l SECONDS] [-a]     -a = 主动扫描（会发 SCAN_REQ，能拿到 Scan Response 里的名字）
 */
#include <errno.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#define SYSPROTO_CONTROL 2
#define AF_SYS_CONTROL   2
#define CTLIOCGINFO      0xC0644E03UL

struct ctl_info { uint32_t ctl_id; char ctl_name[96]; };
struct sockaddr_ctl {
	uint8_t sc_len; uint8_t sc_family; uint16_t ss_sysaddr;
	uint32_t sc_id; uint32_t sc_unit; uint32_t sc_reserved[5];
};

static uint8_t rxb[8192];
static unsigned rxn;

static double now(void) {
	struct timeval tv; gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec / 1e6;
}

static unsigned pkt_len(void) {
	if (rxn < 1) return 0;
	switch (rxb[0]) {
	case 4: return rxn >= 3 ? 3u + rxb[2] : 3;
	case 2: return rxn >= 5 ? 5u + (rxb[3] | (rxb[4] << 8)) : 5;
	case 3: return rxn >= 4 ? 4u + rxb[3] : 4;
	case 5: return rxn >= 5 ? 5u + ((rxb[3] | (rxb[4] << 8)) & 0x3fff) : 5;
	default: return 0;
	}
}

static int get_packet(int fd, double deadline, unsigned *len) {
	for (;;) {
		unsigned n = pkt_len();
		if (rxn && n == 0) { memmove(rxb, rxb + 1, --rxn); continue; }
		if (n && rxn >= n) { *len = n; return 1; }
		double left = deadline - now();
		if (left <= 0) return 0;
		struct pollfd p = { fd, POLLIN, 0 };
		if (poll(&p, 1, (int)(left * 1000) + 1) <= 0) return 0;
		ssize_t r = read(fd, rxb + rxn, sizeof(rxb) - rxn);
		if (r <= 0) { printf("read: %s\n", r == 0 ? "EOF" : strerror(errno)); return 0; }
		rxn += (unsigned)r;
	}
}

static void drop(unsigned n) { memmove(rxb, rxb + n, rxn - n); rxn -= n; }

static int command(int fd, unsigned opcode, const uint8_t *par, unsigned plen, uint8_t *ret, unsigned *retlen) {
	uint8_t pkt[4 + 255];
	pkt[0] = 1; pkt[1] = opcode & 0xff; pkt[2] = opcode >> 8; pkt[3] = plen;
	if (plen) memcpy(pkt + 4, par, plen);
	if (write(fd, pkt, 4 + plen) != (ssize_t)(4 + plen)) { printf("write: %s\n", strerror(errno)); return -1; }
	double deadline = now() + 5;
	unsigned n;
	while (get_packet(fd, deadline, &n)) {
		uint8_t *e = rxb;
		if (e[0] == 4 && e[1] == 0x0e && n >= 7 && (e[4] | (e[5] << 8)) == (int)opcode) {
			int st = e[6];
			if (ret) { memcpy(ret, e + 6, n - 6); *retlen = n - 6; }
			printf("cmd %#06x -> status %#x\n", opcode, st);
			drop(n);
			return st;
		}
		if (e[0] == 4 && e[1] == 0x0f && n >= 7 && (e[5] | (e[6] << 8)) == (int)opcode) {
			int st = e[3];
			printf("cmd %#06x -> status %#x\n", opcode, st);
			drop(n);
			return st;
		}
		drop(n);
	}
	printf("no reply to %#06x within 5 s\n", opcode);
	return -1;
}

/* 打印一条广播/扫描响应：地址、RSSI、名字（AD 0x08/0x09） */
static void report(uint8_t evt_type, uint8_t atype, const uint8_t *addr, const uint8_t *data, uint8_t dlen, int8_t rssi) {
	printf("  %02x:%02x:%02x:%02x:%02x:%02x  type %u  rssi %d dBm  adv %u B  evt %#x",
	    addr[5], addr[4], addr[3], addr[2], addr[1], addr[0], atype, rssi, dlen, evt_type);
	const char *name = NULL; char nb[32] = {0};
	for (unsigned i = 0; i + 1 < dlen; ) {
		unsigned len = data[i];
		if (len == 0 || i + len >= dlen) break;
		uint8_t t = data[i + 1];
		if (t == 0x08 || t == 0x09) {
			unsigned n = len - 1;
			if (n > sizeof(nb) - 1) n = sizeof(nb) - 1;
			memcpy(nb, data + i + 2, n);
			name = nb;
			break;
		}
		i += len + 1;
	}
	if (name) printf("  \"%s\"", name);
	printf("\n");
}

int
main(int argc, char **argv)
{
	int listen_s = 12, active = 0, ch;
	while ((ch = getopt(argc, argv, "l:a")) != -1) {
		switch (ch) {
		case 'l': listen_s = atoi(optarg); break;
		case 'a': active = 1; break;
		default: fprintf(stderr, "usage: btscan [-l seconds] [-a]\n"); return 2;
		}
	}

	int fd = socket(32, SOCK_STREAM, SYSPROTO_CONTROL);
	if (fd < 0) { perror("socket"); return 1; }
	struct ctl_info info;
	memset(&info, 0, sizeof(info));
	strlcpy(info.ctl_name, "com.apple.uart.bluetooth", sizeof(info.ctl_name));
	if (ioctl(fd, CTLIOCGINFO, &info) < 0) {
		fprintf(stderr, "CTLIOCGINFO: %s (transport not up yet?)\n", strerror(errno));
		return 1;
	}
	struct sockaddr_ctl sc;
	memset(&sc, 0, sizeof(sc));
	sc.sc_len = sizeof(sc); sc.sc_family = 32; sc.ss_sysaddr = AF_SYS_CONTROL;
	sc.sc_id = info.ctl_id; sc.sc_unit = 0;
	if (connect(fd, (struct sockaddr *)&sc, sizeof(sc)) < 0) {
		fprintf(stderr, "connect: %s\n", strerror(errno));
		return 1;
	}
	printf("connected (ctl id %u), %s scan\n", info.ctl_id, active ? "active" : "passive");

	struct termios t;
	socklen_t tl = sizeof(t);
	if (getsockopt(fd, SYSPROTO_CONTROL, 0x40487413, &t, &tl) == 0) {
		cfmakeraw(&t); cfsetspeed(&t, 3000000); t.c_iflag |= 4; t.c_cflag = 0x38B00;
		if (setsockopt(fd, SYSPROTO_CONTROL, 0x80487414, &t, sizeof(t)) < 0)
			printf("TIOCSETA: %s\n", strerror(errno));
	}

	if (command(fd, 0x0c03, NULL, 0, NULL, NULL) != 0) { printf("reset failed\n"); return 1; }

	uint8_t par[8];
	par[0] = active ? 0x01 : 0x00;   /* scan type */
	par[1] = 0x60; par[2] = 0x00;    /* interval 60 ms */
	par[3] = 0x30; par[4] = 0x00;    /* window 30 ms */
	par[5] = 0x00;                   /* own addr: public */
	par[6] = 0x00;                   /* filter policy */
	par[7] = 0x01;                   /* 0x01 = 只收可连接/可扫描的；留 0 全收 */
	par[7] = 0x00;
	if (command(fd, 0x200b, par, 7, NULL, NULL) != 0) { printf("set scan params failed\n"); return 1; }

	uint8_t en[2] = { 0x01, 0x00 };
	if (command(fd, 0x200c, en, 2, NULL, NULL) != 0) { printf("scan enable failed\n"); return 1; }

	printf("scanning %d s ...\n", listen_s);
	double end = now() + listen_s;
	unsigned n, count = 0;
	while (get_packet(fd, end, &n)) {
		uint8_t *e = rxb;
		if (e[0] == 4 && e[1] == 0x3e && n >= 5) {
			uint8_t sub = e[3], *p = e + 4, *pend = e + n;
			if (sub == 0x02 && p < pend) {
				uint8_t num = *p++;
				for (uint8_t i = 0; i < num && p + 10 <= pend; i++) {
					uint8_t evt = p[0], atype = p[1];
					const uint8_t *addr = p + 2;
					uint8_t dlen = p[8];
					if (p + 9 + dlen + 1 > pend) break;
					const uint8_t *data = p + 9;
					int8_t rssi = (int8_t)p[9 + dlen];
					report(evt, atype, addr, data, dlen, rssi);
					p += 10 + dlen;
					count++;
				}
			} else if (sub == 0x01 && p < pend) {
				uint8_t ev = *p++;
				if (p + 7 <= pend) {
					printf("  conn complete: status %#x handle %u addr %02x:%02x:%02x:%02x:%02x:%02x\n",
					    ev & 0xff, *(uint16_t *)(p + 4), p[8], p[7], p[6], p[5], p[4], p[3]);
				}
			}
		}
		drop(n);
	}

	uint8_t off[2] = { 0x00, 0x00 };
	command(fd, 0x200c, off, 2, NULL, NULL);
	printf("done: %u adv reports\n", count);
	close(fd);
	return 0;
}
