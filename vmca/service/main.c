/*
 * Copyright © 2012-2016 VMware, Inc.  All Rights Reserved.
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
#include "includes.h"

static
VOID
PrintCurrentState(
	VOID
	);

static DWORD
VMCAParseArgs(
    int argc,
    char* argv[],
    PBOOL pbEnableSysLog
)
{
    DWORD dwError = ERROR_SUCCESS;
    int opt = 0;
    BOOL bEnableSysLog = FALSE;

    while ( (opt = getopt( argc, argv, VMCA_OPTIONS_VALID)) != EOF )
    {
        switch ( opt )
        {
            case VMCA_OPTION_ENABLE_SYSLOG:
                bEnableSysLog = TRUE;
                break;

            default:
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_VMCA_ERROR(dwError);
        }
    }

    if (pbEnableSysLog != NULL)
    {
        *pbEnableSysLog = bEnableSysLog;  
    }

error:
    return dwError;
}

int
main(
    int   argc,
    char* argv[]
    )
{
    DWORD dwError = 0;
    const char* pszSmNotify = NULL;
    int notifyFd = -1;
    int notifyCode = 0;
    int ret = -1;
    BOOL bEnableSysLog = FALSE;

    setlocale(LC_ALL, "");

    VMCABlockSelectedSignals();

    dwError = VMCAParseArgs(argc, argv, &bEnableSysLog);
    BAIL_ON_VMCA_ERROR(dwError);

    if (bEnableSysLog)
    {
        gVMCALogType = VMCA_LOG_TYPE_SYSLOG;
    }
    else
    {
        gVMCALogType = VMCA_LOG_TYPE_FILE;
    }

    dwError  = VMCAInitialize(0, 0);
    BAIL_ON_VMCA_ERROR(dwError);

    VMCA_LOG_INFO("VM Certificate Service started.");

    PrintCurrentState();

    // interact with likewise service manager (start/stop control)
    if ((pszSmNotify = getenv("LIKEWISE_SM_NOTIFY")) != NULL)
    {
        notifyFd = atoi(pszSmNotify);

        do
        {
            ret = write(notifyFd, &notifyCode, sizeof(notifyCode));

        } while (ret != sizeof(notifyCode) && errno == EINTR);

        if (ret < 0)
        {
            VMCA_LOG_ERROR("Could not notify service manager: %s (%i)",
                            strerror(errno),
                            errno);
            dwError = LwErrnoToWin32Error(errno);
            BAIL_ON_VMCA_ERROR(dwError);
        }

        close(notifyFd);
    }

    // main thread waits on signals
    dwError = VMCAHandleSignals();
    BAIL_ON_VMCA_ERROR(dwError);

    VMCA_LOG_INFO("VM Certificate Service exiting...");

cleanup:

    VMCAShutdown();

    return (dwError);

error:

    VMCA_LOG_ERROR("VM Certificate exiting due to error [code:%d]", dwError);

    goto cleanup;
}

static
VOID
PrintCurrentState(
	VOID
	)
{
	DWORD dwFuncLevel = VMCASrvGetFuncLevel();

    if (dwFuncLevel == VMCA_FUNC_LEVEL_INITIAL) {
        printf("VMCA Server Functional level is VMCA_FUNC_LEVEL_INITIAL\n");
    }

    if ((dwFuncLevel & VMCA_FUNC_LEVEL_SELF_CA) == VMCA_FUNC_LEVEL_SELF_CA) {
        printf("VMCA Server Functional level is VMCA_FUNC_LEVEL_SELF_CA\n");
    }
}

