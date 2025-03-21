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
    queue->head = NULL;
    queue->size = 0;
}

/**
 * Cleans up the `EventQueue`.
 *
 * Frees any memory and resources associated with the `EventQueue`.
 * 
 * @param[in,out] queue  Pointer to the `EventQueue` to clean.
 */
void event_queue_clean(EventQueue *queue) {
    if (queue == NULL) {
        return;
    }
    EventNode *current = queue->head;
    EventNode *next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    queue->head = NULL;
    queue->size = 0;
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
    if (queue == NULL || event == NULL){
        return;
    }
    
    // initalizations
    EventNode *new_node = (EventNode*)malloc(sizeof(EventNode));
    if (new_node == NULL) {
        return;
    }
    new_node->event = *event;

    // case 1: our queue is empty or our event belongs at head.
    if (queue->head == NULL || event->priority > queue->head->event.priority) {
        new_node->next = queue->head;
        queue->head = new_node;

    }

    // case 2: our queue is not empty.
    else {
        EventNode *current = queue->head->next;
        EventNode *previous = queue->head;

        // now we must perform traversals     
        while (current !=NULL && current->event.priority >= new_node->event.priority) {
            current = current->next;
            previous = previous->next;
        }

        new_node->next = current;
        previous->next = new_node;
    }

    // increment queue size
    queue->size++;
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
    if (queue->head == NULL) {
        // no events in the queue
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
    
    // event successfully popped
    return 1;
}
