#ifndef __OID_H
#define __OID_H

#ifdef CONFIG_64BIT
typedef unsigned int oid;
#define MAX_SUBID   0xFFFFFFFFUL
#define NETSNMP_PRIo ""
#else
#ifndef EIGHTBIT_SUBIDS
typedef unsigned long oid;
#define MAX_SUBID   0xFFFFFFFFUL
#define NETSNMP_PRIo "l"
#else
typedef unsigned char oid;
#define MAX_SUBID   0xFF
#define NETSNMP_PRIo ""
#endif
#endif /* CONFIG_64BIT */

#endif /* __OID_H */
