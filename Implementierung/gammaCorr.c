#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <ctype.h>






//./gammaCorr -o ~/Desktop/mandrill_gray.ppm ~/Desktop/mandrill.ppm --gamma 2.2 --coeffs 0.299,0.587,0.114
//./main -V1 --gamma 2.2 --coeffs 0.299,0.587,0.114 ~/Desktop/mandrill.ppm -o ~/Desktop/mandrill_gray.ppm





// Default coefficients for grayscale
#define DEFAULT_A 0.299f
#define DEFAULT_B 0.587f
#define DEFAULT_C 0.114f
#define LUT_SIZE 256

// Function prototypes
void gamma_correct(const uint8_t* img, size_t width, size_t height, float a, float b, float c, float gamma, uint8_t* result);
void print_help();


float approx_ln(float b) {
    float x = (b-1.00f); // Transformation to improve convergence
    float k;
    float power = 1.00f;
    float sum = 0.00f;
    for (int i =1 ; i<30 ; i++){ // Truncate after 30 terms
        if (i % 2 == 0){
            k=-1.0f/(float)i;
        }
        else {
            k=1.0f/(float)i;
        }
        power = power *x ;
        sum = sum + k * power ;
    }
    return sum ;
}




float approx_exp(float x) {
  // taylor series

    float term = 1.0f;  // First term of the series
    float sum = 1.0f;   // Start with the first term
    float power = 1.0f; // Keeps track of x^i
    float divisor = 1.0f; // Keeps track of i!

    for (int i = 1; i < 30; i++) {
      //30 represents the number of terms in the Taylor series expansion of ,a stopping criterion to control the level of precision
        power *= x;         // Calculate x^i
        divisor *= i;       // Calculate i! incrementally
        term = power / divisor;  // x^i / i!
        sum += term;

        // Break if the term becomes too small to affect the result
        if (term < 1e-6f && term > -1e-6f) {
            break;
        }
    }

    return sum;
}

float float_power(float base, float power) {
    if (base <= 0.0f) {
        return -1.0f; // Error
    }

    float ln_base = approx_ln(base);
    if (ln_base < -50.0f || ln_base > 50.0f) {
        return 0.0f; // Clamped range to avoid large errors
    }

    float exp_result = approx_exp(power * ln_base);
   // printf("base: %.10f, power: %.10f, ln_base: %.10f, exp_result: %.10f\n",
          // base, power, ln_base, exp_result);
    return exp_result;
}
void gamma_correct(
    const uint8_t* img, size_t width, size_t height,
    float a, float b, float c,
    float gamma,
    uint8_t* result)
{
    size_t num_pixels = width * height;
   //size_t is platform-independent in terms of its use for indexing and representing sizes of objects in memory.
    //The number of pixels in an image can be large, especially for high-resolution images. For example, an image of size
    // 8000x8000 pixels has 64 million pixels. As the resolution increases, so does the number of pixels, potentially reaching billions.
    for (size_t i = 0; i < num_pixels; ++i) {
        uint8_t r = img[i * 3 + 0];
        uint8_t g = img[i * 3 + 1];
        uint8_t b_val = img[i * 3 + 2];

        float gray = (a * r + b * g + c * b_val) / (a + b + c);

        // Gammakorrektur mittels Approximation
        float corrected = float_power(gray / 255.0f, gamma) * 255.0f;

           result[i] = (uint8_t)(corrected > 255.0f ? 255.0f : (corrected < 0.0f ? 0.0f : corrected)); // each result is one byte and not 3



    }

}


void print_help() {
    printf("Help Message\n");
    printf("Usage: gamma_correction [OPTIONS] <input_file>\n");
    printf("\nOptions:\n");
    printf("  -V <number>           Select implementation version (default: 0).\n");
    printf("                         Example: ./main -V 1 input_file.ppm -o output_file.ppm\n");
    printf("  -B[<number>]          Benchmark mode with optional repeats (default: 1).\n");
    printf("                         Example: ./main -B5 input_file.ppm -o output_file.ppm\n");
    printf("  -o <output_file>      Specify the output file.\n");
    printf("                         Example: ./main -o output_file.ppm input_file.ppm\n");
    printf("  --coeffs <a,b,c>      Coefficients for grayscale conversion (default: %.3f,%.3f,%.3f).\n", DEFAULT_A, DEFAULT_B, DEFAULT_C);
    printf("                         Example: ./main --coeffs 0.3,0.6,0.1 input_file.ppm -o output_file.ppm\n");
    printf("  --gamma <value>       Gamma correction value (default: 1.0).\n");
    printf("                         Example: ./main --gamma 2.2 input_file.ppm -o output_file.ppm\n");
    printf("  -h, --help            Show this help message and exit.\n");
    printf("                         Example: ./main --help\n");
}

uint8_t* read_ppm(const char* filename, size_t* width, size_t* height) {

    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s.\n", filename);
        return NULL;
    }

    // Read magic number
    char format[3];
    if (fscanf(file, "%2s", format) != 1) {
        fprintf(stderr, "Error: Could not read format.\n");
        fclose(file);
        return NULL;
    }

    if (strcmp(format, "P6") != 0) {
        fprintf(stderr, "Error: Unsupported file format (only P6 is supported).\n");
        fclose(file);
        return NULL;
    }

    // Skip comments and whitespace before dimensions
    int c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '#') {
            while ((c = fgetc(file)) != EOF && c != '\n');
        } else if (!isspace(c)) {
            ungetc(c, file);
            break;
        }
    }

    // Read dimensions
    if (fscanf(file, "%zu %zu", width, height) != 2) {
        fprintf(stderr, "Error: Could not read dimensions.\n");
        fclose(file);
        return NULL;
    }

    // Skip comments and whitespace before max value
    while ((c = fgetc(file)) != EOF) {
        if (c == '#') {
            while ((c = fgetc(file)) != EOF && c != '\n');
        } else if (!isspace(c)) {
            ungetc(c, file);
            break;
        }
    }

    // Read max value
    int max_val;
    if (fscanf(file, "%d", &max_val) != 1 || max_val != 255) {
        fprintf(stderr, "Error: Invalid max value (must be 255).\n");
        fclose(file);
        return NULL;
    }

    // Skip single whitespace character after max value
    c = fgetc(file);
    if (c == EOF || !isspace(c)) {
        fprintf(stderr, "Error: Expected whitespace after max value.\n");
        fclose(file);
        return NULL;
    }

    // Calculate size of first image
    size_t num_pixels = (*width) * (*height) * 3;
    uint8_t* img = malloc(num_pixels);
    if (!img) {
        fprintf(stderr, "Error: Memory allocation failed for %zu bytes.\n", num_pixels);
        fclose(file);
        return NULL;
    }

    // Read exactly one image worth of data
    size_t bytes_read = 0;
    size_t chunk_size = 8192; // Read in 8KB chunks

    while (bytes_read < num_pixels) {
        size_t to_read = num_pixels - bytes_read;
        if (to_read > chunk_size) {
            to_read = chunk_size;
        }

        size_t result = fread(img + bytes_read, 1, to_read, file);
        if (result == 0) {
            if (feof(file)) {
                fprintf(stderr, "Error: Unexpected end of file after reading %zu bytes.\n", bytes_read);
            } else {
                fprintf(stderr, "Error: Failed to read file after %zu bytes.\n", bytes_read);
            }
            free(img);
            fclose(file);
            return NULL;
        }
        bytes_read += result;
    }

    // Check if we have read exactly one image worth of pixels
    if (bytes_read != num_pixels) {
        fprintf(stderr, "Error: Could not read complete first image (%zu bytes read, expected %zu).\n",
                bytes_read, num_pixels);
        free(img);
        fclose(file);
        return NULL;
    }

    printf("First image loaded: %zu x %zu (%zu bytes)\n", *width, *height, bytes_read);
    fclose(file);
    return img;
}
void write_ppm(const char* filename, const uint8_t* img, size_t width, size_t height) {
   FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s for writing.\n", filename);
        return;
    }

    fprintf(file, "P5\n%zu %zu\n255\n", width, height);
    size_t num_pixels = width * height;
    fwrite(img, 1, num_pixels, file); // img pointer to the data and 1 is the size (in bytes) of each item to be written.
    fclose(file);

    // Debugging line
    printf("Successfully wrote the image to %s\n", filename);
}
// Gamma lookup table
static uint8_t gamma_lut[LUT_SIZE];

// Function to initialize the gamma lookup table
void init_gamma_lut(float gamma) {
     for (int i = 0; i < 256; ++i) {
        float normalized = i / 255.0f;
        if (normalized == 0.0f) {
            normalized = 1e-5f; // Avoid passing zero to float_power
        }

        float corrected = float_power(normalized, gamma);
        if (corrected < 0.0f) {
            corrected = 0.0f; // Clamp negative values
        }

//printf("%e\n", corrected);  // Prints in scientific notation

        gamma_lut[i] = (uint8_t)(corrected * 255.0f);
    }
}


void gamma_correct_V1(const uint8_t* img, size_t width, size_t height,
                       float a, float b, float c, float gamma,
                       uint8_t* result) {
     size_t num_pixels = width * height;

    for (size_t i = 0; i < num_pixels; ++i) {
        uint8_t r = img[i * 3 + 0];
        uint8_t g = img[i * 3 + 1];
        uint8_t b_val = img[i * 3 + 2];

        float gray = (a * r + b * g + c * b_val) / (a + b + c);


        uint8_t corrected = gamma_lut[(int)gray];

        result[i] = corrected;
    }
}


#define MAX_THREADS 8

// Structure to pass data to each thread
typedef struct {
    const uint8_t* img;
    uint8_t* result;
    size_t start_index;
    size_t end_index;
    size_t width;
    size_t height;
    float a, b, c, gamma;
} thread_data_t;

// Dummy LUT initialization for the gamma correction


//Here, a thread starts its work. It receives a "parcel" of instructions and data (arg) that it unwraps into a structure
// called thread_data_t.
void* gamma_correct_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
// The thread is given a specific range of pixels to process, starting at start_index and ending just before end_index.
    // This prevents the threads from stepping on each other’s toes.
    for (size_t i = data->start_index; i < data->end_index; ++i) {
        uint8_t r = data->img[i * 3 + 0];//The thread extracts the red (r), green (g), and blue (b)
                                         // components of the current pixel.


        uint8_t g = data->img[i * 3 + 1];
        uint8_t b_val = data->img[i * 3 + 2];

        float gray = (data->a * r + data->b * g + data->c * b_val) / (data->a + data->b + data->c);

        // Use floor behavior for consistency with V0
        uint8_t corrected = gamma_lut[(int)gray];

        data->result[i ] = corrected;

    }

    return NULL;
}
static int get_optimal_thread_count(size_t num_pixels) {
    // Get number of available CPU cores
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cores <= 0) {
        num_cores = 4; // Fallback if we can't detect
    }

    // Calculate optimal thread count based on workload and cores
    int optimal_threads = (int)num_cores;

    // Don't create more threads than pixels
    if ((size_t)optimal_threads > num_pixels) {
        optimal_threads = (int)num_pixels;
    }

    // Cap maximum threads at 32 to prevent overhead
    if (optimal_threads > 32) {
        optimal_threads = 32;
    }

    return optimal_threads;
}

void gamma_correct_V2(const uint8_t* img, size_t width, size_t height,
                     float a, float b, float c, float gamma, uint8_t* result) {
    size_t num_pixels = width * height;
    int num_threads = get_optimal_thread_count(num_pixels);



    printf("Using %d threads for processing\n", num_threads);

    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];//creates an array to hold the thread identifiers and a corresponding
                                           // array to store the data each thread needs

    // Calculate pixels per thread
    size_t pixels_per_thread = num_pixels / num_threads;
    size_t remaining_pixels = num_pixels % num_threads;
//The function divides the work evenly among the threads. If there’s an uneven number of pixels,
    // the last thread gets a few extra
    // Create and launch threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].img = img;
        thread_data[i].result = result;
        thread_data[i].width = width;
        thread_data[i].height = height;
        thread_data[i].a = a;
        thread_data[i].b = b;
        thread_data[i].c = c;




        thread_data[i].gamma = gamma;

        // Calculate start and end indices for this thread
        thread_data[i].start_index = i * pixels_per_thread;// inclusive
        thread_data[i].end_index = (i + 1) * pixels_per_thread;// exclusive

        // Add remaining pixels to the last thread
        if (i == num_threads - 1) {
            thread_data[i].end_index += remaining_pixels;

        }

       // printf("Thread %d processing pixels %zu to %zu\n",
              // i, thread_data[i].start_index, thread_data[i].end_index);

        if (pthread_create(&threads[i], NULL, gamma_correct_thread, &thread_data[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            exit(1);
        }

    }

    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "Failed to join thread %d\n", i);
            exit(1)
            ;
        }
    }
}
