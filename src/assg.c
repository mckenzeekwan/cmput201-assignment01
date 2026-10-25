#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 300
#define MAX_ROUTES 300
#define MAX_STOPS_PER_ROUTE 100
#define MAX_NAME 100
#define MAX_ROUTE_ID 16
#define MAX_TAPS 2000

// creating my arrays
char route_ids[MAX_ROUTES][MAX_ROUTE_ID];
char route_dirs[MAX_ROUTES];
int route_num_stops[MAX_ROUTES];
int stop_ids[MAX_ROUTES][MAX_STOPS_PER_ROUTE];
char stop_names[MAX_ROUTES][MAX_STOPS_PER_ROUTE][MAX_NAME];

int num_routes = 0; // how many routes are filled in

//tap data
int tap_year[MAX_TAPS], tap_month[MAX_TAPS], tap_day[MAX_TAPS];
int tap_hour[MAX_TAPS], tap_minute[MAX_TAPS];
char tap_type[MAX_TAPS];
int tap_location[MAX_TAPS];
int num_taps = 0;

// does the stop ID exist in any of the route's stop lists
int stop_exists(int stop_id) {
    for (int r = 0; r < num_routes; r++) {
        for (int s = 0; s < route_num_stops[r]; s++) {
            if (stop_ids[r][s] == stop_id) return 1;
        }
    }
    return 0;
}

//12 hour-time and AM/PM 24hr time
int hour24(int h, char ampm0) {
    if (ampm0 == 'A') return (h == 12) ? 0 : h;
    return (h == 12) ? 12 : h + 12;
}

//parses tap history line by line into row i of tap arrays
int parse_tap(char *line, int i) {
    int month, day, yr, hh, mm, loc;
    char ampm[3], type;

    // date, time, type, location
    int n = sscanf(line, "%d-%d-%d,%d:%d %2[AMP],%c,%d", &month, &day, &yr, &hh, &mm, ampm, &type, &loc);

    if (n != 8) {
        // try blank location format (if it is missing tap fare
        n = sscanf(line, "%d%d-%d,%d:%d %2[AMP],%c", &month, &day, &yr, &hh, &mm, ampm, &type);
        if (n != 7) return 0; 
        if (type != 'M') return 0;
        loc = -1;
    }

    if (month < 1 || month > 12) return 0;
    if (day < 1 || day > 31) return 0;
    if (hh < 1 || hh > 12) return 0;
    if (mm < 0 || mm > 59) return 0;
    if (type != 'E' && type != 'X' && type != 'M') return 0;
    if (ampm[0] != 'A' && ampm[0] != 'P') return 0;

    tap_month[i] = month;
    tap_day[i] = day;
    tap_year[i] = yr;
    tap_hour[i] = hour24(hh, ampm[0]);
    tap_minute[i] = mm;
    tap_type[i] = type;
    tap_location[i] = loc;
    return 1;
}

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

    // load tap history
    char line[256];

    // first line = record count
    if (fgets(line, sizeof line, tap_history) == NULL || sscanf(line, "%d", &num_taps) != 1) {
        printf("Error reading tap history file, invalid data format.\n");
        fclose(tap_history);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < num_taps && i < MAX_TAPS; i++) {
        if (fgets(line, sizeof line, tap_history) == NULL || !parse_tap(line, i)) {
            printf("Invalid data format.\n");
            fclose(tap_history);
            return EXIT_FAILURE;
        }
        // when a blank location (-1) is only for type 'M' and has no stop to check
        if (tap_location[i] != -1 && !stop_exists(tap_location[1])) {
            printf("Invalid stop ID\n");
            fclose(tap_history);
            return EXIT_FAILURE;
        }
    }
    fclose(tap_history);

    printf("Loaded %d routes and %d taps.\n", num_routes, num_taps);
    for (int i = 0; i < num_taps; i++) {
        printf("Tap %d: %04d-%02d-%02d %02d:%02d type=%c loc=%d\n", i, tap_year[i], tap_month[i], tap_day[i], tap_hour[i], tap_minute[i], tap_type[i], tap_location[i]);
    }
    return 0;
}