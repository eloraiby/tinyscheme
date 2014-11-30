/*
  Schizo programming language
  Copyright (C) 2014  Wael El Oraiby

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU Affero General Public License as published
  by the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Affero General Public License for more details.

  You should have received a copy of the GNU Affero General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  Also add information on how to contact you by electronic and paper mail.
*/
/*
 grammar parser: using Lemon, not YACC nor BISON
*/
%name parser

%token_type	{ cell_id_t }
%extra_argument { state_t* s }

%include {
#include <stdio.h>
#include "schizo.h"

/* check if list c is an array : ...[] */
bool
is_list_an_index_expr(state_t* s,
                      cell_id_t c)
{
        if( cell_type(s, c) == CELL_PAIR ) {
                cell_t* h = cell_from_index(s, list_head(s, c));
                if( h->type == ATOM_SYMBOL && strcmp(h->object.symbol, "item") == 0 ) {
                        return true;
                } else {
                        return false;
                }
        } else {
                return true;
        }
}

}

%type cell	{ cell_id_t }
%type program	{ cell_id_t }
%type cell_list	{ cell_id_t }

/*
%left BIN_OP3
%left BIN_OP2
%left BIN_OP1
%left BIN_OP0

%left UN_OP3
%left UN_OP2
%left UN_OP1
%left UN_OP0
*/

%syntax_error {
        int n = sizeof(yyTokenName) / sizeof(yyTokenName[0]);
        for (int i = 0; i < n; ++i) {
                int a = yy_find_shift_action(yypParser, (YYCODETYPE)i);
                if (a < YYNSTATE + YYNRULE) {
                        printf("possible token: %s\n", yyTokenName[i]);
                }
        }
}

%start_symbol program

/* a program is a cell */
program		::= atom(B).				{ s->root = B; }
program		::= sexpr(B).				{ s->root = B; }

/* literals */
atom(A)		::= ATOM_SYMBOL(B).			{ A = B; }
atom(A)		::= ATOM_BOOL(B).			{ A = B; }
atom(A)		::= ATOM_CHAR(B).			{ A = B; }
atom(A)		::= ATOM_SINT64(B).			{ A = B; }
atom(A)		::= ATOM_REAL64(B).			{ A = B; }
atom(A)		::= ATOM_STRING(B).			{ A = B; }

/* NEVER USED: these are a hack to have the token ids */
sexpr		::= CELL_FREE.				/* a free cell */
sexpr		::= CELL_PAIR.				/* list */
sexpr		::= CELL_VECTOR.			/* a vector of cells */
sexpr		::= CELL_INDEX.				/* array accessor (index) */
sexpr		::= CELL_APPLY.				/* syntax/closure/foreign function interface */
sexpr		::= CELL_ENVIRONMENT.			/* environment */
sexpr		::= CELL_FRAME.				/* arguments */

/* ( ... ) */
sexpr(A)	::= LPAR members(B) RPAR.		{ A = B; }
sexpr(A)	::= LBR bexpr(B) RBR.			{ A = list_cons(s, atom_new_symbol(s, "begin"), list_reverse_in_place(s, B)); }
sexpr(A)	::= LBR bexpr(B) col RBR.		{ A = list_cons(s, atom_new_symbol(s, "begin"), list_reverse_in_place(s, B)); }

ilist(A)	::= atom(B).				{ A = B; }
ilist(A)	::= sexpr(B).				{ A = B; }
ilist(A)	::= index_expr(B).			{ A = B; }

list(A)		::= unop_expr(B).			{ A = B; }
list(A)		::= binop_expr(B).			{ A = B; }

list(A)		::= list(B) unop_expr(C).		{
                                                                /* check if list(B) is an array, in this case, create a new list regardless */
                                                                if( cell_type(s, B) == CELL_PAIR ) {
                                                                        if( is_list_an_index_expr(s, B)) {
                                                                                A = list_cons(s, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : C, list_new(s, B));
                                                                        } else {
                                                                                A = list_cons(s, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : C, B);
                                                                        }
                                                                } else {
                                                                        A = list_cons(s, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : C, list_new(s, B));
                                                                }
                                                        }


members(A)	::=.					{ cell_id_t nil = { 0 }; A = list_new(s, nil); }
members(A)	::= list(B).				{ A = ( cell_type(s, B) == CELL_PAIR && is_list_an_index_expr(s, B) == false) ? list_reverse_in_place(s, B) : B; }

/*
 * From here on, starts the brackets/array extension
 */

unop_expr(A)	::= ilist(B).				{ A = B; }
unop_expr(A)	::= ATOM_UNARY_OP(B) ilist(C).		{ /* This code is redundant */
                                                                /* check if list(B) is an array, in this case, create a new list regardless */
                                                                if( cell_type(s, B) == CELL_PAIR ) {
                                                                        if( is_list_an_index_expr(s, B)) {
                                                                                A = list_cons(s, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : C, list_new(s, B));
                                                                        } else {
                                                                                A = list_cons(s, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : C, B);
                                                                        }
                                                                } else {
                                                                        A = list_cons(s, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : C, list_new(s, B));
                                                                }
                                                        }

binop_expr(A)	::= unop_expr(B) ATOM_BINARY_OP(C) unop_expr(D).	{ A = list_cons(s, (cell_type(s, D) == CELL_PAIR) ? list_reverse_in_place(s, D) : D,
                                                                                        list_cons(s,
                                                                                                (cell_type(s, B) == CELL_PAIR) ? list_reverse_in_place(s, B) : B,
                                                                                                list_new(s, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : C))); }



col		::= COL.
col		::= col COL.

bexpr(A)	::= members(B).				{ A = list_new(s, B); }
bexpr(A)	::= bexpr(B) col list(C).		{ A = list_cons(s, (cell_type(s, C) == CELL_PAIR && is_list_an_index_expr(s, C) == false) ? list_reverse_in_place(s, C) : C, B); }
bexpr		::= error.				{ fprintf(stderr, "Error: unexpected token: %d\n", yyruleno); }

arr_operand(A)	::= ATOM_SYMBOL(B).			{ A = B; }
arr_operand(A)	::= ATOM_STRING(B).			{ A = B; }
arr_operand(A)	::= sexpr(B).				{ A = B; }

index_expr(A)	::= arr_operand(B) LSQB list(C) RSQB.	{ A = list_cons(s, atom_new_symbol(s, "item"), list_cons(s, B, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : list_new(s, C))); }
index_expr(A)	::= index_expr(B) LSQB list(C) RSQB.	{ A = list_cons(s, atom_new_symbol(s, "item"), list_cons(s, B, (cell_type(s, C) == CELL_PAIR) ? list_reverse_in_place(s, C) : list_new(s, C))); }
