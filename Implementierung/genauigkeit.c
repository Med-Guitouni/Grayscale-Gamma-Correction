//compile with 'gcc genauigkeit.c -o genauigkeit' and run to see the difference in genauigkeit between V0 and V1
#include <stdio.h>
#include <math.h>

float approx_ln(float b) {
    float x = (b-1.00f); 
    float k;
    float power = 1.00f;
    float sum = 0.00f;
    for (int i =1 ; i<30 ; i++){ 
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

    float term = 1.0f;  
    float sum = 1.0f;   
    float power = 1.0f; 
    float divisor = 1.0f; 

    for (int i = 1; i < 30; i++) {
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
        return -1.0f;
    }

    float ln_base = approx_ln(base);
    if (ln_base < -50.0f || ln_base > 50.0f) {
        return 0.0f; 
    }

    float exp_result = approx_exp(power * ln_base);
    return exp_result;
}




int main() {
    float gamma = 2.2f; // Fixed gamma value
    int steps = 100;    
    float step_size = 255.0f / steps; // Increment for gray

    printf("gray,corrected_V0,corrected_V1\n"); // CSV header

    for (int i = 0; i <= steps; i++) {
        float gray = i * step_size;                
        float gray_rounded = roundf(gray);   
        float corrected_V0 = roundf(float_power(gray / 255.0f, gamma) * 255.0f); // V0 calculation
        float corrected_V1 = roundf(float_power(gray_rounded / 255.0f, gamma) * 255.0f); // V1 calculation

        // Output for curve plotting
        printf("%.2f,%.2f,%.2f\n", gray, corrected_V0, corrected_V1);
    }

    return 0;
}