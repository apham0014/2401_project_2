// Ahmad Baytamouni 101335293
// Austin Pham 101333594

#include "defs.h"
#include <stdlib.h>
#include <stdio.h>

/* Event functions */

/**
 * Initializes an `Event` structure.
 *
 * Sets up an `Event` with the provided system, resource, status, priority, and amount.
 *
 * @param[out] event     Pointer to the `Event` to initialize.
 * @param[in]  system    Pointer to the `System` that generated the event.
 * @param[in]  resource  Pointer to the `Resource` associated with the event.
 * @param[in]  status    Status code representing the event type.
 * @param[in]  priority  Priority level of the event.
 * @param[in]  amount    Amount related to the event (e.g., resource amount).
 */
void event_init(Event *event, System *system, Resource *resource, int status, int priority, int amount) {
    event->system = system;
    event->resource = resource;
    event->status = status;
    event->priority = priority;
    event->amount = amount;
}

/* EventQueue functions */

/**
 * Initializes the `EventQueue`.
 *
 * Sets up the queue for use, initializing any necessary data (e.g., semaphores when threading).
 *
 * @param[out] queue  Pointer to the `EventQueue` to initialize.
 */
void event_queue_init(EventQueue *queue) {
    // set the queue head to NULL, indicating an empty queue
    queue->head = NULL;
    // initialize the queue size to 0
    queue->size = 0;
    // initialize the semaphore for thread safe access to the queue
    sem_init(&queue->mutex, 0, 1);
}

/**
 * Cleans up the `EventQueue`.
 *
 * Frees any memory and resources associated with the `EventQueue`.
 * 
 * @param[in,out] queue  Pointer to the `EventQueue` to clean.
 */
void event_queue_clean(EventQueue *queue) {
    // return if the queue is NULL
    if (queue == NULL) {
        return;
    }

    // declare pointers to traverse through the queue
    EventNode *current = queue->head;
    EventNode *next;

    // iterate through each node in the queue, freeing memory
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    // reset the queue to an empty state
    queue->head = NULL;
    queue->size = 0;

    // destroy the semaphore to release resources
    sem_destroy(&queue->mutex);
}

/**
 * Pushes an `Event` onto the `EventQueue`.
 *
 * Adds the event to the queue in a thread-safe manner, maintaining priority order (highest first).
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[in]     event  Pointer to the `Event` to push onto the queue.
 */
void event_queue_push(EventQueue *queue, const Event *event) {
    // return if the queue or event is NULL
    if (queue == NULL || event == NULL){
        return;
    }

    // wait for semaphore to ensure thread-safety
    sem_wait(&queue->mutex);
    
    // initialize a new node for the event
    EventNode *new_node = (EventNode*)malloc(sizeof(EventNode));
    
    // if malloc fails, release the semaphore and return
    if (new_node == NULL) {
        sem_post(&queue->mutex);
        return;
    }
    // assign the event passed in the parameter to the new node's event
    new_node->event = *event;

    // Case 1: queue is empty or event belongs at the head
    if (queue->head == NULL || event->priority > queue->head->event.priority) {
        new_node->next = queue->head;
        queue->head = new_node;
    }
    // Case 2: queue is not empty, need to find correct position
    else {
        // declare pointers for traversal
        EventNode *current = queue->head->next;
        EventNode *previous = queue->head;

        // perform traversal to find the correct position
        while (current != NULL && current->event.priority >= new_node->event.priority) {
            current = current->next;
            previous = previous->next;
        }

        // insert the new node into the queue
        new_node->next = current;
        previous->next = new_node;
    }

    // increment the queue size
    queue->size++;

    // release the semaphore after modifying the queue
    sem_post(&queue->mutex);
}

/**
 * Pops an `Event` from the `EventQueue`.
 *
 * Removes the highest priority event from the queue in a thread-safe manner.
 *
 * @param[in,out] queue  Pointer to the `EventQueue`.
 * @param[out]    event  Pointer to the `Event` structure to store the popped event.
 * @return               Non-zero if an event was successfully popped; zero otherwise.
 */
int event_queue_pop(EventQueue *queue, Event *event) {
    // wait for semaphore to ensure thread safety
    sem_wait(&queue->mutex);

    // if no events in the queue, release the semaphore and return 0
    if (queue->head == NULL) {
        sem_post(&queue->mutex);
        return 0;
    }
    
    // remove the event at the head of the queue
    EventNode *remove = queue->head;
    *event = remove->event;
    queue->head = remove->next;

    // free the memory of the removed node
    free(remove);

    // decrement the size of the queue
    queue->size--;

    // release the semaphore after modifying the queue
    sem_post(&queue->mutex);
    
    // event successfully popped
    return 1;
}
