/*
 * query/complete_file.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2003. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_COMPLETE_FILE_H
#define QUERY_COMPLETE_FILE_H

int complete_file(char *pathname, char ***avp, char **beg, void *unused);
int complete_file_with_path(char *pathname, char ***avp, char **beg,
							void *unused);

#endif /* QUERY_COMPLETE_FILE_H */
