#include <stdio.h>

#include <dima-c/dima.h>

typedef struct {
    int x, y;
} Point;

DIMA_DEFINE(Point)

void increment(Point *p) {
    DEFER_RELEASE(Point, p);
    printf("increment begin\n");
    p->x++;
    p->y++;
}

Point *create_point(int x, int y) {
    ALLOC(Point, p); // The ARC is set to 1 here
    p->x = x;
    p->y = y;
    return REF(Point, p); // The ARC is increased here (Now 2)
} // The ARC is decreased here (Now 1)

int main() {
    printf("BEGIN\n");
    Point *arr[100];
    for (int i = 0; i < 100; i++) {
        arr[i] = create_point(i, i * 2); // The ARC is not changed here, its still 1
    }
    for (int i = 0; i < 100; i++) {
        VAR(Point, p) = arr[i]; // A reference is recieved, the ARC is increased to 2 here
        printf("Point %d: (%d, %d)\n", i, p->x, p->y);
    } // The reference is dropped here, the ARC is decreased to 1
    for (int i = 0; i < 100; i++) {
        RELEASE(Point, arr[i]); // The ARC will reach 0 here, freeing the slot
    }
    printf("END\n");
    return 0;
}
