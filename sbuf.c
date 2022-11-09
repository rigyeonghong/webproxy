/* sbuf - 제한된 버퍼들로의 동시성 접근을 동기화하기 위한 패키지 - 생산자 소비자 문제 적용 */
#include "csapp.h"
#include "sbuf.h"

/*
 * sbuf_init - 슬롯이 n개인 빈 공유 FIFO 버퍼 생성
 */
void sbuf_init(sbuf_t *sp, int n){
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;                               /* 버퍼의 최대 n개 항목 존재 */
    sp->front = sp->rear = 0;                /* 맨 앞, 맨 뒤의 경우 buf 비우기 */
    Sem_init(&sp->mutex, 0, 1);              /* 잠금을 위한 이진 세마포어 */
    Sem_init(&sp->slots, 0, n);              /* 초기화, 버퍼에 n개의 빈 슬롯 존재 */
    Sem_init(&sp->items, 0, 0);              /* 초기화, 버퍼에 0개의 데이터 항목 존재 */
}

/*
 * sbuf_deinit - sp 버퍼 정리
 */
void sbuf_deinit(sbuf_t *sp){
    Free(sp->buf);
}

/*
 * sbuf_insert - 공유 버퍼 sp의 뒤에 item 삽입 (생산자)
 */
void sbuf_insert(sbuf_t *sp, int item){
    P(&sp->slots);                           /* 사용 가능 슬롯 기다림 */ 
    P(&sp->mutex);                           /* 버퍼 잠금 */
    sp->buf[(++sp->rear)%(sp->n)] = item;    /* 아이템 추가 */
    V(&sp->mutex);                           /* 버퍼 잠금 해제 */
    V(&sp->items);                           /* 사용 가능 아이템 알림 */
}

/*
 * sbuf_remove - 공유 버퍼 sp의 첫번째 항목 제거 및 반환 (소비자)
 */
int sbuf_remove(sbuf_t *sp){
    int item;                                
    P(&sp->items);                           /* 사용 가능 아이템 기다림 */
    P(&sp->mutex);                           /* 버퍼 잠금 */
    item = sp->buf[(++sp->front)%(sp->n)];   /* 아이템 삭제 */
    V(&sp->mutex);                           /* 버퍼 잠금 해제 */
    V(&sp->slots);                           /* 사용 가능 슬롯 알림 */
    return item;
}