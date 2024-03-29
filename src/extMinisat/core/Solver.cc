/***************************************************************************************[Solver.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/

#include <math.h>
#include <iostream>
#include <stdexcept>

#include "extMinisat/mtl/Sort.h"
#include "extMinisat/core/Solver.h"
#include "extMinisat/core/ProofVisitor.h"

#include <set>
using namespace Minisat;

//=================================================================================================
// Options:


static const char* _cat = "CORE";

static DoubleOption  opt_var_decay         (_cat, "var-decay",   "The variable activity decay factor",            0.95,     DoubleRange(0, false, 1, false));
static DoubleOption  opt_clause_decay      (_cat, "cla-decay",   "The clause activity decay factor",              0.999,    DoubleRange(0, false, 1, false));
static DoubleOption  opt_random_var_freq   (_cat, "rnd-freq",    "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption  opt_random_seed       (_cat, "rnd-seed",    "Used by the random variable selection",         91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption     opt_ccmin_mode        (_cat, "ccmin-mode",  "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption     opt_phase_saving      (_cat, "phase-saving", "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption    opt_rnd_init_act      (_cat, "rnd-init",    "Randomize the initial activity", false);
static BoolOption    opt_luby_restart      (_cat, "luby",        "Use the Luby restart sequence", true);
static IntOption     opt_restart_first     (_cat, "rfirst",      "The base restart interval", 100, IntRange(1, INT32_MAX));
static DoubleOption  opt_restart_inc       (_cat, "rinc",        "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption  opt_garbage_frac      (_cat, "gc-frac",     "The fraction of wasted memory allowed before a garbage collection is triggered",  0.2 , DoubleRange(0, false, HUGE_VAL, false));

static BoolOption    opt_valid             (_cat, "valid",    "Validate UNSAT answers", true);


//=================================================================================================
// Constructor/Destructor:


Solver::Solver() :

    // Parameters (user settable):
    //
    verbosity        (0)
  , log_proof (opt_valid)
  , ordered_propagate (false)
  , var_decay        (opt_var_decay)
  , clause_decay     (opt_clause_decay)
  , random_var_freq  (opt_random_var_freq)
  , random_seed      (opt_random_seed)
  , luby_restart     (opt_luby_restart)
  , ccmin_mode       (opt_ccmin_mode)
  , phase_saving     (opt_phase_saving)
  , rnd_pol          (false)
  , rnd_init_act     (opt_rnd_init_act)
  , garbage_frac     (opt_garbage_frac)
  , restart_first    (opt_restart_first)
  , restart_inc      (opt_restart_inc)

    // Parameters (the rest):
    //
  , learntsize_factor((double)1/(double)3), learntsize_inc(1.1)

    // Parameters (experimental):
    //
  , learntsize_adjust_start_confl (100)
  , learntsize_adjust_inc         (1.5)

    // Statistics: (formerly in 'SolverStats')
    //
  , solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0), conflicts(0)
  , dec_vars(0), clauses_literals(0), learnts_literals(0), max_literals(0), tot_literals(0)

  , ok                 (true)
  , cla_inc            (1)
  , var_inc            (1)
  , watches            (WatcherDeleted(ca))
  , qhead              (0)
  , simpDB_assigns     (-1)
  , simpDB_props       (0)
  , order_heap         (VarOrderLt(activity))
  , progress_estimate  (0)
  , remove_satisfied   (true)

    // Resource constraints:
    //
  , conflict_budget    (-1)
  , propagation_budget (-1)
  , asynch_interrupt   (false)
  , currentPart (1)
  , start(0)
  , totalPart(1)
  , reorder_proof(false)
  , confl_assumps(CRef_Undef)
  , core_units(false)
{}


Solver::~Solver()
{
}


// === Validation
namespace
{
  struct scopped_ordered_propagate
  {
    Solver &m;
    bool m_mode;
    scopped_ordered_propagate (Solver &s, bool v) : m(s)
    {
      m_mode = m.orderedPropagate ();
      m.orderedPropagate (v);
    }
    ~scopped_ordered_propagate () { m.orderedPropagate (m_mode); }
  };
}

void Solver::runProof()
{
	ProofVisitor v;

	for (int iter=0; iter < 1; iter++)
	{
		bool res = validate();
		if (res == false)
		{
			printf("Validation fails after %d times\n", iter);
			assert(false);
		}

		vec<CRef> oldProof;
		replay(v, &oldProof);

		proof.push(oldProof.last());
	}

	start = 0;
}

bool Solver::validate ()
{
  assert (log_proof);
  assert (!ok || confl_assumps != CRef_Undef);
  assert (proof.size () > 0);

  // If DB was solved under assumptions and multiple assumptions
  // are the reason for unsatisfiability, then add them as a new
  // learn clause, and mark this situation by
  //
  if (confl_assumps != CRef_Undef) {
      Clause& c = ca[confl_assumps];
      if (c.size() > 1) {
          attachClause(confl_assumps);
          learnts.push(confl_assumps);
          proof.push(0);
      }
  }

  scopped_ordered_propagate scp_propagate (*this, true);

  // -- final conflict clause is in the core
  // If the last clause is 0, or the final conflict under assumptions
  // contains only one literal, mark all reasons.
  if (proof.last () != 0 ||
      (confl_assumps != CRef_Undef && ca[confl_assumps].size() == 1)) {
      Clause &last = (proof.last() == 0) ? ca[confl_assumps] : ca [proof.last ()];
      last.core (1);
      // -- mark all reasons for the final conflict as core
      for (int i = 0; i < last.size (); i++)
        {
            // -- validate that the clause is really a conflict clause
            //if (value (last [i]) != l_False) return false;
            Var x = var (last [i]);
            if (reason(x) != CRef_Undef) ca [reason (x)].core (1);
        }
  }
  else
      ca[proof[proof.size()-2]].core(1);

  cancelUntil(0);
  int trail_sz = trail.size ();
  ok = true;

  // -- move back through the proof, shrinking the trail and
  // -- validating the clauses
  for (int i = proof.size () - 2; i >= 0; i--)
    {
      if (verbosity >= 2) fflush (stdout);
      CRef cr = proof [i];
      assert (cr != CRef_Undef);
      Clause &c = ca [cr];

      //if (verbosity >= 2) printf ("Validating lemma #%d ... ", i);

      // -- resurect deleted clauses
      if (c.mark () == 1)
        {
          watches.cleanAll();
          // -- undelete
          c.mark (0);
          Var x = var (c[0]);

          // If satisfied, check that undef lits are at c[1]
          if (satisfied(c) && c.size() > 1)
          {
            for (int k=1; k < c.size() && value(c[1]) != l_Undef; k++)
                if (value(c[k]) == l_Undef){
                    Lit l = c[1];
                    c[1] = c[k], c[k] = l;
                }
          }
          // -- if non-unit clause, attach it
          if (c.size () > 1) attachClause (cr);
          else // -- if unit clause, enqueue it
            {
              bool res = enqueue (c[0], cr);
              assert (res);
            }
          if (verbosity >= 2) printf ("^");
          continue;
        }
      assert (c.mark () == 0);
      // -- detach the clause
      if (locked (c))
        {
          if (core_units) c.core(1);
          // -- undo the bcp
          while (trail[trail_sz - 1] != c[0])
            {
              assert(trail_sz > 0);
              Var x = var (trail [trail_sz - 1]);
              assigns [x] = l_Undef;
              insertVarOrder (x);
              trail_sz--;

              CRef r = reason (x);
              assert (r != CRef_Undef);
              // -- mark literals of core clause as core
              if (core_units) ca[r].core(1);
              if (ca [r].core ())
                {
                  Clause &rc = ca [r];
                  for (int j = 1; j < rc.size (); ++j)
                    {
                      Var x = var (rc [j]);
                      assert(reason(x) != CRef_Undef);
                      ca [reason (x)].core (1);
                    }
                }
            }
          assert (c[0] == trail [trail_sz - 1]);
          // -- unassign the variable
          assigns [var (c[0])] = l_Undef;
          // -- put it back in order heap in case we want to restart
          // -- solving in the future
          insertVarOrder (var (c[0]));
          trail_sz--;
        }
      // -- unit clauses don't need to be detached from watched literals
      if (c.size () > 1) detachClause (cr);
      // -- mark clause deleted
      c.mark (1);
      if (c.core () == 1)
        {
          assert (value (c[0]) == l_Undef);
          // -- put trail in a good state
          trail.shrink (trail.size () - trail_sz);
          qhead = trail.size ();
          if (trail_lim.size () > 0) trail_lim [0] = trail.size ();
          if (verbosity >= 2) printf ("V");
          if (!validateLemma (cr))
          {
              printf("Failed for i=%d and %d...\n", i, proof[i]);
        	  return false;
          }
        }
      else if (verbosity >= 2) printf ("-");


    }
  if (verbosity >= 2) printf ("\n");


  // update trail and qhead
  trail.shrink (trail.size () - trail_sz);
  qhead = trail.size ();
  if (trail_lim.size () > 0) trail_lim [0] = trail.size ();

  // find core clauses in the rest of the trail
  for (int i = trail.size () - 1; i >= 0; --i)
    {
      assert (reason (var (trail [i])) != CRef_Undef);
      Clause &c = ca [reason (var (trail [i]))];
      // -- if c is core, mark all clauses it depends as core
      if (c.core () == 1) {
        for (int j = 1; j < c.size (); ++j)
          {
            Var x = var (c[j]);
            ca[reason (x)].core (1);
          }
        qhead = i;
      }

    }

  // Put units back on the trail
  for (int i=0; i < clauses.size(); i++) {
      Clause& c = ca[clauses[i]];
      if (c.size() == 1)
          enqueue(c[0], clauses[i]);
  }

  watches.cleanAll();
  if (verbosity >= 1) printf ("VALIDATED\n");
  assignParts(); // FS
  return true;
}

bool Solver::validateLemma (CRef cr)
{
  assert (decisionLevel () == 0);
  assert (ok);

  Clause &lemma = ca [cr];
  assert (lemma.core ());
  assert (!locked (lemma));

  // -- go to decision level 1
  newDecisionLevel ();

  for (int i = 0; i < lemma.size (); ++i)
    enqueue (~lemma [i]);

  // -- go to decision level 2
  newDecisionLevel ();
  CRef confl = propagate ();
  if (confl == CRef_Undef)
  {
      // If propagate fails, it may be due to incrementality and missing
      // units. Update qhead and re-propagate the entire trail
      qhead = 0;
      confl = propagate();
      if (confl == CRef_Undef)
      {
          if (verbosity >= 2) printf ("FAILED: No Conflict from propagate()\n");
          cancelUntil(0);
          return false;
      }
  }

  Clause &conflC = ca [confl];
  conflC.core (1);
  for (int i = 0; i < conflC.size (); ++i)
    {
      Var x = var (conflC [i]);
      // -- if the variable got value by propagation,
      // -- mark it to be unrolled
      if (level (x) > 1) seen [x] = 1;
      else if (level (x) <= 0) ca [reason(x)].core (1);
    }

  // mark all level0 literals in the lemma as core
  for (int i = 0; i < lemma.size (); ++i)
    if (value (lemma[i]) != l_Undef && level (var (lemma [i])) <= 0)
      ca[reason (var (lemma [i]))].core (1);

  for (int i = trail.size () - 1; i >= trail_lim[1]; i--)
    {
      Var x = var (trail [i]);
      if (!seen [x]) continue;

      seen [x] = 0;
      assert (reason (x) != CRef_Undef);
      Clause &c = ca [reason (x)];
      c.core (1);

      assert (value (c[0]) == l_True);
      // -- for all other literals in the reason
      for (int j = 1; j < c.size (); ++j)
        {
          Var y = var (c [j]);
          assert (value (c [j]) == l_False);

          // -- if the literal is assigned at level 2,
          // -- mark it for processing
          if (level (y) > 1) seen [y] = 1;
          // -- else if the literal is assigned at level 0,
          // -- mark its reason clause as core
          else if (level (y) <= 0)
            // -- mark the reason for y as core
            ca[reason (y)].core (1);
        }
    }
  // reset
  cancelUntil (0);
  ok = true;
  return true;
}

void Solver::replay (ProofVisitor& v, vec<CRef>* pOldProof)
{
  assert (log_proof);
  assert (proof.size () > 0);
  if (verbosity >= 2) printf ("REPLAYING: ");

  // -- enter ordered propagate mode
  scopped_ordered_propagate scp_propagate (*this, true);

  CRef confl = propagate (true);
  // -- assume that initial clause database is consistent 
  assert (confl == CRef_Undef); // FS: Does anything break without this? Should disappear once we use assumption-based solving.

  labelLevel0(v);

  vec<CRef> newProof;
  bool bConflict = false;
  for (int i = 0; i < proof.size(); ++i)
    {
      if (proof[i] == 0) break;
      if (verbosity >= 2) fflush (stdout);

      CRef cr = proof [i];
      assert (cr != CRef_Undef);
      Clause &c = ca [cr];

      // -- delete clause that was deleted before
      // -- except for locked and core clauses
      if (c.mark() == 0)
      {
    	  if (c.core() && c.learnt())
    	  {
    		  // If it is core, we do not delete it.
    		  continue;
    	  }

    	  // If it is not core, and it is either not locked or satisfied, we remove it
          if (!locked(c) || satisfied(c))
          {
            if (c.size () > 1) detachClause (cr);
            c.mark (1);
            watches.cleanAll();
            if (verbosity >= 2) printf ("-");
            continue;
          }

    	  if (locked(c))
    	  {
    		  assert(c.core());
    		  continue;
    	  }

    	  assert(false);
      }
      else // c.mark() == 1
      {
    	  // The clause is deleted now

    	  // If it is not core, it remains deleted, we do not care about it.
    	  if (!c.core())
    		  continue;

    	  // It cannot be locked?
    	  if (locked(c))
    		  assert(false);

    	  // If it is satisfied, we do not care about it.
    	  if (satisfied(c)) {
    		  continue;
    	  }
      }

      if (verbosity >= 2) printf ("v");

      assert (c.mark() == 1);

      // -- at least one literal must be undefined
      if (value (c[0]) != l_Undef)
        for (int j = 1; j < c.size(); j++)
          if (value(c[j]) == l_Undef)
          {
            Lit tmp = c[0];
            c[0] = c[j];
            c[j] = tmp;
            break;
          }

      assert (value (c[0]) == l_Undef);

      // XXX Can decide and propagate separately, like in the
      // XXX SAT-solver
      // XXX This might help with restructuring the proof
      // XXX (i.e., by asserting and propagating in a certain order), and
      // XXX with making the algorithm more efficient by deciding when
      // XXX there is no need to restructure because it will not yield
      // XXX shared leaves.
      newDecisionLevel (); // decision level 1
      for (int j = 0; j < c.size (); ++j)
        enqueue (~c[j]);


      newDecisionLevel (); // decision level 2
      CRef p = propagate (true);
      if (p == CRef_Undef)
      {
          printf ("BCP Failed at: %d out of: %d. Failure is: %d\n", i, proof.size(), cr);
    	  CRef p = propagate ();
    	  if (p != CRef_Undef) printf("GREAT SUCCESS!\n");
      }
      assert (p != CRef_Undef);

      // -- proof, extract interpolants, etc.
      // -- trail at decision level 0 is implied by the database
      // -- trail at decision level 1 are the decision forced by the clause
      // -- trail at decision level 2 is derived from level 1

      // -- undelete the clause and attach it to the database
      // -- unless the learned clause is already in the database
      bool bRes = true;
      vec<Lit> learnt;
      Range range;
      int part = ca[p].part().max();

      if (reorder_proof == false) part = totalPart.max();

      while (part <= totalPart.max())
      {
        learnt.clear();
        range.reset();
        bRes = traverse(v, cr, p, part, learnt, range);

        // XXX The handling of the case when bRes==false,
        // XXX and the clause cr is irrelevant is inefficient.
        // XXX In this case, the loop continues until part > totalPart.max ()

        // XXX If bRes==false, then nothing was learned. But, even in
        // XXX that case, the conflict still contains literals at
        // XXX level 2.  A better way to do this is to always look at what
        // XXX level the literals of the conflict are and start from
        // XXX that level. Right now, this check is done on 'learnt'
        // XXX clause, but that is only because the learnt clause is
        // XXX treated as a conflict in the next iteration of the loop.

        // nothing learned, move to next partition
        if (bRes == false)
        {
          part++;
          continue;
        }

#if DNDEBUG
        for (int i=0; i < learnt.size(); i++)
          assert(value(learnt[i]) == l_False);
#endif

        // initialize nextPart to impossibly large partition
        int nextPart = totalPart.max() + 1;
        for (int idx=0; idx < learnt.size(); idx++)
        {
          if (level(var (learnt[idx])) == 2)
          {
            int vPart = ca[reason(var (learnt[idx]))].part().max();
            if (vPart < nextPart) nextPart = vPart;
          }
        }
        part = nextPart;

        if (clausesAreEqual(cr, learnt) == false) {
          // -- allocate new learnt clause
          LitOrderLt lt(vardata, assigns);
          sort(learnt, lt);
          p = ca.alloc(learnt, true);
          ca[p].part (range);
          learnts.push(p);

		      newProof.push(p);

          if (learnt.size() > 1)
            attachClause(p);

          ca[p].core(1);
          ca[p].mark(0);

          // IS THIS ALWAYS TRUE?
          // I think that with reordering this is not necessarily true
          ca[cr].core(0);

          // In case that the last conflict is being reduced
          // update it
          if (cr == confl_assumps)
              confl_assumps = p;
          v.visitChainResolvent(p);
        }
        else {
            // In case the clauses are identical, we are done
            ca[cr].part(range);
            ca[cr].mark(0);
            v.visitChainResolvent(cr);
            if (ca[cr].size() > 1) attachClause(cr);
            newProof.push(cr);
            break;
        }

        if (nextPart > totalPart.max())
        {
          ca[cr].core(0);
          cr = p;
          if (newProof.last() != cr) newProof.push(cr);
          break;
        }
      }

      if (bRes)
      {
        cancelUntil (0);
        ca[cr].mark (0);
        if (newProof.last() != cr) {
        	newProof.push(cr);
        	assert(false);
        }
        // Cannot reach this part with the clauses containing
        // conflicting literals
        assert (confl_assumps == CRef_Undef || i < proof.size()-1);
        if (verbosity >= 2 && shared (cr)) printf ("S");
        // -- if unit clause, add to trail and propagate
        if (ca[cr].size () <= 1 || value (ca[cr][1]) == l_False)
        {
          assert (value (ca[cr][0]) == l_Undef);
#ifndef NDEBUG
          for (int j = 1; j < ca[cr].size (); ++j)
            assert (value (ca[cr][j]) == l_False);
#endif
          uncheckedEnqueue (ca[cr][0], cr);
          confl = propagate (true);
          labelLevel0(v);
          // -- if got a conflict at level 0, bail out
          if (confl != CRef_Undef)
          {
            labelFinal(v, confl);
            break;
          }
        }
      }
      else
      {
        // ca[cr] was deduced without propagate. This means it is a
        // duplicate of an already active clause.
        assert (ca[cr].core ());
        assert (ca[cr].mark ());
        // -- mark this clause as non-core. It is not part of the
        // -- proof.
        ca[cr].mark(1);
        ca[cr].core (0);
        cancelUntil (0);

        // FS
        if (cr == confl_assumps) {
          CRef previous_cr = proof[i-1];
          if (!ca[previous_cr].core () || !clausesAreEqual(confl_assumps, previous_cr)) {
            throw std::invalid_argument("Error handling duplicate clause during replay.");
          }
          assert (ca[previous_cr].core ());
          assert(clausesAreEqual(confl_assumps, previous_cr));
          confl_assumps = previous_cr;
        }
      }
    }

  if (proof.size () == 1) labelFinal (v, proof [0]);
  else if (conflict.size() > 0) {
      if (conflict.size() == 1) {
          //assert(value(ca[confl_assumps][0]) == l_False);
          labelFinal(v, reason(var(ca[confl_assumps][0])));
      }
      else
          labelFinal(v, confl_assumps);
  }
  if (verbosity >= 2)
    {
      printf ("\n");
      fflush (stdout);
    }

  // Make sure learnts and clauses contain all needed clauses
    for (int i=0; i < learnts.size(); i++)
        ca[learnts[i]].reloced(1);
    for (int i=0; i < clauses.size(); i++)
        ca[clauses[i]].reloced(1);
    for (int i=0; i < trail.size(); i++) {
          CRef cr = reason(var(trail[i]));
          if (cr != CRef_Undef) {
              Clause& c = ca[cr];
              if (c.mark() == 0 && c.reloced() == 0)
                  (c.learnt() ? learnts : clauses).push(cr), c.reloced(1);
              if (c.learnt()) assert(c.core());
          }
    }

  // Make sure all new proof clauses are core and activated
    for (int i=0; i < newProof.size(); i++) {
        Clause& c = ca[newProof[i]];
        assert(c.core() == 1 && c.mark() == 0);
        if (c.reloced() == 0)
            (c.learnt() ? learnts : clauses).push(newProof[i]);
    }

  // Clean mark
    for (int i=0; i < learnts.size(); i++)
        ca[learnts[i]].reloced(0);
    for (int i=0; i < clauses.size(); i++)
        ca[clauses[i]].reloced(0);

  if (verbosity >= 1 && confl != CRef_Undef) printf ("Replay SUCCESS\n");

  // Make sure that all clauses not in the new proof are deleted
    for (int i=0; i < newProof.size(); i++)
        ca[newProof[i]].reloced(1);
    for (int i=0; i < proof.size()-1; i++) {
        CRef cr = proof[i];
        Clause& c = ca[cr];
        if (c.reloced() == 0) {
            if (c.learnt()) {
            assert (c.mark() == 1);
            assert(c.core() == 0 || satisfied(c));
        }
        else
            assert(false);
      }
      ca[proof[i]].core(0);
    }
  // Clean mark
    for (int i=0; i < newProof.size(); i++)
        ca[newProof[i]].reloced(0);
        // Remove all learnt clauses that are deleted
          int i,j;
          for (i=0, j=0; i < learnts.size(); i++)
              if (ca[learnts[i]].mark() == 0)
                  learnts[j++] = learnts[i];
    learnts.shrink(i-j);

    if (pOldProof != NULL)
        proof.moveTo(*pOldProof);
    else
        proof.clear();

    newProof.copyTo(proof);

  // Clean the rest of the clauses
    for (int i=0; i < clauses.size(); i++) {
      ca[clauses[i]].core(0);
      assert(ca[clauses[i]].mark() == 0);
    }
    for (int i=0; i < learnts.size(); i++) {
      ca[learnts[i]].core(0);
      assert(ca[learnts[i]].mark() == 0);
    }
}

void Solver::labelFinal(ProofVisitor& v, CRef confl)
{
    // The conflict clause is the clause with which we resolve.
    const Clause& source = ca[confl];

    v.chainClauses.clear();
    v.chainPivots.clear();

    v.chainClauses.push(confl);
    // The clause is false, and results in the empty clause,
    // all are therefore seen and resolved.
    for (int i = 0; i < source.size (); ++i)
    {
      v.chainPivots.push(~source [i]);
      v.chainClauses.push(CRef_Undef);
    }
    v.visitChainResolvent(CRef_Undef);
}

// XXX AG: would be cleaner if traverse returns chainClauses and
// XXX chainPivots instead of taking a ProofVisitor object
bool Solver::traverse(ProofVisitor& v, CRef proofClause, 
                      CRef confl, int part, vec<Lit>& out_learnt, Range& range)
{
  vec<char> mySeen(nVars(), 0);
  vec<char> learntSeen(nVars(), 0);
  int pathC = 0;
  int pathZero = 0;

  // -- If p is not  Undef, then the pivot remains p, thus must be the first
  // -- literal in the new clause.
  Lit p = value (ca[confl][0]) == l_True ? ca[confl][0] : lit_Undef;

  if (p != lit_Undef) out_learnt.push(p);
  // Generate conflict clause:
  //
  int index   = trail.size() - 1;

  vec<Lit> chainPivots;
  vec<CRef> chainClauses;

  do{
    assert(confl != CRef_Undef); // (otherwise should be UIP)

    if (reorder_proof && ca[confl].part ().max () < part)
    {
      //assert(p != lit_Undef); // Cannot be entered in the first iteration
      // fixrec checks whether confl needs fixing. If it does, it
      // will create a new clause, set it as a reason and return it.
      // XXX AG: 3rd argument seems unnecessary since fixrec can figure it out on its own.
      confl = fixrec (v, confl, ca[confl].part ().max ());
    }

    // the partition of the learned clause can be computed as the join
    // of partitions of all chainClauses and chainPivots.
    chainClauses.push(confl);
    if (p != lit_Undef && chainClauses.size() > 1) chainPivots.push (p);

    Clause& c = ca[confl];

    range.join(c.part());

    assert (c.part ().max () <= part);

    for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
      Lit q = c[j];

      if (!mySeen[var(q)]){
        // -- don't resolve with clauses from higher partitions
        CRef r = reason(var(q));
        if (r != CRef_Undef && ca[r].part().max() <= part)
        {
          // ensure that reason (var (q)) is in correct partition
          if (level(var(q)) > 1 || level (var (q)) <= 0)
          {
            mySeen[var(q)] = 1;
            pathC++;
          }
          else
            if (!learntSeen[var(q)]) learntSeen[var(q)] = 1 , out_learnt.push(q);
        }
        else
          if (!learntSeen[var(q)]) learntSeen[var(q)] = 1 , out_learnt.push(q);
      }
    }
    if (pathC == 0) break;

    // Select next clause to look at:
    while (!mySeen[var(trail[index--])]);
    p     = trail[index+1];
    confl = reason(var(p));
    mySeen[var(p)] = 0;
    pathC--;

  }while (pathC >= 0);

  v.chainClauses.clear();
  v.chainPivots.clear();

  chainClauses.moveTo(v.chainClauses);
  chainPivots.moveTo(v.chainPivots);

  if (v.chainClauses.size () <= 1) return false;
  return true;

}

// XXX needs a better name than 'fixrec'
CRef Solver::fixrec(ProofVisitor& v, CRef anchor, int part)
{
  // -- Need to check if traverse should be called or not.

  CRef resolvent = anchor;
  vec<Lit> learnt;
  Range range;
  bool bRes = traverse(v, CRef_Undef, anchor, part, learnt, range);

  if (bRes == false) return anchor;

  assert(range.max() <= part);

  if (clausesAreEqual(anchor, learnt) == false)
  {
    LitOrderLt lt(vardata, assigns);
    sort(learnt, lt);
    // -- Create the new clause.
    if (value(learnt[0]) == l_True)
    {
#if DNDEBUG
      for (int i=1; i < learnt.size()-1; i++)
        assert(level(var(learnt[i])) >= level(var(learnt[i+1])));
#endif

      resolvent = ca.alloc(learnt, true);
      ca[resolvent].part (range);
      learnts.push(resolvent);
      if (learnt.size() > 1) attachClause(resolvent);

      vardata[var(learnt[0])].reason = resolvent;
    }
    else
    {
      // XXX is this possible?
#if DNDEBUG
      for (int i=0; i < learnt.size(); i++)
        assert(value(learnt[i]) == l_False);
#endif
      resolvent = ca.alloc(learnt, true);
      ca[resolvent].part (range);
      learnts.push(resolvent);
      if (learnt.size() > 1) attachClause(resolvent);
    }

    Clause& c = ca[resolvent];
    c.core(1);
    c.mark(0);
    c.part(range);

    v.visitChainResolvent(resolvent);
  }

	return resolvent;
}

void Solver::labelLevel0(ProofVisitor& v)
{
    // -- Walk the trail forward
  while (start < trail.size ())
  {
    Lit q = trail [start++];
    Var x = var(q);
    if (reason(x) == CRef_Undef) {
    	// This must be an assumption
    	assert(level(x) > 0);
    	continue;
    }
    assert(reason(x) != CRef_Undef);
    Clause& c = ca [reason(x)];
    Range r = c.part ();
    if (c.size () == 1) trail_part [x] = r;

    // -- The number of resolution steps at this point is size-1
    // -- where size is the number of literals in the reason clause
    // -- for the unit that is currently on the trail.
    else if (c.size () == 2)
    {
      r.join (trail_part [var(c[1])]);
      trail_part [x] = r;
      v.visitResolvent(q, ~c[1], reason(x)); // -- Binary resolution
    }
    else
    {
      v.chainClauses.clear ();
      v.chainPivots.clear ();

      v.chainClauses.push (reason(x));
      // -- The first literal (0) is the result of resolution, start from 1.
      for (int j = 1; j < c.size (); j++)
      {
        r.join (trail_part[var(c[j])]);
        v.chainPivots.push (~c[j]);
        v.chainClauses.push(CRef_Undef);
      }
      trail_part [x] = r;
      v.visitChainResolvent (q);
    }
  }
}

//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//
Var Solver::newVar(bool sign, bool dvar)
{
    int v = nVars();
    watches  .init(mkLit(v, false));
    watches  .init(mkLit(v, true ));
    assigns  .push(l_Undef);
    vardata  .push(mkVarData(CRef_Undef, 0));
    //activity .push(0);
    activity .push(rnd_init_act ? drand(random_seed) * 0.00001 : 0);
    seen     .push(0);
    polarity .push(sign);
    decision .push();
    trail    .capacity(v+1);
    setDecisionVar(v, dvar);

    partInfo.push(Range ());
    trail_part.push (Range ());

    return v;
}


bool Solver::addClause_(vec<Lit>& ps, Range part)
{
    assert(decisionLevel() == 0);
    assert (!log_proof || !part.undef ());

    if (!ok) return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p; int i, j;
    if (log_proof)
      {
        // -- remove duplicates
        for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
          if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
          else if (ps[i] != p)
            ps[j++] = p = ps[i];
        ps.shrink(i - j);

        // -- move false literals to the end
        int sz = ps.size ();
        for (i = 0; i < sz; ++i)
          {
            if (value (ps [i]) == l_False)
              {
                // -- push current literal to the end
                Lit l = ps[i];
                ps[i] = ps[sz-1];
                ps[sz-1] = l;
                sz--;
                i--;
              }
          }
      }
    else
      {
        // -- AG: original minisat code
        for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
          if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
          else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
        ps.shrink(i - j);
      }

    if (ps.size() == 0)
        return ok = false;
    else if (log_proof && value (ps[0]) == l_False)
      {
        // -- this is the conflict clause, log it as the last clause in the proof
        CRef cr = ca.alloc (ps, false);
        Clause &c = ca[cr];
        c.part ().join (part);
        proof.push (cr);
        for (int i = 0; part.singleton () && i < ps.size (); ++i)
          partInfo [var (ps[i])].join (part);
        return ok = false;
      }
    else if (ps.size() == 1 || (log_proof && value (ps[1]) == l_False)){
      if (log_proof)
        {
          CRef cr = ca.alloc (ps, false);
          Clause &c = ca[cr];
          c.part ().join (part);
          clauses.push (cr);
          totalPart.join (part);
          uncheckedEnqueue (ps[0], cr);
          if (ps.size() > 1) attachClause(cr);
        }
      else
        uncheckedEnqueue(ps[0]);

      // -- mark variables as shared if necessary
      /* for (int i = 0; part.singleton () && i < ps.size (); ++i)
        partInfo [var (ps[i])].join (part); */

      CRef confl = propagate ();
      if (log_proof && confl != CRef_Undef) proof.push (confl);
      return ok = (confl == CRef_Undef);
    }else{
        CRef cr = ca.alloc(ps, false);
        Clause& c = ca[cr];
        c.part ().join (part);
        clauses.push(cr);
        totalPart.join (part);
        attachClause(cr);

        /* for (i = 0; part.singleton () && i < ps.size(); i++)
           partInfo[var (ps[i])].join (part); */

    }

    return true;
}


void Solver::attachClause(CRef cr) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);
    watches[~c[0]].push(Watcher(cr, c[1]));
    watches[~c[1]].push(Watcher(cr, c[0]));
    if (c.learnt()) learnts_literals += c.size();
    else            clauses_literals += c.size(); }


void Solver::detachClause(CRef cr, bool strict) {
    const Clause& c = ca[cr];
    assert(c.size() > 1);

    if (strict){
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
    }else{
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
    }

    if (c.learnt()) learnts_literals -= c.size();
    else            clauses_literals -= c.size(); }


void Solver::removeClause(CRef cr) {
    if (log_proof) proof.push (cr);
    Clause& c = ca[cr];
    if (c.size () > 1) detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(c) && !log_proof) vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1);
    if (!log_proof) ca.free(cr);
}


bool Solver::satisfied(const Clause& c) const {
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false; }


// Revert to the state at given level (keeping all assignment at 'level' but not beyond).
//
void Solver::cancelUntil(int level) {
    if (decisionLevel() > level){
        for (int c = trail.size()-1; c >= trail_lim[level]; c--){
            Var      x  = var(trail[c]);
            assigns [x] = l_Undef;
            if (phase_saving > 1 || (phase_saving == 1) && c > trail_lim.last())
                polarity[x] = sign(trail[c]);
            insertVarOrder(x); }
        qhead = trail_lim[level];
        trail.shrink(trail.size() - trail_lim[level]);
        trail_lim.shrink(trail_lim.size() - level);
    } }


//=================================================================================================
// Major methods:


Lit Solver::pickBranchLit()
{
    Var next = var_Undef;

    // Random decision:
    if (drand(random_seed) < random_var_freq && !order_heap.empty()){
        next = order_heap[irand(random_seed,order_heap.size())];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++; }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap.empty()){
            next = var_Undef;
            break;
        }else
            next = order_heap.removeMin();

    return next == var_Undef ? lit_Undef : mkLit(next, rnd_pol ? drand(random_seed) < 0.5 : polarity[next]);
}


/*_________________________________________________________________________________________________
|
|  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
|
|  Description:
|    Analyze conflict and produce a reason clause.
|
|    Pre-conditions:
|      * 'out_learnt' is assumed to be cleared.
|      * Current decision level must be greater than root level.
|
|    Post-conditions:
|      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
|      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the
|        rest of literals. There may be others from the same level though.
|
|________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel,
                     Range &part)
{
    int pathC = 0;
    Lit p     = lit_Undef;

    // Generate conflict clause:
    //
    out_learnt.push();      // (leave room for the asserting literal)
    int index   = trail.size() - 1;

    if (log_proof) part = ca [confl].part ();

    do{
        assert(confl != CRef_Undef); // (otherwise should be UIP)
        Clause& c = ca[confl];

        if (log_proof) part.join (c.part ());

        if (c.learnt())
            claBumpActivity(c);

        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++){
            Lit q = c[j];

            if (!seen[var(q)]){
              if (level(var(q)) > 0)
                {
                  varBumpActivity(var(q));
                  seen[var(q)] = 1;
                  if (level(var(q)) >= decisionLevel())
                    pathC++;
                  else
                    out_learnt.push(q);
                }
              else if (log_proof)
                {
                  assert (!trail_part[var (q)].undef ());
                  // update part based on partition of var(q)
                  part.join (trail_part [var (q)]);
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p     = trail[index+1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;

    }while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    if (ccmin_mode == 2){
        uint32_t abstract_level = 0;
        for (i = 1; i < out_learnt.size(); i++)
            abstract_level |= abstractLevel(var(out_learnt[i])); // (maintain an abstraction of levels involved in conflict)

        for (i = j = 1; i < out_learnt.size(); i++)
          if (reason(var(out_learnt[i])) == CRef_Undef || !litRedundant(out_learnt[i], abstract_level, part))
                out_learnt[j++] = out_learnt[i];

    }else if (ccmin_mode == 1){
      assert (!log_proof);
        for (i = j = 1; i < out_learnt.size(); i++){
            Var x = var(out_learnt[i]);

            if (reason(x) == CRef_Undef)
                out_learnt[j++] = out_learnt[i];
            else{
                Clause& c = ca[reason(var(out_learnt[i]))];
                for (int k = 1; k < c.size(); k++)
                    if (!seen[var(c[k])] && level(var(c[k])) > 0){
                        out_learnt[j++] = out_learnt[i];
                        break; }
            }
        }
    }else
        i = j = out_learnt.size();

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else{
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int i = 2; i < out_learnt.size(); i++)
            if (level(var(out_learnt[i])) > level(var(out_learnt[max_i])))
                max_i = i;
        // Swap-in this literal at index 1:
        Lit p             = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1]     = p;
        out_btlevel       = level(var(p));
    }

    for (int j = 0; j < analyze_toclear.size(); j++) seen[var(analyze_toclear[j])] = 0;    // ('seen[]' is now cleared)
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the algorithm is
// visiting literals at levels that cannot be removed later.
bool Solver::litRedundant(Lit p, uint32_t abstract_levels, Range &part)
{
    analyze_stack.clear(); analyze_stack.push(p);
    // -- partition of all clauses used in the derivation to replace p
    Range lPart;
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0){
        assert(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause& c = ca[reason(var(analyze_stack.last()))]; analyze_stack.pop();
        if (log_proof) lPart.join (c.part ());

        for (int i = 1; i < c.size(); i++){
            Lit p  = c[i];
            if (!seen[var(p)]){
              if (level (var (p)) > 0)
              {
                if (reason(var(p)) != CRef_Undef && (abstractLevel(var(p)) & abstract_levels) != 0){
                    seen[var(p)] = 1;
                    analyze_stack.push(p);
                    analyze_toclear.push(p);
                }else{
                    for (int j = top; j < analyze_toclear.size(); j++)
                        seen[var(analyze_toclear[j])] = 0;
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
              }
              else if (log_proof)
              {
                assert (!trail_part[var (p)].undef ());
                // update part based on partition of var(q)
                lPart.join (trail_part [var (p)]);
              }
            }
        }
    }

    if (log_proof) part.join (lPart);

    return true;
}


/*_________________________________________________________________________________________________
|
|  analyzeFinal : (p : Lit)  ->  [void]
|
|  Description:
|    Specialized analysis procedure to express the final conflict in terms of assumptions.
|    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
|    stores the result in 'out_conflict'.
|________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict, Range& part)
{
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    if (log_proof) {
        part.join(ca[reason(var(p))].part().max());
        part.join(ca[reason(var(p))].part());
    }
    seen[var(p)] = 1;

    for (int i = trail.size()-1; i >= trail_lim[0]; i--){
        Var x = var(trail[i]);
        if (seen[x]){
            if (reason(x) == CRef_Undef){
                assert(level(x) > 0);
                out_conflict.push(~trail[i]);
            }else{
                Clause& c = ca[reason(x)];
                if (log_proof) part.join (c.part ());
                for (int j = 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
                    else if (log_proof)
                        part.join(trail_part[var(c[j])]);
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}


void Solver::uncheckedEnqueue(Lit p, CRef from)
{
    assert(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);

    // -- everything at level 0 has a reason
    assert (!log_proof || decisionLevel () != 0 || from != CRef_Undef);

    if (log_proof && decisionLevel () == 0)
      {
        Clause &c = ca[from];
        Var x = var (p);

        assert (!c.part ().undef ());

        trail_part [x] = c.part ();
        for (int i = 1; i < c.size (); ++i)
          trail_part [x].join (trail_part[var (c[i])]);
      }

}


/*_________________________________________________________________________________________________
|
|  propagate : [void]  ->  [Clause*]
|
|  Description:
|    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
|    otherwise CRef_Undef.
|
|    Post-conditions:
|      * the propagation queue is empty, even if there was a conflict.
|________________________________________________________________________________________________@*/
CRef Solver::propagate(bool coreOnly, int maxPart)
{

    CRef    confl     = CRef_Undef;

    // -- in ordered propagate mode, keep applying propagate with increasing partitions
    if (ordered_propagate && maxPart == 0)
    {
      int init_qhead = qhead;
      for (int i = 1; confl == CRef_Undef && i <= totalPart.max (); i++)
      {
        // -- reset qhead to the beginning
        qhead = init_qhead;
        // -- propagate up to (and including) partition i
        confl = propagate (coreOnly, i);
      }
      return confl;
    }

    int     num_props = 0;
    watches.cleanAll();
    while (qhead < trail.size()){
        Lit            p   = trail[qhead++];     // 'p' is enqueued fact to propagate.
        vec<Watcher>&  ws  = watches[p];
        Watcher        *i, *j, *end;
        num_props++;

        //sort (ws, WatcherLt(ca));
        for (i = j = (Watcher*)ws, end = i + ws.size();  i != end;){
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (value(blocker) == l_True){
                *j++ = *i++; continue; }

            CRef     cr        = i->cref;
            Clause&  c         = ca[cr];

            if (coreOnly && !c.core ()) { *j++ = *i++; continue; }
            if (maxPart > 0 && maxPart < c.part ().max ()) { *j++ = *i++; continue; }

            // Make sure the false literal is data[1]:
            Lit      false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            assert(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit     first = c[0];
            Watcher w     = Watcher(cr, first);
            if (first != blocker && value(first) == l_True){
                *j++ = w; continue; }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++)
                if (value(c[k]) != l_False){
                    c[1] = c[k]; c[k] = false_lit;
                    watches[~c[1]].push(w);
                    goto NextClause; }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;
            if (value(first) == l_False){
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            }else
                uncheckedEnqueue(first, cr);

        NextClause:;
        }
        ws.shrink(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}


/*_________________________________________________________________________________________________
|
|  reduceDB : ()  ->  [void]
|
|  Description:
|    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
|    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
|________________________________________________________________________________________________@*/
struct reduceDB_lt {
    ClauseAllocator& ca;
    reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) {
        return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
};
void Solver::reduceDB()
{
    int     i, j;
    double  extra_lim = cla_inc / learnts.size();    // Remove any clause below this activity

    sort(learnts, reduceDB_lt(ca));
    // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
    // and clauses with activity smaller than 'extra_lim':
    for (i = j = 0; i < learnts.size(); i++){
        Clause& c = ca[learnts[i]];
        if (c.size() > 2 && !locked(c) && (i < learnts.size() / 2 || c.activity() < extra_lim))
            removeClause(learnts[i]);
        else
            learnts[j++] = learnts[i];
    }
    learnts.shrink(i - j);
    checkGarbage();
}


void Solver::removeSatisfied(vec<CRef>& cs)
{
    int i, j;
    for (i = j = 0; i < cs.size(); i++){
        Clause& c = ca[cs[i]];
        if (c.size() > 1 && satisfied(c))
            removeClause(cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}


void Solver::rebuildOrderHeap()
{
    vec<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}


/*_________________________________________________________________________________________________
|
|  simplify : [void]  ->  [bool]
|
|  Description:
|    Simplify the clause database according to the current top-level assigment. Currently, the only
|    thing done here is the removal of satisfied clauses, but more things can be put here.
|________________________________________________________________________________________________@*/
bool Solver::simplify()
{
    assert(decisionLevel() == 0);

    if (!ok) return false;

    CRef confl = propagate ();
    if (confl != CRef_Undef)
    {
      if (log_proof) proof.push (confl);
      return ok = false;
    }

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
#if DNDEBUG
    for (int i=0; i < proof.size(); i++)
        //TODO This should be removed, it is taken care of in Replay
        //TODO but for now leaving it just to make sure
    	if (satisfied(ca[proof[i]]) == true) {
    		if (ca[proof[i]].mark() == 1 && ca[proof[i]].learnt() == true)
    		{
    			bool b = false;
    			for (int j=0; j < proof.size() && !b; j++)
    				if (j != i && proof[j] == proof[i])
    					b = true;

    			if (!b)
    			{
    				for (int j=i+1; j < proof.size(); j++)
    					proof[j-1] = proof[j];
    				proof.shrink(1);
    			}
    			assert(b);
    		}
    	}
#endif

    if (remove_satisfied)        // Can be turned off.
        removeSatisfied(clauses);
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    simpDB_props   = clauses_literals + learnts_literals;   // (shouldn't depend on stats really, but it will do for now)

    return true;
}


/*_________________________________________________________________________________________________
|
|  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
|
|  Description:
|    Search for a model the specified number of conflicts.
|    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
|
|  Output:
|    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
|    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
|    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
|________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts)
{
    assert(ok);
    int         backtrack_level;
    int         conflictC = 0;
    vec<Lit>    learnt_clause;
    Range       part;
    starts++;

    for (;;){
        CRef confl = propagate();

        if (confl != CRef_Undef){

            conflicts++; conflictC++;
            if (decisionLevel() == 0)
              {
                if (log_proof) proof.push (confl);
                return l_False;
              }

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level, part);
            cancelUntil(backtrack_level);

            if (learnt_clause.size() == 1){
              if (log_proof)
                {
                  // Need to log learned unit clauses in the proof
                  CRef cr = ca.alloc (learnt_clause, true);
                  proof.push (cr);
                  ca[cr].part (part);
                  uncheckedEnqueue (learnt_clause [0], cr);
                }
              else
                uncheckedEnqueue(learnt_clause[0]);
            }else{
                CRef cr = ca.alloc(learnt_clause, true);
                if (log_proof) proof.push (cr);
                if (log_proof) ca[cr].part (part);
                learnts.push(cr);
                attachClause(cr);
                claBumpActivity(ca[cr]);
                uncheckedEnqueue(learnt_clause[0], cr);
            }

            varDecayActivity();
            claDecayActivity();

            if (--learntsize_adjust_cnt == 0){
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt    = (int)learntsize_adjust_confl;
                max_learnts             *= learntsize_inc;

                if (verbosity >= 1)
                    printf("| %9d | %7d %8d %8d | %8d %8d %6.0f | %6.3f %% |\n",
                           (int)conflicts,
                           (int)dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]), nClauses(), (int)clauses_literals,
                           (int)max_learnts, nLearnts(), (double)learnts_literals/nLearnts(), progressEstimate()*100);
            }

        }else{
            // NO CONFLICT
            if (nof_conflicts >= 0 && conflictC >= nof_conflicts || !withinBudget()){
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef; }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify())
                return l_False;

            if (learnts.size()-nAssigns() >= max_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()){
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True){
                    // Dummy decision level:
                    newDecisionLevel();
                }else if (value(p) == l_False){
                	part.reset();
                    analyzeFinal(~p, conflict, part);
                    if (log_proof) {
                    	vec<Lit> c;
                    	for (int i=0; i < conflict.size(); i++) c.push(conflict[i]);
                    	confl_assumps = ca.alloc(c, true);
                    	proof.push(confl_assumps);
                    	ca[confl_assumps].part(part);
                    }
                    return l_False;
                }else{
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef){
                // New variable decision:
                decisions++;
                next = pickBranchLit();

                if (next == lit_Undef)
                    // Model found:
                    return l_True;
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}

void Solver::assignParts() {
  resetParts();
  for (int i = 0; i < clauses.size(); i++){
    Clause& c = ca[clauses[i]];
    if (c.core()) {
      for (int j = 0; j < c.size(); j++) {
        partInfo[var (c[j])].join (c.part());
      }
    }
  }
}

void Solver::resetParts() {
  for (Var v = 0; v < nVars(); v++) {
    partInfo[v] = Range();
  }
}


double Solver::progressEstimate() const
{
    double  progress = 0;
    double  F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++){
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

/*
  Finite subsequences of the Luby-sequence:

  0: 1
  1: 1 1 2
  2: 1 1 2 1 1 2 4
  3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
  ...


 */

static double luby(double y, int x){

    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for (size = 1, seq = 0; size < x+1; seq++, size = 2*size+1);

    while (size-1 != x){
        size = (size-1)>>1;
        seq--;
        x = x % size;
    }

    return pow(y, seq);
}

// NOTE: assumptions passed in member-variable 'assumptions'.
lbool Solver::solve_()
{
    model.clear();
    conflict.clear();

    if (confl_assumps != CRef_Undef)
    {
    	if (proof.size() > 0 && proof.last() == 0) proof.pop();
    	confl_assumps = CRef_Undef;
    	start = 0;
    }
    if (!ok) return l_False;

    solves++;

    max_learnts               = nClauses() * learntsize_factor;
    learntsize_adjust_confl   = learntsize_adjust_start_confl;
    learntsize_adjust_cnt     = (int)learntsize_adjust_confl;
    lbool   status            = l_Undef;

    if (verbosity >= 1){
        printf("============================[ Search Statistics ]==============================\n");
        printf("| Conflicts |          ORIGINAL         |          LEARNT          | Progress |\n");
        printf("|           |    Vars  Clauses Literals |    Limit  Clauses Lit/Cl |          |\n");
        printf("===============================================================================\n");
    }

    // Search:
    int curr_restarts = 0;
    while (status == l_Undef){
        double rest_base = luby_restart ? luby(restart_inc, curr_restarts) : pow(restart_inc, curr_restarts);
        status = search(rest_base * restart_first);
        if (!withinBudget()) break;
        curr_restarts++;
    }

    if (verbosity >= 1)
        printf("===============================================================================\n");


    if (status == l_True){
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
    }else if (status == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);
    return status;
}

//=================================================================================================
// Writing CNF to DIMACS:
//
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max)
{
    if (map.size() <= x || map[x] == -1){
        map.growTo(x+1, -1);
        map[x] = max++;
    }
    return map[x];
}


void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max)
{
    //if (satisfied(c)) return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ", sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max)+1);
    fprintf(f, "0\n");
}


void Solver::toDimacs(const char *file, const vec<Lit>& assumps)
{
    FILE* f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}


void Solver::toDimacs(FILE* f, const vec<Lit>& assumps)
{
    // Handle case when solver is in contradictory state:
    if (!ok){
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return; }

    vec<Var> map; Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        //if (!satisfied(ca[clauses[i]]))
            cnt++;

    for (int i = 0; i < clauses.size(); i++)
        if (true) {//!satisfied(ca[clauses[i]])){
            Clause& c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumps.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumps.size(); i++){
        assert(value(assumps[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumps[i]) ? "-" : "", mapVar(var(assumps[i]), map, max)+1);
    }

    assert(totalPart.min() == 1);
	for (int part = 1; part <= totalPart.max(); part++)
	{
		fprintf(f, "c partition\n");
		for (int i = 0; i < clauses.size(); i++)
			if (part == ca[clauses[i]].part().max())
				toDimacs(f, ca[clauses[i]], map, max);
	}

    if (verbosity > 0)
        printf("Wrote %d clauses with %d variables.\n", cnt, max);
}


//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to)
{
    // All watchers:
    //
    // for (int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++){
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher>& ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
        }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++){
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef && (ca[reason(v)].reloced() || locked(ca[reason(v)])))
            ca.reloc(vardata[v].reason, to);
    }

    // All learnt:
    //
    for (int i = 0; i < learnts.size(); i++)
        ca.reloc(learnts[i], to);

    // All original:
    //
    for (int i = 0; i < clauses.size(); i++)
        ca.reloc(clauses[i], to);

    // Clausal proof:
    //
    for (int i = 0; i < proof.size (); i++)
      ca.reloc (proof[i], to);
}




void Solver::garbageCollect()
{
    //assert (!log_proof);
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted());

    relocAll(to);
    if (verbosity >= 2)
        printf("|  Garbage collection:   %12d bytes => %12d bytes             |\n",
               ca.size()*ClauseAllocator::Unit_Size, to.size()*ClauseAllocator::Unit_Size);
    to.moveTo(ca);
}

bool Solver::clausesAreEqual(CRef first, CRef second) const
{
  const Clause& first_clause = ca[first];
  const Clause& second_clause = ca[second];
  if (first_clause.size() == second_clause.size())
  {
    vec<Lit> sortedFirstCopy (first_clause.size ());
    for (int i = 0; i < first_clause.size (); ++i)
      sortedFirstCopy[i] = first_clause[i];
    sort(sortedFirstCopy);

    vec<Lit> sortedSecondCopy (second_clause.size ());
    for (int i = 0; i < second_clause.size (); ++i)
      sortedSecondCopy[i] = second_clause[i];
    sort(sortedSecondCopy);

    for (int i=0; i < first_clause.size(); i++)
      if (sortedFirstCopy[i] != sortedSecondCopy[i])
        return false;
  } else {
    return false;
  }

  return true;
}

bool Solver::clauseSubsumes(CRef first, CRef second) const {
  const Clause& first_clause = ca[first];
  const Clause& second_clause = ca[second];
  if (first_clause.size() <= second_clause.size())
  {
    vec<Lit> sortedFirstCopy (first_clause.size ());
    for (int i = 0; i < first_clause.size (); ++i)
      sortedFirstCopy[i] = first_clause[i];
    sort(sortedFirstCopy);

    vec<Lit> sortedSecondCopy (second_clause.size ());
    for (int i = 0; i < second_clause.size (); ++i)
      sortedSecondCopy[i] = second_clause[i];
    sort(sortedSecondCopy);

    for (int i=0; i < first_clause.size(); i++)
      if (sortedFirstCopy[i] != sortedSecondCopy[i])
        return false;
  } else {
    return false;
  }

  return true;
}

bool Solver::clausesAreEqual(CRef orig, const vec<Lit>& lits) const
{
  const Clause& original = ca[orig];
  if (lits.size() == original.size())
  {
    vec<Lit> sortedOriginalCopy (original.size ());
    for (int i = 0; i < original.size (); ++i)
      sortedOriginalCopy [i] = original [i];
    sort(sortedOriginalCopy);

    vec<Lit> sortedLearntCopy;
    lits.copyTo(sortedLearntCopy);
    sort(sortedLearntCopy);

    for (int i=0; i < lits.size(); i++)
      if (sortedOriginalCopy[i] != sortedLearntCopy[i])
        return false;
  }
  else return false;

  return true;
}
