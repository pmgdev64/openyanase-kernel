# Trình biên dịch & Linker
CC = x86_64-linux-gnu-gcc
AS = x86_64-linux-gnu-as
LD = x86_64-linux-gnu-ld
OBJCOPY = x86_64-linux-gnu-objcopy
QEMU = qemu-system-i386

# Flags
CFLAGS = -m32 -ffreestanding -fno-stack-protector -nostdlib -fno-pie -Isrc
ASFLAGS = --32
LDFLAGS = -m elf_i386 -T linker.ld

# Đường dẫn Syslinux cục bộ
SYS_BIOS = ./bios

# Tìm tất cả file C và chuyển thành .o
C_SOURCES = $(shell find src -name '*.c')
C_OBJS = $(C_SOURCES:.c=.o)

# CỐ ĐỊNH: entry.o phải là đối tượng đầu tiên
KERNEL_OBJS = entry.o $(C_OBJS)

all: openyanase.iso

# Chạy ISO với QEMU
run: openyanase.iso
	qemu-system-i386.exe -cdrom openyanase.iso -vga std -d guest_errors

# ... (Giữ nguyên phần đầu cho đến openyanase.iso) ...

# Tạo file ISO khởi động với Syslinux
openyanase.iso: mykernel.elf syslinux.cfg splash.bmp
	mkdir -p iso_root/boot/isolinux
	mkdir -p iso_root/apps
	
	# Copy Kernel và file cấu hình
	cp mykernel.elf iso_root/boot/mykernel.elf
	cp syslinux.cfg iso_root/boot/isolinux/isolinux.cfg
	
	# --- THÊM DÒNG NÀY ĐỂ COPY ẢNH NỀN ---
	cp splash.png iso_root/boot/isolinux/
	
	# Copy các file module từ thư mục bios vào đúng folder isolinux
	find $(SYS_BIOS) -name "mboot.c32" -exec cp {} iso_root/boot/isolinux/ \;
	find $(SYS_BIOS) -name "libcom32.c32" -exec cp {} iso_root/boot/isolinux/ \;
	find $(SYS_BIOS) -name "libutil.c32" -exec cp {} iso_root/boot/isolinux/ \;
	find $(SYS_BIOS) -name "ldlinux.c32" -exec cp {} iso_root/boot/isolinux/ \;
	find $(SYS_BIOS) -name "isolinux.bin" -exec cp {} iso_root/boot/isolinux/ \;
	find $(SYS_BIOS) -name "vesamenu.c32" -exec cp {} iso_root/boot/isolinux/ \;
	find $(SYS_BIOS) -name "reboot.c32" -exec cp {} iso_root/boot/isolinux/ \;
	
	# Tạo ISO bằng xorriso
	xorriso -as mkisofs -o $@ \
		-b boot/isolinux/isolinux.bin -c boot/isolinux/boot.cat \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		iso_root/

# ... (Phần còn lại giữ nguyên) ...

# Link Kernel - QUAN TRỌNG: Thứ tự nạp $(KERNEL_OBJS) đảm bảo entry.o đứng đầu
mykernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

# Biên dịch Assembly (entry.S)
entry.o: src/entry.S
	$(AS) $(ASFLAGS) -c $< -o $@

# Biên dịch các file C
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf iso_root
	rm -f $(shell find src -name '*.o') entry.o mykernel.elf openyanase.iso

.PHONY: all run clean