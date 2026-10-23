#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 300  // max size for storing file path strings
#define MAX_ROUTES 300
#define MAX_STOPS_PER_ROUTE 100
#define MAX_TAPS 2000
#define MAX_LINE 256
#define MAX_NAME 100
#define MAX_ROUTE_ID 16
#define MAX_MATCHES 14
#define MAX_HISTORY 2000
#define MAX_GUESS_CANDIDATES 200

// creating my arrays
char route_ids[MAX_ROUTES][MAX_ROUTE_ID];
char route_dirs[MAX_ROUTES];
int route_num_stops[MAX_ROUTES];
int stop_ids[MAX_ROUTES][MAX_STOPS_PER_ROUTE];
char stop_names[MAX_ROUTES][MAX_STOPS_PER_ROUTE][MAX_NAME];

int num_routes = 0; // how many routes are filled in

int main(int argc, char *argv[]) { 
    if (argc != 3) {  // expects program name, list file and tap history
        fprintf(stderr, "Usage: %s <list_file> <tap_history>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // opening the list files
    FILE *list_file = fopen(argv[1], "r"); 
    if (list_file == NULL) {  
        perror("Error opening list file");
        return EXIT_FAILURE;
    }

    FILE *tap_history = fopen(argv[2], "r");  
    if (tap_history == NULL) {  
        perror("Error opening tap history file");
        fclose(list_file); 
        return EXIT_FAILURE;
    }

    // printf("Both files opened successfully. \n");

    // COUNT FILES IN LIST
    char filepath[MAX_PATH];
    int total = 0;
    while (fscanf(list_file, "%299s", filepath) == 1) {  // stores word it just read into filepath, %s means read until whitespace(inc newline)
        total++;
    }
    rewind(list_file);  // reset file pointer to the beginning

    // printf("Processing %d schedule files...\n", total);

    // read each schedule file path line by line
    // int count = 0;
    while (num_routes < MAX_ROUTES && fscanf(list_file, "%299s", filepath) == 1) {
        // count++;

        // EXTRACTING FILE NAME FROM PATH - dont use pointers
        char *base = filepath;  // creates pointer (base) to the start of file path string
        for (char *p = filepath; *p != '\0'; p++) {  // loops through every character in filepath until the string ends
            if (*p == '/') {
                base = p + 1;  // update base to point to character after the slash
            }
        }

        // PARSING ROUTE ID AND DIRECTION FROM THE FILENAME
        // "route_1_EB.txt"
        char route_id[MAX_ROUTE_ID];
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
        if (fgets(line, 256, schedule_file) == NULL || 
            sscanf(line, "%*s %*[^,], %d", &num_stops) != 1) {  // try not to use caret
            fprintf(stderr, "Bad header in %s\n", filepath);
            fclose(schedule_file);
            continue;
        }
        
        // printf("schedule #%d is route %s it has %d stops and is in the %c direction\n", count, route_id, num_stops, direction);

        int k = 0;
        int sid;
        char sname[MAX_NAME];
        while (k < MAX_STOPS_PER_ROUTE &&
            fgets(line, sizeof line, schedule_file) != NULL) {
                if (sscanf(line, "%d,%99[^\r\n]", &sid, sname) == 2) {
                    stop_ids[num_routes][k] = sid;
                    
                    int j = 0;
                    while (sname[j] != '\0' && j < MAX_NAME - 1) {
                        stop_names[num_routes][k][j] = sname[j];
                        j++;
                    }
                    stop_names[num_routes][k][j] = '\0';  // null-terminate the string
                    k++;
                }
            }
        int j = 0;
        while (route_id[j] != '\0' && j < MAX_ROUTE_ID - 1) {
            route_ids[num_routes][j] = route_id[j];
            j++;
        }
        route_ids[num_routes][j] = '\0';
        route_dirs[num_routes] = direction;
        route_num_stops[num_routes] = k;

        num_routes++;

        fclose(schedule_file);
    }

    fclose(list_file);

    printf("Loaded %d routes.\n", num_routes);
    for (int r=0; r < num_routes && r < 3; r++) {
        printf("Route %s %c has %d stops. First stop: %d (%s)\n", route_ids[r], route_dirs[r], route_num_stops[r], stop_ids[r][0], stop_names[r][0]);
    }
    fclose(tap_history);
    return 0;
}