#include "gup.h"

static const char *hlist[] =
{
"",
"--------------------------------------------------------------------------",
"The automated news update program accepts the following commands:",
"site, include, exclude, poison, delete, list, newsgroups, help and quit.",
"",
"The 'site' command *must* come prior to any of the other commands.",
"After that any order of commands is acceptable. Note that commands are",
"processed in order and that the 'list' command displays the current state",
"as affected by any preceeding commands.",
"",
"Syntax:",
"",
"site           <sitename>      <password>",
"include        <pattern>",
"exclude        <pattern>",
"pattern        <pattern>",
"delete         <pattern>",
"list",
"newsgroups     <pattern>",
"quit",
"",
"Where:",
"",
 "sitename       must be a valid site registered in the gup config file",
"password       Must match the registered password",
"pattern        Is a single 'wildmat' pattern of a newsgroup.",
"               In the case of 'Include' and 'Exclude' it is matched",
"               against the active file.",
"               With the 'delete' command. <pattern> is matched against the",
"               current group list.",
"",
"The 'include' command adds all groups that match the pattern into your",
"current group list. The 'exclude' command is typically used in conjunction",
"with a wildcard 'include' command, eg:",
"",
"include        comp.sys.*",
"exclude        comp.sys.weirdosystems",
"",
"In the above example, all of the groups in the comp.sys hierarchy will be",
"included, except for comp.sys.weirdosystems. In other words it's a merely",
"a convenient way of refining a large Include list.",
"",
"You can use the poison command to specify pattern which cause all crossposts",
"to matching groups to be dropped, regardless whether the article would be",
"accepted due to other groups. Example:",
"",
"poison         *.binari*",
"",
"The 'delete' command removes all matching 'include', 'exclude' and 'poison'"
"patterns from the current group list.",
"",
"newsgroups lists out all the valid newsgroups that match the pattern.",
"",
"quit is used to stop gup from parsing the rest of the mail, that typically",
"has signatures and such baggage on the end of it",
"--------------------------------------------------------------------------",
    NULL
};


int help(const char **tokens)
{
    const char **hp;

    for (hp = hlist; *hp; hp++)
	logit(L_MAIL, *hp);
    return 0;
}
