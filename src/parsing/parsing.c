#include "parsing.h"

/*
 * Function to parse the port number (where we want to run the server) from
 * command line arguments.
 *
 * Returns the port number on success, and 0 on failure.
 */
uint16_t parse_port_number(char *port_number_string) {
    char *endptr;
    errno = 0;

    unsigned long port_number = strtoul(port_number_string, &endptr, 10);

    if (errno != 0 || *endptr != '\0') {
        perror("Error converting input to number");
        return 0;
    }

    if (port_number > UINT16_MAX) {
        fprintf(stderr, "Error: Value %u out of range for uint16_t\n", UINT16_MAX);
        return 0;
    }

    return (uint16_t)port_number;
}

/*
 * Returns a string describing the given Rpc_AcceptStat.
 *
 * The user of this function takes the responsibility to free the returned string.
 */
char *rpc_accept_stat_to_string(Rpc__AcceptStat accept_stat) {
    switch (accept_stat) {
    case RPC__ACCEPT_STAT__SUCCESS:
        return strdup("SUCCESS");
    case RPC__ACCEPT_STAT__PROG_UNAVAIL:
        return strdup("PROG_UNAVAIAL");
    case RPC__ACCEPT_STAT__PROG_MISMATCH:
        return strdup("PROG_MISMATCH");
    case RPC__ACCEPT_STAT__PROC_UNAVAIL:
        return strdup("PROC_UNAVAIL");
    case RPC__ACCEPT_STAT__GARBAGE_ARGS:
        return strdup("GARBAGE_ARGS");
    case RPC__ACCEPT_STAT__SYSTEM_ERR:
        return strdup("SYSTEM_ERR");
    default:
        return strdup("Unknown");
    }
}

/*
 * Returns a string describing the given Rpc__AuthStat.
 *
 * The user of this function takes the responsibility to free the returned string.
 */
char *auth_stat_to_string(Rpc__AuthStat auth_stat) {
    switch (auth_stat) {
    case RPC__AUTH_STAT__AUTH_OK:
        return strdup("AUTH_OK");
    case RPC__AUTH_STAT__AUTH_BADCRED:
        return strdup("AUTH_BADCRED");
    case RPC__AUTH_STAT__AUTH_REJECTEDCRED:
        return strdup("AUTH_REJECTEDCRED");
    case RPC__AUTH_STAT__AUTH_BADVERF:
        return strdup("AUTH_BADVERF");
    case RPC__AUTH_STAT__AUTH_REJECTEDVERF:
        return strdup("AUTH_REJECTEDVERF");
    case RPC__AUTH_STAT__AUTH_TOOWEAK:
        return strdup("AUTH_TOOWEAK");

    case RPC__AUTH_STAT__AUTH_INVALIDRESP:
        return strdup("AUTH_INVALIDRESP");
    case RPC__AUTH_STAT__AUTH_FAILED:
        return strdup("AUTH_FAILED");

    case RPC__AUTH_STAT__AUTH_KERB_GENERIC:
        return strdup("AUTH_KERB_GENERIC");
    case RPC__AUTH_STAT__AUTH_TIMEEXPIRE:
        return strdup("AUTH_TIMEEXPIRE");
    case RPC__AUTH_STAT__AUTH_TKT_FILE:
        return strdup("AUTH_TKT_FILE");
    case RPC__AUTH_STAT__AUTH_DECODE:
        return strdup("AUTH_DECODE");
    case RPC__AUTH_STAT__AUTH_NET_ADDR:
        return strdup("AUTH_NET_ADDR");

    case RPC__AUTH_STAT__RPCSEC_GSS_CREDPROBLEM:
        return strdup("RPCSEC_GSS_CREDPROBLEM");
    case RPC__AUTH_STAT__RPCSEC_GSS_CTXPROBLEM:
        return strdup("RPCSEC_GSS_CTXPROBLEM");
    default:
        return strdup("Unknown");
    }
}

/*
 * Returns a string describing the given Mount__Stat.
 *
 * The user of this function takes the responsibility to free the returned string.
 */
char *mount_stat_to_string(Mount__Stat stat) {
    switch (stat) {
    case MOUNT__STAT__MNT_OK:
        return strdup("MNT_OK");
    case MOUNT__STAT__MNTERR_NOENT:
        return strdup("MNTERR_NOENT");
    case MOUNT__STAT__MNTERR_NOTEXP:
        return strdup("MNTERR_NOTEXP");
    case MOUNT__STAT__MNTERR_IO:
        return strdup("MNTERR_IO");
    case MOUNT__STAT__MNTERR_ACCES:
        return strdup("MNTERR_ACCES");
    case MOUNT__STAT__MNTERR_NOTDIR:
        return strdup("MNTERR_NOTDIR");
    default:
        return strdup("Unknown");
    }
}

/*
 * Returns a string describing the given Nfs__Stat.
 *
 * The user of this function takes the responsibility to free the returned string.
 */
char *nfs_stat_to_string(Nfs__Stat stat) {
    switch (stat) {
    case NFS__STAT__NFS_OK:
        return strdup("NFS_OK");
    case NFS__STAT__NFSERR_PERM:
        return strdup("NFSERR_PERM");
    case NFS__STAT__NFSERR_NOENT:
        return strdup("NFSERR_NOENT");
    case NFS__STAT__NFSERR_IO:
        return strdup("NFSERR_IO");
    case NFS__STAT__NFSERR_NXIO:
        return strdup("NFSERR_NXIO");
    case NFS__STAT__NFSERR_ACCES:
        return strdup("NFSERR_ACCES");
    case NFS__STAT__NFSERR_EXIST:
        return strdup("NFSERR_EXIST");
    case NFS__STAT__NFSERR_NODEV:
        return strdup("NFSERR_NODEV");
    case NFS__STAT__NFSERR_NOTDIR:
        return strdup("NFSERR_NOTDIR");
    case NFS__STAT__NFSERR_ISDIR:
        return strdup("NFSERR_ISDIR");
    case NFS__STAT__NFSERR_FBIG:
        return strdup("NFSERR_FBIG");
    case NFS__STAT__NFSERR_NOSPC:
        return strdup("NFSERR_NOSPC");
    case NFS__STAT__NFSERR_ROFS:
        return strdup("NFSERR_ROFS");
    case NFS__STAT__NFSERR_NAMETOOLONG:
        return strdup("NFSERR_NAMETOOLONG");
    case NFS__STAT__NFSERR_NOTEMPTY:
        return strdup("NFSERR_NOTEMPTY");
    case NFS__STAT__NFSERR_DQUOT:
        return strdup("NFSERR_DQUOT");
    case NFS__STAT__NFSERR_STALE:
        return strdup("NFSERR_STALE");
    case NFS__STAT__NFSERR_WFLUSH:
        return strdup("NFSERR_WFLUSH");
    default:
        return strdup("Unknown");
    }
}