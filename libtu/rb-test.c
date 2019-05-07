/*
Generic C code for red-black trees.
Copyright (C) 2000 James S. Plank

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

#include "rb.h"
#include <stdio.h>

/* an example -- this allocates a red-black tree for integers.  For a
 * user-specified number of iterations, it does the following:

 * delete a random element in the tree.
 * make two new random elements, and insert them into the tree

 * At the end, it prints the sorted list of elements, and then prints
 * stats of the number of black nodes in any path in the tree, and
 * the minimum and maximum path lengths.

 * Rb_find_ikey and rb_inserti could have been used, but instead
 * rb_find_gkey and rb_insertg were used to show how they work.

 */

int icomp(char *i, char *j)
{
  int I, J;

  I = (int) i;
  J = (int) j;
  if (I == J) return 0;
  if (I > J) return -1; else return 1;
}

main(int argc, char **argv)
{
  int i, j, tb, nb, mxp, mnp, p;
  int iterations;
  Rb_node argt, t;
  int *a;

  if (argc != 2) {
    fprintf(stderr, "usage: main #iterations\n");
    exit(1);
  }
  argt = make_rb();
  srandom(time(0));
  iterations = atoi(argv[1]);
  a = (int *) malloc (iterations*sizeof(int));

  for (i = 0; i < atoi(argv[1]); i++) {
    if (i > 0) {
      j = random()%i;

      rb_delete_node(rb_find_gkey(argt, (char *) (a[j]), icomp));
      a[j] = random() % 1000;
      rb_insertg(argt, (char *) (a[j]), NULL, icomp);
    }
    a[i] = random() % 1000;
    rb_insertg(argt, (char *) (a[i]), NULL, icomp);
  }
  tb = 0;
  mxp = 0;
  mnp = 0;
  rb_traverse(t, argt) {
    printf("%d ", t->k.ikey);
    nb = rb_nblack(t);
    p = rb_plength(t);
    if (tb == 0) {
      tb = nb;
    } else if (tb != nb) {
      printf("Conflict: tb=%d, nb=%d\n", tb, nb);
      exit(1);
    }
    mxp = (mxp == 0 || mxp < p) ? p : mxp;
    mnp = (mnp == 0 || mxp > p) ? p : mnp;
  }
  printf("\n");

  printf("Nblack = %d\n", tb);
  printf("Max    = %d\n", mxp);
  printf("Min    = %d\n", mnp);
}
