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
 * Module Name: vdcrepadmin
 *
 * Filename: main.c
 *
 * Abstract:
 *
 * vdcrepadmin main module entry point
 *
 */

#include "includes.h"

static
DWORD
_VmDirGetConnection(
    PCSTR   pszHostName,
    PCSTR   pszUserName,
    PCSTR   pszPassword,
    PVMDIR_CONNECTION* ppConnection
    )
{
    DWORD   dwError = 0;
    PSTR    pszDomainName = NULL;
    PSTR    pszURI = NULL;
    PVMDIR_CONNECTION pConnection = NULL;

    if ( VmDirIsIPV6AddrFormat( pszHostName ) )
    {
        dwError = VmDirAllocateStringPrintf( &pszURI, "ldap://[%s]", pszHostName );
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else
    {
        dwError = VmDirAllocateStringPrintf( &pszURI, "ldap://%s", pszHostName );
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirGetDomainName( pszHostName, &pszDomainName );
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirConnectionOpen( pszURI, pszDomainName, pszUserName, pszPassword, &pConnection );
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppConnection = pConnection;

cleanup:
    VMDIR_SAFE_FREE_MEMORY(pszDomainName);
    VMDIR_SAFE_FREE_MEMORY(pszURI);
    return dwError;

error:
    VmDirConnectionClose(pConnection);
    goto cleanup;
}

static
DWORD
_VmDirGetDCList(
    PCSTR   pszHostName,
    PCSTR   pszUserName,
    PCSTR   pszPassword,
    PVMDIR_STRING_LIST*  ppDCList
    )
{
    DWORD   dwError = 0;
    DWORD   dwCnt = 0;
    PSTR    pszName = NULL;
    PVMDIR_SERVER_INFO  pServerInfo = NULL;
    DWORD               dwServerInfoCount = 0;
    PVMDIR_STRING_LIST  pDCList = NULL;

    dwError = VmDirStringListInitialize(&pDCList, 16);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetServers(
                pszHostName,
                pszUserName,
                pszPassword,
                &pServerInfo,
                &dwServerInfoCount
                );
    BAIL_ON_VMDIR_ERROR(dwError);

    for (dwCnt=0; dwCnt<dwServerInfoCount; dwCnt++)
    {
        PCSTR   pszTmp = VmDirStringChrA(pServerInfo[dwCnt].pszServerDN, ',');

        if (pszTmp == NULL)
        {
            dwError = VMDIR_ERROR_INVALID_PARAMETER;
            BAIL_ON_VMDIR_ERROR(dwError);
        }

        VMDIR_SAFE_FREE_MEMORY(pszName);
        dwError = VmDirAllocateStringOfLenA(
                    pServerInfo[dwCnt].pszServerDN+3,
                    (DWORD)(pszTmp-pServerInfo[dwCnt].pszServerDN-3),
                    &pszName);
        BAIL_ON_VMDIR_ERROR(dwError);

        dwError = VmDirStringListAdd(pDCList, pszName);
        BAIL_ON_VMDIR_ERROR(dwError);
        pszName = NULL;
    }

    *ppDCList = pDCList;

cleanup:
    for (dwCnt=0; dwCnt<dwServerInfoCount; dwCnt++)
    {
        VMDIR_SAFE_FREE_MEMORY(pServerInfo[dwCnt].pszServerDN);
    }

    VMDIR_SAFE_FREE_MEMORY(pServerInfo);
    VMDIR_SAFE_FREE_MEMORY(pszName);
    return dwError;

error:
    VmDirStringListFree(pDCList);
    goto cleanup;
}

static
VOID
_VmDirPrintReplState(
    PCSTR             pszHostName,
    PVMDIR_REPL_STATE pReplState)
{
    PVMDIR_REPL_UTDVECTOR       pVector = NULL;
    PVMDIR_REPL_REPL_AGREEMENT  pRA = NULL;

    if (pReplState)
    {
        printf("Domain Controller: %s\n",VDIR_SAFE_STRING(pReplState->pszHost) );
        printf("Invocation ID: %s\n",VDIR_SAFE_STRING(pReplState->pszInvocationId) );
        printf("Replication cycle count: %d\n",pReplState->dwCycleCount );
        printf("Max consumable  USN: %lu\n",pReplState->maxConsumableUSN );
        printf("Max originating USN: %lu\n",pReplState->maxOriginatingUSN );

        pVector = pReplState->pReplUTDVec;
        while (pVector)
        {
            printf("Has seen %lu USN from %s\n",pVector->maxOriginatingUSN,
                   VDIR_SAFE_STRING(pVector->pszPartnerInvocationId) );
            pVector = pVector->next;
        }

        pRA = pReplState->pReplRA;
        while (pRA)
        {
            printf("Has processed %lu USN from %s\n", pRA->maxProcessedUSN,
                   VDIR_SAFE_STRING(pRA->pszPartnerName));
            pRA = pRA->next;
        }

        printf("\n\n");
    }
}

static
DWORD
VmDirGetFederationStatus(
    PCSTR   pszHostName,
    PCSTR   pszUserName,
    PCSTR   pszPassword
    )
{
    DWORD   dwError = 0;
    DWORD   dwCnt = 0;
    PVMDIR_STRING_LIST pDCList = NULL;
    PVMDIR_CONNECTION  pConnection = NULL;
    PVMDIR_REPL_STATE  pReplState = NULL;

    dwError = _VmDirGetDCList(
                pszHostName,
                pszUserName,
                pszPassword,
                &pDCList);
    BAIL_ON_VMDIR_ERROR(dwError);

    for (dwCnt=0; dwCnt<pDCList->dwCount; dwCnt++)
    {
        VmDirConnectionClose( pConnection );
        pConnection = NULL;
        dwError = _VmDirGetConnection( pDCList->pStringList[dwCnt],
                                       pszUserName,
                                       pszPassword,
                                       &pConnection);
        if (dwError == VMDIR_ERROR_SERVER_DOWN)
        {
            printf("Domain Controller: %s is NOT available\n\n", pDCList->pStringList[dwCnt] );
            dwError = 0;
            continue;
        }
        BAIL_ON_VMDIR_ERROR(dwError);

        VmDirFreeReplicationState(pReplState);
        pReplState = NULL;
        dwError = VmDirGetReplicationState(pConnection, &pReplState);
        if (dwError == VMDIR_ERROR_ENTRY_NOT_FOUND)
        {
            printf("Domain Controller: %s is NOT supported\n\n", pDCList->pStringList[dwCnt] );
            dwError = 0;
            continue;
        }
        BAIL_ON_VMDIR_ERROR(dwError);

        _VmDirPrintReplState(pDCList->pStringList[dwCnt], pReplState);
    }

cleanup:
    VmDirStringListFree(pDCList);
    VmDirConnectionClose( pConnection );
    VmDirFreeReplicationState(pReplState);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
_VmDirGetReplicateStatusCycle(
    PCSTR   pszHostName,
    PCSTR   pszUserName,
    PCSTR   pszPassword
    )
{
    DWORD   dwError = 0;
    DWORD   dwCycleCount = 0;
    PVMDIR_CONNECTION pConnection = NULL;

    dwError = _VmDirGetConnection( pszHostName,
                                   pszUserName,
                                   pszPassword,
                                   &pConnection);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetReplicationCycleCount( pConnection, &dwCycleCount );
    BAIL_ON_VMDIR_ERROR(dwError);

    if (dwCycleCount == 0)
    {
        printf("First replication cycle done: FALSE\n");
    }
    else
    {
        printf("First replication cycle done: TRUE\n");
    }

cleanup:
    VmDirConnectionClose( pConnection );

    return dwError;

error:
    printf("First replication cycle done: UNKNOWN, error code (%u)\n", dwError);
    goto cleanup;
}

DWORD
VmDirShowReplicationPartnerInfo(
    DWORD               dwNumPartner,
    PVMDIR_REPL_PARTNER_INFO   pReplPartnerInfo
)
{
    DWORD dwError = 0;
    DWORD i = 0;
    for (i=0; i<dwNumPartner; i++)
    {
        printf("%s\n", pReplPartnerInfo[i].pszURI);
    }
    return dwError;
}

DWORD
VmDirShowReplicationPartnerStatus(
    DWORD               dwNumPartner,
    PVMDIR_REPL_PARTNER_STATUS   pReplPartnerStatus
)
{
    DWORD dwError = 0;
    DWORD i = 0;
    USN lag = 0;
    for (i=0; i<dwNumPartner; i++)
    {
        printf("Partner: %s\n", pReplPartnerStatus[i].pszHost);
        printf("Host available:   %s\n", pReplPartnerStatus[i].bHostAvailable ? "Yes" : "No");
        if (pReplPartnerStatus[i].bHostAvailable)
        {
            printf("Status available: %s\n", pReplPartnerStatus[i].bStatusAvailable ? "Yes" : "No");
            if (pReplPartnerStatus[i].bStatusAvailable)
            {
                lag = pReplPartnerStatus[i].targetUsn - pReplPartnerStatus[i].partnerUsn;
#ifdef _WIN32
                printf("My last change number:             %I64d\n", (_Longlong)pReplPartnerStatus[i].targetUsn);
                printf("Partner has seen my change number: %I64d\n", (_Longlong)pReplPartnerStatus[i].partnerUsn);
                printf("Partner is %I64d changes behind.\n", (_Longlong)lag);
#else
                printf("My last change number:             %" PRId64 "\n", (int64_t)pReplPartnerStatus[i].targetUsn);
                printf("Partner has seen my change number: %" PRId64 "\n", (int64_t)pReplPartnerStatus[i].partnerUsn);
                printf("Partner is %" PRId64 " changes behind.\n", (int64_t)lag);
#endif
            }
        }
        if (i < dwNumPartner - 1)
        {
            printf("\n");
        }
    }
    return dwError;
}

DWORD
VmDirShowServerInfo(
    DWORD               dwNumServer,
    PVMDIR_SERVER_INFO   pServerInfo
)
{
    DWORD dwError = 0;
    DWORD i = 0;
    for (i=0; i<dwNumServer; i++)
    {
        printf("%s\n", pServerInfo[i].pszServerDN);
    }
    return dwError;
}

static
int
VmDirMain(int argc, char* argv[])
{
    DWORD       dwError = 0;
    DWORD       i       = 0;
    PSTR        pszFeatureSet  = NULL;
    PSTR        pszSrcHostName = NULL;
    PSTR        pszSrcPort     = NULL;
    PSTR        pszSrcUserName = NULL;
    PSTR        pszSrcPassword = NULL;
    PSTR        pszTgtHostName = NULL;
    PSTR        pszTgtPort     = NULL;
    BOOLEAN     bVerbose       = FALSE;
    BOOLEAN     bTwoWayRepl    = FALSE;
    PSTR        pszErrMsg      = NULL;
    CHAR        pszPasswordBuf[VMDIR_MAX_PWD_LEN + 1] = {0};

    PVMDIR_REPL_PARTNER_INFO    pReplPartnerInfo       = NULL;
    PVMDIR_REPL_PARTNER_STATUS  pReplPartnerStatus     = NULL;
    PVMDIR_SERVER_INFO          pServerInfo            = NULL;
    DWORD                       dwReplPartnerInfoCount = 0;
    DWORD                       dwReplPartnerStatusCount = 0;
    DWORD                       dwServerInfoCount      = 0;

    CHAR        pszPath[MAX_PATH];

#ifndef _WIN32
    setlocale(LC_ALL,"");
#endif

    dwError = VmDirGetVmDirLogPath(pszPath, "vdcrepadmin.log");
    BAIL_ON_VMDIR_ERROR(dwError);
    dwError = VmDirLogInitialize(pszPath, FALSE, NULL, VMDIR_LOG_INFO, VMDIR_LOG_MASK_ALL );
    BAIL_ON_VMDIR_ERROR(dwError);

    //get commandline parameters
    dwError = VmDirParseArgs(
                    argc,
                    argv,
                    &pszFeatureSet,
                    &bTwoWayRepl,
                    &pszSrcHostName,
                    &pszSrcPort,
                    &pszSrcUserName,
                    &pszSrcPassword,
                    &pszTgtHostName,
                    &pszTgtPort,
                    &bVerbose
                    );

    if (bVerbose)
    {
        VmDirSetLogLevel( "VERBOSE" );
    }

    if (dwError)
    {
        ShowUsage();
        goto cleanup;
    }

    if (pszSrcPassword == NULL)
    {
        // read password from stdin
        VmDirReadString(
            "password: ",
            pszPasswordBuf,
            sizeof(pszPasswordBuf),
            TRUE);
        pszSrcPassword = pszPasswordBuf;
    }

    if ( VmDirStringCompareA(VDCREPADMIN_FEATURE_SHOW_PARTNERS, pszFeatureSet, TRUE) == 0 )
    {
        dwError = VmDirGetReplicationPartners(
                        pszSrcHostName,
                        pszSrcUserName,
                        pszSrcPassword,
                        &pReplPartnerInfo,
                        &dwReplPartnerInfoCount
                        );
        BAIL_ON_VMDIR_ERROR(dwError);

        //Show replication partner info
        dwError = VmDirShowReplicationPartnerInfo(
                                dwReplPartnerInfoCount,
                                pReplPartnerInfo
                                );

        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else if ( VmDirStringCompareA(VDCREPADMIN_FEATURE_SHOW_PARTNER_STATUS, pszFeatureSet, TRUE) == 0 )
    {
        dwError = VmDirGetReplicationPartnerStatus(
                        pszSrcHostName,
                        pszSrcUserName,
                        pszSrcPassword,
                        &pReplPartnerStatus,
                        &dwReplPartnerStatusCount
                        );
        BAIL_ON_VMDIR_ERROR(dwError);

        //Show replication partner info
        dwError = VmDirShowReplicationPartnerStatus(
                                dwReplPartnerStatusCount,
                                pReplPartnerStatus
                                );
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else if ( VmDirStringCompareA(VDCREPADMIN_FEATURE_SHOW_FEDERATION_STATUS, pszFeatureSet, TRUE) == 0 )
       {
           dwError = VmDirGetFederationStatus(
                           pszSrcHostName,
                           pszSrcUserName,
                           pszSrcPassword
                           );
           BAIL_ON_VMDIR_ERROR(dwError);

       }
    else if ( VmDirStringCompareA(VDCREPADMIN_FEATURE_SHOW_SERVER_ATTRIBUTE, pszFeatureSet, TRUE) == 0 )
    {
        dwError = VmDirGetServers(
                        pszSrcHostName,
                        pszSrcUserName,
                        pszSrcPassword,
                        &pServerInfo,
                        &dwServerInfoCount
                        );
        BAIL_ON_VMDIR_ERROR(dwError);

        //Show replication partner info
        dwError = VmDirShowServerInfo(
                                dwServerInfoCount,
                                pServerInfo
                                );
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else if ( VmDirStringCompareA(VDCREPADMIN_FEATURE_CREATE_AGREEMENT, pszFeatureSet, TRUE) == 0 )
    {
        dwError = VmDirAddReplicationAgreement(
                    bTwoWayRepl,
                    pszSrcHostName,
                    pszSrcPort,
                    pszSrcUserName,
                    pszSrcPassword,
                    pszTgtHostName,
                    pszTgtPort
                    );
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else if ( VmDirStringCompareA(VDCREPADMIN_FEATURE_REMOVE_AGREEMENT, pszFeatureSet, TRUE) == 0 )
    {
        dwError = VmDirRemoveReplicationAgreement(
                    bTwoWayRepl,
                    pszSrcHostName,
                    pszSrcPort,
                    pszSrcUserName,
                    pszSrcPassword,
                    pszTgtHostName,
                    pszTgtPort
                    );
        BAIL_ON_VMDIR_ERROR(dwError);
    }
    else if ( VmDirStringCompareA(VDCREPADMIN_QUERY_IS_FIRST_CYCLE_DONE, pszFeatureSet, TRUE) == 0 )
    {
        dwError = _VmDirGetReplicateStatusCycle(
                        pszSrcHostName,
                        pszSrcUserName,
                        pszSrcPassword
                        );
        BAIL_ON_VMDIR_ERROR(dwError);
    }

cleanup:
    // Free internal memory used
    for (i=0; i<dwReplPartnerInfoCount; i++)
    {
        VMDIR_SAFE_FREE_MEMORY(pReplPartnerInfo[i].pszURI);
    }
    VMDIR_SAFE_FREE_MEMORY(pReplPartnerInfo);

    for (i=0; i<dwReplPartnerStatusCount; i++)
    {
        VMDIR_SAFE_FREE_MEMORY(pReplPartnerStatus[i].pszHost);
    }
    VMDIR_SAFE_FREE_MEMORY(pReplPartnerStatus);
    VMDIR_SAFE_FREE_MEMORY(pszErrMsg);

    // Free internal memory used
    for (i=0; i<dwServerInfoCount; i++)
    {
        VMDIR_SAFE_FREE_MEMORY(pServerInfo[i].pszServerDN);
    }

    VMDIR_SAFE_FREE_MEMORY(pServerInfo);
    memset(pszPasswordBuf, 0, sizeof(pszPasswordBuf));

    return dwError;

error:
    VmDirGetErrorMessage(dwError, &pszErrMsg ); // ignore error
    printf("Vdcrepadmin failed. Error [%s] [%d]\n", VDIR_SAFE_STRING(pszErrMsg),dwError);
    goto cleanup;
}

#ifdef _WIN32

int wmain(int argc, wchar_t* argv[])
{
    DWORD dwError = 0;
    PSTR* ppszArgs = NULL;
    int   iArg = 0;

    dwError = VmDirAllocateMemory(sizeof(PSTR) * argc, (PVOID*)&ppszArgs);
    BAIL_ON_VMDIR_ERROR(dwError);

    for (; iArg < argc; iArg++)
    {
        dwError = VmDirAllocateStringAFromW(argv[iArg], &ppszArgs[iArg]);
        BAIL_ON_VMDIR_ERROR(dwError);
    }

    dwError = VmDirMain(argc, ppszArgs);
    BAIL_ON_VMDIR_ERROR(dwError);

error:

    if (ppszArgs)
    {
        for (iArg = 0; iArg < argc; iArg++)
        {
            VMDIR_SAFE_FREE_MEMORY(ppszArgs[iArg]);
        }
        VmDirFreeMemory(ppszArgs);
    }

    return dwError;
}
#else

int main(int argc, char* argv[])
{
    return VmDirMain(argc, argv);
}

#endif
