typedef struct {
    int *buf;       /* 버퍼 배열 */
    int n;          /* 슬롯의 최대 갯수 */
    int front;      /* buf[(front+1)%n] 는 첫 아이템 */
    int rear;       /* buf[rear%n] 는 마지막 아이템 */
    sem_t mutex;    /* 버퍼에 대한 접근 보호 */
    sem_t slots;    /* 사용 가능 슬롯 수 */
    sem_t items;    /* 사용 가능 아이템 수 */
} sbuf_t;