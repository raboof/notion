/*
 * ion/query/query.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef ION_QUERY_QUERY_H
#define ION_QUERY_QUERY_H

#include <ioncore/common.h>
#include <ioncore/genframe.h>
#include <ioncore/extl.h>

extern void query_exec(WGenFrame *frame);
extern void query_runwith(WGenFrame *frame, char *cmd, char *prompt);
extern void query_runfile(WGenFrame *frame, char *cmd, char *prompt);
extern void query_attachclient(WGenFrame *frame);
extern void query_gotoclient(WGenFrame *frame);
extern void query_workspace(WGenFrame *frame);
extern void query_workspace_with(WGenFrame *frame);
extern void query_yesno(WGenFrame *frame, const char *prompt, ExtlFn fn);
extern void query_function(WGenFrame *frame);
extern void query_renameworkspace(WGenFrame *frame);
extern void query_renameframe(WGenFrame *frame);

#endif /* ION_QUERY_QUERY_H */
