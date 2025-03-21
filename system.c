#include "defs.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Helper functions just used by this C file to clean up our code
// Using static means they can't get linked into other files

static int system_convert(System *);
static void system_simulate_process_time(System *);
static int system_store_resources(System *);

/**
 * Creates a new `System` object.
 *
 * Allocates memory for a new `System` and initializes its fields.
 * The `name` is dynamically allocated.
 *
 * @param[out] system          Pointer to the `System*` to be allocated and initialized.
 * @param[in]  name            Name of the system (the string is copied).
 * @param[in]  consumed        `ResourceAmount` representing the resource consumed.
 * @param[in]  produced        `ResourceAmount` representing the resource produced.
 * @param[in]  processing_time Processing time in milliseconds.
 * @param[in]  event_queue     Pointer to the `EventQueue` for event handling.
 */
void system_create(System **system, const char *name, ResourceAmount consumed, ResourceAmount produced, int processing_time, EventQueue *event_queue) {

    *system = (System *)malloc(sizeof(System));
    if (*system == NULL) {
        return;
    }

    (*system)->name = (char *)malloc(strlen(name) + 1);
    if ((*system)->name == NULL) {
        free(*system);
        *system = NULL;
        return;
    }

    strcpy((*system)->name, name);
    (*system)->consumed = consumed;
    (*system)->produced = produced;
    (*system)->amount_stored = 0;
    (*system)->processing_time = processing_time;
    (*system)->status = STANDARD;
    (*system)->event_queue = event_queue;
}

/**
 * Destroys a `System` object.
 *
 * Frees all memory associated with the `System`.
 *
 * @param[in,out] system  Pointer to the `System` to be destroyed.
 */
void system_destroy(System *system) {
    if (system == NULL){
        return;
    }
    free(system->name);
    free(system);
}


/**
 * Runs the main loop for a `System`.
 *
 * This function manages the lifecycle of a system, including resource conversion,
 * processing time simulation, and resource storage. It generates events based on
 * the success or failure of these operations.
 *
 * @param[in,out] system  Pointer to the `System` to run.
 */
void system_run(System *system) {
    Event event;
    int result_status;
    
    if (system->amount_stored == 0) {
        // Need to convert resources (consume and process)
        result_status = system_convert(system);

        if (result_status != STATUS_OK) {
            // Report that resources were out / insufficient
            event_init(&event, system, system->consumed.resource, result_status, PRIORITY_HIGH, system->consumed.resource->amount);
            event_queue_push(system->event_queue, &event);    
            // Sleep to prevent looping too frequently and spamming with events
            usleep(SYSTEM_WAIT_TIME * 1000);          
        }
    }

    if (system->amount_stored  > 0) {
        // Attempt to store the produced resources
        result_status = system_store_resources(system);

        if (result_status != STATUS_OK) {
            event_init(&event, system, system->produced.resource, result_status, PRIORITY_LOW, system->produced.resource->amount);
            event_queue_push(system->event_queue, &event);
            // Sleep to prevent looping too frequently and spamming with events
            usleep(SYSTEM_WAIT_TIME * 1000);
        }
    }
}

/**
 * Converts resources in a `System`.
 *
 * Handles the consumption of required resources and simulates processing time.
 * Updates the amount of produced resources based on the system's configuration.
 *
 * @param[in,out] system           Pointer to the `System` performing the conversion.
 * @param[out]    amount_produced  Pointer to the integer tracking the amount of produced resources.
 * @return                         `STATUS_OK` if successful, or an error status code.
 */
static int system_convert(System *system) {
    int status;
    Resource *consumed_resource = system->consumed.resource;
    int amount_consumed = system->consumed.amount;

    // We can always convert without consuming anything
    if (consumed_resource == NULL) {
        status = STATUS_OK;
    } else {
        sem_wait(&consumed_resource->mutex);
        // Attempt to consume the required resources
        if (consumed_resource->amount >= amount_consumed) {
            consumed_resource->amount -= amount_consumed;
            status = STATUS_OK;
        } else {
            status = (consumed_resource->amount == 0) ? STATUS_EMPTY : STATUS_INSUFFICIENT;
        }
        sem_post(&consumed_resource->mutex);
    }

    if (status == STATUS_OK) {
        system_simulate_process_time(system);

        if (system->produced.resource != NULL) {
            system->amount_stored += system->produced.amount;
        }
        else {
            system->amount_stored = 0;
        }
    }

    return status;
}

/**
 * Simulates the processing time for a `System`.
 *
 * Adjusts the processing time based on the system's current status (e.g., SLOW, FAST)
 * and sleeps for the adjusted time to simulate processing.
 *
 * @param[in] system  Pointer to the `System` whose processing time is being simulated.
 */
static void system_simulate_process_time(System *system) {
    int adjusted_processing_time;

    // Adjust based on the current system status modifier
    switch (system->status) {
        case SLOW:
            adjusted_processing_time = system->processing_time * 2;
            break;
        case FAST:
            adjusted_processing_time = system->processing_time / 2;
            break;
        default:
            adjusted_processing_time = system->processing_time;
    }

    // Sleep for the required time
    usleep(adjusted_processing_time * 1000);
}

/**
 * Stores produced resources in a `System`.
 *
 * Attempts to add the produced resources to the corresponding resource's amount,
 * considering the maximum capacity. Updates the `produced_resource_count` to reflect
 * any leftover resources that couldn't be stored.
 *
 * @param[in,out] system                   Pointer to the `System` storing resources.
 * @param[in,out] produced_resource_count  Pointer to the integer value of how many resources need to be stored, updated with the amount that could not be stored.
 * @return                                 `STATUS_OK` if all resources were stored, or `STATUS_CAPACITY` if not all could be stored.
 */
static int system_store_resources(System *system) {
    Resource *produced_resource = system->produced.resource;
    int available_space, amount_to_store;

    // We can always proceed if there's nothing to store
    if (produced_resource == NULL || system->amount_stored == 0) {
        system->amount_stored = 0;
        return STATUS_OK;
    }

    amount_to_store = system->amount_stored;

    sem_wait(&produced_resource->mutex);

    // Calculate available space
    available_space = produced_resource->max_capacity - produced_resource->amount;

    if (available_space >= amount_to_store) {
        // Store all produced resources
        produced_resource->amount += amount_to_store;
        system->amount_stored = 0;
    } else if (available_space > 0) {
        // Store as much as possible
        produced_resource->amount += available_space;
        system->amount_stored = amount_to_store - available_space;
    }

    sem_post(&produced_resource->mutex);

    if (system->amount_stored != 0) {
        return STATUS_CAPACITY;
    }

    return STATUS_OK;
}


/**
 * Initializes the `SystemArray`.
 *
 * Allocates memory for the array of `System*` pointers of capacity 1 and sets up initial values.
 *
 * @param[out] array  Pointer to the `SystemArray` to initialize.
 */
void system_array_init(SystemArray *array) {
    // allocate memory for array of pointers.
    array->systems = (System **)malloc(sizeof(System *) * 1);

    if (array->systems == NULL) {
        return;
    }
    
    // initalize other attributes.
    array->size = 0;
    array->capacity = 1;
}

/**
 * Cleans up the `SystemArray` by destroying all systems and freeing memory.
 *
 * Iterates through the array, cleaning any memory for each System pointed to by the array.
 *
 * @param[in,out] array  Pointer to the `SystemArray` to clean.
 */
void system_array_clean(SystemArray *array) {
    for (int i = 0; i < array->size; i++) {
        system_destroy(array->systems[i]);
    }

    free(array->systems);
}

/**
 * Adds a `System` to the `SystemArray`, resizing if necessary (doubling the size).
 *
 * Resizes the array when the capacity is reached and adds the new `System`.
 * Use of realloc is NOT permitted.
 *
 * @param[in,out] array   Pointer to the `SystemArray`.
 * @param[in]     system  Pointer to the `System` to add.
 */
void system_array_add(SystemArray *array, System *system) {
    // case 1: sufficient capacity
    if (array->size < array->capacity) {
        array->systems[array->size] = system;
        array->size++;
    }
    
    // case 2: insufficient capacity
    else {
        // we must reallocate memory for the array.
        System **temp_systems = (System **)malloc(sizeof(System *) * ((array->capacity)*2));
        if (temp_systems == NULL){
            return;
        }
        
        // then we copy our system struct into our new array.
        for (int i = 0; i < array->size; i++){
            temp_systems[i] = array->systems[i];
        }
        
        free(array->systems);
        array->systems = temp_systems;
        array->capacity *= 2;

        // finally we add our system indicated in the parameter.
        array->systems[array->size] = system;
        array->size++;
    }
}


/**
 * Executes a system in a thread.
 *
 * @param[in]     arg  A pointer to the System object that the thread will operate on. 
 */
void *system_thread(void *arg) {
    System *system = (System *)arg;
    while(system->status != TERMINATE) {
        system_run(system);
    }
    return NULL;
}
