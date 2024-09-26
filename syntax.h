#ifndef SYNTAX_H
#define SYNTAX_H

/* Operators */
#define NOT_STR "-"
#define AND_STR "^"
#define OR_STR "/"
#define IMPL_STR "=>"
#define CON_STR "_|_"

/* Rules */
#define NOT_INTR_STR NOT_STR "i"
#define NOT_ELIM_STR NOT_STR "e"
#define AND_INTR_STR AND_STR "i"
#define AND_ELIM_1_STR AND_STR "e1"
#define AND_ELIM_2_STR AND_STR "e2"
#define OR_INTR_1_STR OR_STR "i1"
#define OR_INTR_2_STR OR_STR "i2"
#define OR_ELIM_STR OR_STR "e"
#define IMPL_INTR_STR IMPL_STR "i"
#define IMPL_ELIM_STR IMPL_STR "e"
#define CON_ELIM_STR CON_STR "e"
#define NOT_NOT_INTR_STR NOT_STR NOT_STR "i"
#define NOT_NOT_ELIM_STR NOT_STR NOT_STR "e"
#define MT_STR "MT"
#define PBC_STR "PBC"
#define LEM_STR "LEM"
#define COPY_STR "copy"

#endif
