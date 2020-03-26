/*
 * Copyright (c) 2000, 2012, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#ifndef SHARE_VM_MEMORY_SPACE_INLINE_HPP
#define SHARE_VM_MEMORY_SPACE_INLINE_HPP

#include "gc_interface/collectedHeap.hpp"
#include "memory/space.hpp"
#include "memory/universe.hpp"
#include "runtime/prefetch.inline.hpp"
#include "runtime/safepoint.hpp"

inline HeapWord* Space::block_start(const void* p) {
  return block_start_const(p);
}

#define SCAN_AND_FORWARD(cp,scan_limit,block_is_obj,block_size) {            \
  /* Compute the new addresses for the live objects and store it in the mark \
   * Used by universe::mark_sweep_phase2()                                   \
   */                                                                        \
\
/*//cgmin*/ \
  _cog_array->clear();\
  unsigned long i;\
  for (i=0;i<_mapMax;i++)\
    _pnMap[i] = 0;\
  /*\
  groupStart->clear();\
  groupSize->clear();\
  groupNum = 0;\
  */\
  /*\
  cp->space->set_compaction_top( );
  cp->space->compaction_top();
  co->space->end();
  */\
\
  unsigned long space_left,align_waste;\
  HeapWord* fail_addr=0;\
  HeapWord* space_limit;\
  HeapWord* compact_end;\
  HeapWord* q2;\
  COG cog;\
  unsigned long diff;\
  bool opt;\
  opt = true;\
\
\
  HeapWord* compact_top; /* This is where we are currently compacting to. */ \
                                                                             \
  /* We're sure to be here before any objects are compacted into this        \
   * space, so this is a good time to initialize this:                       \
   */                                                                        \
  set_compaction_top(bottom());                                              \
                                                                             \
  if (cp->space == NULL) {                                                   \
    assert(cp->gen != NULL, "need a generation");                            \
    assert(cp->threshold == NULL, "just checking");                          \
    assert(cp->gen->first_compaction_space() == this, "just checking");      \
    cp->space = cp->gen->first_compaction_space();                           \
    compact_top = cp->space->bottom();                                       \
    cp->space->set_compaction_top(compact_top);                              \
    cp->threshold = cp->space->initialize_threshold();                       \
  } else {                                                                   \
    compact_top = cp->space->compaction_top();                               \
  }                                                                          \
                                                                             \
  /* We allow some amount of garbage towards the bottom of the space, so     \
   * we don't start compacting before there is a significant gain to be made.\
   * Occasionally, we want to ensure a full compaction, which is determined  \
   * by the MarkSweepAlwaysCompactCount parameter.                           \
   */                                                                        \
  uint invocations = MarkSweep::total_invocations();                         \
  bool skip_dead = ((invocations % MarkSweepAlwaysCompactCount) != 0);       \
                                                                             \
  size_t allowed_deadspace = 0;                                              \
  if (skip_dead) {                                                           \
    const size_t ratio = allowed_dead_ratio();                               \
    allowed_deadspace = (capacity() * ratio / 100) / HeapWordSize;           \
  }                                                                          \
                                                                             \
  HeapWord* q = bottom();                                                    \
  HeapWord* t = scan_limit();                                                \
                                                                             \
  HeapWord*  end_of_live= q;    /* One byte beyond the last byte of the last \
                                   live object. */                           \
  HeapWord*  first_dead = end();/* The first dead object. */                 \
  LiveRange* liveRange  = NULL; /* The current live range, recorded in the   \
                                   first header of preceding free area. */   \
  _first_dead = first_dead;                                                  \
                                                                             \
  const intx interval = PrefetchScanIntervalInBytes;                         \
                                                                             \
  compact_end = cp->space->end();\
  \
  while (q < t) {                                                            \
    assert(!block_is_obj(q) ||                                               \
           oop(q)->mark()->is_marked() || oop(q)->mark()->is_unlocked() ||   \
           oop(q)->mark()->has_bias_pattern(),                               \
           "these are the only valid states during a mark sweep");           \
    if ((block_is_obj(q) && oop(q)->is_gc_marked())/* || q < live_addr*/ /*cgmin*/) {                         \
      /* prefetch beyond q */                                                \
      Prefetch::write(q, interval);                                          \
      size_t size = block_size(q);                                           \
      /* //cgmin forward*/ \
      if (/*true ||*/ (unsigned long)q / 4096 == (unsigned long)(q+size) / 4096 || q < fail_addr)\
      {\
        compact_top = cp->space->forward_no_opt(oop(q), size, cp, compact_top,opt);       \
        q += size;                                                             \
        end_of_live = q;                                                       \
      }\
      else\
      {\
        space_left = (compact_end - compact_top) * sizeof(HeapWord);\
        align_waste = (4096+((unsigned long)q % 4096)-(unsigned long)compact_top % 4096) % 4096;\
        if (space_left >= 4096 + align_waste && (align_waste == 0 || align_waste >= CollectedHeap::min_fill_size()*sizeof(HeapWord)))\
        {\
          cog.start = q;\
          cog.end = 0;\
          space_limit = (HeapWord*)((unsigned long)cog.start + space_left - align_waste);\
          while (q < t && oop(q)->is_gc_marked() && (unsigned long)cog.end / 4096 <= (unsigned long)cog.start / 4096 + 10)\
          {\
            size = block_size(q);\
            if (q + size > space_limit)\
              break;\
            if ((unsigned long)(q + size) / 4096 != (unsigned long)q / 4096)\
              cog.end = q + size;\
/*            oop(q)->init_mark();*/\
            q += size;\
          }\
/*          fail_addr = cog.end;*/\
          if (cog.end == 0 || (unsigned long)cog.end / 4096 <= (unsigned long)cog.start / 4096 + 10)\
          {\
          /*\
          q = cog.start;\
          while(q < live_addr)\
          {\
              oop(q)->set_mark(markOopDesc::prototype()->set_marked());\
              size = block_size(q);\
              q+=size;\
          }\
          */\
            q = cog.start;\
            size = block_size(q);\
            compact_top = cp->space->forward_no_opt(oop(q), size, cp, compact_top,opt);\
            q += size;\
            end_of_live = q;\
          }\
          else\
          {\
            cog.align_waste = align_waste;\
            cog.forward = (HeapWord*)((unsigned long)compact_top + align_waste);\
          q = cog.start;\
            if ((unsigned long)cog.start % 4096 != 0)\
            {\
              oop(cog.start)->forward_to(oop(cog.forward));\
              q+=block_size(q);\
              }\
\
\
            while(q < cog.end)\
            {\
              oop(q)->init_mark();\
              q += block_size(q);\
            }\
            \
          \
/*          q2 = cog.end;*/\
          while (q < t && oop(q)->is_gc_marked())\
          {\
            size = block_size(q);\
            if (q + size > space_limit)\
              break;\
            if ((unsigned long)(q + size) / 4096 != (unsigned long)q / 4096)\
            {\
            /*\
              while(q2 < q + size)\
              {\
                oop(q2)->init_mark();\
                q2+=block_size(q2);\
              }\
              */\
              cog.end = q + size;\
              }\
            oop(q)->init_mark();\
            q += size;\
          }\
          q2 = cog.end;\
          while (q2 < q)\
          {\
               oop(q2)->set_mark(markOopDesc::prototype()->set_marked());\
            q2+=block_size(q2);\
          }\
/*          \
q = cog.start;\
if ((unsigned long)q % 4096 != 0)\
  q+=block_size(q);\
while(q<cog.end)\
{\
  oop(q)->init_mark();\
  q+=block_size(q);\
}\
*/          \
            unsigned long start_i,end_i;\
/*            HeapWord* ttt;*/\
            cog.length = (cog.end - cog.start) * sizeof(HeapWord);\
            _cog_array->append(cog);\
/*            printf("cog %p %p %p %lu %lu\n",cog.start,cog.end,cog.forward,cog.length,cog.align_waste);*/\
            \
            q = cog.end;\
            end_of_live = q;\
            compact_top = cp->space->occupy((align_waste+cog.length)/sizeof(HeapWord),cp,compact_top);\
            /*\
            cp->space->set_compaction_top(compact_top+(align_waste+cog.length)/sizeof(HeapWord));\
            compact_top+=(align_waste+cog.length)/sizeof(HeapWord);\
            */\
            /*\
            q = cog.start;\
            ttt = cog.forward;\
            while (q < cog.end)\
            {\
              if (oop(q)->is_gc_marked() == false)\
                printf("mark error\n ");\
              oop(q)->forward_to(ttt);\
              size = block_size(q);\
              ttt+=size;\
              q+=size;\
            }\
            */\
            \
            start_i = ((unsigned long)cog.start + 4096 -1 - _b4)/4096;\
            end_i = ((unsigned long)cog.end - _b4)/4096;\
            if (cog.start > cog.forward)\
            {\
              diff = cog.start - cog.forward;\
              for (i = start_i; i < end_i; i++)\
              {\
                _pnMap[i] = 2;\
                _dvMap[i] = diff;\
              }\
            }\
            else if (cog.start < cog.forward)\
            {\
              diff = cog.forward - cog.start;\
              for (i=start_i;i<end_i;i++)\
              {\
                _pnMap[i] = 1;\
                _dvMap[i] = diff;\
              }\
            }\
            else\
            {\
              for (i=start_i;i<end_i;i++)\
                _pnMap[i] = 3;\
            }\
          }\
          fail_addr = cog.end;\
        }\
        else\
        {\
/*             q = cog.start;*/\
/*            size = block_size(q);*/\
            compact_top = cp->space->forward_no_opt(oop(q), size, cp, compact_top,opt);\
            q += size;\
            end_of_live = q;\
        }\
      }\
      \
    } else {                                                                 \
      /* run over all the contiguous dead objects */                         \
      HeapWord* end = q;                                                     \
      do {                                                                   \
        /* prefetch beyond end */                                            \
        Prefetch::write(end, interval);                                      \
        end += block_size(end);                                              \
      } while (end < t && (!block_is_obj(end) || !oop(end)->is_gc_marked()));\
                                                                             \
      /* see if we might want to pretend this object is alive so that        \
       * we don't have to compact quite as often.                            \
       */                                                                    \
      if (allowed_deadspace > 0 && q == compact_top) {                       \
        size_t sz = pointer_delta(end, q);                                   \
        if (insert_deadspace(allowed_deadspace, q, sz)) {                    \
          compact_top = cp->space->forward_no_opt(oop(q), sz, cp, compact_top,opt);     \
          q = end;                                                           \
          end_of_live = end;                                                 \
          continue;                                                          \
        }                                                                    \
      }                                                                      \
                                                                             \
      /* otherwise, it really is a free region. */                           \
                                                                             \
      /* for the previous LiveRange, record the end of the live objects. */  \
      if (liveRange) {                                                       \
        liveRange->set_end(q);                                               \
      }                                                                      \
                                                                             \
      /* record the current LiveRange object.                                \
       * liveRange->start() is overlaid on the mark word.                    \
       */                                                                    \
      liveRange = (LiveRange*)q;                                             \
      liveRange->set_start(end);                                             \
      liveRange->set_end(end);                                               \
                                                                             \
      /* see if this is the first dead region. */                            \
      if (q < first_dead) {                                                  \
        first_dead = q;                                                      \
      }                                                                      \
                                                                             \
      /* move on to the next object */                                       \
      q = end;                                                               \
      opt = false; /*cgmin*/\
    }                                                                        \
  }                                                                          \
                                                                             \
  assert(q == t, "just checking");                                           \
  if (liveRange != NULL) {                                                   \
    liveRange->set_end(q);                                                   \
  }                                                                          \
  _end_of_live = end_of_live;                                                \
  if (end_of_live < first_dead) {                                            \
    first_dead = end_of_live;                                                \
  }                                                                          \
  _first_dead = first_dead;                                                  \
                                                                             \
  /* save the compaction_top of the compaction space. */                     \
  cp->space->set_compaction_top(compact_top);                                \
\
/*\
         GrowableArrayIterator<COG> it = _cog_array->begin();\
        while (it != _cog_array->end())\
        {\
          cog = *it;\
          printf("cog %p %p %p %lu %lu\n",cog.start,cog.end,cog.forward,cog.length,cog.align_waste);\
          if (oop(cog.end)->is_gc_marked() == false && oop(cog.end)->mark()->decode_pointer() == NULL)\
            printf("Null false\n");\
          ++it;\
        }\ 
        */\
}

#define SCAN_AND_ADJUST_POINTERS(adjust_obj_size) {                             \
  /* adjust all the interior pointers to point at the new locations of objects  \
   * Used by MarkSweep::mark_sweep_phase3() */                                  \
                                                                                \
  HeapWord* q = bottom();                                                       \
  HeapWord* t = _end_of_live;  /* Established by "prepare_for_compaction". */   \
                                                                                \
  assert(_first_dead <= _end_of_live, "Stands to reason, no?");                 \
                                                                                \
  if (q < t && _first_dead > q &&                                               \
      (!oop(q)->is_gc_marked() || _pnMap[((unsigned long)q-_b4)/4096] == 3)) {                                                \
    /* we have a chunk of the space which hasn't moved and we've                \
     * reinitialized the mark word during the previous pass, so we can't        \
     * use is_gc_marked for the traversal. */                                   \
    HeapWord* end = _first_dead;                                                \
                                                                                \
    while (q < end) {                                                           \
      /* I originally tried to conjoin "block_start(q) == q" to the             \
       * assertion below, but that doesn't work, because you can't              \
       * accurately traverse previous objects to get to the current one         \
       * after their pointers have been                                         \
       * updated, until the actual compaction is done.  dld, 4/00 */            \
      assert(block_is_obj(q),                                                   \
             "should be at block boundaries, and should be looking at objs");   \
                                                                                \
      /* point all the oops to the new location */                              \
      size_t size = oop(q)->adjust_pointers();                                  \
      size = adjust_obj_size(size);                                             \
                                                                                \
      q += size;                                                                \
    }                                                                           \
                                                                                \
    if (_first_dead == t) {                                                     \
      q = t;                                                                    \
    } else {                                                                    \
      /* $$$ This is funky.  Using this to read the previously written          \
       * LiveRange.  See also use below. */                                     \
      q = (HeapWord*)oop(_first_dead)->mark()->decode_pointer();                \
    }                                                                           \
  }                                                                             \
                                                                                \
  const intx interval = PrefetchScanIntervalInBytes;                            \
                                                                                \
  debug_only(HeapWord* prev_q = NULL);                                          \
  while (q < t) {                                                               \
    /* prefetch beyond q */                                                     \
    Prefetch::write(q, interval);                                               \
    if (oop(q)->is_gc_marked() || _pnMap[((unsigned long)q-_b4)/4096] != 0 /* //cgmin */) {                                               \
      /* q is alive */                                                          \
      /* point all the oops to the new location */                              \
      size_t size = oop(q)->adjust_pointers();                                  \
      size = adjust_obj_size(size);                                             \
      debug_only(prev_q = q);                                                   \
      q += size;                                                                \
    } else {                                                                    \
      /* q is not a live object, so its mark should point at the next           \
       * live object */                                                         \
      debug_only(prev_q = q);                                                   \
      q = (HeapWord*) oop(q)->mark()->decode_pointer();                         \
      /*\
      if (q <= prev_q)\
      {\
        GrowableArrayIterator<COG> it = _cog_array->begin();\
        COG cog;\
        while (it != _cog_array->end())\
        {\
          cog = *it;\
          printf("cog %p %p %p %lu %lu\n",cog.start,cog.end,cog.forward,cog.length,cog.align_waste);\
          if (oop(cog.start)->is_gc_marked() == false)\
            printf("false\n");\
          ++it;\
        }\
        printf("assert %p %p\n",q,prev_q);\
        }\
        */\
      assert(q > prev_q, "we should be moving forward through memory");         \
    }                                                                           \
  }                                                                             \
                                                                                \
  assert(q == t, "just checking");                                              \
}

#define SCAN_AND_COMPACT(obj_size) {                                            \
/*printf("c0\n");*/\
  /* Copy all live objects to their new location                                \
   * Used by MarkSweep::mark_sweep_phase4() */                                  \
\
/*unsigned long sum1,sum2;*/\
/*sum1 = sum2 = 0;*/\
COG cog;\
  GrowableArrayIterator<COG> it = _cog_array->begin();\
  HeapWord* forward;\
  size_t size;\
  if (_cog_array->length() == 0)\
    cog.start = 0;\
  else\
    cog = *it;\
                                                                               \
  HeapWord*       q = bottom();                                                 \
  HeapWord* const t = _end_of_live;                                             \
  debug_only(HeapWord* prev_q = NULL);                                          \
  if (q < t && _first_dead > q &&                                               \
      (!oop(q)->is_gc_marked() && !(cog.start == q && cog.start != cog.forward)/* && _pnMap[((unsigned long)q-_b4)/4096] == 0*/)) {                                                \
    debug_only(                                                                 \
    /* we have a chunk of the space which hasn't moved and we've reinitialized  \
     * the mark word during the previous pass, so we can't use is_gc_marked for \
     * the traversal. */                                                        \
    HeapWord* const end = _first_dead;                                          \
                                                                                \
    while (q < end) {                                                           \
    if (q == cog.start)\
    {\
/*    printf("%p %p %p %p %p\n",bottom(),_first_dead,cog.start,cog.forward,cog.end);*/\
      if (q != cog.forward)\
        printf("error %p\n",cog.forward);\
      q = cog.end;\
      ++it;\
      if (it == _cog_array->end())\
        cog.start = 0;\
      else\
        cog = *it;\
      continue;\
    }\
    \
      size_t size = obj_size(q);                                                \
      assert((!oop(q)->is_gc_marked()/* && _pnMap[((unsigned long)q-_b4)/4096] == 0*/),                                           \
             "should be unmarked (special dense prefix handling)");             \
      debug_only(prev_q = q);                                                   \
      q += size;                                                                \
    }                                                                           \
    )  /* debug_only */                                                         \
                                                                                \
    if (_first_dead == t) {                                                     \
      q = t;                                                                    \
    } else {                                                                    \
      /* $$$ Funky */                                                           \
      q = (HeapWord*) oop(_first_dead)->mark()->decode_pointer();               \
    }                                                                           \
  }                                                                             \
                                                                                \
while(cog.start != 0 && it != _cog_array->end())\
{\
  if (q > cog.start)\
  {\
    ++it;\
    cog = *it;\
  }\
  else\
    break;\
}\
if(cog.start != 0 && it == _cog_array->end())\
  cog.start = 0;\
  const intx scan_interval = PrefetchScanIntervalInBytes;                       \
  const intx copy_interval = PrefetchCopyIntervalInBytes;                       \
  \
/*    printf("c1\n");*/\
  while (q < t) {                                                               \
     /* //cgmin compact */ \
      if (q == cog.start) \
      {\
     /* printf("cog %p %p %p %lu %lu\n",cog.start,cog.end,cog.forward,cog.length,cog.align_waste);*/\
     \
/*     \
            q = cog.start;\
            while (q < cog.end)\
            {\
              if (oop(q)->is_gc_marked() == false)\
                printf("mark error\n ");\
              oop(q)->init_mark();\
              size = block_size(q);\
              q+=size;\
            }\
            \
*/            \
        if (cog.align_waste > 0)\
          CollectedHeap::fill_with_objects(cog.forward-cog.align_waste/sizeof(HeapWord),cog.align_waste/sizeof(HeapWord));\
        oop(cog.start)->init_mark();\
        if (cog.start != cog.forward)\
        {\
          if (false)\
            Copy::aligned_conjoint_words(cog.start,cog.forward,cog.length/sizeof(HeapWord));\
            else\
            {\
          forward = cog.forward;\
          size = (HeapWord*)(((unsigned long)cog.start + 4096 -1)/4096*4096) - cog.start;\
          q = cog.start;\
          if (size > 0)\
          {\
            Copy::aligned_conjoint_words(cog.start,forward,size);\
/*            sum1+=size;*/\
            q+=size;\
            forward+=size;\
          }\
          size = ((unsigned long)cog.end/4096*4096-((unsigned long)cog.start+4096-1)/4096*4096) / sizeof(HeapWord);\
          /*Copy::aligned_conjoint_words(q,forward,size);*/ /*temp*/\
          syscall(333,q,forward,size*sizeof(HeapWord));\
/*          sum2+=size;*/\
          /*syscall*/\
          q = (HeapWord*)((unsigned long)cog.end - (unsigned long)cog.end % 4096);\
          forward+=size;\
          size = (unsigned long)cog.end%4096/sizeof(HeapWord);\
          Copy::aligned_conjoint_words(q,forward,size);\
/*          sum1+=size;*/\
        }\
        }\
        q = cog.end;\
        ++it;\
        if (it == _cog_array->end())\
          cog.start = 0;\
        else\
          cog = *it;\
/*        printf("con\n");*/\
        continue;\
      }\
    if (!oop(q)->is_gc_marked() /*&& _pnMap[((unsigned long)q-_b4)/4096] == 0*/) {                                              \
      /* mark is pointer to next marked oop */                                  \
      debug_only(prev_q = q);                                                   \
      q = (HeapWord*) oop(q)->mark()->decode_pointer();                         \
      assert(q > prev_q, "we should be moving forward through memory");         \
    } else {                                                                    \
      /* prefetch beyond q */                                                   \
      Prefetch::read(q, scan_interval);                                         \
                                                                                \
      /* size and destination */                                                \
      size_t size = obj_size(q);                                                \
      HeapWord* compaction_top = (HeapWord*)oop(q)->forwardee();                \
                                                                                \
      /* prefetch beyond compaction_top */                                      \
      Prefetch::write(compaction_top, copy_interval);                           \
                                                                                \
      /* copy object and reinit its mark */                                     \
/*      assert(q != compaction_top, "everything in this pass should be moving"); */ \
      if (q != compaction_top)\
      {\
        Copy::aligned_conjoint_words(q, compaction_top, size);                    \
/*        sum1+=size;*/\
        }\
      oop(compaction_top)->init_mark();                                         \
      assert(oop(compaction_top)->klass() != NULL, "should have a class");      \
                                                                                \
      debug_only(prev_q = q);                                                   \
      q += size;                                                                \
    }                                                                           \
  }                                                                             \
                                                                                \
  /* Let's remember if we were empty before we did the compaction. */           \
  bool was_empty = used_region().is_empty();                                    \
  /* Reset space after compaction is complete */                                \
  reset_after_compaction();                                                     \
  /* We do this clear, below, since it has overloaded meanings for some */      \
  /* space subtypes.  For example, OffsetTableContigSpace's that were   */      \
  /* compacted into will have had their offset table thresholds updated */      \
  /* continuously, but those that weren't need to have their thresholds */      \
  /* re-initialized.  Also mangles unused area for debugging.           */      \
  if (used_region().is_empty()) {                                               \
    if (!was_empty) clear(SpaceDecorator::Mangle);                              \
  } else {                                                                      \
    if (ZapUnusedHeapArea) mangle_unused_area();                                \
  }                                                                             \
/*  printf("bbb %d\nsss %d\n",sum2,sum1);*/\
/*printf("c2\n");*/\
}

inline HeapWord* OffsetTableContigSpace::allocate(size_t size) {
  HeapWord* res = ContiguousSpace::allocate(size);
  if (res != NULL) {
    _offsets.alloc_block(res, size);
  }
  return res;
}

// Because of the requirement of keeping "_offsets" up to date with the
// allocations, we sequentialize these with a lock.  Therefore, best if
// this is used for larger LAB allocations only.
inline HeapWord* OffsetTableContigSpace::par_allocate(size_t size) {
  MutexLocker x(&_par_alloc_lock);
  // This ought to be just "allocate", because of the lock above, but that
  // ContiguousSpace::allocate asserts that either the allocating thread
  // holds the heap lock or it is the VM thread and we're at a safepoint.
  // The best I (dld) could figure was to put a field in ContiguousSpace
  // meaning "locking at safepoint taken care of", and set/reset that
  // here.  But this will do for now, especially in light of the comment
  // above.  Perhaps in the future some lock-free manner of keeping the
  // coordination.
  HeapWord* res = ContiguousSpace::par_allocate(size);
  if (res != NULL) {
    _offsets.alloc_block(res, size);
  }
  return res;
}

inline HeapWord*
OffsetTableContigSpace::block_start_const(const void* p) const {
  return _offsets.block_start(p);
}

#endif // SHARE_VM_MEMORY_SPACE_INLINE_HPP
