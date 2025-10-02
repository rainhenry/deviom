# 编译器和编译选项
CC = gcc
CFLAGS = -Wall -O2
LDFLAGS = 

# 目标程序名
TARGET = deviom

# 源文件
SRCS = main.c deviom.c

# 对象文件
OBJS = $(SRCS:.c=.o)

# 头文件
HEADERS = deviom.h

# 默认目标
all: $(TARGET)

# 链接目标程序
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

# 编译源文件
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# 清理生成的文件
clean:
	-rm -f $(OBJS) $(TARGET)

# 重新构建
rebuild: clean all

# 安装到系统目录（可选）
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/

# 卸载程序（可选）
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# 显示帮助信息
help:
	@echo "Available targets:"
	@echo "  all      - Build the program (default)"
	@echo "  clean    - Remove generated files"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  install  - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  help     - Show this help"

.PHONY: all clean rebuild install uninstall help
