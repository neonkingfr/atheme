/*
 * Copyright (c) 2006 Jilles Tjoelker, et al
 * Rights to this code are as documented in doc/LICENSE.
 *
 * Calls a command without a user_t.
 *
 * $Id: os_testcmd.c 6571 2006-09-29 22:52:13Z jilles $
 */

#include "atheme.h"

DECLARE_MODULE_V1
(
	"operserv/testcmd", FALSE, _modinit, _moddeinit,
	"$Id: os_testcmd.c 6571 2006-09-29 22:52:13Z jilles $",
	"Atheme Development Group <http://www.atheme.org>"
);

struct testcmddata
{
	sourceinfo_t *prevsi;
	boolean_t got_result;
};

static void os_cmd_testcmd(sourceinfo_t *si, int parc, char *parv[]);

static void testcmd_command_fail(sourceinfo_t *si, faultcode_t code, const char *message);
static void testcmd_command_success_nodata(sourceinfo_t *si, const char *message);
static void testcmd_command_success_string(sourceinfo_t *si, const char *result, const char *message);

command_t os_testcmd = { "TESTCMD", "Executes a command without a user_t.",
                        AC_NONE, 3, os_cmd_testcmd };

list_t *os_cmdtree;
list_t *os_helptree;

struct sourceinfo_vtable testcmd_vtable = { testcmd_command_fail, testcmd_command_success_nodata, testcmd_command_success_string };

void _modinit(module_t *m)
{
	MODULE_USE_SYMBOL(os_cmdtree, "operserv/main", "os_cmdtree");
	MODULE_USE_SYMBOL(os_helptree, "operserv/main", "os_helptree");

	command_add(&os_testcmd, os_cmdtree);
	help_addentry(os_helptree, "TESTCMD", "help/operserv/testcmd", NULL);
}

void _moddeinit()
{
	command_delete(&os_testcmd, os_cmdtree);
	help_delentry(os_helptree, "TESTCMD");
}

static void testcmd_command_fail(sourceinfo_t *si, faultcode_t code, const char *message)
{
	struct testcmddata *udata = si->callerdata;

	command_success_nodata(udata->prevsi, "Command failed with fault %d, \"%s\"", code, message);
	udata->got_result = TRUE;
}

static void testcmd_command_success_nodata(sourceinfo_t *si, const char *message)
{
	struct testcmddata *udata = si->callerdata;

	if (udata->got_result)
		command_success_nodata(udata->prevsi, "More comment \"%s\"", message);
	else
		command_success_nodata(udata->prevsi, "Command succeeded with no data, \"%s\"", message);
	udata->got_result = TRUE;
}

static void testcmd_command_success_string(sourceinfo_t *si, const char *result, const char *message)
{
	struct testcmddata *udata = si->callerdata;

	command_success_nodata(udata->prevsi, "Command succeeded with string \"%s\", \"%s\"", result, message);
	udata->got_result = TRUE;
}

static void os_cmd_testcmd(sourceinfo_t *si, int parc, char *parv[])
{
	service_t *svs;
	command_t *cmd;
	sourceinfo_t newsi;
	struct testcmddata udata;
	int newparc;
	char *newparv[256];

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "TESTCMD");
		command_fail(si, fault_needmoreparams, "Syntax: TESTCMD <service> <command> [arguments]");
		return;
	}

	svs = find_service(parv[0]);
	if (svs == NULL)
	{
		command_fail(si, fault_nosuch_target, "No such service \2%s\2", parv[0]);
		return;
	}
	if (svs->cmdtree == NULL)
	{
		command_fail(si, fault_noprivs, "Service \2%s\2 has no commands", svs->name);
		return;
	}
	cmd = command_find(svs->cmdtree, parv[1]);
	if (cmd == NULL)
	{
		command_fail(si, fault_nosuch_key, "No such command \2%s\2 in service \2%s\2", parv[1], svs->name);
		return;
	}
	udata.prevsi = si;
	udata.got_result = FALSE;
	memset(newparv, '\0', sizeof newparv);
	if (parc >= 3)
		newparc = sjtoken(parv[2], ';', newparv);
	else
		newparc = 0;
	memset(&newsi, '\0', sizeof newsi);
	newsi.smu = si->smu;
	newsi.service = svs;
	newsi.v = &testcmd_vtable;
	newsi.callerdata = &udata;
	command_exec(svs, &newsi, cmd, newparc, newparv);
	if (!udata.got_result)
		command_success_nodata(si, "Command returned without giving a result");
}
