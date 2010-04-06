#ifndef _PVRCONFIG_H
#define _PVRCONFIG_H

#define SGX530				1

#ifdef CONFIG_PVR_DEBUG
# define PVR_BUILD_TYPE			"debug"
# define DEBUG				1
#elif defined(CONFIG_PVR_TIMING)
# define PVR_BUILD_TYPE			"timing"
# define TIMING				1
#elif defined(CONFIG_PVR_RELEASE)
# define PVR_BUILD_TYPE			"release"
#endif

#ifdef CONFIG_PVR_NO_HARDWARE
# define NO_HARDWARE			1
#endif

#ifdef CONFIG_PVR_DEBUG_EXTRA
# define DEBUG_LINUX_MEMORY_ALLOCATIONS	1
# define DEBUG_LINUX_MEM_AREAS		1
# define DEBUG_LINUX_MMAP_AREAS		1
# define DEBUG_BRIDGE_KM		1
#endif

# if (defined CONFIG_PVR_DEBUG_PDUMP) || (defined CONFIG_PVR_DEBUG_PDUMP_MODULE)
#  define PDUMP				1
# endif

#ifdef CONFIG_PVR_EDM_DEBUG
# define PVRSRV_USSE_EDM_STATUS_DEBUG	1
#endif

#endif
