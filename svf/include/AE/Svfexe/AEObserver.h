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

class CallICFGNode;
class ICFGNode;

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
/// visits during widening and narrowing. Observers must not mutate the
/// running analysis.
class AEObserver
{
public:
    virtual ~AEObserver() = default;

    /// Observe a reachable node immediately before its transfer.
    virtual void onNodeVisit(const ICFGNode*)
    {
    }

    /// Observe an assertion result.
    virtual void onCheckpoint(const CallICFGNode*, AECheckpointKind,
                              AECheckpointOutcome)
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
