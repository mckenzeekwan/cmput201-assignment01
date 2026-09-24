#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 300  // max size for storing file path strings

int main(int argc, char *argv[]) {  // argc counts cmmd-line arg, argv stores arg as an array of strings
    if (argc != 2) {  // expects program name and <list_file>
        fprintf(stderr, "Usage: %s <list_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE *list_file = fopen(argv[1], "r");  // argv[1] = textfile, list_file = file pointer variable 
    if (list_file == NULL) {  // if file doesn't exist
        perror("Error opening list file");
        return EXIT_FAILURE;
    }

    char filepath[MAX_PATH];

    // count files
    int total = 0;
    while (fscanf(list_file, "%299s", filepath) == 1) {
        total++;
    }
    rewind(list_file);  // reset file pointer to the beginning

    printf("Processing %d schedule files...\n", total);

    // read each schedule file path line by line
    int count = 0;
    while (fscanf(list_file, "%299s", filepath) == 1) {
        count++;

        // find start of filename
        char *base = filepath;
        for (char *p = filepath; *p != '\0'; p++) {
            if (*p == '/') {
                base = p + 1;
            }
        }

        // "101E.txt" -> route_id = "101", direction = 'E'
        // "110XN.txt" -> route_id = "110X", direction = 'N'
        char route_id[16];
        char direction;

        if (sscanf(base, "route_%15[^_]_%c", route_id, &direction) != 2) {
            fprintf(stderr, "Could not parse filename: %s\n", filepath);
            continue;
        }

        //open individual route schedule file
        FILE *schedule_file = fopen(filepath, "r");
        if (schedule_file == NULL) {
            fprintf(stderr, "Error opening schedule file: %s\n", filepath);
            continue;
        }

        // header: "<int> <route_name>, <int> Stops"
        char line[256];
        int num_stops;
        if (fgets(line, sizeof line, schedule_file) == NULL || sscanf(line, "%*s %*[^,], %d", &num_stops) != 1) {
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