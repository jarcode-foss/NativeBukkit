#ifndef NB_INTERFACES_H
#define NB_INTERFACES_H

/* NativeBukkit uses a very primitive approach to OOP, there are
   virtual tables for different, abstract types (ie. interfaces),
   and then there are concrete types (ie. player) that have their
   own data and implementations of these interfaces. Concrete 
   types are represented as nb_type (aka. void*) and are casted to
   their concrete type depending on the context.

   Abstract types do not have any data, they only have an interface
   in which you interact with the underlying concrete type.
   
   For casting and type checking at runtime, the following code is
   valid:
   
   nb_type player = (...);
   nb_vtsender* interface = (...);
   if (interface->type == N_PLAYER) {
       // call method from another interface that N_PLAYER implements
       // (vt collection).(concrete type).(interface).foobar(self, ...)
       
       // usually less verbose to keep a pointer to the vtables you need
       nb_vtplauer vplayer = &(api->vt.player.player);
       vplayer.sethealth(player, 0);
       
       // cast 'player' from nb_type to its concrete type
       nb_tplayer* cplayer = (nb_tplayer*) player;
       nb->logf(state, "%s's ip is: %s", interface->name(player),
                cplayer->address);
   }
*/

/* nb_vtXXX: nativebukkit virtual table (abstract type)  */
/* nb_tXXX: nativebukkit type                            */
/* nb_type: enum that each vtable contains, can be used to
   check type at runtime */

typedef nb_type void*;

struct nb_vtcollection {
    struct {
        nb_vtsender sender;
        nb_vtplayer player;
    } player;
    struct {
        nb_vtsender sender;
    } console;
};

/* some vtables do not have a 'type' member, this is because they only have a single implementation,
   usually the same name (nb_vtplayer <-> nb_tplayer) */

typedef struct {
    enum nb_type type;
    void         (*send)     (nb_type self, const char* cmd);
    const char*  (*name)     (nb_type self);
    bool         (*hasperm)  (nb_type self, const char* perm);
} nb_vtsender;

/* note: some functions from the bukkit API do not exist here in favour of
   using raw packets (some are also deprecated) */

typedef struct {
    
    /* sends a raw packet, will automatically compress and encrypt if either are enabled */
    void         (*sendpacket)     (nb_tplayer self, void* buf, size_t len);
    
    const char*  (*dname)          (nb_tplayer self); /* display name */
    const char*  (*tabname)        (nb_tplayer self); /* tab list name */
    
    int          (*level)          (nb_tplayer self);
    void         (*setlevel)       (nb_tplayer self, int level);
    double       (*exp)            (nb_tplayer self);
    void         (*setexp)         (nb_tplayer self, double exp);
    double       (*exhaustion)     (nb_tplayer self);
    void         (*setexhaustion)  (nb_tplayer self, double exhaustion);
    double       (*saturation)     (nb_tplayer self);
    void         (*setsaturation)  (nb_tplauer self, double saturation);
    int          (*foodlevel)      (nb_tplayer self);
    void         (*setfoodlevel)   (nb_tplayer self, int foodlevel);
    
    double       (*healthscale)    (nb_tplayer self);               /* returns < 0 if not scaled     */
    void         (*sethealthscale) (nb_tplayer self, double scale); /* set to < 0 to disable scaling */
    
    bool         (*canfly)         (nb_tplayer self);
    void         (*allowfly)       (nb_tplayer self, bool allow);
    double       (*flyspeed)       (nb_tplayer self);
    void         (*setflyspeed)    (nb_tplayer self, double speed);
    bool         (*flying)         (nb_tplayer self);
    void         (*setwalkspeed)   (nb_tplayer self, double speed);
    
    bool         (*sneaking)       (nb_tplayer self);
    bool         (*sprinting)      (nb_tplayer self);
    bool         (*sleeping)       (nb_tplayer self);
    
    nb_loc       (*comptarget)     (nb_tplayer self);
    void         (*setcomptarget)  (nb_tplayer self, nb_loc loc);
    nb_loc       (*bedspawn)       (nb_tplayer self);
    void         (*setbedspawn)    (nb_tplayer self, nb_loc loc);
    
    void         (*chat)           (nb_tplayer self, const char* msg); /* perform command or chat */
    void         (*sendraw)        (nb_tplayer self, const char* json);
    void         (*kick)           (nb_tplayer self, const char* msg);
    
    void         (*setresoucepack) (nb_tplayer self, const char* url);
    
} nb_vtplayer;

typedef struct {
    enum nb_type type;
} nb_vtentity;

#endif /* NB_INTERFACES_H */
