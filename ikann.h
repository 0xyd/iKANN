#ifndef IKANN_H
#define IKANN_H

#define KAD_VERSION "r544"

#include <stdio.h>
#include <stdint.h>

#ifdef __STRICT_ANSI__
#define inline
#endif

#define KAD_MAX_DIM 4     /* max dimension */
#define KAD_MAX_OP  64    /* max number of operators */

/* A computational graph is a directed acyclic graph. In the graph, an external
 * node represents a variable, a constant or a feed; an internal node
 * represents an operator; an edge from node v to w indicates v is an operand
 * of w.
 */

#define KAD_VAR        0x1
#define KAD_CONST      0x2
#define KAD_POOL       0x4
#define KAD_SHARE_RNG  0x10 /* with this flag on, different time step shares the same RNG status after unroll */

#define kad_is_back(p)  ((p)->flag & KAD_VAR)
#define kad_is_ext(p)   ((p)->n_child == 0)
#define kad_is_var(p)   (kad_is_ext(p) && kad_is_back(p))
#define kad_is_const(p) (kad_is_ext(p) && ((p)->flag & KAD_CONST))
#define kad_is_feed(p)  (kad_is_ext(p) && !kad_is_back(p) && !((p)->flag & KAD_CONST))
#define kad_is_pivot(p) ((p)->n_child == 1 && ((p)->flag & KAD_POOL))
#define kad_is_switch(p) ((p)->op == 12 && !((p)->flag & KAD_POOL))
#define kad_use_rng(p)  ((p)->op == 15 || (p)->op == 24)

#define kad_eval_enable(p) ((p)->tmp = 1)
#define kad_eval_disable(p) ((p)->tmp = -1)

/* a node in the computational graph */
typedef struct kad_node_t {
	uint8_t     n_d;            /* number of dimensions; no larger than KAD_MAX_DIM */
	uint8_t     flag;           /* type of the node; see KAD_F_* for valid flags */
	uint16_t    op;             /* operator; kad_op_list[op] is the actual function */
	int32_t     n_child;        /* number of operands/child nodes */
	int32_t     tmp;            /* temporary field; MUST BE zero before calling kad_compile() */
	int32_t     ptr_size;       /* size of ptr below */
	int32_t     d[KAD_MAX_DIM]; /* dimensions */
	int32_t     ext_label;      /* labels for external uses (not modified by the kad_* APIs) */
	uint32_t    ext_flag;       /* flags for external uses (not modified by the kad_* APIs) */
	float      *x;              /* value; allocated for internal nodes */
	float      *g;              /* gradient; allocated for internal nodes */
	void       *ptr;            /* for special operators that need additional parameters (e.g. conv2d) */
	void       *gtmp;           /* temporary data generated at the forward pass but used at the backward pass */
	struct kad_node_t **child;  /* operands/child nodes */
	struct kad_node_t  *pre;    /* usually NULL; only used for RNN */
} kad_node_t, *kad_node_p;

#ifdef __cplusplus
extern "C" {
#endif

kad_node_t *kann_new_leaf(uint8_t flag, float x0_01, int n_d, ...); /* flag can be KAD_CONST or KAD_VAR */
kad_node_t *kann_new_leaf2(int *offset, kad_node_p *par, uint8_t flag, float x0_01, int n_d, ...);
kad_node_t *kann_layer_input(int n1);
kad_node_t *kann_layer_dense(kad_node_t *in, int n1);
kad_node_t *kann_layer_dense2(int *offset, kad_node_p *par, kad_node_t *in, int n1);
kad_node_t *kad_feed(int n_d, ...);
kad_node_t *kad_add(kad_node_t *x, kad_node_t *y); /* f(x,y) = x + y (generalized element-wise addition; f[i*n+j]=x[i*n+j]+y[j], n=kad_len(y), 0<j<n, 0<i<kad_len(x)/n) */
kad_node_t *kad_cmul(kad_node_t *x, kad_node_t *y);