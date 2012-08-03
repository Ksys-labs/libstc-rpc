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

#define LOG_TAG "[GPS-RPC] "

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
				RPC_PRINTF("Error at [%s:%d] " x, __func__, __LINE__, ##a); \
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
	RPC_ERROR("%s: error code %d message \'%s\'\n", \
		f, errno, strerror(errno))

#define LOG_ENTRY do { RPC_DEBUG("+%s\n", __func__); } while (0)
#define LOG_EXIT do { RPC_DEBUG("-%s\n", __func__); } while (0)

#endif //__STC_LOG_H__
