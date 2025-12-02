/* Smart Traffic Signal Control System
   - Simulates a 4-lane intersection
   - Variable green time based on vehicle count
   - Per-second simulation of traffic flow
   - Stores average wait statistics to file
   - Demonstrates dynamic memory with pointers
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LANES 4
#define MAX_GREEN 40   // maximum green seconds
#define BASE_GREEN 5   // minimum green seconds
#define VEHICLE_PASS_PER_SEC 1  // vehicles that can pass per second on green

typedef struct {
    int vehicles;           // vehicles currently waiting
    long total_wait_secs;   // accumulated waiting seconds (sum over vehicles)
    long vehicles_served;   // total vehicles passed through
} Lane;

typedef struct {
    char name[64];
    Lane *lanes;            // dynamic array of lanes
    int cycles;             // number of simulation cycles run
} Intersection;

/* Calculate green time dynamically based on current vehicles waiting */
int calculate_green_time(int vehicles_waiting) {
    // simple formula: base + 2 sec per vehicle, with cap
    int t = BASE_GREEN + 2 * vehicles_waiting;
    if (t > MAX_GREEN) t = MAX_GREEN;
    return t;
}

/* Create a new intersection dynamically */
Intersection *create_intersection(const char *name) {
    Intersection *it = (Intersection *)malloc(sizeof(Intersection));
    if (!it) {
        fprintf(stderr, "Allocation failed for intersection\n");
        exit(EXIT_FAILURE);
    }
    strncpy(it->name, name, sizeof(it->name)-1);
    it->name[sizeof(it->name)-1] = '\0';
    it->lanes = (Lane *)malloc(LANES * sizeof(Lane));
    if (!it->lanes) {
        fprintf(stderr, "Allocation failed for lanes\n");
        free(it);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < LANES; ++i) {
        it->lanes[i].vehicles = 0;
        it->lanes[i].total_wait_secs = 0;
        it->lanes[i].vehicles_served = 0;
    }
    it->cycles = 0;
    return it;
}

/* Free intersection memory */
void destroy_intersection(Intersection *it) {
    if (!it) return;
    free(it->lanes);
    free(it);
}

/* Run one full cycle of signals (each lane gets green in order) */
void run_one_cycle(Intersection *it) {
    if (!it) return;
    // iterate through each lane: give it green
    for (int lane_idx = 0; lane_idx < LANES; ++lane_idx) {
        int waiting = it->lanes[lane_idx].vehicles;
        int green = calculate_green_time(waiting);
        if (green < BASE_GREEN) green = BASE_GREEN;

        // Simulate per-second behavior for green seconds
        for (int sec = 0; sec < green; ++sec) {
            // allow vehicles to pass from current lane
            if (it->lanes[lane_idx].vehicles > 0) {
                it->lanes[lane_idx].vehicles -= VEHICLE_PASS_PER_SEC;
                if (it->lanes[lane_idx].vehicles < 0) it->lanes[lane_idx].vehicles = 0;
                it->lanes[lane_idx].vehicles_served += VEHICLE_PASS_PER_SEC;
            }
            // for every other lane, each waiting vehicle accumulates 1 second wait
            for (int j = 0; j < LANES; ++j) {
                if (j == lane_idx) continue;
                if (it->lanes[j].vehicles > 0) {
                    // add waiting seconds equal to number of vehicles waiting
                    it->lanes[j].total_wait_secs += it->lanes[j].vehicles;
                }
            }
        }
    }
    it->cycles++;
}

/* Print current state */
void print_intersection_state(Intersection *it) {
    printf("Intersection: %s | Cycles: %d\n", it->name, it->cycles);
    for (int i = 0; i < LANES; ++i) {
        printf(" Lane %d -> waiting: %d, served: %ld, total_wait_secs: %ld\n",
               i+1, it->lanes[i].vehicles, it->lanes[i].vehicles_served, it->lanes[i].total_wait_secs);
    }
}

/* Compute and save statistics to file */
void save_statistics(Intersection *it, const char *filename) {
    FILE *f = fopen(filename, "a");
    if (!f) {
        perror("fopen");
        return;
    }
    time_t now = time(NULL);
    fprintf(f, "=== Stats for intersection '%s' at %s", it->name, ctime(&now));
    long total_served = 0;
    long total_wait_secs = 0;
    for (int i = 0; i < LANES; ++i) {
        total_served += it->lanes[i].vehicles_served;
        total_wait_secs += it->lanes[i].total_wait_secs;
    }
    double avg_wait_per_vehicle = (total_served > 0) ? ((double)total_wait_secs / total_served) : 0.0;
    fprintf(f, "Cycles run: %d\n", it->cycles);
    fprintf(f, "Total vehicles served: %ld\n", total_served);
    fprintf(f, "Total wait seconds (sum over vehicles): %ld\n", total_wait_secs);
    fprintf(f, "Average wait time per vehicle: %.2f seconds\n\n", avg_wait_per_vehicle);
    fclose(f);
    printf("Statistics saved to %s\n", filename);
}

/* Small helper: prompt user to enter vehicles for each lane */
void input_vehicle_counts(Intersection *it) {
    printf("Enter initial vehicle count for each of the %d lanes:\n", LANES);
    for (int i = 0; i < LANES; ++i) {
        int v;
        printf(" Lane %d: ", i+1);
        if (scanf("%d", &v) != 1) {
            fprintf(stderr, "Invalid input. Exiting.\n");
            exit(EXIT_FAILURE);
        }
        if (v < 0) v = 0;
        it->lanes[i].vehicles = v;
    }
}

/* Optionally add more vehicles arriving between cycles (simple random arrival) */
void random_arrival_between_cycles(Intersection *it) {
    // simple deterministic pseudo-randomness using rand()
    for (int i = 0; i < LANES; ++i) {
        int arrival = rand() % 4; // 0..3 new vehicles per lane per cycle
        it->lanes[i].vehicles += arrival;
    }
}

int main(void) {
    srand((unsigned int)time(NULL));
    Intersection *it = create_intersection("Main_1");

    printf("Smart Traffic Signal Simulation (4 lanes)\n");
    input_vehicle_counts(it);

    int cycles_to_run;
    printf("Enter the number of cycles to simulate (1 cycle = one green for each lane): ");
    if (scanf("%d", &cycles_to_run) != 1 || cycles_to_run <= 0) {
        fprintf(stderr, "Invalid number of cycles. Exiting.\n");
        destroy_intersection(it);
        return 1;
    }

    for (int c = 0; c < cycles_to_run; ++c) {
        printf("\n--- Starting cycle %d ---\n", c+1);
        print_intersection_state(it);
        run_one_cycle(it);

        // Optionally simulate new arrivals between cycles
        random_arrival_between_cycles(it);

        print_intersection_state(it);
    }

    // Final statistics output
    printf("\nSimulation finished. Final state:\n");
    print_intersection_state(it);

    // Save statistics to file
    save_statistics(it, "traffic_stats.txt");

    destroy_intersection(it);
    return 0;
}
