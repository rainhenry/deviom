#include "deviom.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

//  注意！mem访问，需要关闭内核的 CONFIG_STRICT_DEVMEM 

// 函数声明
void show_help(const char *program_name);

/**
 * 显示帮助信息
 */
void show_help(const char *program_name)
{
    printf("Usage: %s [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h, --help               Show this help message\n");
    printf("  -p, --port PORT          Specify I/O port number (hex)\n");
    printf("  -m, --memory ADDR        Specify memory address (hex)\n");
    printf("  -r, --read               Perform read operation\n");
    printf("  -w, --write VALUE        Perform write operation with VALUE\n");
    printf("  -W, --width WIDTH        Data width: 1(byte), 2(halfword), 4(word), 8(double word)\n");
    printf("  -t, --type TYPE          Operation type: io(port), mem(memory)\n");
    printf("\nExamples:\n");
    printf("  %s -t io -p 0x3f8 -r -W 1           # Read 1 byte from I/O port 0x3f8\n", program_name);
    printf("  %s -t io -p 0x3f8 -w 0x41 -W 1      # Write 0x41 to I/O port 0x3f8\n", program_name);
    printf("  %s -t mem -m 0x100000 -r -W 4       # Read 1 word from memory address 0x100000\n", program_name);
    printf("  %s -t mem -m 0x100000 -w 0x12345678 -W 4  # Write data to memory address 0x100000\n", program_name);
}

/**
 * 主函数
 */
int main(int argc, char **argv)
{
    int opt;
    unsigned short port = 0;
    uint64_t address = 0;
    uint64_t value = 0;
    int width = 4;
    int read_op = 0;
    int write_op = 0;
    int type = 0; // 0=none, 1=io, 2=memory
    
    // 初始化I/O系统
    if (init_io_system() < 0) {
        fprintf(stderr, "Failed to initialize I/O system\n");
        exit(EXIT_FAILURE);
    }
    
    // 定义长选项
    static struct option long_options[] = {
        {"help",    no_argument,       0, 'h'},
        {"port",    required_argument, 0, 'p'},
        {"memory",  required_argument, 0, 'm'},
        {"read",    no_argument,       0, 'r'},
        {"write",   required_argument, 0, 'w'},
        {"width",   required_argument, 0, 'W'},
        {"type",    required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    
    // 解析命令行参数
    while ((opt = getopt_long(argc, argv, "hp:m:rw:W:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                show_help(argv[0]);
                cleanup_io_system();
                exit(EXIT_SUCCESS);
            case 'p':
                port = (unsigned short)strtoul(optarg, NULL, 16);
                break;
            case 'm':
                address = strtoull(optarg, NULL, 16);
                break;
            case 'r':
                read_op = 1;
                break;
            case 'w':
                value = strtoull(optarg, NULL, 16);
                write_op = 1;
                break;
            case 'W':
                width = atoi(optarg);
                break;
            case 't':
                if (strcmp(optarg, "io") == 0 || strcmp(optarg, "port") == 0) {
                    type = 1;
                } else if (strcmp(optarg, "mem") == 0 || strcmp(optarg, "memory") == 0) {
                    type = 2;
                } else {
                    fprintf(stderr, "Unknown operation type: %s\n", optarg);
                    cleanup_io_system();
                    exit(EXIT_FAILURE);
                }
                break;
            default:
                fprintf(stderr, "Use %s --help for help information\n", argv[0]);
                cleanup_io_system();
                exit(EXIT_FAILURE);
        }
    }
    
    // 参数验证
    if (type == 0) {
        fprintf(stderr, "Operation type must be specified (-t io or -t mem)\n");
        cleanup_io_system();
        exit(EXIT_FAILURE);
    }
    
    if (type == 1 && port == 0) {
        fprintf(stderr, "I/O port operation requires port number (-p)\n");
        cleanup_io_system();
        exit(EXIT_FAILURE);
    }
    
    if (type == 2 && address == 0) {
        fprintf(stderr, "Memory operation requires address (-m)\n");
        cleanup_io_system();
        exit(EXIT_FAILURE);
    }
    
    if (!read_op && !write_op) {
        fprintf(stderr, "Read (-r) or write (-w) operation must be specified\n");
        cleanup_io_system();
        exit(EXIT_FAILURE);
    }
    
    if (read_op && write_op) {
        fprintf(stderr, "Cannot perform read and write operations simultaneously\n");
        cleanup_io_system();
        exit(EXIT_FAILURE);
    }
    
    // 执行相应操作
    if (type == 1) {
        // I/O端口操作
        if (read_op) {
            unsigned long result = 0;
            if (read_io_port(port, width, &result) == 0) {
                printf("Read from I/O port 0x%04x (%d bytes): 0x%lx\n", port, width, result);
            } else {
                fprintf(stderr, "Failed to read from I/O port 0x%04x\n", port);
                cleanup_io_system();
                exit(EXIT_FAILURE);
            }
        } else if (write_op) {
            if (write_io_port(port, width, value) == 0) {
                printf("Wrote to I/O port 0x%04x (%d bytes): 0x%lx\n", port, width, value);
            } else {
                fprintf(stderr, "Failed to write to I/O port 0x%04x\n", port);
                cleanup_io_system();
                exit(EXIT_FAILURE);
            }
        }
    } else if (type == 2) {
        // 内存操作
        if (read_op) {
            unsigned long result = 0;
            if (read_memory(address, width, &result) == 0) {
                printf("Read from memory address 0x%016llx (%d bytes): 0x%lx\n", 
                       (unsigned long long)address, width, result);
            } else {
                fprintf(stderr, "Failed to read from memory address 0x%016llx\n", 
                        (unsigned long long)address);
                cleanup_io_system();
                exit(EXIT_FAILURE);
            }
        } else if (write_op) {
            if (write_memory(address, width, value) == 0) {
                printf("Wrote to memory address 0x%016llx (%d bytes): 0x%llx\n", 
                       (unsigned long long)address, width, (unsigned long long)value);
            } else {
                fprintf(stderr, "Failed to write to memory address 0x%016llx\n", 
                        (unsigned long long)address);
                cleanup_io_system();
                exit(EXIT_FAILURE);
            }
        }
    }
    
    // 清理资源
    cleanup_io_system();
    
    return 0;
}
