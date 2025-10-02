#include "deviom.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <sys/io.h>

// 内存映射相关常量
#define PAGE_SIZE getpagesize()
#define PAGE_MASK (~(PAGE_SIZE - 1))

// 全局变量
static int mem_fd = -1;
static int port_fd = -1;
static int io_system_initialized = 0;

// I/O端口权限管理数组
static io_port_info_t io_ports[MAX_IO_PORTS];
static int io_port_count = 0;

/**
 * 初始化I/O系统
 */
int init_io_system(void)
{
    if (io_system_initialized) {
        return 0; // 已经初始化
    }
    
    // 初始化端口权限数组
    for (int i = 0; i < MAX_IO_PORTS; i++) {
        io_ports[i].port = 0;
        io_ports[i].width = 0;
        io_ports[i].active = 0;
    }
    
    io_port_count = 0;
    io_system_initialized = 1;
    
    return 0;
}

/**
 * 清理I/O系统资源
 */
void cleanup_io_system(void)
{
    if (!io_system_initialized) {
        return;
    }
    
    // 释放所有活跃的I/O权限
    release_all_io_permissions();
    
    // 关闭文件描述符
    if (mem_fd != -1) {
        close(mem_fd);
        mem_fd = -1;
    }
    
    if (port_fd != -1) {
        close(port_fd);
        port_fd = -1;
    }
    
    io_system_initialized = 0;
}

/**
 * 查找端口在权限管理数组中的索引
 */
static int find_port_index(unsigned short port)
{
    for (int i = 0; i < io_port_count; i++) {
        if (io_ports[i].port == port) {
            return i;
        }
    }
    return -1;
}

/**
 * 添加新的端口到权限管理数组
 */
static int add_port_entry(unsigned short port, int width)
{
    if (io_port_count >= MAX_IO_PORTS) {
        return -1; // 数组已满
    }
    
    int index = io_port_count++;
    io_ports[index].port = port;
    io_ports[index].width = width;
    io_ports[index].active = 0;
    
    return index;
}

/**
 * 获取I/O端口访问权限
 */
int acquire_io_permission(unsigned short port, int width)
{
    if (!io_system_initialized) {
        if (init_io_system() < 0) {
            return -1;
        }
    }
    
    // 查找端口是否已经在管理数组中
    int index = find_port_index(port);
    if (index == -1) {
        // 端口不在数组中，添加新条目
        index = add_port_entry(port, width);
        if (index == -1) {
            return -1; // 无法添加更多端口
        }
    } else {
        // 端口已在数组中，检查宽度是否匹配
        if (io_ports[index].width != width) {
            // 如果权限已激活，需要先释放再重新申请
            if (io_ports[index].active) {
                ioperm(io_ports[index].port, io_ports[index].width, 0);
            }
            io_ports[index].width = width;
            io_ports[index].active = 0;
        }
    }
    
    // 如果权限尚未激活，则申请权限
    if (!io_ports[index].active) {
        if (ioperm(io_ports[index].port, io_ports[index].width, 1) < 0) {
            return -1; // 权限申请失败
        }
        io_ports[index].active = 1;
    }
    
    return 0;
}

/**
 * 释放特定端口的I/O访问权限
 */
int release_io_permission(unsigned short port)
{
    if (!io_system_initialized) {
        return 0; // 系统未初始化，无需释放
    }
    
    int index = find_port_index(port);
    if (index == -1) {
        return 0; // 端口未在管理数组中
    }
    
    // 如果权限已激活，则释放权限
    if (io_ports[index].active) {
        if (ioperm(io_ports[index].port, io_ports[index].width, 0) < 0) {
            return -1; // 权限释放失败
        }
        io_ports[index].active = 0;
    }
    
    // 从数组中移除该端口（将后面的元素前移）
    for (int i = index; i < io_port_count - 1; i++) {
        io_ports[i] = io_ports[i + 1];
    }
    io_port_count--;
    
    return 0;
}

/**
 * 释放所有I/O访问权限
 */
int release_all_io_permissions(void)
{
    if (!io_system_initialized) {
        return 0; // 系统未初始化，无需释放
    }
    
    int result = 0;
    
    // 释放所有活跃的权限
    for (int i = 0; i < io_port_count; i++) {
        if (io_ports[i].active) {
            if (ioperm(io_ports[i].port, io_ports[i].width, 0) < 0) {
                result = -1; // 至少有一个权限释放失败
            }
            io_ports[i].active = 0;
        }
    }
    
    // 清空端口计数
    io_port_count = 0;
    
    return result;
}

/**
 * 从I/O端口读取数据
 */
int read_io_port(unsigned short port, int width, void *value)
{
    // 首先尝试使用ioperm方法
    if (acquire_io_permission(port, width) < 0) {
        // ioperm失败，回退到使用/dev/port的方法
        if (port_fd == -1) {
            port_fd = open("/dev/port", O_RDONLY);
            if (port_fd == -1) {
                perror("Failed to open /dev/port");
                return -1;
            }
        }
        
        // 使用pread进行安全读取
        switch (width) {
            case 1: {
                unsigned char val;
                if (pread(port_fd, &val, 1, port) == 1) {
                    *(unsigned char *)value = val;
                } else {
                    return -1;
                }
                break;
            }
            case 2: {
                unsigned short val;
                if (pread(port_fd, &val, 2, port) == 2) {
                    *(unsigned short *)value = val;
                } else {
                    return -1;
                }
                break;
            }
            case 4: {
                unsigned int val;
                if (pread(port_fd, &val, 4, port) == 4) {
                    *(unsigned int *)value = val;
                } else {
                    return -1;
                }
                break;
            }
            default:
                return -1;
        }
    } else {
        // ioperm成功，使用标准I/O函数
        switch (width) {
            case 1: {
                unsigned char val = inb(port);
                *(unsigned char *)value = val;
                break;
            }
            case 2: {
                unsigned short val = inw(port);
                *(unsigned short *)value = val;
                break;
            }
            case 4: {
                unsigned int val = inl(port);
                *(unsigned int *)value = val;
                break;
            }
            default:
                return -1;
        }
    }
    
    return 0;
}

/**
 * 向I/O端口写入数据
 */
int write_io_port(unsigned short port, int width, unsigned long value)
{
    // 首先尝试使用ioperm方法
    if (acquire_io_permission(port, width) < 0) {
        // ioperm失败，回退到使用/dev/port的方法
        if (port_fd == -1) {
            port_fd = open("/dev/port", O_WRONLY);
            if (port_fd == -1) {
                perror("Failed to open /dev/port");
                return -1;
            }
        }
        
        // 使用pwrite进行安全写入
        switch (width) {
            case 1: {
                unsigned char val = (unsigned char)value;
                if (pwrite(port_fd, &val, 1, port) != 1) {
                    return -1;
                }
                break;
            }
            case 2: {
                unsigned short val = (unsigned short)value;
                if (pwrite(port_fd, &val, 2, port) != 2) {
                    return -1;
                }
                break;
            }
            case 4: {
                unsigned int val = (unsigned int)value;
                if (pwrite(port_fd, &val, 4, port) != 4) {
                    return -1;
                }
                break;
            }
            default:
                return -1;
        }
    } else {
        // ioperm成功，使用标准I/O函数
        switch (width) {
            case 1:
                outb((unsigned char)value, port);
                break;
            case 2:
                outw((unsigned short)value, port);
                break;
            case 4:
                outl((unsigned int)value, port);
                break;
            default:
                return -1;
        }
    }
    
    return 0;
}

/**
 * 从物理内存地址读取数据
 */
int read_memory(uint64_t address, int width, void *value)
{
    void *map_base;
    void *virt_addr;
    uint64_t page_base;
    uint64_t page_offset;
    
    // 打开 /dev/mem
    if (mem_fd == -1) {
        mem_fd = open("/dev/mem", O_RDONLY);
        if (mem_fd == -1) {
            perror("Failed to open /dev/mem");
            return -1;
        }
    }
    
    // 计算页基址和偏移
    page_base = address & PAGE_MASK;
    page_offset = address - page_base;
    
    // 映射内存页面
    map_base = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, mem_fd, (off_t)page_base);
    if (map_base == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }
    
    virt_addr = (char *)map_base + page_offset;
    
    // 根据宽度读取数据
    switch (width) {
        case 1:
            *(uint8_t *)value = *((volatile uint8_t *)virt_addr);
            break;
        case 2:
            *(uint16_t *)value = *((volatile uint16_t *)virt_addr);
            break;
        case 4:
            *(uint32_t *)value = *((volatile uint32_t *)virt_addr);
            break;
        case 8:
            *(uint64_t *)value = *((volatile uint64_t *)virt_addr);
            break;
        default:
            munmap(map_base, PAGE_SIZE);
            return -1;
    }
    
    // 取消映射
    munmap(map_base, PAGE_SIZE);
    return 0;
}

/**
 * 向物理内存地址写入数据
 */
int write_memory(uint64_t address, int width, uint64_t value)
{
    void *map_base;
    void *virt_addr;
    uint64_t page_base;
    uint64_t page_offset;
    
    // 打开 /dev/mem
    if (mem_fd == -1) {
        mem_fd = open("/dev/mem", O_RDWR);
        if (mem_fd == -1) {
            perror("Failed to open /dev/mem");
            return -1;
        }
    }
    
    // 计算页基址和偏移
    page_base = address & PAGE_MASK;
    page_offset = address - page_base;
    
    // 映射内存页面
    map_base = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t)page_base);
    if (map_base == MAP_FAILED) {
        perror("mmap failed");
        return -1;
    }
    
    virt_addr = (char *)map_base + page_offset;
    
    // 根据宽度写入数据
    switch (width) {
        case 1:
            *((volatile uint8_t *)virt_addr) = (uint8_t)value;
            break;
        case 2:
            *((volatile uint16_t *)virt_addr) = (uint16_t)value;
            break;
        case 4:
            *((volatile uint32_t *)virt_addr) = (uint32_t)value;
            break;
        case 8:
            *((volatile uint64_t *)virt_addr) = value;
            break;
        default:
            munmap(map_base, PAGE_SIZE);
            return -1;
    }
    
    // 取消映射
    munmap(map_base, PAGE_SIZE);
    return 0;
}
