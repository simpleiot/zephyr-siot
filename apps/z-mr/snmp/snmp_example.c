/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Dirk Ziegelmeier <dziegel@gmx.de>
 *
 */

#include "lwip/netif.h"
#include "lwip/apps/snmp.h"
#include "lwip/apps/snmp_mib2.h"
#include "lwip/apps/snmpv3.h"
#include "lwip/apps/snmp_snmpv2_framework.h"
#include "lwip/apps/snmp_snmpv2_usm.h"
#if LWIP_SNMP_V3
#include "examples/snmp/snmp_v3/snmpv3_dummy.h"
#endif
#include "lwip/private_mib.h"
#include "lwip/apps/snmp_example.h"

/* mib_ats2 is declared in this file: */
#include "z-mr.mib.h"

#if LWIP_SNMP
static const struct snmp_mib *mibs[] = {&mib2, &mib_ats2
#if LWIP_SNMP_V3
					,
					&snmpframeworkmib, &snmpusmmib
#endif
};
#endif /* LWIP_SNMP */

void init_snmp_agent(void)
{
#if LWIP_SNMP
	s32_t req_nr;

#if SNMP_LWIP_MIB2
#if SNMP_USE_NETCONN
	snmp_threadsync_init(&snmp_mib2_lwip_locks, snmp_mib2_lwip_synchronizer);
#endif /* SNMP_USE_NETCONN */
	snmp_mib2_set_syscontact_readonly((const u8_t *)"root", NULL);
	snmp_mib2_set_syslocation_readonly((const u8_t *)"lwIP development PC", NULL);
	snmp_mib2_set_sysdescr((const u8_t *)"lwIP example", NULL);
#endif /* SNMP_LWIP_MIB2 */

#if LWIP_SNMP_V3
	snmpv3_dummy_init();
#endif

	snmp_set_mibs(mibs, LWIP_ARRAYSIZE(mibs));

	snmp_init();

#endif /* LWIP_SNMP */
}
