cmake_minimum_required(VERSION 3.13)

target_include_directories(MRtos_lib PUBLIC inc)
target_sources(MRtos_lib
        PRIVATE
            inc/mrtos_config.h
            inc/os_core.h
            inc/os_semaphore.h
            inc/os_scheduling.h
            inc/os_threads.h
            inc/os_buffers.h
            src/os_core.c
            src/os_scheduling.c
            src/os_semaphore.c
            src/os_threads.c
            src/os_buffers.c
        )