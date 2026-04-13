#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <time.h>

// Default coefficients for grayscale
#define DEFAULT_A 0.299f
#define DEFAULT_B 0.587f
#define DEFAULT_C 0.114f

// V0 is our base implemenation pressty staright forward
// V1 consists for LUT (look up table )
//V2 threads

// Function prototypes
void gamma_correct(const uint8_t* img, size_t width, size_t height, float a, float b, float c, float gamma, uint8_t* result);
void print_help();
uint8_t* read_ppm(const char* filename, size_t* width, size_t* height);
void write_ppm(const char* filename, const uint8_t* img, size_t width, size_t height);
extern void init_gamma_lut(float gamma);
extern void gamma_correct_V1(const uint8_t* img, size_t width, size_t height,
                               float a, float b, float c, float gamma,
                               uint8_t* result);    
void gamma_correct_V2(const uint8_t* img, size_t width, size_t height,
                     float a, float b, float c, float gamma, uint8_t* result_img);

double duration (struct timespec start, struct timespec ende) {
    return (ende.tv_sec - start.tv_sec) + (ende.tv_nsec - start.tv_nsec) / 1e9;}



int main(int argc, char* argv[]) {
   int implementation = 0; // -V option, default to V0
    int benchmark_repeats = 1; // -B option
    char* input_file = NULL; // Positional argument
    char* output_file = NULL; // -o option
    float coeffs[3] = {DEFAULT_A, DEFAULT_B, DEFAULT_C}; // --coeffs option
    float gamma = 1.0f; // --gamma option

    // Option definitions for getopt_long
    static struct option long_options[] = {
        {"coeffs", required_argument, 0, 0},
        {"gamma", required_argument, 0, 0},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    int t =0;

    while ((opt = getopt_long(argc, argv, "V:B::o:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'V':
                implementation = atoi(optarg); // Set the implementation version
                break;
            case 'B':
                t = 1;
                benchmark_repeats = optarg ? atoi(optarg) : 1;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'h':
                print_help();
                return 0;
            case 0:
                if (strcmp(long_options[option_index].name, "coeffs") == 0) {
                    sscanf(optarg, "%f,%f,%f", &coeffs[0], &coeffs[1], &coeffs[2]);
                } else if (strcmp(long_options[option_index].name, "gamma") == 0) {
                    gamma = atof(optarg);
                }
                break;
            default:
                fprintf(stderr, "Unknown option. Use --help for usage information.\n");
                return 1;
        }
    }
    printf("Implementation Version: %d\n", implementation);

    // Handle positional argument (input file)
    if (optind < argc) {
        input_file = argv[optind];
    } else {
        fprintf(stderr, "Error: Input file is required. Use --help for usage information.\n");
        return 1;
    }

    // Check required options
    if (!output_file) {
        fprintf(stderr, "Error: Output file is required. Use --help for usage information.\n");
        return 1;
    }

 if ( benchmark_repeats <= 0) {
        printf("Error , benchmark repeats cannot be set to 0 or less\n");
        return EXIT_FAILURE; // Exit the program with an error code
    }

    size_t width, height;

    uint8_t* img = read_ppm(input_file, &width, &height);
    if (!img) {
        return 1;
    }

    // Prepare output image buffer
    uint8_t* result_img = malloc(width * height ); // RGB data
   
   
    // Benchmarking logic: Only if benchmark_repeats > 0
    if (benchmark_repeats > 0) {
        struct timespec start, ende;
        double total_time = 0.0;

        for (int i = 0; i < benchmark_repeats; i++) {
            clock_gettime(CLOCK_MONOTONIC, &start);

            if (implementation == 0) {
                gamma_correct(img, width, height, coeffs[0], coeffs[1], coeffs[2], gamma, result_img);
            } else if (implementation == 1) {
                init_gamma_lut(gamma);
                gamma_correct_V1(img, width, height, coeffs[0], coeffs[1], coeffs[2], gamma, result_img);
            } else if (implementation == 2) {
                // Multithreaded gamma correction implementation
                init_gamma_lut(gamma); // Reuse the LUT initialization for consistency
                gamma_correct_V2(img, width, height, coeffs[0], coeffs[1], coeffs[2], gamma, result_img);
            } else {
                // Handle invalid implementation version
                fprintf(stderr, "Error: Invalid implementation version. Use --help for usage information.\n");
                free(img);
                free(result_img);
                return 1;
            }

            clock_gettime(CLOCK_MONOTONIC, &ende);
            total_time += duration(start, ende);
        }

        double avg_time = total_time / benchmark_repeats;
        if(t==1){
         printf("Average execution time over %d run(s): %.6f seconds\n", benchmark_repeats, avg_time);
        }
        
    }


    // Write output image
    write_ppm(output_file, result_img, width, height);

    // Clean up
    free(img);
    free(result_img);

    return 0;
}
