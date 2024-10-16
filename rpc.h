#ifndef rpc__header__INCLUDED
#define rpc__header__INCLUDED

#define RPC_SERVER_PORT 8080

uint32_t generate_rpc_xid() {
    return rand();
}

#endif /* rpc__header__INCLUDED */

/*
enum msg_type {
    CALL    = 0,
    REPLY   = 1
};

enum reply_stat {
    MSG_ACCEPTED    = 0,
    MSG_DENIED      = 1
};

enum accept_stat {
    SUCCESS         = 0, // RPC executed successfully
    PROG_UNAVAIL    = 1, // remote hasn't exported program
    PROG_MISMATCH   = 2, // remote can't support version
    PROC_UNAVAIL    = 3, // program can't support procedure
    GARBAGE_ARGS    = 4, // procedure can't decode params
    SYSTEM_ERR      = 5  // memory allocation failure
};

enum reject_stat {
    RPC_MISMATCH    = 0, // RPC version number != 2
    AUTH_ERROR      = 1  // remote can't authenticate caller
};

enum auth_stat {
    AUTH_OK         = 0,
};

struct call_body {
    unsigned int rpcvers; // 2
    unsigned int prog;  // RPC program number
    unsigned int vers;  // version number of the program
    unsigned int proc;  // procedure number

    // auth missing

    // procedure-specific params here
    
};

struct accepted_reply {
    // auth missing

    accept_stat stat;
    union {
        // case SUCCESS -> procedure-specific results
        struct {
            unsigned int low;
            unsigned int high;
        } mismatch_info; // case PROG_MISMATCH
        // default -> void
    } reply_data;
};

struct rejected_reply {
    reject_stat stat;
    union {
        struct {
            unsigned int low;
            unsigned int high;
        } mismatch_info; // case RPC_MISMATCH
        auth_stat stat;  // case AUTH_ERROR
    } reply_data;
};

struct reply_body {
    reply_stat stat;
    union {
        accepted_reply areply; // case MSG_ACCEPTED
        rejected_reply rreply; // case MSG_DENIED
    } reply;
};

struct rpc_msg {
    unsigned int xid;
    msg_type mtype;
    union {
        call_body cbody;  // case CALL
        reply_body rbody; // case REPLY
    } body;
};
*/