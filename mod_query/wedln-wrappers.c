/*
 * ion/mod_query/wedln-wrappers.c
 *
 * Copyright (c) Tuomo Valkonen 1999-2008. 
 *
 * See the included file LICENSE for details.
 */

#include "wedln.h"
#include "edln.h"
#include "complete.h"

/*EXTL_DOC
 * Move backward one character.
 */
EXTL_EXPORT_MEMBER
void wedln_back(WEdln *wedln)
{
    edln_back(&(wedln->edln));
}

/*EXTL_DOC
 * Move forward one character.
 */
EXTL_EXPORT_MEMBER
void wedln_forward(WEdln *wedln)
{
    edln_forward(&(wedln->edln));
}

/*EXTL_DOC
 * Transpose characters.
 */
EXTL_EXPORT_MEMBER
void wedln_transpose_chars(WEdln *wedln)
{
    edln_transpose_chars(&(wedln->edln));
}

/*EXTL_DOC
 * Transpose words.
 */
EXTL_EXPORT_MEMBER
void wedln_transpose_words(WEdln *wedln)
{
    edln_transpose_words(&(wedln->edln));
}

/*EXTL_DOC
 * Go to the beginning of line.
 */
EXTL_EXPORT_MEMBER
void wedln_bol(WEdln *wedln)
{
    edln_bol(&(wedln->edln));
}

/*EXTL_DOC
 * Go to the end of line.
 */
EXTL_EXPORT_MEMBER
void wedln_eol(WEdln *wedln)
{
    edln_eol(&(wedln->edln));
}

/*EXTL_DOC
 * Go to to end of current sequence of whitespace followed by alphanumeric
 * characters..
 */
EXTL_EXPORT_MEMBER
void wedln_skip_word(WEdln *wedln)
{
    edln_skip_word(&(wedln->edln));
}

/*EXTL_DOC
 * Go to to beginning of current sequence of alphanumeric characters
 * followed by whitespace.
 */
EXTL_EXPORT_MEMBER
void wedln_bskip_word(WEdln *wedln)
{
    edln_bskip_word(&(wedln->edln));
}

/*EXTL_DOC
 * Delete current character.
 */
EXTL_EXPORT_MEMBER
void wedln_delete(WEdln *wedln)
{
    edln_delete(&(wedln->edln));
}

/*EXTL_DOC
 * Delete previous character.
 */
EXTL_EXPORT_MEMBER
void wedln_backspace(WEdln *wedln)
{
    edln_backspace(&(wedln->edln));
}

/*EXTL_DOC
 * Delete all characters from current to end of line.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_to_eol(WEdln *wedln)
{
    edln_kill_to_eol(&(wedln->edln));
}

/*EXTL_DOC
 * Delete all characters from previous to beginning of line.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_to_bol(WEdln *wedln)
{
    edln_kill_to_bol(&(wedln->edln));
}

/*EXTL_DOC
 * Delete the whole line.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_line(WEdln *wedln)
{
    edln_kill_line(&(wedln->edln));
}

/*EXTL_DOC
 * Starting from the current point, delete possible whitespace and
 * following alphanumeric characters until next non-alphanumeric character.
 */
EXTL_EXPORT_MEMBER
void wedln_kill_word(WEdln *wedln)
{
    edln_kill_word(&(wedln->edln));
}

/*EXTL_DOC
 * Starting from the previous characters, delete possible whitespace and
 * preceding alphanumeric characters until previous non-alphanumeric character.
 */
EXTL_EXPORT_MEMBER
void wedln_bkill_word(WEdln *wedln)
{
    edln_bkill_word(&(wedln->edln));
}

/*EXTL_DOC
 * Set \emph{mark} to current cursor position.
 */
EXTL_EXPORT_MEMBER
void wedln_set_mark(WEdln *wedln)
{
    edln_set_mark(&(wedln->edln));
}

/*EXTL_DOC
 * Clear \emph{mark}.
 */
EXTL_EXPORT_MEMBER
void wedln_clear_mark(WEdln *wedln)
{
    edln_clear_mark(&(wedln->edln));
}

/*EXTL_DOC
 * Copy text between \emph{mark} and current cursor position to clipboard
 * and then delete that sequence.
 */
EXTL_EXPORT_MEMBER
void wedln_cut(WEdln *wedln)
{
    edln_cut(&(wedln->edln));
}

/*EXTL_DOC
 * Copy text between \emph{mark} and current cursor position to clipboard.
 */
EXTL_EXPORT_MEMBER
void wedln_copy(WEdln *wedln)
{
    edln_copy(&(wedln->edln));
}

/*EXTL_DOC
 * Replace line editor contents with next entry in history if one exists.
 * If \var{match} is \code{true}, the initial part of the history entry
 * must match the current line from beginning to point.
 */
EXTL_EXPORT_MEMBER
void wedln_history_next(WEdln *wedln, bool match)
{
    edln_history_next(&(wedln->edln), match);
}

/*EXTL_DOC
 * Replace line editor contents with previous in history if one exists.
 * If \var{match} is \code{true}, the initial part of the history entry
 * must match the current line from beginning to point.
 */
EXTL_EXPORT_MEMBER
void wedln_history_prev(WEdln *wedln, bool match)
{
    edln_history_prev(&(wedln->edln), match);
}


/*EXTL_DOC
 * Input \var{str} in wedln at current editing point.
 */
EXTL_EXPORT_AS(WEdln, insstr)
void wedln_insstr_exported(WEdln *wedln, const char *str)
{
    edln_insstr(&(wedln->edln), str);
}


/*EXTL_DOC
 * Get line editor contents.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
const char *wedln_contents(WEdln *wedln)
{
    return wedln->edln.p;
}

/*EXTL_DOC
 * Get current editing point. 
 * Beginning of the edited line is point 0.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int wedln_point(WEdln *wedln)
{
    return wedln->edln.point;
}

/*EXTL_DOC
 * Get current mark (start of selection) for \var{wedln}.
 * Return value of -1 indicates that there is no mark, and
 * 0 is the beginning of the line.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
int wedln_mark(WEdln *wedln)
{
    return wedln->edln.mark;
}


/*EXTL_DOC
 * Set history context for \var{wedln}.
 */
EXTL_EXPORT_MEMBER
void wedln_set_context(WEdln *wedln, const char *context)
{
    edln_set_context(&(wedln->edln), context);
}


/*EXTL_DOC
 * Get history context for \var{wedln}.
 */
EXTL_SAFE
EXTL_EXPORT_MEMBER
const char *wedln_context(WEdln *wedln)
{
    return wedln->edln.context;
}

