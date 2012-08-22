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

#ifndef __STC_LOG_H__
#define __STC_LOG_H__

enum debug_level {
	LOG_SILENT,
	LOG_ERROR,
	LOG_INFO,
	LOG_DEBUG,
};

#define LOG_SILENT 0
#define LOG_ERROR (LOG_SILENT + 1)
#define LOG_INFO (LOG_ERROR + 1)
#define LOG_DEBUG (LOG_INFO + 1)

#ifndef DEBUG_LEVEL
	#define DEBUG_LEVEL LOG_DEBUG
#endif

#ifndef LOG_TAG
	#define LOG_TAG "[GPS-RPC] "
#endif

#ifdef ANDROID
	#include <utils/Log.h>
#endif

#ifdef ANDROID
	#define RPC_PRINTF(x, a...) ALOGI(x, ##a)
#else
	#define RPC_PRINTF(x, a...) do {\
			printf("\033[31m" LOG_TAG x "\033[0m\n", ##a);\
		} while (0)
#endif

#define RPC_PUTS(x) do {\
	RPC_PRINTF("%s", x);\
} while (0)

#if DEBUG_LEVEL >= LOG_ERROR
	#ifdef ANDROID
		#define RPC_ERROR(x, a...) do {\
				ALOGE("Error at [%s:%d] " x, __func__, __LINE__, ##a);\
			} while (0)
	#else
		#define RPC_ERROR(x, a...) do {\
				RPC_PRINTF("Error at [%s:%d] " x "\n", __func__, __LINE__, ##a); \
			} while (0)
	#endif
#else
	#define RPC_ERROR(x, a...) do {} while (0)
#endif

#if DEBUG_LEVEL >= LOG_DEBUG
	#ifdef ANDROID
		#define RPC_DEBUG(x, a...) ALOGD(x, ##a)
	#else
		#define RPC_DEBUG(x, a...) do {\
				RPC_PRINTF(x, ##a); \
			} while (0)
	#endif
#else
	#define RPC_DEBUG(x, a...) do {} while (0)
#endif

#if DEBUG_LEVEL >= LOG_INFO
	#ifdef ANDROID
		#define RPC_INFO(x, a...) ALOGI(x, ##a)
	#else
		#define RPC_INFO(x, a...) do {\
				RPC_PRINTF(x, ##a); \
			} while (0)
	#endif
#else
	#define RPC_INFO(x, a...) do {} while (0)
#endif

#define RPC_PERROR(f) \
	RPC_ERROR("%s: error code %d message \'%s\'", \
		f, errno, strerror(errno))

#define LOG_ENTRY do { RPC_DEBUG("+%s", __FUNCTION__); } while (0)
#define LOG_EXIT do { RPC_DEBUG("-%s", __FUNCTION__); } while (0)

#endif //__STC_LOG_H__
