#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 300  // max size for storing file path strings

int main(int argc, char *argv[]) { 
    if (argc != 2) {  // expects program name and <list_file>
        fprintf(stderr, "Usage: %s <list_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // OPEN LIST FILES
    FILE *list_file = fopen(argv[1], "r");  // argv[1] = textfile, list_file = file pointer variable 
    if (list_file == NULL) {  // if file doesn't exist
        perror("Error opening list file");
        return EXIT_FAILURE;
    }


    // COUNT FILES IN LIST
    char filepath[MAX_PATH];
    int total = 0;
    while (fscanf(list_file, "%299s", filepath) == 1) {  // stores word it just read into filepath, %s means read until whitespace(inc newline)
        total++;
    }
    rewind(list_file);  // reset file pointer to the beginning

    // START PROCESSING LOOP
    printf("Processing %d schedule files...\n", total);

    // read each schedule file path line by line
    int count = 0;
    while (fscanf(list_file, "%299s", filepath) == 1) {
        count++;

        // EXTRACTING FILE NAME FROM PATH - dont use pointers
        char *base = filepath;  // creates pointer (base) to the start of file path string
        for (char *p = filepath; *p != '\0'; p++) {  // loops through every character in filepath until the string ends
            if (*p == '/') {
                base = p + 1;  // update base to point to character after the slash
            }
        }

        // PARSING ROUTE ID AND DIRECTION FROM THE FILENAME
        // "route_1_EB.txt"
        char route_id[16];
        char direction;

        if (sscanf(base, "route_%15[^_]_%c", route_id, &direction) != 2) {
            fprintf(stderr, "Could not parse filename: %s\n", filepath);
            continue;
        }

        // OPEN INDIVIDUAL ROUTE SCHEDULE FILE
        FILE *schedule_file = fopen(filepath, "r");
        if (schedule_file == NULL) {
            fprintf(stderr, "Error opening schedule file: %s\n", filepath);
            continue;
        }


        // READING FILE HEADER AND NUMBER OF STOPS
        // header: "<int> <route_name>, <int> Stops"
        char line[256];
        int num_stops;
        if (fgets(line, 256, schedule_file) == NULL || sscanf(line, "%*s %*[^,], %d", &num_stops) != 1) {  // try not to use caret
            fprintf(stderr, "Bad header in %s\n", filepath);
            fclose(schedule_file);
            continue;
        }
        
        printf("schedule #%d is route %s it has %d stops and is in the %c direction\n", count, route_id, num_stops, direction);

        fclose(schedule_file);
    }

    fclose(list_file);
    return 0;
}