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

# Font Asset
FONT_PSF = src/assets/font.psf
FONT_OBJ = font.o

# CỐ ĐỊNH: entry.o phải đứng đầu, sau đó là font.o và các file C
KERNEL_OBJS = entry.o src/interrupt.o $(FONT_OBJ) $(C_OBJS)

# Tên file output
INITRD_NAME = initrd-x86.tar
# Thư mục chứa asset
ASSETS_DIR = src/assets

all: openyanase.iso

# Chạy ISO với QEMU
run: openyanase.iso
	qemu-system-i386 -cdrom openyanase.iso -vga std -d guest_errors

# Tạo file ISO khởi động với Syslinux
openyanase.iso: mykernel.elf $(INITRD_NAME) syslinux.cfg splash.png
	mkdir -p iso_root/boot/isolinux
	mkdir -p iso_root/apps
	
	# Copy Kernel và file cấu hình
	cp mykernel.elf iso_root/boot/mykernel.elf
	cp $(INITRD_NAME) iso_root/boot/$(INITRD_NAME)
	cp syslinux.cfg iso_root/boot/isolinux/isolinux.cfg
	
	# Copy ảnh nền
	cp splash.png iso_root/boot/isolinux/
	
	# Copy các file module từ thư mục bios
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
		
# Rule tạo initrd
# Đảm bảo file được tạo ra ở thư mục hiện tại

# --- SỬA ĐỔI TRIỆT ĐỂ PHẦN INITRD ---
$(INITRD_NAME):
	@echo "[STAGE 1] Preparing assets in $(ASSETS_DIR)..."
	@mkdir -p $(ASSETS_DIR)
	
	# Đảm bảo init.yc luôn tồn tại trước khi đóng gói
	@if [ ! -f $(ASSETS_DIR)/init.yc ]; then \
		echo "theme=bluearchive" > $(ASSETS_DIR)/init.yc; \
		echo "core=openyanase-core.jar" >> $(ASSETS_DIR)/init.yc; \
		echo "autostart=false" >> $(ASSETS_DIR)/init.yc; \
		echo " [ OK ] Created new init.yc"; \
	fi

	# Kiểm tra sự tồn tại của các file quan trọng khác (ví dụ font)
	@if [ ! -f $(FONT_PSF) ]; then \
		echo " [ WARN ] $(FONT_PSF) missing! GUI might fail."; \
	fi

	@echo "[TAR] Packaging assets into $(INITRD_NAME)..."
	# -C src/assets giúp lấy file ở root của tar, không bị dính đường dẫn src/assets/ vào tên file
	# find . -maxdepth 1 -type f lấy tất cả file (bao gồm init.yc, font.psf, .jar)
	cd $(ASSETS_DIR) && find . -maxdepth 1 -type f | xargs tar -cvf ../../$(INITRD_NAME)
	
	@echo "[ DONE ] $(INITRD_NAME) generated."

# Link Kernel
mykernel.elf: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)

# --- BƯỚC NHÚNG FONT PSF ---
# Biến file .psf thành file .o có thể link được
$(FONT_OBJ): $(FONT_PSF)
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

# Biên dịch Assembly (entry.S)
entry.o: src/entry.S
	$(AS) $(ASFLAGS) -c $< -o $@

src/interrupt.o: src/interrupt.S
	x86_64-linux-gnu-gcc -m32 -c src/interrupt.S -o src/interrupt.o
# ^^^ CHỖ NÀY PHẢI LÀ DẤU TAB, KHÔNG PHẢI SPACE!	
# Biên dịch các file C
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf iso_root
	rm -f $(shell find src -name '*.o') entry.o font.o mykernel.elf openyanase.iso $(INITRD_NAME)

.PHONY: all run clean