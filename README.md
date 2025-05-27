# FPGA-Based-Implementation-of-Convolutional-Autoencoder
This project implements a 22-layer Convolutional Autoencoder for real-time image compression and reconstruction on the Intel DE1-SoC FPGA. The design follows a hardware-software co-design approach, with Verilog-based MAC units accelerating 3×3 convolutions and an embedded C program managing control flow and post-processing. The Keras model is quantized using full integer quantization (INT8) for compatibility with FPGA resources.


## Features

- Full 22-layer Convolutional Autoencoder with encoder-decoder symmetry
- Verilog-based hardware acceleration for 3×3 MAC operations
- Embedded C software for ReLU, pooling, upsampling, and activation management
- INT8 quantization of weights, inputs, outputs; INT32 biases
- Memory-mapped I/O using Avalon-MM for FPGA-CPU communication
- Optimized for low latency and efficient resource utilization on Cyclone V FPGA
- UART-based logging and execution monitoring from embedded Linux shell


## Tools Used

- **Intel Quartus Prime** – Verilog synthesis and FPGA design
- **Python + TensorFlow/Keras** – Model training and INT8 quantization
- **DE1-SoC Board** – Deployment and hardware testing
- - **U-Boot / UART Terminal** – Runtime logging and diagnostics via serial interface


## Architecture Diagrams

### System-Level Hardware-Software Co-Design

![System Architecture](images/fpga_hps_architecture.png)
*Figure: Interaction between ARM HPS and Verilog-based PEs on the FPGA.*

### 22-Layer Autoencoder Architecture

![CAE Architecture](images/autoencoder_22_layers.png)
*Figure: Full encoder-decoder pipeline showing convolution, pooling, and upsampling layers.*


