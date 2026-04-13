//compile with gcc -o pixels_curve pixels_curve.c gammaCorr.c -O2 -Wall -lm -lrt

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

// Function prototypes
void gamma_correct(const uint8_t* img, size_t width, size_t height, float a, float b, float c, float gamma, uint8_t* result);
void init_gamma_lut(float gamma);
void gamma_correct_V1(const uint8_t* img, size_t width, size_t height, float a, float b, float c, float gamma, uint8_t* result);
void gamma_correct_V2(const uint8_t* img, size_t width, size_t height, float a, float b, float c, float gamma, uint8_t* result);


double duration1(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}


uint8_t* read_ppm(const char* filename, size_t* width, size_t* height);

int main() {
    const char* input_files[50];
    for (int i = 0; i < 50; i++) {
        input_files[i] = malloc(20);
        sprintf((char*)input_files[i], "test_image_%d.ppm", i + 1);
    }

    float coeffs[3] = {0.299f, 0.587f, 0.114f}; // Grayscale coefficients
    float gamma = 2.5;  // Fixed gamma value 
    printf("Pixels,Version,Time\n"); // CSV-style output

    // Loop through all 50 input files
    for (int file_index = 0; file_index < 50; file_index++) {
        size_t width, height;

        // Read the input image
        uint8_t* img = read_ppm(input_files[file_index], &width, &height);
        if (!img) {
            fprintf(stderr, "Error: Failed to load input file %s.\n", input_files[file_index]);
            continue;
        }

        // Allocate memory for the result image
        uint8_t* result_img = malloc(width * height);
        if (!result_img) {
            fprintf(stderr, "Error: Memory allocation failed.\n");
            free(img);
            continue;
        }

        size_t pixels = width * height; // Total number of pixels

        // Benchmark each implementation
        for (int implementation = 0; implementation < 3; ++implementation) {
            struct timespec start, end;
            clock_gettime(CLOCK_MONOTONIC, &start);

            if (implementation == 0) {
                gamma_correct(img, width, height, coeffs[0], coeffs[1], coeffs[2], gamma, result_img);
            } else if (implementation == 1) {
                init_gamma_lut(gamma);
                gamma_correct_V1(img, width, height, coeffs[0], coeffs[1], coeffs[2], gamma, result_img);
            } else if (implementation == 2) {
                init_gamma_lut(gamma); 
                gamma_correct_V2(img, width, height, coeffs[0], coeffs[1], coeffs[2], gamma, result_img);
            } else {
                fprintf(stderr, "Error: Invalid implementation version.\n");
                free(img);
                free(result_img);
                return 1;
            }

            clock_gettime(CLOCK_MONOTONIC, &end);
            double elapsed_time = duration1(start, end);

            // Output the results to paste in CSV format
            printf("%zu,V%d,%.6f\n", pixels, implementation, elapsed_time);
        }

        free(img);
        free(result_img);
    }

    for (int i = 0; i < 50; i++) {
        free((void*)input_files[i]);
    }

    return 0;
}
