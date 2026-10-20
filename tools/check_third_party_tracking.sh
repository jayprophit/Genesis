#!/bin/sh
# Refuse to commit third-party source that is not ours to ship.
#
# The Aetherius-OS provenance guard proves that the unlicensed OmniAgent
# reference tree is not tracked. That guard is a test, and a test only runs
# when something runs it. A blanket `git add -A` re-staged all forty-seven
# files, and the commit went out before the test was ever consulted.
#
# This hook closes that gap. It runs on every commit, and it refuses the
# commit rather than reporting the problem afterwards.
#
# The rule: anything under references/ is third-party reference material.
# It stays on disk so the guard can inspect it, and it is never tracked.
# references/ is deliberately NOT gitignored, so this check is what stands
# between the tree and a careless add.

set -e

repo_root=$(git rev-parse --show-toplevel)
cd "$repo_root"

# Files under references/ that are already tracked are left alone: the
# repository's history is not this hook's business.
newly_added=$(git diff --cached --name-only --diff-filter=A -- references/ 2>/dev/null || true)

if [ -n "$newly_added" ]; then
    count=$(printf '%s\n' "$newly_added" | grep -c . || true)
    echo ""
    echo "REFUSED: $count file(s) under references/ are staged for commit."
    echo ""
    echo "references/ holds third-party reference material that is not ours to"
    echo "ship. It stays on disk and stays untracked. Unstage it with:"
    echo ""
    printf '  git rm --cached -r -- "%s"\n' "$newly_added" | head -20
    echo ""
    echo "If a file under references/ genuinely is first-party, move it out"
    echo "of references/ first. Do not weaken this check."
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
