#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <direct.h>

#define MAX_NAME_LEN 64
#define MAX_RIDES 500
#define MAX_TRIP_RIDES 20
#define MAX_PASSENGERS 50
#define MAX_LINE_LEN 256
#define RECEIPT_DIR "receipts"

typedef struct {
    double base_total;
    double paid_total;
} Totals;

typedef struct {
    int ride_no;
    int passenger_no;
    int trip_no;
    const char *line_name;
    int discount_type;
    double base_fare;
    double final_fare;
    char detail[128];
} RideSummary;

typedef struct {
    const char *line_name;
    int discount_type;
    double base_fare;
    double final_fare;
    char detail[128];
} TripRide;


typedef struct {
    char name[MAX_NAME_LEN];
    Totals totals;
    int used_card;
    double start_balance;
    double end_balance;
    double topup_total;
} PassengerSummary;

typedef struct {
    int short_max;
    int medium_max;
    int long_max;
    double short_fare;
    double medium_fare;
    double long_fare;
} Lrt1Fare;

typedef struct {
    int short_max;
    int medium_max;
    double short_fare;
    double medium_fare;
    double long_fare;
} Lrt2Fare;

typedef struct {
    int max1;
    int max2;
    int max3;
    int max4;
    int max5;
    double fare1;
    double fare2;
    double fare3;
    double fare4;
    double fare5;
} Mrt3Fare;

typedef struct {
    Lrt1Fare lrt1;
    Lrt2Fare lrt2;
    Mrt3Fare mrt3;
} FareConfig;

typedef struct {
    int from_line;
    int from_station;
    int to_line;
    int to_station;
    const char *note;
} TransferRule;

static const char *LRT1_STATIONS[] = {
    "Baclaran", "EDSA", "Libertad", "Gil Puyat", "Vito Cruz", "Quirino",
    "Pedro Gil", "UN Ave", "Central", "Carriedo", "Doroteo Jose", "Bambang",
    "Tayuman", "Blumentritt", "Abad Santos", "R. Papa", "5th Ave",
    "Monumento", "Balintawak", "Roosevelt (FPJ)"
};

static const char *LRT2_STATIONS[] = {
    "Recto", "Legarda", "Pureza", "V. Mapa", "J. Ruiz", "Gilmore",
    "Betty Go-Belmonte", "Cubao", "Anonas", "Katipunan", "Santolan",
    "Marikina", "Antipolo"
};

static const char *MRT3_STATIONS[] = {
    "North Ave", "Quezon Ave", "Kamuning", "Cubao", "Santolan-Annapolis",
    "Ortigas", "Shaw", "Boni", "Guadalupe", "Buendia", "Ayala",
    "Magallanes", "Taft"
};

static const TransferRule TRANSFER_RULES[] = {
    {1, 10, 2, 0, "Doroteo Jose (LRT-1) to Recto (LRT-2)"},
    {2, 0, 1, 10, "Recto (LRT-2) to Doroteo Jose (LRT-1)"},
    {3, 12, 1, 1, "Taft (MRT-3) to EDSA (LRT-1)"},
    {1, 1, 3, 12, "EDSA (LRT-1) to Taft (MRT-3)"},
    {2, 7, 3, 3, "Cubao (LRT-2) to Cubao (MRT-3)"},
    {3, 3, 2, 7, "Cubao (MRT-3) to Cubao (LRT-2)"}
};

static const char *line_label(int line) {
    return (line == 1) ? "LRT-1" : (line == 2) ? "LRT-2" : "MRT-3";
}

static void print_station_list(const char *title, const char *stations[], int count) {
    int i;
    printf("\n%s stations:\n", title);
    for (i = 0; i < count; i++) {
        printf("%2d. %s\n", i + 1, stations[i]);
    }
}

static void trim_newline(char *text) {
    size_t len = strlen(text);
    if (len > 0 && text[len - 1] == '\n') {
        text[len - 1] = '\0';
    }
}

static void read_line(const char *prompt, char *buffer, size_t size) {
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, (int)size, stdin) == NULL) {
            printf("Input error. Try again.\n");
            clearerr(stdin);
            continue;
        }
        trim_newline(buffer);
        if (buffer[0] == '\0') {
            printf("Please enter a value.\n");
            continue;
        }
        return;
    }
}

static void read_line_optional(const char *prompt, char *buffer, size_t size) {
    while (1) {
        printf("%s", prompt);
        if (fgets(buffer, (int)size, stdin) == NULL) {
            printf("Input error. Try again.\n");
            clearerr(stdin);
            continue;
        }
        trim_newline(buffer);
        return;
    }
}

static int is_number_string(const char *text) {
    size_t i;
    if (text[0] == '\0') {
        return 0;
    }
    for (i = 0; text[i] != '\0'; i++) {
        if (!isdigit((unsigned char)text[i])) {
            return 0;
        }
    }
    return 1;
}

static int read_int_range(const char *prompt, int min, int max) {
    char buffer[MAX_LINE_LEN];
    while (1) {
        read_line(prompt, buffer, sizeof(buffer));
        if (!is_number_string(buffer)) {
            printf("Invalid input. Try again.\n");
            continue;
        }
        int value = (int)strtol(buffer, NULL, 10);
        if (value < min || value > max) {
            printf("Please enter a value from %d to %d.\n", min, max);
            continue;
        }
        return value;
    }
}

static double read_double_min(const char *prompt, double min) {
    char buffer[MAX_LINE_LEN];
    while (1) {
        char *end = NULL;
        read_line(prompt, buffer, sizeof(buffer));
        double value = strtod(buffer, &end);
        if (end == buffer || *end != '\0') {
            printf("Invalid input. Try again.\n");
            continue;
        }
        if (value < min) {
            printf("Please enter a value of at least %.2f.\n", min);
            continue;
        }
        return value;
    }
}

static int read_yes_no(const char *prompt) {
    char buffer[MAX_LINE_LEN];
    while (1) {
        read_line(prompt, buffer, sizeof(buffer));
        if (buffer[0] == 'y' || buffer[0] == 'Y') {
            return 1;
        }
        if (buffer[0] == 'n' || buffer[0] == 'N') {
            return 0;
        }
        printf("Please enter Y or N.\n");
    }
}

static int string_equals_ignore_case(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static int string_contains_ignore_case(const char *text, const char *pattern) {
    size_t text_len = strlen(text);
    size_t pat_len = strlen(pattern);
    if (pat_len == 0) {
        return 0;
    }
    for (size_t i = 0; i + pat_len <= text_len; i++) {
        size_t j;
        for (j = 0; j < pat_len; j++) {
            char a = (char)tolower((unsigned char)text[i + j]);
            char b = (char)tolower((unsigned char)pattern[j]);
            if (a != b) {
                break;
            }
        }
        if (j == pat_len) {
            return 1;
        }
    }
    return 0;
}

static double apply_discount(double fare, int discount_type) {
    if (discount_type == 2 || discount_type == 3 || discount_type == 4) {
        return fare * 0.5;
    }
    return fare;
}

static const char *discount_label(int discount_type) {
    switch (discount_type) {
    case 2:
        return "Student (50% off)";
    case 3:
        return "PWD (50% off)";
    case 4:
        return "Senior (50% off)";
    default:
        return "None";
    }
}

static void set_default_fares(FareConfig *config) {
    config->lrt1.short_max = 5;
    config->lrt1.medium_max = 10;
    config->lrt1.long_max = 20;
    config->lrt1.short_fare = 15.0;
    config->lrt1.medium_fare = 20.0;
    config->lrt1.long_fare = 30.0;

    config->lrt2.short_max = 5;
    config->lrt2.medium_max = 10;
    config->lrt2.short_fare = 15.0;
    config->lrt2.medium_fare = 20.0;
    config->lrt2.long_fare = 30.0;

    config->mrt3.max1 = 3;
    config->mrt3.max2 = 6;
    config->mrt3.max3 = 9;
    config->mrt3.max4 = 12;
    config->mrt3.max5 = 13;
    config->mrt3.fare1 = 13.0;
    config->mrt3.fare2 = 16.0;
    config->mrt3.fare3 = 20.0;
    config->mrt3.fare4 = 24.0;
    config->mrt3.fare5 = 28.0;
}

static int load_fares(const char *path, FareConfig *config) {
    FILE *file = fopen(path, "r");
    char line[MAX_LINE_LEN];
    if (!file) {
        return 0;
    }

    while (fgets(line, sizeof(line), file)) {
        char key[MAX_LINE_LEN];
        char value[MAX_LINE_LEN];
        trim_newline(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        if (sscanf(line, "%255[^=]=%255s", key, value) != 2) {
            continue;
        }

        if (strcmp(key, "LRT1_SHORT_MAX") == 0) {
            config->lrt1.short_max = atoi(value);
        } else if (strcmp(key, "LRT1_MEDIUM_MAX") == 0) {
            config->lrt1.medium_max = atoi(value);
        } else if (strcmp(key, "LRT1_LONG_MAX") == 0) {
            config->lrt1.long_max = atoi(value);
        } else if (strcmp(key, "LRT1_SHORT_FARE") == 0) {
            config->lrt1.short_fare = atof(value);
        } else if (strcmp(key, "LRT1_MEDIUM_FARE") == 0) {
            config->lrt1.medium_fare = atof(value);
        } else if (strcmp(key, "LRT1_LONG_FARE") == 0) {
            config->lrt1.long_fare = atof(value);
        } else if (strcmp(key, "LRT2_SHORT_MAX") == 0) {
            config->lrt2.short_max = atoi(value);
        } else if (strcmp(key, "LRT2_MEDIUM_MAX") == 0) {
            config->lrt2.medium_max = atoi(value);
        } else if (strcmp(key, "LRT2_SHORT_FARE") == 0) {
            config->lrt2.short_fare = atof(value);
        } else if (strcmp(key, "LRT2_MEDIUM_FARE") == 0) {
            config->lrt2.medium_fare = atof(value);
        } else if (strcmp(key, "LRT2_LONG_FARE") == 0) {
            config->lrt2.long_fare = atof(value);
        } else if (strcmp(key, "MRT3_RANGE1_MAX") == 0) {
            config->mrt3.max1 = atoi(value);
        } else if (strcmp(key, "MRT3_RANGE2_MAX") == 0) {
            config->mrt3.max2 = atoi(value);
        } else if (strcmp(key, "MRT3_RANGE3_MAX") == 0) {
            config->mrt3.max3 = atoi(value);
        } else if (strcmp(key, "MRT3_RANGE4_MAX") == 0) {
            config->mrt3.max4 = atoi(value);
        } else if (strcmp(key, "MRT3_RANGE5_MAX") == 0) {
            config->mrt3.max5 = atoi(value);
        } else if (strcmp(key, "MRT3_FARE1") == 0) {
            config->mrt3.fare1 = atof(value);
        } else if (strcmp(key, "MRT3_FARE2") == 0) {
            config->mrt3.fare2 = atof(value);
        } else if (strcmp(key, "MRT3_FARE3") == 0) {
            config->mrt3.fare3 = atof(value);
        } else if (strcmp(key, "MRT3_FARE4") == 0) {
            config->mrt3.fare4 = atof(value);
        } else if (strcmp(key, "MRT3_FARE5") == 0) {
            config->mrt3.fare5 = atof(value);
        }
    }

    fclose(file);
    return 1;
}

static double fare_lrt1(const FareConfig *config, int stations_traveled) {
    if (stations_traveled <= config->lrt1.short_max) {
        return config->lrt1.short_fare;
    } else if (stations_traveled <= config->lrt1.medium_max) {
        return config->lrt1.medium_fare;
    }
    return config->lrt1.long_fare;
}

static double fare_lrt2(const FareConfig *config, int stations_traveled) {
    if (stations_traveled <= config->lrt2.short_max) {
        return config->lrt2.short_fare;
    } else if (stations_traveled <= config->lrt2.medium_max) {
        return config->lrt2.medium_fare;
    }
    return config->lrt2.long_fare;
}

static double fare_mrt3(const FareConfig *config, int stations_traveled) {
    if (stations_traveled <= config->mrt3.max1) {
        return config->mrt3.fare1;
    } else if (stations_traveled <= config->mrt3.max2) {
        return config->mrt3.fare2;
    } else if (stations_traveled <= config->mrt3.max3) {
        return config->mrt3.fare3;
    } else if (stations_traveled <= config->mrt3.max4) {
        return config->mrt3.fare4;
    }
    return config->mrt3.fare5;
}

static int station_count(int from_index, int to_index) {
    int diff = from_index - to_index;
    if (diff < 0) {
        diff = -diff;
    }
    return diff;
}

static int read_station_index(const char *prompt, const char *stations[], int count) {
    char buffer[MAX_LINE_LEN];
    while (1) {
        read_line(prompt, buffer, sizeof(buffer));

        if (is_number_string(buffer)) {
            int value = atoi(buffer);
            if (value >= 1 && value <= count) {
                return value - 1;
            }
            printf("Please enter a number from 1 to %d.\n", count);
            continue;
        }

        int exact_index = -1;
        for (int i = 0; i < count; i++) {
            if (string_equals_ignore_case(buffer, stations[i])) {
                exact_index = i;
                break;
            }
        }
        if (exact_index != -1) {
            return exact_index;
        }

        int matches[MAX_LINE_LEN];
        int match_count = 0;
        for (int i = 0; i < count; i++) {
            if (string_contains_ignore_case(stations[i], buffer)) {
                matches[match_count++] = i;
            }
        }
        if (match_count == 1) {
            return matches[0];
        }
        if (match_count > 1) {
            printf("Multiple matches found:\n");
            for (int i = 0; i < match_count; i++) {
                printf("%2d. %s\n", matches[i] + 1, stations[matches[i]]);
            }
            continue;
        }

        printf("No station match. Try again.\n");
    }
}

static int find_transfer(int line, int station, TransferRule *rule) {
    int count = (int)(sizeof(TRANSFER_RULES) / sizeof(TRANSFER_RULES[0]));
    for (int i = 0; i < count; i++) {
        if (TRANSFER_RULES[i].from_line == line && TRANSFER_RULES[i].from_station == station) {
            if (rule) {
                *rule = TRANSFER_RULES[i];
            }
            return 1;
        }
    }
    return 0;
}

static void ensure_receipt_dir(void) {
    _mkdir(RECEIPT_DIR);
}

static void write_trip_receipt(const PassengerSummary *passenger,
                               int passenger_no,
                               int trip_no,
                               const TripRide *rides,
                               int ride_count,
                               double trip_base,
                               double trip_paid) {
    char filename[256];
    time_t now = time(NULL);
    struct tm *local = localtime(&now);

    ensure_receipt_dir();

    snprintf(filename, sizeof(filename),
             RECEIPT_DIR "\\receipt_%04d%02d%02d_%02d%02d%02d_P%d_T%d.txt",
             local->tm_year + 1900,
             local->tm_mon + 1,
             local->tm_mday,
             local->tm_hour,
             local->tm_min,
             local->tm_sec,
             passenger_no,
             trip_no);

    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Could not write receipt file.\n");
        return;
    }

    fprintf(file, "Train Fare Receipt\n");
    fprintf(file, "Passenger: %s\n", passenger->name);
    fprintf(file, "Trip: %d\n\n", trip_no);
    fprintf(file, "Ride | Line  | Discount           | Base Fare | Final Fare | Detail\n");
    fprintf(file, "-----+-------+--------------------+-----------+-----------+-------------------------\n");
    for (int i = 0; i < ride_count; i++) {
        fprintf(file, "%4d | %-5s | %-18s | Php %6.2f | Php %6.2f | %s\n",
                i + 1,
                rides[i].line_name,
                discount_label(rides[i].discount_type),
                rides[i].base_fare,
                rides[i].final_fare,
                rides[i].detail);
    }
    fprintf(file, "\nTrip total before discounts: Php %.2f\n", trip_base);
    fprintf(file, "Trip total after discounts: Php %.2f\n", trip_paid);
    fprintf(file, "Trip discount: Php %.2f\n", trip_base - trip_paid);

    if (passenger->used_card) {
        fprintf(file, "\nCard starting balance: Php %.2f\n", passenger->start_balance);
        fprintf(file, "Card ending balance:   Php %.2f\n", passenger->end_balance);
        fprintf(file, "Total top-up:          Php %.2f\n", passenger->topup_total);
    }

    fclose(file);
    printf("Receipt saved to %s\n", filename);
}

int main(void) {
    int choice;
    FareConfig fare_config;
    set_default_fares(&fare_config);
    int has_config = load_fares("fares.cfg", &fare_config);

    double total_base = 0.0;
    double total_paid = 0.0;
    int ride_count = 0;
    int trip_count = 0;
    RideSummary ride_summaries[MAX_RIDES];
    Totals line_totals[3] = {0};
    Totals discount_totals[4] = {0};
    PassengerSummary passenger_summaries[MAX_PASSENGERS];

    printf("Train Fare System (LRT-1, LRT-2, MRT-3)\n");
    printf("Discounts: Student, PWD, Senior = 50%% off\n");
    printf("Fare config: %s\n", has_config ? "Loaded fares.cfg" : "Using built-in defaults");

    int passenger_count = 1;
    int passenger_no = 1;
    PassengerSummary *passenger = &passenger_summaries[0];
    memset(passenger, 0, sizeof(*passenger));

        char name_input[MAX_NAME_LEN];
        read_line_optional("Passenger name (blank for default): ", name_input, sizeof(name_input));
        if (name_input[0] == '\0') {
            snprintf(passenger->name, sizeof(passenger->name), "Passenger %d", passenger_no);
        } else {
            strncpy(passenger->name, name_input, sizeof(passenger->name) - 1);
            passenger->name[sizeof(passenger->name) - 1] = '\0';
        }

        passenger->used_card = read_yes_no("Use stored-value card? (Y/N): ");
        if (passenger->used_card) {
            passenger->start_balance = read_double_min("Enter starting card balance: ", 0.0);
            passenger->end_balance = passenger->start_balance;
        }

    while (1) {
            double trip_base = 0.0;
            double trip_paid = 0.0;
            int leg_index = 0;
            int trip_ride_count = 0;
            TripRide trip_rides[MAX_TRIP_RIDES];

            printf("\nNote: transfers are charged as separate rides.\n");
            printf("\nTrip ride breakdown:\n");
            printf("Ride | Line  | Discount           | Base Fare | Final Fare | Detail\n");
            printf("-----+-------+--------------------+-----------+-----------+-------------------------\n");

            int has_forced_transfer = 0;
            int forced_line = 0;
            int forced_start = -1;

            while (trip_ride_count < MAX_TRIP_RIDES) {
                printf("\nRide %d\n", leg_index + 1);
                int line = 0;
                if (has_forced_transfer) {
                    line = forced_line;
                    printf("Transfer selected. Next line fixed to %s.\n", line_label(line));
                } else {
                    printf("Select line:\n");
                    printf("1. LRT-1 (Red line)\n");
                    printf("2. LRT-2 (Purple line)\n");
                    printf("3. MRT-3 (EDSA line)\n");
                    line = read_int_range("Line: ", 1, 3);
                }

                printf("\nDiscount type:\n");
                printf("1. None\n");
                printf("2. Student (50%% off)\n");
                printf("3. PWD (50%% off)\n");
                printf("4. Senior (50%% off)\n");
                int discount = read_int_range("Choose discount for this ride: ", 1, 4);

                double base_fare = 0.0;
                char detail[128];
                const char *line_name = line_label(line);
                int end_station = -1;

                if (line == 1) {
                    int count = (int)(sizeof(LRT1_STATIONS) / sizeof(LRT1_STATIONS[0]));
                    print_station_list("LRT-1", LRT1_STATIONS, count);
                    int from;
                    int to;
                    int stations = 0;
                    while (stations == 0) {
                        if (has_forced_transfer) {
                            from = forced_start;
                            printf("From station fixed: %s\n", LRT1_STATIONS[from]);
                        } else {
                            from = read_station_index("From station number or name: ", LRT1_STATIONS, count);
                        }
                        to = read_station_index("To station number or name: ", LRT1_STATIONS, count);
                        stations = station_count(from, to);
                        if (stations == 0) {
                            printf("From and to stations must be different.\n");
                        }
                    }
                    end_station = to;
                    base_fare = fare_lrt1(&fare_config, stations);
                    snprintf(detail, sizeof(detail), "%s -> %s (%d gaps)",
                             LRT1_STATIONS[from], LRT1_STATIONS[to], stations);
                    printf("Stations traveled (gaps): %d\n", stations);
                } else if (line == 2) {
                    int count = (int)(sizeof(LRT2_STATIONS) / sizeof(LRT2_STATIONS[0]));
                    print_station_list("LRT-2", LRT2_STATIONS, count);
                    int from;
                    int to;
                    int stations = 0;
                    while (stations == 0) {
                        if (has_forced_transfer) {
                            from = forced_start;
                            printf("From station fixed: %s\n", LRT2_STATIONS[from]);
                        } else {
                            from = read_station_index("From station number or name: ", LRT2_STATIONS, count);
                        }
                        to = read_station_index("To station number or name: ", LRT2_STATIONS, count);
                        stations = station_count(from, to);
                        if (stations == 0) {
                            printf("From and to stations must be different.\n");
                        }
                    }
                    end_station = to;
                    base_fare = fare_lrt2(&fare_config, stations);
                    snprintf(detail, sizeof(detail), "%s -> %s (%d gaps)",
                             LRT2_STATIONS[from], LRT2_STATIONS[to], stations);
                    printf("Stations traveled (gaps): %d\n", stations);
                } else {
                    int count = (int)(sizeof(MRT3_STATIONS) / sizeof(MRT3_STATIONS[0]));
                    print_station_list("MRT-3", MRT3_STATIONS, count);
                    int from;
                    int to;
                    int stations = 0;
                    while (stations == 0) {
                        if (has_forced_transfer) {
                            from = forced_start;
                            printf("From station fixed: %s\n", MRT3_STATIONS[from]);
                        } else {
                            from = read_station_index("From station number or name: ", MRT3_STATIONS, count);
                        }
                        to = read_station_index("To station number or name: ", MRT3_STATIONS, count);
                        stations = station_count(from, to);
                        if (stations == 0) {
                            printf("From and to stations must be different.\n");
                        }
                    }
                    end_station = to;
                    base_fare = fare_mrt3(&fare_config, stations);
                    snprintf(detail, sizeof(detail), "%s -> %s (%d gaps)",
                             MRT3_STATIONS[from], MRT3_STATIONS[to], stations);
                    printf("Stations traveled (gaps): %d\n", stations);
                }

                double final_fare = apply_discount(base_fare, discount);

                if (passenger->used_card) {
                    if (passenger->end_balance < final_fare) {
                        double needed = final_fare - passenger->end_balance;
                        char topup_prompt[MAX_LINE_LEN];
                        snprintf(topup_prompt, sizeof(topup_prompt),
                                 "Insufficient balance. Top-up at least %.2f: ", needed);
                        double topup = read_double_min(topup_prompt, needed);
                        passenger->topup_total += topup;
                        passenger->end_balance += topup;
                    }
                    passenger->end_balance -= final_fare;
                }

                printf("Base fare: Php %.2f\n", base_fare);
                printf("Discount: %s\n", discount_label(discount));
                printf("Final fare: Php %.2f\n", final_fare);
                printf("%4d | %-5s | %-18s | Php %6.2f | Php %6.2f | %s\n",
                       leg_index + 1,
                       line_name,
                       discount_label(discount),
                       base_fare,
                       final_fare,
                       detail);

                trip_base += base_fare;
                trip_paid += final_fare;
                total_base += base_fare;
                total_paid += final_fare;
                ride_count++;

                passenger->totals.base_total += base_fare;
                passenger->totals.paid_total += final_fare;

                line_totals[line - 1].base_total += base_fare;
                line_totals[line - 1].paid_total += final_fare;
                if (discount >= 1 && discount <= 4) {
                    discount_totals[discount - 1].base_total += base_fare;
                    discount_totals[discount - 1].paid_total += final_fare;
                }

                if (leg_index < MAX_TRIP_RIDES) {
                    trip_rides[leg_index].line_name = line_name;
                    trip_rides[leg_index].discount_type = discount;
                    trip_rides[leg_index].base_fare = base_fare;
                    trip_rides[leg_index].final_fare = final_fare;
                    strncpy(trip_rides[leg_index].detail, detail,
                            sizeof(trip_rides[leg_index].detail) - 1);
                    trip_rides[leg_index].detail[sizeof(trip_rides[leg_index].detail) - 1] = '\0';
                }

                if (ride_count <= MAX_RIDES) {
                    RideSummary *summary = &ride_summaries[ride_count - 1];
                    summary->ride_no = ride_count;
                    summary->passenger_no = passenger_no;
                    summary->trip_no = trip_count + 1;
                    summary->line_name = line_name;
                    summary->discount_type = discount;
                    summary->base_fare = base_fare;
                    summary->final_fare = final_fare;
                    strncpy(summary->detail, detail, sizeof(summary->detail) - 1);
                    summary->detail[sizeof(summary->detail) - 1] = '\0';
                }

                has_forced_transfer = 0;
                if (end_station >= 0) {
                    TransferRule transfer_rule;
                    if (find_transfer(line, end_station, &transfer_rule)) {
                        printf("\nTransfer available: %s\n", transfer_rule.note);
                        if (read_yes_no("Transfer to the connected line? (Y/N): ")) {
                            has_forced_transfer = 1;
                            forced_line = transfer_rule.to_line;
                            forced_start = transfer_rule.to_station;
                            if (forced_line == 1) {
                                printf("Next ride will start at %s (LRT-1).\n", LRT1_STATIONS[forced_start]);
                            } else if (forced_line == 2) {
                                printf("Next ride will start at %s (LRT-2).\n", LRT2_STATIONS[forced_start]);
                            } else {
                                printf("Next ride will start at %s (MRT-3).\n", MRT3_STATIONS[forced_start]);
                            }
                            leg_index++;
                            trip_ride_count++;
                            continue;
                        }
                    }
                }

                if (read_yes_no("Add another ride in this trip? (Y/N): ")) {
                    has_forced_transfer = 0;
                    leg_index++;
                    trip_ride_count++;
                    continue;
                }

                trip_ride_count++;
                break;
            }

            printf("\nTrip total before discounts: Php %.2f\n", trip_base);
            printf("Trip total after discounts: Php %.2f\n", trip_paid);
            printf("Trip discount: Php %.2f\n", trip_base - trip_paid);
            trip_count++;

            write_trip_receipt(passenger,
                               passenger_no,
                               trip_count,
                               trip_rides,
                               trip_ride_count,
                               trip_base,
                               trip_paid);

        if (!read_yes_no("Add another trip? (Y/N): ")) {
            break;
        }
    }

    printf("\nPassenger summary (%s):\n", passenger->name);
    printf("Total fare before discounts: Php %.2f\n", passenger->totals.base_total);
    printf("Total paid (after discounts): Php %.2f\n", passenger->totals.paid_total);
    printf("Total discount: Php %.2f\n", passenger->totals.base_total - passenger->totals.paid_total);
    if (passenger->used_card) {
        printf("Card starting balance: Php %.2f\n", passenger->start_balance);
        printf("Card ending balance:   Php %.2f\n", passenger->end_balance);
        printf("Total top-up:          Php %.2f\n", passenger->topup_total);
    }

    printf("\n----- Summary -----\n");
    printf("Passengers: %d\n", passenger_count);
    printf("Trips: %d\n", trip_count);
    printf("Rides: %d\n", ride_count);
    printf("Total fare before discounts: Php %.2f\n", total_base);
    printf("Total paid (after discounts): Php %.2f\n", total_paid);
    printf("Total discount: Php %.2f\n", total_base - total_paid);

    printf("\nTotals per line:\n");
    printf("Line  | Base Total | Paid Total | Discount\n");
    printf("------+------------+------------+----------\n");
    printf("LRT-1 | Php %6.2f | Php %6.2f | Php %6.2f\n",
           line_totals[0].base_total,
           line_totals[0].paid_total,
           line_totals[0].base_total - line_totals[0].paid_total);
    printf("LRT-2 | Php %6.2f | Php %6.2f | Php %6.2f\n",
           line_totals[1].base_total,
           line_totals[1].paid_total,
           line_totals[1].base_total - line_totals[1].paid_total);
    printf("MRT-3 | Php %6.2f | Php %6.2f | Php %6.2f\n",
           line_totals[2].base_total,
           line_totals[2].paid_total,
           line_totals[2].base_total - line_totals[2].paid_total);

    printf("\nTotals per discount type:\n");
    printf("Type    | Base Total | Paid Total | Discount\n");
    printf("--------+------------+------------+----------\n");
    printf("None    | Php %6.2f | Php %6.2f | Php %6.2f\n",
           discount_totals[0].base_total,
           discount_totals[0].paid_total,
           discount_totals[0].base_total - discount_totals[0].paid_total);
    printf("Student | Php %6.2f | Php %6.2f | Php %6.2f\n",
           discount_totals[1].base_total,
           discount_totals[1].paid_total,
           discount_totals[1].base_total - discount_totals[1].paid_total);
    printf("PWD     | Php %6.2f | Php %6.2f | Php %6.2f\n",
           discount_totals[2].base_total,
           discount_totals[2].paid_total,
           discount_totals[2].base_total - discount_totals[2].paid_total);
    printf("Senior  | Php %6.2f | Php %6.2f | Php %6.2f\n",
           discount_totals[3].base_total,
           discount_totals[3].paid_total,
           discount_totals[3].base_total - discount_totals[3].paid_total);

    if (ride_count > 0) {
        int max_print = (ride_count < MAX_RIDES) ? ride_count : MAX_RIDES;
        printf("\nAll rides breakdown:\n");
        printf("Ride | Passenger | Trip | Line  | Discount           | Base Fare | Final Fare | Detail\n");
        printf("-----+-----------+------+-------+--------------------+-----------+-----------+-------------------------\n");
        for (int i = 0; i < max_print; i++) {
            RideSummary *summary = &ride_summaries[i];
            printf("%4d | %9d | %4d | %-5s | %-18s | Php %6.2f | Php %6.2f | %s\n",
                   summary->ride_no,
                   summary->passenger_no,
                   summary->trip_no,
                   summary->line_name,
                   discount_label(summary->discount_type),
                   summary->base_fare,
                   summary->final_fare,
                   summary->detail);
        }
        if (ride_count > MAX_RIDES) {
            printf("... %d more rides not shown (limit %d).\n", ride_count - MAX_RIDES, MAX_RIDES);
        }
    }

    printf("Thank you!\n");
    return 0;
}
