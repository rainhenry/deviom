#ifndef DEVIOM_H
#define DEVIOM_H

#include <stdint.h>

// 最大支持的端口数量
#define MAX_IO_PORTS 256

// 端口权限状态结构
typedef struct {
    unsigned short port;
    int width;
    int active;
} io_port_info_t;

// 函数声明
int init_io_system(void);
void cleanup_io_system(void);

// I/O端口权限管理API
int acquire_io_permission(unsigned short port, int width);
int release_io_permission(unsigned short port);
int release_all_io_permissions(void);

// I/O端口读写操作
int read_io_port(unsigned short port, int width, void *value);
int write_io_port(unsigned short port, int width, unsigned long value);

// 内存读写操作
int read_memory(uint64_t address, int width, void *value);
int write_memory(uint64_t address, int width, uint64_t value);

#endif // DEVIOM_H
