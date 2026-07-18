//===- AEObserver.h -- Abstract Execution observation API ---------------//
//
//                     SVF: Static Value-Flow Analysis
//
// Copyright (C) <2013->  <Yulei Sui>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//===----------------------------------------------------------------------===//

#pragma once

namespace SVF
{

class AbstractState;
class CallICFGNode;
class ICFGNode;

/// Location of an observed state relative to an ICFG node's transfer.
enum class AEStatePoint
{
    Before,
    After
};

/// Kind of source-level checkpoint handled by abstract execution.
enum class AECheckpointKind
{
    Assert,
    AssertEqual
};

/// Result of evaluating a checkpoint.
enum class AECheckpointOutcome
{
    Proved,
    Candidate,
    Unreached
};

/// Result of classifying a reached external call in AbsExtAPI.
enum class AEExternalCallOutcome
{
    Modeled,
    Unmodeled
};

/// Controls whether assertion failures terminate AE.  FailFast preserves the
/// legacy behavior: candidates of either kind and an unreached svf_assert are
/// fatal, while a historically untracked, unreached svf_assert_eq is not.
/// Continue makes all assertion outcomes nonfatal.
enum class AECheckpointFailurePolicy
{
    FailFast,
    Continue
};

/// Optional, non-owning observation interface for abstract execution.
///
/// Node callbacks are emitted for every analysis visit, including repeated
/// visits during widening and narrowing.  Consumers that need a final trace
/// should copy the latest state for each node.  State references are borrowed
/// and valid only for the duration of the callback.
///
/// An observer may query the AbstractInterpretation that owns it during a
/// callback.  This is useful for materializing values whose authoritative
/// storage is at a sparse definition site.  Observers must not mutate the
/// running analysis.
class AEObserver
{
public:
    virtual ~AEObserver() = default;

    /// Observe a reachable node immediately before or after its transfer.
    virtual void onNodeState(const ICFGNode*, AEStatePoint,
                             const AbstractState&)
    {
    }

    /// Observe an assertion result.  `state` is the reached checkpoint's
    /// current state for Proved/Candidate and is null for Unreached.
    virtual void onCheckpoint(const CallICFGNode*, AECheckpointKind,
                              AECheckpointOutcome, const AbstractState*)
    {
    }

    /// Observe a reached external call after model classification and before
    /// its registered/annotated model or unknown-call fallback is applied.
    /// Emitted once per reached visit, including repeated fixpoint visits.
    virtual void onExternalCall(const CallICFGNode*, AEExternalCallOutcome)
    {
    }
};

} // namespace SVF
