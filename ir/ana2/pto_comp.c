/* -*- c -*- */

/*
   Project:     libFIRM
   File name:   ir/ana/pto_comp.c
   Purpose:     Main Implementation of PTO
   Author:      Florian
   Modified by:
   Created:     Sat Nov 13 19:35:27 CET 2004
   CVS-ID:      $Id$
   Copyright:   (c) 1999-2004 Universitšt Karlsruhe
   Licence:     This file is protected by the GPL -  GNU GENERAL PUBLIC LICENSE.
*/

# ifdef HAVE_CONFIG_H
#  include <config.h>
# endif

/*
  pto_comp: Main Implementation of PTO
*/

# include <string.h>            /* for memset */

# include "pto_comp.h"
# include "pto_util.h"
# include "pto_name.h"
# include "pto_ctx.h"

# include "irnode.h"
# include "irprog.h"
# include "xmalloc.h"
# include "irmemwalk.h"

# include "pto_debug.h"
# include "pto_init.h"

# include "ecg.h"

/* Local Defines: */

/* Local Data Types: */
typedef struct pto_env_str {
  int ctx_idx;
} pto_env_t;


/* Local Variables: */

/* Debug only: */
char *spaces = NULL;

/* Local Prototypes: */
static void pto_call (ir_graph*, ir_node*, pto_env_t*);
static pto_t *get_pto (ir_node*);

/* ===================================================
   Local Implementation:
   =================================================== */
/* Transfer the actual arguments to the formal arguments */
static void set_graph_args (ir_graph *graph, ir_node *call)
{
  assert (iro_Call == get_irn_opcode (call));

  type *meth = get_entity_type (get_irg_entity (graph));
  ir_node **args = find_irg_args (graph);
  int n_args = get_Call_n_params (call);
  int i;

  for (i = 0; i < n_args; i ++) {
    if (NULL != args [i]) {
      if (mode_P == get_type_mode (get_method_param_type (meth, i))) {
        ir_node *call_arg = get_Call_param (call, i);
        pto_t *pto = get_pto (call_arg);
        assert (pto);
        set_node_pto (args [i], pto);

        DBGPRINT (0, (stdout, "%s: arg [%i]: %s[%li] -> %s[%li] (%p)\n",
                      __FUNCTION__,
                      i,
                      OPNAME (call_arg), OPNUM (call_arg),
                      OPNAME (args [i]), OPNUM (args [i]),
                      (void*) pto));
      }
    }
  }
}

/* Transfer the graph's result to the call */
static void set_graph_result (ir_graph *graph, ir_node *call)
{
  type *tp = get_entity_type (get_irg_entity (graph));

  if (0 == get_method_n_ress (tp)) {
    return;
  }

  tp = get_method_res_type (tp, 0);

  if (mode_P != get_type_mode (tp)) {
    set_node_pto (call, NULL);
    return;
  }

  ir_node *end_block = get_irg_end_block (graph);
  pto_t *ret_pto = get_node_pto (end_block);

  pto_t *call_pto = get_node_pto (call);

  assert (call_pto);

  qset_insert_all (call_pto->values, ret_pto->values);
}

/* Propagation of PTO values */
static pto_t *get_pto_proj (ir_node *proj)
{
  ir_node *proj_in = get_Proj_pred (proj);
  const long proj_proj = get_Proj_proj (proj);
  const opcode in_op = get_irn_opcode (proj_in);
  pto_t *in_pto = NULL;
  pto_t *proj_pto = get_node_pto (proj);

  ir_node *proj_in_in = NULL;

  switch (in_op) {
  case (iro_Start):             /* ProjT (Start) */
    assert (0 && "pto from ProjT(Start) requested");

    return (NULL);
  case (iro_Proj):              /* ProjT (Start), ProjT (Call) */
    proj_in_in = get_Proj_pred (proj_in);
    const opcode in_in_op = get_irn_opcode (proj_in_in);
    const long proj_in_proj = get_Proj_proj (proj_in);

    assert ((pn_Start_T_args == proj_in_proj) ||
            (pn_Call_T_result == proj_in_proj));

    switch (in_in_op) {
    case (iro_Start):           /* ProjArg (ProjT (Start)) */
      /* then the pto value must have been set to the node */
      assert (proj_pto);

      return (proj_pto);
    case (iro_Call):            /* ProjV (ProjT (Call)) */
      if (NULL != proj_pto) {
        return (proj_pto);
      } else {
        in_pto = get_pto (proj_in);
        set_node_pto (proj, in_pto);

        assert (in_pto);

        return (in_pto);
      }
    default: assert (0 && "Proj(Proj(?))");
    }
    /* done with case (in_op == iro_Proj) */

  case (iro_Load):              /* ProjV (Load) */
    assert (pn_Load_res == proj_proj);
    /* FALLTHROUGH */
  case (iro_Call):              /* ProjT (Call) */
    /* FALLTHROUGH */
  case (iro_Alloc):             /* ProjV (Alloc) */
    if (NULL != proj_pto) {
      return (proj_pto);
    } else {
      in_pto = get_pto (proj_in);
      assert (in_pto);

      set_node_pto (proj, in_pto);
      return (in_pto);
    }
  default:
    fprintf (stderr, "%s: not handled: proj (%s[%li])\n",
             __FUNCTION__,
             get_op_name (get_irn_op (proj_in)),
             get_irn_node_nr (proj_in));
    assert (0 && "Proj(?)");
  }

}

static pto_t *get_pto_phi (ir_node *phi)
{
  assert (mode_P == get_irn_mode (phi));

  pto_t *pto = get_node_pto (phi);

  assert (pto);                 /* must be initialised */

  int n_ins = get_irn_arity (phi);
  int i;

  for (i = 0; i < n_ins; i ++) {
    ir_node *in = get_irn_n (phi, i);
    pto_t *in_pto = get_pto (in);

    assert (in_pto);

    qset_insert_all (pto->values, in_pto->values);
  }

  return (pto);
}

static pto_t *get_pto_sel (ir_node *sel)
{
  pto_t *pto = get_node_pto (sel);

  if (NULL == pto) {
    ir_node *in = get_Sel_ptr (sel);

    pto = get_pto (in);
    set_node_pto (sel, pto);
  }

  return (pto);
}

static pto_t *get_pto_ret (ir_node *ret)
{
  pto_t *pto = get_node_pto (ret);

  if (NULL == pto) {
    ir_node *in = get_Return_res (ret, 0);

    pto = get_pto (in);
    set_node_pto (ret, pto);
  }

  assert (pto);

  return (pto);
}


/* Dispatch to propagate PTO values */
static pto_t *get_pto (ir_node *node)
{
  const opcode op = get_irn_opcode (node);

  switch (op) {
  case (iro_Cast):   return (get_pto (get_Cast_op (node)));
  case (iro_Proj):   return (get_pto_proj (node));
  case (iro_Phi):    return (get_pto_phi (node));
  case (iro_Sel):    return (get_pto_sel (node));
  case (iro_Alloc):  return (get_alloc_pto (node));
  case (iro_Return): return (get_pto_ret (node));

  case (iro_Call):              /* FALLTHROUGH */
  case (iro_Load):              /* FALLTHROUGH */
  case (iro_Const):             /* FALLTHROUGH */
  case (iro_SymConst): return (get_node_pto (node));

  default:
    /* stopgap measure */
    fprintf (stderr, "%s: not handled: node[%li].op = %s\n",
             __FUNCTION__,
             get_irn_node_nr (node),
             get_op_name (get_irn_op (node)));
    assert (0 && "something not handled");

  }
}


/* Actions for the nodes: */
static void pto_load (ir_node *load, pto_env_t *pto_env)
{
  /* perform load */
  DBGPRINT (1, (stdout, "%s (%s[%li]): pto = %p\n", __FUNCTION__,
                OPNAME (load), OPNUM (load), (void*) get_node_pto (load)));

  ir_node *ptr = get_Load_ptr (load);

  if (is_dummy_load_ptr (ptr)) {
    return;
  }

  entity *ent = get_ptr_ent (ptr);

  pto_t *ptr_pto = get_pto (ptr);

  assert (ptr_pto);

  DBGPRINT (0, (stdout, "%s (%s[%li]): ptr = %p\n", __FUNCTION__,
                OPNAME (ptr), OPNUM (ptr), (void*) ptr_pto));
}

static void pto_store (ir_node *store, pto_env_t *pto_env)
{
  /* perform store */
  DBGPRINT (1, (stdout, "%s (%s[%li]) (no pto)\n", __FUNCTION__,
                OPNAME (store), OPNUM (store)));

  ir_node *ptr = get_Store_ptr (store);
  ir_node *val = get_Store_value (store);

  if (mode_P != get_irn_mode (val)) {
    return;
  }

  entity *ent = get_ptr_ent (ptr);

  pto_t *ptr_pto = get_pto (ptr);
  pto_t *val_pto = get_pto (val);

  assert (ptr_pto);
  assert (val_pto);

  DBGPRINT (0, (stdout, "%s (%s[%li]): ptr_pto = %p\n", __FUNCTION__,
                OPNAME (store), OPNUM (store), (void*) ptr_pto));
  DBGPRINT (0, (stdout, "%s (%s[%li]): val_pto = %p\n", __FUNCTION__,
                OPNAME (store), OPNUM (store), (void*) val_pto));
}

static void pto_method (ir_node *call, pto_env_t *pto_env)
{
  DBGPRINT (1, (stdout, "%s:%i (%s[%li]): pto = %p\n",
                __FUNCTION__, __LINE__, OPNAME (call), OPNUM (call),
                (void*) get_node_pto (call)));

  callEd_info_t *callEd_info = ecg_get_callEd_info (call);

  if (NULL == callEd_info) {
    DBGPRINT (1, (stdout, "%s:%i (%s[%li]), no graph\n",
                  __FUNCTION__, __LINE__, OPNAME (call), OPNUM (call)));
  }

  int i = 0;
  while (NULL != callEd_info) {
    DBGPRINT (1, (stdout, "%s:%i (%s[%li]), graph %i\n",
                  __FUNCTION__, __LINE__, OPNAME (call), OPNUM (call), i ++));

    pto_call (callEd_info->callEd, call, pto_env);

    callEd_info = callEd_info->prev;
  }
}


/* Continue PTO for one of the graphs called at a Call */
static void pto_call (ir_graph *graph, ir_node *call, pto_env_t *pto_env)
{
  /* perform call */
  DBGPRINT (0, (stdout, "%s:%i (%s[%li])\n",
                __FUNCTION__, __LINE__, OPNAME (call), OPNUM (call)));

  /* only for debugging stuff: */
  entity *ent = get_irg_entity (graph);
  const char *ent_name = (char*) get_entity_name (ent);
  const char *own_name = (char*) get_type_name (get_entity_owner (ent));

  if (! get_irg_is_mem_visited (graph)) {
    graph_info_t *ginfo = ecg_get_info (graph);

    /* Save CTX */
    int ctx_idx = find_ctx_idx (call, ginfo, get_curr_ctx ());
    ctx_info_t *call_ctx = get_ctx (ginfo, ctx_idx);
    ctx_info_t *old_ctx = set_curr_ctx (call_ctx);
    DBGPRINT (2, (stdout, "%s>CTX: ", -- spaces));
    DBGEXE (2, ecg_print_ctx (call_ctx, stdout));

    /* Todo: Compute Arguments */
    set_graph_args (graph, call);

    /* Initialise Alloc Names and Node values (nope, done in pto_graph ()) */
    /* pto_reset_graph_pto (graph, ctx_idx); */

    /* Visit/Iterate Graph */
    pto_graph (graph, ctx_idx);

    /* Restore CTX */
    set_curr_ctx (old_ctx);

    set_graph_result (graph, call);

    DBGPRINT (2, (stdout, "%s<CTX: ", spaces ++));
    DBGEXE (2, ecg_print_ctx (call_ctx, stdout));

    /* Don't need to reset alloc names unless we handle recursion here  */


    /* Get Return Value from Graph */
  } else {
    DBGPRINT (1, (stdout, "%s: recursion into \"%s.%s\"\n",
                  __FUNCTION__, own_name, ent_name));
  }

  /* Todo: Set 'Unknown' Value as Return Value when the graph is not
     known */
}

static void pto_raise (ir_node *raise, pto_env_t *pto_env)
{
  /* perform raise */
  DBGPRINT (1, (stdout, "%s (%s[%li]): pto = %p\n", __FUNCTION__,
                OPNAME (raise), OPNUM (raise), (void*) get_node_pto (raise)));
}

static void pto_end_block (ir_node *end_block, pto_env_t *pto_env)
{
  /* perform end block */
  type *tp = get_entity_type (get_irg_entity (get_irn_irg (end_block)));

  if (0 == get_method_n_ress (tp)) {
    return;
  }

  tp = get_method_res_type (tp, 0);

  if (mode_P != get_type_mode (tp)) {
    return;
  }

  DBGPRINT (1, (stdout, "%s (%s[%li]): pto = %p\n", __FUNCTION__,
                OPNAME (end_block), OPNUM (end_block),
                (void*) get_node_pto (end_block)));

  pto_t *end_pto = get_node_pto (end_block);

  assert (end_pto);

  int n_ins = get_irn_arity (end_block);
  int i;
  for (i = 0; i < n_ins; i ++) {
    ir_node *in = get_irn_n (end_block, i);

    if (iro_Return == get_irn_opcode (in)) {
      pto_t *in_pto = get_pto (in);

      qset_insert_all (end_pto->values, in_pto->values);
    }
  }
}

/* Perform the appropriate action on the given node */
static void pto_node_node (ir_node *node, pto_env_t *pto_env)
{
  DBGPRINT (0, (stdout, "%s (%s[%li])\n", __FUNCTION__,
                OPNAME (node), OPNUM (node)));

  const opcode op = get_irn_opcode (node);

  switch (op) {
  case (iro_Start): /* nothing */ break;
  case (iro_Load):
    pto_load (node, pto_env);
    break;

  case (iro_Store):
    pto_store (node, pto_env);
    break;

  case (iro_Call):
    pto_method (node, pto_env);
    break;

  case (iro_Raise):
    pto_raise (node, pto_env);
    break;

  case (iro_Return):
    /* nothing to do */
    break;

  case (iro_Alloc):
    /* nothing to do */
    break;

  case (iro_Block):
    pto_end_block (node, pto_env);
    break;

  case (iro_Phi):
    /* must be a PhiM */
    assert (mode_M == get_irn_mode (node));
    /* nothing to do */
    break;

    /* uninteresting stuff: */
  case (iro_Div):
  case (iro_Quot):
  case (iro_Mod):
  case (iro_DivMod): /* nothing to do */ break;

  default:
    /* stopgap measure */
    fprintf (stderr, "%s: not handled: node[%li].op = %s\n",
             __FUNCTION__,
             get_irn_node_nr (node),
             get_op_name (get_irn_op (node)));
    assert (0 && "something not handled");
  }



}


/* Callback function to execute in pre-order */
static void pto_node_pre (ir_node *node, void *env)
{
  /* nothing */
}

/* Callback function to execute in post-order */
static void pto_node_post (ir_node *node, void *env)
{
  pto_env_t *pto_env = (pto_env_t*) env;

  pto_node_node (node, pto_env);
}

/* Perform a single pass over the given graph */
static void pto_graph_pass (ir_graph *graph, void *pto_env)
{
  entity *ent = get_irg_entity (graph);
  const char *ent_name = (char*) get_entity_name (ent);
  const char *own_name = (char*) get_type_name (get_entity_owner (ent));
  HERE3 ("start", own_name, ent_name);

  irg_walk_mem (graph, pto_node_pre, pto_node_post, pto_env);

  HERE3 ("end  ", own_name, ent_name);
}

/* ===================================================
   Exported Implementation:
   =================================================== */
/* Main loop: Initialise and iterate over the given graph */
void pto_graph (ir_graph *graph, int ctx_idx)
{
  pto_env_t *pto_env = xmalloc (sizeof (pto_env_t));
  pto_env->ctx_idx = ctx_idx;

  /* HERE ("start"); */

  DBGPRINT (1, (stdout, "%s: start for ctx %i\n",
                __FUNCTION__,
                ctx_idx));

  /* todo: pto_reset_graph_pto */
  pto_reset_graph_pto (graph, ctx_idx);

  /* todo (here): iterate, obey 'changed' attribute */
  pto_graph_pass (graph, pto_env);

  memset (pto_env, 0x00, sizeof (pto_env_t));
  free (pto_env);
  HERE ("end");
}

/* Set the PTO value for the given non-alloc node */
void set_node_pto (ir_node *node, pto_t *pto)
{
  assert (iro_Alloc != get_irn_opcode (node));

  set_irn_link (node, (void*) pto);
}

/*Get the PTO value for the given non-alloc node */
pto_t *get_node_pto (ir_node *node)
{
  assert (iro_Alloc != get_irn_opcode (node));

  return ((pto_t*) get_irn_link (node));
}

/* Set the PTO value for the given alloc node */
void set_alloc_pto (ir_node *alloc, alloc_pto_t *alloc_pto)
{
  assert (iro_Alloc == get_irn_opcode (alloc));

  set_irn_link (alloc, (void*) alloc_pto);
}

/*Get the current PTO value for the given alloc node */
pto_t *get_alloc_pto (ir_node *alloc)
{
  alloc_pto_t *alloc_pto = (alloc_pto_t*) get_irn_link (alloc);

  assert (iro_Alloc == get_irn_opcode (alloc));

  return (alloc_pto -> curr_pto);
}


/*
  $Log$
  Revision 1.4  2004/11/26 15:59:40  liekweg
  verify pto_{load,store}

  Revision 1.3  2004/11/24 14:53:55  liekweg
  Bugfixes

  Revision 1.2  2004/11/20 21:21:56  liekweg
  Finalise initialisation

  Revision 1.1  2004/11/18 16:37:34  liekweg
  rewritten


*/
