/*
 * The Initial Developer of the Original Code is International
 * Business Machines Corporation. Portions created by IBM
 * Corporation are Copyright (C) 2005 International Business
 * Machines Corporation. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the Common Public License as published by
 * IBM Corporation; either version 1 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Common Public License for more details.
 *
 * You should have received a copy of the Common Public License
 * along with this program; if not, a copy can be viewed at
 * http://www.opensource.org/licenses/cpl1.0.php.
 */

#include "tcm_utils.h"
#include "tcm_tspi.h"

static BOOL passUnicode = FALSE;
static BOOL isWellKnown = FALSE;
TSM_HCONTEXT hContext = 0;

static void help(const char *aCmd)
{
	logCmdHelp(aCmd);
	logUnicodeCmdOption();
	logCmdOption("-z, --well-known", _("Use TSM_WELL_KNOWN_SECRET as the operator's default secret."));
	logCmdOption("-p, --op_password_unicode", _("Use TSM UNICODE encoding for operator password to comply with applications using TSM popup boxes"));
}

static int parse(const int aOpt, const char *aArg)
{

	switch (aOpt) {
	case 'p':
		passUnicode = TRUE;
		break;
	case 'z':
		isWellKnown = TRUE;
		break;
	default:
		return -1;
	}
	return 0;
}

static TSM_RESULT
tpmSetOpAuth(TSM_HTCM a_hTpm, TSM_HPOLICY aOpPolicy)
{
	TSM_RESULT result = Tspi_TCM_SetOperatorAuth(a_hTpm, aOpPolicy);
	tspiResult("Tspi_TCM_SetOperatorAuth", result);
	return result;
}

int main(int argc, char **argv)
{

	int iRc = -1;
	char *passwd = NULL;
	int pswd_len;
	TSM_HPOLICY hNewPolicy;
	TSM_HTCM hTpm;
	struct option opts[] = {
	{"well-known", no_argument, NULL, 'z'},
	{"op_password_unicode", no_argument, NULL, 'p'},
	};
	BYTE wellKnown[TCPA_SM3_256_HASH_LEN] = TSM_WELL_KNOWN_SECRET;

	initIntlSys();
	if (genericOptHandler
	    (argc, argv, "zp", opts, sizeof(opts) / sizeof(struct option),
	     parse, help) != 0)
		goto out;

	//Connect to TSM and TCM
	if (contextCreate(&hContext) != TSM_SUCCESS)
		goto out;

	if (contextConnect(hContext) != TSM_SUCCESS)
		goto out_close;

	if (contextGetTpm(hContext, &hTpm) != TSM_SUCCESS)
		goto out_close;

	//Prompt for operator password
	if (!isWellKnown) {
		passwd = _GETPASSWD(_("Enter operator password: "), (int *)&pswd_len, TRUE,
				    passUnicode || useUnicode );
		if (!passwd) {
			logError(_("Failed to get operator password\n"));
			goto out_close;
		}
	} else {
		passwd = (char *)wellKnown;
		pswd_len = sizeof(wellKnown);
	}

	if (contextCreateObject(hContext, TSM_OBJECT_TYPE_POLICY, TSM_POLICY_OPERATOR,
			&hNewPolicy) != TSM_SUCCESS)
		goto out_close;

	if (policySetSecret(hNewPolicy, (UINT32)pswd_len, (BYTE *)passwd) != TSM_SUCCESS)
		goto out_close;

	if (!isWellKnown)
		shredPasswd(passwd);
	passwd = NULL;

	if (tpmSetOpAuth(hTpm, hNewPolicy) != TSM_SUCCESS)
		goto out_close;

	iRc = 0;
	out_close:
	contextClose(hContext);
	out:
	return iRc;
}
