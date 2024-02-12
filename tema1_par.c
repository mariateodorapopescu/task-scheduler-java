// Author: APD team, except where source was noted
// Popescu Maria Teodora, 332CC
// I also added comments between the lines
#include "helpers.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

// I had to encapsulate the arguments given to the thread, 
// I couldn't give just one so I made a structure
typedef struct ThreadArgs{
    ppm_image* image;
    ppm_image* scaled_image;
    int start;
    int end;
} ThreadArgs;

// From the example from java version of the threads, I wanted to try something
// This is a class/object-oriented wannabe version but it's simply plain C.
typedef struct Runnable {
    pthread_t thread;
    ThreadArgs args;
}Runnable;

// Creates a map between the binary configuration (e.g. 0110_2) and the corresponding pixels
// that need to be set on the output image. An array is used for this map since the keys are
// binary numbers in 0-15. Contour images are located in the './contours' directory.
// ---------------------------------------------------------------
// I did not modify this one because I did not find something to make the processing take long
ppm_image **init_contour_map() {
    ppm_image **map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!map) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "./contours/%d.ppm", i);
        map[i] = read_ppm(filename);
    }

    return map;
}

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
// ---------------------------------------------------------------
// I did not modify this because the very extensive processing takes place at rescale
void update_image(ppm_image *image, ppm_image *contour, int x, int y) {
    for (int i = 0; i < contour->x; i++) {
        for (int j = 0; j < contour->y; j++) {
            int contour_pixel_index = contour->x * i + j;
            int image_pixel_index = (x + i) * image->y + y + j;

            image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
            image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
            image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
        }
    }
}

// So, there I took the original rescale function, and all I did to modify was the values from for
// according to a formula I found.
// Moreover, it is inspired after the labs in which I had to find some start and end values 
// for the parallelised fors according to the threads/number of threads
void* thread_function(void* args) {
    uint8_t sample[3];
    ThreadArgs* cv = (ThreadArgs*)args;
    ppm_image* image = cv->image;
    ppm_image* new_image = cv->scaled_image;

    for (int i = 0; i < new_image->x; i++) {
        for (int j = cv->start; j < cv->end; j++) {  // Used new_image->y
            float u = (float)i / (float)(new_image->x - 1);
            float v = (float)j / (float)(new_image->y - 1);
            sample_bicubic(image, u, v, sample);  // Used image instead of cv->image

            new_image->data[i * new_image->y + j].red = sample[0];
            new_image->data[i * new_image->y + j].green = sample[1];
            new_image->data[i * new_image->y + j].blue = sample[2];
        }
    }

    return NULL;
}

// there is missing the rescale function because I made it with threads and I have to move it in main

// Corresponds to step 1 of the marching squares algorithm, which focuses on sampling the image.
// Builds a p x q grid of points with values which can be either 0 or 1, depending on how the
// pixel values compare to the `sigma` reference value. The points are taken at equal distances
// in the original image, based on the `step_x` and `step_y` arguments.
// ---------------------------------------------------------------
// While trying to parralelise the functions independently and tested the speedup for each one,
// I noticed that parallelising the rescale function was optimal, so everything else is unchanged
unsigned char **sample_grid(ppm_image *image, int step_x, int step_y, unsigned char sigma) {
    int p = image->x / step_x;
    int q = image->y / step_y;

    unsigned char **grid = (unsigned char **)malloc((p + 1) * sizeof(unsigned char*));
    if (!grid) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i <= p; i++) {
        grid[i] = (unsigned char *)malloc((q + 1) * sizeof(unsigned char));
        if (!grid[i]) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }
    }

    for (int i = 0; i < p; i++) {
        for (int j = 0; j < q; j++) {
            ppm_pixel curr_pixel = image->data[i * step_x * image->y + j * step_y];

            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > sigma) {
                grid[i][j] = 0;
            } else {
                grid[i][j] = 1;
            }
        }
    }
    grid[p][q] = 0;

    // last sample points have no neighbours below / to the right, so we use pixels on the
    // last row / column of the input image for them
    for (int i = 0; i < p; i++) {
        ppm_pixel curr_pixel = image->data[i * step_x * image->y + image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma) {
            grid[i][q] = 0;
        } else {
            grid[i][q] = 1;
        }
    }
    for (int j = 0; j < q; j++) {
        ppm_pixel curr_pixel = image->data[(image->x - 1) * image->y + j * step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > sigma) {
            grid[p][j] = 0;
        } else {
            grid[p][j] = 1;
        }
    }

    return grid;
}

// Corresponds to step 2 of the marching squares algorithm, which focuses on identifying the
// type of contour which corresponds to each subgrid. It determines the binary value of each
// sample fragment of the original image and replaces the pixels in the original image with
// the pixels of the corresponding contour image accordingly.
// ---------------------------------------------------------------
// No change from the original from the skeleton
void march(ppm_image *image, unsigned char **grid, ppm_image **contour_map, int step_x, int step_y) {
    int p = image->x / step_x;
    int q = image->y / step_y;

    for (int i = 0; i < p; i++) {
        for (int j = 0; j < q; j++) {
            unsigned char k = 8 * grid[i][j] + 4 * grid[i][j + 1] + 2 * grid[i + 1][j + 1] + 1 * grid[i + 1][j];
            update_image(image, contour_map[k], i * step_x, j * step_y);
        }
    }
}

// Calls `free` method on the used resources.
// ---------------------------------------------------------------
// No change, it is the same as the skeleton.
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x) {
    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        free(contour_map[i]->data);
        free(contour_map[i]);
    }
    free(contour_map);

    for (int i = 0; i <= image->x / step_x; i++) {
        free(grid[i]);
    }
    free(grid);

    free(image->data);
    free(image);
}

int main(int argc, char *argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Usage: ./tema1 <in_file> <out_file>\n");
        return 1;
    }

    ppm_image *image = read_ppm(argv[1]);
    int step_x = STEP;
    int step_y = STEP;

    // 0. Initialise contour map
    ppm_image **contour_map = init_contour_map();

    // 0.5. Extract the number of threads used from arguments, according to the number of args
    int num_threads;
    if (argc >= 4) {
        num_threads = atoi(argv[3]);
    } else {
        fprintf(stderr, "Arg no.3 is not ok!\n");
        return 1;
    }

    // 1. Rescale the image
    ppm_image *scaled_image = (ppm_image*)malloc(sizeof(ppm_image));

    int ok = 1; // I assume I need rescaling

    // we only rescale downwards -> from the skeleton
    if (image->x <= RESCALE_X && image->y <= RESCALE_Y) {
        ok = 0;
    }

    if (ok == 1) // if I need to rescale the image
    {   scaled_image->x = RESCALE_X;
        scaled_image->y = RESCALE_Y;

        scaled_image->data = (ppm_pixel*)malloc(scaled_image->x * scaled_image->y * sizeof(ppm_pixel));
        if (!scaled_image->data) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }

        // inspired from the java practical course + java threads in general
        Runnable* runnables = (Runnable*)malloc(num_threads * sizeof(Runnable)); 

        for (int ID = 0; ID < num_threads; ID++) {
            runnables[ID].args.start = ID * (double)scaled_image->y / num_threads;
            runnables[ID].args.end = fmin((ID + 1) * (double)scaled_image->y / num_threads, scaled_image->y);
            runnables[ID].args.image = image;
            runnables[ID].args.scaled_image = scaled_image;

            pthread_create(&runnables[ID].thread, NULL, &thread_function, &runnables[ID].args);
        }

        for (int i = 0; i < num_threads; i++) {
            pthread_join(runnables[i].thread, NULL);
        }
        free(runnables);
    } else {
        scaled_image = image;
    }

    // 2. Sample the grid
    // ---------------------------------------------------------------
    // it is done according to scaled_image which is either the rescaled version or the original
    unsigned char **grid = sample_grid(scaled_image, step_x, step_y, SIGMA);

    // 3. March the squares
    march(scaled_image, grid, contour_map, step_x, step_y);

    // 4. Write output
    write_ppm(scaled_image, argv[2]);

    free_resources(scaled_image, contour_map, grid, step_x);

    return 0;
}
