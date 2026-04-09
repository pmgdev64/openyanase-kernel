# Trình biên dịch & Linker
CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

# Flags
# -Isrc giúp trình biên dịch tìm thấy các file .h trong thư mục src
CFLAGS = -m32 -ffreestanding -fno-stack-protector -nostdlib -fno-pie -Isrc
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld

# Danh sách các file Object
# Tự động tìm tất cả file .c và .S trong src rồi đổi đuôi thành .o
OBJS = entry.o main.o vga_buffer.o gdt.o keyboard.o echo.o
all: kernel.bin

# Link các file object thành file ELF
mykernel.elf: $(OBJS)
	$(LD) $(LDFLAGS) -o mykernel.elf $(OBJS)

# Chuyển file ELF sang binary thuần túy để Bootloader có thể nạp
kernel.bin: mykernel.elf
	$(OBJCOPY) -O binary mykernel.elf kernel.bin

# Quy tắc biên dịch file Assembly (Lưu ý: src/entry.S của bro là chữ S hoa)
entry.o: src/entry.S
	$(AS) $(ASFLAGS) src/entry.S -o entry.o

# Quy tắc biên dịch cho các file trong src/applications/
echo.o: src/applications/echo.c
	$(CC) $(CFLAGS) -c $< -o $@

# Quy tắc chung cho tất cả các file .c
# % đại diện cho tên file bất kỳ. $< là file nguồn, $@ là file đích .o
%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o mykernel.elf kernel.bin