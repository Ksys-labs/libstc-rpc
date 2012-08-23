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

#ifndef __STC_RPC_H__
#define __STC_RPC_H__

#include "stc_log.h"

#define RPC_TIMEOUT_US (1000 * 500)
#define RPC_PAYLOAD_MAX (2048)

typedef struct rpc_request_hdr_t {
	unsigned code;
	char buffer[RPC_PAYLOAD_MAX];
} __attribute__((packed)) rpc_request_hdr_t;

typedef struct rpc_reply_t {
	unsigned code;	
	char buffer[RPC_PAYLOAD_MAX];
} __attribute__((packed)) rpc_reply_t;

typedef struct rpc_request_t {
	rpc_request_hdr_t header;
	rpc_reply_t reply;
	
	int *reply_marker;
} __attribute__((packed)) rpc_request_t;

typedef int (*rpc_handler_t)(rpc_request_hdr_t *hdr, rpc_reply_t *reply);

struct rpc;
typedef struct rpc rpc_t;

#define RPC_PACK_RAW(buffer, idx, pvalue, size) do {\
	if (idx + size >= RPC_PAYLOAD_MAX) {\
		RPC_ERROR("data too large");\
		goto fail; \
	}\
	memcpy(buffer + idx, pvalue, size);\
	idx += size;\
} while (0)

#define RPC_PACK(buffer, idx, value) \
	RPC_PACK_RAW(buffer, idx, &value, sizeof(value))

#define RPC_PACK_S(buffer, idx, str) do {\
	size_t __len = strlen(str) + 1; \
	RPC_PACK_RAW(buffer, idx, str, __len); \
} while (0)

#define RPC_UNPACK_RAW(buffer, idx, pvalue, size) do {\
	if (idx + size >= RPC_PAYLOAD_MAX) {\
		RPC_ERROR("data too large : %d bytes", idx + size);\
		goto fail;\
	}\
	memcpy(pvalue, buffer + idx, size);\
	idx += size;\
} while (0)

#define RPC_UNPACK(buffer, idx, value) \
	RPC_UNPACK_RAW(buffer, idx, &value, sizeof(value))

#define RPC_UNPACK_S(buffer, idx, str) do {\
	size_t __len = strlen(buffer + idx) + 1;\
	RPC_UNPACK_RAW(buffer, idx, str, __len);\
} while (0)


/*
 * @brief Allocates the rpc structure
 *
 * @return pointer to the allcated structure
 * @return NULL in case of a failure
 */
rpc_t *rpc_alloc(void);


/*
 * @brief De-allocates the rpc structure
 *
 * @param rpc The pointer to the rpc structure or NULL
 */
void rpc_free(rpc_t *rpc);

/*
 * @brief initializes the RPC structure
 *
 * @param fd the file descriptor of a socket for rpc
 * this must be a two-way socket, not a pipe
 * this function sets the file descriptor into a NONBLOCK mode
 * @param handler the pointer to the callback processing RPC messages
 * @param rpc the pointer to the structure to setup
 *
 * @return zero (0) in case of success
 * @return negative error code in case of a failure
 */
int rpc_init(int fd, rpc_handler_t handler, rpc_t *rpc);

/*
 * @brief Performs the RPC call and blocks until reply is received if
 * the RPC procedure is supposed to return a value
 *
 * @return zero (0) in case of success
 * @return negative error code in case of a failure
 */
int rpc_call(rpc_t *rpc, rpc_request_t *req);

/*
 * @brief Performs the RPC call and returns immediately without waiting
 * for the request to complete
 *
 * @return zero (0) in case of success
 * @return negative error code in case of a failure
 */
int rpc_call_noreply(rpc_t *rpc, rpc_request_t *req);

/*
 * @brief starts the RPC worker thread
 *
 * @param rpc the pointer to the rpc_init'ed RPC structure
 *
 * @return zero (0) in case of success
 * @return negative error code in case of a failure
 */
int rpc_start(rpc_t *rpc);

/*
 * @brief waits for the rpc thread to complete
 *
 * @param rpc the pointer to the rpc_init'ed RPC structure
 *
 * @return zero (0) in case of success
 * @return negative error code in case of a failure
 */
int rpc_join(rpc_t *rpc); 

#endif //__STC_RPC_H__
