#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <cjson/cJSON.h>

#define MAX_STATES 32
#define NAME_SIZE 32

struct State {
    char name[NAME_SIZE];
    char syscall[NAME_SIZE];
};

/*
 * Convert the kernel's syscall symbol
 * into the syscall name used by the FSM.
 */
static const char *normalize_syscall(const char *event)
{
    if (strcmp(event, "__x64_sys_openat") == 0)
        return "openat";

    if (strcmp(event, "__x64_sys_read") == 0)
        return "read";

    if (strcmp(event, "__x64_sys_write") == 0)
        return "write";

    return NULL;
}

int main(void)
{
    FILE *file;
    long file_size;
    char *json_text;

    cJSON *root;
    cJSON *states;

    int state_count;

    struct State state_list[MAX_STATES];

    /*
     * Current FSM state.
     */
    int current_state = 0;

    /*
     * File descriptor for the Phoenix
     * kernel-to-user communication channel.
     */
    int fd;

    /*
     * Buffer for receiving a syscall event.
     */
    char event_buffer[128];

    ssize_t bytes_read;

    /*
     * Open the JSON file.
     */
    file = fopen("fsm.json", "r");

    if (file == NULL) {
        perror("Cannot open fsm.json");
        return 1;
    }

    /*
     * Find the file size.
     */
    fseek(file, 0, SEEK_END);

    file_size = ftell(file);

    rewind(file);

    /*
     * Allocate memory for the JSON text.
     */
    json_text = malloc(file_size + 1);

    if (json_text == NULL) {
        printf("Memory allocation failed.\n");

        fclose(file);

        return 1;
    }

    /*
     * Read the JSON file.
     */
    fread(json_text, 1, file_size, file);

    json_text[file_size] = '\0';

    fclose(file);

    /*
     * Parse JSON.
     */
    root = cJSON_Parse(json_text);

    if (root == NULL) {
        printf("Invalid JSON.\n");

        free(json_text);

        return 1;
    }

    /*
     * Find the states array.
     */
    states = cJSON_GetObjectItem(root, "states");

    if (!cJSON_IsArray(states)) {
        printf("'states' is not an array.\n");

        cJSON_Delete(root);
        free(json_text);

        return 1;
    }

    /*
     * Get the number of states.
     */
    state_count = cJSON_GetArraySize(states);

    if (state_count <= 0 || state_count > MAX_STATES) {
        printf("Invalid number of states: %d\n",
               state_count);

        cJSON_Delete(root);
        free(json_text);

        return 1;
    }

    /*
     * Read every state from the JSON file.
     */
    for (int i = 0; i < state_count; i++) {

        cJSON *state;
        cJSON *name;
        cJSON *syscall;

        state = cJSON_GetArrayItem(states, i);

        if (!cJSON_IsObject(state)) {
            printf("State %d is not an object.\n", i);

            cJSON_Delete(root);
            free(json_text);

            return 1;
        }

        name = cJSON_GetObjectItem(state, "name");

        syscall = cJSON_GetObjectItem(state, "syscall");

        if (!cJSON_IsString(name) ||
            !cJSON_IsString(syscall)) {

            printf("Invalid state %d.\n", i);

            cJSON_Delete(root);
            free(json_text);

            return 1;
        }

        strncpy(state_list[i].name,
                name->valuestring,
                NAME_SIZE - 1);

        state_list[i].name[NAME_SIZE - 1] = '\0';

        strncpy(state_list[i].syscall,
                syscall->valuestring,
                NAME_SIZE - 1);

        state_list[i].syscall[NAME_SIZE - 1] = '\0';
    }

    /*
     * JSON loading completed successfully.
     */
    printf("FSM loaded successfully.\n");

    printf("Number of states: %d\n\n",
           state_count);

    /*
     * Open the Phoenix device.
     */
    fd = open("/dev/phoenix", O_RDONLY);

    if (fd < 0) {
        perror("Cannot open /dev/phoenix");

        cJSON_Delete(root);
        free(json_text);

        return 1;
    }

    /*
     * Read one syscall event from the kernel.
     */
    bytes_read = read(fd,
                      event_buffer,
                      sizeof(event_buffer) - 1);

    if (bytes_read < 0) {
        perror("Failed to read from /dev/phoenix");

        close(fd);

        cJSON_Delete(root);
        free(json_text);

        return 1;
    }

    /*
     * No event is currently available.
     */
    if (bytes_read == 0) {

        printf("No syscall event available.\n");

        close(fd);

        cJSON_Delete(root);
        free(json_text);

        return 0;
    }

    /*
     * Make the received data a valid C string.
     */
    event_buffer[bytes_read] = '\0';

    /*
     * Remove a possible trailing newline.
     */
    event_buffer[strcspn(event_buffer, "\n")] = '\0';

    printf("Raw event from kernel: %s\n",
           event_buffer);

    /*
     * Convert the kernel symbol into
     * the syscall name used by the FSM.
     */
    const char *event =
        normalize_syscall(event_buffer);

    if (event == NULL) {

        printf("Unknown syscall event.\n");

        close(fd);

        cJSON_Delete(root);
        free(json_text);

        return 1;
    }

    printf("Normalized syscall: %s\n\n",
           event);

    /*
     * Get the syscall expected by
     * the current FSM state.
     */
    const char *expected =
        state_list[current_state].syscall;

    printf("Current state: %d (%s)\n",
           current_state,
           state_list[current_state].name);

    printf("Expected syscall: %s\n",
           expected);

    printf("Received syscall: %s\n",
           event);

    /*
     * Check whether the received syscall
     * matches the expected syscall.
     */
    if (strcmp(event, expected) == 0) {

        printf("MATCH: moving to next state.\n");

        current_state++;

        /*
         * If we reach the end of the FSM,
         * return to state 0.
         */
        if (current_state >= state_count) {

            current_state = 0;

            printf("Reached the end. "
                   "Returning to State 0.\n");
        }

    } else {

        printf("NO MATCH: state does not change.\n");
    }

    printf("New current state: %d (%s)\n",
           current_state,
           state_list[current_state].name);

    /*
     * Close the Phoenix device.
     */
    close(fd);

    /*
     * Free memory.
     */
    cJSON_Delete(root);

    free(json_text);

    return 0;
}
