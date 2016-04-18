#ifndef NB_TYPES_H
#define NB_TYPES_H

typedef struct {
    double x, y, z;
    nb_tworld* world;
} nb_loc;

/* nb_ctype: nativebukkit concrete types */
enum nb_type {
    N_PLAYER,
    N_CONSOLE,
    N_WORLD
};

/* N_PLAYER */
/* most interaction with player types is done through the player interface */
typedef struct {
    const char* address;  /* IPv4 or IPv6 address that the player is connected with */
    short port;           /* port that the player is connected with */
    const char* hostname; /* hostname of this player */
} nb_tplayer;

/* N_WORLD */
typedef struct {
    const char* name;
    /* ... */
} nb_tworld;

#endif /* NB_TYPES_H */
