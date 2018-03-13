/*
 * Copyright (c) 2003-2004 E. Will et al.
 * Rights to this code are documented in doc/LICENSE.
 *
 * This file contains routines to handle the CService SET command.
 */

#include "atheme.h"

// This is imported by other modules/chanserv/set_*.so
mowgli_patricia_t *cs_set_cmdtree;

static void
cs_help_set(struct sourceinfo *si, const char *subcmd)
{
	if (!subcmd)
	{
		command_success_nodata(si, _("***** \2%s Help\2 *****"), chansvs.me->disp);
		command_success_nodata(si, _("Help for \2SET\2:"));
		command_success_nodata(si, " ");
		command_success_nodata(si, _("SET allows you to set various control flags\n"
					"for channels that change the way certain\n"
					"operations are performed on them."));
		command_success_nodata(si, " ");
		command_help(si, cs_set_cmdtree);
		command_success_nodata(si, " ");
		command_success_nodata(si, _("For more specific help use \2/msg %s HELP SET \37command\37\2."), chansvs.me->disp);
		command_success_nodata(si, _("***** \2End of Help\2 *****"));
	}
	else
		help_display_as_subcmd(si, si->service, "SET", subcmd, cs_set_cmdtree);
}

// SET <#channel> <setting> <parameters>
static void
cs_cmd_set(struct sourceinfo *si, int parc, char *parv[])
{
	char *chan;
	char *cmd;
	struct command *c;

	if (parc < 2)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "SET");
		command_fail(si, fault_needmoreparams, _("Syntax: SET <#channel> <setting> [parameters]"));
		return;
	}

	if (parv[0][0] == '#')
	{
		chan = parv[0];
		cmd = parv[1];
	}
	else if (parv[1][0] == '#')
	{
		cmd = parv[0];
		chan = parv[1];
	}
	else
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "SET");
		command_fail(si, fault_badparams, _("Syntax: SET <#channel> <setting> [parameters]"));
		return;
	}

	c = command_find(cs_set_cmdtree, cmd);
	if (c == NULL)
	{
		command_fail(si, fault_badparams, _("Invalid command. Use \2/%s%s help\2 for a command listing."), (ircd->uses_rcommand == false) ? "msg " : "", chansvs.me->disp);
		return;
	}

	parv[1] = chan;
	command_exec(si->service, si, c, parc - 1, parv + 1);
}

static struct command cs_set = {
	.name           = "SET",
	.desc           = N_("Sets various control flags."),
	.access         = AC_NONE,
	.maxparc        = 3,
	.cmd            = &cs_cmd_set,
	.help           = { .func = &cs_help_set },
};

static void
mod_init(struct module ATHEME_VATTR_UNUSED *const restrict m)
{
	service_named_bind_command("chanserv", &cs_set);

	cs_set_cmdtree = mowgli_patricia_create(strcasecanon);
}

static void
mod_deinit(const enum module_unload_intent ATHEME_VATTR_UNUSED intent)
{
	service_named_unbind_command("chanserv", &cs_set);

	mowgli_patricia_destroy(cs_set_cmdtree, NULL, NULL);
}

SIMPLE_DECLARE_MODULE_V1("chanserv/set_core", MODULE_UNLOAD_CAPABILITY_OK)
