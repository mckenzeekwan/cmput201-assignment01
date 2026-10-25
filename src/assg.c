#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH 300
#define MAX_ROUTES 300
#define MAX_STOPS_PER_ROUTE 100
#define MAX_NAME 100
#define MAX_ROUTE_ID 16
#define MAX_TAPS 2000
#define MAX_MATCHES 14
#define MAX_HISTORY 2000

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

int hist_entry_stop[MAX_HISTORY];
int hist_exit_stop[MAX_HISTORY];
int hist_entry_hour[MAX_HISTORY];
int hist_count = 0;

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
        n = sscanf(line, "%d-%d-%d,%d:%d %2[AMP],%c", &month, &day, &yr, &hh, &mm, ampm, &type);
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

// check for leap year
int is_leap(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// convert full date+time into total minutes since year 1
long total_minutes(int y, int m, int d, int h, int mi) {
    int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    long days = 0;

    for (int yy = 1; yy < y; yy++) {
        days += is_leap(yy) ? 366 : 365;
    }
    for (int mm = 1; mm <m; mm++) {
        days += days_in_month[mm - 1];
        if (mm == 2 && is_leap(y)) {
            days += 1;
        }
    }
    days += d;
    return days * 24L * 60L + h * 60L + mi;
}

void format_iso(int y, int m, int d, int h, int mi, char *out) {
    sprintf(out, "%04d-%02d-%02dT%02d:%02d:00", y, m, d, h, mi);
}

void format_time(long minutes, char *out) {
    long total_seconds = minutes * 60;
    long hrs = total_seconds / 3600;
    long mins = (total_seconds % 3600) / 60;
    long secs = total_seconds % 60;
    sprintf(out, "%ld:%02ld:%02ld", hrs, mins, secs);
}

// guess algorithm 
int guess_exit(int entry_stop, int entry_hour) {
    int candidate_stops[MAX_HISTORY];
    int candidate_counts[MAX_HISTORY];
    int num_candidates = 0;

    for (int i = 0; i < hist_count; i++) {
        if (hist_entry_stop[i] != entry_stop) continue;
        if (hist_entry_hour[i] != entry_hour) continue;

        // check if exit stop has been seen before
        int found = 0;
        for (int j = 0; j < num_candidates; j++) {
            if (candidate_stops[j] == hist_exit_stop[i]) {
                candidate_counts[j]++;
                found = 1;
                break;
            }
        }
        if (!found) {
            candidate_stops[num_candidates] = hist_exit_stop[i];
            candidate_counts[num_candidates] = 1;
            num_candidates++;
        }

    }

    if (num_candidates == 0) return -1; // no matching history exists

    // pick highest count, smaller stopID for ties
    int best = 0;
    for (int j = 1; j < num_candidates; j++) {
        if (candidate_counts[j] > candidate_counts[best] || (candidate_counts[j] == candidate_counts[best] && candidate_stops[j] < candidate_stops[best])) {
            best = j;
        }
    }
    return candidate_stops[best];
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
            printf("Invalid data format. Failed on line %d: [%s]\n", i, line);
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
    FILE *out = fopen("output.txt", "w");
    if (out == NULL) {
        perror("Errpr openig output file");
        return EXIT_FAILURE;
    }


    int pending = -1; // 'E' index waiting for its matching exit

    // looping backward through taps[] to process in proper order
    for (int idx = num_taps - 1; idx >= 0; idx--) {

        if (tap_type[idx] == 'E') {
            pending = idx;
            continue;
        }

        if (pending == -1) {
            continue;
        }

        if (tap_type[idx] == 'X' || tap_type[idx] == 'M') {
            int entry_stop = tap_location[pending];
            int exit_stop;
            int guessed = 0;

            if (tap_type[idx] == 'X') {
                exit_stop = tap_location[idx];

                if (hist_count < MAX_HISTORY) {
                    hist_entry_stop[hist_count] = entry_stop;
                    hist_exit_stop[hist_count] = exit_stop;
                    hist_entry_hour[hist_count] = tap_hour[pending];
                    hist_count++;
                }
            } else {
                exit_stop = guess_exit(entry_stop, tap_hour[pending]);
                guessed = 1;
                if (exit_stop == -1) {
                    pending = -1;
                    continue;
                }
            }


            int matches[MAX_MATCHES];
            int match_count = find_matching_routes(entry_stop, exit_stop, matches);

            if (match_count == 1) {
                int r = matches[0];

                char entry_name[MAX_NAME], exit_name[MAX_NAME];
                find_stop_name(r, entry_stop, entry_name);
                find_stop_name(r, exit_stop, exit_name);

                char entry_iso[32], exit_iso[32], dur[32];
                format_iso(tap_year[pending], tap_month[pending], tap_day[pending], tap_hour[pending], tap_minute[pending], entry_iso);
                format_iso(tap_year[idx], tap_month[idx], tap_day[idx], tap_hour[idx], tap_minute[idx], exit_iso);

                long entry_total = total_minutes(tap_year[pending], tap_month[pending], tap_day[pending], tap_hour[pending], tap_minute[pending]);
                long exit_total = total_minutes(tap_year[idx], tap_month[idx], tap_day[idx], tap_hour[idx], tap_minute[idx]);
                format_time(exit_total - entry_total, dur);

                fprintf(out, "%s %c,%s,%s,%s,%s,%s\n", route_ids[r], route_dirs[r], entry_name, exit_name, entry_iso, exit_iso, dur);
                if (guessed) fprintf(out, ",Guessed");
                fprintf(out, "\n");

            } else if (match_count > 1) {
                char entry_name[MAX_NAME], exit_name[MAX_NAME];
                find_stop_name(matches[0], entry_stop, entry_name);
                find_stop_name(matches[0], exit_stop, exit_name);

                printf("Ambiguous trip detected:\n");
                printf("Entry: %d (%s)\n", entry_stop, entry_name);
                printf("Exit: %d (%s)\n", exit_stop, exit_name);
                printf("Possible Routes:\n");
                for (int m = 0; m < match_count; m++) {
                    printf("%d. %s %c\n", m + 1, route_ids[matches[m]], route_dirs[matches[m]]);

                }
                printf("Select route (1-%d): ", match_count);

                int choice;
                scanf("%d", &choice);

                if (choice >= 1 && choice <= match_count) {
                    int r = matches[choice - 1];

                    char chosen_entry_name[MAX_NAME], chosen_exit_name[MAX_NAME];
                    find_stop_name(r, entry_stop, chosen_entry_name);
                    find_stop_name(r, exit_stop, chosen_exit_name);

                    char entry_iso[32], exit_iso[32], dur[32];
                    format_iso(tap_year[pending], tap_month[pending], tap_day[pending], tap_hour[pending], tap_minute[pending], entry_iso);
                    format_iso(tap_year[idx], tap_month[idx], tap_day[idx], tap_hour[idx], tap_minute[idx], exit_iso);

                    long entry_total = total_minutes(tap_year[pending], tap_month[pending], tap_day[pending], tap_hour[pending], tap_minute[pending]);
                    long exit_total = total_minutes(tap_year[idx], tap_month[idx], tap_day[idx], tap_hour[idx], tap_minute[idx]);
                    format_time(exit_total - entry_total, dur);

                    fprintf(out, "%s %c,%s,%s,%s,%s,%s\n", route_ids[r], route_dirs[r], chosen_entry_name, chosen_exit_name, entry_iso, exit_iso, dur);
                    if (guessed) fprintf(out, ",Guessed");
                    fprintf(out, "\n");
                }
            }

        }
  

        pending = -1;
    }
    fclose(out);
    return 0;
}