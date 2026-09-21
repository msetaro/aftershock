#!/usr/bin/env bash
# Immutable, repository-local CI builds. Never create or move a rolling tag.
set -euo pipefail
export GH_HOST=github.com
[[ ${GITHUB_REPOSITORY:-} == msetaro/aftershock ]] || { echo 'Unexpected publication repository' >&2; exit 1; }
[[ ${GITHUB_SHA:-} =~ ^[0-9a-f]{40}$ ]] || { echo 'Expected a full commit SHA' >&2; exit 1; }
[[ $# -gt 0 ]] || { echo 'No build archives supplied' >&2; exit 1; }
for archive in "$@"; do
    [[ -f $archive ]] || { echo "Missing build archive: $archive" >&2; exit 1; }
done

tag=build-$GITHUB_SHA
repo=$GITHUB_REPOSITORY
# Matching-refs returns an empty list for a missing tag; API errors still fail.
ref=$(gh api "repos/$repo/git/matching-refs/tags/$tag" \
    --jq "map(select(.ref == \"refs/tags/$tag\")) | .[].object.sha")
[[ -z $ref || $ref == "$GITHUB_SHA" ]] || { echo 'Existing build tag points at a different object; refusing to move it' >&2; exit 1; }
if gh release view "$tag" --repo "$repo" >/dev/null 2>&1; then
    [[ $ref == "$GITHUB_SHA" ]] || { echo 'Release has no matching immutable tag' >&2; exit 1; }
    gh release upload "$tag" --repo "$repo" --clobber "$@"
else
    gh release create "$tag" --repo "$repo" --target "$GITHUB_SHA" \
        --title "Build $GITHUB_SHA" --prerelease --latest=false \
        --notes "Automated main build at $GITHUB_SHA. Archives contain engine binaries; game content is not included." "$@"
fi
