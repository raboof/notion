/*
 * query/complete_file.h
 *
 * Copyright (c) Tuomo Valkonen 1999-2002. 
 * See the included file LICENSE for details.
 */

#ifndef QUERY_COMPLETE_FILE_H
#define QUERY_COMPLETE_FILE_H

int complete_file(char *pathname, char ***avp, char **beg);
int complete_file_with_path(char *pathname, char ***avp, char **beg);

#endif /* QUERY_COMPLETE_FILE_H */
