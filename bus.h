#ifndef BUS_H
#define BUS_H 1

struct Bus {
    char *prompt;
    void (*init)();
    void (*exit)();
    void (*start)();
    void (*stop)();
    uint8_t (*xact)(uint8_t);
};

#endif
