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

Point *test() {
    ALLOC(Point, p);
    p->x = 10;
    p->y = 20;
    increment(REF(Point, p));
    printf("Point(%d, %d)\n", p->x, p->y);
    return REF(Point, p);
} // 'p' is automatically released here!

int main() {
    VAR(Point, p) = test();
    if (!VAR_VALID(Point, p)) {
        printf("Pointer is null\n");
    } else {
        printf("Pointer is not null\n");
        printf("Point(%d, %d)\n", p->x, p->y);
    }
    printf("END\n");
    return 0;
}
