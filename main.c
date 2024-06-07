#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <signal.h>

struct Payload {
    int bay_no;
    int isParked[9];
    time_t timeStamp;
};

int main() {
    pid_t pi_mqtt_display_pub, db;
    struct Payload* shm_alloc;

    int shmid = shmget(9876, sizeof(struct Payload), IPC_CREAT | 0660);
    shm_alloc = (struct Payload*) shmat(shmid, NULL, 0);

    db = fork();
    if (db == 0) {
        printf("[ CHILD ] => Starting dbsim\n");
        execl("./dbsim","dbsim", NULL);
    } else {
        pi_mqtt_display_pub = fork();
        if (pi_mqtt_display_pub == 0) {
            printf("[ CHILD ] => Starting pi_mqtt_display_pub\n");
            execl("./pi_mqtt_display_pub","pi_mqtt_display_pub", NULL);
        } else {
            printf("<=====> TERMINATE CHILDREN (any key) <=====>\n");
            getchar();
            kill(pi_mqtt_display_pub, SIGKILL);
            kill(db, SIGKILL);

            shmctl(shmid, IPC_RMID, NULL);
        }
    }

    return 0;
}