/*
Generic C code for red-black trees.
Copyright (C) 2000 James S. Plank,
              2004 Tuomo Valkonen.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* Revision 1.2.  Jim Plank */

/* Original code by Jim Plank (plank@cs.utk.edu) */
/* modified for THINK C 6.0 for Macintosh by Chris Bartley */

#ifndef LIBTU_RB_H
#define LIBTU_RB_H

typedef struct rb_status {
  unsigned red : 1 ;
  unsigned internal : 1 ;
  unsigned left : 1 ;
  unsigned root : 1 ;
  unsigned head : 1 ;
} Rb_status;
 
/* Main rb_node.  You only ever use the fields

   c.list.flink
   c.list.blink

   k.key or k.ikey
   v.val
*/


typedef struct rb_node {
  union {
    struct {
      struct rb_node *flink;
      struct rb_node *blink;
    } list;
    struct {
      struct rb_node *left;
      struct rb_node *right;
    } child;
  } c;
  union {
    struct rb_node *parent;
    struct rb_node *root;
  } p;
  Rb_status s;
  union {
    int ikey;
    const void *key;
    struct rb_node *lext;
  } k;
  union {
    int ival;
    void *val;
    struct rb_node *rext;
  } v;
} *Rb_node;


typedef int Rb_compfn(const void *, const void *);


extern Rb_node make_rb();   /* Creates a new rb-tree */

/* Creates a node with key key and val val and inserts it into the tree.
   rb_insert uses strcmp() as comparison funcion.  rb_inserti uses <>=,
   rb_insertg uses func() */

extern Rb_node rb_insert(Rb_node tree, const char *key, void *val);
extern Rb_node rb_inserti(Rb_node tree, int ikey, void *val);
extern Rb_node rb_insertp(Rb_node tree, const void *key, void *val);
extern Rb_node rb_insertg(Rb_node tree, const void *key, void *val, 
                          Rb_compfn *func);


/* returns an external node in t whose value is equal
  k or whose value is the smallest value greater than k. */

extern Rb_node rb_find_key(Rb_node root, const char *key);
extern Rb_node rb_find_ikey(Rb_node root, int ikey);
extern Rb_node rb_find_pkey(Rb_node root, const void *key);
extern Rb_node rb_find_gkey(Rb_node root, const void *key, Rb_compfn *func);


/* Works just like the find_key versions only it returns whether or not
   it found the key in the integer variable found */

extern Rb_node rb_find_key_n(Rb_node root, const char *key, int *found);
extern Rb_node rb_find_ikey_n(Rb_node root, int ikey, int *found);
extern Rb_node rb_find_pkey_n(Rb_node root, const void *key, int *found);
extern Rb_node rb_find_gkey_n(Rb_node root, const void *key, 
                              Rb_compfn *func, int *found);


/* Creates a node with key key and val val and inserts it into the 
   tree before/after node nd.  Does not check to ensure that you are 
   keeping the correct order */

extern Rb_node rb_insert_b(Rb_node nd, const void *key, void *val);
extern Rb_node rb_insert_a(Rb_node nd, const void *key, void *val);


extern void rb_delete_node(Rb_node node);  /* Deletes and frees a node (but 
                                              not the key or val) */
extern void rb_free_tree(Rb_node root);  /* Deletes and frees an entire tree */

extern void *rb_val(Rb_node node);  /* Returns node->v.val -- this is to shut
                                       lint up */

extern int rb_nblack(Rb_node n); /* returns # of black nodes in path from
                                    n to the root */
int rb_plength(Rb_node n);       /* returns the # of nodes in path from
				    n to the root */
 
#define rb_first(n) (n->c.list.flink)
#define rb_last(n) (n->c.list.blink)
#define rb_next(n) (n->c.list.flink)
#define rb_prev(n) (n->c.list.blink)
#define rb_empty(t) (t->c.list.flink == t)
#ifndef rb_nil
#define rb_nil(t) (t)
#endif
 
#define rb_traverse(ptr, lst) \
  for(ptr = rb_first(lst); ptr != rb_nil(lst); ptr = rb_next(ptr))
 
#endif /* LIBTU_RB_H */
