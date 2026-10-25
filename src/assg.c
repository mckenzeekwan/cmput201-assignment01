#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 300
#define MAX_ROUTES 300
#define MAX_STOPS_PER_ROUTE 100
#define MAX_NAME 100
#define MAX_ROUTE_ID 16
#define MAX_TAPS 2000
#define MAX_MATCHES 14

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

// pairing entry/exit taps
int route_has_both_stops(int route_idx, int entry_id, int exit_id) {
    int has_entry = 0, has_exit = 0;
    for (int s = 0; s < route_num_stops[route_idx]; s++) {
        if (stop_ids[route_idx][s] == entry_id) has_entry = 1;
        if (stop_ids[route_idx][s] == exit_id) has_exit = 1;
    }
    return has_entry && has_exit;
}

// search all routes and find matches woth every route index that contains both stops - returns how many were find
int find_matching_routes(int entry_id, int exit_id, int matches[]) {
    int count = 0;
    for (int r = 0; r < num_routes && count < MAX_MATCHES; r++) {
        if (route_has_both_stops(r, entry_id, exit_id)) {
            matches[count] = r;
            count++;
        }
    }
    return count;
}

// find stop name
void find_stop_name(int route_idx, int stop_id, char *out) {
    for (int s = 0; s < route_num_stops[route_idx]; s++) {
        if (stop_ids[route_idx][s] == stop_id) {
            int j = 0;
            while (stop_names[route_idx][s][j] != '\0') {
                out[j] = stop_names[route_idx][s][j];
                j++;
            }
            out[j] = '\0';  // null-terminate the string
            return;
        }
    }
    out[0] = '\0';  // if not found, return empty string
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

    // load route/schedule data
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
        if (tap_location[i] != -1 && !stop_exists(tap_location[i])) {
            printf("Invalid stop ID: %d (tap #%d)\n", tap_location[i], i);
            fclose(tap_history);
            return EXIT_FAILURE;
        }
    }
    fclose(tap_history);

    // reconstructing trips
    int pending = -1; // 'E' index waiting for its matching exit

    // looping backward through taps[] to process in proper order
    for (int idx = num_taps - 1; idx >= 0; idx--) {

        if (tap_type[idx] == 'E') {
            pending = idx;
            continue;
        }

        if (tap_type[idx] == 'X') {
            int entry_stop = tap_location[pending];
            int exit_stop = tap_location[idx];

            int matches[MAX_MATCHES];
            int match_count = find_matching_routes(entry_stop, exit_stop, matches);

            printf("Trip: entry stop %d -> exit stop %d\n", entry_stop, exit_stop);
            printf("Matching routes: %d\n", match_count);
            for (int m = 0; m < match_count; m++) {
                char entry_name[MAX_NAME], exit_name[MAX_NAME];
                find_stop_name(matches[m], entry_stop, entry_name);
                find_stop_name(matches[m], exit_stop, exit_name);
            printf("Route %s % c: %s -> %s\n", route_ids[matches[m]], route_dirs[matches[m]], entry_name, exit_name);
            }
        }
        // missing type handled later

        pending = -1;
    }
    return 0;
}