#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <mysql.h>


struct Payload {
    int bay_no;
    int isParked[9];
    time_t timeStamp;
};


#define rows 9
#define column 9 
#define DEBUG 1
#define BAY_ROWS 3
#define PARK_COL 3
#define EMP_PROB 3
#define BAY_SCROL 3

void finish_with_error(MYSQL *con){
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}

void initiate_bays(int bays[rows][column]){
  for(int i=0; i<rows;i++){
    for(int j=0; j<column; j++){
      bays[i][j] = j+1;
    }
  }
}

int main(){
    MYSQL *con = mysql_init(NULL);
    int bays[rows][column] = {0};

    if (con == NULL){
        fprintf(stderr, "mysql_init() failed\n");
        exit(1);
    }

    if (mysql_real_connect(con, "localhost", "user", "marvel","parking", 0, NULL, 0) == NULL) {
        finish_with_error(con);
    }

    if (mysql_query(con, "SELECT * FROM floor")){
        finish_with_error(con);
    }

    MYSQL_RES *result = mysql_store_result(con);

    if (result == NULL){
        finish_with_error(con);
    }

    int num_fields = mysql_num_fields(result);
    MYSQL_ROW row;

    initiate_bays(bays);
    while ((row = mysql_fetch_row(result)) != NULL){
        int a = (int)row[0][0] - '0';
        int b = (int)row[0][1] - '0';
        bays[a-1][b-1] = 0;
    }

    mysql_free_result(result);
    mysql_close(con);

    int shmid;
    int i, j;
    struct Payload PAYLOAD_ARR[BAY_ROWS*PARK_COL];
    struct Payload *shm_alloc;

    time_t cur_time = time(NULL);

    srand((unsigned int)6969);

    for (i = 0; i < BAY_ROWS * BAY_SCROL; i++) {
        PAYLOAD_ARR[i].bay_no = ((int)(i / BAY_ROWS)) * BAY_ROWS + 1;
        for (j = 0; j < PARK_COL * BAY_ROWS; j++) {
            PAYLOAD_ARR[i].isParked[j] = bays[i][j];
        }
    }
    
    shmid = shmget(9876, sizeof(struct Payload), 0660);
    shm_alloc = (struct Payload*) shmat(shmid, NULL, 0);

    while (1) {
        for (i = 0; i < BAY_ROWS * BAY_SCROL; i++) {
            *shm_alloc = PAYLOAD_ARR[i];
            shm_alloc->timeStamp = cur_time;
            while (shm_alloc->timeStamp == cur_time) sleep(0.5);
        }
    }
    
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}