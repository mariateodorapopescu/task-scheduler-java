// Author: APD team, except where source was noted

#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

// pe aici, mai imi pun comentarii cu ce se intampla

// Source: [1]
ppm_image *read_ppm(const char *filename) {
    char buff[16];
    ppm_image *img; // deja ne am facut noi o structura de imagine
    FILE *fp; // asta e fisierul nostru
    int c, rgb_comp_color;

    // open PPM file for reading
    fp = fopen(filename, "rb"); // deschide fifiser binar ca e imagine
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    // read image format
    if (!fgets(buff, sizeof(buff), fp)) { // citeste pe caprarii, mai intai metadatele, formatul
        perror(filename);
        exit(1);
    }

    // check the image format
    if (buff[0] != 'P' || buff[1] != '6') {
        fprintf(stderr, "Invalid image format (must be 'P6')\n"); // ppm means p6 format sooo
        exit(1);
    }

    // alloc memory for image
    img = (ppm_image *)malloc(sizeof(ppm_image)); // cred ca face cu realocari ca altfel nu vad
    if (!img) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    // check for comments
    c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n'); // again ia pe caprarii, aici trecem peste anumite date. 
        // noi am avut nevoie numai de format si de img in sine sooo

        c = getc(fp);
    }

    ungetc(c, fp);

    // read image size information
    // abia aici vine dimensiunea imaginii
    if (fscanf(fp, "%d %d", &img->x, &img->y) != 2) {
        fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
        exit(1);
    }

    // read RGB component
    if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
        fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename); // aici vad ce fel de imagine e cum ar veni
        // tot parte din metadate
        exit(1);
    }

    // check RGB component depth
    if (rgb_comp_color != RGB_COMPONENT_COLOR) {
        fprintf(stderr, "'%s' does not have 8-bits components\n", filename); 
        exit(1);
    }

    while (fgetc(fp) != '\n') ;

    // memory allocation for pixel data
    img->data = (ppm_pixel*)malloc(img->x * img->y * sizeof(ppm_pixel));

    if (!img) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }
    // abia acum citesc imaginea
    // read pixel data from file
    if ((int)fread(img->data, 3 * img->x, img->y, fp) != img->y) {
        // 3 ptr ca rgb
        fprintf(stderr, "Error loading image '%s'\n", filename);
        exit(1);
    }

    // asta e doar o citire. nu cred ca merge paralelizat, probabil sunt si chestii muuuult mai complexe de atat
    // chiar daca e mult de citit

    // deci asta intoarce o imagine, noi avem o structura de imagine cu care lucram

    fclose(fp);
    return img;
}

// Source: [1]
void write_ppm(ppm_image *img, const char *filename) {
    FILE *fp;

    // open file for output
    fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "Unable to open file '%s'\n", filename);
        exit(1);
    }

    // write the header file image format
    fprintf(fp, "P6\n");

    // image size
    fprintf(fp, "%d %d\n", img->x, img->y);

    // RGB component depth
    fprintf(fp, "%d\n", RGB_COMPONENT_COLOR);
    // aici afisez noua imagine
    // pixel data
    fwrite(img->data, 3 * img->x, img->y, fp);
    fclose(fp);
}

// Source: [2]
float cubic_hermite(float A, float B, float C, float D, float t) {
    // aici e ceva calcul ok-ish cu bicubic sau ceva pe acolo

    float a = -A / 2.0f + (3.0f * B) / 2.0f - (3.0f * C) / 2.0f + D / 2.0f;
    float b = A - (5.0f * B) / 2.0f + 2.0f * C - D / 2.0f;
    float c = -A / 2.0f + C / 2.0f;
    float d = B;

    // A, B, C, D sunt cele patru parti in care se imparte img, vezi ultima fct

    return a * t * t * t + b * t * t + c * t + d;
}

// Source: [2]
void get_pixel_clamped(ppm_image *source_image, int x, int y, uint8_t temp[]) {
    // aici e gen sa faca sa incapa in dimensiunile imaginii

    CLAMP(x, 0, source_image->x - 1); // ia minimul daca e mai mic decat el, sau maximul daca il depaseste 
    // gen il intinde ca sa incapa;  0 e min, chestie cu sageata e maxim, seems legit
    CLAMP(y, 0, source_image->y - 1);

    // deci si pe ox si pe oy intinde pixelii, apoi ii pune in ceva provizoriu

    temp[0] = source_image->data[x + (source_image->x * y)].red;
    temp[1] = source_image->data[x + (source_image->x * y)].green;
    temp[2] = source_image->data[x + (source_image->x * y)].blue;
    // temp e un singur pixel!
}

// Source: [2]
void sample_bicubic(ppm_image *source_image, float u, float v, uint8_t sample[]) {
    float x = (u * source_image->x) - 0.5;
    int xint = (int)x;
    float xfract = x - floor(x);

    float y = (v * source_image->y) - 0.5;
    int yint = (int)y;
    float yfract = y - floor(y);
    // e un fel de factor de scalare aproximat in lipsa??
    int i;

    // sunt de dimensiune 3 ca reprezinta un pixel cu comp rgb

    // gandeste-te la quad tree

    uint8_t p00[3];
    uint8_t p10[3];
    uint8_t p20[3];
    uint8_t p30[3];

    uint8_t p01[3];
    uint8_t p11[3];
    uint8_t p21[3];
    uint8_t p31[3];

    uint8_t p02[3];
    uint8_t p12[3];
    uint8_t p22[3];
    uint8_t p32[3];

    uint8_t p03[3];
    uint8_t p13[3];
    uint8_t p23[3];
    uint8_t p33[3];
    
    // imparte imaginea in patru??
    
    // de vazut ce se intampla la bicubic, poate asa e acel algoritm
    
    // 1st row
    get_pixel_clamped(source_image, xint - 1, yint - 1, p00);
    get_pixel_clamped(source_image, xint + 0, yint - 1, p10);
    get_pixel_clamped(source_image, xint + 1, yint - 1, p20);
    get_pixel_clamped(source_image, xint + 2, yint - 1, p30);

    // 2nd row
    get_pixel_clamped(source_image, xint - 1, yint + 0, p01);
    get_pixel_clamped(source_image, xint + 0, yint + 0, p11);
    get_pixel_clamped(source_image, xint + 1, yint + 0, p21);
    get_pixel_clamped(source_image, xint + 2, yint + 0, p31);

    // 3rd row
    get_pixel_clamped(source_image, xint - 1, yint + 1, p02);
    get_pixel_clamped(source_image, xint + 0, yint + 1, p12);
    get_pixel_clamped(source_image, xint + 1, yint + 1, p22);
    get_pixel_clamped(source_image, xint + 2, yint + 1, p32);

    // 4th row
    get_pixel_clamped(source_image, xint - 1, yint + 2, p03);
    get_pixel_clamped(source_image, xint + 0, yint + 2, p13);
    get_pixel_clamped(source_image, xint + 1, yint + 2, p23);
    get_pixel_clamped(source_image, xint + 2, yint + 2, p33);

    // dupa ce A, B, C, D le intinde sa fie ok

    // interpolate bi-cubically
    for (i = 0; i < 3; i++) {

        float col0 = cubic_hermite(p00[i], p10[i], p20[i], p30[i], xfract);
        float col1 = cubic_hermite(p01[i], p11[i], p21[i], p31[i], xfract);
        float col2 = cubic_hermite(p02[i], p12[i], p22[i], p32[i], xfract);
        float col3 = cubic_hermite(p03[i], p13[i], p23[i], p33[i], xfract);

        float value = cubic_hermite(col0, col1, col2, col3, yfract);

        CLAMP(value, 0.0f, 255.0f);

        sample[i] = (uint8_t)value; // pune iar pixelul intins
    }
}

// again, astea sunt ale lor, helparele, API-ul pe care o sa-l folosesc
// biblioteca, na, si nu cred c-ar trebuie sa mi bat capul cu ea

// totusi... care e faza cu transformarea bicubica??

// Sources:
// [1] https://stackoverflow.com/questions/2693631/read-ppm-file-and-store-it-in-an-array-coded-with-c
// [2] https://stackoverflow.com/questions/34622717/bicubic-interpolation-in-c