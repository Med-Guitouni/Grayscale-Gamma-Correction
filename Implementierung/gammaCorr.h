
#ifndef GAMMACORR_H
#define GAMMACORR_H

#include  <stdint.h>

#include <stddef.h>

void print_help(void);
void init_gamma_lut(float gamma);
void gamma_correct(uint8_t* image, size_t width, size_t height, float r, float g, float b, float gamma, uint8_t* output);
void gamma_correct_V1(uint8_t* image, size_t width, size_t height, float r, float g, float b, float gamma, uint8_t* output);
void gamma_correct_V2(uint8_t* image, size_t width, size_t height, float r, float g, float b, float gamma, uint8_t* output);
void write_ppm(const char* filename, uint8_t* image, size_t width, size_t height);
uint8_t* read_ppm(const char* filename, size_t* width, size_t* height);

#endif // GAMMACORR_H
