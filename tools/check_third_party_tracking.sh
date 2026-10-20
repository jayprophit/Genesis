#!/bin/sh
# Refuse to commit third-party source that is not ours to ship.
#
# The Aetherius-OS provenance guard proves the same thing, but it is a
# test and a test only runs when something runs it. A blanket
# `git add -A` re-staged all forty-seven files, and the commit went out
# before the test was ever consulted.
#
# This hook closes that gap. It runs on every commit, and it refuses the
# commit rather than reporting the problem afterwards.
#
# The rule: anything under references/ is third-party reference material
# and is never tracked, EXCEPT the trees named in OWNER_REFERENCE_PATHS
# below, which are the owner's own work kept as a reference.
#
# The exception is a named path, not a relaxation. The owner confirmed
# that the OmniAgent Runtime and Workspace tree is their own AI Studio
# application, written during vibe coding and exported from AI Studio,
# and it is kept in the repository because it is a reference for the
# builds. The guard still refuses every other unlicensed tree under
# references/.
#
# references/ is deliberately NOT gitignored, so this check is what stands
# between a tree and a careless add.

set -e

repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

# The reference trees that are the owner's own work and may be tracked.
OWNER_REFERENCE_PATHS="references/omniagent-runtime-&-workspace/"

# Files under references/ that are already tracked are left alone: the
# repository's history is not this hook's business.
newly_added=$(git diff --cached --name-only --diff-filter=A -- references/ 2>/dev/null || true)

# Split the staged additions into the ones that are the owner's own and
# the ones that are not.
not_ours=""
for path in $newly_added; do
    allowed=0
    for prefix in $OWNER_REFERENCE_PATHS; do
        case "$path" in
            "$prefix"*) allowed=1 ;;
        esac
    done
    if [ "$allowed" -eq 0 ]; then
        not_ours="$not_ours
$path"
    fi
done

if [ -n "$not_ours" ]; then
    count=$(printf '%s\n' "$not_ours" | grep -c . || true)
    echo ""
    echo "REFUSED: $count file(s) under references/ are staged for commit."
    echo ""
    echo "references/ holds third-party reference material that is not ours to"
    echo "ship. It stays on disk and stays untracked. Unstage it with:"
    echo ""
    printf '%s\n' "$not_ours" | grep . | head -20 | sed 's/^/  git rm --cached -r -- "/; s/$/"/'
    echo ""
    echo "If a file under references/ genuinely is first-party, move it out"
    echo "of references/ first, or name it in OWNER_REFERENCE_PATHS in"
    echo "tools/check_third_party_tracking.sh with the reason. Do not weaken"
    echo "this check."
    echo ""
    exit 1
fi

# A second rule, looser but still worth having: a new file that is a
# dependency lockfile or a vendored third-party tree, by name.
case "$(git diff --cached --name-only --diff-filter=A 2>/dev/null || true)" in
    *"/vendor/"*|*"node_modules/"*)
        echo ""
        echo "REFUSED: a vendored or node_modules path is staged for commit."
        echo ""
        exit 1
        ;;
esac

exit 0
