/*
 * Copyright © 2012-2015 VMware, Inc.  All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the “License”); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an “AS IS” BASIS, without
 * warranties or conditions of any kind, EITHER EXPRESS OR IMPLIED.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */


/*
*
* Module Name:  store.c
*
* Abstract: VMware Domain Name Service.
*
* Storage interface - currently only ldap implementation
*/

#include "includes.h"

DWORD
VmDnsStoreCreateZone(
    PVMDNS_ZONE_INFO    pZoneInfo
    )
{
#ifdef _DEBUG
	VmDnsSetState(VMDNS_READY);
	return 0;
#else
    return VmDnsDirCreateZone(pZoneInfo);
#endif
}

DWORD
VmDnsStoreUpdateZone(
    PVMDNS_ZONE_INFO    pZoneInfo
    )
{
#ifdef _DEBUG
	return 0;
#else
	return VmDnsDirUpdateZone(pZoneInfo);
#endif
}

DWORD
VmDnsStoreDeleteZone(
    PCSTR               pszZoneName
    )
{
#ifdef _DEBUG
	return 0;
#else
	return VmDnsDirDeleteZone(pszZoneName);
#endif
}

DWORD
VmDnsStoreAddZoneRecord(
    PCSTR               pszZoneName,
    PVMDNS_RECORD       pRecord
    )
{
#ifdef _DEBUG
	return 0;
#else
	return VmDnsDirAddZoneRecord(pszZoneName, pRecord);
#endif
}

DWORD
VmDnsStoreDeleteZoneRecord(
    PCSTR               pszZoneName,
    PVMDNS_RECORD       pRecord
    )
{
#ifdef _DEBUG
	return 0;
#else
	return VmDnsDirDeleteZoneRecord(pszZoneName, pRecord);
#endif
}

DWORD
VmDnsStoreSaveForwarders(
    DWORD               dwCount,
    PSTR*               ppszForwarders
    )
{
#ifdef _DEBUG
	return 0;
#else
	return VmDnsDirSaveForwarders(dwCount, ppszForwarders);
#endif
}
