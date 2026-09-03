#include "MOESIF_protocol.h"
#include "../sim/mreq.h"
#include "../sim/sim.h"
#include "../sim/hash_table.h"

extern Simulator *Sim;

/*************************
 * Constructor/Destructor.
 *************************/
MOESIF_protocol::MOESIF_protocol (Hash_table *my_table, Hash_entry *my_entry)
    : Protocol (my_table, my_entry)
{
    this->state = MOESIF_CACHE_I;
}

MOESIF_protocol::~MOESIF_protocol ()
{    
}

void MOESIF_protocol::dump (void)
{
#ifdef DEBUG
    // Define states including intermediate ones for debugging
    const char *block_states[12] = {"X","I","S","E","M","O","F","IS","IM","SM","OM","FM"}; // Added FM
    fprintf (stdout, "MOESIF_protocol - state: %s\\n", block_states[state]);
#endif
}

void MOESIF_protocol::process_cache_request (Mreq *request)
{
    switch (state) {
        case MOESIF_CACHE_I:  do_cache_I (request); break;
        case MOESIF_CACHE_S:  do_cache_S (request); break;
        case MOESIF_CACHE_E:  do_cache_E (request); break;
        case MOESIF_CACHE_M:  do_cache_M (request); break;
        case MOESIF_CACHE_O:  do_cache_O (request); break;
        case MOESIF_CACHE_F:  do_cache_F (request); break;
        // Intermediate states
        case MOESIF_CACHE_IS: do_cache_IS(request); break;
        case MOESIF_CACHE_IM: do_cache_IM(request); break;
        case MOESIF_CACHE_SM: do_cache_SM(request); break;
        case MOESIF_CACHE_OM: do_cache_OM(request); break;
        case MOESIF_CACHE_FM: do_cache_FM(request); break; // Added FM case
    default:
        fatal_error ("Invalid Cache State for MOESIF Protocol: %d\\n", state);
    }
}

void MOESIF_protocol::process_snoop_request (Mreq *request)
{
    switch (state) {
        case MOESIF_CACHE_I:  do_snoop_I (request); break;
        case MOESIF_CACHE_S:  do_snoop_S (request); break;
        case MOESIF_CACHE_E:  do_snoop_E (request); break;
        case MOESIF_CACHE_M:  do_snoop_M (request); break;
        case MOESIF_CACHE_O:  do_snoop_O (request); break;
        case MOESIF_CACHE_F:  do_snoop_F (request); break;
        // Intermediate states
        case MOESIF_CACHE_IS: do_snoop_IS(request); break;
        case MOESIF_CACHE_IM: do_snoop_IM(request); break;
        case MOESIF_CACHE_SM: do_snoop_SM(request); break;
        case MOESIF_CACHE_OM: do_snoop_OM(request); break;
        case MOESIF_CACHE_FM: do_snoop_FM(request); break; // Added FM case
    default:
        fatal_error ("Invalid Cache State for MOESIF Protocol: %d\\n", state);
    }
}

inline void MOESIF_protocol::do_cache_F (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            // Read hit
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            // Write hit - Need exclusive access (Upgrade Miss)
            send_GETM(request->addr);
            state = MOESIF_CACHE_FM; // Changed: F -> FM
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: F state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_I (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            send_GETS(request->addr);
            state = MOESIF_CACHE_IS;
            Sim->cache_misses++;
            break;
        case STORE:
            send_GETM(request->addr);
            state = MOESIF_CACHE_IM;
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_S (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            // Read hit
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            // Write hit, need exclusive (Upgrade Miss)
            send_GETM(request->addr);
            state = MOESIF_CACHE_SM; // S -> SM
            Sim->cache_misses++;
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_E (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            // Read hit
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            // Write hit - Silent upgrade E -> M
            state = MOESIF_CACHE_M;
            Sim->silent_upgrades++;
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_O (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
            // Read hit
            send_DATA_to_proc(request->addr);
            break;
        case STORE:
            // Write hit - Need to invalidate sharers (S/F states)
            send_GETM(request->addr);
            state = MOESIF_CACHE_OM; // O -> OM
            // Can potentially satisfy processor immediately
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: O state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_M (Mreq *request)
{
    switch (request->msg) {
        case LOAD:
        case STORE:
            // Read/Write hit
            send_DATA_to_proc(request->addr);
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_IS (Mreq *request)
{
    switch(request->msg) {
        case LOAD: case STORE:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: IS state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_IM (Mreq *request)
{
    switch(request->msg) {
        case LOAD: case STORE:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: IM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_SM (Mreq *request)
{
    switch(request->msg) {
        case LOAD: case STORE:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: SM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_OM (Mreq *request)
{
    switch(request->msg) {
        case LOAD: case STORE:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: OM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_cache_FM (Mreq *request)
{
    switch(request->msg) {
        case LOAD: case STORE:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Should only have one outstanding request per processor!"); break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: FM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}


inline void MOESIF_protocol::do_snoop_F (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            // Other cache reads, provide data as the forwarder, remain Forward (F)
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            // Remain F
            break;
        case GETM:
            // Other cache writes, invalidate self
            state = MOESIF_CACHE_I; // F -> I on GETM
            break;
        case DATA:
             // Tolerate data, might happen in race conditions
             break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: F state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_I (Mreq *request)
{
    switch (request->msg) {
        case GETS:
        case GETM:
        case DATA:
            // Ignore snoops in Invalid state
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: I state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_S (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            // Another cache reads, remain Shared. Do nothing.
            break;
        case GETM:
            // Another cache writes, invalidate self
            state = MOESIF_CACHE_I;
            break;
        case DATA:
             // Should not normally see data while in S state. Tolerate.
             break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: S state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_E (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            // Other cache reads, provide data, transition to Forward (F)
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_F; // E -> F on GETS
            break;
        case GETM:
            // Other cache writes, provide data, transition to Invalid (I)
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_I; // E -> I on GETM
            break;
        case DATA:
            fatal_error ("Should not see data for this line! I have the line exclusively (E)!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: E state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_O (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            // Other cache reads, provide data, transition to Forward (F)
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            // state = MOESIF_CACHE_F; // O -> F on GETS
            // state = MOESIF_CACHE_O;
            break;
        case GETM:
            // Other cache writes, provide data, transition to Invalid (I)
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_I; // O -> I on GETM
            break;
        case DATA:
            fatal_error ("Should not see data for this line! I have the line in O state!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: O state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_M (Mreq *request)
{
    switch (request->msg) {
        case GETS:
            // Other cache reads, provide data, transition to Forward (F)
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            // state = MOESIF_CACHE_F; // M -> F on GETS
            state = MOESIF_CACHE_O;
            break;
        case GETM:
            // Other cache writes, provide data, transition to Invalid (I)
            send_DATA_on_bus(request->addr, request->src_mid);
            Sim->cache_to_cache_transfers++;
            state = MOESIF_CACHE_I; // M -> I on GETM
            break;
        case DATA:
            fatal_error ("Should not see data for this line! I have the line modified (M)!");
            break;
        default:
            request->print_msg (my_table->moduleID, "ERROR");
            fatal_error ("Client: M state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_IS (Mreq *request)
{
    switch(request->msg) {
        case GETS: case GETM: break; // Ignore other reqs while waiting
        case DATA:
            send_DATA_to_proc(request->addr);
            if (get_shared_line()) { // Data from Cache (E/M/O/F)
                state = MOESIF_CACHE_S; // Become sharer
            } else { // Data from Memory
                state = MOESIF_CACHE_E; // Become exclusive
            }
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: IS state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_IM (Mreq *request)
{
    switch(request->msg) {
        case GETS: case GETM: break; // Ignore other reqs
        case DATA:
            send_DATA_to_proc(request->addr);
            state = MOESIF_CACHE_M; // Go to Modified
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: IM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_SM (Mreq *request)
{
    switch(request->msg) {
        case GETS:
             break; // Ignore
        case GETM:
             break; // Ignore
        case DATA:
            // Received data/ack for our GETM (S->M upgrade)
            send_DATA_to_proc(request->addr); // Satisfy original STORE
            state = MOESIF_CACHE_M; // Go to Modified
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: SM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_OM (Mreq *request)
{
    switch(request->msg) {
        case GETS:
             break; // Ignore
        case GETM:
             break; // Ignore
         case DATA:
             // Received implicit ACK for GETM (O->M transition)
             // Processor already received data in do_cache_O
             state = MOESIF_CACHE_M;
             break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: OM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

inline void MOESIF_protocol::do_snoop_FM (Mreq *request)
{
    switch(request->msg) {
        case GETS:
             // Another cache requested read access while we are upgrading.
             break; // Ignore
        case GETM:
             // Ignore own or other GETM while waiting for DATA ack.
             break; // Ignore
        case DATA:
            // Received data/ack for our GETM (F->M upgrade)
            send_DATA_to_proc(request->addr); // Satisfy original STORE
            state = MOESIF_CACHE_M; // Go to Modified
            break;
        default:
            request->print_msg(my_table->moduleID, "ERROR");
            fatal_error("Client: FM state shouldn't see this message: %s\\n", Mreq::message_t_str[request->msg]);
    }
}

