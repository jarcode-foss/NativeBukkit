#ifndef NB_EVENTS_H
#define NB_EVENTS_H

/* nb_ep: nativebukkit event priority */
enum nb_ep {
    NBEP_HIGHEST = 4,
    NBEP_HIGH    = 3,
    NBEP_NORMAL  = 2,
    NBEP_LOW     = 1,
    NBEP_LOWEST  = 0
};

/* nb_ev: nativebukkit event */
enum nb_ev {
    NBEV_CMD
};

typedef struct {
    nb_type sender;
    nb_vtsender vtsender;
    const char const** argv;
    int argc;
    bool cancelled;
} nb_evcmd;

#endif /* NB_EVENTS_H */
