# ⚛️ PhotonX-OS (High-Performance Optical Microkernel)

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![Platform](https://img.shields.io/badge/platform-ARM64%20%7C%20Xilinx%20Kria-blue)
![License](https://img.shields.io/badge/license-MIT-orange)

**PhotonX-OS** is a specialized, bare-metal microkernel designed to power the next generation of **Hybrid Optical Computing Systems (HOCS)**.

Unlike traditional general-purpose operating systems, PhotonX-OS is engineered with a singular focus: **Ultra-Low Latency Control** for photonic processors and FPGA accelerators.

## 🎯 Project Goal
To eliminate the software bottleneck in optical computing.
While optical processors can perform matrix multiplications at the speed of light O(1), standard kernels (Linux/Windows) introduce unpredictable latency via interrupts and context switching.

**PhotonX-OS** solves this by providing direct, deterministic access to the optical data path (Photonic BUS) on **ARM64 (Xilinx Zynq UltraScale+)** architecture.

## 🏗️ System Architecture

| Layer | Component | Function |
| :--- | :--- | :--- |
| **User Space** | **Web-Container Engine** | Runs VS Code, Python SDK via isolated WASM containers. |
| **Kernel Space** | **PhotonX Microkernel** | Manages Memory, Task Scheduling, and Hardware Abstraction. |
| **Hardware** | **Xilinx Kria KV260** | Controls Lasers (VCSELs), Photodetectors, and FPGA Logic. |

## 🚀 Key Features
* **Minimalist Design:** < 1MB Kernel size. No bloatware.
* **Optical DMA:** Zero-Copy Direct Memory Access for photonic sensor data.
* **Real-Time Scheduler:** Deterministic execution for precise laser synchronization.
* **Hybrid Execution:** Offloads heavy math to the Optical Core, manages logic on ARM64 Cortex-A53.

## 🛠️ Build & Simulation (Development)

We use **QEMU** to simulate the ARM64 environment before deploying to physical FPGA hardware.

### Prerequisites
* Linux (Ubuntu/WSL2)
* `aarch64-linux-gnu-gcc` (Cross Compiler)
* `qemu-system-aarch64` (Emulator)

### How to Run
```bash
# 1. Clone the Repository
git clone [https://github.com/YourUsername/PhotonX-OS.git](https://github.com/YourUsername/PhotonX-OS.git)
cd PhotonX-OS

# 2. Build the Kernel Image
make

# 3. Launch QEMU Emulator
make run
