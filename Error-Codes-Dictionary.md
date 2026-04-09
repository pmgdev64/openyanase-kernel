# Yanase Error Code System (YEC)

## 📌 Overview

Yanase Error Codes (YEC) sử dụng format **3 từ khóa**:

```
[CATEGORY]_[TARGET]_[STATE]
```

* **CATEGORY**: subsystem (CPU, MEM, SYS, USR)
* **TARGET**: thành phần bị lỗi
* **STATE**: trạng thái / bản chất lỗi

---

## 🧠 CATEGORY (Prefix)

| Code | Meaning            |
| ---- | ------------------ |
| CPU  | CPU / Exception    |
| MEM  | Memory system      |
| SYS  | Kernel / System    |
| USR  | Userland / Runtime |

---

## 🎯 TARGET (Object)

### CPU

* DIVIDE
* OPCODE
* PROTECTION
* FAULT
* INTERRUPT

### Memory

* PAGE
* STACK
* HEAP
* POINTER
* ACCESS

### System

* KERNEL
* TASK
* STATE
* DEVICE
* CONFIG
* BOOT
* WATCHDOG
* SYSTEM

### Userland

* PROCESS
* RUNTIME
* BINARY
* APP
* DEPENDENCY

---

## ⚠️ STATE (Status)

* ZERO
* INVALID
* FAILED
* UNHANDLED
* DETECTED
* CORRUPTED
* TERMINATED
* EXITED
* TIMEOUT
* PANIC
* FATAL
* ERROR

---

## 🔥 Standard Error Codes

### CPU Exceptions

```
CPU_DIVIDE_BY_ZERO
CPU_INVALID_OPCODE_ERROR
CPU_GENERAL_PROTECTION_FAULT
CPU_DOUBLE_FAULT_ERROR
CPU_INTERRUPT_UNEXPECTED_ERROR
```

---

### Memory Errors

```
MEM_PAGE_FAULT_UNHANDLED
MEM_NULL_POINTER_DEREFERENCE
MEM_STACK_CORRUPTION_DETECTED
MEM_HEAP_CORRUPTION_DETECTED
MEM_ACCESS_VIOLATION_ERROR
```

---

### System Errors

```
SYS_KERNEL_PANIC_FATAL
SYS_KERNEL_ASSERT_FAILED
SYS_INVALID_STATE_ERROR
SYS_CRITICAL_TASK_EXITED
SYS_DEVICE_INIT_FAILED
SYS_BOOT_SEQUENCE_FAILED
SYS_CONFIG_DATA_CORRUPTED
```

---

### Runtime / Userland

```
USR_PROCESS_CRASHED_FATAL
USR_RUNTIME_DEPENDENCY_FAILED
USR_BINARY_FORMAT_INVALID
USR_APP_EXECUTION_FAILED
USR_PROCESS_TERMINATED_CRITICAL
```

---

### Hard Failure / Watchdog

```
SYS_WATCHDOG_TIMEOUT_FATAL
SYS_SYSTEM_HANG_DETECTED
SYS_HARDWARE_NOT_RESPONDING
SYS_UNRECOVERABLE_ERROR_FATAL
```

---

## 🔊 Beep Mapping (Optional)

| Code                         | Short |
| ---------------------------- | ----- |
| MEM_PAGE_FAULT_UNHANDLED     | PF    |
| CPU_GENERAL_PROTECTION_FAULT | GPF   |
| CPU_DOUBLE_FAULT_ERROR       | DF    |
| SYS_WATCHDOG_TIMEOUT_FATAL   | WD    |
| SYS_SYSTEM_HANG_DETECTED     | SH    |

---

## 🖥 Example Crash Output

```
openYanase Kernel Panic

CODE: 0x0000000E
NAME: MEM_PAGE_FAULT_UNHANDLED
ADDR: 0xDEADBEEF
EIP: 0x0010ABCD
```

---

## 📌 Design Rules

* Use ALL_CAPS
* Always 3 segments
* Keep names concise (≤ 4 words total)
* No kernel name prefix
* Consistent vocabulary across system

---

## 🚀 Future Extensions

* Add severity levels
* Add module-specific categories
* Expand watchdog / hardware error mapping

---
