# 🌀 openYanase-kernel

**openYanase** is a 32-bit Multiboot-compliant microkernel developed in **C** and **x86 Assembly**. This project focuses on low-level hardware interaction, implementing essential OS components such as segment management (GDT), VGA text-mode manipulation, and an interactive embedded command shell.

---

## 🛠 Technical Architecture

Based on the provided source code, the kernel implements the following layers:

### 1. Bootstrapping (Assembly)
* **Multiboot Compliance**: Uses a standard Multiboot header in `entry.S`, allowing the kernel to be loaded at the **1MB** mark by compatible bootloaders like QEMU or GRUB.
* **Stack Setup**: Initializes a dedicated **8KB** stack space (`resb 8192`) for kernel operations.
* **GDT Flushing**: Includes the `gdt_flush` routine to reload segment registers (`ds`, `es`, `fs`, `gs`, `ss`) and perform a far jump to update the Code Segment (`CS`).

### 2. Core Systems (C)
* **GDT (Global Descriptor Table)**: Implements a **Flat Memory Model** (0 -> 4GB). It defines three primary gates: Null, Code (executable/readable), and Data (readable/writable).
* **VGA Driver**: Directly manages the video buffer at `0xB8000`.
    * Supports **Hardware Cursor** tracking via VGA I/O ports `0x3D4` and `0x3D5`.
    * Features **Screen Scrolling** to handle output exceeding the 25-line limit.
* **Keyboard Driver (Polling)**: Reads raw scancodes (Set 1) from I/O ports `0x60` and `0x64`. It processes user input through a polling mechanism in `main.c`.

### 3. openYanase shell system (conhostman)
The kernel features a built-in Command Line Interface (CLI) to interact with the system:
* **`help`**: Lists available system commands.
* **`clear`**: Resets the VGA buffer and cursor position.
* **`echo [msg]`**: Prints arguments back to the console (handled by `src/applications/echo.c`).
* **`hello`**: Displays a welcome message from the kernel.
* **`reboot`**: Triggers a hardware reset via the 8042 keyboard controller at port `0x64`.

---

## 🚀 Build & Run (WSL / Ubuntu 22.04)

The project is optimized for development on **Windows Subsystem for Linux (WSL)** using the default Ubuntu toolchain (`x86_64-linux-gnu-`).

### 1. Environment Setup
Install the necessary cross-compilation and emulation tools:
\`\`\`bash
sudo apt update
sudo apt install build-essential nasm qemu-system-x86 gcc-multilib
\`\`\`

### 2. Compilation
Use the provided **Makefile** to automate the build process:
\`\`\`bash
make
\`\`\`
The build process involves:
1. Compiling assembly sources with `nasm` (`elf32`).
2. Compiling C sources with `gcc -m32` (using `-ffreestanding` and `-fno-stack-protector`).
3. Linking everything into `mykernel.elf` using the `linker.ld` script.
4. Generating a raw `kernel.bin` using `objcopy`.

### 3. Execution
Run the kernel directly using QEMU:
\`\`\`bash
qemu-system-x86_64 -kernel kernel.bin
\`\`\`

---

## 📁 Project Structure

* `src/entry.S`: Entry point, Multiboot header, and GDT flushing.
* `src/main.c`: Kernel entry point, Shell logic, and Keyboard polling.
* `src/gdt.c`: Global Descriptor Table implementation.
* `src/vga_buffer.c`: VGA driver with scrolling and hardware cursor support.
* `src/keyboard.c`: Scancode-to-ASCII translation.
* `src/applications/echo.c`: Embedded echo application logic.
* `src/include/`: System headers (`io.h`, `types.h`, `vga_buffer.h`).
* `linker.ld`: Memory layout definition (Entry: `start`, Base: `1MB`).
* `makefile`: Automated build system.

---

## 📅 Roadmap

- [x] Multiboot support and GDT initialization.
- [x] VGA Text-mode driver with scrolling and hardware cursor.
- [x] Polling-based Shell with command parsing.
- [ ] Transition from Polling to **Interrupt-driven Keyboard (IDT)**.
- [ ] Implementation of a Physical Memory Manager (PMM).

---

**Author**: pmgdev64  
**License**: MIT
