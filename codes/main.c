#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include <sys/time.h>

#define HW_MMAP_BASE   0xFF200000
#define MAP_SIZE       0x10000

#define PE_BASE_0      0x0400
#define PE_BASE_1      0x0800
#define PE_BASE_2      0x0C00
#define PE_BASE_3      0x1000
#define PE_BASE_4      0x1400
#define PE_BASE_5      0x1800
#define PE_BASE_6      0x1C00
#define PE_BASE_7      0x2000
#define PE_BASE_8      0x2400
#define PE_BASE_9      0x2800
#define PE_BASE_10      0x2C00
#define PE_BASE_11      0x3000
#define PE_BASE_12      0x3400
#define PE_BASE_13      0x3800
#define PE_BASE_14      0x3C00
#define PE_BASE_15      0x4000
#define PE_BASE_16      0x4400

#define IMG_SIZE       416
#define PADDING        1
#define PADDED_SIZE    (IMG_SIZE + 2 * PADDING)
#define FILTER_SIZE    3

#define NUM_FILTERS_L1 32
#define NUM_FILTERS_L2 64
#define NUM_FILTERS_L3 128
#define NUM_FILTERS_L4 256
#define NUM_FILTERS_L5 512

#define OUTPUT_L1_SIZE (IMG_SIZE / 2)        // 208
#define OUTPUT_L2_SIZE (OUTPUT_L1_SIZE / 2)  // 104
#define OUTPUT_L3_SIZE (OUTPUT_L2_SIZE / 2)  
#define OUTPUT_L4_SIZE (OUTPUT_L3_SIZE / 2)
#define OUTPUT_L5_SIZE (OUTPUT_L4_SIZE / 2)
#define NUM_PES        17

#define DEC_NUM_FILTERS_L1 512
#define DEC_NUM_FILTERS_L2 256
#define DEC_NUM_FILTERS_L3 128
#define DEC_NUM_FILTERS_L4 64
#define DEC_NUM_FILTERS_L5 32

#define DEC_LATENT_SIZE    13

#define DEC_OUT_SIZE_1     DEC_LATENT_SIZE
#define DEC_UPSAMPLED_SIZE_1 (DEC_OUT_SIZE_1 * 2)
#define DEC_OUT_SIZE_2 DEC_UPSAMPLED_SIZE_1      // 26
#define DEC_UPSAMPLED_SIZE_2 (DEC_OUT_SIZE_2 * 2) // 52
#define DEC_OUT_SIZE_3 DEC_UPSAMPLED_SIZE_2
#define DEC_UPSAMPLED_SIZE_3 (DEC_OUT_SIZE_3 * 2) // 104
#define DEC_OUT_SIZE_4 DEC_UPSAMPLED_SIZE_3
#define DEC_UPSAMPLED_SIZE_4 (DEC_OUT_SIZE_4 * 2) // 208
#define DEC_OUT_SIZE_5 DEC_UPSAMPLED_SIZE_4
#define DEC_UPSAMPLED_SIZE_5 (DEC_OUT_SIZE_5 * 2) // 416

#define FINAL_NUM_OUTPUTS 1
#define FINAL_NUM_INPUTS  DEC_NUM_FILTERS_L5  // 32
#define FINAL_FILTER_SIZE FILTER_SIZE         // 3
#define FINAL_OUTPUT_SIZE 416

// Load 416x416 image
int load_image_from_file(const char *filename, int8_t image[IMG_SIZE][IMG_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open input image file");
        return -1;
    }
    for (int i = 0; i < IMG_SIZE; i++) {
        for (int j = 0; j < IMG_SIZE; j++) {
            int val;
            if (fscanf(fp, "%d", &val) != 1) {
                fclose(fp);
                fprintf(stderr, "Invalid format in input.txt\n");
                return -1;
            }
            image[i][j] = (int8_t)val;
        }
    }
    fclose(fp);
    return 0;
}


// Load 32 filters of 3x3
int load_filters_layer1(const char *filename, int8_t filters[NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open filter file");
        return -1;
    }

    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < NUM_FILTERS_L1; f++) {
        for (int i = 0; i < FILTER_SIZE; i++) {
            for (int j = 0; j < FILTER_SIZE; j++) {
                int val;
                if (fscanf(fp, "%d", &val) != 1) {
                    fclose(fp);
                    fprintf(stderr, "Invalid format in %s\n", filename);
                    return -1;
                }
                filters[f][i][j] = (int8_t)val;
            }
        }
    }

    fclose(fp);
    return 0;
}

int load_biases_layer1(const char *filename, int32_t bias[NUM_FILTERS_L1]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open bias file");
        return -1;
    }

    printf("Reading int32 biases from %s\n", filename);
    for (int i = 0; i < NUM_FILTERS_L1; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fclose(fp);
            fprintf(stderr, "Invalid format in %s\n", filename);
            return -1;
        }
        bias[i] = (int32_t)val;
    }

    fclose(fp);
    return 0;
}



// Load 64 filters of 3x3x32 for layer 2
int load_filters_layer2(const char *filename, int8_t filters[NUM_FILTERS_L2][NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open filters2.txt");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);

    for (int f = 0; f < NUM_FILTERS_L2; f++) {
        for (int c = 0; c < NUM_FILTERS_L1; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fclose(fp);
                        fprintf(stderr, "Invalid format in filters2.txt at filter %d, channel %d\n", f, c);
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

int load_biases_layer2(const char *filename, int32_t bias[NUM_FILTERS_L2]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open biases2.txt");
        return -1;
    }

    printf("Reading int32 biases from %s\n", filename);
    for (int i = 0; i < NUM_FILTERS_L2; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fclose(fp);
            fprintf(stderr, "Invalid format in biases2.txt\n");
            return -1;
        }
        bias[i] = (int32_t)val;
    }

    fclose(fp);
    return 0;
}


int load_filters_layer3(const char *filename, int8_t filters[NUM_FILTERS_L3][NUM_FILTERS_L2][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open filters3.txt");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < NUM_FILTERS_L3; f++) {
        for (int c = 0; c < NUM_FILTERS_L2; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fclose(fp);
                        fprintf(stderr, "Invalid format in filters3.txt\n");
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

int load_biases_layer3(const char *filename, int32_t bias[NUM_FILTERS_L3]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open bias3.txt");
        return -1;
    }
    printf("Reading int32 biases from %s\n", filename);
    for (int i = 0; i < NUM_FILTERS_L3; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fclose(fp);
            fprintf(stderr, "Invalid format in bias3.txt at index %d\n", i);
            return -1;
        }
        bias[i] = (int32_t)val;
    }
    fclose(fp);
    return 0;
}



int load_filters_layer4(const char *filename, int8_t filters[NUM_FILTERS_L4][NUM_FILTERS_L3][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open filters4.txt");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < NUM_FILTERS_L4; f++) {
        for (int c = 0; c < NUM_FILTERS_L3; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fclose(fp);
                        fprintf(stderr, "Invalid format in filters4.txt at filter %d, channel %d, (%d,%d)\n", f, c, i, j);
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}


int load_biases_layer4(const char *filename, int32_t bias[NUM_FILTERS_L4]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open bias4.txt");
        return -1;
    }
    printf("Reading int32 biases from %s\n", filename);
    for (int i = 0; i < NUM_FILTERS_L4; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fclose(fp);
            fprintf(stderr, "Invalid format in bias4.txt at index %d\n", i);
            return -1;
        }
        bias[i] = (int32_t)val;
    }
    fclose(fp);
    return 0;
}


int load_filters_layer5(const char *filename, int8_t filters[NUM_FILTERS_L5][NUM_FILTERS_L4][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open filters5.txt");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < NUM_FILTERS_L5; f++) {
        for (int c = 0; c < NUM_FILTERS_L4; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fclose(fp);
                        fprintf(stderr, "Invalid format in filters5.txt at filter %d, channel %d, (%d,%d)\n", f, c, i, j);
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}


int load_biases_layer5(const char *filename, int32_t bias[NUM_FILTERS_L5]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open bias5.txt");
        return -1;
    }
    printf("Reading int32 biases from %s\n", filename);
    for (int i = 0; i < NUM_FILTERS_L5; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fclose(fp);
            fprintf(stderr, "Invalid format in bias5.txt at index %d\n", i);
            return -1;
        }
        bias[i] = (int32_t)val;
    }
    fclose(fp);
    return 0;
}

int load_filters_decoder1(const char *filename,
                          int8_t filters[DEC_NUM_FILTERS_L1][DEC_NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder filter file");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < DEC_NUM_FILTERS_L1; f++) {
        for (int c = 0; c < DEC_NUM_FILTERS_L1; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fclose(fp);
                        fprintf(stderr, "Error in decoder filter file format\n");
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }
    fclose(fp);
    return 0;
}

int load_biases_decoder1(const char *filename, int32_t bias[DEC_NUM_FILTERS_L1]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder bias file");
        return -1;
    }
    printf("Reading int8 biases from %s\n", filename);
    for (int i = 0; i < DEC_NUM_FILTERS_L1; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fclose(fp);
            fprintf(stderr, "Error in decoder bias file format\n");
            return -1;
        }
        bias[i] = (int32_t)val;
    }
    fclose(fp);
    return 0;
}

int load_filters_decoder2(const char *filename,
                          int8_t filters[DEC_NUM_FILTERS_L2][DEC_NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder filters file");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < DEC_NUM_FILTERS_L2; f++) {
        for (int c = 0; c < DEC_NUM_FILTERS_L1; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int temp;
                    if (fscanf(fp, "%d", &temp) != 1) {
                        fprintf(stderr, "Invalid format at filter %d, channel %d, (%d,%d)\n", f, c, i, j);
                        fclose(fp);
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)temp;
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

int load_biases_decoder2(const char *filename, int32_t bias[DEC_NUM_FILTERS_L2]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder bias file");
        return -1;
    }
    printf("Reading int8 biases from %s\n", filename);

    for (int i = 0; i < DEC_NUM_FILTERS_L2; i++) {
        int temp;
        if (fscanf(fp, "%d", &temp) != 1) {
            fprintf(stderr, "Invalid format at bias %d\n", i);
            fclose(fp);
            return -1;
        }
        bias[i] = (int32_t)temp;
    }

    fclose(fp);
    return 0;
}

int load_filters_decoder3(const char *filename,
                          int8_t filters[DEC_NUM_FILTERS_L3][DEC_NUM_FILTERS_L2][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder filters file");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < DEC_NUM_FILTERS_L3; f++) {
        for (int c = 0; c < DEC_NUM_FILTERS_L2; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int temp;
                    if (fscanf(fp, "%d", &temp) != 1) {
                        fprintf(stderr, "Invalid format at filter %d, channel %d, (%d,%d)\n", f, c, i, j);
                        fclose(fp);
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)temp;
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

int load_biases_decoder3(const char *filename, int32_t bias[DEC_NUM_FILTERS_L3]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder bias file");
        return -1;
    }
    printf("Reading int8 biases from %s\n", filename);

    for (int i = 0; i < DEC_NUM_FILTERS_L3; i++) {
        int temp;
        if (fscanf(fp, "%d", &temp) != 1) {
            fprintf(stderr, "Invalid format at bias %d\n", i);
            fclose(fp);
            return -1;
        }
        bias[i] = (int32_t)temp;
    }

    fclose(fp);
    return 0;
}

int load_filters_decoder4(const char *filename,
                          int8_t filters[DEC_NUM_FILTERS_L4][DEC_NUM_FILTERS_L3][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder4 filters file");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < DEC_NUM_FILTERS_L4; f++) {
        for (int c = 0; c < DEC_NUM_FILTERS_L3; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fprintf(stderr, "Invalid format in filters4.txt at f=%d c=%d (%d,%d)\n", f, c, i, j);
                        fclose(fp);
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }

    fclose(fp);
    return 0;
}



int load_biases_decoder4(const char *filename, int32_t bias[DEC_NUM_FILTERS_L4]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder4 bias file");
        return -1;
    }
    printf("Reading int8 biases from %s\n", filename);

    for (int i = 0; i < DEC_NUM_FILTERS_L4; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fprintf(stderr, "Invalid format in decoder bias4.txt at index %d\n", i);
            fclose(fp);
            return -1;
        }
        bias[i] = (int32_t)val;
    }

    fclose(fp);
    return 0;
}

int load_filters_decoder5(const char *filename,
                          int8_t filters[DEC_NUM_FILTERS_L5][DEC_NUM_FILTERS_L4][FILTER_SIZE][FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder5 filters file");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < DEC_NUM_FILTERS_L5; f++) {
        for (int c = 0; c < DEC_NUM_FILTERS_L4; c++) {
            for (int i = 0; i < FILTER_SIZE; i++) {
                for (int j = 0; j < FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fprintf(stderr, "Invalid format in filters5.txt at f=%d c=%d (%d,%d)\n", f, c, i, j);
                        fclose(fp);
                        return -1;
                    }
                    filters[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

int load_biases_decoder5(const char *filename, int32_t bias[DEC_NUM_FILTERS_L5]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open decoder5 bias file");
        return -1;
    }
    printf("Reading int8 biases from %s\n", filename);

    for (int i = 0; i < DEC_NUM_FILTERS_L5; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fprintf(stderr, "Invalid format in decoder bias5.txt at index %d\n", i);
            fclose(fp);
            return -1;
        }
        bias[i] = (int32_t)val;
    }

    fclose(fp);
    return 0;
}


int load_final_conv_filter(const char *filename, int8_t filter[FINAL_NUM_OUTPUTS][FINAL_NUM_INPUTS][FINAL_FILTER_SIZE][FINAL_FILTER_SIZE]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open final conv filter file");
        return -1;
    }
    printf("Reading int8 filters from %s\n", filename);
    for (int f = 0; f < FINAL_NUM_OUTPUTS; f++) {
        for (int c = 0; c < FINAL_NUM_INPUTS; c++) {
            for (int i = 0; i < FINAL_FILTER_SIZE; i++) {
                for (int j = 0; j < FINAL_FILTER_SIZE; j++) {
                    int val;
                    if (fscanf(fp, "%d", &val) != 1) {
                        fprintf(stderr, "Invalid format in final conv filter at (%d,%d,%d,%d)\n", f, c, i, j);
                        fclose(fp);
                        return -1;
                    }
                    filter[f][c][i][j] = (int8_t)val;
                }
            }
        }
    }

    fclose(fp);
    return 0;
}

int load_final_conv_bias(const char *filename, int32_t bias[FINAL_NUM_OUTPUTS]) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open final conv bias file");
        return -1;
    }
    printf("Reading int8 biases from %s\n", filename);

    for (int i = 0; i < FINAL_NUM_OUTPUTS; i++) {
        int val;
        if (fscanf(fp, "%d", &val) != 1) {
            fprintf(stderr, "Invalid format in final conv bias at index %d\n", i);
            fclose(fp);
            return -1;
        }
        bias[i] = (int32_t)val;
    }

    fclose(fp);
    return 0;
}




void* map_physical_memory(off_t base, size_t span, int *fd) {
    *fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (*fd < 0) {
        perror("ERROR: could not open /dev/mem");
        return NULL;
    }
    void *virtual_base = mmap(NULL, span, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, base);
    if (virtual_base == MAP_FAILED) {
        perror("ERROR: mmap() failed");
        close(*fd);
        return NULL;
    }
    return virtual_base;
}

void unmap_physical_memory(void *virtual_base, size_t span, int fd) {
    munmap(virtual_base, span);
    close(fd);
}


void perform_layer1_convolution(
    void *virtual_base,
    int8_t padded[PADDED_SIZE][PADDED_SIZE],
    int8_t filters[NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE],
    int8_t output[NUM_FILTERS_L1][IMG_SIZE][IMG_SIZE],
    int32_t bias[NUM_FILTERS_L1]
) {
    printf("Starting Encoder Layer 1 convolution...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < NUM_FILTERS_L1; f += NUM_PES) {
        int pes_to_use = (NUM_FILTERS_L1 - f < NUM_PES) ? (NUM_FILTERS_L1 - f) : NUM_PES;

        for (int i = 0; i < IMG_SIZE; i++) {
            for (int j = 0; j < IMG_SIZE; j++) {

                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);

                    // Write 3x3 image patch
                    for (int m = 0; m < FILTER_SIZE; m++) {
                        for (int n = 0; n < FILTER_SIZE; n++) {
                            int8_t pix = padded[i + m][j + n];
                            *(volatile uint32_t *)((char *)pe_base + ((m * FILTER_SIZE + n) * 4)) = pix;
                        }
                    }

                    // Write 3x3 filter weights
                    for (int k = 0; k < 9; k++) {
                        *(volatile uint32_t *)((char *)pe_base + ((9 + k) * 4)) = filters[f + pe][k / 3][k % 3];
                    }

                    // Start computation
                    *(volatile uint32_t *)((char *)pe_base + (18 * 4)) = 1;
                    //printf("Started PE %d for filter %d at (%d,%d)\n", pe, f + pe, i, j);

                }

                // Wait for done signal and fetch result
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    while ((*(volatile uint32_t *)((char *)pe_base + (31 * 4))) == 0);

                    *(volatile uint32_t *)((char *)pe_base + (18 * 4)) = 0;  // Clear start

                    int32_t result = *(volatile uint32_t *)((char *)pe_base + (30 * 4));
                    result += bias[f + pe];  // Add bias

                    // Compute MSB position
                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((result >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    // Shift and saturate to int8 range
                    int32_t shifted = result >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    // Apply ReLU
                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Encoder Layer 1 convolution complete...\n");
}


void max_pool_layer1(int8_t input[NUM_FILTERS_L1][IMG_SIZE][IMG_SIZE],
                     int8_t pooled_output[NUM_FILTERS_L1][OUTPUT_L1_SIZE][OUTPUT_L1_SIZE]) {
    printf("Starting Encoder Layer 1 max pooling...\n");
    for (int f = 0; f < NUM_FILTERS_L1; f++) {
        for (int i = 0; i < OUTPUT_L1_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L1_SIZE; j++) {
                int32_t max_val = input[f][2*i][2*j];
                for (int m = 0; m < 2; m++) {
                    for (int n = 0; n < 2; n++) {
                        int32_t val = input[f][2*i + m][2*j + n];
                        if (val > max_val) max_val = val;
                    }
                }
                pooled_output[f][i][j] = max_val;
            }
        }
    }
    printf("Encoder Layer 1 max pooling complete...\n");
    
}

void write_output_L1(const char *filename, int8_t output[NUM_FILTERS_L1][OUTPUT_L1_SIZE][OUTPUT_L1_SIZE]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open output file");
        return;
    }

    for (int f = 0; f < NUM_FILTERS_L1; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < OUTPUT_L1_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L1_SIZE; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void perform_layer2_convolution(
    void *virtual_base,
    int8_t input[NUM_FILTERS_L1][OUTPUT_L1_SIZE][OUTPUT_L1_SIZE],
    int8_t filters[NUM_FILTERS_L2][NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE],
    int8_t output[NUM_FILTERS_L2][OUTPUT_L1_SIZE][OUTPUT_L1_SIZE],
    int32_t bias[NUM_FILTERS_L2]
) {
    printf("Starting Encoder Layer 2 convolution...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < NUM_FILTERS_L2; f += NUM_PES) {
        int pes_to_use = (NUM_FILTERS_L2 - f < NUM_PES) ? (NUM_FILTERS_L2 - f) : NUM_PES;

        for (int i = 0; i < OUTPUT_L1_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L1_SIZE; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < NUM_FILTERS_L1; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int x = i + m - 1;
                                int y = j + n - 1;
                                int8_t val = (x >= 0 && x < OUTPUT_L1_SIZE && y >= 0 && y < OUTPUT_L1_SIZE)
                                              ? input[c][x][y] : 0;
                                *(volatile uint32_t *)((char *)pe_base + ((m * FILTER_SIZE + n) * 4)) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][m][n];
                                *(volatile uint32_t *)((char *)pe_base + ((9 + m * FILTER_SIZE + n) * 4)) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        //printf("Started PE %d for filter %d at (%d,%d)\n", pe, f + pe, i, j);

                        while ((*(volatile uint32_t *)((char *)pe_base + 31 * 4)) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    sum += bias[f + pe];

                    // Dynamic shift based on MSB position
                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Encoder Layer 2 convolution complete...\n");
}




void max_pool_layer2(int8_t input[NUM_FILTERS_L2][OUTPUT_L1_SIZE][OUTPUT_L1_SIZE],
                     int8_t output[NUM_FILTERS_L2][OUTPUT_L2_SIZE][OUTPUT_L2_SIZE]) {

    printf("Starting Encoder Layer 2 max pooling...\n");

    for (int f = 0; f < NUM_FILTERS_L2; f++) {
        for (int i = 0; i < OUTPUT_L2_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L2_SIZE; j++) {
                int32_t max_val = input[f][2*i][2*j];
                for (int m = 0; m < 2; m++) {
                    for (int n = 0; n < 2; n++) {
                        int32_t val = input[f][2*i + m][2*j + n];
                        if (val > max_val) max_val = val;
                    }
                }
                output[f][i][j] = max_val;
            }
        }
    }

    printf("Encoder Layer 2 max pooling complete...\n");
}

void write_output_L2(const char *filename, int8_t output[NUM_FILTERS_L2][OUTPUT_L2_SIZE][OUTPUT_L2_SIZE]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open output file");
        return;
    }

    for (int f = 0; f < NUM_FILTERS_L2; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < OUTPUT_L2_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L2_SIZE; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void perform_layer3_convolution(
    void *virtual_base,
    int8_t input[NUM_FILTERS_L2][OUTPUT_L2_SIZE][OUTPUT_L2_SIZE],
    int8_t filters[NUM_FILTERS_L3][NUM_FILTERS_L2][FILTER_SIZE][FILTER_SIZE],
    int8_t output[NUM_FILTERS_L3][OUTPUT_L2_SIZE][OUTPUT_L2_SIZE],
    int32_t bias[NUM_FILTERS_L3]
) {
    printf("Starting Encoder Layer 3 convolution...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < NUM_FILTERS_L3; f += NUM_PES) {
        int pes_to_use = (NUM_FILTERS_L3 - f < NUM_PES) ? (NUM_FILTERS_L3 - f) : NUM_PES;

        for (int i = 0; i < OUTPUT_L2_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L2_SIZE; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < NUM_FILTERS_L2; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int x = i + m - 1;
                                int y = j + n - 1;
                                int8_t val = (x >= 0 && x < OUTPUT_L2_SIZE && y >= 0 && y < OUTPUT_L2_SIZE)
                                              ? input[c][x][y] : 0;
                                *(volatile uint32_t *)((char *)pe_base + (m * FILTER_SIZE + n) * 4) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][m][n];
                                *(volatile uint32_t *)((char *)pe_base + (9 + m * FILTER_SIZE + n) * 4) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        //printf("Started PE %d for filter %d at (%d,%d)\n", pe, f + pe, i, j);

                        while ((*(volatile uint32_t *)((char *)pe_base + 31 * 4)) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    sum += bias[f + pe];

                    // Compute dynamic shift to reduce to int8
                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Encoder Layer 3 convolution complete...\n");
}



void max_pool_layer3(int8_t input[NUM_FILTERS_L3][OUTPUT_L2_SIZE][OUTPUT_L2_SIZE],
                     int8_t output[NUM_FILTERS_L3][OUTPUT_L3_SIZE][OUTPUT_L3_SIZE]) {

    printf("Starting Encoder Layer 3 max pooling...\n");

    for (int f = 0; f < NUM_FILTERS_L3; f++) {
        for (int i = 0; i < OUTPUT_L3_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L3_SIZE; j++) {
                int32_t max_val = input[f][2 * i][2 * j];
                for (int m = 0; m < 2; m++) {
                    for (int n = 0; n < 2; n++) {
                        int32_t val = input[f][2 * i + m][2 * j + n];
                        if (val > max_val) max_val = val;
                    }
                }
                output[f][i][j] = max_val;
            }
        }
    }

    printf("Encoder Layer 3 max pooling complete...\n");
}

void write_output_L3(const char *filename, int8_t output[NUM_FILTERS_L3][OUTPUT_L3_SIZE][OUTPUT_L3_SIZE]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open output file");
        return;
    }

    for (int f = 0; f < NUM_FILTERS_L3; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < OUTPUT_L3_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L3_SIZE; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}


void perform_layer4_convolution(
    void *virtual_base,
    int8_t input[NUM_FILTERS_L3][OUTPUT_L3_SIZE][OUTPUT_L3_SIZE],
    int8_t filters[NUM_FILTERS_L4][NUM_FILTERS_L3][FILTER_SIZE][FILTER_SIZE],
    int8_t output[NUM_FILTERS_L4][OUTPUT_L3_SIZE][OUTPUT_L3_SIZE],
    int32_t bias[NUM_FILTERS_L4]
) {
    printf("Starting Encoder Layer 4 convolution...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < NUM_FILTERS_L4; f += NUM_PES) {
        int pes_to_use = (NUM_FILTERS_L4 - f < NUM_PES) ? (NUM_FILTERS_L4 - f) : NUM_PES;

        for (int i = 0; i < OUTPUT_L3_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L3_SIZE; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < NUM_FILTERS_L3; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int x = i + m - 1;
                                int y = j + n - 1;
                                int8_t val = (x >= 0 && x < OUTPUT_L3_SIZE && y >= 0 && y < OUTPUT_L3_SIZE)
                                             ? input[c][x][y] : 0;
                                *(volatile uint32_t *)((char *)pe_base + (m * FILTER_SIZE + n) * 4) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][m][n];
                                *(volatile uint32_t *)((char *)pe_base + (9 + m * FILTER_SIZE + n) * 4) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        //printf("Started PE %d for filter %d at (%d,%d)\n", pe, f + pe, i, j);

                        while ((*(volatile uint32_t *)((char *)pe_base + 31 * 4)) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    // Add bias
                    sum += bias[f + pe];

                    // Compute shift to fit into int8
                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    // Apply ReLU
                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Encoder Layer 4 convolution complete...\n");
}



void max_pool_layer4(int8_t input[NUM_FILTERS_L4][OUTPUT_L3_SIZE][OUTPUT_L3_SIZE],
                     int8_t output[NUM_FILTERS_L4][OUTPUT_L4_SIZE][OUTPUT_L4_SIZE]) {

    printf("Starting Encoder Layer 4 max pooling...\n");

    for (int f = 0; f < NUM_FILTERS_L4; f++) {
        for (int i = 0; i < OUTPUT_L4_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L4_SIZE; j++) {
                int32_t max_val = input[f][2 * i][2 * j];
                for (int m = 0; m < 2; m++) {
                    for (int n = 0; n < 2; n++) {
                        int32_t val = input[f][2 * i + m][2 * j + n];
                        if (val > max_val) max_val = val;
                    }
                }
                output[f][i][j] = max_val;
            }
        }
    }

    printf("Encoder Layer 4 max pooling complete...\n");
}

void write_output_L4(const char *filename, int8_t output[NUM_FILTERS_L4][OUTPUT_L4_SIZE][OUTPUT_L4_SIZE]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open output file");
        return;
    }

    for (int f = 0; f < NUM_FILTERS_L4; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < OUTPUT_L4_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L4_SIZE; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}


void perform_layer5_convolution(
    void *virtual_base,
    int8_t input[NUM_FILTERS_L4][OUTPUT_L4_SIZE][OUTPUT_L4_SIZE],
    int8_t filters[NUM_FILTERS_L5][NUM_FILTERS_L4][FILTER_SIZE][FILTER_SIZE],
    int8_t output[NUM_FILTERS_L5][OUTPUT_L4_SIZE][OUTPUT_L4_SIZE],
    int32_t bias[NUM_FILTERS_L5]
) {
    printf("Starting Encoder Layer 5 convolution...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < NUM_FILTERS_L5; f += NUM_PES) {
        int pes_to_use = (NUM_FILTERS_L5 - f < NUM_PES) ? (NUM_FILTERS_L5 - f) : NUM_PES;

        for (int i = 0; i < OUTPUT_L4_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L4_SIZE; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < NUM_FILTERS_L4; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int x = i + m - 1;
                                int y = j + n - 1;
                                int8_t val = (x >= 0 && x < OUTPUT_L4_SIZE && y >= 0 && y < OUTPUT_L4_SIZE)
                                             ? input[c][x][y] : 0;
                                *(volatile uint32_t *)((char *)pe_base + (m * FILTER_SIZE + n) * 4) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][m][n];
                                *(volatile uint32_t *)((char *)pe_base + (9 + m * FILTER_SIZE + n) * 4) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        //printf("Started PE %d for filter %d at (%d,%d)\n", pe, f + pe, i, j);

                        while ((*(volatile uint32_t *)((char *)pe_base + 31 * 4)) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    // Add bias
                    sum += bias[f + pe];

                    // Dynamic right shift to int8 range
                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    // Apply ReLU
                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Encoder Layer 5 convolution complete...\n");
}



void max_pool_layer5(int8_t input[NUM_FILTERS_L5][OUTPUT_L4_SIZE][OUTPUT_L4_SIZE],
                     int8_t output[NUM_FILTERS_L5][OUTPUT_L5_SIZE][OUTPUT_L5_SIZE]) {

    printf("Starting Encoder Layer 5 max pooling...\n");

    for (int f = 0; f < NUM_FILTERS_L5; f++) {
        for (int i = 0; i < OUTPUT_L5_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L5_SIZE; j++) {
                int32_t max_val = input[f][2 * i][2 * j];
                for (int m = 0; m < 2; m++) {
                    for (int n = 0; n < 2; n++) {
                        int32_t val = input[f][2 * i + m][2 * j + n];
                        if (val > max_val) max_val = val;
                    }
                }
                output[f][i][j] = max_val;
            }
        }
    }

    printf("Encoder Layer 5 max pooling complete...\n");
}

void write_output_L5(const char *filename, int8_t output[NUM_FILTERS_L5][OUTPUT_L5_SIZE][OUTPUT_L5_SIZE]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open output file");
        return;
    }

    for (int f = 0; f < NUM_FILTERS_L5; f++) {
        //fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < OUTPUT_L5_SIZE; i++) {
            for (int j = 0; j < OUTPUT_L5_SIZE; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void decoder_conv2dtranspose1(
    void *virtual_base,
    int8_t input[DEC_NUM_FILTERS_L1][DEC_LATENT_SIZE][DEC_LATENT_SIZE],
    int8_t filters[DEC_NUM_FILTERS_L1][DEC_NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE],
    int32_t bias[DEC_NUM_FILTERS_L1],
    int8_t output[DEC_NUM_FILTERS_L1][DEC_OUT_SIZE_1][DEC_OUT_SIZE_1]
) {
    printf("Starting Decoder Layer 1 convolution...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < DEC_NUM_FILTERS_L1; f += NUM_PES) {
        int pes_to_use = (DEC_NUM_FILTERS_L1 - f < NUM_PES) ? (DEC_NUM_FILTERS_L1 - f) : NUM_PES;

        for (int i = 1; i < DEC_OUT_SIZE_1 - 1; i++) {
            for (int j = 1; j < DEC_OUT_SIZE_1 - 1; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < DEC_NUM_FILTERS_L1; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int row = i + m - 1;
                                int col = j + n - 1;
                                int8_t val = (row >= 0 && row < DEC_LATENT_SIZE && col >= 0 && col < DEC_LATENT_SIZE)
                                              ? input[c][row][col] : 0;
                                *(volatile uint32_t *)((char *)pe_base + ((m * FILTER_SIZE + n) * 4)) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][FILTER_SIZE - 1 - m][FILTER_SIZE - 1 - n];
                                *(volatile uint32_t *)((char *)pe_base + ((9 + m * FILTER_SIZE + n) * 4)) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        //printf("Started PE %d for filter %d at (%d,%d)\n", pe, f + pe, i, j);

                        while ((*(volatile uint32_t *)((char *)pe_base + 31 * 4)) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    sum += bias[f + pe];

                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Decoder Layer 1 convolution...\n");
}



void decoder_upsample2d_1(int8_t input[DEC_NUM_FILTERS_L1][DEC_OUT_SIZE_1][DEC_OUT_SIZE_1],
                          int8_t output[DEC_NUM_FILTERS_L1][DEC_UPSAMPLED_SIZE_1][DEC_UPSAMPLED_SIZE_1]) {
    printf("Starting Decoder Upsampling Layer 1...\n");
    for (int c = 0; c < DEC_NUM_FILTERS_L1; c++) {
        for (int i = 0; i < DEC_OUT_SIZE_1; i++) {
            for (int j = 0; j < DEC_OUT_SIZE_1; j++) {
                int val = input[c][i][j];
                output[c][2 * i][2 * j] = val;
                output[c][2 * i + 1][2 * j] = val;
                output[c][2 * i][2 * j + 1] = val;
                output[c][2 * i + 1][2 * j + 1] = val;
            }
        }
    }
    printf("Decoder Upsampling Layer 1 complete...\n");
}


void write_output_decoder_L1(const char *filename,
                              int8_t output[DEC_NUM_FILTERS_L1][DEC_UPSAMPLED_SIZE_1][DEC_UPSAMPLED_SIZE_1]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open decoder output file");
        return;
    }

    for (int f = 0; f < DEC_NUM_FILTERS_L1; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < DEC_UPSAMPLED_SIZE_1; i++) {
            for (int j = 0; j < DEC_UPSAMPLED_SIZE_1; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void decoder_conv2dtranspose2(
    void *virtual_base,
    int8_t input[DEC_NUM_FILTERS_L1][DEC_UPSAMPLED_SIZE_1][DEC_UPSAMPLED_SIZE_1],
    int8_t filters[DEC_NUM_FILTERS_L2][DEC_NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE],
    int32_t bias[DEC_NUM_FILTERS_L2],
    int8_t output[DEC_NUM_FILTERS_L2][DEC_OUT_SIZE_2][DEC_OUT_SIZE_2]
) {
    printf("Starting Decoder Conv2DTranspose Layer 2...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < DEC_NUM_FILTERS_L2; f += NUM_PES) {
        int pes_to_use = (DEC_NUM_FILTERS_L2 - f < NUM_PES) ? (DEC_NUM_FILTERS_L2 - f) : NUM_PES;

        for (int i = 1; i < DEC_OUT_SIZE_2 - 1; i++) {
            for (int j = 1; j < DEC_OUT_SIZE_2 - 1; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < DEC_NUM_FILTERS_L1; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int row = i + m - 1;
                                int col = j + n - 1;
                                int8_t val = (row >= 0 && row < DEC_UPSAMPLED_SIZE_1 && col >= 0 && col < DEC_UPSAMPLED_SIZE_1)
                                             ? input[c][row][col] : 0;
                                *(volatile uint32_t *)((char *)pe_base + ((m * FILTER_SIZE + n) * 4)) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][FILTER_SIZE - 1 - m][FILTER_SIZE - 1 - n]; // flipped
                                *(volatile uint32_t *)((char *)pe_base + ((9 + m * FILTER_SIZE + n) * 4)) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        while ((*(volatile uint32_t *)((char *)pe_base + 31 * 4)) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    sum += bias[f + pe];

                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Decoder Conv2DTranspose Layer 2 complete...\n");
}


void decoder_upsample2d_2(int8_t input[DEC_NUM_FILTERS_L2][DEC_OUT_SIZE_2][DEC_OUT_SIZE_2],
                          int8_t output[DEC_NUM_FILTERS_L2][DEC_UPSAMPLED_SIZE_2][DEC_UPSAMPLED_SIZE_2]) {
    printf("Starting Decoder Upsampling Layer 2...\n");

    for (int c = 0; c < DEC_NUM_FILTERS_L2; c++) {
        for (int i = 0; i < DEC_OUT_SIZE_2; i++) {
            for (int j = 0; j < DEC_OUT_SIZE_2; j++) {
                int val = input[c][i][j];
                output[c][2 * i][2 * j] = val;
                output[c][2 * i + 1][2 * j] = val;
                output[c][2 * i][2 * j + 1] = val;
                output[c][2 * i + 1][2 * j + 1] = val;
            }
        }
    }

    printf("Decoder Upsampling Layer 2 complete...\n");
}

void write_output_decoder_L2(const char *filename,
                             int8_t output[DEC_NUM_FILTERS_L2][DEC_UPSAMPLED_SIZE_2][DEC_UPSAMPLED_SIZE_2]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open decoder output file");
        return;
    }

    for (int f = 0; f < DEC_NUM_FILTERS_L2; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < DEC_UPSAMPLED_SIZE_2; i++) {
            for (int j = 0; j < DEC_UPSAMPLED_SIZE_2; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void decoder_conv2dtranspose3(
    void *virtual_base,
    int8_t input[DEC_NUM_FILTERS_L2][DEC_UPSAMPLED_SIZE_2][DEC_UPSAMPLED_SIZE_2],
    int8_t filters[DEC_NUM_FILTERS_L3][DEC_NUM_FILTERS_L2][FILTER_SIZE][FILTER_SIZE],
    int32_t bias[DEC_NUM_FILTERS_L3],
    int8_t output[DEC_NUM_FILTERS_L3][DEC_OUT_SIZE_3][DEC_OUT_SIZE_3]
) {
    printf("Starting Decoder Conv2DTranspose Layer 3...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < DEC_NUM_FILTERS_L3; f += NUM_PES) {
        int pes_to_use = (DEC_NUM_FILTERS_L3 - f < NUM_PES) ? (DEC_NUM_FILTERS_L3 - f) : NUM_PES;

        for (int i = 1; i < DEC_OUT_SIZE_3 - 1; i++) {
            for (int j = 1; j < DEC_OUT_SIZE_3 - 1; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < DEC_NUM_FILTERS_L2; c++) {
                        // Push 3x3 input patch
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int row = i + m - 1;
                                int col = j + n - 1;
                                int8_t val = (row >= 0 && row < DEC_UPSAMPLED_SIZE_2 && col >= 0 && col < DEC_UPSAMPLED_SIZE_2)
                                              ? input[c][row][col] : 0;
                                *(volatile uint32_t *)((char *)pe_base + ((m * FILTER_SIZE + n) * 4)) = val;
                            }
                        }

                        // Push flipped filter weights
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t weight = filters[f + pe][c][FILTER_SIZE - 1 - m][FILTER_SIZE - 1 - n];
                                *(volatile uint32_t *)((char *)pe_base + ((9 + m * FILTER_SIZE + n) * 4)) = weight;
                            }
                        }

                        // Start MAC
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;

                        // Wait for done
                        while (*(volatile uint32_t *)((char *)pe_base + 31 * 4) == 0);

                        // Read and accumulate result
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    // Add bias
                    sum += bias[f + pe];

                    // Dynamic shift to fit into int8_t
                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    // ReLU
                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Decoder Conv2DTranspose Layer 3 complete...\n");
}


void decoder_upsample2d_3(int8_t input[DEC_NUM_FILTERS_L3][DEC_OUT_SIZE_3][DEC_OUT_SIZE_3],
                          int8_t output[DEC_NUM_FILTERS_L3][DEC_UPSAMPLED_SIZE_3][DEC_UPSAMPLED_SIZE_3]) {
    printf("Starting Decoder Upsampling Layer 3...\n");

    for (int c = 0; c < DEC_NUM_FILTERS_L3; c++) {
        for (int i = 0; i < DEC_OUT_SIZE_3; i++) {
            for (int j = 0; j < DEC_OUT_SIZE_3; j++) {
                int val = input[c][i][j];
                output[c][2 * i][2 * j] = val;
                output[c][2 * i + 1][2 * j] = val;
                output[c][2 * i][2 * j + 1] = val;
                output[c][2 * i + 1][2 * j + 1] = val;
            }
        }
    }

    printf("Decoder Upsampling Layer 3 complete...\n");
}



void write_output_decoder_L3(const char *filename,
                             int8_t output[DEC_NUM_FILTERS_L3][DEC_UPSAMPLED_SIZE_3][DEC_UPSAMPLED_SIZE_3]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open decoder output file");
        return;
    }

    for (int f = 0; f < DEC_NUM_FILTERS_L3; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < DEC_UPSAMPLED_SIZE_3; i++) {
            for (int j = 0; j < DEC_UPSAMPLED_SIZE_3; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void decoder_conv2dtranspose4(
    void *virtual_base,
    int8_t input[DEC_NUM_FILTERS_L3][DEC_UPSAMPLED_SIZE_3][DEC_UPSAMPLED_SIZE_3],
    int8_t filters[DEC_NUM_FILTERS_L4][DEC_NUM_FILTERS_L3][FILTER_SIZE][FILTER_SIZE],
    int32_t bias[DEC_NUM_FILTERS_L4],
    int8_t output[DEC_NUM_FILTERS_L4][DEC_OUT_SIZE_4][DEC_OUT_SIZE_4]
) {
    printf("Starting Decoder Conv2DTranspose Layer 4...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < DEC_NUM_FILTERS_L4; f += NUM_PES) {
        int pes_to_use = (DEC_NUM_FILTERS_L4 - f < NUM_PES) ? (DEC_NUM_FILTERS_L4 - f) : NUM_PES;

        for (int i = 1; i < DEC_OUT_SIZE_4 - 1; i++) {
            for (int j = 1; j < DEC_OUT_SIZE_4 - 1; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < DEC_NUM_FILTERS_L3; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int x = i + m - 1;
                                int y = j + n - 1;
                                int8_t val = (x >= 0 && x < DEC_UPSAMPLED_SIZE_3 && y >= 0 && y < DEC_UPSAMPLED_SIZE_3)
                                             ? input[c][x][y] : 0;
                                *(volatile uint32_t *)((char *)pe_base + ((m * FILTER_SIZE + n) * 4)) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][FILTER_SIZE - 1 - m][FILTER_SIZE - 1 - n];
                                *(volatile uint32_t *)((char *)pe_base + ((9 + m * FILTER_SIZE + n) * 4)) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        while (*(volatile uint32_t *)((char *)pe_base + 31 * 4) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    sum += bias[f + pe];

                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Decoder Conv2DTranspose Layer 4 complete...\n");
}


void decoder_upsample2d_4(int8_t input[DEC_NUM_FILTERS_L4][DEC_OUT_SIZE_4][DEC_OUT_SIZE_4],
                          int8_t output[DEC_NUM_FILTERS_L4][DEC_UPSAMPLED_SIZE_4][DEC_UPSAMPLED_SIZE_4]) {
    printf("Starting Decoder Upsampling Layer 4...\n");

    for (int c = 0; c < DEC_NUM_FILTERS_L4; c++) {
        for (int i = 0; i < DEC_OUT_SIZE_4; i++) {
            for (int j = 0; j < DEC_OUT_SIZE_4; j++) {
                int val = input[c][i][j];
                output[c][2 * i][2 * j] = val;
                output[c][2 * i + 1][2 * j] = val;
                output[c][2 * i][2 * j + 1] = val;
                output[c][2 * i + 1][2 * j + 1] = val;
            }
        }
    }

    printf("Decoder Upsampling Layer 4 complete...\n");
}

void write_output_decoder_L4(const char *filename,
                             int8_t output[DEC_NUM_FILTERS_L4][DEC_UPSAMPLED_SIZE_4][DEC_UPSAMPLED_SIZE_4]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open decoder output file");
        return;
    }

    for (int f = 0; f < DEC_NUM_FILTERS_L4; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < DEC_UPSAMPLED_SIZE_4; i++) {
            for (int j = 0; j < DEC_UPSAMPLED_SIZE_4; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

void decoder_conv2dtranspose5(
    void *virtual_base,
    int8_t input[DEC_NUM_FILTERS_L4][DEC_UPSAMPLED_SIZE_4][DEC_UPSAMPLED_SIZE_4],
    int8_t filters[DEC_NUM_FILTERS_L5][DEC_NUM_FILTERS_L4][FILTER_SIZE][FILTER_SIZE],
    int32_t bias[DEC_NUM_FILTERS_L5],
    int8_t output[DEC_NUM_FILTERS_L5][DEC_OUT_SIZE_5][DEC_OUT_SIZE_5]
) {
    printf("Starting Decoder Conv2DTranspose Layer 5...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < DEC_NUM_FILTERS_L5; f += NUM_PES) {
        int pes_to_use = (DEC_NUM_FILTERS_L5 - f < NUM_PES) ? (DEC_NUM_FILTERS_L5 - f) : NUM_PES;

        for (int i = 1; i < DEC_OUT_SIZE_5 - 1; i++) {
            for (int j = 1; j < DEC_OUT_SIZE_5 - 1; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < DEC_NUM_FILTERS_L4; c++) {
                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int x = i + m - 1;
                                int y = j + n - 1;
                                int8_t val = (x >= 0 && x < DEC_UPSAMPLED_SIZE_4 && y >= 0 && y < DEC_UPSAMPLED_SIZE_4)
                                             ? input[c][x][y] : 0;
                                *(volatile uint32_t *)((char *)pe_base + ((m * FILTER_SIZE + n) * 4)) = val;
                            }
                        }

                        for (int m = 0; m < FILTER_SIZE; m++) {
                            for (int n = 0; n < FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][FILTER_SIZE - 1 - m][FILTER_SIZE - 1 - n];
                                *(volatile uint32_t *)((char *)pe_base + ((9 + m * FILTER_SIZE + n) * 4)) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        while (*(volatile uint32_t *)((char *)pe_base + 31 * 4) == 0);
                        sum += *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    sum += bias[f + pe];

                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    output[f + pe][i][j] = (shifted > 0) ? (int8_t)shifted : 0;
                }
            }
        }
    }

    printf("Decoder Conv2DTranspose Layer 5 complete...\n");
}


void decoder_upsample2d_5(int8_t input[DEC_NUM_FILTERS_L5][DEC_OUT_SIZE_5][DEC_OUT_SIZE_5],
                          int8_t output[DEC_NUM_FILTERS_L5][DEC_UPSAMPLED_SIZE_5][DEC_UPSAMPLED_SIZE_5]) {
    printf("Starting Decoder Upsampling Layer 5...\n");

    for (int c = 0; c < DEC_NUM_FILTERS_L5; c++) {
        for (int i = 0; i < DEC_OUT_SIZE_5; i++) {
            for (int j = 0; j < DEC_OUT_SIZE_5; j++) {
                int val = input[c][i][j];
                output[c][2 * i][2 * j] = val;
                output[c][2 * i + 1][2 * j] = val;
                output[c][2 * i][2 * j + 1] = val;
                output[c][2 * i + 1][2 * j + 1] = val;
            }
        }
    }

    printf("Decoder Upsampling Layer 5 complete...\n");
}


void write_output_decoder_L5(const char *filename,
                             int8_t output[DEC_NUM_FILTERS_L5][DEC_UPSAMPLED_SIZE_5][DEC_UPSAMPLED_SIZE_5]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open decoder output file");
        return;
    }

    for (int f = 0; f < DEC_NUM_FILTERS_L5; f++) {
        fprintf(fp, "Filter %d:\n", f);
        for (int i = 0; i < DEC_UPSAMPLED_SIZE_5; i++) {
            for (int j = 0; j < DEC_UPSAMPLED_SIZE_5; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
}

double exp_taylor(double x, int terms) {
    if (x < -10.0) x = -10.0;
    if (x > 10.0)  x = 10.0;

    double result = 1.0;
    double term = 1.0;

    for (int i = 1; i < terms; i++) {
        term *= x / i;
        result += term;
    }

    return result;
}

int sigmoid_taylor(int32_t shifted) {
    double x = shifted / 128.0; // normalize int8 range to [-1, 1]
    double exp_neg_x = exp_taylor(-x, 10); // 10 terms Taylor expansion
    double sigmoid = 1.0 / (1.0 + exp_neg_x);
    return (sigmoid >= 0.5) ? 1 : 0;
}

void final_conv2d(
    void *virtual_base,
    int8_t input[DEC_NUM_FILTERS_L5][FINAL_OUTPUT_SIZE][FINAL_OUTPUT_SIZE],
    int8_t filters[FINAL_NUM_OUTPUTS][DEC_NUM_FILTERS_L5][FINAL_FILTER_SIZE][FINAL_FILTER_SIZE],
    int32_t bias[FINAL_NUM_OUTPUTS],
    int8_t output[FINAL_NUM_OUTPUTS][FINAL_OUTPUT_SIZE][FINAL_OUTPUT_SIZE]
) {
    printf("Starting final Conv2D layer with sigmoid activation...\n");

    int pe_bases[NUM_PES] = {
        PE_BASE_0, PE_BASE_1, PE_BASE_2, PE_BASE_3, PE_BASE_4,
        PE_BASE_5, PE_BASE_6, PE_BASE_7, PE_BASE_8, PE_BASE_9,
        PE_BASE_10, PE_BASE_11, PE_BASE_12, PE_BASE_13, PE_BASE_14,
        PE_BASE_15, PE_BASE_16
    };

    for (int f = 0; f < FINAL_NUM_OUTPUTS; f += NUM_PES) {
        int pes_to_use = (FINAL_NUM_OUTPUTS - f < NUM_PES) ? (FINAL_NUM_OUTPUTS - f) : NUM_PES;

        for (int i = 1; i < FINAL_OUTPUT_SIZE - 1; i++) {
            for (int j = 1; j < FINAL_OUTPUT_SIZE - 1; j++) {
                for (int pe = 0; pe < pes_to_use; pe++) {
                    volatile uint32_t *pe_base = (volatile uint32_t *)((char *)virtual_base + pe_bases[pe]);
                    int32_t sum = 0;

                    for (int c = 0; c < DEC_NUM_FILTERS_L5; c++) {
                        for (int m = 0; m < FINAL_FILTER_SIZE; m++) {
                            for (int n = 0; n < FINAL_FILTER_SIZE; n++) {
                                int x = i + m - 1;
                                int y = j + n - 1;
                                int8_t val = input[c][x][y];
                                int offset = (m * FINAL_FILTER_SIZE + n) * 4;
                                *(volatile uint32_t *)((char *)pe_base + offset) = val;
                            }
                        }

                        for (int m = 0; m < FINAL_FILTER_SIZE; m++) {
                            for (int n = 0; n < FINAL_FILTER_SIZE; n++) {
                                int8_t w = filters[f + pe][c][m][n];  // no flip
                                int offset = (9 + m * FINAL_FILTER_SIZE + n) * 4;
                                *(volatile uint32_t *)((char *)pe_base + offset) = w;
                            }
                        }

                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 1;
                        while (*(volatile uint32_t *)((char *)pe_base + 31 * 4) == 0);
                        int32_t mac_result = *(volatile uint32_t *)((char *)pe_base + 30 * 4);
                        sum += mac_result;
                        *(volatile uint32_t *)((char *)pe_base + 18 * 4) = 0;
                    }

                    sum += bias[f + pe];

                    // Dynamic shift
                    int shift = 0;
                    for (int b = 31; b > 7; b--) {
                        if ((sum >> b) & 1) {
                            shift = b - 7;
                            break;
                        }
                    }

                    int32_t shifted = sum >> shift;
                    if (shifted > 127) shifted = 127;
                    if (shifted < -128) shifted = -128;

                    // Normalize and apply sigmoid
                    output[f + pe][i][j] = sigmoid_taylor(shifted);

                }
            }
        }
    }

    printf("Final Conv2D layer complete...\n");
}



void write_final_output(const char *filename, int8_t output[FINAL_NUM_OUTPUTS][FINAL_OUTPUT_SIZE][FINAL_OUTPUT_SIZE]) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Failed to open final output file");
        return;
    }

    for (int f = 0; f < FINAL_NUM_OUTPUTS; f++) {
        fprintf(fp, "Final Output:\n");
        for (int i = 0; i < FINAL_OUTPUT_SIZE; i++) {
            for (int j = 0; j < FINAL_OUTPUT_SIZE; j++) {
                fprintf(fp, "%d ", output[f][i][j]);
            }
            fprintf(fp, "\n");
        }
    }

    fclose(fp);
}


int main() {
    int fd;
    void *virtual_base = map_physical_memory(HW_MMAP_BASE, MAP_SIZE, &fd);
    if (virtual_base == NULL) return 1;

    static int8_t image[IMG_SIZE][IMG_SIZE];

    static int8_t padded[PADDED_SIZE][PADDED_SIZE] = {0};


    static int8_t filters1[NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE];
    static int8_t filters2[NUM_FILTERS_L2][NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE];
    static int8_t filters3[NUM_FILTERS_L3][NUM_FILTERS_L2][FILTER_SIZE][FILTER_SIZE];
    static int8_t filters4[NUM_FILTERS_L4][NUM_FILTERS_L3][FILTER_SIZE][FILTER_SIZE];
    static int8_t filters5[NUM_FILTERS_L5][NUM_FILTERS_L4][FILTER_SIZE][FILTER_SIZE];

    static int32_t bias1[NUM_FILTERS_L1];
    static int32_t bias2[NUM_FILTERS_L2];
    static int32_t bias3[NUM_FILTERS_L3];
    static int32_t bias4[NUM_FILTERS_L4];
    static int32_t bias5[NUM_FILTERS_L5];


    
    static int8_t output1[NUM_FILTERS_L1][IMG_SIZE][IMG_SIZE];
    static int8_t pooled1[NUM_FILTERS_L1][OUTPUT_L1_SIZE][OUTPUT_L1_SIZE];
    
    static int8_t output2[NUM_FILTERS_L2][OUTPUT_L1_SIZE][OUTPUT_L1_SIZE];
    static int8_t pooled2[NUM_FILTERS_L2][OUTPUT_L2_SIZE][OUTPUT_L2_SIZE];
    
    static int8_t output3[NUM_FILTERS_L3][OUTPUT_L2_SIZE][OUTPUT_L2_SIZE];
    static int8_t pooled3[NUM_FILTERS_L3][OUTPUT_L3_SIZE][OUTPUT_L3_SIZE];
    
    
    static int8_t output4[NUM_FILTERS_L4][OUTPUT_L3_SIZE][OUTPUT_L3_SIZE];
    static int8_t pooled4[NUM_FILTERS_L4][OUTPUT_L4_SIZE][OUTPUT_L4_SIZE];
    
    
    static int8_t output5[NUM_FILTERS_L5][OUTPUT_L4_SIZE][OUTPUT_L4_SIZE];
    static int8_t pooled5[NUM_FILTERS_L5][OUTPUT_L5_SIZE][OUTPUT_L5_SIZE];

    static int8_t decoder_filters1[DEC_NUM_FILTERS_L1][DEC_NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE];
    static int32_t decoder_bias1[DEC_NUM_FILTERS_L1];
    static int8_t decoder_out1[DEC_NUM_FILTERS_L1][DEC_OUT_SIZE_1][DEC_OUT_SIZE_1];
    static int8_t decoder_upsampled1[DEC_NUM_FILTERS_L1][DEC_UPSAMPLED_SIZE_1][DEC_UPSAMPLED_SIZE_1];
    
    static int8_t decoder_filters2[DEC_NUM_FILTERS_L2][DEC_NUM_FILTERS_L1][FILTER_SIZE][FILTER_SIZE];
    static int32_t decoder_bias2[DEC_NUM_FILTERS_L2];
    static int8_t decoder_out2[DEC_NUM_FILTERS_L2][DEC_OUT_SIZE_2][DEC_OUT_SIZE_2];
    static int8_t decoder_upsampled2[DEC_NUM_FILTERS_L2][DEC_UPSAMPLED_SIZE_2][DEC_UPSAMPLED_SIZE_2];
    
    static int8_t decoder_filters3[DEC_NUM_FILTERS_L3][DEC_NUM_FILTERS_L2][FILTER_SIZE][FILTER_SIZE];
    static int32_t decoder_bias3[DEC_NUM_FILTERS_L3];
    static int8_t decoder_out3[DEC_NUM_FILTERS_L3][DEC_OUT_SIZE_3][DEC_OUT_SIZE_3];
    static int8_t decoder_upsampled3[DEC_NUM_FILTERS_L3][DEC_UPSAMPLED_SIZE_3][DEC_UPSAMPLED_SIZE_3];
    
    static int8_t decoder_filters4[DEC_NUM_FILTERS_L4][DEC_NUM_FILTERS_L3][FILTER_SIZE][FILTER_SIZE];
    static int32_t decoder_bias4[DEC_NUM_FILTERS_L4];
    static int8_t decoder_out4[DEC_NUM_FILTERS_L4][DEC_OUT_SIZE_4][DEC_OUT_SIZE_4];
    static int8_t decoder_upsampled4[DEC_NUM_FILTERS_L4][DEC_UPSAMPLED_SIZE_4][DEC_UPSAMPLED_SIZE_4];
    
    static int8_t decoder_filters5[DEC_NUM_FILTERS_L5][DEC_NUM_FILTERS_L4][FILTER_SIZE][FILTER_SIZE];
    static int32_t decoder_bias5[DEC_NUM_FILTERS_L5];
    static int8_t decoder_out5[DEC_NUM_FILTERS_L5][DEC_OUT_SIZE_5][DEC_OUT_SIZE_5];
    static int8_t decoder_upsampled5[DEC_NUM_FILTERS_L5][DEC_UPSAMPLED_SIZE_5][DEC_UPSAMPLED_SIZE_5];
    
    static int8_t final_conv_filter[FINAL_NUM_OUTPUTS][FINAL_NUM_INPUTS][FINAL_FILTER_SIZE][FINAL_FILTER_SIZE];
    static int32_t final_conv_bias[FINAL_NUM_OUTPUTS];
    static int8_t final_output[FINAL_NUM_OUTPUTS][FINAL_OUTPUT_SIZE][FINAL_OUTPUT_SIZE];
    
        
    


    printf("== Loading inputs ==\n");
    if (load_image_from_file("input.txt", image) != 0) return 1;
    
    if (load_filters_layer1("filters1.txt", filters1) != 0) return 1;
    if (load_biases_layer1("bias1.txt", bias1) != 0) return 1;
    if (load_filters_layer2("filters2.txt", filters2) != 0) return 1;
    if (load_biases_layer2("bias2.txt", bias2) != 0) return 1;
    if (load_filters_layer3("filters3.txt", filters3) != 0) return 1;
    if (load_biases_layer3("bias3.txt", bias3) != 0) return 1;
    if (load_filters_layer4("filters4.txt", filters4) != 0) return 1;
    if (load_biases_layer4("bias4.txt", bias4) != 0) return 1;
    if (load_filters_layer5("filters5.txt", filters5) != 0) return 1;
    if (load_biases_layer5("bias5.txt", bias5) != 0) return 1;
    

    if (load_filters_decoder1("filters1_1.txt", decoder_filters1) != 0) return 1;
    if (load_biases_decoder1("bias1_1.txt", decoder_bias1) != 0) return 1;
    
    if (load_filters_decoder2("filters2_2.txt", decoder_filters2) != 0) return 1;
    if (load_biases_decoder2("bias2_2.txt", decoder_bias2) != 0) return 1;
    if (load_filters_decoder3("filters3_3.txt", decoder_filters3) != 0) return 1;
    if (load_biases_decoder3("bias3_3.txt", decoder_bias3) != 0) return 1;
    if (load_filters_decoder4("filters4_4.txt", decoder_filters4) != 0) return 1;
    if (load_biases_decoder4("bias4_4.txt", decoder_bias4) != 0) return 1;
    if (load_filters_decoder5("filters5_5.txt", decoder_filters5) != 0) return 1;
    if (load_biases_decoder5("bias5_5.txt", decoder_bias5) != 0) return 1;
    if (load_final_conv_filter("final_filters.txt", final_conv_filter) != 0) return 1;
    if (load_final_conv_bias("final_bias.txt", final_conv_bias) != 0) return 1;
    
    


    for (int i = 0; i < IMG_SIZE; i++)
        for (int j = 0; j < IMG_SIZE; j++)
            padded[i + PADDING][j + PADDING] = image[i][j];

    printf("== Starting processing ==\n");
    struct timeval start, end;
    gettimeofday(&start, NULL);

    perform_layer1_convolution(virtual_base, padded, filters1, output1, bias1);
    max_pool_layer1(output1, pooled1);
    write_output_L1("output_layer1_1.txt", pooled1);

    perform_layer2_convolution(virtual_base, pooled1, filters2, output2, bias2);
    max_pool_layer2(output2, pooled2);
    write_output_L2("output_layer2_2.txt", pooled2);
    
    perform_layer3_convolution(virtual_base, pooled2, filters3, output3, bias3);
    max_pool_layer3(output3, pooled3);
    write_output_L3("output_layer3_3.txt", pooled3);
    
    perform_layer4_convolution(virtual_base, pooled3, filters4, output4, bias4);
    max_pool_layer4(output4, pooled4);
    write_output_L4("output_layer4_4.txt", pooled4);
    
    perform_layer5_convolution(virtual_base, pooled4, filters5, output5, bias5);
    max_pool_layer5(output5, pooled5);
    write_output_L5("output_layer5_5.txt", pooled5);

    decoder_conv2dtranspose1(virtual_base, pooled5, decoder_filters1, decoder_bias1, decoder_out1);
    decoder_upsample2d_1(decoder_out1, decoder_upsampled1);
    write_output_decoder_L1("decoder_upsampled1.txt", decoder_upsampled1);
    
  
    decoder_conv2dtranspose2(virtual_base, decoder_upsampled1, decoder_filters2, decoder_bias2, decoder_out2);
    decoder_upsample2d_2(decoder_out2, decoder_upsampled2);
    write_output_decoder_L2("decoder_upsampled2.txt", decoder_upsampled2);
    
    decoder_conv2dtranspose3(virtual_base, decoder_upsampled2, decoder_filters3, decoder_bias3, decoder_out3);
    decoder_upsample2d_3(decoder_out3, decoder_upsampled3);
    write_output_decoder_L3("decoder_upsampled3.txt", decoder_upsampled3);
    
    decoder_conv2dtranspose4(virtual_base, decoder_upsampled3, decoder_filters4, decoder_bias4, decoder_out4);
    decoder_upsample2d_4(decoder_out4, decoder_upsampled4);
    write_output_decoder_L4("decoder_upsampled4.txt", decoder_upsampled4);
    
    decoder_conv2dtranspose5(virtual_base, decoder_upsampled4, decoder_filters5, decoder_bias5, decoder_out5);
    decoder_upsample2d_5(decoder_out5, decoder_upsampled5);
    write_output_decoder_L5("decoder_upsampled5.txt", decoder_upsampled5);
    
    final_conv2d(virtual_base, decoder_upsampled5, final_conv_filter, final_conv_bias, final_output);
    write_final_output("final_decoder_output.txt", final_output);
	
    

    gettimeofday(&end, NULL);
    
    // use long long to avoid overflow
    long long time_us = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec);
    long long total_seconds = time_us / 1000000LL;
    int hours = total_seconds / 3600;
    int minutes = (total_seconds % 3600) / 60;
    int seconds = (total_seconds % 60);
    
    // nicely formatted output
    printf("\n Total Execution Time: ");
    if (hours > 0)
        printf("%d hours ", hours);
    if (minutes > 0 || hours > 0)
        printf("%d minutes ", minutes);
    printf("%d seconds (%.3f minutes, %.0f microseconds)\n", seconds, time_us / 60000000.0, (double)time_us);
    

    unmap_physical_memory(virtual_base, MAP_SIZE, fd);
    return 0;
}
