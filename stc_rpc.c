/**
 * This file is part of stc-rpc.
 *
 * Copyright (C) 2012 Alexander Tarasikov <alexander.tarasikov@gmail.com>
 *
 * stc-rpc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * stc-rpc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with stc-rpc.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <pthread.h>

#include <stc_rpc.h>
#include <stc_log.h>

struct rpc {
	int fd;
	int pipefd[2];
	int active;
	rpc_handler_t handler;
	
	pthread_mutex_t fd_mutex;
	pthread_t rpc_thread;

	pthread_mutex_t cond_mtx;
	pthread_cond_t cond;
};

enum {
	READ_END = 0,
	WRITE_END = 1,
};

static void set_nonblocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static inline int max(int a, int b) {
	return a > b ? a : b;
}

static void rpc_cond_wait(struct rpc *rpc) {
	pthread_mutex_lock(&rpc->cond_mtx);
	pthread_cond_wait(&rpc->cond, &rpc->cond_mtx);
	pthread_mutex_unlock(&rpc->cond_mtx);
}

static void rpc_cond_signal(struct rpc *rpc) {
	pthread_mutex_lock(&rpc->cond_mtx);
	pthread_cond_broadcast(&rpc->cond);
	pthread_mutex_unlock(&rpc->cond_mtx);
}

static int select_write(int fd) {
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	
	struct timeval tv = {
		.tv_usec = RPC_TIMEOUT_US,
	};

	return select(fd + 1, NULL, &fds, NULL, &tv);
}

static int rpc_read(int fd, void *data, size_t size) {
	int ret;
	LOG_ENTRY;
	do {
		ret = recv(fd, data, size, 0);
	} while (ret < 0 && errno == EAGAIN);
	if (ret < 0) {
		RPC_PERROR("recv");
	}
	LOG_EXIT;
	return ret;
}

static int rpc_write(int fd, void *data, size_t size) {
	int ret;
	LOG_ENTRY;
	do {
		select_write(fd);
		ret = send(fd, data, size, 0);
	} while (ret < 0 && errno == EAGAIN);
	if (ret < 0) {
		RPC_PERROR("send");
	}
	LOG_EXIT;
	return ret;
}

static int rpc_send(struct rpc *rpc) {
	LOG_ENTRY;

	struct rpc_request_t req;
	if (read(rpc->pipefd[READ_END], &req, sizeof(req)) < 0) {
		RPC_PERROR("read");
		goto fail;
	}

	RPC_DEBUG(">>> code %x", req.header.code);
	
	if (rpc_write(rpc->fd, &req.header, sizeof(rpc_request_hdr_t)) < 0) {
		RPC_PERROR("rpc_write");
		goto fail;
	}
	RPC_DEBUG(">>> sent header");

	if (rpc_read(rpc->fd, &req.reply, sizeof(rpc_reply_t)) < 0) {
		RPC_PERROR("rpc_read");
		goto fail;
	}
	RPC_DEBUG(">>> reply code %x", req.reply.code);

	if (req.reply_marker) {
		req.reply_marker[0] = 1;
	}
	rpc_cond_signal(rpc);

	LOG_EXIT;
	return 0;
fail:
	return -1;
}

static int rpc_recv(struct rpc *rpc) {
	struct rpc_request_hdr_t hdr;
	struct rpc_reply_t reply;
	LOG_ENTRY;
		
	memset(&reply, 0, sizeof(reply));

	if (rpc_read(rpc->fd, &hdr, sizeof(hdr)) < 0) {
		RPC_PERROR("rpc_read");
		goto fail;
	}

	RPC_DEBUG("<<< header code %x", hdr.code);
	if (rpc->handler(&hdr, &reply)) {
		RPC_ERROR("failed to handle message");
	}
	RPC_DEBUG("<<< handled message %x", hdr.code);

	if (rpc_write(rpc->fd, &reply, sizeof(reply)) < 0) {
		RPC_PERROR("rpc_write");
		goto fail;
	}
	RPC_DEBUG("<<< done with message %x", hdr.code);

	LOG_EXIT;
	return 0;
fail:
	return -1;
}

static void* do_rpc_thread(void *data) {
	LOG_ENTRY;

	struct rpc *rpc = data;
	if (!rpc || !rpc->handler) {
		RPC_PUTS("bad rpc");
		LOG_EXIT;
		return NULL;
	}
	
	rpc->active = 1;
	rpc_cond_signal(rpc);

	fd_set fds;

	while (rpc->active) {
		RPC_DEBUG("+loop");
		
		FD_ZERO(&fds);
		FD_SET(rpc->fd, &fds);
		FD_SET(rpc->pipefd[READ_END], &fds);

		struct timeval tv = {
			.tv_usec = RPC_TIMEOUT_US,
		};

		select(max(rpc->fd, rpc->pipefd[READ_END]) + 1, &fds, NULL, NULL, &tv);
		pthread_mutex_lock(&rpc->fd_mutex);
		if (FD_ISSET(rpc->fd, &fds)) {
			RPC_DEBUG("receiving RPC message");
			if (rpc_recv(rpc) < 0) {
				RPC_ERROR("receive error");
				rpc->active = 0;
			}
		}
		if (FD_ISSET(rpc->pipefd[READ_END], &fds)) {
			RPC_DEBUG("calling remote party");
			if (rpc_send(rpc) < 0) {
				RPC_ERROR("call error");
				rpc->active = 0;
			}
		}
		pthread_mutex_unlock(&rpc->fd_mutex);
		RPC_DEBUG("-loop");
	}

	LOG_EXIT;
	return NULL;
}

int rpc_call(struct rpc *rpc, struct rpc_request_t *req) {
	int ret = 0;
	LOG_ENTRY;

	int done = 0;
	req->reply_marker = &done;

	pthread_mutex_lock(&rpc->fd_mutex);
	ret = write(rpc->pipefd[WRITE_END], req, sizeof(rpc_request_t));
	pthread_mutex_unlock(&rpc->fd_mutex);

	if (ret < 0) {
		RPC_PERROR("write");
		goto fail;
	}

	RPC_DEBUG("waiting for reply");
	while (!done) {
		rpc_cond_wait(rpc);
	}
	RPC_DEBUG("got reply");
	ret = 0;

fail:
	LOG_EXIT;
	return ret;
}

rpc_t *rpc_alloc(void) {
	rpc_t *ret = NULL;

	ret = (rpc_t*)malloc(sizeof(rpc_t));
	if (!ret) {
		RPC_DEBUG("out of memory");
		goto fail;
	}
	memset(ret, 0, sizeof(rpc_t));

	return ret;
fail:
	if (ret) {
		free(ret);
	}
	return NULL;
}

void rpc_free(rpc_t *rpc) {
	if (!rpc) {
		return;
	}
	free(rpc);
}

int rpc_init(int fd, rpc_handler_t handler, rpc_t *rpc) {
	if (fd < 0) {
		RPC_DEBUG("bad fd %d", fd);
		goto fail;
	}

	if (!handler) {
		RPC_DEBUG("handler is NULL");
		goto fail;
	}
	
	if (!rpc) {
		RPC_DEBUG("rpc is NULL");
		goto fail;
	}

	if (pthread_mutex_init(&rpc->fd_mutex, NULL)) {
		RPC_PERROR("init fd mutex");
		goto fail;
	}

	if (pthread_mutex_init(&rpc->cond_mtx, NULL)) {
		RPC_PERROR("init condition mutex");
		goto fail_cond_mutex;
	}

	if (pthread_cond_init(&rpc->cond, NULL)) {
		RPC_PERROR("init condition");
		goto fail_cond;
	}

	if (pipe(rpc->pipefd) < 0) {
		RPC_PERROR("pipe");
		goto fail_pipe;
	}
	set_nonblocking(rpc->pipefd[READ_END]);
	set_nonblocking(fd);
	
	rpc->fd = fd;
	rpc->handler = handler;

	return 0;
fail_pipe:
	pthread_cond_destroy(&rpc->cond);
fail_cond:
	pthread_mutex_destroy(&rpc->cond_mtx);
fail_cond_mutex:
	pthread_mutex_destroy(&rpc->fd_mutex);
fail:
	return -1;
}

int rpc_start(rpc_t *rpc) {
	if (!rpc) {
		RPC_DEBUG("rpc is NULL");
		goto fail;
	}

	if (!rpc->handler) {
		RPC_DEBUG("rpc handler is NULL");
		goto fail;
	}
	
	if (pthread_create(&rpc->rpc_thread, NULL, do_rpc_thread, rpc)) {
		RPC_PERROR("pthread_create");
		goto fail;
	}

	while (!rpc->active) {
		rpc_cond_wait(rpc);
	}

	return 0;

fail:
	return -1;
}

int rpc_join(rpc_t *rpc) {
	return pthread_join(rpc->rpc_thread, NULL);
}
